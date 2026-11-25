"""Procedurally generate the game assets using only the standard library.

- assets/spritesheet.png: the layout must match src/Game/Presentation/SpriteAtlas.h, 7x3 tiles of 16x16 pixels.
- assets/sounds/*.wav: 8-bit style square-wave effects played by src/Game/Presentation/SoundSystem.cpp.

Generating assets from code keeps the binaries reproducible and reviewable in diffs.
"""

import math
import struct
import wave
import zlib
from pathlib import Path

TILE = 16
COLS = 7
ROWS = 3
WIDTH = COLS * TILE
HEIGHT = ROWS * TILE

ASSETS_DIR = Path(__file__).resolve().parent.parent / "assets"
OUTPUT = ASSETS_DIR / "spritesheet.png"
SOUNDS_DIR = ASSETS_DIR / "sounds"

SAMPLE_RATE = 22050
# Square waves are loud by nature; this headroom keeps several overlapping effects from clipping in the mixer
AMPLITUDE = 0.35
# A few milliseconds of fade removes the click a square wave makes when it starts or stops mid-period
FADE_SECONDS = 0.005

YELLOW = (255, 255, 0, 255)
WHITE = (255, 255, 255, 255)
FRIGHT_BLUE = (33, 33, 255, 255)
PALE = (255, 184, 174, 255)
RED = (255, 0, 0, 255)
PUPIL_BLUE = (33, 33, 222, 255)


