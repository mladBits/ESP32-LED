#ifndef PALETTE_MANAGER_H
#define PALETTE_MANAGER_H

#include <map>
#include <string>
#include <FastLED.h>

class PaletteManager {
    private:
        const std::map<int, std::pair<std::string, const CRGBPalette16>> paletteMap;
    
    public:
        PaletteManager();
        const CRGBPalette16 getPaletteById(int paletteId) const;
        const String getPaletteNamesInJson() const;
};

#endif