#!/usr/bin/env python3
"""
tools/gen_iden_placeholder.py

Emits a 16x16 RGBA PNG of a deliberately-generic pixel-art figure to
games/my_rpg/assets/characters/iden_placeholder.png.

Pure stdlib (zlib for deflate, struct for chunk packing). No Pillow, no
external dependencies. Run from the repo root:

    python tools/gen_iden_placeholder.py

The script + the output are both committed: edit the SPRITE matrix below
to iterate on the placeholder, then re-run.

Iden is intentionally vague in v1 (per docs/GameDesign.md §6.7 — no
portrait, no committed gender, "you" used in dialogue). This placeholder
matches: a small figure, three muted colors, no facial features.
"""

import os
import struct
import sys
import zlib
from pathlib import Path


# Glyphs: each row is one scanline. Spaces = transparent. Otherwise the
# letter keys into PALETTE below.
SPRITE = [
    "                ",
    "                ",
    "      BBBB      ",
    "     BSSSSB     ",
    "     BSSSSB     ",
    "     BSSSSB     ",
    "      BBBB      ",
    "      BCCB      ",
    "     BCCCCB     ",
    "    BBCCCCBB    ",
    "   BCCCCCCCCB   ",
    "   BCCCCCCCCB   ",
    "   BCCCCCCCCB   ",
    "    BBCCCCBB    ",
    "     BB  BB     ",
    "                ",
]

PALETTE = {
    " ": (0x00, 0x00, 0x00, 0x00),  # transparent
    "B": (0x1A, 0x1B, 0x2E, 0xFF),  # dark navy outline
    "S": (0xC8, 0xB1, 0x96, 0xFF),  # neutral muted skin
    "C": (0x4A, 0x6E, 0x8E, 0xFF),  # desaturated dusk blue cloak
}


def encode_png(pixels, width, height):
    """Encode a 2D list of (r,g,b,a) tuples into a minimal RGBA PNG bytes."""
    # PNG signature.
    sig = b"\x89PNG\r\n\x1a\n"

    def chunk(chunk_type, data):
        length = struct.pack(">I", len(data))
        crc = struct.pack(">I", zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
        return length + chunk_type + data + crc

    # IHDR: width(4) height(4) depth(1)=8 color(1)=6(RGBA) compression(1)=0
    # filter(1)=0 interlace(1)=0
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)

    # IDAT: each scanline is prefixed with a filter byte (0 = none),
    # followed by width*4 RGBA bytes. Then deflate the whole thing.
    raw = bytearray()
    for row in pixels:
        raw.append(0)  # filter: none
        for px in row:
            raw.extend(px)
    idat = zlib.compress(bytes(raw), 9)

    return sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b"")


def main():
    width = max(len(row) for row in SPRITE)
    height = len(SPRITE)

    # Normalize ragged rows to `width` spaces.
    pixels = []
    for row in SPRITE:
        padded = row.ljust(width)
        scan = []
        for ch in padded:
            if ch not in PALETTE:
                print(f"unknown glyph {ch!r} in sprite", file=sys.stderr)
                return 1
            scan.append(PALETTE[ch])
        pixels.append(scan)

    out_path = (
        Path(__file__).resolve().parent.parent
        / "games" / "my_rpg" / "assets" / "characters"
        / "iden_placeholder.png"
    )
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(encode_png(pixels, width, height))

    size = out_path.stat().st_size
    print(f"wrote {out_path}  ({width}x{height}, {size} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
