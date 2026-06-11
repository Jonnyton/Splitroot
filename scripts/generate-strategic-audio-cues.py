"""
Generate the 14 strategic-layer audio cues as real WAV files.

Spec source: FactoryContracts/strategic_audio_events.json + the audio polish plan
on the connector (splitroot-polish-audio-direction-2026-05-24).

Output: games/splitroot/Content/Audio/Strategic/*.wav as 16-bit 44.1 kHz stereo.
These WAV files are real binary game assets. When Unreal opens the project, it
auto-imports them as USoundWave assets at the corresponding paths under
/Game/Audio/Strategic/.

Cues are placeholder synth — sine-wave motifs that match the spec's
tonal description (rising / falling / pulse / sustain). Production polish
swaps these for sourced/recorded assets per the audio direction plan.
"""

from __future__ import annotations

import math
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "games" / "splitroot" / "Content" / "Audio" / "Strategic"
SAMPLE_RATE = 44100
CHANNELS = 2
SAMPLE_WIDTH_BYTES = 2  # 16-bit
PEAK = 0.65  # leave 3 dB headroom; mixing is downstream


def note_freq_hz(midi_note: int) -> float:
    return 440.0 * (2.0 ** ((midi_note - 69) / 12.0))


def envelope_ad(t: float, total: float, attack: float = 0.02, decay_tail: float = 0.18) -> float:
    """Simple attack-decay envelope normalised to [0, 1]."""
    if t < attack:
        return t / attack
    fade_start = max(0.0, total - decay_tail)
    if t > fade_start:
        return max(0.0, (total - t) / decay_tail)
    return 1.0


def render_samples(seconds: float, generator) -> bytes:
    """Generate stereo 16-bit PCM bytes from a per-sample (t) -> float in [-1, 1]."""
    n = int(seconds * SAMPLE_RATE)
    buf = bytearray()
    max_short = 32767
    for i in range(n):
        t = i / SAMPLE_RATE
        val = generator(t)
        val = max(-1.0, min(1.0, val * PEAK))
        s = int(val * max_short)
        # stereo: same on both channels (mono presentation, stereo file for Unreal)
        buf += struct.pack("<hh", s, s)
    return bytes(buf)


def gen_match_start() -> bytes:
    """Three rising tones (E4-A4-D5) + low whoosh tail. Duration 3.0s."""
    e4, a4, d5 = note_freq_hz(64), note_freq_hz(69), note_freq_hz(74)
    schedule = [(0.0, 0.7, e4), (0.7, 0.7, a4), (1.4, 1.0, d5)]

    def gen(t):
        v = 0.0
        for start, dur, freq in schedule:
            if start <= t < start + dur:
                env = envelope_ad(t - start, dur, attack=0.05, decay_tail=0.15)
                v += 0.55 * env * math.sin(2 * math.pi * freq * t)
        # low whoosh under the last note
        if t > 1.6:
            env = max(0.0, (3.0 - t) / 1.4)
            v += 0.35 * env * math.sin(2 * math.pi * 65.0 * t)
        return v

    return render_samples(3.0, gen)


def two_note_motif(seconds: float, n1: int, n2: int, attack=0.04, tail=0.20) -> bytes:
    f1, f2 = note_freq_hz(n1), note_freq_hz(n2)
    half = seconds / 2.0

    def gen(t):
        if t < half:
            env = envelope_ad(t, half, attack=attack, decay_tail=tail)
            return 0.7 * env * math.sin(2 * math.pi * f1 * t)
        else:
            env = envelope_ad(t - half, half, attack=attack, decay_tail=tail)
            return 0.7 * env * math.sin(2 * math.pi * f2 * t)

    return render_samples(seconds, gen)


def gen_side_resource_ally() -> bytes:
    """Bright two-note rise G4 -> C5. Duration 1.5s."""
    return two_note_motif(1.5, 67, 72)


def gen_side_resource_enemy() -> bytes:
    """Dim two-note fall G4 -> D4. Duration 1.5s."""
    return two_note_motif(1.5, 67, 62)


def gen_central_resource_flipped() -> bytes:
    """Heavier four-note phrase E4-G4-A4-C5 + low rumble. Duration 2.5s."""
    notes = [note_freq_hz(n) for n in (64, 67, 69, 72)]
    note_dur = 0.45

    def gen(t):
        v = 0.0
        for i, f in enumerate(notes):
            start = i * note_dur
            if start <= t < start + note_dur:
                env = envelope_ad(t - start, note_dur, attack=0.04, decay_tail=0.15)
                v += 0.55 * env * math.sin(2 * math.pi * f * t)
        # low rumble underneath the whole phrase
        rumble_env = max(0.0, min(1.0, t / 0.3)) * max(0.0, (2.5 - t) / 0.6)
        v += 0.30 * rumble_env * math.sin(2 * math.pi * 55.0 * t)
        return v

    return render_samples(2.5, gen)


