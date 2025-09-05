#pragma once

#include <map>
#include <string>
#include <FastLED.h>
#include <ArduinoJson.h>

class PaletteManager {
    private:
        std::map<std::string, const CRGBPalette16> paletteMap;
    public:
        PaletteManager();
        const CRGBPalette16* getPaletteByName(const char* name) const;
        void buildPaletteJson(JsonDocument& doc) const;
};
