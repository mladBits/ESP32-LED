#include "NeonGrid.h"
#include "AnimUtil.h"

void NeonGrid::update(Strip& strip) const {
    const uint16_t n = strip.numLeds;
    const uint32_t t = millis();

    tickPalette(strip, t);

    uint8_t base = 90;    // ambient floor
    uint8_t depth = 140;  // modulation depth
    uint8_t hue0 = (uint8_t)(t / 20);

    for (uint16_t i = 0; i < n; i++) {
        uint16_t d = centerDist(i, n);

        uint8_t a = sin8((uint8_t)(d * 10 + (t / 8)));   // outward wave
        uint8_t b = sin8((uint8_t)(d * 6  - (t / 13)));  // counter wave
        uint8_t m = scale8(a, b);                        // interference

        uint8_t bri = qadd8(base, scale8(m, depth));
        uint8_t idx = hue0 + (uint8_t)(d * 5);

        strip.leds[i] = ColorFromPalette(strip.currentPalette, idx, bri);
    }
}