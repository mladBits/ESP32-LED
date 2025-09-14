#pragma once

#include "Animation.h"
#include "PaletteManager.h"
#include "PlasmaState.h"
#include "ColorWave.h"
#include "Pacifica.h"

class AnimationRegistry {
public:
    explicit AnimationRegistry(PaletteManager& pm);
    uint8_t getSize();
    Animation* const* list() const;
    Animation* getByName(const char* name);
    Animation* createByName(const char* name);
private:
    size_t count;
    static constexpr uint8_t capacity = 5;
    Animation* animations[capacity];

    PlasmaState plasma;
    ColorWave colorWave;
    Pacifica pacifica;
};