# ACC Prototype Software Architecture

## Scope and evidence boundaries

The attached PDF is treated as reference context, not as a new instruction from the user. Its design calls for a BLDC/ESC drive, encoder speed feedback, cascaded speed and gap PID loops, a 1-D Kalman filter, optional Raspberry Pi prediction, and an OLED interface. The user's actual request and hardware list take priority.

The supplied kit photo confirms a 4WD chassis, four small geared DC motors, wheels, two encoder discs, a 4xAA holder, and wires. It does not show encoder sensors. The user has also confirmed a 38-pin ESP32 NodeMCU, a 0.96-inch 4-pin I2C OLED, and two TB6612FNG modules. The GPIO mapping in `esp32/acc_prototype/config.h` is a proposed mapping for that board family and must still be checked against the labels on the physical boards.

## Recommended architecture

| Component | What it does | Required now | Technology and connection |
|---|---|---:|---|
| ESP32 firmware | Owns the real-time loop and fail-safe stop | Yes | Arduino framework C++ in one sketch folder. Reads sensor/switch, calls ACC, commands TB6612, updates OLED, prints serial telemetry. |
| HC-SR04 driver | Generates trigger pulse, measures ECHO, rejects timeouts, filters readings | Yes | Small C++ module using `pulseIn`, timeout, and a 3-sample median. Publishes metres plus a validity flag to ACC. |
| Sensor/data processing | Converts microseconds to metres and smooths noise | Yes, minimal | In the ESP32 distance module. No Kalman filter is required for the first physical test. |
| ACC control logic | Selects OFF, CRUISE, FOLLOW, STOP, or FAULT and calculates a safe PWM request | Yes | Plain C++ state machine. Distance error is bounded and output is rate-limited. It sends a `MotorCommand` to the motor module. |
| Motor control | Drives four TB6612FNG channels, standby, direction, PWM, and gentle ramping | Yes | Plain C++ using ESP32 LEDC-compatible PWM. One motor is connected to each channel; all four receive the same forward command initially. |
| OLED software | Shows mode, distance, target gap, PWM, and fault status | Yes | Adafruit SSD1306 + Adafruit GFX over I2C. The display is informational; it must never be the only safety mechanism. |
| Start/stop input | Enables or disables ACC | Yes | Debounced `INPUT_PULLUP` switch to GND. A switch open means stop. |
| Simulation/testing | Tests controller behavior, scenarios, faults, and tuning without crashing the car | Yes for development; not required on car | Use the teammate repository's Python numerical engine and React/Three.js UI. Add unit-style dry runs for this simplified controller. |
| PC/laptop software | Flashing, serial monitor, CSV logging, and plotting | Helpful, not required to drive | Arduino IDE or PlatformIO; optional Python later for log plots. USB Serial is the only current connection. |
| Optional future encoder module | Measures wheel speed and distance travelled | No | ESP32 GPIO interrupt plus a pulse counter. Enables a true inner speed loop and better gap prediction. |
| Optional future Kalman filter | Estimates gap and lead speed from noisy distance data | No | Plain C++ 2-state filter after the basic controller is stable. |
| Optional future Raspberry Pi/PC link | Sends telemetry or runs prediction/visualization | No | USB/UART and Python. Keep outside the safety-critical stop path. |

## Data flow

```text
HC-SR04 -> DistanceReading(valid, metres) -> ACC state machine
Start/stop switch -------------------------> ACC state machine
ACC state machine -> MotorCommand ----------> TB6612FNG -> four brushed motors
All state ----------------------------------> OLED and USB Serial
```

## Current controller behavior

- Switch off: motor PWM is zero and TB6612 standby is asserted.
- Invalid or timed-out ultrasonic reading: enter FAULT and stop.
- Valid target farther than the target gap: command cruise PWM, but ramp upward.
- Valid target inside the target gap: reduce PWM in proportion to gap error.
- At or below the emergency gap: command zero PWM and brake/standby.
- A valid reading in the far part of the configured range is treated as clear and uses cruise PWM. A timeout, out-of-range value, or repeated invalid reading stops the car; this is safer for a first prototype, although it means a clear lane beyond the HC-SR04 range will not automatically count as clear.

This is intentionally not the PDF's cascaded speed/gap PID. Without a verified encoder, a speed PID would be pretending to have feedback it does not have.

## Difference from the reference design

The reference's A2212/BLDC + ESC path is replaced by the 4WD brushed motors and TB6612FNG. The reference's encoder speed loop is omitted until an encoder sensor is actually wired. The Raspberry Pi predictor, Kalman filter, battery monitor, dual HMI buttons, UART protocol, and emergency dynamic stopping calculation are not needed for the first low-cost prototype. They can be added after distance sensing, motor polarity, power, and basic stopping behavior are characterized.

## Development path

1. Run the teammate simulator's `acc_sim.py`, scenarios, acceptance metrics, and browser visualization.
2. Test the ESP32 serial boot and GPIO modules with the standalone sketches.
3. Test HC-SR04 on a stationary car and record noisy readings.
4. Test each TB6612 motor channel with wheels lifted, then test both sides at low PWM.
5. Combine distance and motor control with the switch disconnected until the stop path is verified.
6. Enable the switch and run the simplified ACC at low speed behind a large cardboard target.
7. Log serial CSV-like telemetry, tune one constant at a time, and only then add optional encoders/Kalman/UART.
