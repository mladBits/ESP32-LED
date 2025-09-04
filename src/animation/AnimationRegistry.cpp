#include "AnimationRegistry.h"

AnimationRegistry::AnimationRegistry(PaletteManager& pm) :
plasma(pm.getPaletteById(1), false),
colorWave(),
pacifica(),
count(0) {
    animations[count++] = &plasma;
    animations[count++] = &colorWave;
    animations[count++] = &pacifica;
}

Animation* AnimationRegistry::getByName(const char* name) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(animations[i]->getName(), name) == 0) {
            return animations[i];
        }
    }
    return nullptr;
}

uint8_t AnimationRegistry::getSize() {
    return count;
}

Animation* const* AnimationRegistry::list() const {
    return animations;
}
