#include <unity.h>
#include "animation/AnimationDirection.h"

void setUp() {}
void tearDown() {}

void parsesInward() {
    TEST_ASSERT_EQUAL(Inward, AnimationDirectionHelper::parse("Inward"));
}

void parsesOutward() {
    TEST_ASSERT_EQUAL(Outward, AnimationDirectionHelper::parse("Outward"));
}

void unknownDefaultsToOutward() {
    TEST_ASSERT_EQUAL(Outward, AnimationDirectionHelper::parse("sideways"));
    TEST_ASSERT_EQUAL(Outward, AnimationDirectionHelper::parse(""));
}

void parseIsCaseSensitive() {
    // MQTT payloads must match exactly; "inward" is not a valid direction
    TEST_ASSERT_EQUAL(Outward, AnimationDirectionHelper::parse("inward"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(parsesInward);
    RUN_TEST(parsesOutward);
    RUN_TEST(unknownDefaultsToOutward);
    RUN_TEST(parseIsCaseSensitive);
    return UNITY_END();
}
