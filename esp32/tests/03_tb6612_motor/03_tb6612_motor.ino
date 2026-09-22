#include <Arduino.h>

// Test with wheels lifted and a low duty cycle first.
// Start with one motor. Change to true only after one channel works safely.
constexpr bool TEST_ALL_MOTORS = false;
constexpr int STBY = 23;
constexpr int M1_IN1 = 26, M1_IN2 = 27, M1_PWM = 25;
constexpr int M2_IN1 = 32, M2_IN2 = 33, M2_PWM = 14;
constexpr int M3_IN1 = 16, M3_IN2 = 17, M3_PWM = 13;
constexpr int M4_IN1 = 5, M4_IN2 = 12, M4_PWM = 19;

void setMotor(int in1, int in2, int pwmPin, int pwm) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(pwmPin, pwm);
}

void setTestPwm(int pwm) {
  setMotor(M1_IN1, M1_IN2, M1_PWM, pwm);
  if (TEST_ALL_MOTORS) {
    setMotor(M2_IN1, M2_IN2, M2_PWM, pwm);
    setMotor(M3_IN1, M3_IN2, M3_PWM, pwm);
    setMotor(M4_IN1, M4_IN2, M4_PWM, pwm);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(STBY, OUTPUT);
  pinMode(M1_IN1, OUTPUT); pinMode(M1_IN2, OUTPUT); pinMode(M1_PWM, OUTPUT);
  pinMode(M2_IN1, OUTPUT); pinMode(M2_IN2, OUTPUT); pinMode(M2_PWM, OUTPUT);
  pinMode(M3_IN1, OUTPUT); pinMode(M3_IN2, OUTPUT); pinMode(M3_PWM, OUTPUT);
  pinMode(M4_IN1, OUTPUT); pinMode(M4_IN2, OUTPUT); pinMode(M4_PWM, OUTPUT);
  analogWriteFrequency(M1_PWM, 20000); analogWriteFrequency(M2_PWM, 20000);
  analogWriteFrequency(M3_PWM, 20000); analogWriteFrequency(M4_PWM, 20000);
  analogWriteResolution(M1_PWM, 8); analogWriteResolution(M2_PWM, 8);
  analogWriteResolution(M3_PWM, 8); analogWriteResolution(M4_PWM, 8);
  digitalWrite(STBY, HIGH);
}

void loop() {
  for (int pwm = 0; pwm <= 90; pwm += 10) {
    setTestPwm(pwm);
    Serial.printf("forward_pwm=%d, all_motors=%d\n", pwm, TEST_ALL_MOTORS ? 1 : 0);
    delay(500);
  }
  analogWrite(M1_PWM, 0); analogWrite(M2_PWM, 0);
  analogWrite(M3_PWM, 0); analogWrite(M4_PWM, 0);
  digitalWrite(STBY, LOW);
  Serial.println("motor stop; reset board to repeat");
  while (true) delay(1000);
}
