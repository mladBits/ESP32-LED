#include "led/LEDController.h"
#include "animation/AnimClock.h"

void LEDController::addStrip(CRGB* ledArray, int count) {
    if (numStrips < MAX_STRIPS) {
        strips[numStrips] = Strip(numStrips, ledArray, count);
        numStrips++;
    }
}

void LEDController::setBrightness(uint8_t b) {
    FastLED.setBrightness(b);
}

void LEDController::clear() {
    for (int i = 0; i < numStrips; i++) {
        fill_solid(strips[i].leds, strips[i].numLeds, CRGB::Black);
    }
}

void LEDController::applyHsv(uint8_t  h, uint8_t  s, uint8_t v) {
    // CHSV's default conversion, hsv2rgb_rainbow, on purpose. It spaces hues for
    // LEDs rather than screens: an LED's green looks far brighter than its red
    // and blue, so standard HSV (hsv2rgb_fullspectrum, which matches HA's
    // picker numerically) looked wrong on the strips - reds orange, yellow
    // greenish, green washed out (tried and reverted in Nov 2026).
    CHSV target(h, s, v);

    for (int i = 0; i < numStrips; i++) {
        fill_solid(strips[i].leds, strips[i].numLeds, target);
    }
}

// One-shot power-down frame. See header. The collapse phase (0..COLLAPSE_END)
// blacks out the strip from the ends inward — monotonic, so once a pixel is
// dark it stays dark on later frames — leaving the frozen frame's colours in a
// shrinking centre window behind a hot leading edge. The tail (COLLAPSE_END..
// 255) pops a small white core and dims it to black.
void LEDController::shutdownCollapse(uint8_t progress) {
    static constexpr uint8_t COLLAPSE_END = 200;

    for (int si = 0; si < numStrips; si++) {
        CRGB* leds = strips[si].leds;
        const int n = strips[si].numLeds;
        if (!leds || n <= 0) continue;

        const int center = n / 2;

        if (progress < COLLAPSE_END) {
            // cp: 0..255 through the collapse phase; window shrinks to nothing.
            const uint8_t cp = (uint8_t)(((uint16_t)progress * 255) / COLLAPSE_END);
            const int half = ((n / 2) * (255 - cp)) / 255;

            for (int i = 0; i < n; i++) {
                int d = i - center;
                if (d < 0) d = -d;
                if (d > half) leds[i] = CRGB::Black;
            }

            // Bright edges ride the window inward, growing hotter as it closes.
            CRGB edge = CRGB::White;
            edge.nscale8_video((uint8_t)(scale8(cp, 210) + 45));
            const int lo = center - half;
            const int hi = center + half;
            if (lo >= 0 && lo < n) leds[lo] += edge;
            if (hi >= 0 && hi < n && hi != lo) leds[hi] += edge;
        } else {
            // Flash + fade: a small white core pops, then dims to black.
            const uint8_t fp = (uint8_t)(((uint16_t)(progress - COLLAPSE_END) * 255)
                                         / (255 - COLLAPSE_END));
            fill_solid(leds, n, CRGB::Black);

            const uint8_t bri = 255 - fp;
            const int coreHalf = 1 + n / 60;
            for (int i = center - coreHalf; i <= center + coreHalf; i++) {
                if (i >= 0 && i < n) leds[i] = CRGB(bri, bri, bri);
            }
        }
    }
}

