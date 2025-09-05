#pragma once

#include "Animation.h"

class PlasmaState : public Animation {
    private:
        void plasma(Strip* strip);
    public:
        PlasmaState();
        void update(Strip* strip) override;
        const char* getName() const override {
            return "Plasma";
        }
};
