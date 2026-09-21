"""
ai_benchmark.py - does the AI prediction actually make the car better? Measure it.
This is the honest test that belongs in your report. It drives EVERY scenario
several times with the prediction switched off, then exactly the same runs with
it switched on, and prints the two sets of numbers side by side.
    python3 sim/ai_benchmark.py                  # 5 seeds per scenario
    python3 sim/ai_benchmark.py --seeds 10
    python3 sim/ai_benchmark.py --slow-sensor    # pretend the sensor is 4x slower
Why the slow-sensor option? Predicting the future only helps when you are
LATE. With our 20 Hz ultrasonic sensor the car already knows what the lead
vehicle is doing, so there is almost nothing left to win. Make the sensor slow
(as slow as a camera that needs 200 ms to think) and the prediction starts to
pay for itself. Run both and write down what you find - a measured "it did not
help here, and here is why" is worth more marks than a guess that it did.
"""
import argparse
import os
import statistics
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)
import acc_metrics     # noqa: E402
import acc_sim         # noqa: E402
import lead_predictor  # noqa: E402
import lead_profiles   # noqa: E402
def measure(use_ai, seeds, names):
    predictor = lead_predictor.LeadPredictor() if use_ai else None
    steady, settle, overshoot, min_gap, collisions = [], [], [], [], 0
    for name in names:
        for seed in range(seeds):
            rows, result = acc_sim.run_scenario(name, seed=seed, use_ai=use_ai, predictor=predictor)
            summary = acc_metrics.summarise(rows)
            if summary["gap_error_settled_m"] == summary["gap_error_settled_m"]:
                steady.append(summary["gap_error_settled_m"])
            if summary["settle_max_s"] == summary["settle_max_s"]:
                settle.append(summary["settle_max_s"])
            overshoot.append(summary["overshoot_m"])
            min_gap.append(result["min_true_gap_m"])
            collisions += 1 if result["collision"] else 0
    return {
        "runs": len(min_gap),
        "steady_cm": statistics.mean(steady) * 100 if steady else float("nan"),
        "steady_worst_cm": max(steady) * 100 if steady else float("nan"),
        "settle_s": statistics.mean(settle) if settle else float("nan"),
        "settle_worst_s": max(settle) if settle else float("nan"),
        "overshoot_cm": max(overshoot) * 100,
        "min_gap_cm": min(min_gap) * 100,
        "collisions": collisions,
    }
def main():
    parser = argparse.ArgumentParser(description="Compare the ACC with and without the AI prediction.")
    parser.add_argument("--seeds", type=int, default=5, help="runs per scenario")
    parser.add_argument("--slow-sensor", action="store_true", help="pretend the gap sensor is 4x slower")
    args = parser.parse_args()
    if args.slow_sensor:
        acc_sim.US_PING_S = 0.200
        print("The gap sensor is now 5 readings per second instead of 20.")
    names = [name for name in lead_profiles.PROFILES if name != "none"]
    print(f"{len(names)} scenarios x {args.seeds} runs, twice. Please wait...")
    off = measure(False, args.seeds, names)
    on = measure(True, args.seeds, names)
    print()
    print(f"{'measurement':<34}{'AI off':>10}{'AI on':>10}{'change':>10}")
    for key, label, better_low in [
        ("steady_cm", "gap error while steady (cm)", True),
        ("steady_worst_cm", "worst steady gap error (cm)", True),
        ("settle_s", "settling time, average (s)", True),
        ("settle_worst_s", "settling time, worst (s)", True),
        ("overshoot_cm", "worst overshoot, too close (cm)", True),
        ("min_gap_cm", "smallest gap of all runs (cm)", False),
    ]:
        a, b = off[key], on[key]
        change = b - a
        good = (change < 0) if better_low else (change > 0)
        mark = "better" if abs(change) > 0.05 and good else ("worse" if abs(change) > 0.05 else "same")
        print(f"{label:<34}{a:10.2f}{b:10.2f}{change:+9.2f}  {mark}")
    print(f"{'collisions':<34}{off['collisions']:10d}{on['collisions']:10d}")
    print()
    print(f"({off['runs']} runs on each side, same random numbers, same scenarios.)")
    print("Write BOTH columns in your report, and say plainly which is better.")
    return 0
if __name__ == "__main__":
    sys.exit(main())
