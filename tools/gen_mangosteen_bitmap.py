#!/usr/bin/env python3
"""Generate 48x48 ST7565 mangosteen boot icon from a source PNG."""

from PIL import Image
import sys

SRC = (
    r"C:\Users\EthanYan\.cursor\projects\d-File-code-uvk1-mangosteen\assets"
    r"\c__Users_EthanYan_AppData_Roaming_Cursor_User_workspaceStorage_"
    r"d21b478989f91fda9dc1c82d38e9775f_images_image-3c98242c-3d64-498a-8095-f728330b941e.png"
)
OUT_C = r"d:\File\code\uvk1\mangosteen\App\bitmap_mangosteen.c"
OUT_H = r"d:\File\code\uvk1\mangosteen\App\bitmap_mangosteen.h"

W, H = 48, 48


def main() -> None:
    src = sys.argv[1] if len(sys.argv) > 1 else SRC
    img = Image.open(src).convert("RGBA")
    bg = Image.new("RGBA", img.size, (255, 255, 255, 255))
    bg.paste(img, mask=img.split()[-1])
    gray = bg.convert("L")
    gray.thumbnail((W, H), Image.Resampling.LANCZOS)
    canvas = Image.new("L", (W, H), 255)
    canvas.paste(gray, ((W - gray.width) // 2, (H - gray.height) // 2))

    px = canvas.load()
    for y in range(H):
        for x in range(W):
            old = px[x, y]
            new = 0 if old < 128 else 255
            px[x, y] = new
            err = old - new
            if x + 1 < W:
                px[x + 1, y] = max(0, min(255, px[x + 1, y] + err * 7 // 16))
            if x - 1 >= 0 and y + 1 < H:
                px[x - 1, y + 1] = max(0, min(255, px[x - 1, y + 1] + err * 3 // 16))
            if y + 1 < H:
                px[x, y + 1] = max(0, min(255, px[x, y + 1] + err * 5 // 16))
            if x + 1 < W and y + 1 < H:
                px[x + 1, y + 1] = max(0, min(255, px[x + 1, y + 1] + err // 16))

    pages = H // 8
    data = []
    for page in range(pages):
        for x in range(W):
            b = 0
            for bit in range(8):
                y = page * 8 + bit
                if px[x, y] < 128:
                    b |= 1 << bit
            data.append(b)

    lines = [
        "/* Auto-generated mangosteen boot icon, 48x48 ST7565 column-page format */",
        "#include <stdint.h>",
        '#include "bitmap_mangosteen.h"',
        "",
        "const uint8_t BITMAP_Mangosteen[BITMAP_MANGOSTEEN_PAGES * BITMAP_MANGOSTEEN_WIDTH] = {",
    ]
    for i in range(0, len(data), 12):
        chunk = ", ".join(f"0x{v:02X}" for v in data[i : i + 12])
        lines.append(f"\t{chunk},")
    lines.append("};")
    lines.append("")
    with open(OUT_C, "w", newline="\n") as f:
        f.write("\n".join(lines))

    with open(OUT_H, "w", newline="\n") as f:
        f.write(
            "#ifndef BITMAP_MANGOSTEEN_H\n"
            "#define BITMAP_MANGOSTEEN_H\n"
            "\n"
            "#include <stdint.h>\n"
            "\n"
            "#define BITMAP_MANGOSTEEN_WIDTH  48\n"
            "#define BITMAP_MANGOSTEEN_HEIGHT 48\n"
            "#define BITMAP_MANGOSTEEN_PAGES  (BITMAP_MANGOSTEEN_HEIGHT / 8)\n"
            "\n"
            "extern const uint8_t BITMAP_Mangosteen[BITMAP_MANGOSTEEN_PAGES * BITMAP_MANGOSTEEN_WIDTH];\n"
            "\n"
            "#endif\n"
        )
    print(f"wrote {OUT_C} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
