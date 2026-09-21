"""
acc_sim.py - a complete simulation of the ACC car, with NO hardware at all.
WHY A SIMULATOR? Because a real car takes 5 minutes per test, runs its
battery flat and can crash into the lead vehicle. Here a test takes 20
milliseconds, so you can run hundreds of them, try new numbers and only
then go to the floor. Everything that matters is copied from the firmware:
    * the same Kalman gap filter          (acc_controller.h)
    * the same modes and the same rules   (acc_controller.h)
    * the same speed PID and feed-forward (p04_acc.ino)
    * the same ultrasonic sensor timing, noise and dropouts
WHAT IS PRETENDED
    * the car answers the motor like a first-order system (it takes about
      0.3 s to reach a new speed) and its real feed-forward numbers are a
      bit different from the ones in config.h - so the PID has work to do,
      exactly as on the floor
    * the ultrasonic sensor has +/- 1.5 cm of noise and misses 2 % of pings
USE IT
    python3 acc_sim.py                          # one run of stop_and_go
    python3 acc_sim.py --scenario stop --log    # save a CSV like a real run
    python3 acc_sim.py --all                    # every scenario, one table
    python3 acc_sim.py --trials 50              # 50 random runs: safety statistics
    python3 acc_sim.py --collect 30             # 30 random runs -> training data
    python3 acc_sim.py --all --ai               # with the AI prediction switched on
"""
import argparse
import csv
import math
import os
import random
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))    # so we can import config.py and acc_metrics.py
sys.path.insert(0, HERE)                     # so we can import lead_profiles.py
import config            # noqa: E402  (the Pi settings)
import lead_profiles     # noqa: E402
# ----------------------------------------------------------------------
#  COPIED FROM esp32/p04_acc/config.h - keep the two files in step!
# ----------------------------------------------------------------------
SPEED_LOOP_S = 0.010
GAP_LOOP_S = 0.020
PWM_MAX = 1023
SPEED_FF_PWM_START = 150.0
SPEED_FF_PWM_PER_MPS = 1000.0
SPEED_KP = 600.0
SPEED_KI = 3000.0
SPEED_INTEGRAL_BAND = 0.15
SPEED_FILTER_ALPHA = 0.30
PWM_MAX_STEP = 25
BRAKE_ASSIST_MPS = 0.12
GAP_MODE_ENTER_M = 1.50
GAP_MODE_EXIT_M = 1.70
GAP_EXIT_HOLD_S = 0.50
GAP_KP = 0.80
GAP_KI = 0.10
GAP_I_LIMIT = 0.15
ACCEL_LIMIT_MPS2 = 0.70
DECEL_LIMIT_MPS2 = 1.50
STOP_SPEED_MPS = 0.03
RESUME_GAP_M = 0.08
RESUME_LEAD_MPS = 0.10
STOP_ENTER_LEAD_MPS = 0.05
MODE_DWELL_S = 0.30
RESUME_HOLD_S = 0.20
D_MIN_M = 0.25
EMERG_REACTION_S = 0.10
SENSOR_LAG_S = 0.10
EMERG_DECEL_MPS2 = 2.00
EMERG_MARGIN_M = 0.02
EMERG_CLEAR_M = 0.10
US_PING_S = 0.050
US_MAX_RANGE_M = 3.0
US_FAR_M = 4.0
US_MIN_RANGE_M = 0.03
US_MEDIAN_WINDOW = 5
US_FAST_CLOSE_M = 0.15
US_FAULT_COUNT = 3
KF_MEAS_STD_M = 0.02
KF_ACCEL_STD = 1.0
KF_GATE_M = 0.35
PREDICTION_MAX_AGE_S = 0.20
TELEMETRY_S = 0.050
# ----------------------------------------------------------------------
#  THE PRETEND CAR (this part is NOT in the firmware)
# ----------------------------------------------------------------------
REAL_FF_START = 168.0        # the PWM the real car needs to start moving
REAL_FF_PER_MPS = 1085.0     # ... and per m/s  (on purpose different from config.h)
SPEED_TAU_S = 0.30           # how long the car takes to reach a new speed
BRAKE_DECEL_MPS2 = 2.2       # how hard the electric brake slows it down
SPEED_NOISE_MPS = 0.010      # noise in the measured speed
US_NOISE_M = 0.015           # noise in one ultrasonic reading
US_DROPOUT = 0.02            # 2 % of pings come back as nothing
MODE_NAMES = ["OFF", "SPEED", "GAP", "STOP", "EMERG", "FAULT", "LOW BAT"]
MODE_OFF, MODE_SPEED, MODE_GAP, MODE_STOP, MODE_EMERG, MODE_FAULT, MODE_LOWBAT = range(7)
# ----------------------------------------------------------------------
#  1) The ultrasonic sensor: 20 pings per second, median of 5, fast-close
#     (the same rules as ultrasonic_ranger.cpp)
# ----------------------------------------------------------------------
class SimRanger:
    def __init__(self, rng):
        self.rng = rng
        self.history = []
        self.raw = US_FAR_M
        self.filtered = US_FAR_M
        self.ok = True
        self.misses = 0
        self.miss_run = 0
        self.next_ping = 0.0
        self.broken_from = None       # set a time to simulate a broken sensor
    def lag_s(self):
        return ((US_MEDIAN_WINDOW - 1) / 2.0) * US_PING_S
    def update(self, t, true_gap):
        if t < self.next_ping:
            return False
        self.next_ping = t + US_PING_S
        if self.broken_from is not None and t >= self.broken_from:
            self.misses += 1
            self.miss_run += 1
            if self.miss_run >= US_FAULT_COUNT:
                self.ok = False
            return False
        if self.rng.random() < US_DROPOUT:
            metres = US_FAR_M                                  # the echo was lost: "nothing there"
        elif true_gap > US_MAX_RANGE_M:
            metres = US_FAR_M
        else:
            metres = max(US_MIN_RANGE_M, true_gap + self.rng.gauss(0.0, US_NOISE_M))
        previous = self.raw
        self.raw = metres
        self.history.append(metres)
        if len(self.history) > US_MEDIAN_WINDOW:
            self.history.pop(0)
        filtered = sorted(self.history)[len(self.history) // 2] if len(self.history) >= 3 else US_FAR_M
        closer = max(metres, previous)                          # the careful one of the last two
        if len(self.history) >= 2 and closer < filtered - US_FAST_CLOSE_M:
            filtered = closer
        self.filtered = filtered
        self.miss_run = 0
        return True
# ----------------------------------------------------------------------
#  2) The Kalman gap filter (the same maths as GapFilter in acc_controller.h)
# ----------------------------------------------------------------------
class GapFilter:
    def __init__(self):
        self.tracking = False
        self.gap = US_FAR_M
        self.lead = 0.0
        self.p = [[1.0, 0.0], [0.0, 1.0]]
        self.far_count = 0
        self.suspicious = 0
    def start(self, gap):
        self.tracking = True
        self.gap = gap
        self.lead = 0.0
        self.p = [[KF_MEAS_STD_M ** 2, 0.0], [0.0, 0.09]]
        self.suspicious = 0
    def predict(self, my_distance, dt):
        if not self.tracking:
            return
        self.gap += self.lead * dt - my_distance
        q = KF_ACCEL_STD ** 2
        p = self.p
        p00 = p[0][0] + dt * (p[0][1] + p[1][0]) + dt * dt * p[1][1] + q * dt ** 4 / 4.0
        p01 = p[0][1] + dt * p[1][1] + q * dt ** 3 / 2.0
        p11 = p[1][1] + q * dt * dt
        self.p = [[p00, p01], [p01, p11]]
    def update(self, measured, lag_s, my_speed):
        if measured >= US_MAX_RANGE_M:
            if self.tracking:
                self.far_count += 1
                if self.far_count >= 3:
                    self.tracking = False
            return
        self.far_count = 0
        if not self.tracking:
            self.start(measured)
            return
        z = measured + (self.lead - my_speed) * lag_s
        innovation = z - self.gap
        if innovation < -KF_GATE_M:
            self.start(z)
            return
        if innovation > KF_GATE_M:
            self.suspicious += 1
            if self.suspicious >= 2:
                self.start(z)
            return
        self.suspicious = 0
        p = self.p
        s = p[0][0] + KF_MEAS_STD_M ** 2
        k0, k1 = p[0][0] / s, p[1][0] / s
        self.gap += k0 * innovation
        self.lead = max(-1.0, min(2.0, self.lead + k1 * innovation))
        p00 = (1 - k0) * p[0][0]
        p01 = (1 - k0) * p[0][1]
        p11 = p[1][1] - k1 * p[0][1]
        self.p = [[p00, p01], [p01, p11]]
    def gap_m(self):
        return max(0.0, self.gap) if self.tracking else US_FAR_M
    def lead_mps(self):
        return self.lead if self.tracking else 0.0
def dynamic_min_gap(speed):
    if speed <= 0:
        return D_MIN_M + EMERG_MARGIN_M
    return (D_MIN_M + EMERG_MARGIN_M + speed * (EMERG_REACTION_S + SENSOR_LAG_S) +
            speed * speed / (2.0 * EMERG_DECEL_MPS2))
# ----------------------------------------------------------------------
#  3) The modes and the gap controller (the same rules as AccLogic)
# ----------------------------------------------------------------------
class AccLogic:
    def __init__(self):
        self.mode = MODE_OFF
        self.gap_integral = 0.0
        self.last_command = 0.0
        self.far_since = None
        self.mode_since = 0.0
        self.resume_since = None
    def step(self, t, dt, engaged, set_speed, gap_set, my_speed, tracking, gap, lead,
             prediction=None, sensor_ok=True, battery_low=False):
        min_gap = dynamic_min_gap(my_speed)
        mode = self.mode
        if not engaged:
            mode = MODE_OFF
        elif battery_low:
            mode = MODE_LOWBAT
        elif not sensor_ok:
            mode = MODE_FAULT
        elif tracking and gap < min_gap:
            mode = MODE_EMERG
        else:
            if mode in (MODE_OFF, MODE_FAULT, MODE_LOWBAT):
                mode = MODE_SPEED
                self.gap_integral = 0.0
            if mode == MODE_SPEED:
                if tracking and gap < GAP_MODE_ENTER_M:
                    mode = MODE_GAP
                    self._bumpless(gap, gap_set, lead if prediction is None else prediction)
            elif mode == MODE_GAP:
                if (not tracking) or gap > GAP_MODE_EXIT_M:
                    if self.far_since is None:
                        self.far_since = t
                    if t - self.far_since >= GAP_EXIT_HOLD_S:
                        mode = MODE_SPEED
                else:
                    self.far_since = None
                    standing = my_speed < STOP_SPEED_MPS and self.last_command < STOP_SPEED_MPS
                    lead_standing = lead < STOP_ENTER_LEAD_MPS
                    settled = t - self.mode_since >= MODE_DWELL_S
                    if standing and lead_standing and settled and gap < gap_set + RESUME_GAP_M:
                        mode = MODE_STOP
            elif mode == MODE_STOP:
                if lead > RESUME_LEAD_MPS:
                    if self.resume_since is None:
                        self.resume_since = t
                else:
                    self.resume_since = None
                really_moving = self.resume_since is not None and t - self.resume_since >= RESUME_HOLD_S
                if t - self.mode_since < MODE_DWELL_S:
                    pass                                   # wait before driving off again
                elif (not tracking) or gap > gap_set + RESUME_GAP_M or really_moving:
                    mode = MODE_GAP if (tracking and gap < GAP_MODE_ENTER_M) else MODE_SPEED
                    self.gap_integral = 0.0
                    self.resume_since = None
            elif mode == MODE_EMERG:
                if my_speed < STOP_SPEED_MPS and ((not tracking) or gap > D_MIN_M + EMERG_CLEAR_M):
                    mode = MODE_STOP
                    self.gap_integral = 0.0
        changed = mode != self.mode
        self.mode = mode
        if changed:
            self.mode_since = t
            if mode != MODE_GAP:
                self.far_since = None
        used_prediction = False
        wanted = 0.0
        if mode == MODE_SPEED:
            wanted = set_speed
        elif mode == MODE_GAP:
            lead_ff = lead
            if prediction is not None:
                lead_ff = prediction
                used_prediction = True
            error = gap - gap_set
            raw = lead_ff + GAP_KP * error + GAP_KI * self.gap_integral
            limited = max(0.0, min(set_speed, raw))
            clamped_high = raw > set_speed and error > 0
            clamped_low = raw < 0 and error < 0
            if not clamped_high and not clamped_low:
                limit = GAP_I_LIMIT / GAP_KI
                self.gap_integral = max(-limit, min(limit, self.gap_integral + error * dt))
            wanted = limited
        if mode in (MODE_SPEED, MODE_GAP):
            up = ACCEL_LIMIT_MPS2 * dt
            down = DECEL_LIMIT_MPS2 * dt
            self.last_command = max(self.last_command - down, min(self.last_command + up, wanted))
            return max(0.0, self.last_command), False, changed, min_gap, used_prediction
        self.last_command = 0.0
        return 0.0, True, changed, min_gap, used_prediction
    def _bumpless(self, gap, gap_set, lead_ff):
        without = lead_ff + GAP_KP * (gap - gap_set)
        limit = GAP_I_LIMIT / GAP_KI
        self.gap_integral = max(-limit, min(limit, (self.last_command - without) / GAP_KI))
# ----------------------------------------------------------------------
#  4) The pretend car and one complete run
# ----------------------------------------------------------------------
class SimCar:
    def __init__(self, rng):
        self.rng = rng
        self.speed = 0.0          # the TRUE speed
        self.distance = 0.0       # the TRUE distance driven
        self.measured = 0.0       # what the ESP32 believes (noisy and filtered)
    def step(self, dt, pwm, braking):
        if braking:
            self.speed = max(0.0, self.speed - BRAKE_DECEL_MPS2 * dt)
        else:
            steady = max(0.0, (pwm - REAL_FF_START) / REAL_FF_PER_MPS) if pwm > 0 else 0.0
            self.speed += (steady - self.speed) * (dt / SPEED_TAU_S)
            self.speed = max(0.0, self.speed)
        self.distance += self.speed * dt
    def measure(self):
        noisy = max(0.0, self.speed + self.rng.gauss(0.0, SPEED_NOISE_MPS))
        self.measured += SPEED_FILTER_ALPHA * (noisy - self.measured)
        return self.measured
def run_scenario(name, set_speed=None, gap_set=None, seed=0, use_ai=False, predictor=None,
                 pieces=None, start_gap=None, sensor_breaks_at=None, duration=None):
    """Drives one whole scenario. Returns (rows, result) where rows look exactly
    like the CSV that main.py writes on the real car."""
    rng = random.Random(seed)
    set_speed = config.SET_SPEED_MPS if set_speed is None else set_speed
    gap_set = config.DEFAULT_GAP_M if gap_set is None else gap_set
    pieces = pieces or lead_profiles.PROFILES[name]
    start_gap = lead_profiles.START_GAP_M.get(name, 1.0) if start_gap is None else start_gap
    duration = duration or (lead_profiles.profile_duration(pieces) + 2.0)
    lead = lead_profiles.LeadVehicle(pieces, start_gap, noise=0.01, seed=seed)
    ranger = SimRanger(rng)
    ranger.broken_from = sensor_breaks_at
    gap_filter = GapFilter()
    logic = AccLogic()
    car = SimCar(rng)
    pwm_command = 0.0
    integral = 0.0
    target_speed = 0.0
    hold_brake = True
    braking = False
    prediction = None
    prediction_time = -10.0
    last_distance = 0.0
    rows = []
    events = []
    collision = False
    min_true_gap = 99.0
    dt = 0.005
    steps = int(duration / dt)
    next_speed_loop = 0.0
    next_gap_loop = 0.0
    next_log = 0.0
    for step in range(steps):
        t = step * dt
        lead.step(t, dt)
        true_gap = lead.position - car.distance
        min_true_gap = min(min_true_gap, true_gap)
        if true_gap <= 0.0:
            collision = True
            break
        if ranger.update(t, true_gap):
            gap_filter.update(ranger.filtered, ranger.lag_s(), car.measured)
        if t >= next_speed_loop:                       # ---- inner loop, 100 Hz ----
            next_speed_loop += SPEED_LOOP_S
            speed = car.measure()
            gap_filter.predict(car.distance - last_distance, SPEED_LOOP_S)
            last_distance = car.distance
            if hold_brake or target_speed <= 0.0:
                integral = 0.0
                pwm_command = 0.0
                braking = True
            else:
                feed_forward = SPEED_FF_PWM_START + SPEED_FF_PWM_PER_MPS * target_speed
                error = target_speed - speed
                new_integral = integral
                if abs(error) <= SPEED_INTEGRAL_BAND:      # integral band, as in pid_controller.h
                    new_integral += error * SPEED_LOOP_S
                correction = SPEED_KP * error + SPEED_KI * new_integral
                low, high = -feed_forward, PWM_MAX - feed_forward
                if correction > high:                      # anti-windup, exactly as in the firmware
                    correction = high
                    if error < 0:
                        integral = new_integral             # only let the I part shrink
                elif correction < low:
                    correction = low
                    if error > 0:
                        integral = new_integral
                else:
                    integral = new_integral
                wanted = max(0.0, min(float(PWM_MAX), feed_forward + correction))
                braking = speed > target_speed + BRAKE_ASSIST_MPS
                if braking:
                    wanted = 0.0
                change = max(-PWM_MAX_STEP, min(PWM_MAX_STEP, wanted - pwm_command))
                pwm_command += change
        if t >= next_gap_loop:                         # ---- outer loop, 50 Hz ----
            next_gap_loop += GAP_LOOP_S
            fresh = prediction if (use_ai and t - prediction_time <= PREDICTION_MAX_AGE_S) else None
            target_speed, hold_brake, changed, min_gap, used = logic.step(
                t, GAP_LOOP_S, True, set_speed, gap_set, car.measured, gap_filter.tracking,
                gap_filter.gap_m(), gap_filter.lead_mps(), fresh, ranger.ok, False)
            if changed:
                events.append((t, MODE_NAMES[logic.mode], gap_filter.gap_m(), car.speed))
        car.step(dt, pwm_command, hold_brake or braking)
        if t >= next_log:                              # ---- telemetry, 20 Hz ----
            next_log += TELEMETRY_S
            if use_ai and predictor is not None:
                predictor.add(t, gap_filter.gap_m(), gap_filter.lead_mps(), car.measured)
                if gap_filter.tracking:
                    value = predictor.predict()
                    if value is not None:
                        prediction, prediction_time = value, t
                else:
                    predictor.samples.clear()
            rows.append({
                "t": round(t, 3), "esp_ms": int(t * 1000), "mode": MODE_NAMES[logic.mode],
                "speed_mps": round(car.measured, 4), "target_mps": round(target_speed, 4),
                "gap_m": round(gap_filter.gap_m(), 4), "raw_gap_m": round(ranger.raw, 4),
                "lead_mps": round(gap_filter.lead_mps(), 4), "set_mps": set_speed,
                "gap_set_m": gap_set, "pwm": int(pwm_command),
                "battery_v": 11.6, "tracking": int(gap_filter.tracking),
                "sensor_ok": int(ranger.ok), "braking": int(hold_brake or braking),
                "used_prediction": int(bool(use_ai and prediction is not None)), "remote": 0,
                "prediction_mps": round(prediction, 4) if prediction is not None else "",
                "true_gap_m": round(true_gap, 4), "true_speed_mps": round(car.speed, 4),
                "true_lead_mps": round(lead.speed, 4),
            })
    result = {
        "scenario": name, "collision": collision, "min_true_gap_m": min_true_gap,
        "events": events, "seed": seed,
    }
    return rows, result
# ----------------------------------------------------------------------
#  5) Saving, printing and the command line
# ----------------------------------------------------------------------
LOG_FIELDS = ["t", "esp_ms", "mode", "speed_mps", "target_mps", "gap_m", "raw_gap_m", "lead_mps",
              "set_mps", "gap_set_m", "pwm", "battery_v", "tracking", "sensor_ok", "braking",
              "used_prediction", "remote", "prediction_mps", "true_gap_m", "true_speed_mps",
              "true_lead_mps"]
def save_log(rows, name, folder=None):
    folder = folder or os.path.join(config.LOG_DIR, "sim")
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, f"sim_{name}.csv")
    with open(path, "w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=LOG_FIELDS)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)
    return path
def describe(rows, result):
    import acc_metrics
    summary = acc_metrics.summarise(rows)
    summary["collision"] = result["collision"]
    summary["min_true_gap_m"] = result["min_true_gap_m"]
    return summary
def print_run(rows, result, show_events=True):
    summary = describe(rows, result)
    print(f"--- {result['scenario']} (seed {result['seed']}) ---")
    if result["collision"]:
        print("  *** COLLISION: the car touched the lead vehicle ***")
    print(f"  smallest TRUE gap {summary['min_true_gap_m'] * 100:6.1f} cm   "
          f"(the filter thought {summary['min_gap_m'] * 100:.1f} cm)")
    print(f"  gap error mean {summary['gap_error_mean_m'] * 100:5.1f} cm, "
          f"max {summary['gap_error_max_m'] * 100:5.1f} cm, "
          f"steady {summary['gap_error_settled_m'] * 100:5.1f} cm")
    print(f"  settling: {summary['settle_count']} changes, worst {summary['settle_max_s']:.2f} s   "
          f"largest acceleration {summary['accel_max_mps2']:.2f} m/s^2")
    print(f"  time following {summary['time_following_s']:.1f} s, free {summary['time_speed_s']:.1f} s, "
          f"emergencies {summary['emergencies']}")
    if show_events:
        for t, mode, gap, speed in result["events"]:
            print(f"    {t:5.2f}s -> {mode:<7} gap {gap:4.2f} m, speed {speed:4.2f} m/s")
    return summary
def run_all(use_ai=False, seed=0, save=False):
    import acc_metrics
    predictor = make_predictor(use_ai)
    print(f"{'scenario':<12} {'min gap':>8} {'steady err':>11} {'max err':>8} {'settle':>7} "
          f"{'accel':>7} {'emerg':>6} {'hit':>4}")
    worst = []
    for name in lead_profiles.PROFILES:
        rows, result = run_scenario(name, seed=seed, use_ai=use_ai, predictor=predictor)
        summary = describe(rows, result)
        if save:
            save_log(rows, name + ("_ai" if use_ai else ""))
        print(f"{name:<12} {summary['min_true_gap_m'] * 100:7.1f}cm "
              f"{summary['gap_error_settled_m'] * 100:10.1f}cm {summary['gap_error_max_m'] * 100:7.1f}cm "
              f"{summary['settle_max_s']:6.2f}s {summary['accel_max_mps2']:6.2f} "
              f"{summary['emergencies']:6d} {'YES' if result['collision'] else 'no':>4}")
        worst.append(summary)
    print()
    print("acceptance over all scenarios:")
    def worst_of(key, how=max):
        values = [s[key] for s in worst if s[key] == s[key]]
        return how(values) if values else float("nan")
    combined = {
        "min_gap_m": min(s["min_true_gap_m"] for s in worst),
        "speed_error_pct": worst_of("speed_error_pct"),
        "stop_gap_m": worst_of("stop_gap_m", min),
        "switch_spike_pct": worst_of("switch_spike_pct"),
        "gap_error_settled_m": worst_of("gap_error_settled_m"),
        "overshoot_m": max(s["overshoot_m"] for s in worst),
        "settle_max_s": worst_of("settle_max_s"),
        "accel_max_mps2": max(s["accel_max_mps2"] for s in worst),
        "decel_max_mps2": worst_of("decel_max_mps2"),
    }
    for check, value, limit, ok in acc_metrics.check_acceptance(combined):
        mark = "PASS" if ok else ("----" if ok is None else "FAIL")
        shown = "n/a" if value is None or value != value else f"{value:.3f}"
        print(f"  [{mark}] {check}: {shown} (limit {limit})")
def make_predictor(use_ai):
    if not use_ai:
        return None
    import lead_predictor
    predictor = lead_predictor.LeadPredictor()
    print(f"AI prediction is ON, model: {predictor.model_name}")
    return predictor
def run_trials(count, use_ai=False, seed=0):
    """Many runs with different random numbers: this is the safety evidence."""
    predictor = make_predictor(use_ai)
    names = [n for n in lead_profiles.PROFILES if n != "none"]
    collisions = 0
    min_gaps = []
    errors = []
    for index in range(count):
        name = names[index % len(names)]
        rows, result = run_scenario(name, seed=seed + index, use_ai=use_ai, predictor=predictor,
                                    set_speed=random.Random(seed + index).choice([0.35, 0.45, 0.55, 0.65]),
                                    gap_set=random.Random(seed + index + 7).choice(config.GAP_SETTINGS_M))
        summary = describe(rows, result)
        collisions += 1 if result["collision"] else 0
        min_gaps.append(result["min_true_gap_m"])
        if summary["gap_error_mean_m"] == summary["gap_error_mean_m"]:
            errors.append(summary["gap_error_mean_m"])
    print(f"{count} runs: {collisions} collisions, smallest gap {min(min_gaps) * 100:.1f} cm, "
          f"mean gap error {sum(errors) / len(errors) * 100:.1f} cm")
    below = sum(1 for g in min_gaps if g < D_MIN_M)
    print(f"runs that came closer than Dmin ({D_MIN_M * 100:.0f} cm): {below}")
    return collisions, min(min_gaps)
def collect_training_data(count, folder=None, seed=100):
    """Drives many random profiles and saves the logs. tools/train_predictor.py
    turns these files into the AI model."""
    folder = folder or os.path.join(config.LOG_DIR, "train")
    os.makedirs(folder, exist_ok=True)
    saved = []
    for index in range(count):
        pieces = lead_profiles.random_profile(seed + index)
        rows, result = run_scenario("random", seed=seed + index, pieces=pieces,
                                    start_gap=random.Random(seed + index).uniform(0.5, 1.4),
                                    set_speed=random.Random(seed + index + 3).choice([0.4, 0.5, 0.6, 0.7]),
                                    gap_set=random.Random(seed + index + 5).choice(config.GAP_SETTINGS_M))
        path = save_log(rows, f"train_{index:03d}", folder)
        saved.append(path)
        if result["collision"]:
            print(f"  run {index}: collision (kept: the model must learn from it too)")
    print(f"saved {len(saved)} runs into {folder}")
    print("now train the model:  python3 tools/train_predictor.py")
    return saved
def main():
    parser = argparse.ArgumentParser(description="Simulate the ACC car with no hardware.")
    parser.add_argument("--scenario", default="stop_and_go", choices=sorted(lead_profiles.PROFILES))
    parser.add_argument("--speed", type=float, default=None, help="set speed m/s")
    parser.add_argument("--gap", type=float, default=None, help="gap setting m")
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--ai", action="store_true", help="switch the AI prediction on")
    parser.add_argument("--all", action="store_true", help="run every scenario")
    parser.add_argument("--trials", type=int, default=0, help="run many random trials")
    parser.add_argument("--collect", type=int, default=0, help="save this many runs as training data")
    parser.add_argument("--log", action="store_true", help="save the run as a CSV file")
    parser.add_argument("--sensor-breaks", type=float, default=None,
                        help="pretend the ultrasonic sensor dies at this time (seconds)")
    args = parser.parse_args()
    if args.collect:
        collect_training_data(args.collect)
        return 0
    if args.trials:
        run_trials(args.trials, use_ai=args.ai, seed=args.seed)
        return 0
    if args.all:
        run_all(use_ai=args.ai, seed=args.seed, save=args.log)
        return 0
    predictor = make_predictor(args.ai)
    rows, result = run_scenario(args.scenario, set_speed=args.speed, gap_set=args.gap, seed=args.seed,
                                use_ai=args.ai, predictor=predictor, sensor_breaks_at=args.sensor_breaks)
    print_run(rows, result)
    if args.log:
        path = save_log(rows, args.scenario + ("_ai" if args.ai else ""))
        print(f"saved {path}")
        print(f"plot it:  python3 tools/plot_log.py {path}")
    return 0
if __name__ == "__main__":
    sys.exit(main())
