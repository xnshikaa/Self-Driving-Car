// =====================================================================
//  p04_acc.ino  —  Project 4: Adaptive Cruise Control (ACC) Prototype
// =====================================================================
//  WHAT THIS PROGRAM DOES
//    * measures our own speed from the motor pulses          (100 times/s)
//    * measures the gap to the lead vehicle with an HC-SR04   (20 times/s)
//    * decides a TARGET SPEED: hold the set speed, or keep the gap
//                                                             (50 times/s)
//    * makes the motor hold that target speed with a PID      (100 times/s)
//    * shows everything on the OLED and sends it to the Pi    (20 times/s)
//
//  THE TWO LOOPS (this is the heart of the project)
//    INNER loop, every 10 ms: "drive at 0.45 m/s"  -> motor PWM
//    OUTER loop, every 20 ms: "the lead vehicle is 0.6 m away and moving
//                              at 0.3 m/s"          -> "drive at 0.45 m/s"
//    The outer loop never touches the motor. It only changes the target
//    of the inner loop. This is called CASCADED control and it is exactly
//    how a real car does adaptive cruise control.
//
//  SAFETY
//    * the gap may never fall below Dmin + the distance we need to stop
//    * no ultrasonic echo 3 times in a row      -> FAULT, motor off
//    * battery below 10.0 V                     -> LOW BATTERY, motor off
//    * ACC button off                           -> motor off
//    Always test with the car on a book first, wheels in the air!
// =====================================================================
#include "config.h"
#include "pwm_compat.h"
#include "drive_motor.h"
#include "speed_sensor.h"
#include "ultrasonic_ranger.h"
#include "pid_controller.h"
#include "acc_controller.h"
#include "hmi_input.h"
#include "oled_display.h"
#include "uart_link.h"
#include "battery_monitor.h"
DriveMotor motor;
PIDController speedPid(SPEED_KP, SPEED_KI, SPEED_KD);
GapFilter gapFilter;
AccLogic acc;
Hmi hmi;
Display oled;
BatteryMonitor battery;
CommandReader piReader(Serial2, false);    // the Pi may only send proper $ messages
CommandReader usbReader(Serial, true);     // a person may also type short commands
// ---- what the car is doing right now -------------------------------
float g_speed = 0;            // measured speed (m/s)
float g_targetSpeed = 0;      // what the inner loop must hold (m/s)
float g_pwmCommand = 0;       // the PWM we are sending (kept as a float for the slew limit)
bool  g_holdBrake = true;
long  g_lastPulses = 0;
bool  g_batteryLow = false;
bool  g_usedPrediction = false;
float g_minGapNow = D_MIN_M;
// ---- things the Raspberry Pi can ask for ---------------------------
bool  g_remote = false;             // $M: the Pi drives at a fixed speed (test tools)
bool  g_remoteBlocked = false;      // true while we refuse a remote order (too close)
float g_remoteSpeed = 0;
unsigned long g_remoteMs = 0;
bool  g_remoteHmi = false;          // $G: the Pi replaces the knob and the buttons
float g_remoteSet = 0.5f, g_remoteGap = 0.5f;
bool  g_remoteEngage = false;
unsigned long g_remoteHmiMs = 0;
float g_prediction = 0;             // $P: the AI guess of the lead speed
unsigned long g_predictionMs = 0;
// ---- timers ---------------------------------------------------------
unsigned long g_lastSpeedUs = 0, g_lastGapMs = 0, g_lastTelemetryMs = 0, g_lastScreenMs = 0;
const float METRES_PER_PULSE = PI * WHEEL_DIAMETER_M / PULSES_PER_WHEEL_REV;
// ---------------------------------------------------------------------
void sendEvent(const char *what, float value) {
  char body[80];
  snprintf(body, sizeof(body), "E,%lu,%s,%d,%d,%d", millis(), what,
           (int)lroundf(g_speed * 1000), (int)lroundf(gapFilter.gapM() * 1000),
           (int)lroundf(value * 1000));
  sendFrame(Serial2, body);
  if (DEBUG_PRINT) {
    Serial.print("EVENT ");
    Serial.print(what);
    Serial.print(" speed=");
    Serial.print(g_speed, 2);
    Serial.print(" gap=");
    Serial.print(gapFilter.gapM(), 2);
    Serial.print(" value=");
    Serial.println(value, 2);
  }
}
void printHelp() {
  Serial.println(F("Commands:  e = ACC on/off   s 0.5 = set speed   g 0.4 = gap"));
  Serial.println(F("           v 0.3 = remote drive   p 0.25 = fake AI prediction"));
  Serial.println(F("           x = stop   k kp ki gkp gki = new gains   ? = this help"));
}
void handleCommand(const Command &c) {
  switch (c.type) {
    case 'M':
      g_remote = c.on;
      g_remoteSpeed = c.speedMps;
      g_remoteMs = millis();
      if (!c.on) g_remoteSpeed = 0;
      break;
    case 'G':
      if (c.manual) {                       // typed by a person: change only what was typed
        if (c.setSpeedMps > 0) g_remoteSet = c.setSpeedMps;
        if (c.gapSetM > 0) g_remoteGap = c.gapSetM;
        g_remoteEngage = hmi.engaged();
      } else {
        g_remoteSet = c.setSpeedMps;
        g_remoteGap = c.gapSetM;
        g_remoteEngage = c.engage;
      }
      g_remoteHmi = true;
      g_remoteHmiMs = millis();
      break;
    case 'E':
      hmi.setEngaged(!hmi.engaged());
      g_remoteHmi = false;                  // back to the knob and the buttons
      sendEvent(hmi.engaged() ? "ENGAGE" : "OFF", 0);
      break;
    case 'P':
      g_prediction = c.leadMps;
      g_predictionMs = millis();
      break;
    case 'K':
      speedPid.setGains(c.kp, c.ki, SPEED_KD);
      Serial.print(F("new gains: speed Kp="));
      Serial.print(c.kp);
      Serial.print(F(" Ki="));
      Serial.println(c.ki);
      break;
    case 'S':
      g_remote = false;
      g_remoteSpeed = 0;
      g_remoteEngage = false;
      hmi.setEngaged(false);
      g_targetSpeed = 0;
      g_holdBrake = true;
      motor.setPwm(0);
      motor.setBrake(true);
      sendEvent("STOP", 0);
      break;
    case 'H':
      printHelp();
      break;
    default:
      break;
  }
}
// ---------------------------------------------------------------------
//  INNER LOOP: hold the target speed (every SPEED_LOOP_MS = 10 ms)
// ---------------------------------------------------------------------
void speedLoop(float dt) {
  g_speed = SpeedSensor::update(micros());
  // Tell the Kalman filter EXACTLY how far we moved (counted pulses, not
  // speed x time). Wheel slip and speed noise then cannot fool the gap.
  long pulses = SpeedSensor::pulses();
  float myDistance = (pulses - g_lastPulses) * METRES_PER_PULSE;
  g_lastPulses = pulses;
  gapFilter.predict(myDistance, dt);
  if (g_holdBrake || g_targetSpeed <= 0.0f) {
    speedPid.reset();
    g_pwmCommand = 0;
    motor.setPwm(0);
    motor.setBrake(true);
    return;
  }
  // 1) FEED-FORWARD: the PWM we already know this speed needs (from test t3).
  float feedForward = SPEED_FF_PWM_START + SPEED_FF_PWM_PER_MPS * g_targetSpeed;
  // 2) PID: only has to correct what the feed-forward gets wrong (slope, carpet, battery).
  //    Its limits are set so that feedForward + correction always lands inside 0..PWM_MAX;
  //    that way the I part can never wind up while the motor is already flat out.
  speedPid.setOutputLimits(-feedForward, PWM_MAX - feedForward);
  speedPid.setIntegralBand(SPEED_INTEGRAL_BAND);
  float correction = speedPid.update(g_targetSpeed, g_speed, dt);
  float wanted = constrain(feedForward + correction, 0.0f, (float)PWM_MAX);
  // 3) Rolling faster than we want (going downhill, or the lead braked):
  //    cutting the power is not enough, use the electric brake for a moment.
  if (g_speed > g_targetSpeed + BRAKE_ASSIST_MPS) {
    motor.setBrake(true);
    wanted = 0;
  } else {
    motor.setBrake(false);
  }
  // 4) Slew-rate limit: never jump the PWM by more than PWM_MAX_STEP in one
  //    step. A sudden jump makes the wheels slip and the battery voltage dip.
  g_pwmCommand += constrain(wanted - g_pwmCommand, (float)-PWM_MAX_STEP, (float)PWM_MAX_STEP);
  motor.setPwm((int)lroundf(g_pwmCommand));
}
// ---------------------------------------------------------------------
//  OUTER LOOP: choose the target speed (every GAP_LOOP_MS = 20 ms)
// ---------------------------------------------------------------------
void gapLoop(float dt, unsigned long nowMs) {
  bool remoteHmiFresh = g_remoteHmi && (nowMs - g_remoteHmiMs < REMOTE_TIMEOUT_MS);
  if (g_remote && nowMs - g_remoteMs > REMOTE_TIMEOUT_MS) {     // the Pi stopped talking
    g_remote = false;
    g_remoteSpeed = 0;
    sendEvent("LINKLOST", 0);
  }
  AccInputs in;
  in.nowMs = nowMs;
  in.dt = dt;
  in.engaged = remoteHmiFresh ? g_remoteEngage : hmi.engaged();
  in.setSpeed = remoteHmiFresh ? g_remoteSet : hmi.setSpeed();
  in.gapSet = remoteHmiFresh ? g_remoteGap : hmi.gapSet();
  in.mySpeed = g_speed;
  in.tracking = gapFilter.tracking();
  in.gap = max(0.0f, gapFilter.gapM() - SENSOR_TO_BUMPER_M);
  in.leadSpeed = gapFilter.leadSpeed();
  in.predictionValid = hmi.predictionWanted() && (nowMs - g_predictionMs <= PREDICTION_MAX_AGE_MS);
  in.predictedLeadSpeed = g_prediction;
  in.sensorOk = Ranger::reading().ok;
  in.batteryLow = g_batteryLow;
  if (g_remote) in.engaged = false;            // the ACC logic rests while the Pi drives
  AccOutputs out = acc.step(in);
  g_usedPrediction = out.usedPrediction;
  g_minGapNow = out.minGapNow;
  if (g_remote) {
    // The Pi is driving (a test tool). We still refuse to hit anything.
    bool tooClose = in.tracking && in.gap < out.minGapNow;
    g_targetSpeed = tooClose ? 0.0f : g_remoteSpeed;
    g_holdBrake = tooClose;
    if (tooClose && !g_remoteBlocked) sendEvent("REMOTE_STOP", in.gap);
    g_remoteBlocked = tooClose;
  } else {
    g_targetSpeed = out.targetSpeed;
    g_holdBrake = out.holdBrake;
    g_remoteBlocked = false;
  }
  if (out.modeChanged) {
    sendEvent(MODE_NAMES[acc.mode()], in.gap);
    if (acc.mode() == MODE_EMERGENCY || acc.mode() == MODE_OFF) speedPid.reset();
  }
}
// ---------------------------------------------------------------------
//  Telemetry to the Raspberry Pi (20 times per second)
// ---------------------------------------------------------------------
void sendTelemetry(unsigned long nowMs) {
  const RangeReading &r = Ranger::reading();
  uint8_t flags = 0;
  if (gapFilter.tracking()) flags |= 0x01;
  if (r.ok) flags |= 0x02;
  if (motor.braking()) flags |= 0x04;
  if (g_usedPrediction) flags |= 0x08;
  if (g_remote) flags |= 0x10;
  if (g_batteryLow) flags |= 0x20;
  char body[160];
  snprintf(body, sizeof(body), "T,%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u", nowMs, (int)acc.mode(),
           (int)lroundf(g_speed * 1000), (int)lroundf(g_targetSpeed * 1000),
           (int)lroundf(gapFilter.gapM() * 1000), (int)lroundf(r.filteredM * 1000),
           (int)lroundf(gapFilter.leadSpeed() * 1000),
           (int)lroundf((g_remoteHmi ? g_remoteSet : hmi.setSpeed()) * 1000),
           (int)lroundf((g_remoteHmi ? g_remoteGap : hmi.gapSet()) * 1000), motor.pwm(),
           (int)lroundf(battery.volts() * 1000), flags);
  sendFrame(Serial2, body);
}
// ---------------------------------------------------------------------
//  The OLED dashboard (redrawn 10 times per second, sent one page at a time)
// ---------------------------------------------------------------------
void drawScreen() {
  if (!oled.present()) return;
  char line[24];
  float setSpeed = g_remoteHmi ? g_remoteSet : hmi.setSpeed();
  float gapSet = g_remoteHmi ? g_remoteGap : hmi.gapSet();
  float gap = gapFilter.gapM();
  oled.clearPage(0);
  oled.print(0, 0, g_remote ? "REMOTE" : MODE_NAMES[acc.mode()]);
  snprintf(line, sizeof(line), "SET %.2f", setSpeed);
  oled.print(128 - 8 * 6, 0, line);
  oled.clearPage(1);
  oled.clearPage(2);
  snprintf(line, sizeof(line), "%.2f", g_speed);
  oled.printBig(0, 1, line);
  oled.print(52, 2, "M/S");
  snprintf(line, sizeof(line), "T %.2f", g_targetSpeed);
  oled.print(80, 2, line);
  oled.clearPage(3);
  if (gapFilter.tracking()) snprintf(line, sizeof(line), "GAP %.2f  LEAD %.2f", gap, gapFilter.leadSpeed());
  else snprintf(line, sizeof(line), "GAP ----  NO LEAD");
  oled.print(0, 3, line);
  // A bar 0 ... 1.5 m with a mark where the set gap is.
  oled.clearPage(4);
  oled.clearPage(5);
  oled.bar(0, 4, 128, gapFilter.tracking() ? gap / GAP_MODE_ENTER_M : 0.0f);
  oled.marker(constrain((int)(gapSet / GAP_MODE_ENTER_M * 126) + 1, 0, 127), 5);
  oled.clearPage(6);
  snprintf(line, sizeof(line), "AI %s %.2f", hmi.predictionWanted() ? "ON " : "OFF", g_prediction);
  oled.print(0, 6, line);
  oled.clearPage(7);
  snprintf(line, sizeof(line), "BAT %.1fV  PWM %4d", battery.volts(), motor.pwm());
  oled.print(0, 7, line);
}
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial2.begin(PI_BAUD, SERIAL_8N1, PIN_PI_RX, PIN_PI_TX);
  pinMode(PIN_STATUS_LED, OUTPUT);
  delay(300);
  Serial.println();
  Serial.println(F("=== Project 4: Adaptive Cruise Control ==="));
  motor.begin();
  SpeedSensor::begin();
  Ranger::begin();
  hmi.begin();
  battery.begin();
  bool screen = oled.begin();
  Serial.print(F("OLED: "));
  Serial.println(screen ? F("found") : F("NOT found (check SDA/SCL and the address)"));
  speedPid.setOutputLimits(-PWM_MAX, PWM_MAX);
  speedPid.setIntegralBand(SPEED_INTEGRAL_BAND);
  g_lastPulses = SpeedSensor::pulses();
  g_lastSpeedUs = micros();
  g_lastGapMs = g_lastTelemetryMs = g_lastScreenMs = millis();
  if (DRIVE_TYPE == DRIVE_ESC) Serial.println(F("Arming the ESC: keep the wheels OFF the ground!"));
  printHelp();
}
void loop() {
  unsigned long nowMs = millis();
  unsigned long nowUs = micros();
  // 1) messages from the Pi and from the Serial Monitor
  Command c;
  while (piReader.poll(c)) handleCommand(c);
  while (usbReader.poll(c)) handleCommand(c);
  // 2) the ultrasonic sensor, as often as possible (it decides when to ping)
  if (Ranger::update(nowMs)) {
    const RangeReading &r = Ranger::reading();
    gapFilter.update(r.filteredM, Ranger::lagS(), g_speed);
  }
  // 3) inner loop: 100 times per second
  if (nowUs - g_lastSpeedUs >= SPEED_LOOP_MS * 1000UL) {
    float dt = (nowUs - g_lastSpeedUs) * 1e-6f;
    g_lastSpeedUs = nowUs;
    speedLoop(dt);
  }
  // 4) outer loop: 50 times per second
  if (nowMs - g_lastGapMs >= GAP_LOOP_MS) {
    float dt = (nowMs - g_lastGapMs) * 1e-3f;
    g_lastGapMs = nowMs;
    float volts = battery.update();
    if (volts > BATTERY_PRESENT_V) {                       // ignore this when running on USB only
      if (!g_batteryLow && volts < BATTERY_LOW_V) {
        g_batteryLow = true;
        sendEvent("LOWBAT", volts);
      } else if (g_batteryLow && volts > BATTERY_RESUME_V) {
        g_batteryLow = false;
      }
    }
    gapLoop(dt, nowMs);
  }
  // 5) telemetry and the screen
  if (nowMs - g_lastTelemetryMs >= TELEMETRY_PERIOD_MS) {
    g_lastTelemetryMs = nowMs;
    sendTelemetry(nowMs);
  }
  if (nowMs - g_lastScreenMs >= 100) {
    g_lastScreenMs = nowMs;
    drawScreen();
  }
  oled.service(nowMs);                                     // sends ONE page, about 3 ms
  // 6) the little blue LED: slow = idle, fast = driving, solid = emergency
  int period = (acc.mode() == MODE_EMERGENCY) ? 0 : (g_targetSpeed > 0 ? 200 : 1000);
  digitalWrite(PIN_STATUS_LED, period == 0 ? HIGH : ((nowMs % period) < period / 2));
}
