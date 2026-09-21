"""
tools/train_predictor.py - teaches the small AI to predict the lead vehicle speed.
WHERE THE DATA COMES FROM
    * real runs:       python3 main.py   (every run writes logs/run_*.csv)
    * simulated runs:  python3 sim/acc_sim.py --collect 30
  Both give the same kind of CSV, so you can train on either or on both.
WHAT HAPPENS HERE
    1. every log line becomes ONE example: the features are the last 0.5 s of
       gap and speed, and the answer is the lead speed 0.5 s LATER
    2. what the model must guess is the CHANGE of the lead speed, not the speed
       itself (see lead_predictor.py for why that matters so much)
    3. the examples are split: the first 75 % to learn from (training) and the
       last 25 % to be tested on (validation). NEVER test on data the model has
       already seen, or you fool yourself
    4. the model is compared with the honest baseline ("it keeps its speed")
    5. only if the model is better is it saved to models/lead_predictor.joblib
    python3 tools/train_predictor.py                      # everything in logs/
    python3 tools/train_predictor.py --model mlp          # the small neural net
    python3 tools/train_predictor.py logs/sim/train/*.csv
"""
import argparse
import glob
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import acc_metrics     # noqa: E402
import config          # noqa: E402
import lead_predictor  # noqa: E402
def collect_examples(paths, horizon_s):
    features, targets, baselines = [], [], []
    used = 0
    for path in paths:
        rows = acc_metrics.read_log(path)
        if len(rows) < 40:
            continue
        x, y, base = lead_predictor.build_dataset(rows, horizon_s)
        if not x:
            continue
        used += 1
        features.extend(x)
        targets.extend(y)
        baselines.extend(base)
    return features, targets, baselines, used
def main():
    parser = argparse.ArgumentParser(description="Train the lead-speed predictor.")
    parser.add_argument("logs", nargs="*", help="log files (default: everything under logs/)")
    parser.add_argument("--model", default=config.PREDICT_MODEL, choices=["ridge", "mlp"])
    parser.add_argument("--horizon", type=float, default=config.PREDICT_HORIZON_S)
    parser.add_argument("--out", default=config.MODEL_FILE)
    parser.add_argument("--force", action="store_true", help="save even if the baseline is better")
    args = parser.parse_args()
    paths = args.logs
    if not paths:
        paths = sorted(glob.glob(os.path.join(config.LOG_DIR, "**", "*.csv"), recursive=True))
        paths = [p for p in paths if "trials_" not in os.path.basename(p)]
    if not paths:
        print("No log files found. Record some runs first:")
        print("   python3 sim/acc_sim.py --collect 30      (no hardware needed)")
        print("   python3 main.py                          (on the real car)")
        return 1
    features, targets, baselines, used = collect_examples(paths, args.horizon)
    print(f"{len(features)} examples from {used} runs ({len(paths)} files looked at)")
    if len(features) < config.PREDICT_MIN_SAMPLES:
        print(f"That is too few (at least {config.PREDICT_MIN_SAMPLES}). Record more runs.")
        return 1
    cut = int(len(features) * 0.75)
    train_x, train_y = features[:cut], targets[:cut]
    test_x, test_y, test_base = features[cut:], targets[cut:], baselines[cut:]
    print(f"learning from {len(train_x)} examples, testing on {len(test_x)} unseen ones")
    model = lead_predictor.train_model(train_x, train_y, args.model)
    predicted = list(model.predict(test_x))
    model_error = lead_predictor.mean_absolute_error(predicted, test_y)
    base_error = lead_predictor.mean_absolute_error(test_base, test_y)
    print()
    print(f"average mistake of the baseline (it keeps its speed): {base_error * 100:.2f} cm/s")
    print(f"average mistake of the {args.model} model:              {model_error * 100:.2f} cm/s")
    better = (base_error - model_error) / base_error * 100 if base_error > 0 else 0.0
    print(f"the model is {better:+.1f} % better than the baseline")
    if hasattr(model[-1], "coef_"):
        print()
        print("what the model looks at (bigger number = more important):")
        pairs = sorted(zip(lead_predictor.FEATURE_NAMES, model[-1].coef_),
                       key=lambda p: -abs(p[1]))
        for name, weight in pairs:
            print(f"   {name:<16} {weight:+7.3f}")
    if model_error < base_error or args.force:
        os.makedirs(os.path.dirname(args.out), exist_ok=True)
        import joblib
        joblib.dump(model, args.out)
        print()
        print(f"saved {args.out}")
        print("Now compare the car with and without it:")
        print("   python3 sim/acc_sim.py --all          (prediction off)")
        print("   python3 sim/acc_sim.py --all --ai     (prediction on)")
    else:
        print()
        print("The model is NOT better than the baseline, so it was not saved.")
        print("That is a real result - write it in your report. Try: more runs,")
        print("--model mlp, or a shorter horizon (--horizon 0.3).")
    return 0
if __name__ == "__main__":
    sys.exit(main())
