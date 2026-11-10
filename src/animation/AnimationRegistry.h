#pragma once

#include "Animation.h"
#include "ColorWave.h"
#include "Pacifica.h"
#include "Drift.h"
#include "LavaLamp.h"
#include "CityLights.h"
#include "Aurora.h"

class AnimationRegistry {
public:
    AnimationRegistry();

    uint8_t getSize() const;
    const Animation* const* list() const;

    Animation* getByName(const char* name) const;
private:
    size_t count;
    static constexpr uint8_t capacity = 6;
    Animation* animations[capacity];

    ColorWave colorWave;
    Pacifica pacifica;
    Drift drift;
    LavaLamp lavaLamp;
    CityLights cityLights;
    Aurora aurora;
};