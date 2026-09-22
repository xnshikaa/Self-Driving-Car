# ESP32 Adaptive Cruise Control Prototype

This project is a beginner-friendly starting point for a small robotic-car Adaptive Cruise Control prototype.

The idea is:

```text
HC-SR04 measures distance
        ↓
ESP32 decides whether to move, slow down, or stop
        ↓
TB6612FNG drivers control the motors
        ↓
OLED and Serial Monitor show the status
```

The project is being built in stages. It does not require every planned component yet.

## Hardware currently confirmed

- 38-pin ESP32 NodeMCU development board
- GoldenMorning 0.96-inch 4-pin I2C OLED
- HC-SR04 ultrasonic sensor
- Two TB6612FNG motor-driver modules
- Four DC gear motors and chassis from the robotic-car kit
- Two encoder discs, but no encoder sensors yet

The encoder discs cannot measure speed by themselves. Optical or Hall sensors will be needed later.

## What the current software does

The current firmware can:

1. Read distance from the HC-SR04.
2. Reject invalid or timed-out readings.
3. Smooth readings with a small median filter.
4. Read a start/stop switch.
5. Choose OFF, CRUISE, FOLLOW, STOP, or FAULT modes.
6. Drive all four motors through the two TB6612FNG boards.
7. Increase and decrease motor PWM gradually.
8. Show status on the OLED.
9. Print debugging information through USB Serial.

This is currently open-loop motor control. The ESP32 commands PWM, but it cannot measure actual vehicle speed because encoder sensors have not been purchased.

## Project folders

```text
esp32/
├── platformio.ini
├── acc_prototype/
│   ├── acc_prototype.ino       Main program
│   ├── config.h                 Pins and settings
│   ├── distance_sensor.*        HC-SR04 code
│   ├── motor_driver.*           Four-motor TB6612FNG code
│   ├── input_switch.*           Start/stop switch code
│   ├── acc_controller.*         ACC decision logic
│   └── oled_ui.*                OLED display code
└── tests/
    ├── 01_basic_serial          Test ESP32 programming
    ├── 02_hcsr04                Test distance sensor
    ├── 03_tb6612_motor          Test one motor first, then all four
    ├── 04_oled_switch           Test OLED and switch
    └── 05_acc_logic_dry_run     Test ACC decisions without motors
```

## Pin map used by the current code

Open `esp32/acc_prototype/config.h` if you need to change anything.

| Function | ESP32 GPIO |
|---|---:|
| HC-SR04 TRIG | 18 |
| HC-SR04 ECHO | 34 |
| OLED SDA | 21 |
| OLED SCL | 22 |
| Run switch | 4 |
| Both TB6612 STBY pins | 23 |
| Motor 1 | PWM 25, IN1 26, IN2 27 |
| Motor 2 | PWM 14, IN1 32, IN2 33 |
| Motor 3 | PWM 13, IN1 16, IN2 17 |
| Motor 4 | PWM 19, IN1 5, IN2 12 |

GPIO34 is input-only, which is suitable for HC-SR04 ECHO. ECHO must still be reduced to 3.3 V with a resistor divider.

GPIO5 and GPIO12 are boot-strapping pins on classic ESP32 boards. If the board does not boot reliably, those two motor-control pins must be moved to other suitable GPIOs.

## Connecting the two TB6612FNG boards

Use one motor per channel:

```text
TB6612 #1 channel A → Motor 1
TB6612 #1 channel B → Motor 2
TB6612 #2 channel A → Motor 3
TB6612 #2 channel B → Motor 4
```

Connect both `STBY` pins to GPIO23. Connect the logic VCC pins to the correct logic supply, motor VM pins to the motor battery, and all grounds together.

Do not power the motors from the ESP32. Check motor stall current before running all four motors. If the motors demand more current than a TB6612FNG can provide, the driver may overheat or shut down.

## Connecting the OLED

```text
OLED VCC → suitable supply for the module
OLED GND → ESP32 GND
OLED SDA → GPIO21
OLED SCL → GPIO22
```

The code assumes an SSD1306 128×64 I2C display at address `0x3C`. If the display is not detected, try `0x3D` in `config.h`.

## Software libraries

Install in Arduino IDE:

- ESP32 board support
- Adafruit GFX Library
- Adafruit SSD1306 Library

`Wire` is included with the ESP32 Arduino platform.

## Arduino IDE upload

1. Install the ESP32 board package.
2. Install the two Adafruit libraries.
3. Open `esp32/acc_prototype/acc_prototype.ino`.
4. Select an ESP32 DevKit-compatible board.
5. Select the correct COM port.
6. Upload with motor power switched off.
7. Open Serial Monitor at `115200` baud.

## Testing order

Run the sketches in `esp32/tests/` one at a time:

1. Basic Serial test.
2. HC-SR04 test.
3. TB6612FNG test with wheels raised.
4. OLED and switch test.
5. ACC logic dry run.
6. Full firmware with wheels raised.
7. Low-speed floor test.

Do not begin with the full car running on the floor. Verify every module separately first.

## Future additions

- Potentiometer for adjustable cruise speed
- Buttons for changing target gap
- Encoder sensors for real speed feedback
- Speed PID control
- Better filtering or a Kalman filter
- Battery-voltage monitoring
- Hardware emergency-stop circuit
- PC logging and simulation comparison

The current code is intentionally simple so every upgrade can be understood and tested separately.
