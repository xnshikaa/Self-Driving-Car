"""
lead_predictor.py - the small AI that guesses what the lead vehicle will do next.
WHY? The ultrasonic sensor tells us where the lead vehicle IS. By the time we
react it has already moved. If we can guess its speed HALF A SECOND FROM NOW,
the ACC can start easing off before the gap shrinks, which feels much smoother
and keeps the gap much closer to the setting.
WHAT KIND OF AI? Classical machine learning from scikit-learn, not a neural
network with millions of weights:
    "ridge" = a straight-line fit (linear regression with a safety belt that
              stops it chasing noise). Trains in a blink, easy to explain.
    "mlp"   = a very small neural network (one hidden layer). Try it second
              and compare the error - often it is barely better.
THE FEATURES (what the model looks at) - the last 0.5 s of driving:
    gap now, how much the gap changed in the last 0.25 s and 0.5 s,
    the estimated lead speed now / 0.25 s ago / 0.5 s ago,
    our own speed, and the smallest, largest and average lead speed.
THE TARGET (what it must guess): how much the lead vehicle speed will CHANGE
in the next 0.5 s - not the speed itself. That sounds like a detail but it is
the most important decision in this file. If the model had to guess the speed,
a model that is a little bit wrong in one direction would make the car keep a
gap that is always too big or always too small. Guessing the CHANGE means that
a lazy model which always answers "no change" is exactly the old baseline, so
anything it does learn can only help.
THE BASELINE we must beat: "the lead vehicle keeps doing what it is doing
now" (constant speed). If the model cannot beat that, we keep the baseline -
an honest result is worth more than a fancy one.
"""
import collections
import os
import config
FEATURE_NAMES = [
    "gap_m", "gap_change_025", "gap_change_050", "lead_now", "lead_025", "lead_050",
    "own_speed", "lead_min", "lead_max", "lead_mean",
]
def make_features(samples):
    """samples = list of (time_s, gap_m, lead_mps, own_mps), oldest first, about 20 per second.
    Returns a list of numbers (the features), or None if there is not enough history.
    """
    if len(samples) < 3:
        return None
    now_t, gap_now, lead_now, own_now = samples[-1]
    span = now_t - samples[0][0]
    if span < config.HISTORY_S * 0.6:            # less than about 0.3 s of history
        return None
    def at(age_s):
        """The sample closest to age_s seconds ago."""
        best = samples[0]
        for sample in samples:
            if abs((now_t - sample[0]) - age_s) < abs((now_t - best[0]) - age_s):
                best = sample
        return best
    quarter = at(0.25)
    half = at(0.50)
    leads = [s[2] for s in samples]
    return [
        gap_now,
        gap_now - quarter[1],
        gap_now - half[1],
        lead_now,
        quarter[2],
        half[2],
        own_now,
        min(leads),
        max(leads),
        sum(leads) / len(leads),
    ]
def baseline_prediction(samples, horizon_s):
    """The simple guess: the lead vehicle keeps its present speed, with a little
    of its recent acceleration added. This is what the model has to beat."""
    if not samples:
        return 0.0
    now_t, _, lead_now, _ = samples[-1]
    older = samples[0]
    dt = now_t - older[0]
    if dt < 0.15:
        return lead_now
    acceleration = (lead_now - older[2]) / dt
    acceleration = max(-1.5, min(1.5, acceleration))
    guess = lead_now + 0.5 * acceleration * horizon_s      # only half the acceleration: be careful
    return max(0.0, min(config.PREDICT_MAX_MPS, guess))
