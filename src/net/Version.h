#pragma once

#include <stdint.h>
#include <stdlib.h>

// Firmware versions as stamped by scripts/fw_version.py (`git describe`):
//   "v1.2.0"              a release
//   "v1.2.0-3-gabc1234"   a build 3 commits past v1.2.0 (local, unreleased)
//   "...-dirty"           with uncommitted changes
//   anything else         e.g. "abc1234", no release tag reachable
struct FwVersion {
    bool     tagged;   // starts with vMAJOR.MINOR.PATCH
    bool     release;  // exactly vMAJOR.MINOR.PATCH, nothing after
    uint32_t major, minor, patch;
};

static inline bool parseVersionNumber(const char*& p, uint32_t& out) {
    if (*p < '0' || *p > '9') return false;
    char* end;
    out = (uint32_t)strtoul(p, &end, 10);
    p = end;
    return true;
}

static inline FwVersion parseFwVersion(const char* s) {
    FwVersion v = {};
    if (!s || *s != 'v') return v;
    const char* p = s + 1;
    if (!parseVersionNumber(p, v.major) || *p++ != '.' ||
        !parseVersionNumber(p, v.minor) || *p++ != '.' ||
        !parseVersionNumber(p, v.patch)) {
        return FwVersion{};
    }
    v.tagged = true;
    v.release = (*p == '\0');
    return v;
}

// Whether `candidate`, a published release, should replace the running
// `installed` build. Only plain releases qualify. A build past vX.Y.Z ranks at
// or above it, so a local build is never offered the release it came after;
// an untagged build ranks below every release.
static inline bool isNewerRelease(const char* candidate, const char* installed) {
    const FwVersion c = parseFwVersion(candidate);
    if (!c.release) return false;
    const FwVersion i = parseFwVersion(installed);
    if (!i.tagged) return true;
    if (c.major != i.major) return c.major > i.major;
    if (c.minor != i.minor) return c.minor > i.minor;
    return c.patch > i.patch;
}
