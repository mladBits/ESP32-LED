#pragma once

#include <FastLED.h>
#include "led/Strip.h"
#include "AnimClock.h"
#include "AnimMath.h"

// Ambient colour base so an animation's dark gaps still carry a hint of the
// palette instead of pure black — a dim, slowly drifting gradient sampled from
// the strip's current palette. Bright features are drawn on top.

// drift is a 16-bit phase; the palette is walked there and back (foldPhase),
// so the gradient never jumps from the palette's last colour to its first.
inline CRGB underlayerColorAt(const Strip& strip, int i, uint8_t bri, uint16_t drift) {
    int n = strip.numLeds > 0 ? strip.numLeds : 1;
    // Gentle half-palette spread: a quarter of a there-and-back cycle.
    const uint16_t phase = drift + (uint16_t)(((uint32_t)i * 16384) / n);
    return ColorFromPalette(strip.currentPalette, scale8(foldPhase(phase), 240), bri, LINEARBLEND);
}

// One there-and-back cycle every 2 minutes: the palette drifts by one
// palette-length a minute, as it did when it wrapped once a minute (beat8(1)).
inline uint16_t underlayerDrift() {
    return beat88(128, AnimClock::timebase());
}

// Overwrite the strip with the dim gradient (use before adding bright features).
inline void fillUnderlayer(Strip& strip, uint8_t bri) {
    const uint16_t drift = underlayerDrift();
    for (int i = 0; i < strip.numLeds; i++) {
        strip.leds[i] = underlayerColorAt(strip, i, bri, drift);
    }
}

// Ease the current frame toward the dim gradient, so trails settle into colour
// rather than fading to black.
inline void blendTowardUnderlayer(Strip& strip, uint8_t bri, uint8_t amt) {
    const uint16_t drift = underlayerDrift();
    for (int i = 0; i < strip.numLeds; i++) {
        CRGB base = underlayerColorAt(strip, i, bri, drift);
        nblend(strip.leds[i], base, amt);
    }
}
