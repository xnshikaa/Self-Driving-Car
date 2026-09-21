"""
tools/plot_log.py - draws a run log as a picture you can paste into your report.
It writes an SVG file (a drawing made of text), so it needs NO extra library:
it works on a fresh Raspberry Pi with nothing installed. Open the SVG in any
web browser, or drag it into Word.
    python3 tools/plot_log.py logs/run_2026-05-04_10-31-22.csv
    python3 tools/plot_log.py logs/sim/sim_stop_and_go.csv --out picture.svg
Three panels, sharing one time axis:
    1. the gap: the ultrasonic readings, the filtered gap, the set gap and the
       safety line Dmin
    2. the speeds: our speed, the speed the controller asked for, the estimated
       lead vehicle speed and (if used) the AI prediction
    3. the motor PWM, with the mode written in colour along the top
"""
import argparse
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import acc_metrics   # noqa: E402
WIDTH = 900
PANEL_HEIGHT = 190
LEFT = 60
RIGHT = 20
MODE_COLOURS = {"OFF": "#e5e7eb", "SPEED": "#dbeafe", "GAP": "#dcfce7", "STOP": "#fef3c7",
                "EMERG": "#fecaca", "FAULT": "#f5d0fe", "LOW BAT": "#fed7aa"}
class Panel:
    """One graph: turns (time, value) into (x, y) on the page."""
    def __init__(self, top, title, y_min, y_max, unit):
        self.top = top
        self.title = title
        self.y_min = y_min
        self.y_max = max(y_max, y_min + 1e-6)
        self.unit = unit
        self.parts = []
    def x(self, t, t0, t1):
        return LEFT + (WIDTH - LEFT - RIGHT) * (t - t0) / max(1e-6, t1 - t0)
    def y(self, value):
        value = max(self.y_min, min(self.y_max, value))
        return self.top + PANEL_HEIGHT - 30 - (PANEL_HEIGHT - 45) * (value - self.y_min) / (self.y_max - 
self.y_min)
    def line(self, points, colour, width=1.6, dash=None):
        if len(points) < 2:
            return
        path = " ".join(f"{'M' if i == 0 else 'L'}{px:.1f},{py:.1f}" for i, (px, py) in enumerate(points))
        style = f' stroke-dasharray="{dash}"' if dash else ""
        self.parts.append(f'<path d="{path}" fill="none" stroke="{colour}" stroke-width="{width}"{style}/>')
    def dots(self, points, colour, radius=1.3):
        for px, py in points:
            self.parts.append(f'<circle cx="{px:.1f}" cy="{py:.1f}" r="{radius}" fill="{colour}"/>')
    def frame(self, t0, t1, ticks=6):
        out = [f'<rect x="{LEFT}" y="{self.top + 12}" width="{WIDTH - LEFT - RIGHT}" '
               f'height="{PANEL_HEIGHT - 42}" fill="#ffffff" stroke="#cbd5e1"/>']
        out.append(f'<text x="{LEFT}" y="{self.top + 8}" font-size="12" font-weight="bold" '
                   f'fill="#0f172a">{self.title}</text>')
        for step in range(ticks + 1):
            value = self.y_min + (self.y_max - self.y_min) * step / ticks
            py = self.y(value)
            out.append(f'<line x1="{LEFT}" y1="{py:.1f}" x2="{WIDTH - RIGHT}" y2="{py:.1f}" '
                       f'stroke="#eef2f7"/>')
            out.append(f'<text x="{LEFT - 6}" y="{py + 3:.1f}" font-size="9" text-anchor="end" '
                       f'fill="#475569">{value:.2f}</text>')
        for step in range(7):
            t = t0 + (t1 - t0) * step / 6
            px = self.x(t, t0, t1)
            out.append(f'<line x1="{px:.1f}" y1="{self.top + 12}" x2="{px:.1f}" '
                       f'y2="{self.top + PANEL_HEIGHT - 30}" stroke="#eef2f7"/>')
            out.append(f'<text x="{px:.1f}" y="{self.top + PANEL_HEIGHT - 18}" font-size="9" '
                       f'text-anchor="middle" fill="#475569">{t:.1f}s</text>')
        out.append(f'<text x="{LEFT - 44}" y="{self.top + PANEL_HEIGHT / 2:.0f}" font-size="9" '
                   f'fill="#475569" transform="rotate(-90 {LEFT - 44},{self.top + PANEL_HEIGHT / 2:.0f})" '
                   f'text-anchor="middle">{self.unit}</text>')
        return out
def legend(x, y, items):
    out = []
    for name, colour in items:
        out.append(f'<rect x="{x}" y="{y - 7}" width="16" height="3" fill="{colour}"/>')
        out.append(f'<text x="{x + 21}" y="{y - 1}" font-size="10" fill="#334155">{name}</text>')
        x += 24 + 7 * len(name)
    return out
