#include "LEDController.h"

LEDController::LEDController() {
    isAnimated = false;
}

void LEDController::addStrip(CRGB* ledArray, int count) {
    if (numStrips < MAX_STRIPS) {
        strips[numStrips++] = {ledArray, count};
    }
}

bool LEDController::isAnimationActive() {
    return isAnimated;
}

void LEDController::isAnimationActive(bool toggle) {
    this->isAnimated = toggle;
}

void LEDController::apply_hsv(uint8_t  h, uint8_t  s, uint8_t v) {
    CHSV target(h, s, v);

    for (int i = 0; i < numStrips; i++) {
        //nblend(strips[i].hsv, target, 12);
        fill_solid(strips[i].leds, strips[i].numLeds, target);
    }
    FastLED.show();
}

void LEDController::animate(Animation* animation) {
    for (int i = 0; i < numStrips; i++) {
        animation->update(strips[i].leds, strips[i].numLeds);
    }
    FastLED.show();
}
