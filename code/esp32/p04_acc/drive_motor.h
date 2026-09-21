// =====================================================================
//  drive_motor.h  —  the brushless (BLDC) drive motor
// =====================================================================
//  A BLDC motor has no brushes: its driver (or ESC) switches the current
//  through three coils in turn, so the magnets keep chasing the next coil.
//  We never switch the coils ourselves; we only tell the driver HOW FAST.
//
//  Option 1 — 12 V geared BLDC motor with a built-in driver (default)
//     PWM wire   : 20 kHz pulses, more ON-time = faster
//     DIR wire   : which way the motor turns (we always drive forward)
//     BRAKE wire : shorts the coils electrically -> the motor stops quickly
//     FG wire    : a pulse for every step of the motor (read by speed_sensor.h)
//
//  Option 2 — A2212 motor + 30 A ESC (Electronic Speed Controller)
//     One signal wire with a "servo pulse" 50 times per second:
//     1000 us = stop ... 2000 us = full throttle. The ESC must see
//     1000 us for about 3 s after power-on ("arming") before it will start.
// =====================================================================
#pragma once
#include <Arduino.h>
#include "config.h"
#include "pwm_compat.h"
class DriveMotor {
 public:
  void begin() {
    if (DRIVE_TYPE == DRIVE_ESC) {
      pwmAttach(PIN_MOTOR_PWM, 0, 50, 16);          // 50 Hz, 16-bit resolution
      writeEscMicros(ESC_MIN_US);
      armedAtMs_ = millis() + ESC_ARM_MS;
    } else {
      pinMode(PIN_MOTOR_DIR, OUTPUT);
      digitalWrite(PIN_MOTOR_DIR, MOTOR_DIR_FORWARD);
      if (PIN_MOTOR_BRAKE >= 0) pinMode(PIN_MOTOR_BRAKE, OUTPUT);
      pwmAttach(PIN_MOTOR_PWM, 0, MOTOR_PWM_FREQ_HZ, PWM_BITS);
      armedAtMs_ = millis();
    }
    setBrake(true);
    setPwm(0);
  }
  // power: 0 = no drive ... PWM_MAX = full power (forward only)
  void setPwm(int power) {
    power = constrain(power, 0, PWM_MAX);
    if (!ready()) power = 0;
    pwm_ = power;
    if (DRIVE_TYPE == DRIVE_ESC) {
      writeEscMicros(ESC_MIN_US + (long)(ESC_MAX_US - ESC_MIN_US) * power / PWM_MAX);
    } else {
      pwmWrite(PIN_MOTOR_PWM, 0, MOTOR_PWM_INVERTED ? PWM_MAX - power : power);
    }
  }
  void setBrake(bool on) {
    braking_ = on;
    if (DRIVE_TYPE == DRIVE_ESC) {
      if (on) setPwm(0);                              // a normal ESC can only cut the power
    } else if (PIN_MOTOR_BRAKE >= 0) {
      digitalWrite(PIN_MOTOR_BRAKE, on ? MOTOR_BRAKE_ACTIVE : !MOTOR_BRAKE_ACTIVE);
    }
  }
  bool ready() const { return millis() >= armedAtMs_; }   // ESC armed (always true for the BLDC driver)
  int pwm() const { return pwm_; }
  bool braking() const { return braking_; }
 private:
  void writeEscMicros(long us) {
    pwmWrite(PIN_MOTOR_PWM, 0, (uint32_t)(us * 65535L / 20000L));   // 20000 us = one 50 Hz period
  }
  unsigned long armedAtMs_ = 0;
  int pwm_ = 0;
  bool braking_ = true;
};
