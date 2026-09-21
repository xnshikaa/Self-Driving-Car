// =====================================================================
//  oled_display.h  —  the 0.96 inch OLED dashboard (SSD1306, I2C)
// =====================================================================
//  WHY OUR OWN DRIVER? The popular libraries are big and, worse, they
//  send the whole screen (1024 bytes) at once, which blocks the program
//  for about 25 ms. Our speed loop must run every 10 ms, so that would
//  ruin the control. Here we keep a copy of the screen in RAM, draw into
//  it (fast, no I2C), and send only ONE of the 8 pages at a time - about
//  3 ms of I2C. The whole screen is refreshed in 8 x 25 ms = 0.2 s.
//
//  HOW THE SSD1306 IS ORGANISED
//    * 128 columns x 64 rows of pixels
//    * memory is 8 PAGES of 128 bytes; one byte = 8 pixels stacked
//      vertically (bit 0 on top). Page 0 = rows 0-7, page 1 = rows 8-15 ...
//    * every I2C message starts with a control byte: 0x00 = commands
//      follow, 0x40 = pixel data follows
//
//  The dashboard we draw:
//      page 0      MODE       SET 0.50
//      page 1-2    big speed number  (2x tall and 2x wide)
//      page 3      GAP 0.48 M  (or GAP ----)
//      page 4      bar showing the gap against the set gap
//      page 6      LEAD 0.31  AI
//      page 7      BAT 11.8V
// =====================================================================
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "oled_font.h"
const uint8_t OLED_ADDRESS = 0x3C;      // some boards are 0x3D (see the solder blob on the back)
const int OLED_WIDTH = 128;
const int OLED_PAGES = 8;
class Display {
 public:
  bool begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(400000);                 // 400 kHz "fast mode"
    Wire.beginTransmission(OLED_ADDRESS);
    present_ = (Wire.endTransmission() == 0);
    if (!present_) return false;
    static const uint8_t INIT[] = {
      0xAE,              // display off
      0xD5, 0x80,        // clock divide / frequency
      0xA8, 0x3F,        // multiplex = 64 rows
      0xD3, 0x00,        // no vertical offset
      0x40,              // start line 0
      0x8D, 0x14,        // charge pump ON (the OLED makes its own high voltage)
      0x20, 0x00,        // horizontal addressing mode
      0xA1,              // left-right not mirrored
      0xC8,              // top-bottom not mirrored
      0xDA, 0x12,        // COM pin layout for a 128x64 panel
      0x81, 0xCF,        // contrast
      0xD9, 0xF1,        // pre-charge
      0xDB, 0x40,        // VCOMH level
      0xA4,              // show the RAM (not "all pixels on")
      0xA6,              // normal (not inverted)
      0xAF               // display on
    };
    for (uint8_t c : INIT) command(c);
    clear();
    for (int p = 0; p < OLED_PAGES; p++) flushPage(p);
    return true;
  }
  bool present() const { return present_; }
  void clear() {
    memset(buffer_, 0, sizeof(buffer_));
    dirty_ = 0xFF;
  }
  void clearPage(int page) {
    if (page < 0 || page >= OLED_PAGES) return;
    memset(&buffer_[page * OLED_WIDTH], 0, OLED_WIDTH);
    dirty_ |= (1 << page);
  }
  // Normal text: 5x7 pixels, 6 pixels apart, 21 characters per line.
  void print(int x, int page, const char *text) { draw(x, page, text, 1); }
  // Big text: every pixel becomes 2x2, so the characters are 2 pages tall.
  void printBig(int x, int page, const char *text) { draw(x, page, text, 2); }
  // A horizontal bar: "value" of "full" pixels are filled, inside a frame.
  void bar(int x, int page, int width, float fraction) {
    if (page < 0 || page >= OLED_PAGES) return;
    fraction = constrain(fraction, 0.0f, 1.0f);
    int filled = (int)(fraction * (width - 2));
    uint8_t *row = &buffer_[page * OLED_WIDTH];
    for (int i = 0; i < width; i++) {
      int col = x + i;
      if (col < 0 || col >= OLED_WIDTH) continue;
      if (i == 0 || i == width - 1) row[col] = 0x7E;           // the two ends of the frame
      else row[col] = (i <= filled) ? 0x7E : 0x42;             // filled block, or just top+bottom lines
    }
    dirty_ |= (1 << page);
  }
  // A small marker (an arrow tip) under one pixel column: shows the target.
  void marker(int x, int page) {
    if (page < 0 || page >= OLED_PAGES || x < 0 || x >= OLED_WIDTH) return;
    buffer_[page * OLED_WIDTH + x] = 0xFF;
    dirty_ |= (1 << page);
  }
  // Call often. Sends at most ONE page and only every OLED_PAGE_MS.
  void service(unsigned long nowMs) {
    if (!present_ || dirty_ == 0) return;
    if (nowMs - lastFlushMs_ < OLED_PAGE_MS) return;
    for (int i = 0; i < OLED_PAGES; i++) {
      int page = (nextPage_ + i) % OLED_PAGES;
      if (dirty_ & (1 << page)) {
        flushPage(page);
        dirty_ &= ~(1 << page);
        nextPage_ = (page + 1) % OLED_PAGES;
        lastFlushMs_ = nowMs;
        return;
      }
    }
  }
 private:
  void command(uint8_t c) {
    Wire.beginTransmission(OLED_ADDRESS);
    Wire.write(0x00);
    Wire.write(c);
    Wire.endTransmission();
  }
  void flushPage(int page) {
    command(0xB0 | page);      // page address
    command(0x00);             // column low nibble  = 0
    command(0x10);             // column high nibble = 0
    const uint8_t *src = &buffer_[page * OLED_WIDTH];
    for (int start = 0; start < OLED_WIDTH; start += 32) {     // 32 bytes per message (Wire buffer)
      Wire.beginTransmission(OLED_ADDRESS);
      Wire.write(0x40);
      Wire.write(&src[start], 32);
      Wire.endTransmission();
    }
  }
  void draw(int x, int page, const char *text, int scale) {
    for (const char *p = text; *p; p++) {
      char c = *p;
      if (c >= 'a' && c <= 'z') c -= 32;                       // lowercase -> capitals
      if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR) c = ' ';
      const uint8_t *glyph = &FONT5X7[(c - FONT_FIRST_CHAR) * FONT_WIDTH];
      for (int col = 0; col < FONT_WIDTH; col++) {
        uint8_t bits = pgm_read_byte(glyph + col);
        for (int s = 0; s < scale; s++) putColumn(x + (col * scale) + s, page, bits, scale);
      }
      for (int s = 0; s < scale; s++) putColumn(x + FONT_WIDTH * scale + s, page, 0, scale);
      x += (FONT_WIDTH + 1) * scale;
      if (x >= OLED_WIDTH) return;
    }
  }
  // Writes one 7-pixel column, stretched "scale" times vertically.
  void putColumn(int x, int page, uint8_t bits, int scale) {
    if (x < 0 || x >= OLED_WIDTH) return;
    uint16_t tall = 0;
    for (int b = 0; b < 7; b++) {
      if (bits & (1 << b)) {
        for (int s = 0; s < scale; s++) tall |= (uint16_t)1 << (b * scale + s);
      }
    }
    for (int extra = 0; extra < scale; extra++) {
      int p = page + extra;
      if (p < 0 || p >= OLED_PAGES) continue;
      buffer_[p * OLED_WIDTH + x] = (tall >> (extra * 8)) & 0xFF;
      dirty_ |= (1 << p);
    }
  }
  uint8_t buffer_[OLED_WIDTH * OLED_PAGES];
  uint8_t dirty_ = 0;
  int nextPage_ = 0;
  unsigned long lastFlushMs_ = 0;
  bool present_ = false;
};
