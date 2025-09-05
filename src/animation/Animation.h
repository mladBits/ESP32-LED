#pragma once
#include <FastLED.h>
#include "Strip.h"

class Animation {
    public:
        virtual void update(Strip* strip) = 0;
        virtual const char* getName() const = 0;
};