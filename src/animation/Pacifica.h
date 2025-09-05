#pragma once

#include "Animation.h"

class Pacifica : public Animation {
    public:
        Pacifica();
        void update(Strip* strip) override;
        const char* getName() const override {
            return "Pacifica";
        }
};