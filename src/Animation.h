#pragma once
#include <FastLED.h>

class Animation {
    private:
        bool isPaletteStatic = false;
    public:
        virtual void update(CRGB* leds, int numLeds) = 0;
        virtual const char* getName() const = 0;

        virtual void setStatic(bool isStatic) {
            isPaletteStatic = isStatic;
        }

        bool getStatic() {
            return isPaletteStatic;
        }

};