// =====================================================================
//  battery_monitor.h  —  measures the battery voltage safely
// =====================================================================
//  The battery can be 12.6 V, but an ESP32 pin must never see more than
//  3.3 V. Two resistors (100 kOhm on top, 22 kOhm at the bottom) divide
//  the voltage:  pin voltage = battery x 22 / (100 + 22)  -> 12.6 V gives 2.27 V.
//  We multiply back by (100 + 22) / 22 to get the real battery voltage.
// =====================================================================
#pragma once
#include <Arduino.h>
#include "config.h"
class BatteryMonitor {
 public:
  void begin() {
    pinMode(PIN_BATTERY, INPUT);
    volts_ = readNow();
  }
  // Call regularly; returns a smoothed voltage.
  float update() {
    volts_ += 0.1f * (readNow() - volts_);
    return volts_;
  }
  float volts() const { return volts_; }
 private:
  float readNow() {
    // analogReadMilliVolts() uses the ESP32's factory calibration -> more accurate than analogRead()
    return analogReadMilliVolts(PIN_BATTERY) / 1000.0f * BATTERY_DIVIDER;
  }
  float volts_ = 0;
};
