#include "../../acc_prototype/config.h"
#include "../../acc_prototype/acc_controller.h"
// Include the implementation so this standalone Arduino sketch exercises the
// same controller code without compiling the motor/sensor/OLED modules.
#include "../../acc_prototype/acc_controller.cpp"

AccController controller;

void setup() {
  Serial.begin(115200);
  Serial.println("ACC logic dry run; no motor pins are used.");
  const float distances[] = {3.0f, 0.80f, 0.55f, 0.35f, 0.18f, NAN, 3.0f};
  for (float d : distances) {
    DistanceReading r;
    r.metres = d;
    r.valid = !isnan(d);
    const AccDecision out = controller.update(true, r);
    Serial.printf("input_m=%s mode=%s requested_pwm=%d brake=%d\n",
                  r.valid ? String(d, 2).c_str() : "INVALID",
                  AccController::modeName(out.mode), out.requestedPwm, out.brake ? 1 : 0);
  }
  controller.reset();
}

void loop() {}
