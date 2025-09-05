#pragma once
#include <FastLED.h>
#include "animation/Animation.h"

class Animation;

struct Strip {
    uint8_t id;
    CRGB* leds;
    int numLeds;

    CRGBPalette16 currentPalette = RainbowColors_p;
    CRGBPalette16 targetPalette = RainbowColors_p;

    bool usePalette = false;
    bool animationGeneratesPalette = false;

    Animation* animation = nullptr; 

    Strip() : id(0), leds(nullptr), numLeds(0) {}

    Strip(
        uint8_t _id, 
        CRGB* _leds, 
        int _numLeds,
        const CRGBPalette16& _current = RainbowColors_p,
        const CRGBPalette16& _target = RainbowColors_p,
        bool _usePalette = false,
        bool _animationGeneratesPalette = false)
    : 
    id(_id), 
    leds(_leds),
    numLeds(_numLeds),
    currentPalette(_current),
    targetPalette(_target),
    usePalette(_usePalette),
    animationGeneratesPalette(_animationGeneratesPalette)
    {}
};