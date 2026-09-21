import { SimRNG } from './rng.js';
import { SimCar } from './Car.js';
import { SimRanger } from './Ranger.js';
import { GapFilter } from './GapFilter.js';
import { AccLogic } from './AccLogic.js';
import { LeadVehicle, PROFILES, START_GAP_M, profile_duration } from './LeadVehicle.js';
import {
  SPEED_LOOP_S, GAP_LOOP_S, TELEMETRY_S, PWM_MAX, SPEED_FF_PWM_START,
  SPEED_FF_PWM_PER_MPS, SPEED_INTEGRAL_BAND, SPEED_KP, SPEED_KI,
  PWM_MAX_STEP, BRAKE_ASSIST_MPS, PREDICTION_MAX_AGE_S, MODE_NAMES
} from './constants.js';
import { SET_SPEED_MPS, DEFAULT_GAP_M } from './config.js';

export function runScenario(name, options = {}) {
  const seed = options.seed !== undefined ? options.seed : 0;
  const rng = new SimRNG(seed);
  const set_speed = options.set_speed !== undefined ? options.set_speed : SET_SPEED_MPS;
  const gap_set = options.gap_set !== undefined ? options.gap_set : DEFAULT_GAP_M;
  const pieces = options.pieces || PROFILES[name] || PROFILES['stop_and_go'];
  const start_gap = options.start_gap !== undefined ? options.start_gap : (START_GAP_M[name] !== undefined ? START_GAP_M[name] : 1.0);
  const duration = options.duration || (profile_duration(pieces) + 2.0);

  const lead = new LeadVehicle(pieces, start_gap, 0.01, rng);
  const ranger = new SimRanger(rng);
  ranger.broken_from = options.sensor_breaks_at !== undefined ? options.sensor_breaks_at : null;
  if (options.ping_interval) {
    ranger.ping_interval = options.ping_interval;
  }

  const gap_filter = new GapFilter();
  const logic = new AccLogic();
  const car = new SimCar(rng);

  let pwm_command = 0.0;
  let integral = 0.0;
  let target_speed = 0.0;
  let hold_brake = true;
  let braking = false;
  let prediction = null;
  let prediction_time = -10.0;
  let last_distance = 0.0;

  const rows = [];
  const events = [];
  let collision = false;
  let min_true_gap = 99.0;
  const dt = 0.005;
  const steps = Math.floor(duration / dt);

  let next_speed_loop = 0.0;
  let next_gap_loop = 0.0;
  let next_log = 0.0;

  for (let step = 0; step < steps; step++) {
    const t = step * dt;
    lead.step(t, dt);

    const true_gap = lead.position - car.distance;
    min_true_gap = Math.min(min_true_gap, true_gap);

    if (true_gap <= 0.0) {
      collision = true;
      break;
    }

    if (ranger.update(t, true_gap)) {
      gap_filter.update(ranger.filtered, ranger.lag_s(), car.measured);
    }

    // Inner speed loop (100 Hz = 0.010 s)
    if (t >= next_speed_loop) {
      next_speed_loop += SPEED_LOOP_S;
      const speed = car.measure();
      gap_filter.predict(car.distance - last_distance, SPEED_LOOP_S);
      last_distance = car.distance;

      if (hold_brake || target_speed <= 0.0) {
        integral = 0.0;
        pwm_command = 0.0;
        braking = true;
      } else {
        const feed_forward = SPEED_FF_PWM_START + SPEED_FF_PWM_PER_MPS * target_speed;
        const error = target_speed - speed;
        let new_integral = integral;

        if (Math.abs(error) <= SPEED_INTEGRAL_BAND) {
          new_integral += error * SPEED_LOOP_S;
        }

        let correction = SPEED_KP * error + SPEED_KI * new_integral;
        const low = -feed_forward;
        const high = PWM_MAX - feed_forward;

        if (correction > high) {
          correction = high;
          if (error < 0) integral = new_integral;
        } else if (correction < low) {
          correction = low;
          if (error > 0) integral = new_integral;
        } else {
          integral = new_integral;
        }

        let wanted = Math.max(0.0, Math.min(PWM_MAX, feed_forward + correction));
        braking = speed > target_speed + BRAKE_ASSIST_MPS;
        if (braking) wanted = 0.0;

        const change = Math.max(-PWM_MAX_STEP, Math.min(PWM_MAX_STEP, wanted - pwm_command));
        pwm_command += change;
      }
    }

    // Outer gap loop (50 Hz = 0.020 s)
    if (t >= next_gap_loop) {
      next_gap_loop += GAP_LOOP_S;
      const fresh = (options.use_ai && (t - prediction_time <= PREDICTION_MAX_AGE_S)) ? prediction : null;
      const resultLogic = logic.step(
        t, GAP_LOOP_S, true, set_speed, gap_set, car.measured, gap_filter.tracking,
        gap_filter.gap_m(), gap_filter.lead_mps(), fresh, ranger.ok, false
      );

      target_speed = resultLogic.target_speed;
      hold_brake = resultLogic.hold_brake;

      if (resultLogic.changed) {
        events.push({ t, mode: MODE_NAMES[logic.mode], gap: gap_filter.gap_m(), speed: car.speed });
      }
    }

    car.step(dt, pwm_command, hold_brake || braking);

    // Telemetry logging (20 Hz = 0.050 s)
    if (t >= next_log) {
      next_log += TELEMETRY_S;
      rows.push({
        t: Number(t.toFixed(3)),
        esp_ms: Math.floor(t * 1000),
        mode: MODE_NAMES[logic.mode],
        speed_mps: Number(car.measured.toFixed(4)),
        target_mps: Number(target_speed.toFixed(4)),
        gap_m: Number(gap_filter.gap_m().toFixed(4)),
        raw_gap_m: Number(ranger.raw.toFixed(4)),
        lead_mps: Number(gap_filter.lead_mps().toFixed(4)),
        set_mps: set_speed,
        gap_set_m: gap_set,
        pwm: Math.floor(pwm_command),
        battery_v: 11.6,
        tracking: gap_filter.tracking ? 1 : 0,
        sensor_ok: ranger.ok ? 1 : 0,
        braking: (hold_brake || braking) ? 1 : 0,
        used_prediction: (options.use_ai && prediction !== null) ? 1 : 0,
        remote: 0,
        prediction_mps: prediction !== null ? Number(prediction.toFixed(4)) : "",
        true_gap_m: Number(true_gap.toFixed(4)),
        true_speed_mps: Number(car.speed.toFixed(4)),
        true_lead_mps: Number(lead.speed.toFixed(4)),
      });
    }
  }

  return {
    rows,
    result: {
      scenario: name,
      collision,
      min_true_gap_m: min_true_gap,
      events,
      seed
    }
  };
}
