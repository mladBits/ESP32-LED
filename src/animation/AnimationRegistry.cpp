#include "AnimationRegistry.h"

AnimationRegistry::AnimationRegistry(PaletteManager& pm) :
plasma(), colorWave(), pacifica(), drift(), count(0) {
    animations[count++] = &plasma;
    animations[count++] = &colorWave;
    animations[count++] = &pacifica;
    animations[count++] = &drift;
}

Animation* AnimationRegistry::getByName(const char* name) const {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(animations[i]->getName(), name) == 0) {
            return animations[i];
        }
    }
    return nullptr;
}

uint8_t AnimationRegistry::getSize() const {
    return (uint8_t)count;
}

const Animation* const* AnimationRegistry::list() const {
    return animations;
}
