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

void hexColor_parsesUppercase() {
    uint8_t r, g, b;
    TEST_ASSERT_TRUE(parseHexColor("#1A2B3C", r, g, b));
    TEST_ASSERT_EQUAL_UINT8(0x1A, r);
    TEST_ASSERT_EQUAL_UINT8(0x2B, g);
    TEST_ASSERT_EQUAL_UINT8(0x3C, b);
}

void hexColor_parsesLowercase() {
    uint8_t r, g, b;
    TEST_ASSERT_TRUE(parseHexColor("#ff00aa", r, g, b));
    TEST_ASSERT_EQUAL_UINT8(0xFF, r);
    TEST_ASSERT_EQUAL_UINT8(0x00, g);
    TEST_ASSERT_EQUAL_UINT8(0xAA, b);
}

void hexColor_parsesBounds() {
    uint8_t r, g, b;
    TEST_ASSERT_TRUE(parseHexColor("#000000", r, g, b));
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);
    TEST_ASSERT_TRUE(parseHexColor("#FFFFFF", r, g, b));
    TEST_ASSERT_EQUAL_UINT8(255, r);
    TEST_ASSERT_EQUAL_UINT8(255, g);
    TEST_ASSERT_EQUAL_UINT8(255, b);
}

void hexColor_rejectsMalformed() {
    uint8_t r = 1, g = 2, b = 3;
    TEST_ASSERT_FALSE(parseHexColor(nullptr, r, g, b));
    TEST_ASSERT_FALSE(parseHexColor("", r, g, b));
    TEST_ASSERT_FALSE(parseHexColor("FF00AA", r, g, b));    // missing '#'
    TEST_ASSERT_FALSE(parseHexColor("#FFF", r, g, b));      // too short
    TEST_ASSERT_FALSE(parseHexColor("#FF00AABB", r, g, b)); // too long
    TEST_ASSERT_FALSE(parseHexColor("#GG00AA", r, g, b));   // non-hex chars
    // outputs untouched on failure
    TEST_ASSERT_EQUAL_UINT8(1, r);
    TEST_ASSERT_EQUAL_UINT8(2, g);
    TEST_ASSERT_EQUAL_UINT8(3, b);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(tZero_returnsStart);
    RUN_TEST(tFull_reachesTarget);
    RUN_TEST(equalHues_stayPut);
    RUN_TEST(wrapsForwardAcrossZero);
    RUN_TEST(wrapsBackwardAcrossZero);
    RUN_TEST(wrappingBlend_neverTakesLongPath);
    RUN_TEST(hexColor_parsesUppercase);
    RUN_TEST(hexColor_parsesLowercase);
    RUN_TEST(hexColor_parsesBounds);
    RUN_TEST(hexColor_rejectsMalformed);
    return UNITY_END();
}
