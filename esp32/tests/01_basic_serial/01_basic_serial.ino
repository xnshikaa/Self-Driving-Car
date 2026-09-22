void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("ESP32 basic serial test: PASS");
  Serial.println("If this text is visible, board selection, upload, and USB serial work.");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.printf("uptime_ms=%lu\n", static_cast<unsigned long>(millis()));
  }
}
