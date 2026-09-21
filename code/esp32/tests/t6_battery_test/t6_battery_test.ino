// =====================================================================
//  TEST 6 — Battery voltage through the resistor divider
// =====================================================================
//  Wiring:  battery + (after fuse and switch) -> 100 kOhm -> GPIO36 (VP)
//           GPIO36 -> 22 kOhm -> GND        (plus 100 nF from GPIO36 to GND)
//  CHECK WITH A MULTIMETER FIRST: the voltage on GPIO36 must be below 3.0 V
//  with a fully charged battery (expected about 2.3 V at 12.6 V).
//  What you should see: the printed battery voltage within about 0.2 V of
//  your multimeter. If it is always a little off, adjust CORRECTION.
// =====================================================================
const int BATTERY_PIN = 36;
const float DIVIDER = (100.0 + 22.0) / 22.0;
const float CORRECTION = 1.00;   // e.g. 1.03 if the reading is 3 % too low
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("TEST 6: battery voltage");
}
void loop() {
  long sum = 0;
  for (int i = 0; i < 20; i++) {   // average 20 readings to reduce noise
    sum += analogReadMilliVolts(BATTERY_PIN);
    delay(5);
  }
  float pinVolts = (sum / 20.0) / 1000.0;
  float batteryVolts = pinVolts * DIVIDER * CORRECTION;
  float perCell = batteryVolts / 3.0;
  Serial.printf("pin = %.3f V   battery = %.2f V   (%.2f V per cell) %s\n", pinVolts, batteryVolts, perCell,
                batteryVolts < 5.0 ? "<- no battery?" : (perCell < 3.4 ? "<- CHARGE NOW" : "OK"));
  delay(500);
}
