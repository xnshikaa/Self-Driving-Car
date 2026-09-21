"""
acc_metrics.py - reads a run log and works out how good the ACC was.
A run log is one CSV file written by main.py (or by sim/acc_sim.py), one line
every 50 ms. This file turns those thousands of lines into the handful of
numbers your report needs:
    minimum gap        - did we ever get closer than Dmin (25 cm)?  [safety]
    gap error          - how far from the set gap we sat while following
    settling time      - after the lead vehicle changed speed, how long until
                         the gap was back inside +/- 10 cm and stayed there
    overshoot          - the largest swing past the set gap
    comfort            - the largest acceleration we asked the car for
    time in each mode  - SPEED / GAP / STOP / EMERG ...
Use it from the command line:
    python3 acc_metrics.py logs/run_2026-05-04_10-31-22.csv
"""
import csv
import os
import sys
import config
NUMERIC = ["t", "esp_ms", "speed_mps", "target_mps", "gap_m", "raw_gap_m", "lead_mps",
           "set_mps", "gap_set_m", "pwm", "battery_v", "prediction_mps"]
BOOLEAN = ["tracking", "sensor_ok", "braking", "used_prediction", "remote"]
def read_log(path):
    """Reads one CSV log into a list of dictionaries (numbers already converted)."""
    rows = []
    with open(path, newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None or "t" not in reader.fieldnames:
            return []            # not a run log (for example a trials_*.csv table)
        for raw in reader:
            row = dict(raw)
            for key in NUMERIC:
                if key in row and row[key] not in (None, ""):
                    try:
                        row[key] = float(row[key])
                    except ValueError:
                        row[key] = 0.0
            for key in BOOLEAN:
                if key in row:
                    row[key] = str(row[key]).strip().lower() in ("1", "true", "yes")
            rows.append(row)
    rows.sort(key=lambda r: r["t"])
    return rows
def _following(rows):
    """Only the moments when we were really following a lead vehicle."""
    return [r for r in rows if r.get("mode") == "GAP" and r.get("tracking")]
def _settled(rows, quiet_s=2.0, quiet_change=0.08):
    """The moments when we were following AND the lead vehicle had been driving
    steadily for at least quiet_s. This is the fair way to measure how well we
    HOLD the gap: right after the lead vehicle brakes, a big error is normal
    and is measured separately as the settling time."""
    following = _following(rows)
    # Ignore everything before the gap was correct for the FIRST time: driving up
    # to the lead vehicle at the start is an approach, not gap holding.
    tolerance = config.ACCEPT_GAP_ERROR_M
    first_good = None
    for row in following:
        if abs(row["gap_m"] - row["gap_set_m"]) <= tolerance:
            first_good = row["t"]
            break
    if first_good is None:
        return []
    settled = []
    for row in following:
        if row["t"] < first_good:
            continue
        window = [r["lead_mps"] for r in rows
                  if 0 <= row["t"] - r["t"] <= quiet_s and r.get("tracking")]
        if len(window) < 10:
            continue
        if max(window) - min(window) <= quiet_change:
            settled.append(row)
    return settled
def _smooth(rows, key, window=5):
    """A moving average: takes the jitter out of a measured signal."""
    values = [r.get(key, 0.0) for r in rows]
    out = []
    for index in range(len(values)):
        piece = values[max(0, index - window + 1):index + 1]
        out.append(sum(piece) / len(piece))
    return out
def settling_times(rows, tolerance_m=None):
    """Finds every moment the lead vehicle clearly changed speed, and measures
    how long the gap took to come back inside the tolerance and STAY there.
    Two details that matter:
      * the lead speed is SMOOTHED first, otherwise sensor noise looks like a
        speed change and the answer is nonsense
      * moments before the gap was ever correct are skipped: driving up to the
        lead vehicle for the first time is an approach, not a settling time
    """
    tolerance_m = tolerance_m or config.ACCEPT_GAP_ERROR_M
    following = _following(rows)
    first_good = None
    for row in following:
        if abs(row["gap_m"] - row["gap_set_m"]) <= tolerance_m:
            first_good = row["t"]
            break
    if first_good is None:
        return []
    smooth_lead = _smooth(rows, "lead_mps")
    # First find WHEN the lead vehicle changed speed ...
    changes = []
    for index in range(10, len(rows)):
        row = rows[index]
        if not row.get("tracking") or row.get("mode") != "GAP" or row["t"] < first_good:
            continue
        change = smooth_lead[index] - smooth_lead[index - 10]
        if abs(change) < 0.10:                       # not a real change of speed
            continue
        if changes and row["t"] - changes[-1][1] < 2.0:   # still inside the previous one
            continue
        changes.append((index, row["t"], change))
    # ... then measure each one, but only until the NEXT change (you cannot
    # measure a settling time through a new disturbance).
    events = []
    for order, (index, event_t, change) in enumerate(changes):
        row = rows[index]
        next_t = changes[order + 1][1] if order + 1 < len(changes) else float("inf")
        # Settling time = from the speed change until the LAST moment the gap
        # error is outside the tolerance (looking at most 8 s ahead). That is
        # the usual engineering definition: after this moment it stays good.
        last_bad = row["t"]
        for later in rows[index:]:
            if later["t"] - row["t"] > 8.0 or later["t"] >= next_t:
                break
            if later.get("mode") == "STOP":
                break            # we came to a halt behind it: that IS settled
            if later.get("mode") != "GAP":
                continue
            if abs(later["gap_m"] - later["gap_set_m"]) > tolerance_m:
                last_bad = later["t"]
        events.append((row["t"], last_bad - row["t"], change))
    return events
def speed_hold(rows, settle_s=1.5):
    """Test A of the specification: how well we hold the set speed on a clear road.
    Only counted while the car is in SPEED mode, has been there for settle_s, and
    the controller is really asking for the set speed (not still ramping up)."""
    errors = []
    mode_since = None
    for row in rows:
        if row.get("mode") != "SPEED":
            mode_since = None
            continue
        if mode_since is None:
            mode_since = row["t"]
        target, wanted = row["target_mps"], row["set_mps"]
        if wanted < 0.05 or abs(target - wanted) > 0.01:
            continue                      # still speeding up towards the set speed
        if row["t"] - mode_since < settle_s:
            continue
        errors.append(abs(row["speed_mps"] - wanted) / wanted * 100.0)
    return errors
def mode_switch_spikes(rows):
    """Test D of the specification: switching between SPEED and GAP mode must not
    make the car jump. We measure the jump in the ORDERED speed across the switch,
    as a percentage of the set speed. (The rate limiter allows about 6 %, so a
    bumpless switch stays well under the 10 % the specification allows.)"""
    spikes = []
    for a, b in zip(rows, rows[1:]):
        if {a.get("mode"), b.get("mode")} != {"SPEED", "GAP"}:
            continue
        wanted = max(0.05, b["set_mps"])
        jump = abs(b["target_mps"] - a["target_mps"]) / wanted * 100.0
        excursion = 0.0
        at_switch = a["speed_mps"]
        for later in rows[rows.index(b):]:
            if later["t"] - b["t"] > 1.0:
                break
            excursion = max(excursion, (later["speed_mps"] - at_switch) / wanted * 100.0)
        spikes.append((b["t"], a["mode"], b["mode"], jump, excursion))
    return spikes
def summarise(rows):
    """Turns a whole run into one dictionary of numbers."""
    if not rows:
        return {}
    duration = rows[-1]["t"] - rows[0]["t"]
    following = _following(rows)
    errors = [abs(r["gap_m"] - r["gap_set_m"]) for r in following]
    steady = [abs(r["gap_m"] - r["gap_set_m"]) for r in _settled(rows)]
    tracked = [r for r in rows if r.get("tracking") and r["gap_m"] < 3.0]
    # Comfort: how hard we changed speed between two log lines. Speeding up and
    # slowing down are judged separately - braking hard is SAFE, and an emergency
    # stop is not a comfort problem at all, so those moments are left out.
    accelerations, decelerations = [], []
    for a, b in zip(rows, rows[1:]):
        dt = b["t"] - a["t"]
        if not (0.01 < dt < 0.5):
            continue
        change = (b["speed_mps"] - a["speed_mps"]) / dt
        if a.get("mode") == "EMERG" or b.get("mode") == "EMERG":
            continue
        if change >= 0:
            accelerations.append(change)
        else:
            decelerations.append(-change)
    modes = {}
    for a, b in zip(rows, rows[1:]):
        dt = min(0.5, max(0.0, b["t"] - a["t"]))
        modes[a.get("mode", "?")] = modes.get(a.get("mode", "?"), 0.0) + dt
    settles = settling_times(rows)
    # Two different things, measured only AFTER the gap was correct for the first
    # time (driving up to the lead vehicle at the start is an approach, not a swing):
    #   overshoot  = how far we came CLOSER than the set gap   -> a safety number
    #   opening    = how far the gap OPENED beyond the setting -> only comfort,
    #                and normal when the lead vehicle drives away faster than we may
    overshoot = 0.0
    opening = 0.0
    first_good = None
    for row in following:
        error = row["gap_m"] - row["gap_set_m"]
        if first_good is None:
            if abs(error) <= config.ACCEPT_GAP_ERROR_M:
                first_good = row["t"]
            continue
        overshoot = max(overshoot, -error)
        opening = max(opening, error)
    stopped = [r["gap_m"] for r in rows if r.get("mode") == "STOP" and r.get("tracking")]
    speed_errors = speed_hold(rows)
    spikes = mode_switch_spikes(rows)
    resumes = sum(1 for a, b in zip(rows, rows[1:])
                  if a.get("mode") == "STOP" and b.get("mode") in ("GAP", "SPEED"))
    summary = {
        "duration_s": duration,
        "stop_gap_m": (sum(stopped) / len(stopped)) if stopped else float("nan"),
        "samples": len(rows),
        "min_gap_m": min((r["gap_m"] for r in tracked), default=float("nan")),
        "gap_error_mean_m": (sum(errors) / len(errors)) if errors else float("nan"),
        "gap_error_max_m": max(errors) if errors else float("nan"),
        "gap_error_settled_m": (sum(steady) / len(steady)) if steady else float("nan"),
        "gap_error_settled_max_m": max(steady) if steady else float("nan"),
        "settled_samples": len(steady),
        "gap_error_rms_m": (sum(e * e for e in errors) / len(errors)) ** 0.5 if errors else float("nan"),
        "overshoot_m": overshoot,
        "opening_m": opening,
        "settle_count": len(settles),
        "settle_max_s": max((s[1] for s in settles), default=float("nan")),
        "settle_mean_s": (sum(s[1] for s in settles) / len(settles)) if settles else float("nan"),
        "accel_max_mps2": max(accelerations, default=float("nan")),
        "decel_max_mps2": max(decelerations, default=float("nan")),
        "time_following_s": modes.get("GAP", 0.0),
        "time_speed_s": modes.get("SPEED", 0.0),
        "time_emergency_s": modes.get("EMERG", 0.0),
        "emergencies": sum(1 for a, b in zip(rows, rows[1:])
                           if a.get("mode") != "EMERG" and b.get("mode") == "EMERG"),
        "speed_error_pct": (sum(speed_errors) / len(speed_errors)) if speed_errors else float("nan"),
        "speed_error_max_pct": max(speed_errors) if speed_errors else float("nan"),
        "speed_hold_samples": len(speed_errors),
        "switch_count": len(spikes),
        "switch_spike_pct": max((s[3] for s in spikes), default=float("nan")),
        "switch_excursion_pct": max((s[4] for s in spikes), default=float("nan")),
        "resumes": resumes,
        "used_prediction": sum(1 for r in rows if r.get("used_prediction")),
        "battery_min_v": min((r["battery_v"] for r in rows if r["battery_v"] > 1.0), default=float("nan")),
    }
    return summary
def check_acceptance(summary):
    """Compares the summary with the limits in config.py.
    The first five are the acceptance tests A to E of the project specification;
    the rest are our own extra checks for a car that also drives nicely."""
    checks = [
        ("A. speed hold (% of set speed)", summary.get("speed_error_pct"), "<=", 
config.ACCEPT_SPEED_ERROR_PCT),
        ("B. gap hold (m, lead steady)", summary.get("gap_error_settled_m"), "<=", 
config.ACCEPT_GAP_ERROR_M),
        ("C. stop-and-go: stop gap (m)", summary.get("stop_gap_m"), ">=", config.ACCEPT_STOP_GAP_M),
        ("D. mode switch jump (% of set)", summary.get("switch_spike_pct"), "<=", 
config.ACCEPT_SWITCH_SPIKE_PCT),
        ("E. never closer than (m)", summary.get("min_gap_m"), ">=", config.ACCEPT_CUTIN_MIN_M),
        ("our own Dmin (m)", summary.get("min_gap_m"), ">=", config.ACCEPT_MIN_GAP_M),
        ("overshoot, too close (m)", summary.get("overshoot_m"), "<=", config.ACCEPT_OVERSHOOT_M),
        ("settling time (s)", summary.get("settle_max_s"), "<=", config.ACCEPT_SETTLE_S),
        ("comfort: speeding up (m/s2)", summary.get("accel_max_mps2"), "<=", config.ACCEPT_ACCEL_MPS2),
        ("braking (m/s2)", summary.get("decel_max_mps2"), "<=", config.ACCEPT_DECEL_MPS2),
    ]
    results = []
    for name, value, how, limit in checks:
        if value is None or value != value:            # not a number (nan) = not measured
            results.append((name, value, limit, None))
            continue
        ok = (value >= limit) if how == ">=" else (value <= limit)
        results.append((name, value, limit, ok))
    return results
def print_report(path, rows=None):
    rows = rows if rows is not None else read_log(path)
    summary = summarise(rows)
    print(f"--- {os.path.basename(path)} ---")
    print(f"  {summary['samples']} samples over {summary['duration_s']:.1f} s")
    print(f"  following a lead vehicle for {summary['time_following_s']:.1f} s, "
          f"free cruising {summary['time_speed_s']:.1f} s")
    print(f"  gap error: mean {summary['gap_error_mean_m'] * 100:.1f} cm, "
          f"max {summary['gap_error_max_m'] * 100:.1f} cm, rms {summary['gap_error_rms_m'] * 100:.1f} cm")
    print(f"  gap error while the lead vehicle drove steadily: "
          f"{summary['gap_error_settled_m'] * 100:.1f} cm "
          f"(worst {summary['gap_error_settled_max_m'] * 100:.1f} cm, "
          f"{summary['settled_samples']} samples)")
    print(f"  smallest gap: {summary['min_gap_m'] * 100:.1f} cm")
    if summary["stop_gap_m"] == summary["stop_gap_m"]:
        print(f"  stopped {summary['stop_gap_m'] * 100:.1f} cm behind a standing lead vehicle")
    print(f"  settling: {summary['settle_count']} speed changes, "
          f"mean {summary['settle_mean_s']:.2f} s, worst {summary['settle_max_s']:.2f} s")
    print(f"  swings after settling: {summary['overshoot_m'] * 100:.1f} cm too close, "
          f"{summary['opening_m'] * 100:.1f} cm too far")
    if summary["speed_hold_samples"]:
        print(f"  speed hold on a clear road: {summary['speed_error_pct']:.2f} % of the set speed "
              f"(worst {summary['speed_error_max_pct']:.2f} %, {summary['speed_hold_samples']} samples)")
    if summary["switch_count"]:
        print(f"  mode switches: {summary['switch_count']}, biggest jump in the ordered speed "
              f"{summary['switch_spike_pct']:.1f} % of the set speed")
    print(f"  comfort: hardest speed-up {summary['accel_max_mps2']:.2f} m/s^2, "
          f"hardest braking {summary['decel_max_mps2']:.2f} m/s^2 (emergencies not counted)")
    print(f"  emergencies: {summary['emergencies']}  (time in EMERG {summary['time_emergency_s']:.1f} s)")
    print("  acceptance:")
    for name, value, limit, ok in check_acceptance(summary):
        mark = "PASS" if ok else ("----" if ok is None else "FAIL")
        shown = "n/a" if value is None or value != value else f"{value:.3f}"
        print(f"    [{mark}] {name}: {shown} (limit {limit})")
    return summary
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Use:  python3 acc_metrics.py logs/run_....csv")
        sys.exit(1)
    for log_path in sys.argv[1:]:
        print_report(log_path)
