#!/usr/bin/env python3
"""
makes the voice clips the hub plays, into firmware/hub/data/

needs internet (gTTS uses google) and ffmpeg on the pc:
    pip install gTTS
    python tools/make_voice_clips.py en      # or hi / mr

then upload them with:  pio run -d firmware/hub -t uploadfs

the hub works without these, it just beeps the bed number instead.
"""
import os
import subprocess
import sys

from gtts import gTTS

OUT = os.path.join(os.path.dirname(__file__), "..", "firmware", "hub", "data")

TEXT = {
    "en": {
        "bed": "Bed {n}",
        "critical": "Critical. Check oxygen immediately.",
        "warning": "Warning. Please check the patient.",
    },
    "hi": {
        "bed": "बेड नंबर {n}",
        "critical": "गंभीर स्थिति। तुरंत ऑक्सीजन जांचें।",
        "warning": "चेतावनी। कृपया मरीज को देखें।",
    },
    "mr": {
        "bed": "बेड क्रमांक {n}",
        "critical": "गंभीर स्थिती. ताबडतोब ऑक्सिजन तपासा.",
        "warning": "सूचना. कृपया रुग्णाला तपासा.",
    },
}


def make(text, lang, name):
    mp3 = os.path.join(OUT, name + ".mp3")
    wav = os.path.join(OUT, name + ".wav")
    gTTS(text=text, lang=lang).save(mp3)
    # 16 kHz mono 16 bit, that is what audio.cpp expects
    subprocess.run(
        ["ffmpeg", "-y", "-loglevel", "error", "-i", mp3,
         "-ar", "16000", "-ac", "1", "-sample_fmt", "s16", wav],
        check=True,
    )
    os.remove(mp3)
    print("wrote", wav)


def main():
    lang = sys.argv[1] if len(sys.argv) > 1 else "en"
    if lang not in TEXT:
        sys.exit("language must be one of: " + ", ".join(TEXT))
    os.makedirs(OUT, exist_ok=True)
    t = TEXT[lang]
    for n in range(1, 11):
        make(t["bed"].format(n=n), lang, "bed_%d" % n)
    make(t["critical"], lang, "critical")
    make(t["warning"], lang, "warning")


if __name__ == "__main__":
    main()
