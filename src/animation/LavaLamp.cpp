#include "LavaLamp.h"
#include "AnimClock.h"

// Noise coordinates are 8.8 fixed point: 256 = one noise cell, about one blob.
// 9 per LED puts a blob every ~28 LEDs.
static constexpr uint16_t NOISE_PER_LED = 9;

// Brightness swings between these, so blobs glow and recede but never go dark.
static constexpr uint8_t MIN_BRI = 150;

// Colour at noise position x on this strip's slice y, at time z, with the
// palette rotated by colorShift.
static CRGB lavaColor(const CRGBPalette16& pal, uint16_t x, uint16_t y, uint16_t z, uint8_t colorShift) {
    // inoise8 clusters around mid-range; stretch ~48..208 to the full byte.
    int v = ((int)inoise8(x, y, z) - 48) * 255 / 160;
    if (v < 0) v = 0;
    if (v > 255) v = 255;

    // Fold through a triangle wave while slowly rotating, so every part of the
    // palette gets a turn without a seam where its last colour meets its first.
    // Scaled to 0..240 like ColorWave, since palettes are written one-way.
    uint8_t idx = scale8(triwave8((uint8_t)(v / 2) + colorShift), 240);

    // A coarser second octave swells blobs brighter and dimmer.
    uint8_t bri = MIN_BRI + scale8(inoise8(x / 2, y + 0x8000, z / 2), 255 - MIN_BRI);

    return ColorFromPalette(pal, idx, bri, LINEARBLEND);
}

void LavaLamp::update(Strip& strip) const {
    const uint32_t t = AnimClock::now();

    // Blobs morph one noise cell every ~4 s and drift one every ~16 s. Both are
    // t shifted down to 16 bits, which stays continuous when t wraps.
    const uint16_t z    = (uint16_t)(t >> 4);
    const uint16_t flow = (uint16_t)(t >> 6);
    // Each strip gets its own slice of the noise field.
    const uint16_t y = (uint16_t)(strip.id * 0x3000);
    // Palette rotation: one full turn every ~160 s.
    const uint8_t colorShift = beat88(96, AnimClock::timebase()) >> 8;

    const uint32_t now = millis();
    if (now - strip.paletteLastBlendMs >= 40) {
        strip.paletteLastBlendMs = now;
        nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 12);
    }

    // Subtracting the flow makes features travel towards larger offsets.
    const bool outward = strip.direction == Outward;

#if defined(DEVICE_ID_ESP32_WALL)
    for (int i = 0; i < strip.numLeds; i++) {
        uint16_t x = (uint16_t)(i * NOISE_PER_LED);
        x = outward ? (uint16_t)(x - flow) : (uint16_t)(x + flow);
        strip.leds[i] = lavaColor(strip.currentPalette, x, y, z, colorShift);
    }
#else
    // Mirrored around the centre, like the other centre-out animations.
    const int mid = strip.numLeds / 2;
    for (int d = 0; d <= mid; d++) {
        uint16_t x = (uint16_t)(d * NOISE_PER_LED);
        x = outward ? (uint16_t)(x - flow) : (uint16_t)(x + flow);
        const CRGB c = lavaColor(strip.currentPalette, x, y, z, colorShift);

        const int left  = mid - d;
        const int right = mid + d;
        if (left >= 0) strip.leds[left] = c;
        if (right < strip.numLeds) strip.leds[right] = c;
    }
#endif
}
