#include "led/LEDController.h"

void LEDController::addStrip(CRGB* ledArray, int count) {
    if (numStrips < MAX_STRIPS) {
        strips[numStrips] = Strip(numStrips, ledArray, count);
        strips[numStrips].paletteLastBlendMs = millis();
        numStrips++;
    }
}

void LEDController::setBrightness(uint8_t b) {
    FastLED.setBrightness(b);
}

void LEDController::clear() {
    for (int i = 0; i < numStrips; i++) {
        fill_solid(strips[i].leds, strips[i].numLeds, CRGB::Black);
    }
}

void LEDController::applyHsv(uint8_t  h, uint8_t  s, uint8_t v) {
    CHSV target(h, s, v);

    for (int i = 0; i < numStrips; i++) {
        fill_solid(strips[i].leds, strips[i].numLeds, target);
    }
}

void LEDController::animate() {
    for (int i = 0; i < numStrips; i++) {
        if (strips[i].animation) {
            strips[i].animation->update(strips[i]);
        }
    }
}

void LEDController::setAnimation(const Animation* anim) {
    const uint32_t now = millis();

    for (int i = 0; i < numStrips; i++) {
        strips[i].animation = anim;
        strips[i].paletteLastBlendMs = now;
    }
}

void LEDController::registerAnimation(const AnimationRegistry* ar, const char* name) {
    Animation* a = ar->getByName(name);
    if (!a) return;

    setAnimation(a);
}

void LEDController::updatePalette(const CRGBPalette16& palette, bool immediate) {
    const uint32_t now = millis();

    for (int i = 0; i < numStrips; i++) {
        strips[i].targetPalette = palette;

        if (immediate) {
            strips[i].currentPalette = palette;
        }

        strips[i].animationGeneratesPalette = false;
        strips[i].paletteLastBlendMs = now;        
    }
}

void LEDController::updateDirection(AnimationDirection direction) {
    for (int i = 0; i < numStrips; i++) {
        strips[i].direction = direction;
    }
}

void LEDController::show() { FastLED.show(); }