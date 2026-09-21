// =====================================================================
//  speed_sensor.cpp  —  the interrupt code behind speed_sensor.h
// =====================================================================
#include "speed_sensor.h"
#include "config.h"
static volatile long s_count = 0;
static volatile unsigned long s_lastPulseUs = 0;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static long s_prevCount = 0;
static unsigned long s_prevPulseUs = 0;
static float s_rawSpeed = 0, s_speed = 0;
// Runs automatically on every rising edge of the speed pulse wire.
static void IRAM_ATTR onPulse() {
  unsigned long now = micros();
  if (now - s_lastPulseUs < MIN_PULSE_GAP_US) return;   // electrical noise, not a real pulse
  portENTER_CRITICAL_ISR(&s_lock);
  s_count = s_count + 1;                                // (not s_count++: that is deprecated on volatile)
  s_lastPulseUs = now;
  portEXIT_CRITICAL_ISR(&s_lock);
}
namespace SpeedSensor {
void begin() {
  // The FG wire of most BLDC drivers is "open collector": it can only pull
  // LOW, so the input needs a pull-up. The ESP32's internal one (about
  // 45 kOhm) is weak: add 10 kOhm from the wire to 3.3 V as well.
  pinMode(PIN_SPEED_PULSE, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_SPEED_PULSE), onPulse, RISING);
  s_prevPulseUs = micros();
}
float update(unsigned long nowMicros) {
  portENTER_CRITICAL(&s_lock);
  long count = s_count;
  unsigned long lastUs = s_lastPulseUs;
  portEXIT_CRITICAL(&s_lock);
  const float metersPerPulse = PI * WHEEL_DIAMETER_M / PULSES_PER_WHEEL_REV;
  long newPulses = count - s_prevCount;
  if (newPulses > 0 && lastUs != s_prevPulseUs) {
    // average speed over whole pulses: from the last pulse we saw before to the newest one
    float seconds = (lastUs - s_prevPulseUs) * 1e-6f;
    if (seconds > 0) s_rawSpeed = newPulses * metersPerPulse / seconds;
    s_prevCount = count;
    s_prevPulseUs = lastUs;
  } else {
    // no new pulse: the speed cannot be higher than one pulse in the time since the last one
    float seconds = (nowMicros - s_prevPulseUs) * 1e-6f;
    float limit = (seconds > 0) ? metersPerPulse / seconds : s_rawSpeed;
    if (limit < s_rawSpeed) s_rawSpeed = limit;
    if (seconds > 0.5f) s_rawSpeed = 0;                   // nothing for half a second: standing still
  }
  s_speed += SPEED_FILTER_ALPHA * (s_rawSpeed - s_speed);
  return s_speed;
}
float speed() { return s_speed; }
float distanceM() { return pulses() * PI * WHEEL_DIAMETER_M / PULSES_PER_WHEEL_REV; }
long pulses() {
  portENTER_CRITICAL(&s_lock);
  long c = s_count;
  portEXIT_CRITICAL(&s_lock);
  return c;
}
}  // namespace SpeedSensor
