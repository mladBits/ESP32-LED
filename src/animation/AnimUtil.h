#pragma once
#include <stdint.h>

static inline uint16_t centerDist(uint16_t i, uint16_t n) {
    uint16_t mid = n / 2;
    uint16_t left  = (mid == 0) ? 0 : (uint16_t)(mid - 1);
    uint16_t right = mid;

    uint16_t dl = (i > left)  ? (uint16_t)(i - left)  : (uint16_t)(left - i);
    uint16_t dr = (i > right) ? (uint16_t)(i - right) : (uint16_t)(right - i);
    return (dl < dr) ? dl : dr;
}

static inline uint16_t maxCenterDist(uint16_t n) {
    if (n == 0) return 0;
    uint16_t d0 = centerDist(0, n);
    uint16_t d1 = centerDist((uint16_t)(n - 1), n);
    return (d0 > d1) ? d0 : d1;
}

static inline void tickPalette(Strip& strip, uint32_t now, uint16_t intervalMs = 40, uint8_t maxChanges = 16) {
    if (now - strip.paletteLastBlendMs < intervalMs) return;
    strip.paletteLastBlendMs = now;
    nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, maxChanges);
}