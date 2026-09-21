// Exact port of parameters from config.h and acc_sim.py
// DO NOT CHANGE THESE GAINS OR THRESHOLDS - THEY ARE THE SOURCE OF TRUTH

export const SPEED_LOOP_S = 0.010;
export const GAP_LOOP_S = 0.020;
export const PWM_MAX = 1023;
export const SPEED_FF_PWM_START = 150.0;
export const SPEED_FF_PWM_PER_MPS = 1000.0;
export const SPEED_KP = 600.0;
export const SPEED_KI = 3000.0;
export const SPEED_INTEGRAL_BAND = 0.15;
export const SPEED_FILTER_ALPHA = 0.30;
export const PWM_MAX_STEP = 25;
export const BRAKE_ASSIST_MPS = 0.12;

export const GAP_MODE_ENTER_M = 1.50;
export const GAP_MODE_EXIT_M = 1.70;
export const GAP_EXIT_HOLD_S = 0.50;
export const GAP_KP = 0.80;
export const GAP_KI = 0.10;
export const GAP_I_LIMIT = 0.15;
export const ACCEL_LIMIT_MPS2 = 0.70;
export const DECEL_LIMIT_MPS2 = 1.50;
export const STOP_SPEED_MPS = 0.03;
export const RESUME_GAP_M = 0.08;
export const RESUME_LEAD_MPS = 0.10;
export const STOP_ENTER_LEAD_MPS = 0.05;
export const MODE_DWELL_S = 0.30;
export const RESUME_HOLD_S = 0.20;

// Requirement Distinct Safety Thresholds
export const D_MIN_M = 0.25;                 // Internal tuned Dmin (guide tuned value)
export const OFFICIAL_D_MIN_M = 0.20;        // Official Requirement R3/R5 (20 cm)
export const TARGET_GAP_M = 0.50;            // Driver target gap (50 cm)
export const TARGET_GAP_TOLERANCE_M = 0.08;  // R2 tolerance (50 cm ± 8 cm)
export const SPEED_HOLD_TOLERANCE_PCT = 5.0; // R1 speed accuracy (± 5%)
export const SENSOR_FAULT_SHUTDOWN_MS = 300; // R7 sensor fault limit (300 ms)

export const EMERG_REACTION_S = 0.10;
export const SENSOR_LAG_S = 0.10;
export const EMERG_DECEL_MPS2 = 2.00;
export const EMERG_MARGIN_M = 0.02;
export const EMERG_CLEAR_M = 0.10;

export const US_PING_S = 0.050;
export const US_MAX_RANGE_M = 3.0;
export const US_FAR_M = 4.0;
export const US_MIN_RANGE_M = 0.03;
export const US_MEDIAN_WINDOW = 5;
export const US_FAST_CLOSE_M = 0.15;
export const US_FAULT_COUNT = 3;

export const KF_MEAS_STD_M = 0.02;
export const KF_ACCEL_STD = 1.0;
export const KF_GATE_M = 0.35;
export const PREDICTION_MAX_AGE_S = 0.20;
export const TELEMETRY_S = 0.050;

// Pretend Car Parameters (Physics abstraction)
export const REAL_FF_START = 168.0;
export const REAL_FF_PER_MPS = 1085.0;
export const SPEED_TAU_S = 0.30;
export const BRAKE_DECEL_MPS2 = 2.2;
export const SPEED_NOISE_MPS = 0.010;
export const US_NOISE_M = 0.015;
export const US_DROPOUT = 0.02;

export const MODE_NAMES = ["OFF", "SPEED", "GAP", "STOP", "EMERG", "FAULT", "LOW BAT"];
export const MODE_OFF = 0;
export const MODE_SPEED = 1;
export const MODE_GAP = 2;
export const MODE_STOP = 3;
export const MODE_EMERG = 4;
export const MODE_FAULT = 5;
export const MODE_LOWBAT = 6;
