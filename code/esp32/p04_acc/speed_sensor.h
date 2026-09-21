// =====================================================================
//  speed_sensor.h  —  our own speed from the FG pulses (or an optical disc)
// =====================================================================
//  Every pulse means the wheel turned a fixed small angle:
//     metres per pulse = wheel circumference / PULSES_PER_WHEEL_REV
//
//  Two ways to turn pulses into speed:
//   * COUNT pulses in a fixed time. Simple, but at 100 Hz there are only a
//     few pulses per step, so the speed jumps up and down.
//   * TIME the pulses: speed = pulses / (time between the first and the
//     last of them). Much smoother, and it still works at very low speed.
//  We use the second way. If no pulse comes for a while, the speed
//  can be at most "one pulse divided by the time since the last pulse",
//  so the reading falls smoothly to zero when the car stops.
// =====================================================================
#pragma once
#include <Arduino.h>
namespace SpeedSensor {
void begin();
// Call every SPEED_LOOP_MS. Returns the filtered speed in m/s.
float update(unsigned long nowMicros);
float speed();          // latest filtered speed (m/s)
float distanceM();      // distance driven since power-on (m)
long pulses();          // raw pulse count
}  // namespace SpeedSensor
