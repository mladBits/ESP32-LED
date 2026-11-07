#pragma once

#include <FastLED.h>
#include "led/Strip.h"
#include "AnimClock.h"

// Ambient colour base so an animation's dark gaps still carry a hint of the
// palette instead of pure black — a dim, slowly drifting gradient sampled from
// the strip's current palette. Bright features are drawn on top.

inline CRGB underlayerColorAt(const Strip& strip, int i, uint8_t bri, uint8_t drift) {
    int n = strip.numLeds > 0 ? strip.numLeds : 1;
    uint8_t idx = drift + (uint8_t)(((uint32_t)i * 128) / n);   // gentle half-palette spread
    return ColorFromPalette(strip.currentPalette, idx, bri, LINEARBLEND);
}

// Overwrite the strip with the dim gradient (use before adding bright features).
inline void fillUnderlayer(Strip& strip, uint8_t bri) {
    uint8_t drift = beat8(1, AnimClock::timebase());
    for (int i = 0; i < strip.numLeds; i++) {
        strip.leds[i] = underlayerColorAt(strip, i, bri, drift);
    }
}

// Ease the current frame toward the dim gradient, so trails settle into colour
// rather than fading to black.
inline void blendTowardUnderlayer(Strip& strip, uint8_t bri, uint8_t amt) {
    uint8_t drift = beat8(1, AnimClock::timebase());
    for (int i = 0; i < strip.numLeds; i++) {
        CRGB base = underlayerColorAt(strip, i, bri, drift);
        nblend(strip.leds[i], base, amt);
    }
}
