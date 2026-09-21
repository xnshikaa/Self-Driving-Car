import {
  BRAKE_DECEL_MPS2, REAL_FF_START, REAL_FF_PER_MPS, SPEED_TAU_S,
  SPEED_NOISE_MPS, SPEED_FILTER_ALPHA
} from './constants.js';

export class SimCar {
  constructor(rng) {
    this.rng = rng;
    this.speed = 0.0;     // TRUE speed (m/s)
    this.distance = 0.0;  // TRUE distance driven (m)
    this.measured = 0.0;  // Measured speed feedback (noisy and filtered)
  }

  step(dt, pwm, braking) {
    if (braking) {
      this.speed = Math.max(0.0, this.speed - BRAKE_DECEL_MPS2 * dt);
    } else {
      const steady = pwm > 0 ? Math.max(0.0, (pwm - REAL_FF_START) / REAL_FF_PER_MPS) : 0.0;
      this.speed += (steady - this.speed) * (dt / SPEED_TAU_S);
      this.speed = Math.max(0.0, this.speed);
    }
    this.distance += this.speed * dt;
  }

  measure() {
    const noisy = Math.max(0.0, this.speed + this.rng.gauss(0.0, SPEED_NOISE_MPS));
    this.measured += SPEED_FILTER_ALPHA * (noisy - this.measured);
    return this.measured;
  }
}
