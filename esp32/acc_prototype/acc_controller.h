#pragma once

#include <Arduino.h>
#include "distance_sensor.h"

enum class AccMode : uint8_t { OFF, CRUISE, FOLLOW, STOP, FAULT };

struct AccDecision {
  AccMode mode = AccMode::OFF;
  int requestedPwm = 0;
  bool brake = true;
  float distanceM = NAN;
  bool sensorValid = false;
};

class AccController {
 public:
  AccDecision update(bool runRequested, const DistanceReading &reading);
  void reset();
  AccMode mode() const { return mode_; }
  static const char *modeName(AccMode mode);

 private:
  AccMode mode_ = AccMode::OFF;
  uint8_t invalidCount_ = 0;
  int lastPwm_ = 0;
  int rampTo(int target);
};
