#pragma once

#include <FastLED.h>
#include "animation/Animation.h"
#include "animation/AnimationRegistry.h"
#include "animation/AnimationDirection.h"
#include "led/Strip.h"

class LEDController {
    private:
        static constexpr uint8_t MAX_STRIPS = 8;
        Strip strips[MAX_STRIPS];
        uint8_t numStrips = 0;

    public:
        LEDController() = default;

        void addStrip(CRGB* ledsArray, int count);
        
        void clear();
        void setBrightness(uint8_t b);
        void applyHsv(uint8_t  h, uint8_t  s, uint8_t v);
        void animate();
        void show();
        
        // animation control
        void setAnimation(const Animation* anim);
        void registerAnimation(const AnimationRegistry* ar, const char* name);
        
        // palette control
        void updatePalette(const CRGBPalette16& palette, bool immediate = false);

        // direction control
        void updateDirection(AnimationDirection direction);

        uint8_t getNumStrips() const { return numStrips; }
        
        bool isOn = false;
};