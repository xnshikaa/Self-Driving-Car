// =====================================================================
//  TEST 2 — The BLDC drive motor: direction, brake and the slowest speed
// =====================================================================
//  DO THIS FIRST, WITH THE CAR ON A BOOK AND THE WHEELS IN THE AIR.
//
//  Wiring (12 V geared BLDC motor with a built-in driver):
//     driver +12 V / GND  -> battery through the fuse and the switch
//     driver GND          -> ALSO to an ESP32 GND pin (a shared ground)
//     PWM   -> GPIO25     DIR -> GPIO26     BRAKE -> GPIO27
//     FG (speed pulses)   -> GPIO13, plus a 10 kOhm resistor from GPIO13 to 3V3
//  If your motor is an A2212 with a 30 A ESC instead, set USE_ESC to true:
//     the ESC signal wire -> GPIO25, the ESC ground -> an ESP32 GND pin.
//     NEVER connect the red BEC wire of the ESC to the ESP32 5 V pin unless
//     your guide says so - it can fight the other 5 V supply.
//
//  Type:   p 300   = PWM 300 (of 1023)      d 0 / d 1 = direction
//          b 1 / b 0 = brake on / off       m = find the slowest PWM that moves
//          x       = stop
//  Watch: at which PWM does the wheel just start to turn? Write it down -
//  that number becomes SPEED_FF_PWM_START in config.h.
// =====================================================================
const bool USE_ESC = false;        // true = A2212 + ESC, false = BLDC driver board
const int PIN_PWM = 25;
const int PIN_DIR = 26;
const int PIN_BRAKE = 27;
const int PIN_FG = 13;
const int PWM_MAX = 1023;
volatile unsigned long g_pulses = 0;
int g_pwm = 0;
bool g_brake = true;
int g_dir = HIGH;
void IRAM_ATTR onPulse() { g_pulses = g_pulses + 1; }
// --- PWM helpers that work on ESP32 core 2.x and 3.x ------------------
void pwmSetup(int pin, uint32_t freqHz, uint8_t bits) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, freqHz, bits);
#else
  ledcSetup(0, freqHz, bits);
  ledcAttachPin(pin, 0);
#endif
}
void pwmSet(int pin, uint32_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(0, duty);
#endif
}
void applyPwm(int value) {
  g_pwm = constrain(value, 0, PWM_MAX);
  if (USE_ESC) {
    long us = 1000 + (long)1000 * g_pwm / PWM_MAX;          // 1000 us = stop, 2000 us = full
    pwmSet(PIN_PWM, (uint32_t)(us * 65535L / 20000L));
  } else {
    pwmSet(PIN_PWM, g_pwm);
  }
}
void setBrake(bool on) {
  g_brake = on;
  if (!USE_ESC) digitalWrite(PIN_BRAKE, on ? LOW : HIGH);   // most drivers: LOW = brake
  if (on) applyPwm(0);
}
// Raises the PWM slowly until the FG pulses start: that is the "stiction" point.
void findSlowest() {
  Serial.println(F("Looking for the slowest PWM that still turns the wheel..."));
  setBrake(false);
  for (int pwm = 0; pwm <= 600; pwm += 10) {
    applyPwm(pwm);
    delay(400);
    noInterrupts();
    unsigned long before = g_pulses;
    interrupts();
    delay(500);
    noInterrupts();
    unsigned long after = g_pulses;
    interrupts();
    unsigned long rate = (after - before) * 2;              // pulses per second
    Serial.printf("PWM %4d -> %5lu pulses/s\n", pwm, rate);
    if (rate > 10) {
      Serial.printf("The wheel starts to turn at about PWM %d.\n", pwm);
      Serial.println(F("Write this number down: it is SPEED_FF_PWM_START in config.h."));
      break;
    }
  }
  applyPwm(0);
  setBrake(true);
}
void setup() {
  Serial.begin(115200);
  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, g_dir);
  pinMode(PIN_BRAKE, OUTPUT);
  pinMode(PIN_FG, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FG), onPulse, RISING);
  if (USE_ESC) {
    pwmSetup(PIN_PWM, 50, 16);                              // servo-style pulses
    applyPwm(0);
    Serial.println(F("Arming the ESC - keep the wheels in the air. Wait 3 seconds..."));
    delay(3000);
  } else {
    pwmSetup(PIN_PWM, 20000, 10);                           // 20 kHz, values 0..1023
    applyPwm(0);
  }
  setBrake(true);
  delay(300);
  Serial.println(F("TEST 2: BLDC motor. Commands: p 300 | d 0 | d 1 | b 1 | b 0 | m | x"));
}
void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    char key = line.length() ? tolower(line[0]) : ' ';
    float value = line.substring(1).toFloat();
    if (key == 'p') {
      setBrake(false);
      applyPwm((int)value);
      Serial.printf("PWM = %d\n", g_pwm);
    } else if (key == 'd') {
      g_dir = (value > 0) ? HIGH : LOW;
      digitalWrite(PIN_DIR, g_dir);
      Serial.printf("DIR = %s  (which way does the car roll? that level is MOTOR_DIR_FORWARD)\n",
                    g_dir == HIGH ? "HIGH" : "LOW");
    } else if (key == 'b') {
      setBrake(value > 0);
      Serial.printf("BRAKE = %s\n", g_brake ? "ON" : "OFF");
    } else if (key == 'm') {
      findSlowest();
    } else if (key == 'x') {
      applyPwm(0);
      setBrake(true);
      Serial.println(F("stopped"));
    }
  }
  static unsigned long lastMs = 0;
  static unsigned long lastCount = 0;
  if (millis() - lastMs >= 500) {
    noInterrupts();
    unsigned long count = g_pulses;
    interrupts();
    Serial.printf("PWM %4d  DIR %s  BRAKE %s  FG %5lu pulses/s\n", g_pwm, g_dir == HIGH ? "H" : "L",
                  g_brake ? "ON " : "OFF", (count - lastCount) * 2);
    lastCount = count;
    lastMs = millis();
  }
}
