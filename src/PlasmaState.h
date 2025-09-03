#ifndef PLASMA_STATE_H
#define PLASMA_STATE_H

#include "Animation.h"

class PlasmaState : public Animation {
    private:
        CRGBPalette16 currentPalette;
        CRGBPalette16 targetPalette;
        void plasma(CRGB* leds, int numLeds);
    public:
        PlasmaState(CRGBPalette16 c, bool isStatic);
        void setPalette(CRGBPalette16 c);
        void update(CRGB* leds, int numLeds) override;
        const char* getName() const override;
};

#endif