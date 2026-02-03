#pragma once

#include "Animation.h"

class PlasmaState : public Animation {
    private:
        void plasma(Strip& strip) const;
    public:
        PlasmaState();
        void update(Strip& strip) const override;
        const char* getName() const override {
            return "Plasma";
        }
};
