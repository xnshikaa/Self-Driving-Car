# Project 4: Adaptive Cruise Control (ACC) Prototype

**Course:** Intelligent Transportation Systems & Autonomous Mobility  
**Program:** B.Tech Computer Science (AI & ML)  
**Academic Validation Status:** `SIMULATION RESULT: PASS` | `PHYSICAL VALIDATION: PENDING ESP32 + BLDC HARDWARE`

---

## 1. Project Overview

This repository contains the software architecture, 1-D Kalman gap estimator, dual-loop cascaded PID controller, finite-state machine (FSM), optional machine learning lead-speed predictor, and an interactive WebGL / Three.js 3D visual engineering simulator for **Project 4: Adaptive Cruise Control (ACC) Prototype**.

The vehicle:
1. Maintains a driver-set speed when the road ahead is clear ($0.50\text{ m/s}$).
2. Detects a lead vehicle using ultrasonic ranging (HC-SR04).
3. Regulates speed to hold a safe configurable gap behind the lead vehicle ($50\text{ cm} \pm 8\text{ cm}$).
4. Slows down smoothly when the lead vehicle decelerates.
5. Stops safely ($\ge 20\text{ cm}$ behind lead) when the lead vehicle stops.
6. Automatically resumes driving when the lead vehicle moves again.
7. Enforces a priority safety override layer ($D_{\text{min}}$ dynamic limit and sensor fault shutdown in $\le 300\text{ ms}$).

---

## 2. System Architecture

```
Lead Vehicle Position
       │
       ▼
HC-SR04 Distance Ranging (20 Hz)
       │
       ▼
Median Filter (Window of 5)
       │
       ▼
1-D Kalman Filter (State: gap, v_lead) ──▶ [Optional AI Lead Speed Predictor]
       │                                            │
       ▼                                            ▼
Outer GAP Controller / FSM (50 Hz) ◄────────────────┘
       │
       ▼
Target Speed Command (m/s)
       │
       ▼
Inner SPEED PID + Feed-Forward (100 Hz)
       │
       ▼
PWM Motor Command (0 - 1023)
       │
       ▼
Vehicle Dynamics (1st Order Response, tau = 0.3s)
       │
       ▼
Wheel Speed Feedback (Encoder, 100 Hz) ──▶ Inner Speed Loop
```

---

## 3. Official Acceptance Tests & Performance Summary

| Requirement / Test | Official Limit | Simulated Result | Status |
| :--- | :--- | :--- | :--- |
| **Test A: Speed Hold (R1)** | $\le \pm 5.0\%$ of set speed | **0.92%** | **SIMULATION PASS** |
| **Test B: Gap Hold (R2)** | $50\text{ cm} \pm 8\text{ cm}$ ($0.08\text{ m}$) | **3.6 cm error** | **SIMULATION PASS** |
| **Test C: Stop & Go (R3)** | Stop gap $\ge 0.20\text{ m}$ & resume | **35.0 cm stop gap** | **SIMULATION PASS** |
| **Test D: Mode Switch Jump (R4)** | $\le 10.0\%$ jump in target speed | **7.0% jump** | **SIMULATION PASS** |
| **Test E: 10 Cut-in Trials (R5)** | Never $< 0.20\text{ m}$ in 10 trials | **31.2 cm min gap** | **SIMULATION PASS** |
| **Test F: Sensor Fault (R7)** | Shutdown motor command $\le 300\text{ ms}$ | **150 ms response** | **SIMULATION PASS** |
| **Internal Dmin Check** | $\ge 0.25\text{ m}$ | **31.2 cm min gap** | **SIMULATION PASS** |
| **50-Trial Monte Carlo** | 0 Collisions | **0 Collisions (Min Gap 27.5 cm)** | **SIMULATION PASS** |

---

## 4. WebGL / Three.js Interactive Visual Simulator

The browser-based visual engineering simulator is built using Vite, React, Three.js, Recharts, and Lucide React.

### Key Features:
- **Hero 3D Viewport:** Asphalt track grid with lane markings, chase follow camera, ACC vehicle (blue) with rotating wheels & front transducer pod, lead vehicle (red) with reactive red brake lights, $50\text{ cm}$ target ring, and $20-25\text{ cm}$ safety zone.
- **Prominent Simulation Clock:** Displays real-time `SIM TIME: 00.00 s`.
- **Dynamic Real-time Explanation Panel:** Explains real-time vehicle decisions in plain English directly from controller state variables.
- **Scenario Event Timeline:** Displays chronological state transitions (`SPEED` $\rightarrow$ `GAP` $\rightarrow$ `STOP`).
- **Beginner vs. Engineering Telemetry Modes:** Switch between clean essential cards and raw signal / Kalman / PID internal variables.
- **Scenario Selector:**
  - `Stop & Go` (Default)
  - `Constant Speed`
  - `Speed Hold` (Clear road)
  - `Gap Hold`
  - `Sudden Stop`
  - `Cut-In`
  - `Sensor Failure` (`INJECT SENSOR FAILURE`)

---

## 5. Directory Structure

```
├── code/
│   ├── esp32/
│   │   └── p04_acc/              # ESP32 C++ firmware (config.h, acc_controller.h, p04_acc.ino, etc.)
│   └── raspberry_pi/
│       ├── main.py                # Raspberry Pi host telemetry script
│       ├── config.py              # System parameters
│       ├── acc_metrics.py         # Performance evaluation suite
│       ├── lead_predictor.py      # AI lead-speed predictor
│       ├── sim/
│       │   ├── acc_sim.py         # Reference Python ACC simulator
│       │   ├── lead_profiles.py   # Driving scenarios
│       │   └── ai_benchmark.py    # AI ON vs OFF 70-run benchmark
│       └── tools/                 # Plotting & analysis tools
├── simulator/                     # WebGL / Three.js visual engineering simulator app
│   ├── index.html
│   ├── package.json
│   ├── test_engine.js             # Node.js numerical engine verification script
│   └── src/
│       ├── App.jsx
│       ├── index.css
│       ├── sim/                   # Exact 1-to-1 JavaScript port of acc_sim.py
│       ├── visualization/         # Three.js 3D Viewport
│       └── components/            # HUDs, Controls, Dashboards
├── .gitignore
└── README.md
```

---

## 6. Getting Started & How to Run

### A. Run 3D Visual Simulator (Browser UI)
```powershell
cd simulator
npm install
npm run dev
```
Open browser at **`http://localhost:5173/`**.

### B. Run Node.js Engine Verification Script
```powershell
cd simulator
node test_engine.js
```

### C. Run Python Reference Simulator & AI Benchmark
```powershell
cd code/raspberry_pi/sim
python acc_sim.py --all
python acc_sim.py --trials 50
python ai_benchmark.py
```

---

## 7. Academic Integrity & Hardware Disclaimer

- **Simulation Results Only:** All results reported in this repository are software simulation results.
- **Physical Validation Status:** `PHYSICAL VALIDATION: PENDING ESP32 + BLDC HARDWARE INTEGRATION`.
- **Prototyping Platform Note:** The current physical 4WD chassis (Brushed DC motors + TB6612 driver) is used exclusively as an initial prototyping/learning platform and does NOT represent final Project 4 BLDC hardware validation.
