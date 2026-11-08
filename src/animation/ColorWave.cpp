// ColorWavesWithPalettes
// Animated shifting color waves, with several cross-fading color palettes.
// by Mark Kriegsman, August 2015
//
// https://gist.github.com/kriegsman/8281905786e8b2632aeb

#include "ColorWave.h"
#include "AnimClock.h"
#include "AnimMath.h"

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

    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    // Fold into a full 0..255..0 sweep (the original demo's `h16_128 >> 1`
    // only reached 127, so it showed just the first half of each palette).
    if (h16_128 & 0x100) {
      hue8 = 255 - (h16_128 & 0xFF);
    } else {
      hue8 = h16_128 & 0xFF;
    }

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;
    uint16_t bri16 = (uint32_t)b16 * (uint32_t)b16 / 65536;
    uint8_t bri8 = (uint32_t)bri16 * brightdepth / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = scale8(hue8, 240);
    CRGB newcolor = ColorFromPalette(strip.currentPalette, index, bri8);

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
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    // Fold into a full 0..255..0 sweep (the original demo's `h16_128 >> 1`
    // only reached 127, so it showed just the first half of each palette).
    if (h16_128 & 0x100) {
      hue8 = 255 - (h16_128 & 0xFF);
    } else {
      hue8 = h16_128 & 0xFF;
    }

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;
    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = scale8(hue8, 240);
    CRGB newcolor = ColorFromPalette(strip.currentPalette, index, bri8);

    if (left >= 0)  nblend(strip.leds[left], newcolor, 128);
    if (right < strip.numLeds) nblend(strip.leds[right], newcolor, 128);
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

  const uint32_t now = millis();
  if (now - strip.paletteLastBlendMs >= 40) {
    strip.paletteLastBlendMs = now;
    nblendPaletteTowardPalette(strip.currentPalette, strip.targetPalette, 16);
  }

#if defined(DEVICE_ID_ESP32_WALL)
  colorwaves(strip, hue16, pseudotime);
#else
  colorwavesCenter(strip, hue16, pseudotime);
#endif
}
