#pragma once

#include <Arduino.h>

struct DistanceReading {
  float metres = NAN;
  bool valid = false;
  uint32_t echoMicros = 0;
};

class DistanceSensor {
 public:
  void begin();
  DistanceReading read();
  DistanceReading readFiltered();

 private:
  static constexpr uint8_t HISTORY_SIZE = 3;
  float history_[HISTORY_SIZE] = {NAN, NAN, NAN};
  uint8_t count_ = 0;
  uint8_t next_ = 0;
  static float median3(float a, float b, float c);
};
