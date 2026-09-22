#pragma once

#include <Arduino.h>

class RunSwitch {
 public:
  void begin();
  void update(uint32_t nowMs);
  bool runRequested() const { return stableRun_; }

 private:
  bool rawRun_ = false;
  bool stableRun_ = false;
  uint32_t changedAtMs_ = 0;
};
