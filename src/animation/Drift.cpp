#include "Drift.h"
#include "AnimClock.h"
#include "AnimMath.h"

// Shared per-frame params — computed once and read by both linear and center variants
static uint16_t sHueInc16;
static uint8_t  sBrightDepth;
static uint16_t sBrightThetaInc16;

static void computeParams() {
    const uint32_t tb = AnimClock::timebase();
    // Narrow hue spread so the palette reads as soft ribbon sections rather
    // than rapid cycling. Very slow oscillation. (Halved when the palette walk
    // became a there-and-back fold, which covers twice the distance per cycle.)
    sHueInc16        = beatsin88(30,  40, 150, tb);
    // High brightness floor: depth 40–100 means floor stays at 155–215 — never goes dark
    sBrightDepth     = beatsin88(110, 40, 100, tb);
    // Slow brightness wave spacing
    sBrightThetaInc16 = beatsin88(70, 15 * 256, 25 * 256, tb);
}

static CRGB samplePalette(const CRGBPalette16& pal, uint16_t& hue16, uint16_t& brightTheta) {
    hue16   += sHueInc16;
    // Walk the palette there and back (0..255..0) instead of wrapping: a wrap
    // jumps straight from the palette's last colour to its first, a visible
    // seam, since palettes are written one-way (see palette-service's README).
    const uint16_t h9 = hue16 >> 7;
    const uint8_t hue8 = (h9 & 0x100) ? 255 - (h9 & 0xFF) : (h9 & 0xFF);

    brightTheta += sBrightThetaInc16;
    uint16_t b16  = sin16(brightTheta) + 32768;
    uint16_t bri16 = (uint32_t)b16 * b16 / 65536;
    uint8_t  bri8  = (uint32_t)bri16 * sBrightDepth / 65536;
    bri8 += (255 - sBrightDepth);  // lift floor so it stays bright

    return ColorFromPalette(pal, scale8(hue8, 240), bri8);
}

void Drift::driftLinear(Strip& strip, uint16_t hue16, uint16_t brightTheta) const {
    computeParams();

    for (int i = strip.numLeds - 1; i >= 0; i--) {
        CRGB c = samplePalette(strip.currentPalette, hue16, brightTheta);
        nblend(strip.leds[i], c, 64);
    }
}

void Drift::driftCenter(Strip& strip, uint16_t hue16, uint16_t brightTheta) const {
    computeParams();
    const int mid = strip.numLeds / 2;

    // Outward: process edge-to-center so colors emanate outward over time
    int startOffset, step;
    if (strip.direction == Outward) { startOffset = mid; step = -1; }
    else                            { startOffset = 0;   step =  1; }

    for (int offset = startOffset;
         (step > 0) ? (offset <= mid) : (offset >= 0);
         offset += step)
    {
        CRGB c = samplePalette(strip.currentPalette, hue16, brightTheta);

        int left  = mid - offset;
        int right = mid + offset;
        if (left  >= 0 && left  < strip.numLeds) nblend(strip.leds[left],  c, 64);
        if (right >= 0 && right < strip.numLeds && right != left) nblend(strip.leds[right], c, 64);
    }
}

void Drift::update(Strip& strip) const {
    // Pure functions of the shared clock (see AnimClock). The hue drifts at
    // 0.5–1 units/ms, ~8x slower than ColorWave's 5–9 (half its old 1–2, as the
    // folded palette walk covers twice the distance per cycle).
    const uint32_t t = AnimClock::now();
    const uint16_t brightTheta = beatsinIntegral16(50,  1 * 256, 2 * 256, t);
    const uint16_t hue16       = beatsinIntegral16(120, 128, 256, t);

    const uint32_t now = millis();
    if (now - strip.paletteLastBlendMs >= 40) {
        strip.paletteLastBlendMs = now;
        nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 12);
    }

#if defined(DEVICE_ID_ESP32_WALL)
    driftLinear(strip, hue16, brightTheta);
#else
    driftCenter(strip, hue16, brightTheta);
#endif
}
