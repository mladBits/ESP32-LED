#pragma once
#include <stdint.h>

// Interpolate from a to b (t = 0..255) taking the shortest path around
// the 0..255 hue circle, so e.g. 250 -> 5 blends through 0 instead of
// sweeping backwards across the whole spectrum.
static inline uint8_t lerpHueShortest(uint8_t a, uint8_t b, uint8_t t) {
    int16_t diff = (int16_t)b - (int16_t)a;

    // wrap to shortest path on the 0..255 hue circle
    if (diff > 127) diff -= 256;
    if (diff < -128) diff += 256;

    // linear interpolation along that shortest diff
    return (uint8_t)(a + ((diff * (int16_t)t) >> 8));
}
