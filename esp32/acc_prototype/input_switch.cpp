#include "input_switch.h"
#include "config.h"

void RunSwitch::begin() {
  pinMode(PIN_RUN_SWITCH, INPUT_PULLUP);
  rawRun_ = digitalRead(PIN_RUN_SWITCH) == LOW;
  stableRun_ = rawRun_;
  changedAtMs_ = millis();
}

void RunSwitch::update(uint32_t nowMs) {
  const bool sample = digitalRead(PIN_RUN_SWITCH) == LOW;
  if (sample != rawRun_) {
    rawRun_ = sample;
    changedAtMs_ = nowMs;
  } else if (sample != stableRun_ && nowMs - changedAtMs_ >= SWITCH_DEBOUNCE_MS) {
    stableRun_ = sample;
  }
}
