// =====================================================================
//  acc_controller.h  —  the brain of the adaptive cruise control
// =====================================================================
//  Three parts:
//
//  1. GapFilter (a Kalman filter). State = [gap, lead vehicle speed].
//     Every step it PREDICTS: the gap shrinks by how far WE drove
//     (from the speed sensor) and grows by how far the lead vehicle
//     drove. Every new ultrasonic reading CORRECTS that prediction.
//
//  2. AccLogic: the outer loop and the modes:
//       SPEED mode - no lead vehicle within 1.5 m: hold the set speed
//       GAP mode   - lead vehicle closer than 1.5 m: hold the set gap
//       STOP       - the lead vehicle stopped: we stop behind it
//       EMERGENCY  - the gap is below the safe minimum: brake now
//     The outer loop gives a TARGET SPEED to the inner speed PID.
//
//  3. Safety layer, checked first: battery low, sensor broken, or
//     gap < Dmin + the distance we need to stop -> EMERGENCY.
//
//  The simulator (raspberry_pi/sim/acc_sim.py) uses exactly the same
//  rules in Python. If you change a rule here, change it there too.
// =====================================================================
#pragma once
#include <Arduino.h>
#include "config.h"
enum AccMode : uint8_t {
  MODE_OFF = 0,        // ACC switched off (button) - the car stands still
  MODE_SPEED = 1,      // holding the set speed
  MODE_GAP = 2,        // holding the gap behind the lead vehicle
  MODE_STOP = 3,       // stopped behind a standing lead vehicle
  MODE_EMERGENCY = 4,  // gap too small: brake
  MODE_FAULT = 5,      // ultrasonic sensor not answering
  MODE_LOW_BATTERY = 6
};
const char *const MODE_NAMES[] = {"OFF", "SPEED", "GAP", "STOP", "EMERG", "FAULT", "LOW BAT"};
// ---------------------------------------------------------------------
// 1) Kalman filter: state = [gap (m), lead speed (m/s)]
// ---------------------------------------------------------------------
class GapFilter {
 public:
  // myDistance = how far WE drove since the last call (metres, from the speed sensor)
  void predict(float myDistance, float dt) {
    if (!tracking_) return;
    gap_ += leadSpeed_ * dt - myDistance;
    float q = KF_ACCEL_STD * KF_ACCEL_STD;
    float dt2 = dt * dt;
    float p00 = p00_ + dt * (p01_ + p10_) + dt2 * p11_ + q * dt2 * dt2 / 4.0f;
    float p01 = p01_ + dt * p11_ + q * dt2 * dt / 2.0f;
    float p11 = p11_ + q * dt2;
    p00_ = p00;
    p01_ = p10_ = p01;
    p11_ = p11;
  }
  // measured = filtered ultrasonic gap, lagS = how old it is, mySpeed = our speed now
  void update(float measured, float lagS, float mySpeed) {
    if (measured >= US_MAX_RANGE_M) {                 // nothing ahead any more
      if (tracking_ && ++farCount_ >= 3) tracking_ = false;
      return;
    }
    farCount_ = 0;
    if (!tracking_) {
      start(measured);
      return;
    }
    float z = measured + (leadSpeed_ - mySpeed) * lagS;   // move the old reading forward to now
    float innovation = z - gap_;
    if (innovation < -KF_GATE_M) {                    // MUCH closer than expected: believe it at once
      start(z);
      return;
    }
    if (innovation > KF_GATE_M) {                     // much further: wait for a second reading
      if (++suspicious_ >= 2) start(z);
      return;
    }
    suspicious_ = 0;
    float s = p00_ + KF_MEAS_STD_M * KF_MEAS_STD_M;
    float k0 = p00_ / s, k1 = p10_ / s;
    gap_ += k0 * innovation;
    leadSpeed_ = constrain(leadSpeed_ + k1 * innovation, -1.0f, 2.0f);
    float p00 = (1 - k0) * p00_, p01 = (1 - k0) * p01_;
    float p11 = p11_ - k1 * p01_;
    p00_ = p00;
    p01_ = p10_ = p01;
    p11_ = p11;
  }
  bool tracking() const { return tracking_; }
  float gapM() const { return tracking_ ? max(0.0f, gap_) : US_FAR_M; }
  float leadSpeed() const { return tracking_ ? leadSpeed_ : 0.0f; }
 private:
  // A new lead vehicle. The careful first guess: assume it is standing still.
  void start(float gap) {
    tracking_ = true;
    gap_ = gap;
    leadSpeed_ = 0;
    p00_ = KF_MEAS_STD_M * KF_MEAS_STD_M;
    p01_ = p10_ = 0;
    p11_ = 0.3f * 0.3f;
    suspicious_ = 0;
  }
  bool tracking_ = false;
  float gap_ = US_FAR_M, leadSpeed_ = 0;
  float p00_ = 1, p01_ = 0, p10_ = 0, p11_ = 1;
  int farCount_ = 0, suspicious_ = 0;
};
// ---------------------------------------------------------------------
// 2) + 3) Modes, gap controller and safety layer
// ---------------------------------------------------------------------
struct AccInputs {
  unsigned long nowMs = 0;
  float dt = 0.02f;
  bool engaged = false;       // the driver pressed the ACC button (or the Pi started a test)
  float setSpeed = 0.5f;      // from the knob
  float gapSet = 0.5f;        // from the GAP button
  float mySpeed = 0;          // from the speed sensor
  bool tracking = false;      // the gap filter is following a lead vehicle
  float gap = US_FAR_M;
  float leadSpeed = 0;        // Kalman estimate
  bool predictionValid = false;
  float predictedLeadSpeed = 0;   // from the AI model on the Pi (0.5 s ahead)
  bool sensorOk = true;
  bool batteryLow = false;
};
struct AccOutputs {
  float targetSpeed = 0;      // what the inner speed PID must hold
  bool holdBrake = false;     // press the brake (standing still or emergency)
  bool modeChanged = false;
  AccMode previous = MODE_OFF;
  float gapError = 0;
  float leadSpeedUsed = 0;    // the Kalman value, or the AI prediction if it was used
  bool usedPrediction = false;
  float minGapNow = 0;        // the dynamic safety distance at this speed
};
// The gap we must never come closer than: Dmin + how far we roll before we stop.
inline float dynamicMinGap(float speed) {
  if (speed <= 0) return D_MIN_M + EMERG_MARGIN_M;
  return D_MIN_M + EMERG_MARGIN_M + speed * (EMERG_REACTION_S + SENSOR_LAG_S) +
         speed * speed / (2.0f * EMERG_DECEL_MPS2);
}
class AccLogic {
 public:
  AccOutputs step(const AccInputs &in) {
    AccOutputs out;
    out.minGapNow = dynamicMinGap(in.mySpeed);
    out.gapError = in.tracking ? (in.gap - in.gapSet) : 0.0f;
    // ---------- which mode? Safety first ----------
    AccMode next = mode_;
    if (!in.engaged) {
      next = MODE_OFF;
    } else if (in.batteryLow) {
      next = MODE_LOW_BATTERY;
    } else if (!in.sensorOk) {
      next = MODE_FAULT;
    } else if (in.tracking && in.gap < out.minGapNow) {
      next = MODE_EMERGENCY;
    } else {
      if (next == MODE_OFF || next == MODE_FAULT || next == MODE_LOW_BATTERY) {
        next = MODE_SPEED;                                   // the problem is gone: start again
        gapIntegral_ = 0;
      }
      switch (next) {
        case MODE_SPEED:
          if (in.tracking && in.gap < GAP_MODE_ENTER_M) {
            next = MODE_GAP;
            bumpless(in);                                    // no jump in the target speed
          }
          break;
        case MODE_GAP:
          if (!in.tracking || in.gap > GAP_MODE_EXIT_M) {    // lead vehicle gone (0.5 s of hysteresis)
            if (farSinceMs_ == 0) farSinceMs_ = in.nowMs;
            if (in.nowMs - farSinceMs_ >= GAP_EXIT_HOLD_MS) next = MODE_SPEED;
          } else {
            farSinceMs_ = 0;
            // Stop behind the lead vehicle only when BOTH of us are standing still.
            // The lead-speed limit here (0.05) is lower than the one that makes us
            // drive off again (0.10), and a mode must last MODE_DWELL_MS. Without
            // those two rules the car would flicker between GAP and STOP.
            bool standing = in.mySpeed < STOP_SPEED_MPS && lastCommand_ < STOP_SPEED_MPS;
            bool leadStanding = in.leadSpeed < STOP_ENTER_LEAD_MPS;
            bool settled = in.nowMs - modeSinceMs_ >= MODE_DWELL_MS;
            if (standing && leadStanding && settled && in.gap < in.gapSet + RESUME_GAP_M) next = MODE_STOP;
          }
          break;
        case MODE_STOP: {
          // The gap estimate wobbles by a centimetre or two even when both cars
          // stand still. So we only drive off when the gap REALLY grew, or when
          // the lead vehicle has been moving for RESUME_HOLD_MS without a break.
          if (in.leadSpeed > RESUME_LEAD_MPS) {
            if (resumeSinceMs_ == 0) resumeSinceMs_ = in.nowMs;
          } else {
            resumeSinceMs_ = 0;
          }
          bool leadReallyMoving = resumeSinceMs_ != 0 && in.nowMs - resumeSinceMs_ >= RESUME_HOLD_MS;
          if (in.nowMs - modeSinceMs_ < MODE_DWELL_MS) break;   // wait before driving off again
          if (!in.tracking || in.gap > in.gapSet + RESUME_GAP_M || leadReallyMoving) {
            next = (in.tracking && in.gap < GAP_MODE_ENTER_M) ? MODE_GAP : MODE_SPEED;
            gapIntegral_ = 0;
            resumeSinceMs_ = 0;
          }
          break;
        }
        case MODE_EMERGENCY:
          if (in.mySpeed < STOP_SPEED_MPS && (!in.tracking || in.gap > D_MIN_M + EMERG_CLEAR_M)) {
            next = MODE_STOP;
            gapIntegral_ = 0;
          }
          break;
        default:
          break;
      }
    }
    if (next != mode_) {
      out.modeChanged = true;
      out.previous = mode_;
      mode_ = next;
      modeSinceMs_ = in.nowMs;
      if (mode_ != MODE_GAP) farSinceMs_ = 0;
    }
    // ---------- what speed do we want? ----------
    float wanted = 0;
    out.leadSpeedUsed = in.leadSpeed;
    if (mode_ == MODE_SPEED) {
      wanted = in.setSpeed;
    } else if (mode_ == MODE_GAP) {
      float leadFF = in.leadSpeed;
      if (in.predictionValid) {                              // the AI tells us what the lead will do
        leadFF = in.predictedLeadSpeed;
        out.usedPrediction = true;
        out.leadSpeedUsed = leadFF;
      }
      float error = in.gap - in.gapSet;
      float raw = leadFF + GAP_KP * error + GAP_KI * gapIntegral_;
      float limited = constrain(raw, 0.0f, in.setSpeed);
      // anti-windup: the I part may only grow while the answer is not clamped
      bool clampedHigh = raw > in.setSpeed && error > 0;
      bool clampedLow = raw < 0 && error < 0;
      if (!clampedHigh && !clampedLow) {
        float limit = GAP_I_LIMIT / GAP_KI;
        gapIntegral_ = constrain(gapIntegral_ + error * in.dt, -limit, limit);
      }
      wanted = limited;
    }
    // ---------- smooth it (comfort) or stop at once (safety) ----------
    if (mode_ == MODE_SPEED || mode_ == MODE_GAP) {
      float up = ACCEL_LIMIT_MPS2 * in.dt, down = DECEL_LIMIT_MPS2 * in.dt;
      lastCommand_ = constrain(wanted, lastCommand_ - down, lastCommand_ + up);
      out.targetSpeed = max(0.0f, lastCommand_);
      out.holdBrake = false;
    } else {
      lastCommand_ = 0;                                      // OFF, STOP, EMERGENCY, FAULT, LOW BATTERY
      out.targetSpeed = 0;
      out.holdBrake = true;
    }
    return out;
  }
  AccMode mode() const { return mode_; }
  unsigned long modeSinceMs() const { return modeSinceMs_; }
  float gapIntegral() const { return gapIntegral_; }
  float commandedSpeed() const { return lastCommand_; }
 private:
  // Entering GAP mode: pick the I part so the new target speed equals the old one (no jump).
  void bumpless(const AccInputs &in) {
    float leadFF = in.predictionValid ? in.predictedLeadSpeed : in.leadSpeed;
    float without = leadFF + GAP_KP * (in.gap - in.gapSet);
    float limit = GAP_I_LIMIT / GAP_KI;
    gapIntegral_ = constrain((lastCommand_ - without) / GAP_KI, -limit, limit);
  }
  AccMode mode_ = MODE_OFF;
  unsigned long modeSinceMs_ = 0, farSinceMs_ = 0, resumeSinceMs_ = 0;
  float gapIntegral_ = 0;
  float lastCommand_ = 0;      // the rate-limited target speed
};
