#!python3
import freetype
import zlib
import sys
import re
import math
import argparse
from collections import namedtuple

parser = argparse.ArgumentParser(description="Generate a header file from a font to be used with epdiy.")
parser.add_argument("name", action="store", help="name of the font.")
parser.add_argument("size", type=int, help="font size to use.")
parser.add_argument("fontstack", action="store", nargs='+', help="list of font files, ordered by descending priority.")
parser.add_argument("--compress", dest="compress", action="store_true", help="compress glyph bitmaps.")
args = parser.parse_args()

GlyphProps = namedtuple("GlyphProps", ["width", "height", "advance_x", "left", "top", "compressed_size", "data_offset", "code_point"])

font_stack = [freetype.Face(f) for f in args.fontstack]
compress = args.compress
size = args.size
font_name = args.name

# inclusive unicode code point intervals
# must not overlap and be in ascending order
# Extended to include Latin-1 Supplement (0xA0-0xFF) for degree symbol and other extended characters
intervals = [
    (32, 126),      # Basic ASCII printable characters
    (160, 255),     # Latin-1 Supplement (includes degree symbol at 0xB0/176)
]


def norm_floor(val):
    return int(math.floor(val / (1 << 6)))

def norm_ceil(val):
    return int(math.ceil(val / (1 << 6)))

for face in font_stack:
    # shift by 6 bytes, because sizes are given as 6-bit fractions
    # the display has about 150 dpi.
    face.set_char_size(size << 6, size << 6, 150, 150)

def chunks(l, n):
    for i in range(0, len(l), n):
        yield l[i:i + n]

def get_glyph_data(char_code):
    """
    Try to get glyph data from the font stack, which may contain the same
    font in different formats.
    """
    for f in font_stack:
        glyph_index = f.get_char_index(char_code)
        if glyph_index <= 0:
            continue
        f.load_glyph(glyph_index, freetype.FT_LOAD_RENDER)
        props = f.glyph
        bitmap = props.bitmap
        return (props, bitmap)

    raise ValueError(
            f"code point U+{char_code:X} ({chr(char_code)}) "
            f"not found in {[f.family_name for f in font_stack]}!"
    )


def chunks(l, n):
    for i in range(0, len(l), n):
        yield l[i:i + n]

current_data_offset = 0
glyphs = []

for i_start, i_end in intervals:
    for code_point in range(i_start, i_end + 1):
        props, bitmap = get_glyph_data(code_point)
        assert bitmap.pixel_mode == freetype.FT_PIXEL_MODE_GRAY
        assert bitmap.num_grays == 256

        buf = bytes(bitmap.buffer)
        # epdiy uses 4bpp grayscale, pillow and freetype use 8bpp
        buf = bytes([b >> 4 for b in buf])

        width = bitmap.width
        height = bitmap.rows
        # advance_x: the space to the next caracter
        advance_x = norm_ceil(props.advance.x)
        # left is the x pixles from the current pos to start the bitmap
        left = props.bitmap_left
        # top is the pixels from the baseline to the top of the bitmap
        top = props.bitmap_top

        packed_buf = []
        if width > 0 and height > 0:
            for row in chunks(buf, width):
                packed_row = []
                for byte_index, byte_tuple in enumerate(chunks(row, 2)):
                    if len(byte_tuple) > 1:
                        packed_row.append((byte_tuple[0] << 4) | byte_tuple[1])
                    else:
                        packed_row.append((byte_tuple[0] << 4))
                packed_buf.extend(packed_row)

        packed_buf = bytes(packed_buf)

        if compress:
            compressed_buf = zlib.compress(packed_buf)
            compressed_size = len(compressed_buf)
        else:
            compressed_buf = packed_buf
            compressed_size = 0

        glyph = GlyphProps(
            width=width,
            height=height,
            advance_x=advance_x,
            left=left,
            top=top,
            compressed_size=compressed_size,
            data_offset=current_data_offset,
            code_point=code_point
        )
        glyphs.append((glyph, compressed_buf))
        current_data_offset += len(compressed_buf)


total_size = sum(len(g[1]) for g in glyphs)
compressed_size = sum(g[0].compressed_size for g in glyphs)

print(f"total {total_size}", file=sys.stderr)
print(f"compressed {compressed_size}", file=sys.stderr)

print("#pragma once")
print('#include "epd_driver.h"')
print(f"const uint8_t {font_name}Bitmaps[{total_size}] = {{")
for _, buf in glyphs:
    print("   ", end="")
    for c in buf:
        print(f" 0x{c:02X},", end="")
    print()
print("};")

print(f"const GFXglyph {font_name}Glyphs[] = {{")
for glyph, _ in glyphs:
    # Special handling for backslash to avoid multi-line comment warning
    char_display = chr(glyph.code_point)
    if char_display == '\\':
        char_display = "backslash"
    print(f"    {{ {glyph.width}, {glyph.height}, {glyph.advance_x}, {glyph.left}, {glyph.top}, {glyph.compressed_size}, {glyph.data_offset} }}, // {char_display}")
print("};")

interval_structs = []
offset = 0
for i_start, i_end in intervals:
    interval_structs.append(f"    {{ 0x{i_start:X}, 0x{i_end:X}, 0x{offset:X} }}")
    offset += i_end - i_start + 1

print(f"const UnicodeInterval {font_name}Intervals[] = {{")
print(",\n".join(interval_structs))
print("};")

print(f"const GFXfont {font_name} = {{")
print(f"    (uint8_t*){font_name}Bitmaps,")
print(f"    (GFXglyph*){font_name}Glyphs,")
print(f"    (UnicodeInterval*){font_name}Intervals,")
print(f"    {len(intervals)},")
print(f"    1,")
face = font_stack[0]
# max bbox for all glyphs (in pixels)
print(f"    {norm_ceil(face.bbox.xMax - face.bbox.xMin)},")
# ascender from baseline to the top (in pixels)
print(f"    {norm_ceil(face.ascender)},")
# descender form the baseline (in pixels, negative)
print(f"    {norm_floor(face.descender)},")
print("};")
