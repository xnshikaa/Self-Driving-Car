# Simulation workflow

The teammate repository is:

`https://github.com/xnshikaa/Self-Driving-Car`

It contains three related layers:

- `code/raspberry_pi/sim/acc_sim.py`, `lead_profiles.py`, and `ai_benchmark.py`: a Python numerical simulation with a noisy ultrasonic model, median/fast-close filtering, a 2-state gap/lead-speed filter, ACC modes, a first-order car model, scenarios, CSV logs, and trial/AI benchmarks.
- `simulator/src/sim/`: a JavaScript port of the numerical engine (`SimulationEngine.js`, `AccLogic.js`, `GapFilter.js`, `Ranger.js`, `Car.js`, `LeadVehicle.js`, and `Metrics.js`).
- `simulator/src/components/` and `simulator/src/visualization/`: React, Three.js, charts, HUDs, timeline, and acceptance controls.

The repository also has an ESP32 suite under `code/esp32/p04_acc/` and standalone tests under `code/esp32/tests/`. Its firmware is designed around BLDC/ESC and speed feedback, so it is useful as a reference for control structure but is not drop-in code for this TB6612 prototype.

## How to use it before hardware

1. Clone the repository and run the Python simulation with `python acc_sim.py --all`, `python acc_sim.py --trials 50`, and `python acc_sim.py --scenario stop_and_go --log`.
2. Use the scenario profiles for clear road, stop, stop-and-go, slow-down, speed-up, cut-in, wavy motion, and no lead vehicle.
3. Inspect `rows`/CSV telemetry: time, mode, target speed, gap, lead speed, PWM, braking, sensor health, true gap, and true vehicle speeds.
4. Use the browser app for visual playback, sensor-failure injection, acceptance checks, trial statistics, and CSV export.
5. Before porting any logic, replace the teammate's plant assumptions with the real prototype's inputs: distance in metres, switch state, and actual PWM command. Do not use the BLDC feed-forward constants for the TB6612 motors.
6. Port only controller ideas that have matching signals: state transitions, gap hysteresis, output slew limiting, invalid-sensor stop, and test scenario definitions.
7. Validate the same scenarios on the car at much lower speed. Compare serial telemetry to the simulated time-series, but expect different PWM-to-speed behavior because the simulator's `Car.js`/`acc_sim.py` uses a first-order motor model.

## Important limitation

The repository README reports simulation PASS and pending physical BLDC validation. That is a repository claim, not a physical result. Source inspection shows the simulator and firmware are designed to mirror each other, but the simplified TB6612 firmware in this workspace is a separate implementation that must be tested independently.
