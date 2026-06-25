#!/usr/bin/env python3
"""
tools/gen_placeholder_tileset.py

Emits a small 16x80 (1 col x 5 rows) procedural tileset PNG to
games/my_rpg/assets/tiles/placeholder_tileset.png.

Pure stdlib (zlib + struct). No external deps. Run from the repo root:

    python tools/gen_placeholder_tileset.py

Layout (tile IDs are 1-indexed; row 0 is transparent at index 0 = empty):

    Row 0 (tileId 1):  grass   — green
    Row 1 (tileId 2):  path    — tan
    Row 2 (tileId 3):  stone   — gray   (SOLID)
    Row 3 (tileId 4):  wall    — dark   (SOLID)
    Row 4 (tileId 5):  water   — blue   (SOLID)

The TileSet's atlas math is `columns=1`, so tileId N -> row (N-1).

The TimeFantasy 'outside.png' pack is available locally (gitignored)
and will be swapped in later via a Game-Director art-direction pass
once specific tile IDs are picked. The engine code (TileSet, Tilemap,
SweepMove, collision) is tileset-agnostic — only the tile IDs in the
map data + the MarkSolid calls change.
"""

import struct
import sys
import zlib
from pathlib import Path

TILE_SIZE = 16
TILE_COUNT = 5
WIDTH = TILE_SIZE
HEIGHT = TILE_SIZE * TILE_COUNT

# Subtle JRPG-leaning palette (RGB tuples).
PALETTE = [
    ( 96, 152, 96),   # grass  — muted green
    (200, 168, 120),  # path   — warm tan
    (128, 128, 128),  # stone  — neutral gray
    ( 64,  56,  56),  # wall   — dark stone
    ( 64,  96, 160),  # water  — desaturated dusk blue (matches GameDesign §5.1)
]


def encode_png(pixels, width, height):
    """Encode a 2D list of (r,g,b,a) tuples into a minimal RGBA PNG."""
    sig = b"\x89PNG\r\n\x1a\n"

    def chunk(chunk_type, data):
        length = struct.pack(">I", len(data))
        crc = struct.pack(">I", zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
        return length + chunk_type + data + crc

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)

    raw = bytearray()
    for row in pixels:
        raw.append(0)
        for px in row:
            raw.extend(px)
    idat = zlib.compress(bytes(raw), 9)

    return sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b"")


def slight_noise(base, x, y, seed):
    """A tiny pseudo-noise so tiles read as 'pixel art' not 'flat fill'."""
    h = (x * 73856093) ^ (y * 19349663) ^ (seed * 83492791)
    h = (h ^ (h >> 13)) & 0xFF
    delta = (h % 9) - 4  # -4..+4
    return max(0, min(255, base + delta))


def gen_tile(palette_idx, tile_y_offset):
    """Generate a TILE_SIZE x TILE_SIZE square with subtle per-pixel noise."""
    r, g, b = PALETTE[palette_idx]
    seed = palette_idx + 1
    rows = []
    for y in range(TILE_SIZE):
        row = []
        for x in range(TILE_SIZE):
            row.append((
                slight_noise(r, x, y + tile_y_offset, seed),
                slight_noise(g, x, y + tile_y_offset, seed),
                slight_noise(b, x, y + tile_y_offset, seed),
                255,
            ))
        rows.append(row)
    return rows


def main():
    pixels = []
    for i in range(TILE_COUNT):
        tile = gen_tile(i, i * TILE_SIZE)
        pixels.extend(tile)

    out_path = (
        Path(__file__).resolve().parent.parent
        / "games" / "my_rpg" / "assets" / "tiles" / "placeholder_tileset.png"
    )
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(encode_png(pixels, WIDTH, HEIGHT))
    size = out_path.stat().st_size
    print(f"wrote {out_path}  ({WIDTH}x{HEIGHT}, {size} bytes, {TILE_COUNT} tiles)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
