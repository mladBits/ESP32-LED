#include "Beams.h"
#include "Underlayer.h"
#include "AnimClock.h"
#include "AnimMath.h"

// Number of concurrent beams travelling along the axis.
static constexpr uint8_t NUM_BEAMS = 3;

// Ambient palette glow beneath the beams.
static constexpr uint8_t BASE_BRI = 40;

CRGB Beams::beamColorAt(const CRGBPalette16& pal, int q, int axisLen, uint16_t pos16) const {
    CRGB c = CRGB::Black;
    const uint32_t tb = AnimClock::timebase();

    int w = axisLen / 6;
    if (w < 2) w = 2;
    const int w256 = w * 256;          // half-width in Q8 (pixels * 256)

    // Beams travel across an extended span (axisLen + 2w) so they fade in/out
    // past the ends instead of popping when the phase wraps.
    const uint32_t span = (uint32_t)axisLen + 2 * w;
    const int q256 = q * 256;

    for (uint8_t b = 0; b < NUM_BEAMS; b++) {
        // Evenly staggered positions around the travel phase.
        uint16_t p = pos16 + (uint16_t)(b * (65536UL / NUM_BEAMS));
        // Sub-pixel centre in Q8 so slow motion slides smoothly instead of
        // hopping whole pixels. center*256 = (p*span/65536)*256 = (p*span)>>8.
        int center256 = (int)(((uint32_t)p * span) >> 8) - w256;   // -w .. axisLen+w

        int d = q256 - center256;
        if (d < 0) d = -d;
        if (d >= w256) continue;

        // Smooth bell falloff: full brightness at the centre, ~0 at the edge.
        uint8_t angle = (uint8_t)(((uint32_t)d * 128) / w256);
        uint8_t f = cos8(angle);

        // Slow, per-beam brightness pulsation (staggered so they breathe apart).
        uint8_t env = beatsin8(4 + b, 40, 255, tb, (uint8_t)(b * 85));
        uint8_t bri = scale8(f, env);

        // Per-beam palette region plus a very slow global hue drift.
        uint8_t idx = (uint8_t)(b * (256 / NUM_BEAMS) + beat8(1, tb));

        c += ColorFromPalette(pal, idx, bri, LINEARBLEND);
    }
    return c;
}

void Beams::beamsLinear(Strip& strip, uint16_t pos16) const {
    const int L = strip.numLeds;
    fillUnderlayer(strip, BASE_BRI);
    for (int x = 0; x < L; x++) {
        strip.leds[x] += beamColorAt(strip.currentPalette, x, L, pos16);
    }
    blur1d(strip.leds, L, 32);
}

void Beams::beamsCenter(Strip& strip, uint16_t pos16) const {
    const int mid = strip.numLeds / 2;
    const int axisLen = mid + 1;

    fillUnderlayer(strip, BASE_BRI);
    for (int a = 0; a <= mid; a++) {
        // Outward: beams emanate from the centre; Inward: they converge to it.
        int q = (strip.direction == Outward) ? a : (mid - a);
        CRGB c = beamColorAt(strip.currentPalette, q, axisLen, pos16);

        int left  = mid - a;
        int right = mid + a;
        if (left  >= 0 && left  < strip.numLeds) strip.leds[left]  += c;
        if (right >= 0 && right < strip.numLeds && right != left) strip.leds[right] += c;
    }
    blur1d(strip.leds, strip.numLeds, 32);
}

void Beams::update(Strip& strip) const {
    // Travel phase from the shared clock (see AnimClock). Slow drift: a full
    // traversal takes ~20s at 2–5 units/ms, speed wandering gently.
    const uint16_t pos16 = beatsinIntegral16(40, 2 * 256, 5 * 256, AnimClock::now());

    const uint32_t now = millis();
    if (now - strip.paletteLastBlendMs >= 40) {
        strip.paletteLastBlendMs = now;
        nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 12);
    }

#if defined(DEVICE_ID_ESP32_WALL)
    beamsLinear(strip, pos16);
#else
    beamsCenter(strip, pos16);
#endif
}
