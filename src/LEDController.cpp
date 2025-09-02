#include "LEDController.h"

LEDController::LEDController() {
    colorState = new SolidColorState();
    isAnimated = false;
}

void LEDController::setState(LEDState* newState) {
    if (currentState) {
        delete currentState; // clean up previous state.
    }
    currentState = newState;
}

void LEDController::addStrip(CRGB* ledArray, int count) {
    if (numStrips < MAX_STRIPS) {
        CRGB* copy = new CRGB[count];      // allocate memory on heap
        memcpy(copy, ledArray, count * sizeof(CRGB));  // copy contents
        strips[numStrips++] = {copy, count};
    }
}

LEDState* LEDController::getState() {
    return currentState;
}

bool LEDController::isAnimationActive() {
    return isAnimated;
}

void LEDController::update(uint8_t  h, uint8_t  s, uint8_t v) {
    for (int i = 0; i < numStrips; i++) {
        colorState->update(strips[i].leds, strips[i].numLeds, CHSV(h, s, v));
    }
}

LEDController::~LEDController() {
    if (currentState) {
        delete currentState;
    }
}