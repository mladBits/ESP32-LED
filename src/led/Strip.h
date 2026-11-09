#pragma once
#include <FastLED.h>
#include "animation/AnimationDirection.h"
#include "animation/Animation.h"

class Animation;

struct Strip {
    uint8_t id;
    CRGB* leds = nullptr;
    int numLeds;

    CRGBPalette16 currentPalette = RainbowColors_p;
    CRGBPalette16 targetPalette = RainbowColors_p;

    AnimationDirection direction = Outward;
    const Animation* animation = nullptr; 

    // Timed crossfade from fromPalette to targetPalette (LEDController).
    CRGBPalette16 fromPalette = RainbowColors_p;
    uint32_t paletteFadeStartMs = 0;
    bool paletteFading = false;

    Strip() = default;

    Strip(
        uint8_t _id, 
        CRGB* _leds, 
        int _numLeds,
        const CRGBPalette16& _current = RainbowColors_p,
        const CRGBPalette16& _target = RainbowColors_p,
        AnimationDirection _direction = Outward
    )
    :
    id(_id),
    leds(_leds),
    numLeds(_numLeds),
    currentPalette(_current),
    targetPalette(_target),
    direction(_direction)
    {}
};