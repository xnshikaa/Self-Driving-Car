#include <Wire.h>

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("I2C scanner starting...");
}

void loop() {
  int found = 0;
  Serial.println("Scanning...");

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("I2C device found at 0x%02X\n", address);
      ++found;
    }
  }

  if (found == 0) Serial.println("No I2C devices found");
  Serial.println("---");
  delay(2000);
}
