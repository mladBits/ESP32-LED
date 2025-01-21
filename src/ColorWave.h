#ifndef COLOR_WAVE_H
#define COLOR_WAVE_H

#include "LEDState.h"

class ColorWaveState : public LEDState {
    private:
        CRGBPalette16 currentPalette;
        CRGBPalette16 targetPalette;
    public:
        ColorWaveState();
        void setPalette(CRGBPalette16 c);
        void plasma(CRGB* leds, int numLeds);
        void update(CRGB* leds, int numLeds) override;
};

#endif