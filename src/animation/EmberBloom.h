#pragma once

#include "Animation.h"

class EmberBloom : public Animation {
    public:
        void update(Strip& strip) const override;
        const char* getName() const override {
            return "Ember Bloom";
        }
};