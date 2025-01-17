#ifndef LED_STATE_H
#define LED_STATE_H

#include <FastLED.h>

class LEDState {
    public:
        virtual void update(CRGB* leds, int numLeds) = 0;
        virtual ~LEDState() {}
};

#endif