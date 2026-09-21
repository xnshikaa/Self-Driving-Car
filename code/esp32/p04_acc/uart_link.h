// =====================================================================
//  uart_link.h  —  the "language" the Raspberry Pi and the ESP32 speak
// =====================================================================
//  Every message is ONE line of text:     $TYPE,field,field,...*CS
//  CS = checksum: two hex digits = XOR of all characters between $ and *.
//  If a character is damaged on the wire the checksum will not match and
//  the message is thrown away (GPS modules use exactly the same trick).
//
//  Whole numbers travel on the wire, not decimals: speeds in mm/s and
//  distances in mm. Integers are short and can never be "0.30000001".
//
//  Raspberry Pi -> ESP32
//    $M,seq,on,speed_mm_s*CS   remote drive: on=1 hold this speed (tools),
//                              on=0 give control back to the driver
//    $G,set_mm_s,gap_mm,engage*CS  remote dashboard: set speed, gap setting
//                              and the ACC on/off switch (repeatable trials)
//    $P,lead_mm_s,horizon_ms*CS    the AI prediction of the lead speed
//    $K,kp,ki,gap_kp_x100,gap_ki_x100*CS   new gains while tuning
//    $S*CS                     stop and switch ACC off
//  ESP32 -> Raspberry Pi   (20 times per second)
//    $T,ms,mode,speed_mm_s,target_mm_s,gap_mm,raw_mm,lead_mm_s,set_mm_s,
//       gapset_mm,pwm,battery_mV,flags*CS
//    $E,ms,event,speed_mm_s,gap_mm,value*CS     (events: see p04_acc.ino)
//
//  flags: bit0 tracking, bit1 sensor ok, bit2 brake, bit3 AI prediction used,
//         bit4 remote drive, bit5 battery low
//
//  You can also type short commands in the Arduino Serial Monitor
//  (115200 baud, line ending "Newline"):
//     e        ACC on / off            s 0.5   set speed 0.5 m/s
//     g 0.4    gap setting 0.4 m       v 0.3   remote drive at 0.3 m/s
//     p 0.25   pretend the AI predicts 0.25 m/s   x  stop    ?  help
// =====================================================================
#pragma once
#include <Arduino.h>
struct Command {
  char type = 0;        // 'M' remote drive, 'G' dashboard, 'P' prediction,
                        // 'K' gains, 'S' stop, 'E' engage toggle, 'H' help
  bool manual = false;  // true = typed by a person in the Serial Monitor
  long seq = 0;
  bool on = false;
  float speedMps = 0;
  float setSpeedMps = 0;
  float gapSetM = 0;
  bool engage = false;
  bool hasEngage = false;
  float leadMps = 0;
  int horizonMs = 0;
  float kp = 0, ki = 0, gapKp = 0, gapKi = 0;
};
inline uint8_t frameChecksum(const char *text, size_t length) {
  uint8_t cs = 0;
  for (size_t i = 0; i < length; i++) cs ^= (uint8_t)text[i];
  return cs;
}
// Sends  "$" + body + "*CS" + newline
inline void sendFrame(Stream &port, const char *body) {
  char tail[6];
  snprintf(tail, sizeof(tail), "*%02X\n", frameChecksum(body, strlen(body)));
  port.write('$');
  port.print(body);
  port.print(tail);
}
class CommandReader {
 public:
  CommandReader(Stream &port, bool allowManual) : port_(port), allowManual_(allowManual) {}
  // Call very often. Returns true when a complete, valid command arrived.
  bool poll(Command &command) {
    while (port_.available() > 0) {
      char c = (char)port_.read();
      if (c == '\r') continue;
      if (c == '\n') {
        line_[length_] = '\0';
        bool good = (length_ > 0) && parseLine(line_, command);
        length_ = 0;
        if (good) return true;
        continue;
      }
      if (c == '$') length_ = 0;  // a '$' always starts a fresh message
      if (length_ < sizeof(line_) - 1) {
        line_[length_++] = c;
      } else {
        length_ = 0;              // far too long: must be garbage
      }
    }
    return false;
  }
  unsigned long badFrames() const { return badFrames_; }
 private:
  bool parseLine(char *line, Command &command) {
    if (line[0] == '$') return parseFrame(line, command);
    return allowManual_ && parseManual(line, command);
  }
  bool parseFrame(char *line, Command &command) {
    char *star = strchr(line, '*');
    if (star == nullptr || strlen(star) < 3) return reject();
    uint8_t expected = (uint8_t)strtol(star + 1, nullptr, 16);
    if (frameChecksum(line + 1, (size_t)(star - line - 1)) != expected) return reject();
    *star = '\0';  // cut off "*CS"
    char *fields[8];
    int count = 0;
    char *save = nullptr;
    for (char *tok = strtok_r(line + 1, ",", &save); tok != nullptr && count < 8;
         tok = strtok_r(nullptr, ",", &save)) {
      fields[count++] = tok;
    }
    if (count == 0) return reject();
    Command c;
    c.type = fields[0][0];
    if (c.type == 'M' && count == 4) {
      c.seq = strtol(fields[1], nullptr, 10);
      c.on = strtol(fields[2], nullptr, 10) != 0;
      c.speedMps = strtol(fields[3], nullptr, 10) / 1000.0f;
    } else if (c.type == 'G' && count == 4) {
      c.setSpeedMps = strtol(fields[1], nullptr, 10) / 1000.0f;
      c.gapSetM = strtol(fields[2], nullptr, 10) / 1000.0f;
      c.engage = strtol(fields[3], nullptr, 10) != 0;
      c.hasEngage = true;
    } else if (c.type == 'P' && count == 3) {
      c.leadMps = strtol(fields[1], nullptr, 10) / 1000.0f;
      c.horizonMs = (int)strtol(fields[2], nullptr, 10);
    } else if (c.type == 'K' && count == 5) {
      c.kp = strtof(fields[1], nullptr);
      c.ki = strtof(fields[2], nullptr);
      c.gapKp = strtol(fields[3], nullptr, 10) / 100.0f;
      c.gapKi = strtol(fields[4], nullptr, 10) / 100.0f;
    } else if (c.type == 'S') {
      // nothing to read
    } else {
      return reject();
    }
    command = c;
    return true;
  }
  bool parseManual(char *line, Command &command) {
    while (*line == ' ') line++;
    Command c;
    c.manual = true;
    char key = (char)tolower(line[0]);
    char *rest = line + 1;
    if (key == 'v') {
      c.type = 'M';
      c.on = true;
      c.speedMps = strtof(rest, nullptr);
    } else if (key == 'x') {
      c.type = 'S';
    } else if (key == 'e') {
      c.type = 'E';
    } else if (key == 's') {
      c.type = 'G';
      c.setSpeedMps = strtof(rest, nullptr);
    } else if (key == 'g') {
      c.type = 'G';
      c.gapSetM = strtof(rest, nullptr);
    } else if (key == 'p') {
      c.type = 'P';
      c.leadMps = strtof(rest, nullptr);
      c.horizonMs = 500;
    } else if (key == 'k') {
      char *end = rest;
      c.type = 'K';
      c.kp = strtof(end, &end);
      c.ki = strtof(end, &end);
      c.gapKp = strtof(end, &end);
      c.gapKi = strtof(end, &end);
    } else if (key == '?') {
      c.type = 'H';
    } else {
      return false;
    }
    command = c;
    return true;
  }
  bool reject() {
    badFrames_++;
    return false;
  }
  Stream &port_;
  bool allowManual_;
  char line_[96];
  size_t length_ = 0;
  unsigned long badFrames_ = 0;
};
