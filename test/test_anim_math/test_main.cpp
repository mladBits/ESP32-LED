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

// Drift's slow hue: 1..2 units/ms at 120/256 BPM.
void driftHue_tracksAccumulator() {
    tracksAccumulator(120, 1 * 256, 2 * 256, 123456789u, 4);
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

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(colorWavePhase_tracksAccumulator);
    RUN_TEST(driftHue_tracksAccumulator);
    RUN_TEST(wobble_tracksAccumulatorAcrossWrap);
    RUN_TEST(continuousAtClockWrap);
    return UNITY_END();
}
