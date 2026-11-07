#pragma once

#include "Animation.h"

class ColorWave : public Animation {
    private:
        void colorwaves(Strip& strip, uint16_t hue16, uint16_t pseudotime) const;
        void colorwavesCenter(Strip& strip, uint16_t hue16, uint16_t pseudotime) const;
    public:
        ColorWave() = default;

        void update(Strip& strip) const override;
        const char* getName() const override {
            return "ColorWave";
        }
};
