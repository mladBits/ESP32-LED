#include "Aurora.h"
#include "AnimClock.h"
#include "AnimMath.h"
#include "PaletteSample.h"
#include "Underlayer.h"

// ---- Tuning ----------------------------------------------------------------

static constexpr int CURTAINS = 3;

// Dim palette glow behind the curtains, so the gaps are never black.
static constexpr uint8_t BASE_BRI = 14;

// Positions along the strip are fractions of its span in 16 bits (0 = first
// LED, 65535 = last). Each curtain wanders around its own home, up to WANDER
// either way (22% of the span), far enough to cross its neighbours.
static constexpr int32_t HOME[CURTAINS] = { 10923, 32768, 54613 };   // 1/6, 3/6, 5/6
static constexpr uint16_t WANDER = 14400;

// A curtain's reach either side of its centre breathes between 14% and 30% of
// the span. Its edges are very soft: the part brighter than half is about a
// third of that reach either side.
static constexpr uint16_t REACH_MIN = 9175;
static constexpr uint16_t REACH_MAX = 19661;

// Peak brightness swells between these, so curtains come and go but never
// vanish completely.
static constexpr uint8_t INTENSITY_MIN = 90;
static constexpr uint8_t INTENSITY_MAX = 255;

// Speeds, as FastLED bpm88 (256 = one cycle a minute). Every curtain gets its
// own, unrelated to the others', so the picture never visibly repeats.
static constexpr uint16_t WANDER_A_BPM[CURTAINS]  = { 341, 307, 379 };   // ~45, 50, 40 s
static constexpr uint16_t WANDER_B_BPM[CURTAINS]  = { 219, 241, 199 };   // ~70, 64, 77 s
static constexpr uint16_t REACH_BPM[CURTAINS]     = { 512, 463, 566 };   // ~30, 33, 27 s
static constexpr uint16_t INTENSITY_BPM[CURTAINS] = { 614, 472, 384 };   // ~25, 33, 40 s

// The curtains sit a third of a there-and-back palette cycle apart and travel
// through it together, one cycle every 4 minutes.
static constexpr uint16_t COLOUR_BPM = 64;

// Rays: fine noise that dims the curtains to RAY_MIN..255 of their brightness.
// Noise coordinates are 16.16 fixed point (65536 = one noise cell, about one
// ray), so this puts a ray every ~6 LEDs.
static constexpr uint32_t RAY_PER_LED = 11 << 10;
static constexpr uint8_t  RAY_MIN     = 153;   // 60%

// inoise16 clusters around mid-range; stretch this window to the full range
// (the same window LavaLamp measured and uses).
static constexpr int32_t NOISE_LO   = 15000;
static constexpr int32_t NOISE_SPAN = 35700;

// ----------------------------------------------------------------------------

// One curtain at the current instant. Positions are in 1/256 LED, so curtains
// glide between pixels instead of stepping.
struct Curtain {
    int32_t  centre;      // may lie a little off either end of the strip
    uint32_t reach;
    uint8_t  intensity;
    CRGB     colour;      // at full brightness
};

// Where the curtains are right now on a strip whose positions run 0..lastPos
// (1/256 LED). Everything comes from the shared clock.
static void computeCurtains(const Strip& strip, uint32_t lastPos, Curtain curtains[CURTAINS]) {
    const uint32_t tb = AnimClock::timebase();
    const uint16_t colourShift = beat88(COLOUR_BPM, tb);

    for (int k = 0; k < CURTAINS; k++) {
        // A different starting phase for every oscillator, curtain and strip.
        const uint16_t seed = (uint16_t)(k * 0x5555 + strip.id * 0x61C9);

        // Two unrelated slow swings added together: a wander that takes many
        // minutes to come back to where it was.
        const int32_t wander =
            (int32_t)beatsin88(WANDER_A_BPM[k], 0, WANDER, tb, seed) +
            (int32_t)beatsin88(WANDER_B_BPM[k], 0, WANDER, tb, (uint16_t)(seed + 0x9E37)) -
            WANDER;
        const uint16_t reach16 =
            beatsin88(REACH_BPM[k], REACH_MIN, REACH_MAX, tb, (uint16_t)(seed + 0x3C6F));

        Curtain& c = curtains[k];
        c.centre    = (int32_t)((int64_t)(HOME[k] + wander) * (int64_t)lastPos / 65536);
        c.reach     = (uint32_t)(((uint64_t)reach16 * lastPos) >> 16);
        c.intensity = (uint8_t)beatsin88(INTENSITY_BPM[k], INTENSITY_MIN, INTENSITY_MAX, tb,
                                         (uint16_t)(seed + 0xDAA6));
        c.colour    = paletteFolded(strip.currentPalette,
                                    (uint16_t)(colourShift + k * 21845), 255);
    }
}

