#include "Embers.h"
#include "Underlayer.h"

// Ambient palette glow beneath the embers.
static constexpr uint8_t BASE_BRI = 30;

void Embers::update(Strip& strip) const {
    const uint32_t now = millis();
    uint8_t s = strip.id;
    if (s >= MAX_STRIPS) s = 0;

    if (now - strip.paletteLastBlendMs >= 80) {
        strip.paletteLastBlendMs = now;
        nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 20);
    }

    int count = strip.numLeds / 12;
    if (count < 1) count = 1;
    if (count > MAX_EMBERS) count = MAX_EMBERS;

    // Dim palette gradient base each frame — embers don't move, they only breathe.
    fillUnderlayer(strip, BASE_BRI);

    for (int i = 0; i < count; i++) {
        EmberState& e = embers[s][i];

        // (Re)spawn when finished (all zero-initialised -> seeded on first frame,
        // with a randomised delay so they don't pulse in unison).
        if (now >= e.startMs + e.lifeMs) {
            e.startMs = now + random16(0, 1600);
            e.lifeMs  = random16(2500, 5000);
            e.pos     = random16(0, strip.numLeds);
            e.hue     = random8();
            continue;   // not visible yet
        }
        if (now < e.startMs) continue;   // waiting to appear

        uint32_t age = now - e.startMs;
        // quadwave8 gives a smooth 0 -> 255 -> 0 swell across the ember's life.
        uint8_t phase = (uint8_t)((age * 255) / e.lifeMs);
        uint8_t env   = quadwave8(phase);

        if (e.pos < strip.numLeds) {
            strip.leds[e.pos] += ColorFromPalette(strip.currentPalette, e.hue, env, LINEARBLEND);
        }
    }

    // Soft glow around each point.
    blur1d(strip.leds, strip.numLeds, 40);
}
