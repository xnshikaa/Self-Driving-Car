#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// VERIFY PINS/address.
constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr int SWITCH_PIN = 4;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found; check address and wiring");
    while (true) delay(1000);
  }
}

void loop() {
  const bool run = digitalRead(SWITCH_PIN) == LOW;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(run ? "RUN" : "STOP");
  display.setTextSize(1);
  display.setCursor(0, 32);
  display.println("Switch to GND = RUN");
  display.display();
  Serial.printf("switch_run=%d\n", run ? 1 : 0);
  delay(250);
}
