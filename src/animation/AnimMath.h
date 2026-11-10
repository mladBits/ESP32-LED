#pragma once

#include <stdint.h>
#include <math.h>

// Closed-form running total, mod 2^16, of a speed that follows FastLED's
// beatsin88(bpm88, lo, hi): what `acc += deltams * speed` accumulates frame by
// frame, computed directly from the time t (ms) instead.
//
// Animations use this so their state is a pure function of the shared clock
// (AnimClock::now()) and every board computes the same value. Speeds are Q8
// (256 = 1 unit per ms) so fractional ones can be expressed; lo8/hi8 may be
// negative for a pure wobble around zero. Continuous across t's 2^32 wrap.
static inline uint16_t beatsinIntegral16(uint16_t bpm88, int32_t lo8, int32_t hi8, uint32_t t) {
    // Mean speed in exact integer math. (lo8+hi8) * 2^32 / 512 is a multiple of
    // 2^16, so this term is continuous when t wraps.
    const int64_t linear = ((int64_t)(lo8 + hi8) * (int64_t)t) / 512;

    // Swing: the integral of amp*sin(w*t) is -(amp/w)*cos(w*t). The phase is
    // beat88's at full 32-bit resolution (one cycle = 2^32), so a wrap of t is
    // a whole number of cycles and this term is continuous too.
    const uint32_t step  = (uint32_t)bpm88 * 280u;          // beat88 phase per ms
    const uint32_t phase = t * step;                        // wraps mod 2^32
    const float    turn  = 6.28318530718f / 4294967296.0f;  // radians per phase unit
    const float    amp   = (float)(hi8 - lo8) / 512.0f;     // units per ms
    const float    osc   = -(amp / ((float)step * turn)) * cosf((float)phase * turn);

    return (uint16_t)(linear + (int64_t)lroundf(osc));
}

// A 16-bit phase folded into a triangle: 0 up to 65535 and back to 0 over one
// cycle (steps of 2). Sample palettes through this instead of letting the index
// wrap: palettes are written one-way, so a wrap jumps straight from the last
// colour to the first, a visible seam. Continuous everywhere, including across
// the 65535 -> 0 wrap. paletteFolded() (PaletteSample.h) does the lookup.
static inline uint16_t foldPhase16(uint16_t phase) {
    const uint16_t f = (uint16_t)(phase << 1);
    return (phase & 0x8000) ? (uint16_t)~f : f;
}

// A soft-edged bump: 65535 at dist 0, easing down to 0 at dist == width and
// beyond (smoothstep, so it is flat at the peak and lands on 0 without a
// corner). dist and width share any unit; a width of 0 gives 0. Never rises as
// dist grows.
static inline uint16_t smoothBump16(uint32_t dist, uint32_t width) {
    if (dist >= width) return 0;
    // x = 1 - dist/width in Q16 (0 < x <= 65536), then x^2 * (3 - 2x).
    const uint32_t x  = (uint32_t)(((uint64_t)(width - dist) << 16) / width);
    const uint64_t x2 = (uint64_t)x * x;                             // Q32
    const uint64_t v  = (x2 * (3u * 65536u - 2u * x)) >> 32;         // Q16, <= 65536
    return v > 65535 ? 65535 : (uint16_t)v;
}
