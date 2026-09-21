import {
  US_FAR_M, US_MAX_RANGE_M, KF_MEAS_STD_M, KF_ACCEL_STD, KF_GATE_M
} from './constants.js';

export class GapFilter {
  constructor() {
    this.tracking = false;
    this.gap = US_FAR_M;
    this.lead = 0.0;
    this.p = [[1.0, 0.0], [0.0, 1.0]];
    this.far_count = 0;
    this.suspicious = 0;
  }

  start(gap) {
    this.tracking = true;
    this.gap = gap;
    this.lead = 0.0;
    this.p = [[KF_MEAS_STD_M * KF_MEAS_STD_M, 0.0], [0.0, 0.09]];
    this.suspicious = 0;
  }

  predict(my_distance, dt) {
    if (!this.tracking) return;
    this.gap += this.lead * dt - my_distance;
    const q = KF_ACCEL_STD * KF_ACCEL_STD;
    const p = this.p;
    const p00 = p[0][0] + dt * (p[0][1] + p[1][0]) + dt * dt * p[1][1] + (q * Math.pow(dt, 4)) / 4.0;
    const p01 = p[0][1] + dt * p[1][1] + (q * Math.pow(dt, 3)) / 2.0;
    const p11 = p[1][1] + q * dt * dt;
    this.p = [[p00, p01], [p01, p11]];
  }

  update(measured, lag_s, my_speed) {
    if (measured >= US_MAX_RANGE_M) {
      if (this.tracking) {
        this.far_count += 1;
        if (this.far_count >= 3) {
          this.tracking = false;
        }
      }
      return;
    }

    this.far_count = 0;
    if (!this.tracking) {
      this.start(measured);
      return;
    }

    const z = measured + (this.lead - my_speed) * lag_s;
    const innovation = z - this.gap;

    if (innovation < -KF_GATE_M) {
      this.start(z);
      return;
    }

    if (innovation > KF_GATE_M) {
      this.suspicious += 1;
      if (this.suspicious >= 2) {
        this.start(z);
      }
      return;
    }

    this.suspicious = 0;
    const p = this.p;
    const s = p[0][0] + KF_MEAS_STD_M * KF_MEAS_STD_M;
    const k0 = p[0][0] / s;
    const k1 = p[1][0] / s;

    this.gap += k0 * innovation;
    this.lead = Math.max(-1.0, Math.min(2.0, this.lead + k1 * innovation));

    const p00 = (1 - k0) * p[0][0];
    const p01 = (1 - k0) * p[0][1];
    const p11 = p[1][1] - k1 * p[0][1];

    this.p = [[p00, p01], [p01, p11]];
  }

  gap_m() {
    return this.tracking ? Math.max(0.0, this.gap) : US_FAR_M;
  }

  lead_mps() {
    return this.tracking ? this.lead : 0.0;
  }
}
