"""
uart_echo_test.py - checks the 3 UART wires between the Raspberry Pi and the ESP32.
Upload esp32/tests/t7_uart_echo to the ESP32 first, then on the Pi:
    python3 tools/uart_echo_test.py                  (uses /dev/serial0)
    python3 tools/uart_echo_test.py --port /dev/ttyUSB0
Good result: "ESP32 got: hello 1", "hello 2", ... for every message.
Nothing comes back? Check: TX goes to RX (crossed), GND is connected, serial
port enabled in raspi-config (and serial LOGIN SHELL disabled).
"""
import argparse
import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import config          # noqa: E402
def main():
    import serial
    p = argparse.ArgumentParser()
    p.add_argument("--port", default=config.SERIAL_PORT)
    p.add_argument("--count", type=int, default=10)
    args = p.parse_args()
    with serial.Serial(args.port, config.SERIAL_BAUD, timeout=0.5) as port:
        time.sleep(0.5)
        port.reset_input_buffer()
        ok = 0
        for i in range(1, args.count + 1):
            message = f"hello {i}"
            port.write((message + "\n").encode("ascii"))
            reply = port.readline().decode("ascii", errors="replace").strip()
            good = reply == f"ESP32 got: {message}"
            ok += good
            print(f"sent '{message}'  ->  got '{reply}'  {'OK' if good else 'WRONG/NOTHING'}")
            time.sleep(0.2)
    print(f"\n{ok} of {args.count} messages came back correctly.")
if __name__ == "__main__":
    main()
