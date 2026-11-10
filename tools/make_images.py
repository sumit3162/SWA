#!/usr/bin/env python3
"""redraws the pictures in docs/images/ (run from the repo root)"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Rectangle, Circle
from PIL import Image, ImageDraw, ImageFont

OUT = "docs/images"
os.makedirs(OUT, exist_ok=True)

BLUE, GREEN, AMBER, RED, GREY = "#2c7be5", "#2fb344", "#f5b800", "#e03131", "#6c7480"
LIGHT = "#eef3fb"


def canvas(w, h, figw=11):
    fig, ax = plt.subplots(figsize=(figw, figw * h / w))
    ax.set_xlim(0, w)
    ax.set_ylim(0, h)
    ax.axis("off")
    return fig, ax


def box(ax, x, y, w, h, text, fc=LIGHT, ec=BLUE, fs=9, bold=False, tc="black"):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.4,rounding_size=1.2",
                                fc=fc, ec=ec, lw=1.5))
    ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs,
            fontweight="bold" if bold else "normal", color=tc, linespacing=1.4)


def arrow(ax, x1, y1, x2, y2, label=None, color="#333", ls="-", fs=8, dy=1.6, dx=0):
    ax.annotate("", xy=(x2, y2), xytext=(x1, y1),
                arrowprops=dict(arrowstyle="-|>", color=color, lw=1.6, ls=ls))
    if label:
        ax.text((x1 + x2) / 2 + dx, (y1 + y2) / 2 + dy, label, ha="center", fontsize=fs, color=color)


# ---------------------------------------------------------------- architecture
def architecture():
    fig, ax = canvas(120, 62)
    ax.text(2, 59, "Smart Ward Assistant - system overview", fontsize=14, fontweight="bold")

    ys = [40, 26, 8]
    names = ["Bed 1 node", "Bed 2 node", "Bed N node (up to 10)"]
    for y, n in zip(ys, names):
        box(ax, 3, y, 24, 10, n + "\nESP32 + MAX30102\nLi-Po battery", fs=8.5)
    ax.text(15, 21.5, ". . .", ha="center", fontsize=14)

    box(ax, 50, 18, 30, 26, "HUB\nESP32-S3-BOX-3\n\nrisk score every 1.5 s\nbaseline per patient\nno internet needed",
        fc="#e4f4ea", ec=GREEN, fs=9.5, bold=False)
    for y in ys:
        arrow(ax, 27.6, y + 5, 49.6, 31, color=BLUE)
    ax.text(38.5, 52, "ESP-NOW, 2.4 GHz\nno router, 9-byte packet / s", ha="center", fontsize=8.5, color=BLUE)

    box(ax, 95, 42, 22, 9, "Voice alert\n\"Bed 7 critical\"", fc="#fdeaea", ec=RED, fs=9)
    box(ax, 95, 29, 22, 9, "Screen at nurse desk\nward overview", fc="#fff7dd", ec=AMBER, fs=9)
    box(ax, 95, 14, 22, 11, "Wi-Fi AP + WebSocket\nbrowser dashboard\n(doctor laptop / phone)", fc=LIGHT, ec=BLUE, fs=8.5)
    arrow(ax, 80.4, 38, 94.6, 46.5, color=RED)
    arrow(ax, 80.4, 31, 94.6, 33.5, color=AMBER)
    arrow(ax, 80.4, 24, 94.6, 19.5, color=BLUE)

    ax.text(60, 9, "MQTT / cloud push: not implemented yet", ha="center", fontsize=8, color=GREY, style="italic")
    fig.savefig(f"{OUT}/architecture.png", dpi=150, bbox_inches="tight", facecolor="white")
    plt.close(fig)


# ---------------------------------------------------------------- data flow
def dataflow():
    fig, ax = canvas(130, 50)
    ax.text(2, 47, "What happens to one heartbeat", fontsize=13, fontweight="bold")

    ax.add_patch(Rectangle((1, 25), 128, 18, fc="#f6f9ff", ec="none"))
    ax.add_patch(Rectangle((1, 3), 128, 18, fc="#f2faf4", ec="none"))
    ax.text(3, 40, "NODE", fontsize=9, color=BLUE, fontweight="bold")
    ax.text(3, 18, "HUB", fontsize=9, color=GREEN, fontweight="bold")

    n = [("MAX30102\nred + IR\n100 Hz", 6), ("high-pass 0.5 Hz\nlow-pass 5 Hz", 30),
         ("peaks -> HR\nAC/DC ratio\n-> SpO2", 54), ("9-byte packet\nonce a second", 78)]
    for t, x in n:
        box(ax, x, 28, 20, 10, t, fs=8)
    for i in range(3):
        arrow(ax, n[i][1] + 20.5, 33, n[i + 1][1] - 0.5, 33)
    arrow(ax, 98.5, 33, 104, 33)
    box(ax, 104, 28, 22, 10, "ESP-NOW\n2.4 GHz", fs=8.5, fc="white")
    arrow(ax, 115, 27.6, 115, 21.2, color=GREY)

    # hub lane runs right to left, the packet comes in from above on the right
    h = [("OK / WARN / CRIT\nvoice + screen\n+ web page", 6), ("debounce\n2 in a row to raise\n4 to clear", 30),
         ("score =\n0.3*HRdev +\n0.7*(SpO2 drop + trend)", 54), ("baseline\n(~5 min, per bed)", 78),
         ("packet in\ncrc checked", 104)]
    for i, (t, x) in enumerate(h):
        last = (i == 0)
        box(ax, x, 6, 20 if x != 104 else 22, 10, t, fs=8,
            ec=RED if last else GREEN, fc="#fdeaea" if last else "white")
    for i in range(1, len(h)):
        arrow(ax, h[i][1] - 0.5, 11, h[i - 1][1] + 20.5, 11)
    ax.text(65, 0.2, "also: SpO2 < 90 is critical no matter what the score says", ha="center", fontsize=8, color=GREY, style="italic")
    fig.savefig(f"{OUT}/dataflow.png", dpi=150, bbox_inches="tight", facecolor="white")
    plt.close(fig)


# ---------------------------------------------------------------- node wiring
def wiring():
    fig, ax = canvas(118, 66)
    ax.text(2, 63, "Bed node wiring", fontsize=14, fontweight="bold")

    # esp32
    ax.add_patch(Rectangle((40, 14), 26, 40, fc="#222", ec="black"))
    ax.text(53, 50, "ESP32 DevKit", color="white", ha="center", fontsize=10, fontweight="bold")
    pins = {"3V3": 44, "GND": 38, "GPIO21 (SDA)": 32, "GPIO22 (SCL)": 26, "GPIO19 (INT)": 20}
    for name, y in pins.items():
        ax.add_patch(Circle((40, y), 0.9, fc="gold", ec="black", zorder=5))
        ax.text(41.8, y, name, color="white", fontsize=8, va="center")
    ax.add_patch(Circle((66, 44), 0.9, fc="gold", ec="black", zorder=5))
    ax.text(64.2, 44, "GPIO34 (ADC)", color="white", fontsize=8, va="center", ha="right")

    # max30102 board
    box(ax, 4, 18, 20, 30, "MAX30102\n(MAXREFDES117)\nboard\n\nVIN\nGND\nSDA\nSCL\nINT", fs=8.5)
    for (y, c) in [(44, RED), (38, "black"), (32, BLUE), (26, GREEN), (20, AMBER)]:
        ax.plot([24.3, 40], [y, y], color=c, lw=2)
    ax.text(32, 45.2, "3.3 V", fontsize=7.5, ha="center")
    ax.text(32, 33.2, "I2C 400 kHz", fontsize=7.5, ha="center")
    ax.text(32, 21.2, "(not used yet)", fontsize=7.5, ha="center")

    # power column on the right
    box(ax, 90, 44, 24, 11, "3.3 V low-dropout\nregulator (see note)", fs=8.5, ec=RED, fc="#fdeaea")
    box(ax, 90, 26, 24, 11, "TP4056 charger\n+ protection", fs=8.5, ec=AMBER, fc="#fff7dd")
    box(ax, 90, 8, 24, 11, "Li-Po 3.7 V\n500 mAh", fs=8.5, ec=GREEN, fc="#e4f4ea")
    ax.plot([102, 102], [19.6, 25.4], color=RED, lw=2)
    ax.plot([102, 102], [37.6, 43.4], color=RED, lw=2)

    # 3.3 V rail: regulator out -> over the top of the esp32 -> down to the 3V3 wire
    ax.plot([90, 78, 78, 32, 32], [49.5, 49.5, 58, 58, 44], color=RED, lw=2, zorder=1)
    ax.add_patch(Circle((32, 44), 0.7, fc=RED, ec=RED, zorder=6))
    ax.text(55, 59.2, "3.3 V rail", fontsize=8, ha="center", color=RED)

    # battery sense divider
    box(ax, 70, 20, 16, 12, "divider\n2 x 100k\nVBAT to GND", fs=7.5, ec=GREY, fc="white")
    ax.plot([66, 78, 78], [44, 44, 32.4], color=GREY, lw=1.5)
    ax.plot([78, 78, 90], [19.6, 13.5, 13.5], color=GREY, lw=1.5)
    ax.text(79.5, 40, "battery level", fontsize=7.5, color=GREY)

    ax.text(2, 4, "Pin numbers match firmware/node/src/config.h", fontsize=8, color=GREY)
    ax.text(2, 1, "Note: AMS1117 drops ~1.1 V, so it can't hold 3.3 V from a 3.7 V cell. Use an HT7333 / AP2112K / MCP1700 type.",
            fontsize=8, color=RED)
    fig.savefig(f"{OUT}/node_wiring.png", dpi=150, bbox_inches="tight", facecolor="white")
    plt.close(fig)


# ---------------------------------------------------------------- hub screen (mock render)
def hubscreen():
    W, H = 320, 240
    img = Image.new("RGB", (W, H), "black")
    d = ImageDraw.Draw(img)
    big = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 17)
    small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 9)
    lab_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 14)
    d.text((4, 3), "Smart Ward Assistant", font=big, fill="white")
    d.text((276, 3), "7/10", font=big, fill="white")
    for x, t in [(4, "BED"), (48, "HR"), (104, "SpO2"), (164, "RISK"), (262, "STATUS")]:
        d.text((x, 26), t, font=small, fill=(130, 130, 130))

    col = {"ok": (0, 255, 0), "warn": (255, 204, 0), "crit": (255, 0, 0), "grey": (70, 70, 70)}
    lab = {"ok": "OK", "warn": "WARN", "crit": "CRIT", "grey": None}
    rows = [
        (1, 74, 97, 0.4, "ok", "OK", False),
        (2, 81, 96, 1.1, "ok", "OK", False),
        (3, 118, 86, 9.2, "crit", "CRIT", False),
        (4, 96, 93, 5.6, "warn", "WARN", True),
        (5, 0, 0, 0.0, "grey", "OFF", False),
        (6, 0, 0, 0.0, "grey", "LOST", False),
        (7, 70, 98, 0.2, "ok", "OK", False),
        (8, None,) * 7, (9, None,) * 7, (10, None,) * 7,
    ]
    for r in rows:
        y = 40 + (r[0] - 1) * 20
        d.text((4, y + 2), str(r[0]), font=big, fill=(255, 255, 255) if r[1] is not None else (70, 70, 70))
        if r[1] is None:
            d.text((48, y + 2), "--", font=big, fill=(70, 70, 70))
            continue
        _, hr, sp, sc, k, label, ack = r
        d.text((48, y + 2), str(hr) if hr else "--", font=big, fill="white")
        d.text((104, y + 2), str(sp) if sp else "--", font=big, fill="white")
        w = min(90, int(sc / 10 * 90))
        d.rectangle([164, y + 4, 255, y + 15], outline=(70, 70, 70))
        if w:
            d.rectangle([165, y + 5, 165 + w, y + 14], fill=col[k])
        d.rectangle([262, y + 1, 317, y + 18], fill=col[k])
        d.text((267, y + 2), label, font=lab_font, fill="black" if k != "grey" else "white")
        if ack:
            d.ellipse([254, y + 8, 258, y + 12], fill="white")

    S = 3
    screen = img.resize((W * S, H * S), Image.NEAREST)
    bezel = Image.new("RGB", (W * S + 60, H * S + 60), (235, 238, 243))
    bd = ImageDraw.Draw(bezel)
    bd.rounded_rectangle([6, 6, bezel.width - 7, bezel.height - 7], radius=26, fill=(38, 40, 46))
    bezel.paste(screen, (30, 30))
    bezel.save(f"{OUT}/hub_screen.png")


# ---------------------------------------------------------------- cost
def cost():
    fig, ax = plt.subplots(figsize=(7.5, 4.2))
    parts = [("Hub (S3-BOX-3)", 4500, GREEN), ("2 bed nodes", 2400, BLUE), ("enclosures etc.", 800, AMBER)]
    bottom = 0
    for name, v, c in parts:
        ax.bar(0, v, bottom=bottom, color=c, width=0.55, label=f"{name}  Rs {v:,}")
        bottom += v
    ax.bar(1, 150000, color=GREY, width=0.55, label="one ICU monitor  Rs 1,50,000")
    ax.text(0, bottom + 3000, "Rs 7,700\n(hub + 2 beds)", ha="center", fontsize=9)
    ax.text(1, 150000 + 3000, "Rs 1,50,000\n(1 bed)", ha="center", fontsize=9)
    ax.set_xticks([])
    ax.set_ylabel("rupees")
    ax.set_ylim(0, 180000)
    ax.legend(loc="upper left", fontsize=8, frameon=False)
    ax.set_title("Cost, figures from the paper", fontsize=11)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    fig.tight_layout()
    fig.savefig(f"{OUT}/cost.png", dpi=150, facecolor="white")
    plt.close(fig)


if __name__ == "__main__":
    architecture()
    dataflow()
    wiring()
    hubscreen()
    cost()
    print("images written to", OUT)
