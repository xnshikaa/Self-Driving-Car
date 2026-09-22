#include "motor_driver.h"
#include "config.h"

void MotorDriver::begin() {
  pinMode(PIN_TB_STBY, OUTPUT);
  pinMode(PIN_M1_IN1, OUTPUT); pinMode(PIN_M1_IN2, OUTPUT); pinMode(PIN_M1_PWM, OUTPUT);
  pinMode(PIN_M2_IN1, OUTPUT); pinMode(PIN_M2_IN2, OUTPUT); pinMode(PIN_M2_PWM, OUTPUT);
  pinMode(PIN_M3_IN1, OUTPUT); pinMode(PIN_M3_IN2, OUTPUT); pinMode(PIN_M3_PWM, OUTPUT);
  pinMode(PIN_M4_IN1, OUTPUT); pinMode(PIN_M4_IN2, OUTPUT); pinMode(PIN_M4_PWM, OUTPUT);
  analogWriteFrequency(PIN_M1_PWM, MOTOR_PWM_FREQUENCY_HZ);
  analogWriteFrequency(PIN_M2_PWM, MOTOR_PWM_FREQUENCY_HZ);
  analogWriteFrequency(PIN_M3_PWM, MOTOR_PWM_FREQUENCY_HZ);
  analogWriteFrequency(PIN_M4_PWM, MOTOR_PWM_FREQUENCY_HZ);
  analogWriteResolution(PIN_M1_PWM, MOTOR_PWM_BITS);
  analogWriteResolution(PIN_M2_PWM, MOTOR_PWM_BITS);
  analogWriteResolution(PIN_M3_PWM, MOTOR_PWM_BITS);
  analogWriteResolution(PIN_M4_PWM, MOTOR_PWM_BITS);
  stop();
}

void MotorDriver::setChannel(int in1, int in2, int pwmPin, int pwm, bool inverted) {
  pwm = constrain(pwm, 0, MOTOR_PWM_MAX);
  if (pwm == 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, 0);
    return;
  }
  const bool forward = !inverted;
  digitalWrite(in1, forward ? HIGH : LOW);
  digitalWrite(in2, forward ? LOW : HIGH);
  analogWrite(pwmPin, pwm);
}

void MotorDriver::apply(MotorCommand command) {
  const int requested = constrain(command.pwm, 0, MAX_ALLOWED_PWM);
  const int delta = requested - appliedPwm_;
  if (delta > MAX_PWM_STEP) appliedPwm_ += MAX_PWM_STEP;
  else if (delta < -MAX_PWM_STEP) appliedPwm_ -= MAX_PWM_STEP;
  else appliedPwm_ = requested;

  const bool driveEnabled = appliedPwm_ > 0 && !command.brake;
  digitalWrite(PIN_TB_STBY, driveEnabled == TB_STBY_ACTIVE_HIGH ? HIGH : LOW);
  setChannel(PIN_M1_IN1, PIN_M1_IN2, PIN_M1_PWM, driveEnabled ? appliedPwm_ : 0, LEFT_MOTOR_INVERTED);
  setChannel(PIN_M2_IN1, PIN_M2_IN2, PIN_M2_PWM, driveEnabled ? appliedPwm_ : 0, RIGHT_MOTOR_INVERTED);
  setChannel(PIN_M3_IN1, PIN_M3_IN2, PIN_M3_PWM, driveEnabled ? appliedPwm_ : 0, LEFT_MOTOR_INVERTED);
  setChannel(PIN_M4_IN1, PIN_M4_IN2, PIN_M4_PWM, driveEnabled ? appliedPwm_ : 0, RIGHT_MOTOR_INVERTED);
  if (!driveEnabled) appliedPwm_ = 0;
}

void MotorDriver::stop() {
  appliedPwm_ = 0;
  digitalWrite(PIN_TB_STBY, TB_STBY_ACTIVE_HIGH ? LOW : HIGH);
  setChannel(PIN_M1_IN1, PIN_M1_IN2, PIN_M1_PWM, 0, LEFT_MOTOR_INVERTED);
  setChannel(PIN_M2_IN1, PIN_M2_IN2, PIN_M2_PWM, 0, RIGHT_MOTOR_INVERTED);
  setChannel(PIN_M3_IN1, PIN_M3_IN2, PIN_M3_PWM, 0, LEFT_MOTOR_INVERTED);
  setChannel(PIN_M4_IN1, PIN_M4_IN2, PIN_M4_PWM, 0, RIGHT_MOTOR_INVERTED);
}
