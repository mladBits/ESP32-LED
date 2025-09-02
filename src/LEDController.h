#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <FastLED.h>
#include "LEDState.h"
#include "SolidColorState.h"

class LEDController {
    private:
        struct Strip {
            CRGB* leds;
            int numLeds;
        };

        static constexpr int MAX_STRIPS = 8;
        Strip strips[MAX_STRIPS];
        int numStrips = 0;

        LEDState* currentState = nullptr;
        SolidColorState* colorState;
        bool isAnimated;

    public:
        LEDController();
        void addStrip(CRGB* ledsArray, int count);
        void setState(LEDState* newState);
        bool isAnimationActive();
        LEDState* getState();
        void update(uint8_t  h, uint8_t  s, uint8_t v);
        ~LEDController();
};

#endif