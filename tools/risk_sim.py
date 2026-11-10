#!/usr/bin/env python3
"""
python version of firmware/lib/swa_core/risk.cpp, to play with the weights
without flashing anything. keep the numbers in sync with RiskConfig in risk.h.

    python tools/risk_sim.py            # prints a table
    python tools/risk_sim.py --plot     # also saves docs/images/risk_demo.png
"""
import sys

W_HR, W_SPO2, TREND_GAIN = 0.3, 0.7, 1.5
WARN_AT, CRIT_AT, SPO2_FLOOR = 5.0, 7.5, 90.0
CONFIRM_UP, CONFIRM_DOWN = 2, 4


class Engine:
    def __init__(self):
        self.hr_base = 0.0
        self.sp_base = 0.0
        self.n = 0
        self.hist = []          # (t, spo2)
        self.level = 0          # 0 normal, 1 warning, 2 critical
        self.up = self.down = 0
        self.score = 0.0

    def add(self, t, hr, sp):
        self.hist.append((t, sp))
        self.hist = self.hist[-96:]
        if self.n < 30:
            self.n += 1
            self.hr_base += (hr - self.hr_base) / self.n
            self.sp_base += (sp - self.sp_base) / self.n
        elif self.level == 0 and sp >= self.sp_base - 2:
            a = 1 / 300
            self.hr_base += a * (hr - self.hr_base)
            self.sp_base += a * (sp - self.sp_base)
        self.hr, self.sp = hr, sp

    def trend(self, t):
        if len(self.hist) < 8:
            return 0.0
        new = sum(s for _, s in self.hist[-5:]) / 5
        old = [s for tt, s in self.hist if 45 <= t - tt <= 65]
        if len(old) < 3:
            return 0.0
        return max(0.0, sum(old) / len(old) - new)

    def evaluate(self, t):
        hr_dev = drop = trend = 0.0
        if self.n >= 30:
            hr_dev = abs(self.hr - self.hr_base)
            drop = max(0.0, self.sp_base - self.sp)
            trend = self.trend(t)
        self.score = W_HR * hr_dev + W_SPO2 * (drop + TREND_GAIN * trend)
        want = 0
        if self.score >= CRIT_AT or self.sp < SPO2_FLOOR:
            want = 2
        elif self.score >= WARN_AT or self.hr < 40 or self.hr > 140:
            want = 1
        if want > self.level:
            self.down = 0
            self.up += 1
            if self.up >= CONFIRM_UP:
                self.level, self.up = want, 0
        elif want < self.level:
            self.up = 0
            self.down += 1
            if self.down >= CONFIRM_DOWN:
                self.level, self.down = want, 0
        else:
            self.up = self.down = 0
        return self.level


def scenario():
    """90 s stable, then spo2 slides from 97 to 86 over 40 s, hr climbs, then recovery"""
    rows = []
    for t in range(0, 200):
        if t < 90:
            sp, hr = 97, 75 + (t % 3)
        elif t < 150:
            i = t - 90
            sp = max(86, 97 - (i * 11) // 40)
            hr = 76 + i // 2
        else:
            sp, hr = 97, 80
        rows.append((t, hr, sp))
    return rows


def run():
    e = Engine()
    out = []
    for t, hr, sp in scenario():
        e.add(t, hr, sp)
        lv = e.evaluate(t)
        out.append((t, hr, sp, e.score, lv))
    return out


if __name__ == "__main__":
    data = run()
    names = ["normal", "WARNING", "CRITICAL"]
    last = None
    for t, hr, sp, sc, lv in data:
        if lv != last:
            print("t=%3ds  hr=%3d  spo2=%2d  score=%5.1f  -> %s" % (t, hr, sp, sc, names[lv]))
            last = lv

    if "--plot" in sys.argv:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt

        t = [d[0] for d in data]
        fig, ax = plt.subplots(2, 1, figsize=(8, 5), sharex=True)
        ax[0].plot(t, [d[2] for d in data], label="SpO2 (%)")
        ax[0].plot(t, [d[1] for d in data], label="HR (bpm)")
        ax[0].legend(loc="center right")
        ax[0].set_ylabel("vitals")
        ax[1].plot(t, [d[3] for d in data], color="k")
        ax[1].axhline(WARN_AT, color="orange", ls="--")
        ax[1].axhline(CRIT_AT, color="red", ls="--")
        ax[1].set_ylabel("risk score")
        ax[1].set_xlabel("seconds (simulated, not patient data)")
        plt.tight_layout()
        plt.savefig("docs/images/risk_demo.png", dpi=130)
