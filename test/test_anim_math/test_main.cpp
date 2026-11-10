#include <unity.h>
#include <math.h>
#include "animation/AnimMath.h"

void setUp() {}
void tearDown() {}

// Signed distance a - b around the 16-bit circle.
static int circDiff(uint16_t a, uint16_t b) {
    return (int16_t)(uint16_t)(a - b);
}

// The speed beatsinIntegral16 integrates: beatsin88's sine, in double precision.
static double speedAt(uint16_t bpm88, int32_t lo8, int32_t hi8, uint32_t t) {
    const double turn  = 6.283185307179586 / 4294967296.0;
    const uint32_t phase = t * ((uint32_t)bpm88 * 280u);
    const double mid = (lo8 + hi8) / 512.0;
    const double amp = (hi8 - lo8) / 512.0;
    return mid + amp * sin(phase * turn);
}

// Runs the frame-by-frame accumulator the animations used to keep (1 ms frames)
// for five minutes from t0 and checks the closed form tracks it throughout.
static void tracksAccumulator(uint16_t bpm88, int32_t lo8, int32_t hi8, uint32_t t0, int tolerance) {
    const uint16_t start = beatsinIntegral16(bpm88, lo8, hi8, t0);
    double acc = 0;
    uint32_t t = t0;
    for (int ms = 1; ms <= 300000; ms++) {
        acc += speedAt(bpm88, lo8, hi8, t);
        t++;  // wraps naturally at 2^32
        if (ms % 997 == 0) {
            const uint16_t expected = (uint16_t)(start + (int64_t)llround(acc));
            const int diff = circDiff(beatsinIntegral16(bpm88, lo8, hi8, t), expected);
            TEST_ASSERT_INT_WITHIN(tolerance, 0, diff);
        }
    }
}

// ColorWave's wave phase: 23..60 units/ms at 147/256 BPM, at a realistic clock.
void colorWavePhase_tracksAccumulator() {
    tracksAccumulator(147, 23 * 256, 60 * 256, 1700000000u, 24);
}

// Drift's slow hue: 0.5..1 units/ms at 120/256 BPM.
void driftHue_tracksAccumulator() {
    tracksAccumulator(120, 128, 256, 123456789u, 4);
}

// foldPhase16 covers 0..65535 and back, and never jumps: neighbouring phases
// (including 65535 -> 0) differ by at most two.
void foldPhase16_isASeamlessTriangle() {
    TEST_ASSERT_EQUAL_UINT16(0,      foldPhase16(0));
    TEST_ASSERT_EQUAL_UINT16(0xFFFE, foldPhase16(0x7FFF));
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, foldPhase16(0x8000));
    TEST_ASSERT_EQUAL_UINT16(1,      foldPhase16(0xFFFF));
    uint16_t lo = 0xFFFF, hi = 0;
    for (uint32_t p = 0; p <= 0xFFFF; p++) {
        const uint16_t v = foldPhase16((uint16_t)p);
        const uint16_t next = foldPhase16((uint16_t)(p + 1));   // p = 0xFFFF wraps to 0
        TEST_ASSERT_INT_WITHIN(2, v, next);
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    TEST_ASSERT_EQUAL_UINT16(0, lo);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, hi);
}

// Pacifica's surge wobble: mean-zero speed, run straight across the 2^32 wrap.
void wobble_tracksAccumulatorAcrossWrap() {
    tracksAccumulator(768, -518, 518, 0xFFFFFFFFu - 150000u, 4);
}

// The step from t = 2^32-1 to t = 0 must be one ordinary millisecond.
void continuousAtClockWrap() {
    const uint16_t before = beatsinIntegral16(147, 23 * 256, 60 * 256, 0xFFFFFFFFu);
    const uint16_t after  = beatsinIntegral16(147, 23 * 256, 60 * 256, 0u);
    const int step = circDiff(after, before);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(22, step);   // speed is 23..60 units/ms
    TEST_ASSERT_LESS_OR_EQUAL_INT(61, step);
}

// smoothBump16 peaks at dist 0, is half-way at half the width and is 0 from the
// width outwards (and for a width of 0).
void smoothBump16_peakMidpointAndEdge() {
    TEST_ASSERT_EQUAL_UINT16(65535, smoothBump16(0, 1000));
    TEST_ASSERT_EQUAL_UINT16(32768, smoothBump16(500, 1000));
    TEST_ASSERT_EQUAL_UINT16(0,     smoothBump16(1000, 1000));
    TEST_ASSERT_EQUAL_UINT16(0,     smoothBump16(5000, 1000));
    TEST_ASSERT_EQUAL_UINT16(0,     smoothBump16(0, 0));
    TEST_ASSERT_EQUAL_UINT16(0,     smoothBump16(7, 0));
    // Widths far beyond a strip's (in 1/256 LED) don't overflow.
    TEST_ASSERT_EQUAL_UINT16(65535, smoothBump16(0, 0xFFFFFFFFu));
    TEST_ASSERT_EQUAL_UINT16(0,     smoothBump16(0xFFFFFFFFu, 0xFFFFFFFFu));
}

// It only ever falls as dist grows, and gently: no step bigger than the
// smoothstep's steepest slope (1.5 / width) allows.
void smoothBump16_fallsSmoothly() {
    const uint32_t width = 4000;   // ~15 LEDs in 1/256 LED units
    uint16_t prev = smoothBump16(0, width);
    for (uint32_t d = 1; d <= width + 10; d++) {
        const uint16_t v = smoothBump16(d, width);
        TEST_ASSERT_LESS_OR_EQUAL_UINT16(prev, v);
        TEST_ASSERT_LESS_OR_EQUAL_INT(26, prev - v);   // 1.5 * 65536 / 4000 = 24.6
        prev = v;
    }
    TEST_ASSERT_EQUAL_UINT16(0, prev);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(smoothBump16_peakMidpointAndEdge);
    RUN_TEST(smoothBump16_fallsSmoothly);
    RUN_TEST(colorWavePhase_tracksAccumulator);
    RUN_TEST(driftHue_tracksAccumulator);
    RUN_TEST(foldPhase16_isASeamlessTriangle);
    RUN_TEST(wobble_tracksAccumulatorAcrossWrap);
    RUN_TEST(continuousAtClockWrap);
    return UNITY_END();
}
