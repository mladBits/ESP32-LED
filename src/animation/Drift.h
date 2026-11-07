#pragma once

#include "Animation.h"

class Drift : public Animation {
    void driftLinear(Strip& strip, uint16_t hue16, uint16_t brightTheta) const;
    void driftCenter(Strip& strip, uint16_t hue16, uint16_t brightTheta) const;
public:
    Drift() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "Drift"; }
};