// How much of the curtains' light the rays let through at noise position x
// (RAY_MIN..255), on this strip's slice y, at time z.
static uint8_t rayAt(uint32_t x, uint32_t y, uint32_t z) {
    int32_t v = (int32_t)(((int64_t)inoise16(x, y, z) - NOISE_LO) * 255 / NOISE_SPAN);
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return RAY_MIN + scale8((uint8_t)v, 255 - RAY_MIN);
}

// The curtains' light at position pos (1/256 LED), dimmed by ray.
static CRGB curtainLight(const Curtain curtains[CURTAINS], int32_t pos, uint8_t ray) {
    CRGB sum = CRGB::Black;
    for (int k = 0; k < CURTAINS; k++) {
        const Curtain& c = curtains[k];
        const int32_t  d = pos - c.centre;
        const uint32_t bump = smoothBump16((uint32_t)(d < 0 ? -d : d), c.reach);
        if (!bump) continue;

        // Squared for long, soft tails; then scaled once, at 16 bits, by the
        // curtain's intensity and the ray, so dim edges keep their precision.
        const uint32_t soft  = (bump * bump) >> 16;
        const uint32_t level = (soft * (c.intensity + 1u)) >> 8;
        const uint8_t  bri   = (uint8_t)((level * (ray + 1u)) >> 16);
        if (!bri) continue;

        CRGB light = c.colour;
        light.nscale8_video(bri);
        sum += light;   // saturating: crossing curtains mix their colours
    }
    return sum;
}

void Aurora::update(Strip& strip) const {
    const uint32_t t = AnimClock::now();

    fillUnderlayer(strip, BASE_BRI);

    // Rays change shape over ~8 s and ripple along at one ray every ~8 s. The
    // noise repeats every 256 cells (2^24), which divides 2^32, so both stay
    // continuous when t wraps.
    const uint32_t z    = t << 3;
    const uint32_t flow = t << 3;
    // Each strip gets its own slice of the noise field.
    const uint32_t y = (uint32_t)strip.id * 0x300000 + 0x1800000;

    // Subtracting the flow makes features travel towards larger offsets.
    const bool outward = strip.direction == Outward;

    Curtain curtains[CURTAINS];

#if defined(DEVICE_ID_ESP32_WALL)
    if (strip.numLeds < 2) return;
    computeCurtains(strip, (uint32_t)(strip.numLeds - 1) * 256, curtains);

    for (int i = 0; i < strip.numLeds; i++) {
        uint32_t x = (uint32_t)i * RAY_PER_LED;
        x = outward ? x - flow : x + flow;
        strip.leds[i] += curtainLight(curtains, i * 256, rayAt(x, y, z));
    }
#else
    // Mirrored around the centre, like the other centre-out animations: the
    // curtains live along the distance from the centre.
    const int mid = strip.numLeds / 2;
    if (mid < 1) return;
    computeCurtains(strip, (uint32_t)mid * 256, curtains);

    for (int d = 0; d <= mid; d++) {
        uint32_t x = (uint32_t)d * RAY_PER_LED;
        x = outward ? x - flow : x + flow;
        const CRGB light = curtainLight(curtains, d * 256, rayAt(x, y, z));

        const int left  = mid - d;
        const int right = mid + d;
        if (left >= 0) strip.leds[left] += light;
        if (right < strip.numLeds && right != left) strip.leds[right] += light;
    }
#endif
}
