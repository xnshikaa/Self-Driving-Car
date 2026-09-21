// =====================================================================
//  config.h  —  EVERY number you may need to change lives in this file.
//  Project 4: Adaptive Cruise Control (ACC) Prototype  (ESP32 = the whole controller)
// =====================================================================
//  How to use: build the car, run the test sketches in /tests, write
//  down what you measure, and copy those numbers here.
// =====================================================================
#pragma once
#include <Arduino.h>
// ---------------------------------------------------------------------
// 1) PINS  (ESP32 DevKit V1 — GPIO numbers, NOT the physical pin order)
// ---------------------------------------------------------------------
// Drive motor
const int PIN_MOTOR_PWM   = 25;   // BLDC driver speed input (PWM)  — or the ESC signal wire
const int PIN_MOTOR_DIR   = 26;   // BLDC driver direction input (CW/CCW)
const int PIN_MOTOR_BRAKE = 27;   // BLDC driver brake input (set to -1 if your motor has no brake wire)
const int PIN_SPEED_PULSE = 13;   // FG speed output of the BLDC motor (or the optical slot sensor)
// HC-SR04 ultrasonic sensor (gap to the lead vehicle). ECHO through a 1 kOhm / 2 kOhm divider!
const int PIN_US_TRIG = 18;
const int PIN_US_ECHO = 19;
// Driver interface (HMI): OLED display, set-speed knob, two push buttons
const int PIN_I2C_SDA    = 21;    // SSD1306 OLED
const int PIN_I2C_SCL    = 22;
const int PIN_POT        = 34;    // potentiometer middle leg (input-only analog pin)
const int PIN_BTN_ENGAGE = 33;    // button to GND: ACC on / off
const int PIN_BTN_GAP    = 14;    // button to GND: gap 40/50/60 cm (hold 1 s: AI prediction on/off)
// UART link to the Raspberry Pi (uses ESP32 "Serial2")
const int  PIN_PI_RX = 16;        // ESP32 RX2  <-  Pi TX (GPIO14, physical pin 8)
const int  PIN_PI_TX = 17;        // ESP32 TX2  ->  Pi RX (GPIO15, physical pin 10)
const long PI_BAUD   = 115200;
// Battery voltage sensing through a resistor divider (100k top, 22k bottom)
const int   PIN_BATTERY = 36;                         // "VP" pin, analog input
const float BATTERY_DIVIDER = (100.0f + 22.0f) / 22.0f;
const int   PIN_STATUS_LED = 2;                       // blue LED on the ESP32 board
// ---------------------------------------------------------------------
// 2) DRIVE MOTOR  (check with test t2)
// ---------------------------------------------------------------------
const int DRIVE_BLDC_DRIVER = 0;  // 12 V geared BLDC motor with a built-in driver (PWM + DIR + BRAKE + FG 
wires)
const int DRIVE_ESC         = 1;  // A2212 motor + 30 A ESC (servo-style pulses 1000-2000 us)
const int DRIVE_TYPE        = DRIVE_BLDC_DRIVER;
// Built-in BLDC driver
const int  MOTOR_PWM_FREQ_HZ     = 20000; // most drivers accept 1-25 kHz; 20 kHz is silent
const bool MOTOR_PWM_INVERTED    = false; // true if your motor runs FASTER with a SMALLER duty cycle (test 
t2)
const int  MOTOR_DIR_FORWARD     = HIGH;  // level on the DIR wire that drives the car FORWARD (test t2)
const int  MOTOR_BRAKE_ACTIVE    = LOW;   // level on the BRAKE wire that brakes (many drivers: LOW = brake)
// ESC (only if DRIVE_TYPE = DRIVE_ESC)
const int  ESC_MIN_US  = 1000;   // pulse for "stop"
const int  ESC_MAX_US  = 2000;   // pulse for "full throttle"
const unsigned long ESC_ARM_MS = 3000;  // an ESC must see "stop" for a few seconds before it starts
// ---------------------------------------------------------------------
// 3) SPEED MEASUREMENT  (measure YOUR car: test t3)
// ---------------------------------------------------------------------
const float WHEEL_DIAMETER_M       = 0.085f; // drive wheel diameter in metres
const float PULSES_PER_WHEEL_REV   = 180.0f; // FG: pulses per motor turn x gear ratio. Optical disc: number 
of slots.
const unsigned long MIN_PULSE_GAP_US = 300;  // pulses closer than this are electrical noise and ignored
// ---------------------------------------------------------------------
// 4) INNER LOOP: SPEED PID, 100 times per second (spec: >= 100 Hz)
// ---------------------------------------------------------------------
const unsigned long SPEED_LOOP_MS = 10;
const int   PWM_BITS  = 10;              // 10-bit PWM -> numbers from 0 to 1023
const int   PWM_MAX   = 1023;
const float SPEED_FF_PWM_START   = 150.0f;  // PWM needed just to start moving (test t3 sweep)
const float SPEED_FF_PWM_PER_MPS = 1000.0f; // extra PWM needed per 1 m/s (test t3 sweep)
const float SPEED_KP = 600.0f;           // PWM per (m/s) of error
const float SPEED_KI = 3000.0f;          // PWM per (m/s x second)
const float SPEED_KD = 0.0f;
const float SPEED_INTEGRAL_BAND = 0.15f; // I part only works when the speed error is below 0.15 m/s
const float SPEED_FILTER_ALPHA  = 0.30f; // 0..1, smaller = smoother but slower speed reading
const int   PWM_MAX_STEP        = 25;    // slew-rate limit: max PWM change per 10 ms step
const float BRAKE_ASSIST_MPS    = 0.12f; // brake when we are this much faster than the target speed
// ---------------------------------------------------------------------
// 5) OUTER LOOP: SET SPEED AND GAP CONTROL, 50 times per second
// ---------------------------------------------------------------------
const unsigned long GAP_LOOP_MS = 20;
const float SET_SPEED_MIN_MPS  = 0.20f;  // knob fully left
const float SET_SPEED_MAX_MPS  = 0.80f;  // knob fully right
const float SET_SPEED_STEP_MPS = 0.05f;  // the set speed moves in steps of 0.05 m/s
const float GAP_SETTINGS_M[3]  = {0.40f, 0.50f, 0.60f};  // GAP button cycles through these (spec: hold 50 
cm)
const int   GAP_DEFAULT_INDEX  = 1;
const float GAP_MODE_ENTER_M   = 1.50f;  // spec: GAP mode if the lead vehicle is within 1.5 m ...
const float GAP_MODE_EXIT_M    = 1.70f;  // ... and back to SPEED mode beyond 1.7 m (hysteresis) ...
const unsigned long GAP_EXIT_HOLD_MS = 500;  // ... for at least 0.5 s
const float GAP_KP      = 0.80f;   // (m/s) of speed per metre of gap error
const float GAP_KI      = 0.10f;   // (m/s) per (metre x second)
const float GAP_I_LIMIT = 0.15f;   // the I part may add or remove at most 0.15 m/s
const float ACCEL_LIMIT_MPS2 = 0.70f;  // gentle speed-up (a real car uses about 1 m/s^2)
const float DECEL_LIMIT_MPS2 = 1.50f;  // firmer slow-down (emergencies ignore this limit)
const float STOP_SPEED_MPS   = 0.03f;  // slower than this = standing still
const float RESUME_GAP_M     = 0.08f;  // leave STOP when the gap grows 8 cm beyond the set gap ...
const float RESUME_LEAD_MPS  = 0.10f;  // ... or the lead vehicle moves faster than 0.1 m/s
const float STOP_ENTER_LEAD_MPS = 0.05f;  // only stop behind a lead vehicle slower than this
const unsigned long MODE_DWELL_MS = 300;  // a mode must last this long before we may swap back
const unsigned long RESUME_HOLD_MS = 200; // the lead must really be moving for this long before we follow
// Safety layer (spec: emergency brake if the gap is below Dmin = 25 cm)
const float D_MIN_M            = 0.25f;
const float EMERG_REACTION_S   = 0.10f;  // the dynamic limit adds the distance we need to stop:
const float SENSOR_LAG_S       = 0.10f;  //   Dmin + speed x (reaction + sensor lag) + speed^2 / (2 x decel)
const float EMERG_DECEL_MPS2   = 2.00f;  // the median filter makes every reading about 0.1 s old, and in
                                         // that time the gap has already shrunk: SENSOR_LAG_S pays for it
