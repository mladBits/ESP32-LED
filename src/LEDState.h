#ifndef LED_STATE_H
#define LED_STATE_H

#include <FastLED.h>

class LEDState {
    protected:
        bool isPaletteStatic = false;
        int paletteId = 1;
    public:
        virtual void update(CRGB* leds, int numLeds) = 0;
        virtual void setStatic(bool isStatic) {
            isPaletteStatic = isStatic;
        }
        
        bool getStatic() {
            return isPaletteStatic;
        }

        virtual void setPaletteId(int c) {
            paletteId = c;
        }

        int getPaletteId() {
            return paletteId;
        }

        virtual ~LEDState() {}
};

#endif