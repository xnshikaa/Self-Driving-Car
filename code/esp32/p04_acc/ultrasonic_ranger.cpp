// =====================================================================
//  ultrasonic_ranger.cpp  —  the timing code behind ultrasonic_ranger.h
// =====================================================================
#include "ultrasonic_ranger.h"
#include "config.h"
static volatile unsigned long s_riseUs = 0, s_fallUs = 0;
static volatile bool s_rose = false, s_fell = false;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_active = false;          // a ping is being timed
static bool s_busyAtTrigger = false;   // ECHO was still HIGH from the last "nothing there" wait
static unsigned long s_pingStartMs = 0;
static int s_missRun = 0;
static RangeReading s_reading;
static float s_history[US_MEDIAN_WINDOW];
static int s_historyCount = 0, s_historyPos = 0;
static void IRAM_ATTR onEcho() {
  unsigned long nowUs = micros();
  bool high = digitalRead(PIN_US_ECHO);
  portENTER_CRITICAL_ISR(&s_lock);
  if (high) {
    if (!s_rose) {
      s_riseUs = nowUs;
      s_rose = true;
    }
  } else if (s_rose && !s_fell) {
    s_fallUs = nowUs;
    s_fell = true;
  }
  portEXIT_CRITICAL_ISR(&s_lock);
}
static float median() {
  float sorted[US_MEDIAN_WINDOW];
  int n = s_historyCount;
  for (int k = 0; k < n; k++) sorted[k] = s_history[k];
  for (int a = 1; a < n; a++) {                 // insertion sort
    float value = sorted[a];
    int b = a - 1;
    while (b >= 0 && sorted[b] > value) {
      sorted[b + 1] = sorted[b];
      b--;
    }
    sorted[b + 1] = value;
  }
  return sorted[n / 2];
}
static void addReading(float metres) {
  float previous = s_reading.rawM;
  s_history[s_historyPos] = metres;
  s_historyPos = (s_historyPos + 1) % US_MEDIAN_WINDOW;
  if (s_historyCount < US_MEDIAN_WINDOW) s_historyCount++;
  s_reading.rawM = metres;
  float filtered = (s_historyCount >= 3) ? median() : US_FAR_M;
  // Fast close: the last two readings agree that something is much closer.
  float closer = max(metres, previous);          // the larger of the two = the careful choice
  if (s_historyCount >= 2 && closer < filtered - US_FAST_CLOSE_M) filtered = closer;
  s_reading.filteredM = filtered;
  s_reading.ok = true;
  s_reading.pings++;
  s_missRun = 0;
}
static bool finishPing() {
  if (!s_active) return false;
  s_active = false;
  portENTER_CRITICAL(&s_lock);
  bool rose = s_rose, fell = s_fell;
  unsigned long riseUs = s_riseUs, fallUs = s_fallUs;
  portEXIT_CRITICAL(&s_lock);
  if (s_busyAtTrigger || (rose && !fell)) {
    addReading(US_FAR_M);                         // the sound never came back: nothing in range
  } else if (rose && fell) {
    float metres = (fallUs - riseUs) * 1e-6f * SOUND_SPEED_MPS / 2.0f;
    if (metres > US_MAX_RANGE_M) metres = US_FAR_M;
    else if (metres < US_MIN_RANGE_M) metres = US_MIN_RANGE_M;
    addReading(metres);
  } else {
    s_reading.misses++;                           // no echo pulse at all: a wiring problem
    if (++s_missRun >= US_FAULT_COUNT) s_reading.ok = false;
    return false;
  }
  return true;
}
static void startPing(unsigned long nowMs) {
  portENTER_CRITICAL(&s_lock);
  s_rose = false;
  s_fell = false;
  portEXIT_CRITICAL(&s_lock);
  s_active = true;
  s_pingStartMs = nowMs;
  s_busyAtTrigger = (digitalRead(PIN_US_ECHO) == HIGH);
  if (!s_busyAtTrigger) {
    digitalWrite(PIN_US_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_US_TRIG, LOW);
  }
}
namespace Ranger {
void begin() {
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  pinMode(PIN_US_ECHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_US_ECHO), onEcho, CHANGE);
  s_pingStartMs = millis() - US_PING_MS;
}
bool update(unsigned long nowMs) {
  if (nowMs - s_pingStartMs < US_PING_MS) {
    if (s_active) {                               // the echo already came back: use it now
      portENTER_CRITICAL(&s_lock);
      bool done = s_fell;
      portEXIT_CRITICAL(&s_lock);
      if (done) return finishPing();
    }
    return false;
  }
  bool got = finishPing();
  startPing(nowMs);
  return got;
}
const RangeReading &reading() { return s_reading; }
float lagS() { return ((US_MEDIAN_WINDOW - 1) / 2.0f) * US_PING_MS / 1000.0f; }
}  // namespace Ranger
