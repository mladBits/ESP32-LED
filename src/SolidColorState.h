#ifndef SOLID_COLOR_STATE_H
#define SOLID_COLOR_STATE_H

#include "LEDState.h"

class SolidColorState {
    public:
        SolidColorState();
        void update(CRGB* leds, int numLeds, CHSV c);
};

#endif