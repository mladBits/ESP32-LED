#pragma once
#include <FastLED.h>
#include "Strip.h"
#include "AnimationDirection.h"

class Animation {
    public:
        virtual void update(Strip* strip) = 0;
        virtual const char* getName() const = 0;
        virtual void setDirection(AnimationDirection dir) { direction = dir; }
    protected:
        AnimationDirection direction = Outward;
};