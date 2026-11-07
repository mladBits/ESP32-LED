#include "Pacifica.h"
#include "AnimClock.h"
#include "AnimMath.h"


void pacifica_one_layer(CRGB* leds, int numLeds, CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff);
void pacifica_loop(CRGB* leds, int numLeds);
void pacifica_add_whitecaps(CRGB* leds, int numLeds);
void pacifica_deepen_colors(CRGB* leds, int numLeds);
//////////////////////////////////////////////////////////////////////////
//
// The code for this animation is more complicated than other examples, and 
// while it is "ready to run", and documented in general, it is probably not 
// the best starting point for learning.  Nevertheless, it does illustrate some
// useful techniques.
//
//////////////////////////////////////////////////////////////////////////
//
// In this animation, there are four "layers" of waves of light.  
//
// Each layer moves independently, and each is scaled separately.
//
// All four wave layers are added together on top of each other, and then 
// another filter is applied that adds "whitecaps" of brightness where the 
// waves line up with each other more.  Finally, another pass is taken
// over the led array to 'deepen' (dim) the blues and greens.
//
// The speed and scale and motion each layer varies slowly within independent 
// hand-chosen ranges, which is why the code has a lot of low-speed 'beatsin8' functions
// with a lot of oddly specific numeric ranges.
//
// These three custom blue-green color palettes were inspired by the colors found in
// the waters off the southern coast of California, https://goo.gl/maps/QQgd97jjHesHZVxQ7
//
CRGBPalette16 pacifica_palette_1 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
CRGBPalette16 pacifica_palette_2 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
CRGBPalette16 pacifica_palette_3 = 
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };

Pacifica::Pacifica() {}

void Pacifica::update(Strip& strip) const {
  //EVERY_N_MILLISECONDS( 20) {
    pacifica_loop(strip.leds, strip.numLeds);
  //}
}


void pacifica_loop(CRGB* leds, int numLeds) {
  // The four "color index start" positions, one per wave layer. Each layer
  // moves at its own beatsin88 speed, scaled by a shared surge factor
  // (beatsin16(3 or 4 BPM) of 179..269/256). The original integrated these
  // speeds frame by frame; here they are closed-form functions of the shared
  // clock (see AnimClock), so every board computes the same positions. Speeds
  // are Q8: a layer's own term is speed x 224 (the mean surge), and the surge
  // term is the layer's mean speed x the surge swing of +/-45/256. Their small
  // product term is dropped.
  const uint32_t t  = AnimClock::now();
  const uint32_t tb = AnimClock::timebase();
  const uint16_t cistart1 =   beatsinIntegral16(1011, 10 * 224, 13 * 224, t)
                            + beatsinIntegral16( 768, -518, 518, t);
  const uint16_t cistart2 = -(beatsinIntegral16( 777,  8 * 224, 11 * 224, t)
                            + beatsinIntegral16( 768, -214, 214, t)
                            + beatsinIntegral16(1024, -214, 214, t));
  const uint16_t cistart3 = -(beatsinIntegral16( 501,  5 * 224,  7 * 224, t)
                            + beatsinIntegral16( 768, -270, 270, t));
  const uint16_t cistart4 = -(beatsinIntegral16( 257,  4 * 224,  6 * 224, t)
                            + beatsinIntegral16(1024, -225, 225, t));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, numLeds, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer(leds, numLeds, pacifica_palette_1, cistart1, beatsin16( 3, 11 * 256, 14 * 256, tb), beatsin8( 10, 70, 130, tb), 0-beat16( 301, tb) );
  pacifica_one_layer(leds, numLeds, pacifica_palette_2, cistart2, beatsin16( 4,  6 * 256,  9 * 256, tb), beatsin8( 17, 40,  80, tb), beat16( 401, tb) );
  pacifica_one_layer(leds, numLeds, pacifica_palette_3, cistart3, 6 * 256, beatsin8( 9, 10,38, tb), 0-beat16(503, tb));
  pacifica_one_layer(leds, numLeds, pacifica_palette_3, cistart4, 5 * 256, beatsin8( 8, 10,28, tb), beat16(601, tb));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps(leds, numLeds);

  // Deepen the blues and greens a bit
  pacifica_deepen_colors(leds, numLeds);
}

// Add one layer of waves into the led array
void pacifica_one_layer(CRGB* leds, int numLeds, CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff){
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < numLeds; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps(CRGB* leds, int numLeds){
  uint8_t basethreshold = beatsin8( 9, 55, 65, AnimClock::timebase());
  uint8_t wave = beat8( 7, AnimClock::timebase());
  
  for( uint16_t i = 0; i < numLeds; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    if( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors(CRGB* leds, int numLeds){
  for( uint16_t i = 0; i < numLeds; i++) {
    leds[i].blue = scale8( leds[i].blue,  145); 
    leds[i].green= scale8( leds[i].green, 200); 
    leds[i] |= CRGB( 2, 5, 7);
  }
}