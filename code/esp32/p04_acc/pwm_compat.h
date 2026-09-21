// =====================================================================
//  pwm_compat.h  —  PWM helper that works on ESP32 Arduino core 2.x AND 3.x
// =====================================================================
//  PWM = "Pulse Width Modulation": switching a pin ON and OFF very fast.
//  The longer it stays ON in each cycle, the more power the motor gets.
//  Espressif renamed the PWM functions in core 3.0, so we hide the
//  difference here. Channel numbers are only used by core 2.x:
//  motors use channels 0 and 1, the servo uses channel 4 (another timer).
// =====================================================================
#pragma once
#include <Arduino.h>
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
inline bool pwmAttach(int pin, int channel, uint32_t freqHz, uint8_t bits) {
  (void)channel;                         // core 3 chooses channels and timers itself
  return ledcAttach(pin, freqHz, bits);
}
inline void pwmWrite(int pin, int channel, uint32_t duty) {
  (void)channel;
  ledcWrite(pin, duty);
}
#else
inline bool pwmAttach(int pin, int channel, uint32_t freqHz, uint8_t bits) {
  ledcSetup(channel, freqHz, bits);      // core 2: we must pick the channel
  ledcAttachPin(pin, channel);
  return true;
}
inline void pwmWrite(int pin, int channel, uint32_t duty) {
  (void)pin;
  ledcWrite(channel, duty);
}
#endif
