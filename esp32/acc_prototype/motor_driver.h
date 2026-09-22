#pragma once

#include <Arduino.h>

struct MotorCommand {
  int pwm = 0;
  bool brake = true;
};

class MotorDriver {
 public:
  void begin();
  void apply(MotorCommand command);
  void stop();
  int appliedPwm() const { return appliedPwm_; }
  bool isStopped() const { return appliedPwm_ == 0; }

 private:
  int appliedPwm_ = 0;
  void setChannel(int in1, int in2, int pwmPin, int pwm, bool inverted);
};
