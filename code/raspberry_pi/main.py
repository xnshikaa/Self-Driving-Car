"""
main.py - the program you run on the Raspberry Pi while the ACC car drives.
WHAT IT DOES
  1. listens to the ESP32 20 times per second (speed, gap, mode, battery)
  2. writes every message into a CSV log file (that is your evidence)
  3. runs the small AI (lead_predictor.py) and sends its guess back to the
     ESP32, which uses it to follow the lead vehicle more smoothly
  4. prints a live dashboard, and a full report when you press Ctrl-C
IMPORTANT: the Pi does NOT drive the car. The ESP32 decides everything by
itself. If you close this program, or the Pi crashes, or you pull the cable
out, the car keeps working - it just stops getting predictions and stops
being logged. That is on purpose: safety must never depend on the slow part.
    python3 main.py                       # watch and log, driver uses the knob
    python3 main.py --speed 0.5 --gap 0.5 # the Pi sets the dashboard as well
    python3 main.py --no-ai               # log without sending predictions
    python3 main.py --seconds 60          # stop by itself after a minute
"""
import argparse
import csv
import datetime
import os
import sys
import time
import acc_metrics
import config
import esp32_link
import lead_predictor
LOG_FIELDS = ["t", "esp_ms", "mode", "speed_mps", "target_mps", "gap_m", "raw_gap_m", "lead_mps",
              "set_mps", "gap_set_m", "pwm", "battery_v", "tracking", "sensor_ok", "braking",
              "used_prediction", "remote", "prediction_mps"]
def new_log_file():
    os.makedirs(config.LOG_DIR, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    path = os.path.join(config.LOG_DIR, f"run_{stamp}.csv")
    handle = open(path, "w", newline="")
    writer = csv.DictWriter(handle, fieldnames=LOG_FIELDS)
    writer.writeheader()
    return path, handle, writer
def dashboard_line(status, prediction):
    bar_len = 20
    gap = min(1.5, status.gap_m)
    filled = int(bar_len * gap / 1.5) if status.tracking else 0
    bar = "#" * filled + "." * (bar_len - filled)
    predicted = f"{prediction:.2f}" if prediction is not None else " -- "
    return (f"{status.mode:<7} speed {status.speed_mps:4.2f}->{status.target_mps:4.2f}  "
            f"gap [{bar}] {status.gap_m:4.2f}/{status.gap_set_m:4.2f}  "
            f"lead {status.lead_mps:4.2f} (AI {predicted})  "
            f"pwm {status.pwm:4d}  bat {status.battery_v:4.1f}V"
            f"{'  [EMERGENCY]' if status.mode == 'EMERG' else ''}")
def main():
    parser = argparse.ArgumentParser(description="Watch, log and help the ACC car.")
    parser.add_argument("--port", default=config.SERIAL_PORT, help="serial port of the ESP32")
    parser.add_argument("--speed", type=float, default=None, help="set speed in m/s (otherwise the knob 
decides)")
    parser.add_argument("--gap", type=float, default=None, help="gap setting in m (otherwise the button 
decides)")
    parser.add_argument("--no-ai", action="store_true", help="do not send predictions")
    parser.add_argument("--seconds", type=float, default=0.0, help="stop by itself after this many seconds")
    parser.add_argument("--quiet", action="store_true", help="no live dashboard, only the report")
    args = parser.parse_args()
    link = esp32_link.Esp32Link(port=args.port)
    if not link.wait_until_ready(5.0):
        print("No messages from the ESP32. Check the wiring (TX<->RX crossed), the baud rate")
        print("and that p04_acc.ino is running. tools/uart_echo_test.py tests the wires alone.")
        link.close()
        return 1
    predictor = lead_predictor.LeadPredictor()
    print(f"Connected. AI model: {predictor.model_name}")
    path, handle, writer = new_log_file()
    print(f"Logging to {path}   (press Ctrl-C to stop)")
    if args.speed is not None or args.gap is not None:
        link.set_dashboard(args.speed if args.speed is not None else config.SET_SPEED_MPS,
                           args.gap if args.gap is not None else config.DEFAULT_GAP_M, engage=True)
        print("The Pi is setting the dashboard. The knob and buttons are ignored while it does.")
    started = time.monotonic()
    last_prediction = 0.0
    last_dashboard = 0.0
    last_esp_ms = -1
    prediction = None
    rows = []
    try:
        while True:
            now = time.monotonic()
            status = link.status
            if status is None or link.status_age() > 1.0:
                print("No telemetry for a second - is the ESP32 still running?")
                time.sleep(0.5)
                continue
            # ---- one new telemetry message: log it ----
            if status.esp_ms != last_esp_ms:
                last_esp_ms = status.esp_ms
                if not args.no_ai:
                    prediction = predictor.update(status)
                row = {
                    "t": round(now - started, 3), "esp_ms": status.esp_ms, "mode": status.mode,
                    "speed_mps": status.speed_mps, "target_mps": status.target_mps,
                    "gap_m": status.gap_m, "raw_gap_m": status.raw_gap_m, "lead_mps": status.lead_mps,
                    "set_mps": status.set_mps, "gap_set_m": status.gap_set_m, "pwm": status.pwm,
                    "battery_v": status.battery_v, "tracking": int(status.tracking),
                    "sensor_ok": int(status.sensor_ok), "braking": int(status.braking),
                    "used_prediction": int(status.used_prediction), "remote": int(status.remote),
                    "prediction_mps": round(prediction, 3) if prediction is not None else "",
                }
                writer.writerow(row)
                rows.append({k: (float(v) if k in acc_metrics.NUMERIC and v != "" else v)
                             for k, v in row.items()})
            # ---- send the prediction (20 times per second) ----
            if not args.no_ai and prediction is not None and now - last_prediction >= 1.0 / 
config.PREDICTION_RATE_HZ:
                last_prediction = now
                link.send_prediction(prediction)
            # ---- keep the dashboard alive if the Pi is in charge of it ----
            if args.speed is not None or args.gap is not None:
                if now - started > 0.2 and int(now * 4) % 2 == 0:
                    link.set_dashboard(args.speed if args.speed is not None else config.SET_SPEED_MPS,
                                       args.gap if args.gap is not None else config.DEFAULT_GAP_M, True)
            for event in link.pop_events():
                print("  EVENT", event.describe())
            if not args.quiet and now - last_dashboard >= 1.0 / config.DASHBOARD_RATE_HZ:
                last_dashboard = now
                print(dashboard_line(status, prediction))
            if args.seconds and now - started >= args.seconds:
                break
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        try:
            link.stop()
        finally:
            link.close()
            handle.close()
    print()
    if rows:
        acc_metrics.print_report(path, rows)
    print(f"Log saved: {path}")
    print("Next:  python3 tools/plot_log.py " + path)
    return 0
if __name__ == "__main__":
    sys.exit(main())