def draw(rows, out_path):
    t0, t1 = rows[0]["t"], rows[-1]["t"]
    gaps = [r["gap_m"] for r in rows if r["tracking"] and r["gap_m"] < 3.5]
    gap_max = max(1.0, (max(gaps) if gaps else 1.0) * 1.15)
    speed_max = max(0.4, max(max(r["speed_mps"], r["target_mps"], r["lead_mps"]) for r in rows) * 1.25)
    gap_panel = Panel(30, "Gap to the lead vehicle", 0.0, gap_max, "metres")
    speed_panel = Panel(30 + PANEL_HEIGHT, "Speeds", 0.0, speed_max, "m/s")
    pwm_panel = Panel(30 + 2 * PANEL_HEIGHT, "Motor PWM and mode", 0, 1023, "PWM")
    def points(panel, key, only_tracking=False, limit=None):
        out = []
        for row in rows:
            if only_tracking and not row["tracking"]:
                continue
            value = row.get(key, "")
            if value == "" or value != value:
                continue
            if limit is not None and value > limit:
                continue
            out.append((panel.x(row["t"], t0, t1), panel.y(float(value))))
        return out
    # panel 1: the gap
    gap_panel.dots(points(gap_panel, "raw_gap_m", limit=3.0), "#cbd5e1", 1.1)
    gap_panel.line(points(gap_panel, "gap_m", only_tracking=True, limit=3.0), "#2563eb", 1.8)
    gap_panel.line(points(gap_panel, "gap_set_m"), "#16a34a", 1.2, dash="5,4")
    dmin = [(gap_panel.x(r["t"], t0, t1), gap_panel.y(0.25)) for r in rows]
    gap_panel.line(dmin, "#dc2626", 1.2, dash="2,3")
    # panel 2: the speeds
    speed_panel.line(points(speed_panel, "target_mps"), "#f59e0b", 1.4, dash="4,3")
    speed_panel.line(points(speed_panel, "speed_mps"), "#0f172a", 1.8)
    speed_panel.line(points(speed_panel, "lead_mps", only_tracking=True), "#2563eb", 1.5)
    prediction = points(speed_panel, "prediction_mps")
    if prediction:
        speed_panel.line(prediction, "#a855f7", 1.3, dash="3,3")
    # panel 3: PWM, with coloured mode bands behind it
    bands = []
    start = rows[0]
    for previous, row in zip(rows, rows[1:]):
        if row["mode"] != previous["mode"] or row is rows[-1]:
            colour = MODE_COLOURS.get(previous["mode"], "#f1f5f9")
            x_start = pwm_panel.x(start["t"], t0, t1)
            x_end = pwm_panel.x(row["t"], t0, t1)
            bands.append(f'<rect x="{x_start:.1f}" y="{pwm_panel.top + 12}" '
                         f'width="{max(0.6, x_end - x_start):.1f}" height="{PANEL_HEIGHT - 42}" '
                         f'fill="{colour}"/>')
            if x_end - x_start > 26:
                bands.append(f'<text x="{(x_start + x_end) / 2:.1f}" y="{pwm_panel.top + 24}" '
                             f'font-size="9" text-anchor="middle" fill="#334155">{previous["mode"]}</text>')
            start = row
    pwm_panel.line(points(pwm_panel, "pwm"), "#475569", 1.4)
    height = 30 + 3 * PANEL_HEIGHT + 30
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{height}" '
           f'viewBox="0 0 {WIDTH} {height}" font-family="Segoe UI, Arial, sans-serif">',
           f'<rect width="{WIDTH}" height="{height}" fill="#f8fafc"/>']
    svg += gap_panel.frame(t0, t1)
    svg += legend(LEFT + 150, 30 + 8, [("raw", "#cbd5e1"), ("filtered gap", "#2563eb"),
                                       ("set gap", "#16a34a"), ("Dmin", "#dc2626")])
    svg += gap_panel.parts
    svg += speed_panel.frame(t0, t1)
    svg += legend(LEFT + 70, 30 + PANEL_HEIGHT + 8, [("our speed", "#0f172a"), ("target", "#f59e0b"),
                                                     ("lead", "#2563eb"), ("AI prediction", "#a855f7")])
    svg += speed_panel.parts
    svg += pwm_panel.frame(t0, t1)
    svg += bands
    svg += pwm_panel.parts
    svg.append(f'<text x="{LEFT}" y="{height - 8}" font-size="10" fill="#64748b">'
               f'{os.path.basename(out_path)} - {len(rows)} samples, {t1 - t0:.1f} s</text>')
    svg.append("</svg>")
    with open(out_path, "w", encoding="utf-8") as handle:
        handle.write("\n".join(svg))
def main():
    parser = argparse.ArgumentParser(description="Draw an ACC run log as an SVG picture.")
    parser.add_argument("log")
    parser.add_argument("--out", default=None, help="where to save (default: next to the log)")
    parser.add_argument("--no-report", action="store_true", help="only draw, do not print the numbers")
    args = parser.parse_args()
    rows = acc_metrics.read_log(args.log)
    if len(rows) < 5:
        print("That log is too short to draw.")
        return 1
    out_path = args.out or os.path.splitext(args.log)[0] + ".svg"
    draw(rows, out_path)
    if not args.no_report:
        acc_metrics.print_report(args.log, rows)
    print(f"picture saved: {out_path}")
    print("Open it in a web browser, or drag it into your report.")
    return 0
if __name__ == "__main__":
    sys.exit(main())
