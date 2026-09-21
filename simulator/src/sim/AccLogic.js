import {
  MODE_OFF, MODE_SPEED, MODE_GAP, MODE_STOP, MODE_EMERG, MODE_FAULT, MODE_LOWBAT, MODE_NAMES,
  D_MIN_M, EMERG_MARGIN_M, EMERG_REACTION_S, SENSOR_LAG_S, EMERG_DECEL_MPS2,
  GAP_MODE_ENTER_M, GAP_MODE_EXIT_M, GAP_EXIT_HOLD_S, STOP_SPEED_MPS, STOP_ENTER_LEAD_MPS,
  MODE_DWELL_S, RESUME_GAP_M, RESUME_LEAD_MPS, RESUME_HOLD_S, EMERG_CLEAR_M,
  GAP_KP, GAP_KI, GAP_I_LIMIT, ACCEL_LIMIT_MPS2, DECEL_LIMIT_MPS2
} from './constants.js';

export function dynamic_min_gap(speed) {
  if (speed <= 0) {
    return D_MIN_M + EMERG_MARGIN_M;
  }
  return (
    D_MIN_M +
    EMERG_MARGIN_M +
    speed * (EMERG_REACTION_S + SENSOR_LAG_S) +
    (speed * speed) / (2.0 * EMERG_DECEL_MPS2)
  );
}

export class AccLogic {
  constructor() {
    this.mode = MODE_OFF;
    this.gap_integral = 0.0;
    this.last_command = 0.0;
    this.far_since = null;
    this.mode_since = 0.0;
    this.resume_since = null;
  }

  _bumpless(gap, gap_set, lead_ff) {
    const without = lead_ff + GAP_KP * (gap - gap_set);
    const limit = GAP_I_LIMIT / GAP_KI;
    this.gap_integral = Math.max(-limit, Math.min(limit, (this.last_command - without) / GAP_KI));
  }

  step(
    t, dt, engaged, set_speed, gap_set, my_speed, tracking, gap, lead,
    prediction = null, sensor_ok = true, battery_low = false
  ) {
    const min_gap = dynamic_min_gap(my_speed);
    let mode = this.mode;

    if (!engaged) {
      mode = MODE_OFF;
    } else if (battery_low) {
      mode = MODE_LOWBAT;
    } else if (!sensor_ok) {
      mode = MODE_FAULT;
    } else if (tracking && gap < min_gap) {
      mode = MODE_EMERG;
    } else {
      if (mode === MODE_OFF || mode === MODE_FAULT || mode === MODE_LOWBAT) {
        mode = MODE_SPEED;
        this.gap_integral = 0.0;
      }

      if (mode === MODE_SPEED) {
        if (tracking && gap < GAP_MODE_ENTER_M) {
          mode = MODE_GAP;
          this._bumpless(gap, gap_set, prediction === null ? lead : prediction);
        }
      } else if (mode === MODE_GAP) {
        if (!tracking || gap > GAP_MODE_EXIT_M) {
          if (this.far_since === null) {
            this.far_since = t;
          }
          if (t - this.far_since >= GAP_EXIT_HOLD_S) {
            mode = MODE_SPEED;
          }
        } else {
          this.far_since = null;
          const standing = my_speed < STOP_SPEED_MPS && this.last_command < STOP_SPEED_MPS;
          const lead_standing = lead < STOP_ENTER_LEAD_MPS;
          const settled = t - this.mode_since >= MODE_DWELL_S;

          if (standing && lead_standing && settled && gap < gap_set + RESUME_GAP_M) {
            mode = MODE_STOP;
          }
        }
      } else if (mode === MODE_STOP) {
        if (lead > RESUME_LEAD_MPS) {
          if (this.resume_since === null) {
            this.resume_since = t;
          }
        } else {
          this.resume_since = null;
        }

        const really_moving = this.resume_since !== null && t - this.resume_since >= RESUME_HOLD_S;

        if (t - this.mode_since < MODE_DWELL_S) {
          // Wait dwell time before driving off
        } else if (!tracking || gap > gap_set + RESUME_GAP_M || really_moving) {
          mode = tracking && gap < GAP_MODE_ENTER_M ? MODE_GAP : MODE_SPEED;
          this.gap_integral = 0.0;
          this.resume_since = null;
        }
      } else if (mode === MODE_EMERG) {
        if (my_speed < STOP_SPEED_MPS && (!tracking || gap > D_MIN_M + EMERG_CLEAR_M)) {
          mode = MODE_STOP;
          this.gap_integral = 0.0;
        }
      }
    }

    const changed = mode !== this.mode;
    this.mode = mode;
    if (changed) {
      this.mode_since = t;
      if (mode !== MODE_GAP) {
        this.far_since = null;
      }
    }

    let used_prediction = false;
    let wanted = 0.0;

    if (mode === MODE_SPEED) {
      wanted = set_speed;
    } else if (mode === MODE_GAP) {
      let lead_ff = lead;
      if (prediction !== null) {
        lead_ff = prediction;
        used_prediction = true;
      }

      const error = gap - gap_set;
      const raw = lead_ff + GAP_KP * error + GAP_KI * this.gap_integral;
      const limited = Math.max(0.0, Math.min(set_speed, raw));

      const clamped_high = raw > set_speed && error > 0;
      const clamped_low = raw < 0 && error < 0;

      if (!clamped_high && !clamped_low) {
        const limit = GAP_I_LIMIT / GAP_KI;
        this.gap_integral = Math.max(-limit, Math.min(limit, this.gap_integral + error * dt));
      }

      wanted = limited;
    }

    if (mode === MODE_SPEED || mode === MODE_GAP) {
      const up = ACCEL_LIMIT_MPS2 * dt;
      const down = DECEL_LIMIT_MPS2 * dt;
      this.last_command = Math.max(this.last_command - down, Math.min(this.last_command + up, wanted));
      return { target_speed: Math.max(0.0, this.last_command), hold_brake: false, changed, min_gap, used_prediction };
    }

    this.last_command = 0.0;
    return { target_speed: 0.0, hold_brake: true, changed, min_gap, used_prediction };
  }
}
