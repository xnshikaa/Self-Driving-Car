"""
esp32_link.py - the Raspberry Pi side of the Pi <-> ESP32 conversation (UART).
    link = Esp32Link()                         # opens config.SERIAL_PORT
    link.wait_until_ready()                    # waits for the first status message
    link.set_dashboard(0.5, 0.5, engage=True)  # set speed, gap setting, ACC on
    link.send_prediction(0.32)                 # "the lead vehicle will move at 0.32 m/s"
    print(link.status)                         # latest Telemetry (or None)
    for event in link.pop_events(): ...        # mode changes and warnings
    link.stop(); link.close()
Every message is one line of text:   $TYPE,field,field,...*CS
CS (checksum) = XOR of all characters between $ and *, as two hex digits.
    Pi -> ESP32   $M,seq,on,speed_mm_s*CS        remote drive (test tools)
                  $G,set_mm_s,gap_mm,engage*CS   set speed, gap setting, ACC on/off
                  $P,lead_mm_s,horizon_ms*CS     the AI prediction
                  $K,kp,ki,gap_kp_x100,gap_ki_x100*CS   new gains
                  $S*CS                          stop and switch ACC off
    ESP32 -> Pi   $T,ms,mode,speed,target,gap,raw,lead,set,gapset,pwm,battery,flags*CS
                  $E,ms,event,speed_mm_s,gap_mm,value*CS
Whole numbers travel on the wire (mm and mm/s): they are short and every
computer reads them the same way.
"""
import collections
import threading
import time
from dataclasses import dataclass
import config
MODE_NAMES = {0: "OFF", 1: "SPEED", 2: "GAP", 3: "STOP", 4: "EMERG", 5: "FAULT", 6: "LOW BAT"}
def checksum(body):
    value = 0
    for byte in body.encode("ascii"):
        value ^= byte
    return value
def make_frame(body):
    return f"${body}*{checksum(body):02X}\n".encode("ascii")
def parse_frame(line):
    """Turns the text $T,1,2*CS into the list [T, 1, 2], or None if it is damaged."""
    line = line.strip()
    if not line.startswith("$") or "*" not in line:
        return None
    body, _, cs = line[1:].rpartition("*")
    try:
        if int(cs, 16) != checksum(body):
            return None
    except ValueError:
        return None
    return body.split(",")
@dataclass
class Telemetry:
    esp_ms: int             # ESP32 clock (milliseconds since it started)
    mode: str               # OFF / SPEED / GAP / STOP / EMERG / FAULT / LOW BAT
    speed_mps: float        # our own speed, from the motor pulses
    target_mps: float       # the speed the inner PID is holding right now
    gap_m: float            # Kalman-filtered gap (4.0 = no lead vehicle)
    raw_gap_m: float        # the median ultrasonic reading behind it
    lead_mps: float         # the estimated speed of the lead vehicle
    set_mps: float          # the set speed (knob or $G)
    gap_set_m: float        # the gap setting (button or $G)
    pwm: int                # what the motor is being given (0..1023)
    battery_v: float
    tracking: bool          # True = a lead vehicle is being followed
    sensor_ok: bool
    braking: bool
    used_prediction: bool   # True = the ESP32 used our AI prediction in this step
    remote: bool            # True = a Pi tool is driving
    battery_low: bool
    received: float         # time.monotonic() on the Pi when this arrived
@dataclass
class Event:
    esp_ms: int
    name: str               # SPEED, GAP, STOP, EMERG, FAULT, LOWBAT, LINKLOST, REMOTE_STOP ...
    speed_mps: float
    gap_m: float
    value: float
    received: float
    def describe(self):
        return f"{self.name}: speed {self.speed_mps:.2f} m/s, gap {self.gap_m:.2f} m, value {self.value:.2f}"
def parse_telemetry(fields, received):
    if not fields or fields[0] != "T" or len(fields) != 13:
        return None
    try:
        v = [int(x) for x in fields[1:]]
    except ValueError:
        return None
    flags = v[11]
    return Telemetry(v[0], MODE_NAMES.get(v[1], str(v[1])), v[2] / 1000.0, v[3] / 1000.0, v[4] / 1000.0,
                     v[5] / 1000.0, v[6] / 1000.0, v[7] / 1000.0, v[8] / 1000.0, v[9], v[10] / 1000.0,
                     bool(flags & 0x01), bool(flags & 0x02), bool(flags & 0x04), bool(flags & 0x08),
                     bool(flags & 0x10), bool(flags & 0x20), received)
def parse_event(fields, received):
    if not fields or fields[0] != "E" or len(fields) != 6:
        return None
    try:
        return Event(int(fields[1]), fields[2], int(fields[3]) / 1000.0, int(fields[4]) / 1000.0,
                     int(fields[5]) / 1000.0, received)
    except ValueError:
        return None
