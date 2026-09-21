"""
tools/acc_trials.py - runs the OFFICIAL test series and writes the results table.
Phase 8 of the guide asks for the same tests repeated three times each. Doing
that by hand and writing the numbers on paper is slow and easy to get wrong.
This tool does it for you:
    for every trial:
        1. it tells you where to put the two cars
        2. you press Enter, then start the lead vehicle (profile on its Serial
           Monitor, or the BOOT button on the helper sketch)
        3. it records the whole run and stops the car at the end
        4. it works out min gap, gap error, settling time and comfort
    at the end it prints one table and saves it as CSV for your report.
    python3 tools/acc_trials.py                         # the standard series
    python3 tools/acc_trials.py --repeats 3             # three of each
    python3 tools/acc_trials.py --speeds 0.4 0.6 --gaps 0.4 0.5 0.6
    python3 tools/acc_trials.py --ai                    # with the AI prediction on
"""
import argparse
import csv
import datetime
import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import acc_metrics   # noqa: E402
import config        # noqa: E402
import esp32_link    # noqa: E402
import lead_predictor  # noqa: E402
import main as main_module  # noqa: E402
def record_run(link, predictor, set_speed, gap_set, seconds, use_ai):
    """Drives one trial and returns the rows (the same shape as a log file)."""
    rows = []
    started = time.monotonic()
    last_ms = -1
    prediction = None
    last_sent = 0.0
    link.set_dashboard(set_speed, gap_set, engage=True)
    while time.monotonic() - started < seconds:
        now = time.monotonic()
        status = link.status
        if status is not None and status.esp_ms != last_ms:
            last_ms = status.esp_ms
            if use_ai:
                prediction = predictor.update(status)
            rows.append({
                "t": round(now - started, 3), "esp_ms": status.esp_ms, "mode": status.mode,
                "speed_mps": status.speed_mps, "target_mps": status.target_mps,
                "gap_m": status.gap_m, "raw_gap_m": status.raw_gap_m, "lead_mps": status.lead_mps,
                "set_mps": status.set_mps, "gap_set_m": status.gap_set_m, "pwm": status.pwm,
                "battery_v": status.battery_v, "tracking": int(status.tracking),
                "sensor_ok": int(status.sensor_ok), "braking": int(status.braking),
                "used_prediction": int(status.used_prediction), "remote": int(status.remote),
                "prediction_mps": round(prediction, 3) if prediction is not None else "",
            })
        if use_ai and prediction is not None and now - last_sent >= 1.0 / config.PREDICTION_RATE_HZ:
            last_sent = now
            link.send_prediction(prediction)
        if int((now - started) * 2) % 8 == 0:
            link.set_dashboard(set_speed, gap_set, engage=True)   # keep the remote dashboard alive
        time.sleep(0.01)
    return rows
def main():
    parser = argparse.ArgumentParser(description="Run the official ACC test series.")
    parser.add_argument("--port", default=config.SERIAL_PORT)
    parser.add_argument("--speeds", type=float, nargs="+", default=[0.4, 0.6])
    parser.add_argument("--gaps", type=float, nargs="+", default=config.GAP_SETTINGS_M)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--seconds", type=float, default=15.0, help="length of one trial")
    parser.add_argument("--ai", action="store_true", help="send AI predictions during the trials")
    args = parser.parse_args()
    link = esp32_link.Esp32Link(port=args.port)
    if not link.wait_until_ready(5.0):
        print("No telemetry from the ESP32.")
        return 1
    predictor = lead_predictor.LeadPredictor() if args.ai else None
    if args.ai:
        print(f"AI prediction ON, model: {predictor.model_name}")
    os.makedirs(config.LOG_DIR, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    table_path = os.path.join(config.LOG_DIR, f"trials_{stamp}.csv")
    fields = ["trial", "set_speed_mps", "gap_set_m", "repeat", "min_gap_m", "gap_error_settled_m",
              "gap_error_max_m", "overshoot_m", "settle_max_s", "accel_max_mps2", "emergencies", "log"]
    results = []
    trial = 0
    try:
        for set_speed in args.speeds:
            for gap_set in args.gaps:
                for repeat in range(1, args.repeats + 1):
                    trial += 1
                    print()
                    print(f"TRIAL {trial}: set speed {set_speed:.2f} m/s, gap {gap_set * 100:.0f} cm, "
                          f"run {repeat} of {args.repeats}")
                    print("  Put the ACC car at the start line and the lead vehicle about 1 m ahead.")
                    input("  Press Enter, then start the lead vehicle profile...")
                    rows = record_run(link, predictor, set_speed, gap_set, args.seconds, args.ai)
                    link.set_dashboard(set_speed, gap_set, engage=False)
                    link.wait_until_stopped(3.0)
                    path, handle, writer = main_module.new_log_file()
                    for row in rows:
                        writer.writerow({k: row.get(k, "") for k in main_module.LOG_FIELDS})
                    handle.close()
                    summary = acc_metrics.summarise(rows)
                    results.append({
                        "trial": trial, "set_speed_mps": set_speed, "gap_set_m": gap_set,
                        "repeat": repeat, "min_gap_m": round(summary["min_gap_m"], 3),
                        "gap_error_settled_m": round(summary["gap_error_settled_m"], 3),
                        "gap_error_max_m": round(summary["gap_error_max_m"], 3),
                        "overshoot_m": round(summary["overshoot_m"], 3),
                        "settle_max_s": round(summary["settle_max_s"], 2),
                        "accel_max_mps2": round(summary["accel_max_mps2"], 2),
                        "emergencies": summary["emergencies"], "log": os.path.basename(path),
                    })
                    print(f"  min gap {summary['min_gap_m'] * 100:.1f} cm, steady error "
                          f"{summary['gap_error_settled_m'] * 100:.1f} cm, "
                          f"settling {summary['settle_max_s']:.2f} s")
    except KeyboardInterrupt:
        print("\nstopped by the user")
    finally:
        link.stop()
        link.close()
    with open(table_path, "w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for row in results:
            writer.writerow(row)
    print()
    print(f"{len(results)} trials saved to {table_path}")
    print("Copy this table straight into your report (Phase 8).")
    return 0
if __name__ == "__main__":
    sys.exit(main())
