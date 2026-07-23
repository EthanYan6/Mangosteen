# UV-K1 GB2312 Font (FontTool layout)

- `uvk1_gb2312_font.bin` — combined image for SPI Flash write at **0x0A0000**
  - 16×16 楷体: offset 0, size 261696 → Flash `0xA0000..0xDFE3F`
  - pad zeros: 448 bytes
  - 8×8: offset `0x40000`, size 65424 → Flash `0xE0000..0xEFF8F`
- Source lattices: `kaiti_16x16.FON`, `8x8.FON`
- Rebuild: `python pack_uvk1_gb2312_font.py`

Addresses match https://gitee.com/oldlicn/uv-k1_font-tool
