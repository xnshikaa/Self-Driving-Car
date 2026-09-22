#include "oled_ui.h"
#include "config.h"

bool OledUi::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  present_ = display_.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (!present_) return false;
  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  display_.setTextSize(1);
  display_.setCursor(0, 0);
  display_.println("ACC prototype");
  display_.display();
  return true;
}

void OledUi::show(const AccDecision &decision, int appliedPwm, bool runRequested) {
  if (!present_) return;
  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setCursor(0, 0);
  display_.print("MODE: ");
  display_.println(AccController::modeName(decision.mode));
  display_.print("RUN:  ");
  display_.println(runRequested ? "YES" : "NO");
  display_.print("DIST: ");
  if (decision.sensorValid) {
    display_.print(decision.distanceM, 2);
    display_.println(" m");
  } else {
    display_.println("INVALID");
  }
  display_.print("GAP:  ");
  display_.print(TARGET_GAP_M, 2);
  display_.println(" m");
  display_.print("PWM:  ");
  display_.print(appliedPwm);
  display_.print("/");
  display_.println(MAX_ALLOWED_PWM);
  display_.println("USB serial = detail");
  display_.display();
}
