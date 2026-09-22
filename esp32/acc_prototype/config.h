#pragma once

// ===================== VERIFY THESE PINS =====================
// Proposed mapping for a common 38-pin ESP32 NodeMCU/DevKit board.
// Confirm the labels on your exact board before wiring.

// HC-SR04. ECHO MUST be reduced to <= 3.3 V with a resistor divider.
constexpr int PIN_US_TRIG = 18;  // output
constexpr int PIN_US_ECHO = 34;  // input-only GPIO; ECHO still needs a divider

// Two TB6612FNG boards provide four channels, so use one motor per channel.
// Connect both driver STBY pins to the same ESP32 GPIO.
constexpr int PIN_TB_STBY = 23;

// Motor 1: TB6612 #1 channel A
constexpr int PIN_M1_IN1 = 26;
constexpr int PIN_M1_IN2 = 27;
constexpr int PIN_M1_PWM = 25;
// Motor 2: TB6612 #1 channel B
constexpr int PIN_M2_IN1 = 32;
constexpr int PIN_M2_IN2 = 33;
constexpr int PIN_M2_PWM = 14;
// Motor 3: TB6612 #2 channel A
constexpr int PIN_M3_IN1 = 16;
constexpr int PIN_M3_IN2 = 17;
constexpr int PIN_M3_PWM = 13;
// Motor 4: TB6612 #2 channel B
// GPIO5 and GPIO12 are boot-strapping pins on classic ESP32 boards. Keep
// their driver inputs LOW/floating during reset, or move them if boot fails.
constexpr int PIN_M4_IN1 = 5;
constexpr int PIN_M4_IN2 = 12;
constexpr int PIN_M4_PWM = 19;

// SSD1306 I2C OLED.
constexpr int PIN_I2C_SDA = 21; // VERIFY WITH YOUR BOARD
constexpr int PIN_I2C_SCL = 22; // VERIFY WITH YOUR BOARD
constexpr uint8_t OLED_ADDRESS = 0x3C; // Try 0x3D if your module uses it.

// Switch to GND. INPUT_PULLUP means LOW = requested run.
constexpr int PIN_RUN_SWITCH = 4; // VERIFY WITH YOUR BOARD

// ===================== HARDWARE SETTINGS =====================
constexpr int MOTOR_PWM_FREQUENCY_HZ = 20000;
constexpr int MOTOR_PWM_BITS = 8;
constexpr int MOTOR_PWM_MAX = (1 << MOTOR_PWM_BITS) - 1;
constexpr bool LEFT_MOTOR_INVERTED = false;
constexpr bool RIGHT_MOTOR_INVERTED = false;
constexpr bool TB_STBY_ACTIVE_HIGH = true;

// ===================== ACC SETTINGS =====================
constexpr float TARGET_GAP_M = 0.50f;
constexpr float GAP_TOLERANCE_M = 0.08f;
constexpr float EMERGENCY_GAP_M = 0.20f;
constexpr float SENSOR_MAX_M = 3.00f;
constexpr float CLEAR_ROAD_M = 2.50f;
constexpr int CRUISE_PWM = 110; // Start low; tune on a raised chassis.
constexpr int MIN_MOVING_PWM = 55;
constexpr int MAX_ALLOWED_PWM = 150; // Conservative first prototype limit.
constexpr int MAX_PWM_STEP = 8;      // PWM change per control tick.
constexpr int CLOSE_PWM = 55;
constexpr int STOP_PWM = 0;
constexpr unsigned long SENSOR_PERIOD_MS = 60;
constexpr unsigned long CONTROL_PERIOD_MS = 60;
constexpr unsigned long DISPLAY_PERIOD_MS = 250;
constexpr unsigned long SERIAL_PERIOD_MS = 250;
constexpr unsigned long SWITCH_DEBOUNCE_MS = 30;
constexpr unsigned long SENSOR_TIMEOUT_US = 30000UL;
constexpr uint8_t INVALID_READINGS_TO_FAULT = 2;

// Simple proportional gap correction: PWM = CRUISE_PWM + Kp * (gap-target).
// This is open-loop PWM, not a speed PID.
constexpr float GAP_TO_PWM = 130.0f;
