#include "CyberPulse.h"
#include "AnimUtil.h"

void CyberPulse::update(Strip& strip) const {
    const uint32_t t = millis();
    tickPalette(strip, t);

    // pulse repeats
    const uint16_t periodMs = 1800;           // try 1400-2400
    const uint16_t maxD = maxCenterDist(strip.numLeds);

    // pulse position in distance-space [0..maxD]
    const uint16_t p = (uint16_t)(((t % periodMs) * (uint32_t)(maxD + 1)) / periodMs);

    // width controls thickness of neon ring
    const uint8_t width = 18;                // try 12-28
    const uint8_t falloffPerStep = 14;       // higher = sharper edge

    // slow palette index drift to keep it alive
    const uint8_t baseIdx = (uint8_t)(t / 10);

    for (uint16_t i = 0; i < strip.numLeds; i++) {
        const uint16_t d = centerDist(i, strip.numLeds);
        const uint16_t dist = (d > p) ? (d - p) : (p - d);

        uint8_t bri = 0;
        if (dist <= width) {
            uint16_t raw = 255 - (uint16_t)dist * falloffPerStep;
            bri = (raw > 255) ? 0 : (uint8_t)raw;
            // soften the peak a bit for wall-bounce
            bri = scale8(bri, 220);
        }

        const uint8_t idx = (uint8_t)(baseIdx + d * 6);
        strip.leds[i] = ColorFromPalette(strip.currentPalette, idx, bri);
    }
}
