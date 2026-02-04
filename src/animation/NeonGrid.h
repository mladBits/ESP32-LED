#pragma once

#include "Animation.h"

class NeonGrid : public Animation {
    public:
        void update(Strip& strip) const override;
        const char* getName() const override {
            return "Neon Grid Breathing";
        }
};