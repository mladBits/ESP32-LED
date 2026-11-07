#pragma once

#include "Animation.h"

// Gentle pulses that well up and expand outward, fading as they travel.
// Each strip runs its own independent ripples (keyed by strip.id) so peg's
// three strips ripple organically rather than in lockstep.
class Ripple : public Animation {
    static constexpr uint8_t MAX_STRIPS  = 8;
    static constexpr uint8_t NUM_RIPPLES = 2;

    struct RippleState {
        uint32_t startMs;   // when this ripple becomes visible
        uint16_t lifeMs;    // how long it takes to cross + fade
        uint8_t  hue;       // palette index
        uint8_t  origin;    // origin index (WALL linear only; center uses mid)
    };
    mutable RippleState ripples[MAX_STRIPS][NUM_RIPPLES] = {};

    void render(Strip& strip, uint32_t now) const;
public:
    Ripple() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "Ripple"; }
};
