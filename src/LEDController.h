#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <FastLED.h>
#include "LEDState.h"

class LEDController {
    private:
        LEDState* currentState = nullptr;
        CRGB* leds;
        int numLeds;

    public:
        LEDController(CRGB* ledArray, int num);
        void setState(LEDState* newState);
        LEDState* getState();
        void update();
        ~LEDController();
};

#endif