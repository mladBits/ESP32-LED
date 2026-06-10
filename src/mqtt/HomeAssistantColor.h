#pragma once
#include <Arduino.h>
#include <stdint.h>

static inline uint8_t clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static inline int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// HA hue degrees [0..360] -> FastLED hue [0..255], rounded
static inline uint8_t haHueDegToFast(int deg) {
    deg = clamp_int(deg, 0, 360);
    // rounding: +180 before /360
    return (uint8_t)((deg * 255L + 180L) / 360L);
}

// HA sat percent [0..100] -> FastLED sat [0..255], rounded
static inline uint8_t haSatPctToFast(int pct) {
    pct = clamp_int(pct, 0, 100);
    return (uint8_t)((pct * 255L + 50L) / 100L);
}

// FastLED hue [0..255] -> HA degrees [0..360], rounded
static inline int fastHueToHaDeg(uint8_t h) {
    return (int)((h * 360L + 127L) / 255L);
}

// FastLED sat [0..255] -> HA percent [0..100], rounded
static inline int fastSatToHaPct(uint8_t s) {
    return (int)((s * 100L + 127L) / 255L);
}