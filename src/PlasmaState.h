#ifndef PLASMA_STATE_H
#define PLASMA_STATE_H

#include "LEDState.h"

class PlasmaState : public LEDState {
    private:
        CRGBPalette16 currentPalette;
        CRGBPalette16 targetPalette;

    public:
        PlasmaState(CRGBPalette16 c);
        void plasma(CRGB* leds, int numLeds);
        void update(CRGB* leds, int numLeds) override;
};

#endif