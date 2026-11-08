#include "led/PaletteManager.h"

// Gradient palettes: {index, r, g, b} stops that FastLED interpolates into a
// smooth 16-entry palette. The last stop must be index 255.
//
// Tuned for ColorWave, which sweeps indices 0..240 back and forth (LavaLamp
// folds the same way), so a palette reads A -> E and needn't loop back to its
// first colour (the 240 stop is the last colour actually shown). Drift wraps
// around the index range, so it shows a seam where the last colour meets the
// first.
//
// Values are LED drive levels, not screen colours: LEDs are linear, so low
// values look brighter than on a monitor and pastels wash out to white. Keep
// colours saturated, and never put opposite hues (red/green, orange/blue)
// next to each other: their linear blend passes through grey/olive.

// --- Cyberpunk ---

// Hot pink -> violet -> electric indigo -> azure -> cyan.
DEFINE_GRADIENT_PALETTE(NeonArcade_gp) {
      0, 255,   0, 120,
     64, 170,   0, 255,
    128,  40,   0, 255,
    192,   0,  90, 255,
    240,   0, 220, 255,
    255,   0, 220, 255};

// Indigo -> magenta -> hot pink -> orange -> amber.
DEFINE_GRADIENT_PALETTE(Synthwave_gp) {
      0,  50,   0, 150,
     64, 180,   0, 200,
    128, 255,   0,  90,
    192, 255,  50,   0,
    240, 255, 140,   0,
    255, 255, 140,   0};

// Teal -> deep blue -> plum -> crimson -> amber: cool to warm through the
// dark purple side, avoiding the muddy teal/orange midpoint.
DEFINE_GRADIENT_PALETTE(NeonNoir_gp) {
      0,   0, 200, 170,
     64,   0,  40, 140,
    128,  90,   0, 120,
    192, 220,   0,  40,
    240, 255, 100,   0,
    255, 255, 100,   0};

// Violet -> blue -> aqua -> acid lime -> yellow-lime.
DEFINE_GRADIENT_PALETTE(AcidRain_gp) {
      0,  60,   0, 160,
     64,   0,  50, 255,
    128,   0, 220, 160,
    192, 100, 255,   0,
    240, 200, 255,   0,
    255, 200, 255,   0};

// --- Moody botanical ---

// Pine -> forest -> moss -> jade -> deep teal.
DEFINE_GRADIENT_PALETTE(DeepForest_gp) {
      0,   0,  60,  35,
     64,  10, 110,  10,
    128,  70, 150,   0,
    192,   0, 120,  70,
    240,   0,  70,  70,
    255,   0,  70,  70};

// Leaf green -> teal -> indigo -> plum -> orchid: foliage into night flowers,
// bridged through blue to avoid the grey green/purple midpoint.
DEFINE_GRADIENT_PALETTE(Nightshade_gp) {
      0,   0, 100,  30,
     64,   0,  90,  90,
    128,  25,  20, 150,
    192, 100,   0, 120,
    240, 210,  20, 130,
    255, 210,  20, 130};

// Moss -> olive -> amber -> rust -> oxblood: autumn undergrowth.
DEFINE_GRADIENT_PALETTE(Lichen_gp) {
      0,  30, 110,   0,
     64, 110, 120,   0,
    128, 200,  90,   0,
    192, 170,  35,   0,
    240, 110,  10,   5,
    255, 110,  10,   5};

// --- Others to try ---

// Green -> teal -> blue -> violet -> magenta.
DEFINE_GRADIENT_PALETTE(Aurora_gp) {
      0,   0, 255,  80,
     64,   0, 200, 180,
    128,   0,  60, 255,
    192, 120,   0, 255,
    240, 255,   0, 170,
    255, 255,   0, 170};

// Navy -> ocean blue -> teal -> aquamarine -> seafoam.
DEFINE_GRADIENT_PALETTE(DeepSea_gp) {
      0,   0,  10,  90,
     64,   0,  50, 210,
    128,   0, 150, 170,
    192,   0, 230, 150,
    240,  70, 255, 200,
    255,  70, 255, 200};

// Deep red -> red-orange -> orange -> amber -> gold (no white-hot core).
DEFINE_GRADIENT_PALETTE(Firelight_gp) {
      0, 120,   0,   0,
     64, 255,  15,   0,
    128, 255,  60,   0,
    192, 255, 120,   0,
    240, 255, 170,  15,
    255, 255, 170,  15};

// Pink -> orchid -> lavender-blue -> sky.
DEFINE_GRADIENT_PALETTE(CottonCandy_gp) {
      0, 255,  30, 110,
     80, 200,  40, 220,
    160, 110,  70, 255,
    240,  40, 140, 255,
    255,  40, 140, 255};

// Red-orange -> orange -> yellow -> lime -> green.
DEFINE_GRADIENT_PALETTE(Citrus_gp) {
      0, 255,  30,   0,
     64, 255, 100,   0,
    128, 255, 190,   0,
    192, 140, 255,   0,
    240,  30, 255,  40,
    255,  30, 255,  40};

// Deep blue -> blue -> ice blue -> frost -> near-white.
DEFINE_GRADIENT_PALETTE(Glacier_gp) {
      0,   0,  20, 110,
     64,   0,  80, 230,
    128,  60, 170, 255,
    192, 160, 220, 255,
    240, 230, 245, 255,
    255, 230, 245, 255};

PaletteManager::PaletteManager() {
    paletteMap = {
        {"RainbowColors", RainbowColors_p},

        {"NeonArcade",  CRGBPalette16(NeonArcade_gp)},
        {"Synthwave",   CRGBPalette16(Synthwave_gp)},
        {"NeonNoir",    CRGBPalette16(NeonNoir_gp)},
        {"AcidRain",    CRGBPalette16(AcidRain_gp)},

        {"DeepForest",  CRGBPalette16(DeepForest_gp)},
        {"Nightshade",  CRGBPalette16(Nightshade_gp)},
        {"Lichen",      CRGBPalette16(Lichen_gp)},

        {"Aurora",      CRGBPalette16(Aurora_gp)},
        {"DeepSea",     CRGBPalette16(DeepSea_gp)},
        {"Firelight",   CRGBPalette16(Firelight_gp)},
        {"CottonCandy", CRGBPalette16(CottonCandy_gp)},
        {"Citrus",      CRGBPalette16(Citrus_gp)},
        {"Glacier",     CRGBPalette16(Glacier_gp)}};
}


const CRGBPalette16* PaletteManager::getPaletteByName(const char* name) const {
    auto it = paletteMap.find(std::string(name));
    return it != paletteMap.end() ? &it->second : nullptr;
}

void PaletteManager::buildPaletteJson(JsonDocument& doc) const {
    JsonArray arr = doc["paletteList"].to<JsonArray>();
    for (auto& kv : paletteMap) {
        arr.add(kv.first.c_str());
    }
}

void PaletteManager::addPaletteOptions(JsonArray arr) const {
    for (auto& kv : paletteMap) {
        arr.add(kv.first.c_str());
    }
}
