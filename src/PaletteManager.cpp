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
        {"SunsetColors", CRGBPalette16(
            CHSV(3, 255, 255), CHSV(11, 255, 255), CHSV(22, 255, 255), CHSV(33, 255, 255),
            CHSV(6, 200, 255), CHSV(17, 200, 255), CHSV(28, 200, 255), CHSV(39, 200, 255),
            CHSV(9, 150, 255), CHSV(21, 150, 255), CHSV(32, 150, 255), CHSV(43, 150, 255),
            CHSV(0, 128, 255), CHSV(14, 128, 255), CHSV(26, 128, 255), CHSV(37, 128, 255)
        )},
        {"AuroraColors", CRGBPalette16(
            CHSV(80, 255, 255), CHSV(91, 255, 255), CHSV(71, 200, 255), CHSV(85, 255, 128),
            CHSV(64, 200, 255), CHSV(96, 200, 255), CHSV(78, 255, 200), CHSV(88, 128, 255),
            CHSV(66, 255, 255), CHSV(92, 200, 255), CHSV(74, 200, 255), CHSV(83, 255, 200),
            CHSV(81, 128, 255), CHSV(87, 255, 255), CHSV(76, 200, 255), CHSV(84, 128, 255)
        )},
        {"CandyColors", CRGBPalette16(
            CHSV(213, 255, 255), CHSV(228, 200, 255), CHSV(236, 255, 200), CHSV(242, 128, 255),
            CHSV(216, 255, 255), CHSV(230, 200, 255), CHSV(238, 255, 200), CHSV(244, 128, 255),
            CHSV(219, 255, 255), CHSV(223, 200, 255), CHSV(230, 255, 200), CHSV(238, 128, 255),
            CHSV(213, 255, 255), CHSV(228, 200, 255), CHSV(236, 255, 200), CHSV(242, 128, 255)
        )},
        {"NeonColors", CRGBPalette16(
            CHSV(114, 255, 255), CHSV(136, 255, 255), CHSV(142, 200, 255), CHSV(128, 255, 200),
            CHSV(121, 255, 255), CHSV(132, 200, 255), CHSV(148, 255, 200), CHSV(157, 128, 255),
            CHSV(118, 255, 255), CHSV(129, 200, 255), CHSV(138, 255, 200), CHSV(145, 128, 255),
            CHSV(116, 255, 255), CHSV(132, 200, 255), CHSV(142, 255, 200), CHSV(148, 128, 255)
        )},
        {"SunriseColors", CRGBPalette16(
            CHSV(5, 255, 255), CHSV(13, 200, 255), CHSV(20, 150, 255), CHSV(30, 128, 255),
            CHSV(7, 255, 255), CHSV(14, 200, 255), CHSV(28, 150, 255), CHSV(42, 128, 255),
            CHSV(8, 255, 255), CHSV(15, 200, 255), CHSV(31, 150, 255), CHSV(47, 128, 255),
            CHSV(10, 255, 255), CHSV(18, 200, 255), CHSV(32, 150, 255), CHSV(50, 128, 255)
        )},
        {"NeonViolet", CRGBPalette16(
            CHSV(131, 255, 255), CHSV(141, 200, 255), CHSV(125, 255, 200), CHSV(155, 128, 255),
            CHSV(136, 255, 255), CHSV(146, 200, 255), CHSV(127, 255, 200), CHSV(158, 128, 255),
            CHSV(132, 255, 255), CHSV(142, 200, 255), CHSV(124, 255, 200), CHSV(154, 128, 255),
            CHSV(131, 255, 255), CHSV(141, 200, 255), CHSV(125, 255, 200), CHSV(155, 128, 255)
        )},
        {"PinkCyanFusion", CRGBPalette16(CHSV(136, 255, 255), 
            CHSV(142, 200, 255),  
            CHSV(149, 255, 255),  
            CHSV(159, 180, 255),  
            CHSV(170, 255, 255),  
            CHSV(180, 200, 255),  
            CHSV(191, 255, 255),  
            CHSV(202, 200, 255),  
            CHSV(213, 255, 255),  
            CHSV(224, 180, 255),  
            CHSV(236, 255, 255),  
            CHSV(247, 200, 255),  
            CHSV(0,   255, 255),  
            CHSV(11,  200, 255),  
            CHSV(21,  255, 255),  
            CHSV(32,  180, 255))}   
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