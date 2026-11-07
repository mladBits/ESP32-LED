#pragma once

#include "Animation.h"
#include "led/PaletteManager.h"
#include "PlasmaState.h"
#include "ColorWave.h"
#include "Pacifica.h"
#include "Drift.h"
#include "Beams.h"
#include "Ripple.h"
#include "Embers.h"

class AnimationRegistry {
public:
    explicit AnimationRegistry(PaletteManager& pm);

    uint8_t getSize() const;
    const Animation* const* list() const;

    Animation* getByName(const char* name) const;
private:
    size_t count;
    static constexpr uint8_t capacity = 7;
    Animation* animations[capacity];

    PlasmaState plasma;
    ColorWave colorWave;
    Pacifica pacifica;
    Drift drift;
    Beams beams;
    Ripple ripple;
    Embers embers;
};