// =====================================================================
//  hmi_input.h  —  the driver controls: one knob and two buttons
// =====================================================================
//  HMI means "Human-Machine Interface": the parts a human touches.
//
//  KNOB (potentiometer): a resistor you can turn. 3.3 V on one end,
//  GND on the other; the middle leg gives a voltage between the two.
//  The ESP32 reads it as a number 0..4095, which we turn into the set
//  speed 0.20 ... 0.80 m/s, rounded to steps of 0.05 m/s so the number
//  on the display does not dance.
//
//  BUTTONS: wired from the pin to GND with the internal pull-up on, so
//  the pin reads HIGH when the button is up and LOW when it is pressed.
//  Mechanical contacts BOUNCE (many fast on/off flickers in 1-2 ms), so
//  a press only counts if the pin stays LOW for DEBOUNCE_MS.
//    ENGAGE: short press  -> ACC on / off
//    GAP   : short press  -> 40 / 50 / 60 cm
//            hold 1 s     -> AI lead-speed prediction on / off
// =====================================================================
#pragma once
#include <Arduino.h>
#include "config.h"
const unsigned long DEBOUNCE_MS = 25;
const unsigned long HOLD_MS = 1000;
class Button {
 public:
  void begin(int pin) {
    pin_ = pin;
    pinMode(pin_, INPUT_PULLUP);
    stable_ = raw_ = digitalRead(pin_);
    changedMs_ = millis();
  }
  // Call every loop. Sets clicked/held for exactly one call.
  void update(unsigned long nowMs) {
    clicked_ = held_ = false;
    int now = digitalRead(pin_);
    if (now != raw_) {
      raw_ = now;
      changedMs_ = nowMs;                       // the level moved: start the debounce timer again
    } else if (now != stable_ && nowMs - changedMs_ >= DEBOUNCE_MS) {
      stable_ = now;                            // the level stayed put: it is real
      if (stable_ == LOW) {
        pressedMs_ = nowMs;
        holdDone_ = false;
      } else if (!holdDone_) {
        clicked_ = true;                        // released before 1 s = a short press
      }
    }
    if (stable_ == LOW && !holdDone_ && nowMs - pressedMs_ >= HOLD_MS) {
      held_ = true;                             // still down after 1 s = a long press
      holdDone_ = true;
    }
  }
  bool clicked() const { return clicked_; }
  bool held() const { return held_; }
  bool down() const { return stable_ == LOW; }
 private:
  int pin_ = -1, raw_ = HIGH, stable_ = HIGH;
  unsigned long changedMs_ = 0, pressedMs_ = 0;
  bool clicked_ = false, held_ = false, holdDone_ = false;
};
class Hmi {
 public:
  void begin() {
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_POT, ADC_11db);      // full 0 ... 3.3 V range
    engage_.begin(PIN_BTN_ENGAGE);
    gap_.begin(PIN_BTN_GAP);
    raw_ = analogRead(PIN_POT);
    setSpeed_ = quantise(raw_);
  }
  void update(unsigned long nowMs) {
    engage_.update(nowMs);
    gap_.update(nowMs);
    if (engage_.clicked()) engaged_ = !engaged_;
    if (gap_.held()) predictionWanted_ = !predictionWanted_;
    else if (gap_.clicked()) gapIndex_ = (gapIndex_ + 1) % 3;
    raw_ += (analogRead(PIN_POT) - raw_) * 0.20f;   // smooth the noisy ADC
    float wanted = quantise(raw_);
    // Only accept a new step when the knob has clearly moved past it
    // (otherwise a value sitting exactly between two steps flickers).
    if (fabsf(wanted - setSpeed_) >= SET_SPEED_STEP_MPS * 0.75f) setSpeed_ = wanted;
  }
  bool engaged() const { return engaged_; }
  void setEngaged(bool on) { engaged_ = on; }
  float setSpeed() const { return setSpeed_; }
  float gapSet() const { return GAP_SETTINGS_M[gapIndex_]; }
  int gapIndex() const { return gapIndex_; }
  bool predictionWanted() const { return predictionWanted_; }
 private:
  static float quantise(float raw) {
    float span = SET_SPEED_MAX_MPS - SET_SPEED_MIN_MPS;
    float value = SET_SPEED_MIN_MPS + span * constrain(raw / 4095.0f, 0.0f, 1.0f);
    float steps = roundf((value - SET_SPEED_MIN_MPS) / SET_SPEED_STEP_MPS);
    return SET_SPEED_MIN_MPS + steps * SET_SPEED_STEP_MPS;
  }
  Button engage_, gap_;
  bool engaged_ = false, predictionWanted_ = false;
  int gapIndex_ = GAP_DEFAULT_INDEX;
  float raw_ = 0, setSpeed_ = SET_SPEED_MIN_MPS;
};
