// =====================================================================
//  TEST 7 — UART wires between Raspberry Pi and ESP32 ("echo" test)
// =====================================================================
//  Wiring (3 wires):  Pi GPIO14 (TXD, pin 8)  -> ESP32 GPIO16 (RX2)
//                     Pi GPIO15 (RXD, pin 10) <- ESP32 GPIO17 (TX2)
//                     Pi GND (pin 6)          -- ESP32 GND
//  Both boards use 3.3 V signals, so NO level shifter is needed.
//  How to test: upload this, then on the Pi run   python3 tools/uart_echo_test.py
//  What you should see: the Pi prints "ESP32 got: hello 1", "hello 2", ...
//  and this Serial Monitor shows the same lines.
// =====================================================================
const int RX2_PIN = 16;
const int TX2_PIN = 17;
String line;
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  delay(500);
  Serial.println("TEST 7: waiting for text from the Raspberry Pi on GPIO16...");
}
void loop() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      Serial.printf("Pi says: %s\n", line.c_str());
      Serial2.printf("ESP32 got: %s\n", line.c_str());  // send it back
      line = "";
    } else if (c != '\r') {
      line += c;
      if (line.length() > 200) line = "";   // protect against garbage
    }
  }
}
