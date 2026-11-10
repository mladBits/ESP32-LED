#include "AnimationRegistry.h"

AnimationRegistry::AnimationRegistry() :
colorWave(), pacifica(), drift(), lavaLamp(), cityLights(), aurora(), count(0) {
    animations[count++] = &colorWave;
    animations[count++] = &pacifica;
    animations[count++] = &drift;
    animations[count++] = &lavaLamp;
    animations[count++] = &cityLights;
    animations[count++] = &aurora;
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
