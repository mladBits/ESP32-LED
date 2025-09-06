#pragma once

#include <FastLED.h>
#include "animation/Animation.h"
#include "animation/AnimationRegistry.h"
#include "Strip.h"

class LEDController {
    private:
        static constexpr uint8_t MAX_STRIPS = 8;
        Strip strips[MAX_STRIPS];
        uint8_t numStrips = 0;
        bool isAnimated;
    public:
        LEDController();
        void addStrip(CRGB* ledsArray, int count);
        bool isAnimationActive();
        void isAnimationActive(bool toggle);
        void animate();
        void registerAnimation(AnimationRegistry* ar, const char* name);
        void applyHsv(uint8_t  h, uint8_t  s, uint8_t v);
        void updatePalette(CRGBPalette16 palette);

        bool isOn = false;
};