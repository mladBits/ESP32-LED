#pragma once

#include "Animation.h"

class ColorWave : public Animation {
    private:
        CRGBPalette16 currentPalette;
        CRGBPalette16 targetPalette;
    public:
        ColorWave();
        void setPalette(CRGBPalette16 c);
        void update(CRGB* leds, int numLeds) override;
        const char* getName() const override;
};
