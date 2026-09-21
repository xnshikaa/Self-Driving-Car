"""
tools/speed_test.py - measures how well the INNER loop holds a speed.
This is the first thing to run after the car is built, because the gap
controller can only be as good as the speed controller underneath it.
What it does: asks the ESP32 (remote drive, $M) for a series of speeds and
records what the car actually did. For every step it prints
    * the average speed once it settled        -> should equal the order
    * the time to reach 90 % of the order      -> rise time
    * the overshoot                            -> should be small
    * the wobble (standard deviation)          -> how steady it is
    python3 tools/speed_test.py                 # 0.2 ... 0.6 m/s
    python3 tools/speed_test.py --speeds 0.3 0.5 0.7
    python3 tools/speed_test.py --seconds 4     # longer steps
Run it with 3 m of clear floor, or with the wheels off the ground for a
first check (the numbers will be higher than on the floor).
"""
import argparse
import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import config        # noqa: E402
import esp32_link    # noqa: E402
def step(link, speed, seconds):
    samples = []
    started = time.monotonic()
    last_ms = -1
    while time.monotonic() - started < seconds:
        link.drive(speed)
        status = link.status
        if status is not None and status.esp_ms != last_ms:
            last_ms = status.esp_ms
            samples.append((time.monotonic() - started, status.speed_mps, status.pwm))
        time.sleep(0.02)
    return samples
def analyse(speed, samples, settle_after=1.0):
    if not samples:
        return None
    settled = [s for s in samples if s[0] >= settle_after]
    if not settled:
        settled = samples[-5:]
    mean = sum(s[1] for s in settled) / len(settled)
    variance = sum((s[1] - mean) ** 2 for s in settled) / len(settled)
    peak = max(s[1] for s in samples)
    rise = None
    for t, value, _ in samples:
        if value >= 0.9 * speed:
            rise = t
            break
    pwm = sum(s[2] for s in settled) / len(settled)
    return {"order": speed, "mean": mean, "error": mean - speed, "sd": variance ** 0.5,
            "overshoot": max(0.0, peak - speed), "rise_s": rise, "pwm": pwm}
def main():
    parser = argparse.ArgumentParser(description="Step test of the speed controller.")
    parser.add_argument("--port", default=config.SERIAL_PORT)
    parser.add_argument("--speeds", type=float, nargs="+", default=[0.2, 0.3, 0.4, 0.5, 0.6])
    parser.add_argument("--seconds", type=float, default=3.0, help="how long each speed is held")
    args = parser.parse_args()
    link = esp32_link.Esp32Link(port=args.port)
    if not link.wait_until_ready(5.0):
        print("No telemetry from the ESP32 - check the wiring and that p04_acc.ino is running.")
        return 1
    print("Keep clear of the car. Starting in 3 seconds...")
    time.sleep(3)
    results = []
    try:
        for speed in args.speeds:
            print(f"  ordering {speed:.2f} m/s ...")
            samples = step(link, speed, args.seconds)
            result = analyse(speed, samples)
            if result:
                results.append(result)
            link.drive(0.0)
            link.release()
            link.wait_until_stopped(3.0)
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("stopped by the user")
    finally:
        link.release()
        link.stop()
        link.close()
    print()
    print("order  measured   error     sd   overshoot  rise   average PWM")
    for r in results:
        rise = f"{r['rise_s']:.2f}s" if r["rise_s"] is not None else "  -  "
        print(f"{r['order']:5.2f}  {r['mean']:8.3f} {r['error']:+6.3f} {r['sd']:6.3f} "
              f"{r['overshoot']:9.3f} {rise:>6}  {r['pwm']:6.0f}")
    print()
    print("What good looks like: |error| < 0.03 m/s, sd < 0.03 m/s, rise < 1.0 s,")
    print("overshoot < 0.05 m/s. If the error grows with speed, your feed-forward")
    print("numbers (SPEED_FF_PWM_START / PER_MPS) are wrong: run test t3 again.")
    if len(results) >= 2:
        first, last = results[0], results[-1]
        slope = (last["pwm"] - first["pwm"]) / max(0.01, last["order"] - first["order"])
        start = first["pwm"] - slope * first["order"]
        print(f"Measured now:  SPEED_FF_PWM_START = {start:.0f},  SPEED_FF_PWM_PER_MPS = {slope:.0f}")
    return 0
if __name__ == "__main__":
    sys.exit(main())
