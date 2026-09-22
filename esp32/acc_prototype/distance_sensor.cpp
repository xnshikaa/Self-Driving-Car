#include "distance_sensor.h"
#include "config.h"

void DistanceSensor::begin() {
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  pinMode(PIN_US_ECHO, INPUT);
}

DistanceReading DistanceSensor::read() {
  DistanceReading result;
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);

  const uint32_t echo = pulseIn(PIN_US_ECHO, HIGH, SENSOR_TIMEOUT_US);
  result.echoMicros = echo;
  if (echo == 0) return result;

  // HC-SR04 round trip: distance = time * speed of sound / 2.
  const float metres = (static_cast<float>(echo) * 0.000001f * 343.0f) / 2.0f;
  if (metres < 0.03f || metres > SENSOR_MAX_M) return result;
  result.metres = metres;
  result.valid = true;
  return result;
}

float DistanceSensor::median3(float a, float b, float c) {
  if (a > b) { const float t = a; a = b; b = t; }
  if (b > c) { const float t = b; b = c; c = t; }
  if (a > b) { const float t = a; a = b; b = t; }
  return b;
}

DistanceReading DistanceSensor::readFiltered() {
  const DistanceReading current = read();
  if (!current.valid) return current;

  history_[next_] = current.metres;
  next_ = (next_ + 1) % HISTORY_SIZE;
  if (count_ < HISTORY_SIZE) ++count_;

  DistanceReading result = current;
  if (count_ == HISTORY_SIZE) {
    result.metres = median3(history_[0], history_[1], history_[2]);
  }
  return result;
}
