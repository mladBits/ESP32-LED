#include "EmberBloom.h"
#include "AnimUtil.h"

void EmberBloom::update(Strip& strip) const {
    const uint32_t t = millis();
    tickPalette(strip, t);

    const uint16_t maxD = maxCenterDist(strip.numLeds);

    // slow breathing envelope
    const uint8_t breath = sin8((uint8_t)(t / 30));   // slower = larger divisor
    const uint8_t base = 30;                          // ambient floor
    const uint8_t depth = 190;

    for (uint16_t i = 0; i < strip.numLeds; i++) {
        const uint16_t d = centerDist(i, strip.numLeds);

        // center weighting: near center brighter, edges dimmer
        // tune the divisor for your strip lengths (108/88)
        const uint8_t centerWeight = (d > maxD) ? 0 : (uint8_t)(255 - (d * 255) / (maxD + 1));

        // subtle flicker noise (very light, to avoid “fire” harshness on a wall)
        const uint8_t flick = inoise8((uint16_t)(i * 20), (uint16_t)(t / 5));

        uint8_t bri = qadd8(base, scale8(scale8(centerWeight, breath), depth));
        bri = qadd8(bri, scale8(flick, 25));

        // palette index stays warm; small shift with distance
        const uint8_t idx = (uint8_t)(8 + d * 5);
        strip.leds[i] = ColorFromPalette(strip.currentPalette, idx, bri);
    }
}