class Canvas:
    """RGBA pixel buffer that starts fully transparent."""

    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = bytearray(width * height * 4)

    def put(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            offset = (y * self.width + x) * 4
            self.pixels[offset : offset + 4] = bytes(color)

    def fill_tile(self, col, row, inside, color):
        # Sampling at pixel centres keeps shapes symmetric around the tile centre (8, 8)
        for y in range(TILE):
            for x in range(TILE):
                if inside(x + 0.5, y + 0.5):
                    self.put(col * TILE + x, row * TILE + y, color)

    def tile_has_pixels(self, col, row):
        for y in range(row * TILE, (row + 1) * TILE):
            for x in range(col * TILE, (col + 1) * TILE):
                if self.pixels[(y * self.width + x) * 4 + 3] != 0:
                    return True
        return False

    def to_png(self):
        def chunk(tag, data):
            body = tag + data
            return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

        stride = self.width * 4
        # Filter type 0 (None) before every scanline: simplest valid encoding
        raw = b"".join(b"\x00" + bytes(self.pixels[y * stride : (y + 1) * stride]) for y in range(self.height))
        header = struct.pack(">IIBBBBB", self.width, self.height, 8, 6, 0, 0, 0)
        return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")


def pie(radius, mouth_half_deg, cx=8.0, cy=8.0):
    """Circle with a wedge cut out towards +X, i.e. the sprite faces right."""

    def inside(x, y):
        dx, dy = x - cx, y - cy
        if dx * dx + dy * dy > radius * radius:
            return False
        return abs(math.degrees(math.atan2(dy, dx))) >= mouth_half_deg

    return inside


def death_sparks(x, y):
    # Final collapse frame: short rays around an empty centre, so the frame is visible but "gone"
    dx, dy = x - 8.0, y - 8.0
    dist = math.hypot(dx, dy)
    if not 3.5 <= dist <= 6.5:
        return False
    angle = math.degrees(math.atan2(dy, dx)) % 45.0
    return angle < 8.0 or angle > 37.0


def ghost_body(phase):
    def inside(x, y):
        if y < 8.0:
            return math.hypot(x - 8.0, y - 8.0) <= 7.0
        if not 1.0 <= x <= 15.0:
            return False
        if y < 13.0:
            return True
        if y < 15.0:
            # Two skirt phases shift the zigzag by one bump, giving a walking wiggle
            return int((x - 1.0 + phase * 2.0) // 2.0) % 2 == 0
        return False

    return inside


def frightened_face(x, y):
    eye = 5.0 <= x < 7.0 or 9.0 <= x < 11.0
    if eye and 5.0 <= y < 7.0:
        return True
    # Wavy mouth: alternating pixels on two rows
    if 3.0 <= x < 13.0 and 10.0 <= y < 12.0:
        upper = int(x) % 2 == 0
        return (y < 11.0) == upper
    return False


def eye_whites(x, y):
    return any(((x - ex) / 2.0) ** 2 + ((y - 7.0) / 2.5) ** 2 <= 1.0 for ex in (5.0, 11.0))


def pupils(dx, dy):
    def inside(x, y):
        for ex in (5.0, 11.0):
            px, py = ex + dx, 7.0 + dy
            if px - 1.0 <= x < px + 1.0 and py - 1.0 <= y < py + 1.0:
                return True
        return False

    return inside


def draw_sheet():
    canvas = Canvas(WIDTH, HEIGHT)

    # Row 0: chomper move frames (closed, half, wide) and four collapse phases
    for col, mouth in enumerate((0.0, 25.0, 45.0)):
        canvas.fill_tile(col, 0, pie(7.0, mouth), YELLOW)
    for offset, mouth in enumerate((90.0, 130.0, 165.0)):
        canvas.fill_tile(3 + offset, 0, pie(7.0, mouth), YELLOW)
    canvas.fill_tile(6, 0, death_sparks, YELLOW)

    # Row 1: white ghost bodies are tinted at draw time; frightened ghosts carry their own colours
    for col in range(2):
        canvas.fill_tile(col, 1, ghost_body(col), WHITE)
    canvas.fill_tile(2, 1, ghost_body(0), FRIGHT_BLUE)
    canvas.fill_tile(2, 1, frightened_face, PALE)
    canvas.fill_tile(3, 1, ghost_body(0), WHITE)
    canvas.fill_tile(3, 1, frightened_face, RED)

    # Row 2: eyes looking Right, Down, Left, Up, then pellet, power pellet and life icon
    for col, (dx, dy) in enumerate(((1.0, 0.0), (0.0, 1.5), (-1.0, 0.0), (0.0, -1.5))):
        canvas.fill_tile(col, 2, eye_whites, WHITE)
        canvas.fill_tile(col, 2, pupils(dx, dy), PUPIL_BLUE)
    canvas.fill_tile(4, 2, lambda x, y: 7.0 <= x < 9.0 and 7.0 <= y < 9.0, PALE)
    canvas.fill_tile(5, 2, lambda x, y: math.hypot(x - 8.0, y - 8.0) <= 5.0, PALE)
    canvas.fill_tile(6, 2, pie(6.0, 40.0), YELLOW)

    return canvas


REQUIRED_TILES = [(col, 0) for col in range(7)] + [(col, 1) for col in range(4)] + [(col, 2) for col in range(7)]


def square(phase, duty):
    return 1.0 if phase % 1.0 < duty else -1.0


def glide(f_start, f_end, duration):
    """Exponential pitch glide: equal musical intervals per unit of time, so sweeps sound even to the ear."""

    def freq_at(t):
        return f_start * (f_end / f_start) ** min(t / duration, 1.0)

    return freq_at


def with_vibrato(freq_at, rate_hz, depth):
    def wobbled(t):
        return freq_at(t) * (1.0 + depth * math.sin(2.0 * math.pi * rate_hz * t))

    return wobbled


def synth(duration, freq_at, duty=0.5, gain_at=None):
    """Square wave whose pitch follows freq_at(t); returns samples in [-1, 1]."""
    count = round(duration * SAMPLE_RATE)
    fade = round(FADE_SECONDS * SAMPLE_RATE)
    samples = []
    phase = 0.0
    for index in range(count):
        t = index / SAMPLE_RATE
        envelope = min(1.0, index / fade, (count - 1 - index) / fade)
        gain = gain_at(t) if gain_at else 1.0
        samples.append(square(phase, duty) * envelope * gain)
        # Phase is integrated from the frequency, so a changing pitch never jumps mid-period (no clicks)
        phase += freq_at(t) / SAMPLE_RATE
    return samples


def chomp():
    # "Wa-ka": the pitch dips and comes back within one bite
    half = 0.05
    down = glide(520.0, 260.0, half)
    up = glide(260.0, 520.0, half)
    return synth(2 * half, lambda t: down(t) if t < half else up(t - half))


def power_pellet():
    return synth(0.25, with_vibrato(glide(200.0, 800.0, 0.25), 12.0, 0.03))


def eat_ghost():
    # A narrow pulse sounds thinner and brighter than a 50% square
    return synth(0.25, glide(900.0, 1800.0, 0.25), duty=0.25)


def death():
    duration = 0.70
    return synth(
        duration,
        with_vibrato(glide(800.0, 110.0, duration), 8.0, 0.06),
        gain_at=lambda t: 1.0 - 0.6 * (t / duration),
    )


VICTORY_NOTES = ((523.25, 0.12), (659.25, 0.12), (783.99, 0.12), (1046.50, 0.34))  # C5 E5 G5, then a held C6


def victory():
    starts = []
    elapsed = 0.0
    for freq, length in VICTORY_NOTES:
        starts.append((elapsed, freq, length))
        elapsed += length

    def note_at(t):
        for start, freq, length in reversed(starts):
            if t >= start:
                return start, freq, length
        return starts[0]

    def freq_at(t):
        return note_at(t)[1]

    def gain_at(t):
        # Each note decays a little, so the arpeggio steps are heard as separate notes
        start, _, length = note_at(t)
        return 1.0 - 0.3 * ((t - start) / length)

    return synth(elapsed, freq_at, gain_at=gain_at)


SOUNDS = {
    "chomp.wav": (chomp, 0.10),
    "power_pellet.wav": (power_pellet, 0.25),
    "eat_ghost.wav": (eat_ghost, 0.25),
    "death.wav": (death, 0.70),
    "victory.wav": (victory, 0.70),
}


def write_wav(path, samples):
    frames = struct.pack(f"<{len(samples)}h", *(round(sample * AMPLITUDE * 32767) for sample in samples))
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(frames)


def read_duration(path):
    with wave.open(str(path), "rb") as wav:
        if wav.getnchannels() != 1 or wav.getsampwidth() != 2 or wav.getframerate() != SAMPLE_RATE:
            return None
        return wav.getnframes() / SAMPLE_RATE


def write_spritesheet():
    canvas = draw_sheet()
    empty = [tile for tile in REQUIRED_TILES if not canvas.tile_has_pixels(*tile)]
    if empty:
        print(f"Empty frames (col, row): {empty}")
        return False

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(canvas.to_png())
    print(f"Wrote {OUTPUT} ({WIDTH}x{HEIGHT} RGBA, {len(REQUIRED_TILES)} frames)")
    return True


def write_sounds():
    SOUNDS_DIR.mkdir(parents=True, exist_ok=True)
    ok = True
    for name, (generate, expected) in SOUNDS.items():
        path = SOUNDS_DIR / name
        write_wav(path, generate())
        # Read the file back, so a broken header or a wrong length fails here and not in the game
        duration = read_duration(path)
        if duration is None or abs(duration - expected) > 1.0 / SAMPLE_RATE:
            print(f"Bad sound {path}: duration {duration}, expected {expected:.2f} s")
            ok = False
            continue
        print(f"Wrote {path} ({duration:.2f} s, {SAMPLE_RATE} Hz mono 16-bit)")
    return ok


def main():
    sprites_ok = write_spritesheet()
    sounds_ok = write_sounds()
    return 0 if sprites_ok and sounds_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
