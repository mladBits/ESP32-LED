#pragma once

#include "Animation.h"

// Palette-coloured lights that fade on and off at random over a dim glow of
// the same palette, like a city skyline at night. Each light's timing comes
// from a hash of its position and the shared clock, so it needs no state;
// strips and boards are salted differently so none twinkle in lockstep.
// Position-agnostic: direction is ignored.
class CityLights : public Animation {
public:
    CityLights() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "CityLights"; }
};