// One-shot power-on frame, the inverse of shutdownCollapse. The content for
// this frame is already in the buffer (normal render ran first); this only
// masks it: a white core blooms at the centre (0..BLOOM_END), then a window
// opens outward behind a fading white edge to reveal the live frame.
void LEDController::powerOnReveal(uint8_t progress) {
    static constexpr uint8_t BLOOM_END = 60;

    for (int si = 0; si < numStrips; si++) {
        CRGB* leds = strips[si].leds;
        const int n = strips[si].numLeds;
        if (!leds || n <= 0) continue;

        const int center = n / 2;

        // Window grows from the centre pixel out to the whole strip.
        const int half = ((n / 2) * progress) / 255;

        for (int i = 0; i < n; i++) {
            int d = i - center;
            if (d < 0) d = -d;
            if (d > half) leds[i] = CRGB::Black;
        }

        // Bright edges ride the window outward, cooling as they go.
        CRGB edge = CRGB::White;
        edge.nscale8_video((uint8_t)(255 - progress));
        const int lo = center - half;
        const int hi = center + half;
        if (lo >= 0 && lo < n) leds[lo] += edge;
        if (hi >= 0 && hi < n && hi != lo) leds[hi] += edge;

        // Initial white core so the light *pops* on before the window opens.
        if (progress < BLOOM_END) {
            const uint8_t bloom = 255 - (uint8_t)(((uint16_t)progress * 255) / BLOOM_END);
            const int coreHalf = 1 + n / 60;
            for (int i = center - coreHalf; i <= center + coreHalf; i++) {
                if (i >= 0 && i < n) leds[i] += CRGB(bloom, bloom, bloom);
            }
        }
    }
}

// Advances a palette change: an eased crossfade over PALETTE_FADE_MS, every
// entry at once. (FastLED's nblendPaletteTowardPalette steps one channel value
// at a time from entry 0 up, so a full change took 30-45 s, swept along the
// palette and passed through off colours.) UI timing, so millis() is fine.
static void stepPaletteFade(Strip& strip, uint32_t now) {
    if (!strip.paletteFading) return;
    const uint32_t elapsed = now - strip.paletteFadeStartMs;
    if (elapsed >= LEDController::PALETTE_FADE_MS) {
        strip.currentPalette = strip.targetPalette;
        strip.paletteFading = false;
        return;
    }
    const uint8_t fract = ease8InOutQuad((uint8_t)(elapsed * 256 / LEDController::PALETTE_FADE_MS));
    blend(strip.fromPalette.entries, strip.targetPalette.entries, strip.currentPalette.entries, 16, fract);
}

void LEDController::animate() {
    // One clock reading per frame: every strip renders the same instant.
    AnimClock::beginFrame();
    const uint32_t now = millis();
    for (int i = 0; i < numStrips; i++) {
        stepPaletteFade(strips[i], now);
        if (strips[i].animation) {
            strips[i].animation->update(strips[i]);
        }
    }
}

void LEDController::setAnimation(const Animation* anim) {
    for (int i = 0; i < numStrips; i++) {
        strips[i].animation = anim;
    }
}

void LEDController::registerAnimation(const AnimationRegistry* ar, const char* name) {
    Animation* a = ar->getByName(name);
    if (!a) return;

    setAnimation(a);
}

void LEDController::updatePalette(const CRGBPalette16& palette, bool immediate) {
    const uint32_t now = millis();

    for (int i = 0; i < numStrips; i++) {
        Strip& s = strips[i];
        s.targetPalette = palette;
        if (immediate) {
            s.currentPalette = s.fromPalette = palette;
            s.paletteFading = false;
        } else {
            // From what's showing now, even mid-fade, so a quick second change
            // doesn't jump.
            s.fromPalette = s.currentPalette;
            s.paletteFadeStartMs = now;
            s.paletteFading = true;
        }
    }
}

void LEDController::updateDirection(AnimationDirection direction) {
    for (int i = 0; i < numStrips; i++) {
        strips[i].direction = direction;
    }
}

void LEDController::show() { FastLED.show(); }

uint16_t LEDController::getTotalLeds() const {
    uint16_t total = 0;
    for (int i = 0; i < numStrips; i++) {
        total += strips[i].numLeds;
    }
    return total;
}

uint32_t LEDController::estimatePowerMw() const {
    // calculate_unscaled_power_mW reports draw as if at full brightness;
    // scale by the active global brightness to estimate the real draw.
    uint32_t unscaledMw = 0;
    for (int i = 0; i < numStrips; i++) {
        unscaledMw += calculate_unscaled_power_mW(strips[i].leds, strips[i].numLeds);
    }
    return (unscaledMw * FastLED.getBrightness()) / 255;
}