#pragma once

#include "Animation.h"
#include "PaletteManager.h"
#include "PlasmaState.h"
#include "ColorWave.h"
#include "Pacifica.h"
#include "CyberPulse.h"
#include "NeonGrid.h"
#include "EmberBloom.h"

class AnimationRegistry {
public:
    explicit AnimationRegistry(PaletteManager& pm);

    uint8_t getSize() const;
    const Animation* const* list() const;

    Animation* getByName(const char* name) const;
private:
    size_t count;
    static constexpr uint8_t capacity = 6;
    Animation* animations[capacity];

    PlasmaState plasma;
    ColorWave colorWave;
    Pacifica pacifica;
    CyberPulse cyberPulse;
    NeonGrid neonGrid;
    EmberBloom emberBloom;
};