#include "PaletteManager.h"

PaletteManager::PaletteManager()
    : paletteMap({
        {1, {"RainbowColors", RainbowColors_p}},
        {2, {"RainbowStripeColors", RainbowStripeColors_p}},
        {3, {"PartyColors", PartyColors_p}},
        {4, {"HeatColors", HeatColors_p}},
        {5, {"OceanColors", OceanColors_p}},
        {6, {"CloudColors", CloudColors_p}},
        {7, {"LavaColors", LavaColors_p}},
        {8, {"ForestColors", ForestColors_p}},
    }) {}

const CRGBPalette16 PaletteManager::getPaletteById(int paletteId) const {
    auto it = paletteMap.find(paletteId);
    if (it != paletteMap.end()) {
        return it->second.second;
    }
    
    return OceanColors_p; //default, but we should handle this properly.
}

const String PaletteManager::getPaletteNamesInJson() const {
    String jsonResult = "[";
    bool isFirst = true;

    for (const auto& entry: paletteMap) {
        if (!isFirst) {
            jsonResult += ",";
        }
        isFirst = false;

        jsonResult += "\"" + String(entry.second.first.c_str()) + "\"";
    }

    jsonResult += "]";
    return jsonResult;
}