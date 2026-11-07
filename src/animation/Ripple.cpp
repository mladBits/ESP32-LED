#include "Ripple.h"
#include "Underlayer.h"

// Ambient palette glow beneath the ripples.
static constexpr uint8_t BASE_BRI = 30;

void Ripple::update(Strip& strip) const {
    const uint32_t now = millis();
    uint8_t s = strip.id;
    if (s >= MAX_STRIPS) s = 0;

    if (now - strip.paletteLastBlendMs >= 60) {
        strip.paletteLastBlendMs = now;
        nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 16);
    }

    // Respawn any ripple that has finished its life (all start at 0 -> seeded
    // on the first frame with staggered, randomised gaps).
    for (uint8_t i = 0; i < NUM_RIPPLES; i++) {
        RippleState& r = ripples[s][i];
        if (now >= r.startMs + r.lifeMs) {
            r.startMs = now + random16(200, 1400);   // brief gap before it reappears
            r.lifeMs  = random16(2600, 4200);        // slow expansion + fade
            r.hue     = random8();
            r.origin  = random8(0, strip.numLeds);   // used on WALL (linear)
        }
    }

    render(strip, now);
}

void Ripple::render(Strip& strip, uint32_t now) const {
    uint8_t s = strip.id;
    if (s >= MAX_STRIPS) s = 0;

    // Soft trails behind each expanding ring, settling into the colour base.
    blendTowardUnderlayer(strip, BASE_BRI, 40);

    const int mid = strip.numLeds / 2;
#if defined(DEVICE_ID_ESP32_WALL)
    const int axisMax = strip.numLeds;
#else
    const int axisMax = mid;
#endif

    for (uint8_t i = 0; i < NUM_RIPPLES; i++) {
        RippleState& r = ripples[s][i];
        if (now < r.startMs) continue;              // waiting to appear
        uint32_t age = now - r.startMs;
        if (age >= r.lifeMs) continue;              // expired (respawns next frame)

        // Radius sweeps the axis exactly once over the ripple's life; brightness
        // fades from full to zero as it goes.
        int     radius = (int)((age * (uint32_t)axisMax) / r.lifeMs);
        uint8_t life8  = 255 - (uint8_t)((age * 255) / r.lifeMs);
        CRGB col = ColorFromPalette(strip.currentPalette, r.hue, life8, LINEARBLEND);

#if defined(DEVICE_ID_ESP32_WALL)
        int a = r.origin - radius;
        int b = r.origin + radius;
#else
        // Outward: expand from centre. Inward: converge toward it.
        int rr = (strip.direction == Outward) ? radius : (mid - radius);
        int a = mid - rr;
        int b = mid + rr;
#endif
        if (a >= 0 && a < strip.numLeds) strip.leds[a] += col;
        if (b >= 0 && b < strip.numLeds && b != a) strip.leds[b] += col;
    }

    // Widen the rings into soft bands.
    blur1d(strip.leds, strip.numLeds, 48);
}
