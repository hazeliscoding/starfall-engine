#!/usr/bin/env python3
"""
tools/gen_placeholder_audio.py

Emits two intentionally minimal placeholder audio files to
games/my_rpg/assets/audio/:

  - embercoast_morning.wav  (~30s, 22 kHz mono 16-bit, 440 Hz sine
                             drone with a slow amplitude envelope)
  - footstep.wav            (~100 ms band-limited click)

Pure stdlib (wave, struct, math). No external dependencies. Run from
the repo root:

    python tools/gen_placeholder_audio.py

The placeholders are *deliberately* unmusical — a sine drone, not a
composition. Their job is to prove the audio system works end-to-end,
not to be pleasant. When a real composer is onboarded, swap the WAV
for an OGG and update the path in games/my_rpg/src/main.cpp.

Per GameDesign.md §5.2:
  Music: acoustic-leaning chiptune. Loops should be long enough (~90s)
  to not exhaust the room. Role of silence: large.
"""

import math
import struct
import wave
from pathlib import Path


SAMPLE_RATE = 22050  # 22 kHz mono — half the file size of CD-quality, plenty for placeholder
NUM_CHANNELS = 1
BYTES_PER_SAMPLE = 2  # 16-bit PCM


def write_wav(path: Path, samples: list[float]) -> None:
    """Write a mono 16-bit PCM WAV. samples are floats in [-1.0, 1.0]."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(NUM_CHANNELS)
        w.setsampwidth(BYTES_PER_SAMPLE)
        w.setframerate(SAMPLE_RATE)
        # Convert float -> int16 and pack as little-endian.
        data = bytearray()
        for s in samples:
            clipped = max(-1.0, min(1.0, s))
            i = int(clipped * 32767.0)
            data.extend(struct.pack("<h", i))
        w.writeframes(bytes(data))


def gen_embercoast_morning() -> list[float]:
    """
    ~30 seconds of a 440 Hz sine drone with a slow amplitude envelope
    breathing between 0.15 and 0.45 amplitude. Quiet enough not to be
    aggressive, present enough that you can tell music is playing.
    """
    duration_s = 30.0
    n = int(SAMPLE_RATE * duration_s)
    freq = 440.0  # A4 — neutral, neither warm nor harsh
    envelope_period_s = 6.0  # one full breath in 6 seconds
    base_amp = 0.30
    amp_swing = 0.15  # so amplitude moves between 0.15 and 0.45

    samples = []
    two_pi = 2.0 * math.pi
    for i in range(n):
        t = i / SAMPLE_RATE
        # Slow amplitude breathing: sine of envelope_period_s rescaled
        # to [base_amp - amp_swing, base_amp + amp_swing].
        env = base_amp + amp_swing * math.sin(two_pi * t / envelope_period_s)
        # The tone itself.
        samples.append(env * math.sin(two_pi * freq * t))
    return samples


def gen_footstep() -> list[float]:
    """
    A ~100 ms band-limited click. A short burst with a rapid attack
    and exponential decay, low-passed by mixing two close frequencies
    so it doesn't sound like a digital pop.
    """
    duration_s = 0.10
    n = int(SAMPLE_RATE * duration_s)
    # Two close low frequencies for body, with a fast decay.
    f1, f2 = 180.0, 240.0
    decay_tau = 0.025  # 25 ms time constant — short, percussive

    samples = []
    two_pi = 2.0 * math.pi
    for i in range(n):
        t = i / SAMPLE_RATE
        env = math.exp(-t / decay_tau)
        body = 0.5 * math.sin(two_pi * f1 * t) + 0.3 * math.sin(two_pi * f2 * t)
        samples.append(env * body)
    return samples


def main() -> int:
    out_dir = Path(__file__).resolve().parent.parent / "games" / "my_rpg" / "assets" / "audio"

    music_path = out_dir / "embercoast_morning.wav"
    music_samples = gen_embercoast_morning()
    write_wav(music_path, music_samples)
    print(f"wrote {music_path}  ({len(music_samples)} samples, {music_path.stat().st_size} bytes)")

    sfx_path = out_dir / "footstep.wav"
    sfx_samples = gen_footstep()
    write_wav(sfx_path, sfx_samples)
    print(f"wrote {sfx_path}  ({len(sfx_samples)} samples, {sfx_path.stat().st_size} bytes)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
