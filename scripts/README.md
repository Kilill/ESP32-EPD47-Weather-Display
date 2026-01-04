# Font Generation Scripts

This directory contains scripts for generating EPDiy-compatible font headers from TrueType fonts.

## Files

- **fontconvert_extended.py** - Python script that converts TrueType fonts to C header files with extended character support
- **generate_fonts.sh** - Bash script that generates all font variants for the weather station

## Character Coverage

The generated fonts include:
- **Basic ASCII** (0x20-0x7E): 95 characters - standard printable ASCII
- **Latin-1 Supplement** (0xA0-0xFF): 96 characters - extended Latin, including:
  - Degree symbol (°) at 0xB0 (176)
  - Non-breaking space at 0xA0 (160)
  - Accented characters (à, é, ñ, ü, etc.)
  - Currency and other symbols

**Total**: 191 characters per font

## Usage

### Generate All Fonts

```bash
cd /net/cyclops/opt/work/weather/Display
./scripts/generate_fonts.sh
```

This will generate all M PLUS Rounded 1c font variants:
- Regular and Medium weights
- 12pt, 20pt, and 32pt sizes
- 6 total font files

### Generate Single Font

```bash
python3 scripts/fontconvert_extended.py --compress FontName 20 path/to/font.ttf > output.h
```

## Font Header Structure

Generated headers include:
```c
#pragma once
#include "epd_driver.h"

#ifdef __cplusplus
typedef GFXfont EpdFont;
typedef GFXglyph EpdGlyph;
typedef UnicodeInterval EpdUnicodeInterval;
#endif

// Bitmap data array
const uint8_t FontNameBitmaps[...] = { ... };

// Glyph metadata array
const GFXglyph FontNameGlyphs[] = { ... };

// Unicode interval definitions
const UnicodeInterval FontNameIntervals[] = {
    { 0x20, 0x7E, 0x0 },   // ASCII at offset 0
    { 0xA0, 0xFF, 0x5F },  // Latin-1 Supplement at offset 95
};

// Font structure
const GFXfont FontName = { ... };
```

## Requirements

- Python 3.x
- freetype-py: `pip install freetype-py`
- TrueType font files in `gui/Fonts/` directory

## Notes

- Bitmaps are compressed using zlib when `--compress` is used
- Grayscale data is converted from 8bpp to 4bpp for EPDiy
- The C++ typedef guards prevent compilation errors when included in C code via `epd_driver.h`'s `extern "C"` block