class Esp32Link:
    def __init__(self, port=None, baud=None, serial_port=None):
        if serial_port is None:
            import serial  # pySerial
            serial_port = serial.Serial()
            serial_port.port = port or config.SERIAL_PORT
            serial_port.baudrate = baud or config.SERIAL_BAUD
            serial_port.timeout = 0.05
            serial_port.dtr = False   # avoids resetting ESP32 boards connected by USB
            serial_port.rts = False
            serial_port.open()
        self._serial = serial_port
        self._write_lock = threading.Lock()
        self._seq = 0
        self.status = None          # latest Telemetry
        self.bad_frames = 0         # damaged messages (wrong checksum)
        self.last_text = ""         # last line that was not a message (ESP32 debug text)
        self._events = collections.deque(maxlen=400)
        self._status_listeners = []
        self._running = True
        self._thread = threading.Thread(target=self._read_loop, daemon=True)
        self._thread.start()
    # ---------------- sending ----------------
    def _send(self, body):
        with self._write_lock:
            self._serial.write(make_frame(body))
    def set_dashboard(self, set_speed_mps, gap_m, engage=True):
        """Replaces the knob and the buttons, so a test runs the same way every time."""
        set_speed_mps = max(0.0, min(config.MAX_SPEED_MPS, set_speed_mps))
        self._send(f"G,{int(round(set_speed_mps * 1000))},{int(round(gap_m * 1000))},{1 if engage else 0}")
    def drive(self, speed_mps):
        """Remote drive: the ACC rests and we hold this speed (tools/speed_test.py uses it)."""
        self._seq += 1
        speed_mps = max(0.0, min(config.MAX_SPEED_MPS, speed_mps))
        self._send(f"M,{self._seq},1,{int(round(speed_mps * 1000))}")
    def release(self):
        """Ends remote drive and gives the car back to the ACC."""
        self._seq += 1
        self._send(f"M,{self._seq},0,0")
    def send_prediction(self, lead_mps, horizon_s=None):
        horizon_ms = int(round((horizon_s if horizon_s else config.PREDICT_HORIZON_S) * 1000))
        lead_mps = max(-config.PREDICT_MAX_MPS, min(config.PREDICT_MAX_MPS, lead_mps))
        self._send(f"P,{int(round(lead_mps * 1000))},{horizon_ms}")
    def set_gains(self, kp, ki, gap_kp, gap_ki):
        self._send(f"K,{kp:.0f},{ki:.0f},{int(round(gap_kp * 100))},{int(round(gap_ki * 100))}")
    def stop(self):
        self._send("S")
    # ---------------- receiving ----------------
    def add_status_listener(self, function):
        """function(telemetry) is called for EVERY status message (used for logging)."""
        self._status_listeners.append(function)
    def _read_loop(self):
        buffer = b""
        while self._running:
            try:
                chunk = self._serial.read(256)
            except Exception as error:     # e.g. the USB cable was pulled out
                print("Serial read error:", error)
                time.sleep(0.5)
                continue
            if not chunk:
                continue
            buffer += chunk
            while b"\n" in buffer:
                raw, buffer = buffer.split(b"\n", 1)
                self._handle_line(raw.decode("ascii", errors="replace").strip())
            if len(buffer) > 2048:          # no newline for a long time: throw the garbage away
                buffer = b""
    def _handle_line(self, line):
        if not line:
            return
        if not line.startswith("$"):
            self.last_text = line
            return
        fields = parse_frame(line)
        now = time.monotonic()
        if fields and fields[0] == "E":
            event = parse_event(fields, now)
            if event is None:
                self.bad_frames += 1
            else:
                self._events.append(event)
            return
        telemetry = parse_telemetry(fields, now)
        if telemetry is None:
            self.bad_frames += 1
            return
        self.status = telemetry
        for function in self._status_listeners:
            function(telemetry)
    def pop_events(self):
        """All events that arrived since the last call (oldest first)."""
        events = []
        while self._events:
            events.append(self._events.popleft())
        return events
    def status_age(self):
        """Seconds since the last status message (infinity if none has arrived)."""
        return float("inf") if self.status is None else time.monotonic() - self.status.received
    def wait_until_ready(self, timeout_s=5.0):
        end = time.monotonic() + timeout_s
        while time.monotonic() < end:
            if self.status is not None:
                return True
            time.sleep(0.05)
        return False
    def wait_until_stopped(self, timeout_s=5.0):
        """Sends stop until the car really stands still (used between test runs)."""
        end = time.monotonic() + timeout_s
        while time.monotonic() < end:
            self.stop()
            time.sleep(0.1)
            status = self.status
            if status is not None and status.speed_mps < 0.05 and status.mode in ("OFF", "STOP"):
                return True
        return False
    def close(self):
        for _ in range(3):                  # say stop a few times, just to be sure
            try:
                self.stop()
            except Exception:
                pass
            time.sleep(0.03)
        self._running = False
        self._thread.join(timeout=1.0)
        self._serial.close()
