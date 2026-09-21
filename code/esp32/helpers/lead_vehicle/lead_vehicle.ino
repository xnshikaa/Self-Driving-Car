// =====================================================================
//  HELPER — "lead vehicle": a second car that drives a repeatable profile
// =====================================================================
//  To test adaptive cruise control you need a car IN FRONT that speeds up,
//  cruises and brakes the SAME WAY every time. Pushing a box by hand is
//  never the same twice, so your results cannot be compared.
//
//  Put this sketch on the ESP32 of your Project 1 / Project 3 car (TB6612
//  motor driver, same pins as those projects) and tape a flat card
//  (15 x 15 cm, white paper) to its back so the ultrasonic sensor of the
//  ACC car always sees a clean, flat surface.
//
//  Profiles (type the number in the Serial Monitor, or press the BOOT button):
//     1  CONSTANT    : 4 s at slow speed
//     2  SLOW DOWN   : cruise 3 s, then slow to half speed, cruise 3 s
//     3  STOP        : cruise 3 s, then brake to a stop and stay
//     4  STOP AND GO : cruise 2 s, stop 3 s, drive again 3 s
//     5  CUT IN      : stand still for 2 s (the ACC car drives up), then go
//     0  stop now
//  Every run starts 2 seconds after you type the number, so you have time
//  to put both cars in place and step back.
// =====================================================================
const int PIN_L_PWM = 25, PIN_L_IN1 = 26, PIN_L_IN2 = 27;
const int PIN_R_PWM = 32, PIN_R_IN1 = 18, PIN_R_IN2 = 19;
const int PIN_STBY = 23;
const int PIN_BOOT_BUTTON = 0;
const int PWM_MAX = 1023;
// Speeds as PWM values. Change these until the lead car drives at a sensible
// speed for your floor (roughly 0.25 m/s slow, 0.45 m/s cruise).
const int PWM_SLOW = 380;
const int PWM_CRUISE = 520;
void pwmSetup(int pin, int channel) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)channel;
  ledcAttach(pin, 20000, 10);
#else
  ledcSetup(channel, 20000, 10);
  ledcAttachPin(pin, channel);
#endif
}
void pwmSet(int pin, int channel, int duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)channel;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(channel, duty);
#endif
}
void drive(int pwm) {
  pwm = constrain(pwm, 0, PWM_MAX);
  digitalWrite(PIN_L_IN1, HIGH);
  digitalWrite(PIN_L_IN2, LOW);
  digitalWrite(PIN_R_IN1, HIGH);
  digitalWrite(PIN_R_IN2, LOW);
  pwmSet(PIN_L_PWM, 0, pwm);
  pwmSet(PIN_R_PWM, 1, pwm);
}
void brake() {
  digitalWrite(PIN_L_IN1, HIGH);
  digitalWrite(PIN_L_IN2, HIGH);       // both inputs HIGH = short the motor = brake
  digitalWrite(PIN_R_IN1, HIGH);
  digitalWrite(PIN_R_IN2, HIGH);
  pwmSet(PIN_L_PWM, 0, PWM_MAX);
  pwmSet(PIN_R_PWM, 1, PWM_MAX);
  delay(200);
  coast();
}
void coast() {
  pwmSet(PIN_L_PWM, 0, 0);
  pwmSet(PIN_R_PWM, 1, 0);
  digitalWrite(PIN_L_IN1, LOW);
  digitalWrite(PIN_L_IN2, LOW);
  digitalWrite(PIN_R_IN1, LOW);
  digitalWrite(PIN_R_IN2, LOW);
}
void runProfile(int profile) {
  Serial.printf("Profile %d starts in 2 seconds...\n", profile);
  delay(2000);
  unsigned long t0 = millis();
  switch (profile) {
    case 1:
      drive(PWM_SLOW);
      delay(4000);
      brake();
      break;
    case 2:
      drive(PWM_CRUISE);
      delay(3000);
      drive(PWM_SLOW);
      delay(3000);
      brake();
      break;
    case 3:
      drive(PWM_CRUISE);
      delay(3000);
      brake();
      break;
    case 4:
      drive(PWM_CRUISE);
      delay(2000);
      brake();
      delay(3000);
      drive(PWM_CRUISE);
      delay(3000);
      brake();
      break;
    case 5:
      coast();
      delay(2000);
      drive(PWM_CRUISE);
      delay(4000);
      brake();
      break;
    default:
      coast();
      break;
  }
  coast();
  Serial.printf("Profile %d finished after %lu ms.\n", profile, millis() - t0);
}
void setup() {
  Serial.begin(115200);
  pinMode(PIN_L_IN1, OUTPUT);
  pinMode(PIN_L_IN2, OUTPUT);
  pinMode(PIN_R_IN1, OUTPUT);
  pinMode(PIN_R_IN2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
  digitalWrite(PIN_STBY, HIGH);
  pwmSetup(PIN_L_PWM, 0);
  pwmSetup(PIN_R_PWM, 1);
  coast();
  delay(300);
  Serial.println(F("LEAD VEHICLE: type 1..5 for a profile, 0 to stop."));
}
void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length()) runProfile(line.toInt());
  }
  if (digitalRead(PIN_BOOT_BUTTON) == LOW) {     // the BOOT button repeats profile 3
    delay(50);
    if (digitalRead(PIN_BOOT_BUTTON) == LOW) {
      while (digitalRead(PIN_BOOT_BUTTON) == LOW) delay(10);
      runProfile(3);
    }
  }
  delay(10);
}
