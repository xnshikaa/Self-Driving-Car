// =====================================================================
//  TEST 5 — The driver controls: OLED screen, set-speed knob, two buttons
// =====================================================================
//  Wiring:
//    SSD1306 OLED: VCC -> 3.3 V, GND -> GND, SDA -> GPIO21, SCL -> GPIO22
//    Potentiometer: one outer leg -> 3.3 V, the other outer leg -> GND,
//                   middle leg -> GPIO34
//    Buttons: one leg -> GPIO33 (ENGAGE) and GPIO14 (GAP), the other leg
//             of each -> GND. No resistors needed: the ESP32 has built-in
//             pull-ups, so the pin reads HIGH until you press the button.
//
//  What you should see:
//    * the screen draws a frame, a bar that follows the knob, and two
//      squares that fill in when you press the buttons
//    * the Serial Monitor prints the ADC value, the set speed it becomes,
//      and the state of both buttons
//  If the screen stays black: the sketch prints every I2C address it finds.
//  Most boards are 0x3C; a few are 0x3D (change OLED_ADDRESS below).
//
//  This test uses NO font: it only draws blocks, so it stays short. The
//  real program (p04_acc.ino) writes text with its own small 5x7 font.
// =====================================================================
#include <Wire.h>
const int PIN_SDA = 21, PIN_SCL = 22;
const int PIN_POT = 34, PIN_BTN_ENGAGE = 33, PIN_BTN_GAP = 14;
uint8_t OLED_ADDRESS = 0x3C;
uint8_t screen[128 * 8];        // our copy of the screen: 8 pages of 128 columns
void command(uint8_t c) {
  Wire.beginTransmission(OLED_ADDRESS);
  Wire.write(0x00);
  Wire.write(c);
  Wire.endTransmission();
}
bool oledBegin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  Wire.beginTransmission(OLED_ADDRESS);
  if (Wire.endTransmission() != 0) return false;
  static const uint8_t INIT[] = {0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14,
                                 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1,
                                 0xDB, 0x40, 0xA4, 0xA6, 0xAF};
  for (uint8_t c : INIT) command(c);
  return true;
}
void oledShow() {
  for (int page = 0; page < 8; page++) {
    command(0xB0 | page);
    command(0x00);
    command(0x10);
    for (int start = 0; start < 128; start += 32) {
      Wire.beginTransmission(OLED_ADDRESS);
      Wire.write(0x40);
      Wire.write(&screen[page * 128 + start], 32);
      Wire.endTransmission();
    }
  }
}
void clearScreen() { memset(screen, 0, sizeof(screen)); }
void box(int x, int y, int w, int h, bool filled) {
  for (int px = x; px < x + w; px++) {
    for (int py = y; py < y + h; py++) {
      if (px < 0 || px > 127 || py < 0 || py > 63) continue;
      bool edge = (px == x || px == x + w - 1 || py == y || py == y + h - 1);
      if (filled || edge) screen[(py / 8) * 128 + px] |= (1 << (py % 8));
    }
  }
}
void scanI2C() {
  Serial.println(F("I2C scan:"));
  int found = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  device at 0x%02X\n", address);
      found++;
    }
  }
  if (found == 0) Serial.println(F("  nothing found - check SDA, SCL, 3.3 V and GND"));
}
void setup() {
  Serial.begin(115200);
  pinMode(PIN_BTN_ENGAGE, INPUT_PULLUP);
  pinMode(PIN_BTN_GAP, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_POT, ADC_11db);
  delay(400);
  Serial.println(F("TEST 5: OLED + knob + buttons"));
  Wire.begin(PIN_SDA, PIN_SCL);
  scanI2C();
  if (!oledBegin()) {
    Serial.printf("No OLED at 0x%02X. Try 0x3D (change OLED_ADDRESS at the top).\n", OLED_ADDRESS);
  } else {
    clearScreen();
    box(0, 0, 128, 64, true);          // every pixel on: check for dead lines
    oledShow();
    delay(700);
  }
}
void loop() {
  int raw = analogRead(PIN_POT);
  bool engage = digitalRead(PIN_BTN_ENGAGE) == LOW;
  bool gap = digitalRead(PIN_BTN_GAP) == LOW;
  float setSpeed = 0.20f + (0.80f - 0.20f) * (raw / 4095.0f);
  clearScreen();
  box(0, 0, 128, 20, false);                                   // the frame of the bar
  box(2, 2, max(2, (int)(124 * raw / 4095.0f)), 16, true);     // the bar itself
  box(4, 30, 50, 30, engage);                                  // ENGAGE button block
  box(74, 30, 50, 30, gap);                                    // GAP button block
  oledShow();
  Serial.printf("pot %4d -> set speed %.2f m/s   ENGAGE %s   GAP %s\n", raw, setSpeed,
                engage ? "PRESSED" : "up     ", gap ? "PRESSED" : "up");
  delay(100);
}
