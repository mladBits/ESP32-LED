#include <unity.h>
#include "led/ColorMath.h"

void setUp() {}
void tearDown() {}

void tZero_returnsStart() {
    TEST_ASSERT_EQUAL_UINT8(10, lerpHueShortest(10, 120, 0));
}

void tFull_reachesTarget() {
    // >>8 fixed-point math may undershoot by 1
    TEST_ASSERT_UINT8_WITHIN(1, 120, lerpHueShortest(10, 120, 255));
}

void equalHues_stayPut() {
    TEST_ASSERT_EQUAL_UINT8(77, lerpHueShortest(77, 77, 0));
    TEST_ASSERT_EQUAL_UINT8(77, lerpHueShortest(77, 77, 128));
    TEST_ASSERT_EQUAL_UINT8(77, lerpHueShortest(77, 77, 255));
}

// 250 -> 5 must blend forward through the 255/0 wrap (distance 11),
// not backwards across the whole spectrum (distance 245).
void wrapsForwardAcrossZero() {
    TEST_ASSERT_EQUAL_UINT8(255, lerpHueShortest(250, 5, 128)); // halfway: at the wrap point
    TEST_ASSERT_UINT8_WITHIN(1, 5, lerpHueShortest(250, 5, 255));
}

// 5 -> 250 must blend backwards through the 0/255 wrap.
void wrapsBackwardAcrossZero() {
    TEST_ASSERT_EQUAL_UINT8(255, lerpHueShortest(5, 250, 128)); // halfway: wrapped below 0
    TEST_ASSERT_UINT8_WITHIN(1, 250, lerpHueShortest(5, 250, 255));
}

// every intermediate step of a wrapping blend stays inside the short arc
void wrappingBlend_neverTakesLongPath() {
    for (int t = 0; t <= 255; t++) {
        uint8_t v = lerpHueShortest(250, 5, (uint8_t)t);
        TEST_ASSERT_TRUE_MESSAGE(v >= 250 || v <= 5, "left the short arc 250..5");
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(tZero_returnsStart);
    RUN_TEST(tFull_reachesTarget);
    RUN_TEST(equalHues_stayPut);
    RUN_TEST(wrapsForwardAcrossZero);
    RUN_TEST(wrapsBackwardAcrossZero);
    RUN_TEST(wrappingBlend_neverTakesLongPath);
    return UNITY_END();
}
