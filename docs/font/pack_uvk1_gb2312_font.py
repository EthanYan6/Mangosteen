#!/usr/bin/env python3
"""Pack UV-K1 FontTool GB2312 lattices into one SPI Flash image.

Layout (UV-K1_FontTool / https://gitee.com/oldlicn/uv-k1_font-tool):
  0x0A0000  16x16 (楷体)  8178 glyphs x 32 bytes = 261696
  0x0DFE40  pad zeros                     448 bytes
  0x0E0000  8x8           8178 glyphs x 8  bytes = 65424

Output is written starting at flash base 0x0A0000 (bin offset 0).
"""

from __future__ import annotations

import argparse
import pathlib
import sys

GLYPHS = 87 * 94  # GB2312 zones 0xA1..0xF7
FONT16_SIZE = GLYPHS * 32
FONT8_SIZE = GLYPHS * 8
GAP_SIZE = 0xE0000 - (0xA0000 + FONT16_SIZE)  # 448
EXPECTED_BIN = FONT16_SIZE + GAP_SIZE + FONT8_SIZE


def strip_trailing_crlf(data: bytes) -> bytes:
    if len(data) >= 2 and data[-2:] == b"\r\n":
        return data[:-2]
    return data


def load_fon(path: pathlib.Path, expected: int) -> bytes:
    raw = strip_trailing_crlf(path.read_bytes())
    if len(raw) != expected:
        raise SystemExit(f"{path}: got {len(raw)} bytes, expected {expected}")
    return raw


def pack(font16: bytes, font8: bytes) -> bytes:
    return font16 + (b"\x00" * GAP_SIZE) + font8


def main() -> int:
    here = pathlib.Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--font16",
        type=pathlib.Path,
        help="16x16 / 楷体 .FON (default: sibling kaiti_16x16.FON)",
    )
    ap.add_argument(
        "--font8",
        type=pathlib.Path,
        default=here / "8x8.FON",
        help="8x8 .FON",
    )
    ap.add_argument(
        "-o",
        "--output",
        type=pathlib.Path,
        default=here / "uvk1_gb2312_font.bin",
    )
    args = ap.parse_args()

    font16_path = args.font16
    if font16_path is None:
        for name in ("kaiti_16x16.FON", "kangti_16x16.FON", "楷体.FON", "康体.FON", "16x16.FON"):
            cand = here / name
            if cand.is_file():
                font16_path = cand
                break
        if font16_path is None:
            raise SystemExit("No 16x16/楷体 .FON found; pass --font16")

    font16 = load_fon(font16_path, FONT16_SIZE)
    font8 = load_fon(args.font8, FONT8_SIZE)
    blob = pack(font16, font8)
    if len(blob) != EXPECTED_BIN:
        raise SystemExit(f"internal size mismatch: {len(blob)} != {EXPECTED_BIN}")
    args.output.write_bytes(blob)
    print(
        f"Wrote {args.output} ({len(blob)} bytes); "
        f"flash 0xA0000..0x{(0xA0000 + len(blob) - 1):X}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
