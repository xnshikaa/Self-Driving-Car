#include "config.h"
#include "distance_sensor.h"
#include "motor_driver.h"
#include "input_switch.h"
#include "acc_controller.h"
#include "oled_ui.h"

DistanceSensor distanceSensor;
MotorDriver motor;
RunSwitch runSwitch;
AccController controller;
OledUi oled;

uint32_t lastSensorMs = 0;
uint32_t lastControlMs = 0;
uint32_t lastDisplayMs = 0;
uint32_t lastSerialMs = 0;
DistanceReading latestReading;
AccDecision latestDecision;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("=== Low-cost ESP32 ACC prototype ==="));
  Serial.println(F("Edit config.h pin placeholders before connecting hardware."));
  distanceSensor.begin();
  motor.begin();
  runSwitch.begin();
  const bool oledOk = oled.begin();
  Serial.print(F("OLED: "));
  Serial.println(oledOk ? F("OK") : F("NOT FOUND"));
  motor.stop();
  controller.reset();
}

void loop() {
  const uint32_t now = millis();
  runSwitch.update(now);

  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;
    latestReading = distanceSensor.readFiltered();
  }

  if (now - lastControlMs >= CONTROL_PERIOD_MS) {
    lastControlMs = now;
    latestDecision = controller.update(runSwitch.runRequested(), latestReading);
    motor.apply({latestDecision.requestedPwm, latestDecision.brake});
  }

  if (now - lastDisplayMs >= DISPLAY_PERIOD_MS) {
    lastDisplayMs = now;
    oled.show(latestDecision, motor.appliedPwm(), runSwitch.runRequested());
  }

  if (now - lastSerialMs >= SERIAL_PERIOD_MS) {
    lastSerialMs = now;
    Serial.print(F("mode="));
    Serial.print(AccController::modeName(latestDecision.mode));
    Serial.print(F(",run="));
    Serial.print(runSwitch.runRequested() ? 1 : 0);
    Serial.print(F(",valid="));
    Serial.print(latestReading.valid ? 1 : 0);
    Serial.print(F(",distance_m="));
    if (latestReading.valid) Serial.print(latestReading.metres, 3);
    else Serial.print(F("nan"));
    Serial.print(F(",requested_pwm="));
    Serial.print(latestDecision.requestedPwm);
    Serial.print(F(",applied_pwm="));
    Serial.println(motor.appliedPwm());
  }
}
