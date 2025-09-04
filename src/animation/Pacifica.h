#pragma once

#include "Animation.h"

class Pacifica : public Animation {
    private:
        CRGBPalette16 currentPalette;
        CRGBPalette16 targetPalette;
    public:
        Pacifica();
        void setPalette(CRGBPalette16 c);
        void update(CRGB* leds, int numLeds) override;
        const char* getName() const override {
            return "Pacifica";
        }
};