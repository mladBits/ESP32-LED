#pragma once

#include <Arduino.h>

enum AnimationDirection {
    Inward,
    Outward
};

class AnimationDirectionHelper {
public:
    // Map string to enum
    static AnimationDirection parse(const char* str) {
        if (strcmp(str, "Inward") == 0) return Inward;
        if (strcmp(str, "Outward") == 0) return Outward;
        return Outward; // default
    }
private:
    AnimationDirectionHelper() = delete; // prevent instantiation
};