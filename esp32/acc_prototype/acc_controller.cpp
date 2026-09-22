#include "acc_controller.h"
#include "config.h"

const char *AccController::modeName(AccMode mode) {
  switch (mode) {
    case AccMode::OFF: return "OFF";
    case AccMode::CRUISE: return "CRUISE";
    case AccMode::FOLLOW: return "FOLLOW";
    case AccMode::STOP: return "STOP";
    case AccMode::FAULT: return "FAULT";
  }
  return "UNKNOWN";
}

void AccController::reset() {
  mode_ = AccMode::OFF;
  invalidCount_ = 0;
  lastPwm_ = 0;
}

int AccController::rampTo(int target) {
  target = constrain(target, 0, MAX_ALLOWED_PWM);
  const int delta = target - lastPwm_;
  if (delta > MAX_PWM_STEP) lastPwm_ += MAX_PWM_STEP;
  else if (delta < -MAX_PWM_STEP) lastPwm_ -= MAX_PWM_STEP;
  else lastPwm_ = target;
  return lastPwm_;
}

AccDecision AccController::update(bool runRequested, const DistanceReading &reading) {
  AccDecision out;
  out.distanceM = reading.metres;
  out.sensorValid = reading.valid;

  if (!runRequested) {
    reset();
    out.mode = mode_;
    return out;
  }

  if (!reading.valid) {
    if (invalidCount_ < 255) ++invalidCount_;
    mode_ = invalidCount_ >= INVALID_READINGS_TO_FAULT ? AccMode::FAULT : AccMode::STOP;
    lastPwm_ = 0;
    out.mode = mode_;
    return out;
  }
  invalidCount_ = 0;

  if (reading.metres <= EMERGENCY_GAP_M) {
    mode_ = AccMode::STOP;
    out.requestedPwm = 0;
    out.brake = true;
  } else if (reading.metres <= TARGET_GAP_M + GAP_TOLERANCE_M) {
    mode_ = AccMode::FOLLOW;
    const float error = reading.metres - TARGET_GAP_M;
    const int target = constrain(static_cast<int>(CRUISE_PWM + GAP_TO_PWM * error), CLOSE_PWM, CRUISE_PWM);
    out.requestedPwm = rampTo(target);
    out.brake = false;
  } else if (reading.metres < CLEAR_ROAD_M) {
    mode_ = AccMode::FOLLOW;
    const float error = reading.metres - TARGET_GAP_M;
    const int target = constrain(static_cast<int>(CRUISE_PWM + GAP_TO_PWM * error), MIN_MOVING_PWM, CRUISE_PWM);
    out.requestedPwm = rampTo(target);
    out.brake = false;
  } else {
    mode_ = AccMode::CRUISE;
    out.requestedPwm = rampTo(CRUISE_PWM);
    out.brake = false;
  }

  out.mode = mode_;
  return out;
}
