#pragma once
#include <FastLED.h>
#include "Strip.h"
#include "AnimationDirection.h"

class Animation {
    public:
        virtual void update(Strip& strip) const = 0;
        virtual ~Animation() = default;
        virtual const char* getName() const = 0;
};