// =====================================================================
//  pid_controller.h  —  a small, safe PID controller
// =====================================================================
//  error = target - measured
//  P (Proportional): push harder when the error is big.
//  I (Integral):     slowly add push if a small error never goes away.
//  D (Derivative):   calm down if the measurement is changing fast.
//  "Anti-windup": when the output is already at its limit, we stop the
//  I part from growing, otherwise the car overshoots badly later.
//  "Integral band": the I part only works when the error is already
//  small (for example while cruising), not during a big speed change.
// =====================================================================
#pragma once
#include <Arduino.h>
class PIDController {
 public:
  PIDController(float kp = 0, float ki = 0, float kd = 0) : kp_(kp), ki_(ki), kd_(kd) {}
  void setGains(float kp, float ki, float kd) {
    kp_ = kp;
    ki_ = ki;
    kd_ = kd;
  }
  void setOutputLimits(float low, float high) {
    low_ = low;
    high_ = high;
  }
  void setIntegralBand(float band) { integralBand_ = band; }
  void reset() {
    integral_ = 0;
    havePrevious_ = false;
  }
  // target and measured in the same units; dt in seconds.
  float update(float target, float measured, float dt) {
    float error = target - measured;
    float pTerm = kp_ * error;
    float newIntegral = integral_;
    if (fabs(error) <= integralBand_) newIntegral += error * dt;
    float iTerm = ki_ * newIntegral;
    float dTerm = 0;
    if (havePrevious_ && dt > 0) {
      dTerm = -kd_ * (measured - previousMeasured_) / dt;  // derivative of the measurement
    }
    previousMeasured_ = measured;
    havePrevious_ = true;
    float output = pTerm + iTerm + dTerm;
    if (output > high_) {
      output = high_;
      if (error < 0) integral_ = newIntegral;  // only allow the integral to shrink
    } else if (output < low_) {
      output = low_;
      if (error > 0) integral_ = newIntegral;
    } else {
      integral_ = newIntegral;
    }
    return output;
  }
  float kp() const { return kp_; }
  float ki() const { return ki_; }
  float kd() const { return kd_; }
 private:
  float kp_, ki_, kd_;
  float low_ = -1e9f, high_ = 1e9f;
  float integralBand_ = 1e9f;
  float integral_ = 0;
  float previousMeasured_ = 0;
  bool havePrevious_ = false;
};
