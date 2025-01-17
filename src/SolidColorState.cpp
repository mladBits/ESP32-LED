#include "SolidColorState.h"

SolidColorState::SolidColorState(CRGB c) : color(c) {}

void SolidColorState::setColor(CRGB c) {
    color = c;
}

void SolidColorState::update(CRGB* leds, int numLeds) {
    fill_solid(leds, numLeds,  color);
    FastLED.show();
}