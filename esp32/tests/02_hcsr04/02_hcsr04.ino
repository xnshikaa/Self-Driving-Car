#include <Arduino.h>

// Use a divider on ECHO. GPIO34 is input-only, which is suitable here.
constexpr int TRIG = 18;
constexpr int ECHO = 34;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);
}

void loop() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  const unsigned long us = pulseIn(ECHO, HIGH, 30000UL);
  if (us == 0) Serial.println("distance=INVALID timeout");
  else Serial.printf("distance_cm=%.2f echo_us=%lu\n", us * 0.0343f / 2.0f, us);
  delay(100);
}
