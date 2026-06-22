#include "Drift.h"

// Shared per-frame params — computed once and read by both linear and center variants
static uint16_t sHueInc16;
static uint8_t  sBrightDepth;
static uint16_t sBrightThetaInc16;

static void computeParams() {
    // Narrower hue spread than ColorWave (80–300 vs 300–1500) so palette reads as
    // soft ribbon sections rather than rapid rainbow cycling. Very slow oscillation.
    sHueInc16        = beatsin88(30,  80, 300);
    // High brightness floor: depth 40–100 means floor stays at 155–215 — never goes dark
    sBrightDepth     = beatsin88(110, 40, 100);
    // Slow brightness wave spacing
    sBrightThetaInc16 = beatsin88(70, 15 * 256, 25 * 256);
}

static CRGB samplePalette(const CRGBPalette16& pal, uint16_t& hue16, uint16_t& brightTheta) {
    hue16   += sHueInc16;
    uint8_t hue8 = hue16 >> 8;

    brightTheta += sBrightThetaInc16;
    uint16_t b16  = sin16(brightTheta) + 32768;
    uint16_t bri16 = (uint32_t)b16 * b16 / 65536;
    uint8_t  bri8  = (uint32_t)bri16 * sBrightDepth / 65536;
    bri8 += (255 - sBrightDepth);  // lift floor so it stays bright

    return ColorFromPalette(pal, scale8(hue8, 240), bri8);
}

void Drift::driftLinear(Strip& strip) const {
    computeParams();
    uint16_t hue16      = sHue16;
    uint16_t brightTheta = sPseudotime;

    for (int i = strip.numLeds - 1; i >= 0; i--) {
        CRGB c = samplePalette(strip.currentPalette, hue16, brightTheta);
        nblend(strip.leds[i], c, 64);
    }
}

void Drift::driftCenter(Strip& strip) const {
    computeParams();
    const int mid = strip.numLeds / 2;

    uint16_t hue16      = sHue16;
    uint16_t brightTheta = sPseudotime;

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
    const uint32_t now = millis();

    // Only advance time accumulators once per ms, even when called per-strip
    if (now != lastFrameMs) {
        lastFrameMs = now;
        uint16_t ms     = (uint16_t)now;
        uint16_t deltams = ms - sLastMillis;
        sLastMillis      = ms;

        // ~4x slower than ColorWave: 1–2 units/ms instead of 5–9
        sPseudotime += (uint16_t)(deltams * beatsin88(50,  1, 2));
        sHue16      += (uint16_t)(deltams * beatsin88(120, 1, 2));
    }

    if (now - strip.paletteLastBlendMs >= 40) {
        strip.paletteLastBlendMs = now;
        nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 12);
    }

#if defined(DEVICE_ID_ESP32_WALL)
    driftLinear(strip);
#else
    driftCenter(strip);
#endif
}
