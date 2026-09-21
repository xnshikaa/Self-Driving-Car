// =====================================================================
//  TEST 4 — The HC-SR04 gap sensor: wiring, accuracy and the median filter
// =====================================================================
//  Wiring:  VCC -> 5 V,  GND -> GND (also shared with the ESP32),
//           TRIG -> GPIO18,
//           ECHO -> 1 kOhm -> GPIO19 -> 2 kOhm -> GND
//  The two resistors (a "voltage divider") turn the sensor 5 V output into
//  3.3 V. Without them you can destroy the ESP32 pin. Measure it once with
//  a multimeter: the voltage at GPIO19 must never go above 3.4 V.
//
//  What you see: the raw distance 20 times per second and, next to it, the
//  MEDIAN of the last 5 readings. Wave a book in front of the sensor: the
//  raw number sometimes jumps, the median does not. That is why the real
//  program uses the median.
//    "NO PULSE" = the sensor never answered -> check 5 V, GND, TRIG, ECHO
//    "far"      = nothing within 3 m
//
//  Type  c 50  = CHARACTERISE at 50 cm: hold a flat board (a hardback book)
//  exactly 50 cm in front of the sensor and type  c 50 . The sketch takes
//  100 readings and prints mean, standard deviation, min, max and misses.
//  Repeat at 20, 30, 50, 80, 120, 200 and 300 cm, and with the board at an
//  angle, and fill in the table in Phase 5 of the guide.
// =====================================================================
const int PIN_TRIG = 18;
const int PIN_ECHO = 19;
const float SOUND_SPEED = 346.0;     // m/s at 25 degC: 331.3 + 0.606 x temperature
float g_window[5];
int g_filled = 0, g_pos = 0;
// Returns cm, -1 = no echo pulse at all (wiring problem), 0 = nothing in range
float measureCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(4);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long start = micros();
  while (digitalRead(PIN_ECHO) == LOW) {
    if (micros() - start > 10000) return -1;
  }
  unsigned long rise = micros();
  while (digitalRead(PIN_ECHO) == HIGH) {
    if (micros() - rise > 25000) return 0;
  }
  float cm = (micros() - rise) * 1e-6 * SOUND_SPEED / 2.0 * 100.0;
  return (cm > 300.0) ? 0 : cm;
}
float median5(float value) {
  g_window[g_pos] = value;
  g_pos = (g_pos + 1) % 5;
  if (g_filled < 5) g_filled++;
  float sorted[5];
  for (int i = 0; i < g_filled; i++) sorted[i] = g_window[i];
  for (int a = 1; a < g_filled; a++) {
    float v = sorted[a];
    int b = a - 1;
    while (b >= 0 && sorted[b] > v) {
      sorted[b + 1] = sorted[b];
      b--;
    }
    sorted[b + 1] = v;
  }
  return sorted[g_filled / 2];
}
void characterise(float trueCm) {
  Serial.printf("Characterising at %.0f cm (100 readings)...\n", trueCm);
  float sum = 0, sumSq = 0, lo = 1e9, hi = 0;
  int good = 0, misses = 0, noPulse = 0;
  for (int n = 0; n < 100; n++) {
    float cm = measureCm();
    if (cm > 0) {
      good++;
      sum += cm;
      sumSq += cm * cm;
      lo = min(lo, cm);
      hi = max(hi, cm);
    } else if (cm < 0) {
      noPulse++;
    } else {
      misses++;
    }
    delay(50);
  }
  if (good == 0) {
    Serial.printf("No good readings. misses=%d no_pulse=%d\n", misses, noPulse);
    return;
  }
  float mean = sum / good;
  float sd = sqrt(max(0.0f, sumSq / good - mean * mean));
  Serial.println(F("true_cm, mean_cm, std_cm, min_cm, max_cm, error_cm, misses, no_pulse"));
  Serial.printf("%.0f, %.1f, %.2f, %.1f, %.1f, %+.1f, %d, %d\n", trueCm, mean, sd, lo, hi,
                mean - trueCm, misses, noPulse);
  Serial.printf("KF_MEAS_STD_M can be about %.3f (the standard deviation in metres).\n", sd / 100.0);
}
void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO, INPUT);
  delay(500);
  Serial.println(F("TEST 4: HC-SR04. Type  c 50  to characterise at 50 cm."));
}
void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() && tolower(line[0]) == 'c') {
      float trueCm = line.substring(1).toFloat();
      if (trueCm > 0) characterise(trueCm);
      else Serial.println(F("Type the true distance, for example  c 50"));
    }
  }
  float cm = measureCm();
  if (cm < 0) {
    Serial.println(F("NO PULSE   -> check the wiring and the 5 V supply"));
  } else if (cm == 0) {
    Serial.println(F("far        -> nothing within 3 m"));
  } else {
    Serial.printf("raw %6.1f cm   median %6.1f cm\n", cm, median5(cm));
  }
  delay(50);                       // 20 readings per second, like the real program
}
