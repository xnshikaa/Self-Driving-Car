export const LEAD_ACCEL_MPS2 = 0.6;
export const LEAD_DECEL_MPS2 = 1.2;

export const PROFILES = {
  constant: [[2.0, 0.0], [10.0, 0.30]],
  slow_down: [[2.0, 0.0], [5.0, 0.42], [6.0, 0.18], [4.0, 0.18]],
  speed_up: [[2.0, 0.0], [5.0, 0.22], [6.0, 0.42], [3.0, 0.42]],
  stop: [[2.0, 0.0], [5.0, 0.40], [6.0, 0.00]],
  stop_and_go: [[2.0, 0.0], [4.0, 0.40], [4.0, 0.00], [5.0, 0.40], [3.0, 0.00]],
  cut_in: [[3.0, 0.0], [8.0, 0.35]],
  wavy: [[2.0, 0.0], [3.5, 0.42], [3.5, 0.20], [3.5, 0.42], [3.5, 0.20], [3.5, 0.38]],
  none: [[12.0, 0.0]],
};

export const START_GAP_M = {
  constant: 1.20,
  slow_down: 1.00,
  speed_up: 0.60,
  stop: 1.20,
  stop_and_go: 0.90,
  cut_in: 0.45,
  wavy: 0.90,
  none: 9.00,
};

export function profile_speed(pieces, t) {
  let start = 0.0;
  for (const [duration, speed] of pieces) {
    if (t < start + duration) {
      return speed;
    }
    start += duration;
  }
  return pieces[pieces.length - 1][1];
}

export function profile_duration(pieces) {
  return pieces.reduce((sum, [dur]) => sum + dur, 0.0);
}

export class LeadVehicle {
  constructor(pieces, start_gap_m, noise = 0.0, rng = null) {
    this.pieces = pieces;
    this.position = start_gap_m;
    this.speed = 0.0;
    this.noise = noise;
    this.rng = rng;
  }

  step(t, dt) {
    let wanted = profile_speed(this.pieces, t);
    if (this.noise && this.rng) {
      wanted = Math.max(0.0, wanted + this.rng.gauss(0.0, this.noise));
    }
    if (wanted > this.speed) {
      this.speed = Math.min(wanted, this.speed + LEAD_ACCEL_MPS2 * dt);
    } else {
      this.speed = Math.max(wanted, this.speed - LEAD_DECEL_MPS2 * dt);
    }
    this.position += this.speed * dt;
    return this.speed;
  }
}