const float EMERG_MARGIN_M     = 0.02f;  // one standard deviation of the sensor, as a safety margin
const float EMERG_CLEAR_M      = 0.10f;  // leave EMERGENCY only when the gap is 10 cm larger than Dmin
// ---------------------------------------------------------------------
// 6) ULTRASONIC SENSOR AND GAP FILTER  (characterise with test t4)
// ---------------------------------------------------------------------
const float SOUND_SPEED_MPS    = 346.0f; // at 25 degC. Use 331.3 + 0.606 x temperature(degC)
const unsigned long US_PING_MS = 50;     // spec: gap sensing at 20 Hz
const float US_MAX_RANGE_M     = 3.0f;   // echoes longer than this count as "nothing there"
const float US_FAR_M           = 4.0f;   // the distance we report when nothing is there
const float US_MIN_RANGE_M     = 0.03f;
const int   US_MEDIAN_WINDOW   = 5;      // median of 5 removes single wrong readings
const float US_FAST_CLOSE_M    = 0.15f;  // two readings in a row this much closer than the median are 
believed at once
const int   US_FAULT_COUNT     = 3;      // this many pings in a row with NO echo pulse = sensor fault
const float SENSOR_TO_BUMPER_M = 0.00f;  // if the bumper sticks out in front of the sensor, its length
// Kalman filter (state = gap and the lead vehicle's speed)
const float KF_MEAS_STD_M = 0.02f;       // how noisy one filtered reading is (metres)
const float KF_ACCEL_STD  = 1.0f;        // how quickly the lead vehicle's speed may change (m/s^2)
const float KF_GATE_M     = 0.35f;       // a reading further than this from the prediction is suspicious
// ---------------------------------------------------------------------
// 7) TIMING, PI LINK AND BATTERY
// ---------------------------------------------------------------------
const unsigned long TELEMETRY_PERIOD_MS  = 50;   // status to the Pi 20 times per second
const unsigned long REMOTE_TIMEOUT_MS    = 500;  // a test started by the Pi stops if its orders stop
const unsigned long PREDICTION_MAX_AGE_MS = 200; // an AI prediction older than this is ignored
const unsigned long OLED_PAGE_MS         = 25;   // one of the 8 display rows is redrawn every 25 ms
const float BATTERY_LOW_V      = 10.0f;  // stop below this (3.33 V per cell)
const float BATTERY_RESUME_V   = 10.4f;
const float BATTERY_PRESENT_V  = 5.0f;   // below this we assume "USB only, no battery"
const bool DEBUG_PRINT = true;           // human-readable status on the USB Serial Monitor
