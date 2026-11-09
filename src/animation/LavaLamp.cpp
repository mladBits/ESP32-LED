#include "LavaLamp.h"
#include "AnimClock.h"
#include "PaletteSample.h"

// Noise coordinates are 16.16 fixed point: 65536 = one noise cell, about one
// blob. 9/256 of a cell per LED puts a blob every ~28 LEDs.
static constexpr uint32_t NOISE_PER_LED = 9 << 8;

// Brightness swings between these, so blobs glow and recede but never go dark.
static constexpr uint8_t MIN_BRI = 150;

// inoise16 clusters around mid-range: its 1st..99th percentiles are about
// 17900..47800 (measured over this animation's coordinates against FastLED
// 3.10.9). Stretch a window a little wider than that to the full 0..65535,
// the same proportions as the old 8-bit stretch of inoise8's 48..208.
static constexpr int32_t NOISE_LO   = 15000;
static constexpr int32_t NOISE_SPAN = 35700;

// Colour at noise position x on this strip's slice y, at time z, with the
// palette rotated by colorShift. 16-bit throughout (noise, rotation, palette
// lookup), so the slow morphing glides instead of stepping.
static CRGB lavaColor(const CRGBPalette16& pal, uint32_t x, uint32_t y, uint32_t z, uint16_t colorShift) {
    int32_t v = (int32_t)(((int64_t)inoise16(x, y, z) - NOISE_LO) * 65535 / NOISE_SPAN);
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;

    // Half a there-and-back cycle across the noise range, slowly rotating, so
    // every part of the palette gets a turn without a seam where its last
    // colour meets its first.
    const uint16_t phase = (uint16_t)(v / 2) + colorShift;

    // A coarser second octave swells blobs brighter and dimmer.
    const uint8_t bri = MIN_BRI + scale8(inoise16(x / 2, y + 0x800000, z / 2) >> 8, 255 - MIN_BRI);

    return paletteFolded(pal, phase, bri);
}

void LavaLamp::update(Strip& strip) const {
    const uint32_t t = AnimClock::now();

    // Blobs morph one noise cell every ~4 s and drift one every ~16 s. The noise
    // repeats every 256 cells (2^24), which divides 2^32, so these stay
    // continuous when t wraps.
    const uint32_t z    = t << 4;
    const uint32_t flow = t << 2;
    // Each strip gets its own slice of the noise field.
    const uint32_t y = (uint32_t)strip.id * 0x300000;
    // Palette rotation: one full turn every ~160 s, at 16-bit resolution.
    const uint16_t colorShift = beat88(96, AnimClock::timebase());

    // Subtracting the flow makes features travel towards larger offsets.
    const bool outward = strip.direction == Outward;

#if defined(DEVICE_ID_ESP32_WALL)
    for (int i = 0; i < strip.numLeds; i++) {
        uint32_t x = (uint32_t)i * NOISE_PER_LED;
        x = outward ? x - flow : x + flow;
        strip.leds[i] = lavaColor(strip.currentPalette, x, y, z, colorShift);
    }
#else
    // Mirrored around the centre, like the other centre-out animations.
    const int mid = strip.numLeds / 2;
    for (int d = 0; d <= mid; d++) {
        uint32_t x = (uint32_t)d * NOISE_PER_LED;
        x = outward ? x - flow : x + flow;
        const CRGB c = lavaColor(strip.currentPalette, x, y, z, colorShift);

        const int left  = mid - d;
        const int right = mid + d;
        if (left >= 0) strip.leds[left] = c;
        if (right < strip.numLeds) strip.leds[right] = c;
    }
#endif
}
