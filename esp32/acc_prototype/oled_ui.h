#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "acc_controller.h"

class OledUi {
 public:
  bool begin();
  void show(const AccDecision &decision, int appliedPwm, bool runRequested);
  bool present() const { return present_; }

 private:
  Adafruit_SSD1306 display_{128, 64, &Wire, -1};
  bool present_ = false;
};
