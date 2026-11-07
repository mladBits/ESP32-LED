#include "PlasmaState.h"
#include "AnimClock.h"

// Use qsuba for smooth pixel colouring and qsubd for non-smooth pixel colouring
#define qsubd(x, b)  ((x>b)?b:0)                              // Digital unsigned subtraction macro. if result <0, then => 0. Otherwise, take on fixed value.
#define qsuba(x, b)  ((x>b)?x-b:0) 

PlasmaState::PlasmaState() : Animation() {}

void PlasmaState::plasma(Strip& strip) const {                                                 // This is the heart of this program. Sure is short. . . and fast.

  int thisPhase = beatsin8(6, -64, 64, AnimClock::timebase());                           // Setting phase change for a couple of waves.
  int thatPhase = beatsin8(7, -64, 64, AnimClock::timebase());

  for (int k=0; k < strip.numLeds; k++) {                              // For each of the LED's in the strand, set a brightness based on a wave as follows:

    int colorIndex = cubicwave8((k*23)+thisPhase)/2 + cos8((k*15)+thatPhase)/2;           // Create a wave and add a phase change and add another wave with its own phase change.. Hey, you can even change the frequencies if you wish.
    int thisBright = qsuba(colorIndex, beatsin8(7, 0, 96, AnimClock::timebase()));                                 // qsub gives it a bit of 'black' dead space by setting sets a minimum value. If colorIndex < current value of beatsin8(), then bright = 0. Otherwise, bright = colorIndex..

    strip.leds[k] = ColorFromPalette(strip.currentPalette, colorIndex, thisBright, LINEARBLEND);  // Let's now add the foreground colour.
  }
}

void PlasmaState::update(Strip& strip) const {
    //EVERY_N_MILLISECONDS(50) {                                  // FastLED based non-blocking delay to update/display the sequence.
  plasma(strip);
    //}

  // Per-strip timer, not EVERY_N_MILLISECONDS: that is one static shared by
  // every strip, so the first strip would consume each trigger and the rest
  // would never blend toward a newly selected palette.
  const uint32_t now = millis();
  if (now - strip.paletteLastBlendMs >= 100) {
    strip.paletteLastBlendMs = now;
    nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 24);   // AWESOME palette blending capability.
  }
}