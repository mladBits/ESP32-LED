#pragma once

#include "Animation.h"

class CyberPulse : public Animation {
    public:
        void update(Strip& strip) const override;
        const char* getName() const override {
            return "Cyber Pulse";
        }
};