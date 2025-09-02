#include "SolidColorState.h"

SolidColorState::SolidColorState() {}

void SolidColorState::update(CRGB* leds, int numLeds, CHSV color) {
    fill_solid(leds, numLeds,  color);
    FastLED.show();
}