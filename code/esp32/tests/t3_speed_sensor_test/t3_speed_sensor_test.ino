// =====================================================================
//  TEST 3 — Speed sensor: how many pulses per wheel turn, and PWM vs speed
// =====================================================================
//  Your car has no speedometer. It counts PULSES: the FG wire of the BLDC
//  motor (or an optical slot sensor looking at a striped disc) gives a
//  pulse every time the motor turns a small step.
//      metres per pulse = (pi x wheel diameter) / pulses per wheel turn
//
//  PART A - count the pulses per wheel turn:
//      Type  r  , then turn the drive wheel by hand EXACTLY 10 full turns
//      (mark the tyre with tape), then type  r  again. The sketch divides
//      by 10 and prints PULSES_PER_WHEEL_REV for config.h.
//
//  PART B - PWM vs speed (needs 3 m of clear floor):
//      Type  w 85   to tell the sketch your wheel diameter in mm.
//      Put the car on the floor, hold the USB cable loosely, type  s .
//      The car drives at PWM 150, 250, ... 750 for 2 seconds each and
//      prints the speed at every step, then the straight line that fits
//      them:   PWM = START + PER_MPS x speed
//      Copy START into SPEED_FF_PWM_START and PER_MPS into
//      SPEED_FF_PWM_PER_MPS in config.h. This is the FEED-FORWARD: the
//      PWM the car already knows it needs, so the PID has almost nothing
//      left to correct.
//  Other commands:  p 300 = fixed PWM,  x = stop
// =====================================================================
const bool USE_ESC = false;
const int PIN_PWM = 25;
const int PIN_DIR = 26;
const int PIN_BRAKE = 27;
const int PIN_FG = 13;
const int PWM_MAX = 1023;
volatile long g_count = 0;
volatile unsigned long g_lastPulseUs = 0;
const unsigned long MIN_PULSE_GAP_US = 300;
float g_wheelMm = 85.0;
float g_pulsesPerRev = 180.0;
long g_markCount = 0;
bool g_marked = false;
void IRAM_ATTR onPulse() {
  unsigned long now = micros();
  if (now - g_lastPulseUs < MIN_PULSE_GAP_US) return;   // electrical noise
  g_count = g_count + 1;
  g_lastPulseUs = now;
}
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
  value = constrain(value, 0, PWM_MAX);
  if (USE_ESC) pwmSet(PIN_PWM, (uint32_t)((1000L + 1000L * value / PWM_MAX) * 65535L / 20000L));
  else pwmSet(PIN_PWM, value);
  digitalWrite(PIN_BRAKE, value > 0 ? HIGH : LOW);
}
long pulses() {
  noInterrupts();
  long c = g_count;
  interrupts();
  return c;
}
float metresPerPulse() { return PI * (g_wheelMm / 1000.0f) / g_pulsesPerRev; }
void sweep() {
  Serial.println(F("PWM sweep. Make sure there are 3 metres of clear floor!"));
  delay(1500);
  const int STEPS[] = {150, 250, 350, 450, 550, 650, 750};
  const int N = sizeof(STEPS) / sizeof(STEPS[0]);
  float speeds[N];
  Serial.println(F("pwm, speed_mps, pulses"));
  for (int i = 0; i < N; i++) {
    applyPwm(STEPS[i]);
    delay(1200);                                  // let the speed settle
    long before = pulses();
    unsigned long t0 = millis();
    delay(1000);
    long after = pulses();
    float seconds = (millis() - t0) / 1000.0f;
    speeds[i] = (after - before) * metresPerPulse() / seconds;
    Serial.printf("%d, %.3f, %ld\n", STEPS[i], speeds[i], after - before);
  }
  applyPwm(0);
  // Fit the straight line  pwm = a + b x speed  (least squares, only moving points)
  float sx = 0, sy = 0, sxx = 0, sxy = 0;
  int n = 0;
  for (int i = 0; i < N; i++) {
    if (speeds[i] < 0.02f) continue;              // the car did not move: skip
    sx += speeds[i];
    sy += STEPS[i];
    sxx += speeds[i] * speeds[i];
    sxy += speeds[i] * STEPS[i];
    n++;
  }
  if (n < 2) {
    Serial.println(F("The car hardly moved. Check the battery, the brake wire and the wheels."));
    return;
  }
  float b = (n * sxy - sx * sy) / (n * sxx - sx * sx);
  float a = (sy - b * sx) / n;
  Serial.println();
  Serial.printf("PWM = %.0f + %.0f x speed\n", a, b);
  Serial.printf("  const float SPEED_FF_PWM_START   = %.0ff;\n", a);
  Serial.printf("  const float SPEED_FF_PWM_PER_MPS = %.0ff;\n", b);
  Serial.printf("Top speed at PWM 750: %.2f m/s. Choose SET_SPEED_MAX_MPS below that.\n",
                speeds[N - 1]);
}
void setup() {
  Serial.begin(115200);
  pinMode(PIN_DIR, OUTPUT);
  digitalWrite(PIN_DIR, HIGH);
  pinMode(PIN_BRAKE, OUTPUT);
  digitalWrite(PIN_BRAKE, LOW);
  pinMode(PIN_FG, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FG), onPulse, RISING);
  if (USE_ESC) {
    pwmSetup(PIN_PWM, 50, 16);
    applyPwm(0);
    delay(3000);
  } else {
    pwmSetup(PIN_PWM, 20000, 10);
    applyPwm(0);
  }
  delay(300);
  Serial.println(F("TEST 3: speed sensor. Commands: r | w 85 | s | p 300 | x"));
}
void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    char key = line.length() ? tolower(line[0]) : ' ';
    float value = line.substring(1).toFloat();
    if (key == 'r') {
      if (!g_marked) {
        g_markCount = pulses();
        g_marked = true;
        Serial.println(F("Now turn the drive wheel EXACTLY 10 turns by hand, then type r again."));
      } else {
        long delta = pulses() - g_markCount;
        g_marked = false;
        g_pulsesPerRev = delta / 10.0f;
        Serial.printf("%ld pulses for 10 turns -> %.1f pulses per wheel turn\n", delta, g_pulsesPerRev);
        Serial.printf("  const float PULSES_PER_WHEEL_REV = %.1ff;\n", g_pulsesPerRev);
        Serial.printf("  one pulse = %.4f m\n", metresPerPulse());
      }
    } else if (key == 'w') {
      if (value > 10) g_wheelMm = value;
      Serial.printf("wheel diameter = %.1f mm -> %.4f m per pulse\n", g_wheelMm, metresPerPulse());
    } else if (key == 's') {
      sweep();
    } else if (key == 'p') {
      applyPwm((int)value);
      Serial.printf("PWM = %d\n", (int)value);
    } else if (key == 'x') {
      applyPwm(0);
      Serial.println(F("stopped"));
    }
  }
  static unsigned long lastMs = 0;
  static long lastCount = 0;
  if (millis() - lastMs >= 500) {
    long count = pulses();
    float speed = (count - lastCount) * metresPerPulse() / ((millis() - lastMs) / 1000.0f);
    Serial.printf("pulses %8ld   speed %.3f m/s\n", count, speed);
    lastCount = count;
    lastMs = millis();
  }
}
