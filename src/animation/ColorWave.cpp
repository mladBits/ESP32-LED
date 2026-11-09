// ColorWavesWithPalettes
// Animated shifting color waves, with several cross-fading color palettes.
// by Mark Kriegsman, August 2015
//
// https://gist.github.com/kriegsman/8281905786e8b2632aeb

#include "ColorWave.h"
#include "AnimClock.h"
#include "AnimMath.h"
#include "PaletteSample.h"

// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void ColorWave::colorwaves(Strip& strip, uint16_t hue16, uint16_t pseudotime) const
{
  uint8_t brightdepth = beatsin88(341, 96, 224, AnimClock::timebase());
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256), AnimClock::timebase());

  uint16_t hueinc16 = beatsin88(113, 300, 1500, AnimClock::timebase());

  uint16_t brightnesstheta16 = pseudotime;

  for (int i = strip.numLeds - 1; i >= 0; i--) {
    hue16 += hueinc16;


    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;
    uint16_t bri16 = (uint32_t)b16 * (uint32_t)b16 / 65536;
    uint8_t bri8 = (uint32_t)bri16 * brightdepth / 65536;
    bri8 += (255 - brightdepth);

    // The whole palette, there and back (the original demo's fold reached only
    // half of it), at 16-bit precision.
    CRGB newcolor = paletteFolded(strip.currentPalette, hue16, bri8);

    nblend(strip.leds[i], newcolor, 128);
  }
}

void ColorWave::colorwavesCenter(Strip& strip, uint16_t hue16, uint16_t pseudotime) const
{
  uint8_t brightdepth = beatsin88(341, 96, 224, AnimClock::timebase());
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256), AnimClock::timebase());

  uint16_t hueinc16 = beatsin88(113, 300, 1500, AnimClock::timebase());

  uint16_t brightnesstheta16 = pseudotime;

  int mid = strip.numLeds / 2;

  int startingOffset, step;
  if (strip.direction == Outward) { startingOffset = mid; step = -1; }
  else { startingOffset = 0; step = 1; }

  for (int offset = startingOffset; (step > 0) ? offset <= mid : offset >= 0; offset+=step) {
    int left  = mid - offset;
    int right = mid + offset;
    if (left < 0 && right >= strip.numLeds) break;

    hue16 += hueinc16;

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;
    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    // The whole palette, there and back (the original demo's fold reached only
    // half of it), at 16-bit precision.
    CRGB newcolor = paletteFolded(strip.currentPalette, hue16, bri8);

    if (left >= 0)  nblend(strip.leds[left], newcolor, 128);
    // At offset 0 left == right: blend the centre LED once, like the rest.
    if (right < strip.numLeds && right != left) nblend(strip.leds[right], newcolor, 128);
  }
}

// The wave phase and hue drift are pure functions of the shared clock, so
// every strip and every board renders the same moment. They are the closed
// form of the original per-frame accumulators (`+= deltams * beatsin88(...)`,
// speeds 23..60 and 5..9 units/ms, in Q8 below).
void ColorWave::update(Strip& strip) const {
  const uint32_t t = AnimClock::now();
  const uint16_t pseudotime = beatsinIntegral16(147, 23 * 256, 60 * 256, t);
  const uint16_t hue16      = beatsinIntegral16(400,  5 * 256,  9 * 256, t);

#if defined(DEVICE_ID_ESP32_WALL)
  colorwaves(strip, hue16, pseudotime);
#else
  colorwavesCenter(strip, hue16, pseudotime);
#endif
}
