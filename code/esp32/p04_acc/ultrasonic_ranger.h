// =====================================================================
//  ultrasonic_ranger.h  —  one HC-SR04 that measures the gap to the lead vehicle
// =====================================================================
//  How an HC-SR04 works:
//    1. We give TRIG a 10 microsecond pulse.
//    2. The sensor sends 8 clicks of 40 kHz sound (you cannot hear it).
//    3. It holds ECHO HIGH until the sound comes back from the lead vehicle.
//    4. distance = (echo time x speed of sound) / 2   (there AND back)
//
//  We ping 20 times per second (every 50 ms). An INTERRUPT notes when
//  ECHO goes HIGH and LOW, so the program never waits for the echo.
//
//  Cleaning the readings:
//    * median of the last 5 readings -> a single wrong value is thrown away
//    * "fast close": if the last TWO readings both say something is much
//      closer than the median, we believe them at once (a car cut in!)
//  Sensor FAULT: no echo pulse at all 3 times in a row = broken wire or no 5 V.
// =====================================================================
#pragma once
#include <Arduino.h>
struct RangeReading {
  float rawM = 4.0f;        // last single reading (4.0 = nothing there)
  float filteredM = 4.0f;   // median, or the fast-close value (use this one)
  bool ok = true;           // false = sensor fault
  unsigned long pings = 0;
  unsigned long misses = 0;
};
namespace Ranger {
void begin();
// Call as often as possible. Returns true when a new reading is ready.
bool update(unsigned long nowMs);
const RangeReading &reading();
// How old (in seconds) the filtered value is on average (the median delays it).
float lagS();
}  // namespace Ranger
