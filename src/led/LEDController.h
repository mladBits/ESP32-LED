#pragma once

#include <FastLED.h>
#include "animation/Animation.h"
#include "animation/AnimationRegistry.h"
#include "animation/AnimationDirection.h"
#include "led/Strip.h"

class LEDController {
    private:
        static constexpr uint8_t MAX_STRIPS = 8;
        Strip strips[MAX_STRIPS];
        uint8_t numStrips = 0;

    public:
        LEDController() = default;

        void addStrip(CRGB* ledsArray, int count);
        
        void clear();
        void setBrightness(uint8_t b);
        void applyHsv(uint8_t  h, uint8_t  s, uint8_t v);
        void animate();
        void show();

        // One-shot "power-down" frame, driven by MqttLight while shutting off.
        // Operates in place on whatever the strips last rendered: the lit
        // region retracts toward the centre behind a bright leading edge, then
        // a white core flashes and fades to black. progress runs 0..255 across
        // the whole shutdown.
        void shutdownCollapse(uint8_t progress);

        // One-shot "power-on" frame, the inverse of shutdownCollapse. Masks the
        // freshly-rendered content: a white core pops at the centre, then a
        // window opens outward behind a bright leading edge to reveal the live
        // frame. Must be applied *after* the normal render each tick. progress
        // runs 0..255 across the whole turn-on.
        void powerOnReveal(uint8_t progress);
        
        // animation control
        void setAnimation(const Animation* anim);
        void registerAnimation(const AnimationRegistry* ar, const char* name);
        
        // palette control
        // How long a palette change crossfades (immediate: no fade).
        static constexpr uint32_t PALETTE_FADE_MS = 3000;
        void updatePalette(const CRGBPalette16& palette, bool immediate = false);

        // direction control
        void updateDirection(AnimationDirection direction);

        uint8_t getNumStrips() const { return numStrips; }
        uint16_t getTotalLeds() const;

        // Estimated draw of all strips at the active brightness, using
        // FastLED's own power model (the same one enforcing the power cap).
        // Calibrated for 5V WS2812B; the 12V WALL strip is only approximate.
        uint32_t estimatePowerMw() const;

        bool isOn = false;
};