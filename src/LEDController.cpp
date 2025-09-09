#include "LEDController.h"

LEDController::LEDController() {
    isAnimated = false;
}

void LEDController::addStrip(CRGB* ledArray, int count) {
    if (numStrips < MAX_STRIPS) {
        strips[numStrips] = Strip(numStrips, ledArray, count);
        numStrips++;
    }
}

bool LEDController::isAnimationActive() {
    return isAnimated;
}

void LEDController::isAnimationActive(bool toggle) {
    this->isAnimated = toggle;
}

void LEDController::applyHsv(uint8_t  h, uint8_t  s, uint8_t v) {
    CHSV target(h, s, v);

    for (int i = 0; i < numStrips; i++) {
        fill_solid(strips[i].leds, strips[i].numLeds, target);
    }
    FastLED.show();
}

void LEDController::animate() {
    for (int i = 0; i < numStrips; i++) {
        strips[i].animation->update(&strips[i]);
    }
    FastLED.show();
}

void LEDController::registerAnimation(AnimationRegistry* ar, const char* name) {
    for (int i = 0; i < numStrips; i++) {
        strips[i].animation = ar->createByName(name);
    }
}

void LEDController::updatePalette(CRGBPalette16 palette) {
    for (int i = 0; i < numStrips; i++) {
        strips[i].usePalette = true;
        strips[i].targetPalette = palette;
        strips[i].animationGeneratesPalette = false;
    }
}

void LEDController::updateDirection(AnimationDirection direction) {
    for (int i = 0; i < numStrips; i++) {
        strips[i].animation->setDirection(direction);
    }
}