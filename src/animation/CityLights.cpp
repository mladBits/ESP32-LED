#include "CityLights.h"
#include "AnimClock.h"
#include "Underlayer.h"
#include "config/Config.h"

// Dim palette glow behind the lights.
static constexpr uint8_t BASE_BRI = 18;

// Out of 256: chance a light is on during any one of its slots (~40%).
static constexpr uint8_t DENSITY = 100;

// A light gets a new on/off slot every 2^12 ticks of its own clock, which runs
// at 0.75-1.25x real time: one slot every ~3.3-5.5 s.
static constexpr uint8_t SLOT_SHIFT = 12;

// Integer hash (lowbias32) for stable per-light and per-slot randomness.
static inline uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

// FNV-1a of the device's unique id, so boards don't twinkle identical patterns.
static uint32_t deviceSalt() {
    static const uint32_t salt = [] {
        uint32_t h = 2166136261U;
        for (const char* p = MQTT_UNIQUE_ID; *p; p++) {
            h ^= (uint8_t)*p;
            h *= 16777619U;
        }
        return h;
    }();
    return salt;
}

void CityLights::update(Strip& strip) const {
    const uint32_t t = AnimClock::now();

    const uint32_t now = millis();
    if (now - strip.paletteLastBlendMs >= 60) {
        strip.paletteLastBlendMs = now;
        nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 16);
    }

    fillUnderlayer(strip, BASE_BRI);

    const uint32_t salt = deviceSalt() ^ ((uint32_t)strip.id * 0x9E3779B9U);
    for (int i = 0; i < strip.numLeds; i++) {
        const uint32_t h = hash32((uint32_t)i ^ salt);

        // Each light runs its own clock (own speed and offset), so lights drift
        // in and out of step instead of switching together. 64-bit so the
        // product can't overflow before t itself wraps.
        const uint32_t speed = 192 + (h & 0x7F);
        const uint64_t clk   = (((uint64_t)t * speed) >> 8) + (h >> 8);
        const uint32_t slot  = (uint32_t)(clk >> SLOT_SHIFT);
        const uint8_t  phase = (uint8_t)(clk >> (SLOT_SHIFT - 8));   // 0..255 through the slot

        const uint32_t s = hash32(slot ^ h);
        if ((s & 0xFF) >= DENSITY) continue;   // dark this slot

        // Quick fade-in, long fade-out, squared for soft tails.
        uint8_t env = phase < 64
            ? (uint8_t)(phase * 4)
            : (uint8_t)(255 - ((uint16_t)(phase - 64) * 4) / 3);
        env = scale8(env, env);
        if (!env) continue;

        // A new palette colour each time the light comes on (0..240: palettes
        // are written one-way).
        const uint8_t idx = scale8((uint8_t)(s >> 8), 240);
        strip.leds[i] += ColorFromPalette(strip.currentPalette, idx, env, LINEARBLEND);
    }
}
