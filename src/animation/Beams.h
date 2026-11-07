#pragma once

#include "Animation.h"

// Slow, palette-colored pulses that travel along the strip and gently pulsate.
// Positions derive from a single shared time accumulator (advanced once per
// frame) so all of a device's strips stay coherent — no per-strip state.
class Beams : public Animation {
    mutable uint16_t sPos16     = 0;   // travel phase, wraps 0..65535
    mutable uint16_t sLastMillis = 0;
    mutable uint32_t lastFrameMs = 0;

    // Colour at axis coordinate q (0..axisLen-1) summed over all beams.
    CRGB beamColorAt(const CRGBPalette16& pal, int q, int axisLen) const;

    void beamsLinear(Strip& strip) const;
    void beamsCenter(Strip& strip) const;
public:
    Beams() = default;
    void update(Strip& strip) const override;
    const char* getName() const override { return "Beams"; }
};
