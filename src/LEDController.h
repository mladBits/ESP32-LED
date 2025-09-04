#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <FastLED.h>
#include "animation/Animation.h"

class LEDController {
    private:
        struct Strip {
            CRGB* leds;
            int numLeds;
        };

        static constexpr int MAX_STRIPS = 8;
        Strip strips[MAX_STRIPS];
        int numStrips = 0;
        bool isAnimated;
    public:
        LEDController();
        void addStrip(CRGB* ledsArray, int count);
        bool isAnimationActive();
        void isAnimationActive(bool toggle);
        void animate(Animation* animation);
        void apply_hsv(uint8_t  h, uint8_t  s, uint8_t v);
};

#endif