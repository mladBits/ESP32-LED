#ifndef SOLID_COLOR_STATE_H
#define SOLID_COLOR_STATE_H

#include "LEDState.h"

class SolidColorState : public LEDState {
    private:
        CRGB color;

    public:
        SolidColorState(CRGB c);
        void setColor(CRGB c);
        void update(CRGB* leds, int numLeds) override;
};

#endif