def gen_hero_unlocked_ally() -> bytes:
    """Long shimmering pad sustain + bell. Duration 4.0s."""
    pad = [note_freq_hz(n) for n in (60, 64, 67, 72)]  # C major chord
    bell_freq = note_freq_hz(84)  # C6 bell

    def gen(t):
        env = envelope_ad(t, 4.0, attack=0.6, decay_tail=1.0)
        v = 0.0
        for f in pad:
            v += 0.16 * env * math.sin(2 * math.pi * f * t)
        # bell hit at t=1.5
        if 1.5 <= t < 4.0:
            local = t - 1.5
            bell_env = math.exp(-local * 1.2)
            v += 0.35 * bell_env * math.sin(2 * math.pi * bell_freq * t)
        return v

    return render_samples(4.0, gen)


def gen_hero_unlocked_enemy() -> bytes:
    """Same shimmer pitched lower + slight discord. Duration 4.0s."""
    pad = [note_freq_hz(n) for n in (55, 58, 62, 66)]  # G minor-ish with slight discord
    bell_freq = note_freq_hz(77)

    def gen(t):
        env = envelope_ad(t, 4.0, attack=0.6, decay_tail=1.0)
        v = 0.0
        for f in pad:
            v += 0.16 * env * math.sin(2 * math.pi * f * t)
        if 1.5 <= t < 4.0:
            local = t - 1.5
            bell_env = math.exp(-local * 1.2)
            v += 0.35 * bell_env * math.sin(2 * math.pi * bell_freq * t)
        return v

    return render_samples(4.0, gen)


def gen_hero_arrived_ally() -> bytes:
    """Faction-agnostic placeholder: triumphant ascending arpeggio + sustain. Duration 2.5s."""
    notes = [note_freq_hz(n) for n in (60, 64, 67, 72, 76)]
    arp_dur = 0.30

    def gen(t):
        v = 0.0
        for i, f in enumerate(notes):
            start = i * arp_dur
            if start <= t < start + arp_dur * 1.5:
                env = envelope_ad(t - start, arp_dur * 1.5, attack=0.02, decay_tail=0.15)
                v += 0.45 * env * math.sin(2 * math.pi * f * t)
        # held final note resonance
        if t > 1.5:
            sustain_env = max(0.0, (2.5 - t) / 1.0)
            v += 0.30 * sustain_env * math.sin(2 * math.pi * notes[-1] * t)
        return v

    return render_samples(2.5, gen)


def gen_hero_death_ally() -> bytes:
    """Dampened motif fall (somber, brief). Duration 2.0s."""
    notes = [note_freq_hz(n) for n in (67, 64, 60, 55)]
    note_dur = 0.5

    def gen(t):
        v = 0.0
        for i, f in enumerate(notes):
            start = i * note_dur
            if start <= t < start + note_dur:
                env = envelope_ad(t - start, note_dur, attack=0.08, decay_tail=0.30)
                v += 0.50 * env * math.sin(2 * math.pi * f * t)
        return v

    return render_samples(2.0, gen)


def gen_hero_death_enemy() -> bytes:
    """Brief neutral tonal ping. Duration 1.0s."""
    f = note_freq_hz(72)

    def gen(t):
        env = envelope_ad(t, 1.0, attack=0.01, decay_tail=0.6)
        return 0.50 * env * math.sin(2 * math.pi * f * t)

    return render_samples(1.0, gen)


def gen_base_core_under_attack() -> bytes:
    """Insistent pulsing low tone + clock-tick (urgency). Duration 4.0s, loop-friendly."""
    low_freq = note_freq_hz(36)  # C2
    tick_period = 0.5

    def gen(t):
        # Pulsing low — square-ish modulation
        pulse = 0.5 + 0.5 * math.sin(2 * math.pi * 2.0 * t)  # 2 Hz pulse
        low = 0.55 * pulse * math.sin(2 * math.pi * low_freq * t)
        # Clock tick: short transients at 0.5s intervals
        tick = 0.0
        tick_phase = t % tick_period
        if tick_phase < 0.04:
            tick_env = math.exp(-tick_phase * 80.0)
            tick = 0.45 * tick_env * math.sin(2 * math.pi * 2200.0 * t)
        return low + tick

    return render_samples(4.0, gen)


