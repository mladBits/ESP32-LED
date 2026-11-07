#pragma once

#include "Animation.h"

// Sparse points that swell and fade in palette colours, like embers or
// fireflies, over lots of dark space. Position-agnostic, so it renders the same
// on every device (direction is ignored). Independent per strip via strip.id.
class Embers : public Animation {
    static constexpr uint8_t MAX_STRIPS = 8;
    static constexpr uint8_t MAX_EMBERS = 16;   // upper cap; count scales with length

    struct EmberState {
        uint32_t startMs;   // when this ember begins its swell
        uint16_t lifeMs;    // swell -> peak -> fade duration
        uint16_t pos;       // pixel index
        uint8_t  hue;       // palette index
    };
    mutable EmberState embers[MAX_STRIPS][MAX_EMBERS] = {};
public:
    Embers() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "Embers"; }
};
