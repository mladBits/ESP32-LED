#pragma once

#include "Animation.h"

// A few broad, soft curtains of palette colour that slowly wander, breathe and
// fade over a dim glow, with a faint shimmer of rays inside them, like an
// aurora. Mostly dark and slow: an evening effect. A pure function of the
// shared clock, so boards stay in step; each strip gets its own curtain phases
// and rays, so peg's strips differ. Direction sets which way the rays ripple
// (Outward: away from the centre / towards the strip's end).
class Aurora : public Animation {
public:
    Aurora() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "Aurora"; }
};
