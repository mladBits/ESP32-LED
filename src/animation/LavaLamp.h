#pragma once

#include "Animation.h"

// Slow Perlin-noise blobs in palette colours that morph and drift along the
// strip like a lava lamp: smooth, organic and never repeating. A pure function
// of the shared clock, so boards stay in step; each strip samples its own
// slice of the noise field, so peg's strips differ. Direction sets which way
// the blobs drift (Outward: away from the centre / towards the strip's end).
class LavaLamp : public Animation {
public:
    LavaLamp() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "LavaLamp"; }
};
