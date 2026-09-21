"""
tools/analyse_run.py - turns one or many run logs into the numbers for your report.
    python3 tools/analyse_run.py logs/run_2026-05-04_10-31-22.csv
    python3 tools/analyse_run.py logs/*.csv            # a table of every run
    python3 tools/analyse_run.py --compare logs/no_ai.csv logs/with_ai.csv
The single-file view prints everything acc_metrics.py can measure plus the
acceptance checks. The many-file view prints one line per run so you can see
at a glance whether the car behaves the same way every time (repeatability).
"""
import argparse
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import acc_metrics   # noqa: E402
def table(paths):
    print(f"{'file':<34}{'min gap':>9}{'steady':>9}{'max err':>9}{'settle':>8}{'accel':>8}{'emerg':>7}")
    rows = []
    for path in paths:
        data = acc_metrics.read_log(path)
        if not data:
            print(f"{os.path.basename(path):<34}  (empty)")
            continue
        s = acc_metrics.summarise(data)
        rows.append(s)
        print(f"{os.path.basename(path):<34}{s['min_gap_m'] * 100:8.1f}cm"
              f"{s['gap_error_settled_m'] * 100:8.1f}cm{s['gap_error_max_m'] * 100:8.1f}cm"
              f"{s['settle_max_s']:7.2f}s{s['accel_max_mps2']:8.2f}{s['emergencies']:7d}")
    if len(rows) > 1:
        def average(key):
            values = [r[key] for r in rows if r[key] == r[key]]
            return sum(values) / len(values) if values else float("nan")
        print(f"{'AVERAGE':<34}{average('min_gap_m') * 100:8.1f}cm"
              f"{average('gap_error_settled_m') * 100:8.1f}cm{average('gap_error_max_m') * 100:8.1f}cm"
              f"{average('settle_max_s'):7.2f}s{average('accel_max_mps2'):8.2f}")
    return rows
def compare(path_a, path_b):
    a = acc_metrics.summarise(acc_metrics.read_log(path_a))
    b = acc_metrics.summarise(acc_metrics.read_log(path_b))    
print(f"{'measurement':<28}{os.path.basename(path_a)[:16]:>18}{os.path.basename(path_b)[:16]:>18}{'change':>1
2}")
    for key, name, scale, unit in [
        ("min_gap_m", "smallest gap", 100, "cm"),
        ("gap_error_settled_m", "steady gap error", 100, "cm"),
        ("gap_error_max_m", "largest gap error", 100, "cm"),
        ("overshoot_m", "overshoot (too close)", 100, "cm"),
        ("settle_max_s", "worst settling time", 1, "s"),
        ("accel_max_mps2", "largest acceleration", 1, "m/s2"),
    ]:
        first, second = a.get(key, float("nan")) * scale, b.get(key, float("nan")) * scale
        change = second - first
        print(f"{name:<28}{first:14.2f}{unit:>4}{second:14.2f}{unit:>4}{change:+11.2f}")
def main():
    parser = argparse.ArgumentParser(description="Analyse ACC run logs.")
    parser.add_argument("logs", nargs="+")
    parser.add_argument("--compare", action="store_true", help="compare exactly two logs")
    args = parser.parse_args()
    if args.compare:
        if len(args.logs) != 2:
            print("--compare needs exactly two files")
            return 1
        compare(args.logs[0], args.logs[1])
        return 0
    if len(args.logs) == 1:
        acc_metrics.print_report(args.logs[0])
        return 0
    table(args.logs)
    return 0
if __name__ == "__main__":
    sys.exit(main())
