#pragma once

#include "Animation.h"

// Slow, palette-colored pulses that travel along the strip and gently pulsate.
// Beam positions are a pure function of the shared clock (AnimClock), so all
// strips and boards stay coherent — no per-strip state.
class Beams : public Animation {
    // Colour at axis coordinate q (0..axisLen-1) summed over all beams, for
    // travel phase pos16 (wraps 0..65535).
    CRGB beamColorAt(const CRGBPalette16& pal, int q, int axisLen, uint16_t pos16) const;

    void beamsLinear(Strip& strip, uint16_t pos16) const;
    void beamsCenter(Strip& strip, uint16_t pos16) const;
public:
    Beams() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "Beams"; }
};
