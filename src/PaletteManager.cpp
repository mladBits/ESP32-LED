#include "PaletteManager.h"

PaletteManager::PaletteManager() {
    paletteMap = {
        {"RainbowColors", RainbowColors_p},
        {"RainbowStripeColors", RainbowStripeColors_p},
        {"PartyColors", PartyColors_p},
        {"HeatColors", HeatColors_p},
        {"OceanColors", OceanColors_p},
        {"CloudColors", CloudColors_p},
        {"LavaColors", LavaColors_p},
        {"ForestColors", ForestColors_p},
        {"SunsetColors", CRGBPalette16(CRGB::OrangeRed, CRGB::Orange, CRGB::Yellow, CRGB::DarkRed)},
        {"AuroraColors", CRGBPalette16(CHSV(160, 255, 255), CHSV(180, 255, 255), CHSV(140, 200, 255), CHSV(170, 255, 128))},
        {"CandyColors", CRGBPalette16(CRGB::Magenta, CRGB::Fuchsia, CRGB::Pink, CRGB::Red)},
        {"NeonColors", CRGBPalette16(CRGB::Cyan, CRGB::Magenta, CRGB::Lime, CRGB::Yellow)},
        {"SunriseColors", CRGBPalette16(CHSV(10, 255, 255), CHSV(25, 200, 255), CHSV(40, 150, 255), CHSV(60, 128, 255))},
        {"NeonViolet", CRGBPalette16(CHSV(260, 255, 255), CHSV(280, 200, 255), CHSV(240, 255, 200), CHSV(300, 128, 255))},
        {"ElectricBlue", CRGBPalette16(CHSV(160, 255, 255), CHSV(180, 200, 255), CHSV(140, 255, 200), CHSV(200, 128, 255))},
        {"CyberSunset", CRGBPalette16(CHSV(330, 255, 255), CHSV(20, 200, 255), CHSV(50, 255, 200), CHSV(10, 128, 255))},
        {"FuchsiaDream", CRGBPalette16(CHSV(300, 255, 255), CHSV(320, 200, 255), CHSV(280, 255, 200), CHSV(340, 128, 255))},
        {"LaserPink", CRGBPalette16(CHSV(340, 255, 255), CHSV(350, 200, 255), CHSV(320, 255, 200), CHSV(360, 128, 255))},
        {"SynthWave", CRGBPalette16(CHSV(290, 255, 255), CHSV(310, 255, 200), CHSV(270, 200, 255), CHSV(330, 128, 255))},
        {"NeonCity", CRGBPalette16(CHSV(200, 255, 255), CHSV(220, 200, 255), CHSV(180, 255, 200), CHSV(240, 128, 255))},
        {"HotPinkPulse", CRGBPalette16(CHSV(320, 255, 255), CHSV(340, 200, 255), CHSV(300, 255, 200), CHSV(360, 128, 255))},
        {"DigitalAurora", CRGBPalette16(CHSV(150, 255, 255), CHSV(170, 200, 255), CHSV(130, 255, 200), CHSV(190, 128, 255))},
        {"GlitchPurple", CRGBPalette16(CHSV(250, 255, 255), CHSV(270, 200, 255), CHSV(230, 255, 200), CHSV(290, 128, 255))}
    };
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