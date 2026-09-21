import {
  US_FAR_M, US_PING_S, US_MEDIAN_WINDOW, US_FAULT_COUNT, US_DROPOUT,
  US_MAX_RANGE_M, US_MIN_RANGE_M, US_NOISE_M, US_FAST_CLOSE_M
} from './constants.js';

export class SimRanger {
  constructor(rng) {
    this.rng = rng;
    this.history = [];
    this.raw = US_FAR_M;
    this.filtered = US_FAR_M;
    this.ok = true;
    this.misses = 0;
    this.miss_run = 0;
    this.next_ping = 0.0;
    this.broken_from = null;
    this.ping_interval = US_PING_S; // Configurable ping rate (default 0.050s = 20Hz)
  }

  lag_s() {
    return ((US_MEDIAN_WINDOW - 1) / 2.0) * this.ping_interval;
  }

  update(t, true_gap) {
    if (t < this.next_ping) {
      return false;
    }
    this.next_ping = t + this.ping_interval;

    if (this.broken_from !== null && t >= this.broken_from) {
      this.misses += 1;
      this.miss_run += 1;
      if (this.miss_run >= US_FAULT_COUNT) {
        this.ok = false;
      }
      return false;
    }

    let metres;
    if (this.rng.random() < US_DROPOUT) {
      metres = US_FAR_M;
    } else if (true_gap > US_MAX_RANGE_M) {
      metres = US_FAR_M;
    } else {
      metres = Math.max(US_MIN_RANGE_M, true_gap + this.rng.gauss(0.0, US_NOISE_M));
    }

    const previous = this.raw;
    this.raw = metres;
    this.history.push(metres);

    if (this.history.length > US_MEDIAN_WINDOW) {
      this.history.shift();
    }

    let filtered = US_FAR_M;
    if (this.history.length >= 3) {
      const sorted = [...this.history].sort((a, b) => a - b);
      filtered = sorted[Math.floor(sorted.length / 2)];
    }

    const closer = Math.max(metres, previous);
    if (this.history.length >= 2 && closer < filtered - US_FAST_CLOSE_M) {
      filtered = closer;
    }

    this.filtered = filtered;
    this.miss_run = 0;
    return true;
  }
}