def gen_base_core_destroyed() -> bytes:
    """Massive bass thud + room-rumble sustain. Duration 6.0s."""
    thud_freq = note_freq_hz(24)  # C1

    def gen(t):
        # Initial thud: fast attack, slow decay over first 1.5s
        if t < 1.5:
            env = math.exp(-t * 1.2)
            thud = 0.95 * env * math.sin(2 * math.pi * thud_freq * t)
        else:
            thud = 0.0
        # Room rumble: low-frequency wash, builds, decays
        rumble_env = min(1.0, t / 0.4) * max(0.0, (6.0 - t) / 4.5)
        rumble = 0.45 * rumble_env * (
            math.sin(2 * math.pi * 45.0 * t) + 0.5 * math.sin(2 * math.pi * 67.0 * t)
        )
        return thud + rumble * 0.6

    return render_samples(6.0, gen)


def gen_faction_power_activated_ally() -> bytes:
    """Signature swell (placeholder for per-faction motif). Duration 2.0s."""
    notes = [note_freq_hz(n) for n in (60, 67, 72)]  # C-G-C swell

    def gen(t):
        env = envelope_ad(t, 2.0, attack=0.25, decay_tail=0.6)
        v = 0.0
        for i, f in enumerate(notes):
            # cascading entry
            entry = i * 0.15
            if t >= entry:
                v += 0.30 * env * math.sin(2 * math.pi * f * t)
        return v

    return render_samples(2.0, gen)


def gen_match_victory() -> bytes:
    """Triumphant motif over 6s — major arpeggio, sustained final chord."""
    arp = [note_freq_hz(n) for n in (60, 64, 67, 72, 76, 79)]
    note_dur = 0.35
    chord = [note_freq_hz(n) for n in (60, 64, 67, 72)]

    def gen(t):
        v = 0.0
        # Ascending arpeggio over first 2.1s
        for i, f in enumerate(arp):
            start = i * note_dur
            if start <= t < start + note_dur * 1.2:
                env = envelope_ad(t - start, note_dur * 1.2, attack=0.02, decay_tail=0.15)
                v += 0.35 * env * math.sin(2 * math.pi * f * t)
        # Held final chord 2.1s -> 6.0s
        if t > 2.1:
            chord_env = envelope_ad(t - 2.1, 3.9, attack=0.05, decay_tail=1.5)
            for f in chord:
                v += 0.16 * chord_env * math.sin(2 * math.pi * f * t)
        return v

    return render_samples(6.0, gen)


def gen_match_defeat() -> bytes:
    """Somber motif over 6s — minor descending phrase + low sustain."""
    descend = [note_freq_hz(n) for n in (67, 63, 60, 55, 51)]
    note_dur = 0.55

    def gen(t):
        v = 0.0
        for i, f in enumerate(descend):
            start = i * note_dur
            if start <= t < start + note_dur * 1.5:
                env = envelope_ad(t - start, note_dur * 1.5, attack=0.05, decay_tail=0.30)
                v += 0.35 * env * math.sin(2 * math.pi * f * t)
        # Low sustain underneath
        sustain_env = envelope_ad(t, 6.0, attack=0.5, decay_tail=2.0)
        v += 0.30 * sustain_env * math.sin(2 * math.pi * note_freq_hz(36) * t)
        return v

    return render_samples(6.0, gen)


CUES = {
    "MatchStart": gen_match_start,
    "SideResourceCapturedAlly": gen_side_resource_ally,
    "SideResourceCapturedEnemy": gen_side_resource_enemy,
    "CentralResourceFlipped": gen_central_resource_flipped,
    "HeroUnlockedAlly": gen_hero_unlocked_ally,
    "HeroUnlockedEnemy": gen_hero_unlocked_enemy,
    "HeroArrivedAlly": gen_hero_arrived_ally,
    "HeroDeathAlly": gen_hero_death_ally,
    "HeroDeathEnemy": gen_hero_death_enemy,
    "BaseCoreUnderAttack": gen_base_core_under_attack,
    "BaseCoreDestroyed": gen_base_core_destroyed,
    "FactionPowerActivatedAlly": gen_faction_power_activated_ally,
    "MatchVictory": gen_match_victory,
    "MatchDefeat": gen_match_defeat,
}


def write_wav(path: Path, pcm: bytes) -> None:
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(CHANNELS)
        wav.setsampwidth(SAMPLE_WIDTH_BYTES)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(pcm)


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    sizes = []
    for name, gen in CUES.items():
        pcm = gen()
        out_path = OUT_DIR / f"S_Strategic_{name}.wav"
        write_wav(out_path, pcm)
        sizes.append((name, out_path.name, len(pcm) + 44))
    total_bytes = sum(s for _, _, s in sizes)
    print(f"Wrote {len(sizes)} WAV files to {OUT_DIR}")
    for name, fname, size in sizes:
        print(f"  {name:30s} -> {fname:48s} {size:>10,} bytes")
    print(f"Total: {total_bytes:,} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
