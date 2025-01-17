#include "LEDController.h"

LEDController::LEDController(CRGB* ledArray, int num) : leds(ledArray), numLeds(num) {}

void LEDController::setState(LEDState* newState) {
    if (currentState) {
        delete currentState; // clean up previous state.
    }
    currentState = newState;
}

void LEDController::update() {
    if (currentState) {
        currentState->update(leds, numLeds);
    }
}

LEDController::~LEDController() {
    if (currentState) {
        delete currentState;
    }
}