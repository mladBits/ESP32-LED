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

static inline int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Parses "#RRGGBB" (exactly 7 chars, case-insensitive hex digits).
// Returns false without touching the outputs on malformed input.
static inline bool parseHexColor(const char* s, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (!s || s[0] != '#') return false;
    for (int i = 1; i <= 6; i++) {
        if (hexNibble(s[i]) < 0) return false;
    }
    if (s[7] != '\0') return false;

    r = (uint8_t)((hexNibble(s[1]) << 4) | hexNibble(s[2]));
    g = (uint8_t)((hexNibble(s[3]) << 4) | hexNibble(s[4]));
    b = (uint8_t)((hexNibble(s[5]) << 4) | hexNibble(s[6]));
    return true;
}
