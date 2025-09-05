#pragma once

#include "Animation.h"

class ColorWave : public Animation {
    private:
        uint16_t sPseudotime = 0;
        uint16_t sLastMillis = 0;
        uint16_t sHue16 = 0;
        uint32_t lastBlend = 0;
        void colorwaves(Strip* strip);
        void colorwavesCenter(Strip* strip);
    public:
        ColorWave();
        void update(Strip* strip) override;
        const char* getName() const override {
            return "ColorWave";
        }
};
