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
            CHSV(32,  180, 255))},
        {"Cyberpunk", CRGBPalette16(
            CHSV(200, 255, 255), CHSV(220, 200, 255), CHSV(230, 255, 200), CHSV(245, 180, 255),
            CHSV(195, 255, 255), CHSV(210, 200, 255), CHSV(225, 255, 200), CHSV(240, 180, 255),
            CHSV(205, 255, 255), CHSV(215, 200, 255), CHSV(235, 255, 200), CHSV(250, 180, 255),
            CHSV(200, 255, 255), CHSV(220, 200, 255), CHSV(230, 255, 200), CHSV(245, 180, 255)
        )},
        {"ToxicSlime", CRGBPalette16(
            CHSV(80, 255, 255), CHSV(90, 200, 255), CHSV(100, 255, 200), CHSV(70, 180, 255),
            CHSV(85, 255, 255), CHSV(95, 200, 255), CHSV(105, 255, 200), CHSV(75, 180, 255),
            CHSV(82, 255, 255), CHSV(92, 200, 255), CHSV(102, 255, 200), CHSV(72, 180, 255),
            CHSV(88, 255, 255), CHSV(98, 200, 255), CHSV(108, 255, 200), CHSV(78, 180, 255)
        )},
        {"IceFire", CRGBPalette16(
            CHSV(160, 255, 255), CHSV(170, 200, 255), CHSV(30, 255, 255), CHSV(20, 200, 255),
            CHSV(165, 255, 200), CHSV(175, 200, 255), CHSV(35, 255, 200), CHSV(25, 180, 255),
            CHSV(160, 255, 255), CHSV(170, 200, 255), CHSV(30, 255, 255), CHSV(20, 200, 255),
            CHSV(165, 255, 200), CHSV(175, 200, 255), CHSV(35, 255, 200), CHSV(25, 180, 255)
        )},
        {"RetroArcade", CRGBPalette16(
            CHSV(0, 255, 255),   // Red
            CHSV(43, 255, 255),  // Yellow
            CHSV(85, 255, 255),  // Green
            CHSV(170, 255, 255), // Blue
            CHSV(213, 255, 255), // Purple
            CHSV(0, 255, 200),
            CHSV(85, 255, 200),
            CHSV(170, 255, 200),
            CHSV(43, 255, 180),
            CHSV(213, 200, 255),
            CHSV(0, 200, 180),
            CHSV(85, 200, 180),
            CHSV(170, 200, 180),
            CHSV(43, 200, 180),
            CHSV(213, 180, 255),
            CHSV(0, 255, 255)
        )},
        {"MidnightGlow", CRGBPalette16(
            CHSV(170, 255, 80), CHSV(180, 255, 150), CHSV(200, 200, 255), CHSV(220, 180, 200),
            CHSV(160, 255, 100), CHSV(190, 200, 255), CHSV(210, 150, 200), CHSV(230, 180, 180),
            CHSV(175, 255, 120), CHSV(195, 200, 200), CHSV(215, 150, 255), CHSV(225, 180, 180),
            CHSV(165, 255, 90),  CHSV(185, 200, 255), CHSV(205, 150, 200), CHSV(220, 180, 180)
        )},
        {"DeepSea", CRGBPalette16(
            CHSV(160, 200, 80), CHSV(170, 255, 150), CHSV(180, 180, 200), CHSV(190, 180, 150),
            CHSV(165, 220, 100), CHSV(175, 200, 180), CHSV(185, 180, 200), CHSV(195, 150, 150),
            CHSV(160, 180, 120), CHSV(170, 200, 160), CHSV(180, 180, 180), CHSV(190, 160, 140),
            CHSV(165, 200, 100), CHSV(175, 200, 180), CHSV(185, 150, 180), CHSV(195, 120, 120)
        )},
        {"WarmEmber", CRGBPalette16(
            CHSV(10, 200, 80), CHSV(15, 220, 150), CHSV(20, 180, 120), CHSV(25, 180, 180),
            CHSV(12, 220, 100), CHSV(18, 200, 150), CHSV(22, 150, 100), CHSV(28, 180, 140),
            CHSV(10, 180, 120), CHSV(16, 200, 160), CHSV(20, 180, 140), CHSV(26, 160, 120),
            CHSV(12, 200, 90),  CHSV(18, 180, 140), CHSV(22, 150, 120), CHSV(28, 120, 100)
        )},
        {"TwilightFade", CRGBPalette16(
            CHSV(190, 180, 100), CHSV(200, 160, 150), CHSV(210, 140, 180), CHSV(220, 160, 200),
            CHSV(200, 160, 120), CHSV(210, 140, 150), CHSV(220, 120, 180), CHSV(230, 140, 180),
            CHSV(195, 180, 120), CHSV(205, 160, 160), CHSV(215, 140, 200), CHSV(225, 160, 160),
            CHSV(200, 160, 120), CHSV(210, 140, 150), CHSV(220, 120, 160), CHSV(230, 100, 140)
        )},
        {"NeonStorm", CRGBPalette16(
            CHSV(200, 255, 255), CHSV(210, 255, 255), CHSV(0, 255, 255), CHSV(25, 255, 255),
            CHSV(160, 255, 255), CHSV(180, 255, 255), CHSV(45, 255, 255), CHSV(60, 255, 255),
            CHSV(195, 255, 255), CHSV(220, 255, 255), CHSV(0, 255, 200), CHSV(25, 255, 200),
            CHSV(165, 255, 200), CHSV(185, 255, 200), CHSV(50, 255, 200), CHSV(65, 255, 200)
        )},
        {"ArcaneFire", CRGBPalette16(
            CHSV(170, 255, 255), CHSV(180, 200, 200), CHSV(30, 255, 255), CHSV(200, 200, 255),
            CHSV(175, 255, 200), CHSV(185, 200, 200), CHSV(35, 255, 200), CHSV(205, 200, 200),
            CHSV(170, 255, 255), CHSV(180, 200, 200), CHSV(30, 255, 255), CHSV(200, 200, 255),
            CHSV(175, 255, 200), CHSV(185, 200, 200), CHSV(35, 255, 200), CHSV(205, 200, 200)
        )},
        {"TechnicolorPulse", CRGBPalette16(
            CHSV(0, 255, 255),   // Red
            CHSV(85, 255, 255),  // Green
            CHSV(170, 255, 255), // Blue
            CHSV(43, 255, 255),  // Yellow
            CHSV(213, 255, 255), // Purple
            CHSV(128, 255, 255), // Cyan
            CHSV(0, 255, 200),
            CHSV(85, 255, 200),
            CHSV(170, 255, 200),
            CHSV(43, 255, 200),
            CHSV(213, 255, 200),
            CHSV(128, 255, 200),
            CHSV(0, 255, 180),
            CHSV(85, 255, 180),
            CHSV(170, 255, 180),
            CHSV(43, 255, 180)
        )}
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