class LeadPredictor:
    """Keeps the last half second of data and predicts the lead speed 0.5 s ahead.
        predictor = LeadPredictor()          # loads models/lead_predictor.joblib if it exists
        value = predictor.update(telemetry)  # call for every telemetry message
    """
    def __init__(self, model_file=None, horizon_s=None, blend=None):
        self.horizon_s = horizon_s if horizon_s is not None else config.PREDICT_HORIZON_S
        self.blend = blend if blend is not None else config.PREDICT_BLEND
        self.samples = collections.deque(maxlen=40)     # 2 seconds at 20 Hz
        self.model = None
        self.model_name = "baseline (constant speed)"
        self.last_features = None
        path = model_file or config.MODEL_FILE
        if path and os.path.exists(path):
            try:
                import joblib
                self.model = joblib.load(path)
                self.model_name = os.path.basename(path)
            except Exception as error:
                print("Could not load the model:", error, "- using the baseline instead.")
    def add(self, time_s, gap_m, lead_mps, own_mps):
        self.samples.append((time_s, gap_m, lead_mps, own_mps))
        while self.samples and time_s - self.samples[0][0] > config.HISTORY_S * 2:
            self.samples.popleft()
    def predict(self):
        """Returns the predicted lead speed (m/s) for horizon_s from now."""
        window = [s for s in self.samples if self.samples[-1][0] - s[0] <= config.HISTORY_S]
        if len(window) < 3:
            return None
        lead_now = window[-1][2]
        baseline = baseline_prediction(window, self.horizon_s)
        features = make_features(window)
        self.last_features = features
        if self.model is None or features is None:
            return baseline
        try:
            change = float(self.model.predict([features])[0])       # the model guesses the CHANGE
        except Exception as error:
            print("Prediction failed:", error)
            self.model = None
            return baseline
        # blend: mostly what the model says, a little of the simple acceleration guess
        delta = self.blend * change + (1.0 - self.blend) * (baseline - lead_now)
        if abs(delta) < config.PREDICT_DEADBAND_MPS:
            delta = 0.0            # too small to be sure: say nothing rather than something wrong
        return max(0.0, min(config.PREDICT_MAX_MPS, lead_now + delta))
    def update(self, telemetry):
        """Convenience: feed one telemetry message in, get a prediction out."""
        if telemetry is None or not telemetry.tracking:
            self.samples.clear()
            return None
        self.add(telemetry.received, telemetry.gap_m, telemetry.lead_mps, telemetry.speed_mps)
        return self.predict()
# ----------------------------------------------------------------------
#  Turning a recorded run into training data (used by tools/train_predictor.py)
# ----------------------------------------------------------------------
def true_lead_speed(rows, index, window=3):
    """The lead vehicle speed measured AFTERWARDS, which is more accurate than
    the estimate the car had at the time: we look at how the gap changed just
    before and just after this moment (a centred difference) and add our own speed.
        lead speed = our speed + (gap change / time)
    """
    start = max(0, index - window)
    end = min(len(rows) - 1, index + window)
    if end <= start:
        return None
    dt = rows[end]["t"] - rows[start]["t"]
    if dt <= 0.05:
        return None
    gap_rate = (rows[end]["gap_m"] - rows[start]["gap_m"]) / dt
    own = sum(rows[i]["speed_mps"] for i in range(start, end + 1)) / (end - start + 1)
    return max(0.0, min(config.PREDICT_MAX_MPS, own + gap_rate))
def build_dataset(rows, horizon_s=None):
    """rows = the dictionaries read from one log CSV (sorted by time).
    Returns X (a list of feature lists), y (how much the lead speed CHANGES over
    the next horizon_s) and y_baseline (the change the simple guess expected)."""
    horizon_s = horizon_s if horizon_s is not None else config.PREDICT_HORIZON_S
    features, targets, baselines = [], [], []
    for index, row in enumerate(rows):
        if not row["tracking"]:
            continue
        window = [(r["t"], r["gap_m"], r["lead_mps"], r["speed_mps"])
                  for r in rows[max(0, index - 20):index + 1]
                  if row["t"] - r["t"] <= config.HISTORY_S and r["tracking"]]
        if len(window) < 3:
            continue
        # find the row about horizon_s later
        future = None
        for j in range(index, len(rows)):
            if rows[j]["t"] - row["t"] >= horizon_s:
                future = j
                break
        if future is None:
            continue
        future_speed = true_lead_speed(rows, future)
        now_speed = true_lead_speed(rows, index)
        vector = make_features(window)
        if future_speed is None or now_speed is None or vector is None:
            continue
        features.append(vector)
        targets.append(future_speed - now_speed)                    # the CHANGE
        baselines.append(baseline_prediction(window, horizon_s) - window[-1][2])
    return features, targets, baselines
def train_model(features, targets, kind=None):
    """Fits one of the two small models. Returns the trained pipeline.
    A PIPELINE is a little assembly line: first StandardScaler makes every
    feature the same size (metres and m/s are very different numbers), then
    the model itself learns. Saving the pipeline saves both steps, so the
    car scales its features exactly the same way as the training did.
    """
    from sklearn.linear_model import Ridge
    from sklearn.neural_network import MLPRegressor
    from sklearn.pipeline import make_pipeline
    from sklearn.preprocessing import StandardScaler
    kind = (kind or config.PREDICT_MODEL).lower()
    if kind == "mlp":
        model = MLPRegressor(hidden_layer_sizes=(16,), activation="relu", solver="lbfgs",
                             max_iter=2000, random_state=0)
    else:
        model = Ridge(alpha=1.0)
    pipeline = make_pipeline(StandardScaler(), model)
    pipeline.fit(features, targets)
    return pipeline
def mean_absolute_error(predicted, truth):
    if not predicted:
        return float("nan")
    return sum(abs(p - t) for p, t in zip(predicted, truth)) / len(predicted)
