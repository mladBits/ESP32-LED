#include <unity.h>
#include "mqtt/HomeAssistantColor.h"

void setUp() {}
void tearDown() {}

// --- HA degrees/percent -> FastLED 0..255 ---

void hueDegToFast_bounds() {
    TEST_ASSERT_EQUAL_UINT8(0, haHueDegToFast(0));
    TEST_ASSERT_EQUAL_UINT8(255, haHueDegToFast(360));
}

void hueDegToFast_midpoint() {
    TEST_ASSERT_EQUAL_UINT8(128, haHueDegToFast(180));
}

void hueDegToFast_clampsOutOfRange() {
    TEST_ASSERT_EQUAL_UINT8(0, haHueDegToFast(-45));
    TEST_ASSERT_EQUAL_UINT8(255, haHueDegToFast(500));
}

void satPctToFast_bounds() {
    TEST_ASSERT_EQUAL_UINT8(0, haSatPctToFast(0));
    TEST_ASSERT_EQUAL_UINT8(255, haSatPctToFast(100));
}

void satPctToFast_midpoint() {
    TEST_ASSERT_EQUAL_UINT8(128, haSatPctToFast(50));
}

void satPctToFast_clampsOutOfRange() {
    TEST_ASSERT_EQUAL_UINT8(0, haSatPctToFast(-1));
    TEST_ASSERT_EQUAL_UINT8(255, haSatPctToFast(101));
}

// --- FastLED 0..255 -> HA degrees/percent ---

void fastHueToDeg_bounds() {
    TEST_ASSERT_EQUAL_INT(0, fastHueToHaDeg(0));
    TEST_ASSERT_EQUAL_INT(360, fastHueToHaDeg(255));
}

void fastSatToPct_bounds() {
    TEST_ASSERT_EQUAL_INT(0, fastSatToHaPct(0));
    TEST_ASSERT_EQUAL_INT(100, fastSatToHaPct(255));
}

// --- round trips: HA value -> FastLED -> HA must survive within rounding ---

void hueRoundTrip_within2Degrees() {
    for (int deg = 0; deg <= 360; deg++) {
        int rt = fastHueToHaDeg(haHueDegToFast(deg));
        TEST_ASSERT_INT_WITHIN_MESSAGE(2, deg, rt, "hue degree round trip");
    }
}

void satRoundTrip_within1Percent() {
    for (int pct = 0; pct <= 100; pct++) {
        int rt = fastSatToHaPct(haSatPctToFast(pct));
        TEST_ASSERT_INT_WITHIN_MESSAGE(1, pct, rt, "saturation percent round trip");
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(hueDegToFast_bounds);
    RUN_TEST(hueDegToFast_midpoint);
    RUN_TEST(hueDegToFast_clampsOutOfRange);
    RUN_TEST(satPctToFast_bounds);
    RUN_TEST(satPctToFast_midpoint);
    RUN_TEST(satPctToFast_clampsOutOfRange);
    RUN_TEST(fastHueToDeg_bounds);
    RUN_TEST(fastSatToPct_bounds);
    RUN_TEST(hueRoundTrip_within2Degrees);
    RUN_TEST(satRoundTrip_within1Percent);
    return UNITY_END();
}
