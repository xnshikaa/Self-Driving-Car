// Deterministic PRNG providing random() and gauss(mean, std) matching Python random.Random behavior

export class SimRNG {
  constructor(seed = 0) {
    this.seed = seed >>> 0;
  }

  // Linear Congruential Generator / Mulberry32
  random() {
    let t = (this.seed += 0x6d2b79f5);
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  }

  uniform(a, b) {
    return a + (b - a) * this.random();
  }

  choice(arr) {
    const idx = Math.floor(this.random() * arr.length);
    return arr[idx];
  }

  gauss(mean = 0, std = 1) {
    let u = 0, v = 0;
    while (u === 0) u = this.random();
    while (v === 0) v = this.random();
    const num = Math.sqrt(-2.0 * Math.log(u)) * Math.cos(2.0 * Math.PI * v);
    return mean + num * std;
  }
}
