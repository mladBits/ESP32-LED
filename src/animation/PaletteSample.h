#pragma once

#include <FastLED.h>
#include "AnimMath.h"

// Palette lookups for the animations, at 16-bit precision: FastLED's
// ColorFromPaletteExtended blends 256 steps between neighbouring palette
// entries (ColorFromPalette's 8-bit index: 16), so slow motion glides instead
// of stepping.

// Highest index: exactly the last entry. Indexes above it would blend entry 15
// back into entry 0, and palettes are written one-way.
static constexpr uint16_t PALETTE_INDEX_MAX = 15 * 4096;

// Colour at a 16-bit palette index (0..PALETTE_INDEX_MAX). Brightness is
// applied "video" style (like ColorFromPalette's), so a dim colour never loses
// a channel to 0.
inline CRGB paletteAt(const CRGBPalette16& pal, uint16_t index, uint8_t bri = 255) {
    CRGB c = ColorFromPaletteExtended(pal, index, 255, LINEARBLEND);
    if (bri != 255) c.nscale8_video(bri);
    return c;
}

// Colour at a 16-bit phase walked there and back across the palette: one
// phase cycle goes first entry -> last -> first, never wrapping (foldPhase16).
inline CRGB paletteFolded(const CRGBPalette16& pal, uint16_t phase, uint8_t bri = 255) {
    return paletteAt(pal, scale16(foldPhase16(phase), PALETTE_INDEX_MAX), bri);
}
