#include <unity.h>
#include "net/Version.h"

void setUp() {}
void tearDown() {}

void parsesRelease() {
    FwVersion v = parseFwVersion("v1.12.3");
    TEST_ASSERT_TRUE(v.tagged);
    TEST_ASSERT_TRUE(v.release);
    TEST_ASSERT_EQUAL_UINT32(1, v.major);
    TEST_ASSERT_EQUAL_UINT32(12, v.minor);
    TEST_ASSERT_EQUAL_UINT32(3, v.patch);
}

void localBuildsAreTaggedNotReleases() {
    TEST_ASSERT_TRUE(parseFwVersion("v1.0.0-3-gabc1234").tagged);
    TEST_ASSERT_FALSE(parseFwVersion("v1.0.0-3-gabc1234").release);
    TEST_ASSERT_FALSE(parseFwVersion("v1.0.0-dirty").release);
}

void untaggedAndGarbageAreNotTagged() {
    TEST_ASSERT_FALSE(parseFwVersion("96105fa").tagged);
    TEST_ASSERT_FALSE(parseFwVersion("unknown").tagged);
    TEST_ASSERT_FALSE(parseFwVersion("v1.2").tagged);
    TEST_ASSERT_FALSE(parseFwVersion("vx.1.2").tagged);
    TEST_ASSERT_FALSE(parseFwVersion("").tagged);
    TEST_ASSERT_FALSE(parseFwVersion(nullptr).tagged);
}

void newerReleasesAreOffered() {
    TEST_ASSERT_TRUE(isNewerRelease("v1.0.1", "v1.0.0"));
    TEST_ASSERT_TRUE(isNewerRelease("v1.1.0", "v1.0.9"));
    TEST_ASSERT_TRUE(isNewerRelease("v2.0.0", "v1.9.9"));
    TEST_ASSERT_TRUE(isNewerRelease("v1.10.0", "v1.9.0"));   // numeric, not string, order
}

void sameOrOlderReleasesAreNot() {
    TEST_ASSERT_FALSE(isNewerRelease("v1.0.0", "v1.0.0"));
    TEST_ASSERT_FALSE(isNewerRelease("v1.0.0", "v1.0.1"));
    TEST_ASSERT_FALSE(isNewerRelease("v1.9.0", "v1.10.0"));
}

// A local build past v1.0.0 must not be "updated" back to v1.0.0, but the
// next release is newer.
void localBuildsRankAtOrAboveTheirTag() {
    TEST_ASSERT_FALSE(isNewerRelease("v1.0.0", "v1.0.0-5-g96105fa"));
    TEST_ASSERT_FALSE(isNewerRelease("v1.0.0", "v1.0.0-5-g96105fa-dirty"));
    TEST_ASSERT_TRUE(isNewerRelease("v1.0.1", "v1.0.0-5-g96105fa"));
}

void untaggedBuildsAreOfferedAnyRelease() {
    TEST_ASSERT_TRUE(isNewerRelease("v1.0.0", "b3655d5"));
    TEST_ASSERT_TRUE(isNewerRelease("v1.0.0", "unknown"));
}

// Only plain releases can be installed, even if they'd rank higher.
void onlyPlainReleasesAreCandidates() {
    TEST_ASSERT_FALSE(isNewerRelease("v2.0.0-1-gdeadbee", "v1.0.0"));
    TEST_ASSERT_FALSE(isNewerRelease("v2.0.0-rc1", "v1.0.0"));
    TEST_ASSERT_FALSE(isNewerRelease("2.0.0", "v1.0.0"));
    TEST_ASSERT_FALSE(isNewerRelease(nullptr, "v1.0.0"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(parsesRelease);
    RUN_TEST(localBuildsAreTaggedNotReleases);
    RUN_TEST(untaggedAndGarbageAreNotTagged);
    RUN_TEST(newerReleasesAreOffered);
    RUN_TEST(sameOrOlderReleasesAreNot);
    RUN_TEST(localBuildsRankAtOrAboveTheirTag);
    RUN_TEST(untaggedBuildsAreOfferedAnyRelease);
    RUN_TEST(onlyPlainReleasesAreCandidates);
    return UNITY_END();
}
