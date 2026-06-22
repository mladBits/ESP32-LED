#pragma once

#include "Animation.h"

class Drift : public Animation {
    mutable uint16_t sPseudotime = 0;
    mutable uint16_t sLastMillis = 0;
    mutable uint16_t sHue16 = 0;
    mutable uint32_t lastFrameMs = 0;

    void driftLinear(Strip& strip) const;
    void driftCenter(Strip& strip) const;
public:
    Drift() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "Drift"; }
};
