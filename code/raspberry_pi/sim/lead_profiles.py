"""
lead_profiles.py - how the LEAD VEHICLE drives in the simulator.
These are exactly the profiles the helper sketch drives on a real second car
(esp32/helpers/lead_vehicle/lead_vehicle.ino), so a simulated run and a real
run can be compared line by line.
Each profile is a list of (duration_seconds, target_speed_mps) pieces. The
lead vehicle changes speed smoothly (it cannot jump), using ACCEL and DECEL.
"""
import random
ACCEL_MPS2 = 0.6        # how fast the lead vehicle speeds up
DECEL_MPS2 = 1.2        # how fast it slows down
PROFILES = {
    # name           pieces: (seconds, speed m/s)
    "constant":      [(2.0, 0.0), (10.0, 0.30)],
    "slow_down":     [(2.0, 0.0), (5.0, 0.42), (6.0, 0.18), (4.0, 0.18)],
    "speed_up":      [(2.0, 0.0), (5.0, 0.22), (6.0, 0.42), (3.0, 0.42)],
    "stop":          [(2.0, 0.0), (5.0, 0.40), (6.0, 0.00)],
    "stop_and_go":   [(2.0, 0.0), (4.0, 0.40), (4.0, 0.00), (5.0, 0.40), (3.0, 0.00)],
    "cut_in":        [(3.0, 0.0), (8.0, 0.35)],
    "wavy":          [(2.0, 0.0), (3.5, 0.42), (3.5, 0.20), (3.5, 0.42), (3.5, 0.20), (3.5, 0.38)],
    "none":          [(12.0, 0.0)],          # no lead vehicle at all (see START_GAP_M below)
}
# Where the lead vehicle starts, in metres in front of us
START_GAP_M = {
    "constant": 1.20, "slow_down": 1.00, "speed_up": 0.60, "stop": 1.20,
    "stop_and_go": 0.90, "cut_in": 0.45, "wavy": 0.90, "none": 9.00,
}
def profile_speed(pieces, t):
    """The speed the lead vehicle is TRYING to drive at time t."""
    start = 0.0
    for duration, speed in pieces:
        if t < start + duration:
            return speed
        start += duration
    return pieces[-1][1]
def profile_duration(pieces):
    return sum(duration for duration, _ in pieces)
class LeadVehicle:
    """The car in front. It follows a profile, but changes speed smoothly."""
    def __init__(self, pieces, start_gap_m, noise=0.0, seed=None):
        self.pieces = pieces
        self.position = start_gap_m        # metres in front of our front bumper at t = 0
        self.speed = 0.0
        self.noise = noise                 # small random wobble in its speed (more realistic)
        self.random = random.Random(seed)
    def step(self, t, dt):
        wanted = profile_speed(self.pieces, t)
        if self.noise:
            wanted = max(0.0, wanted + self.random.gauss(0.0, self.noise))
        if wanted > self.speed:
            self.speed = min(wanted, self.speed + ACCEL_MPS2 * dt)
        else:
            self.speed = max(wanted, self.speed - DECEL_MPS2 * dt)
        self.position += self.speed * dt
        return self.speed
def random_profile(seed=None):
    """A made-up profile for collecting training data: 8 random speed changes."""
    rng = random.Random(seed)
    pieces = [(1.5, 0.0)]
    speed = rng.choice([0.25, 0.30, 0.35, 0.40])
    for _ in range(8):
        duration = rng.uniform(1.2, 3.5)
        pieces.append((duration, speed))
        if rng.random() < 0.25:
            speed = 0.0                                  # a full stop
        else:
            speed = max(0.0, min(0.55, speed + rng.uniform(-0.25, 0.25)))
    return pieces
