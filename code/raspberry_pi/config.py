"""
config.py - EVERY setting for the Raspberry Pi side of Project 4 (Adaptive Cruise Control).
Change numbers here, not inside the other files. Each setting has a comment
that says what it does, and the units are written into the names (_M, _S, _HZ).
IMPORTANT: the ACC decisions themselves are made by the ESP32. Its numbers
live in esp32/p04_acc/config.h. The Pi only watches, logs, measures and
(optionally) predicts what the lead vehicle will do next.
"""
import os
BASE_DIR = os.path.dirname(os.path.abspath(__file__))   # the folder this file is in
# ----------------------------------------------------------------------
# 1) Link to the ESP32
# ----------------------------------------------------------------------
SERIAL_PORT = "/dev/serial0"   # the GPIO14/15 wires. With a USB cable use "/dev/ttyUSB0"
SERIAL_BAUD = 115200
PREDICTION_RATE_HZ = 20        # how often we send the AI prediction ($P) to the ESP32
DASHBOARD_RATE_HZ = 5          # how often main.py refreshes the text dashboard
# ----------------------------------------------------------------------
# 2) Driving limits used by the Pi tools
# ----------------------------------------------------------------------
SET_SPEED_MPS = 0.60           # the set speed the tools ask for (spec tests: 0.3 - 0.8 m/s)
MAX_SPEED_MPS = 0.80           # never ask the car for more than this
GAP_SETTINGS_M = [0.40, 0.50, 0.60]   # the same three settings as the GAP button
DEFAULT_GAP_M = 0.50           # the spec asks for a 50 cm gap
# ----------------------------------------------------------------------
# 3) Files
# ----------------------------------------------------------------------
LOG_DIR = os.path.join(BASE_DIR, "logs")            # one CSV per run
MODEL_FILE = os.path.join(BASE_DIR, "models", "lead_predictor.joblib")
# ----------------------------------------------------------------------
# 4) The AI lead-speed predictor  (classical machine learning, scikit-learn)
# ----------------------------------------------------------------------
# What it learns: "given how the gap changed over the last 0.5 s, how fast
# will the lead vehicle be moving 0.5 s from now?" A good guess lets the ACC
# start slowing BEFORE the gap gets small, which feels much smoother.
PREDICT_HORIZON_S = 0.50       # how far into the future we predict
HISTORY_S = 0.50               # how much past data one prediction looks at
SAMPLE_PERIOD_S = 0.05         # telemetry arrives every 50 ms (20 Hz)
PREDICT_MODEL = "ridge"        # "ridge" (a straight-line fit) or "mlp" (a small neural net)
PREDICT_MIN_SAMPLES = 200      # refuse to train on fewer samples than this
PREDICT_MAX_MPS = 1.00         # a prediction is never allowed above this
PREDICT_BLEND = 0.70           # 0 = ignore the model, 1 = trust it fully (0.7 = mostly the model)
PREDICT_DEADBAND_MPS = 0.03    # ignore predicted changes smaller than this. No model is perfect:
                               # this stops a small constant mistake from pushing the gap away
                               # from the setting, and only lets real changes through.
# ----------------------------------------------------------------------
# 5) Acceptance limits - straight from the project specification
# ----------------------------------------------------------------------
# These five are the tests your project is marked against (Section 11 of the spec):
ACCEPT_SPEED_ERROR_PCT = 5.0   # A. speed hold: within +/- 5 % of the set speed
ACCEPT_GAP_ERROR_M = 0.08      # B. gap hold: 50 cm +/- 8 cm behind a lead doing 0.3-0.6 m/s
ACCEPT_STOP_GAP_M = 0.20       # C. stop-and-go: stop at least 20 cm behind a standing lead
ACCEPT_SWITCH_SPIKE_PCT = 10.0 # D. mode switching: no speed jump bigger than 10 % of the set speed
ACCEPT_CUTIN_MIN_M = 0.20      # E. emergency: never closer than 20 cm in the cut-in trials
# Extra limits we set ourselves, to know whether the car drives NICELY as well as safely:
ACCEPT_MIN_GAP_M = 0.25        # our own Dmin, 5 cm stricter than the spec asks
ACCEPT_SETTLE_S = 4.0          # after the lead vehicle changes speed, settle within this
ACCEPT_OVERSHOOT_M = 0.15      # we may come at most this much CLOSER than the set gap
ACCEPT_ACCEL_MPS2 = 1.20       # comfort: how hard we may speed up (the order rises at 0.7,
                               # the car itself may overshoot a little at the start)
ACCEPT_DECEL_MPS2 = 2.50       # how hard we may brake in normal driving (emergencies excluded)
