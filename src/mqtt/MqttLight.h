#pragma once

#include <WiFi.h>
#include <FastLED.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "mqtt/HomeAssistantColor.h"
#include "animation/Animation.h"
#include "animation/AnimationRegistry.h"
#include "led/LEDController.h"
#include "mqtt/UpdateEntity.h"


class MqttLight {
    public:
        MqttLight(PubSubClient& client, const char* mqttUser, const char* mqttPass);
        void begin();   // after setController(): also restores the saved light state
        void loop();
        void setController(LEDController* controller);
        // Saves a pending state change now; call before a deliberate reboot.
        void flushState();

    private:
        enum class EffectMode : uint8_t {SOLID, ANIMATION };

        // The user-visible light state, saved to NVS so it survives reboots,
        // power cuts and firmware updates. Byte-for-byte compared to skip
        // redundant writes; bump STATE_VERSION (MqttLight.cpp) if it changes.
        struct SavedState {
            uint8_t version;
            uint8_t on;
            uint8_t brightness;       // level to come back on at
            uint8_t hue, sat;         // solid colour
            uint8_t animated;         // effect mode
            uint8_t direction;
            char    effect[24];
            char    palette[24];      // name, as palette-service sent it
            uint8_t colors[16 * 3];   // its RGB (version 1: only for "Custom")
        };
        SavedState lastSaved = {};
        bool haveSaved = false;
        bool stateDirty = false;
        uint32_t stateChangedMs = 0;
        AnimationDirection direction = Outward;
        // The shown palette's colours (the board keeps no palette library).
        CRGB paletteColors[16];

        SavedState captureState() const;
        void restoreState();
        void markStateDirty();
        void saveStateIfDue(bool force);

        AnimationRegistry ar;

        PubSubClient& client;
        UpdateEntity updates;   // HA firmware update entity (signed releases)

        const char* mqttUser;
        const char* mqttPass;
 
        LEDController* ledController;
        
        uint8_t brightness = 0;          // current brightness
        uint8_t targetBrightness = 0;    // fade target
        uint8_t lastOnBrightness = 150;
        // time-based fade toward targetBrightness (see stepBrightnessRamp)
        static constexpr uint32_t BRIGHTNESS_STEP_MS = 5;
        uint32_t lastRampMs = 0;
        void stepBrightnessRamp();
        bool blackedOut = false;         // strips cleared+shown after fading to 0

        // one-shot power-down collapse played when toggling off
        bool shuttingDown = false;
        uint32_t shutdownStartMs = 0;
        static constexpr uint16_t shutdownDurationMs = 700;

        // one-shot power-on bloom played when toggling on
        bool poweringOn = false;
        uint32_t powerOnStartMs = 0;
        static constexpr uint16_t powerOnDurationMs = 650;

        // color
        uint8_t hue = 0;
        uint8_t sat = 0;

        uint8_t prevHue = 0xFF;
        uint8_t prevSat = 0xFF;
        uint8_t prevBrightness = 0xFF;

        uint8_t solidHueCur = 0;
        uint8_t solidSatCur = 0;

        uint8_t solidHueFrom = 0;
        uint8_t solidSatFrom = 0;

        uint8_t solidHueTo = 0;
        uint8_t solidSatTo = 0;

        bool solidColorBlending = false;
        uint32_t solidBlendStartMs = 0;
        uint16_t solidBlendDurationMs = 1000; 

        // effect selection
        EffectMode effectWanted = EffectMode::SOLID;
        Animation* wantedAnimation = nullptr;

        // shown palette's name (defaults to the Strip default, RainbowColors_p)
        String currentPaletteName = "RainbowColors";

        // non-blocking reconnect pacing; initialized one interval in the
        // past (unsigned wrap) so the first attempt at boot is immediate
        static constexpr uint32_t reconnectIntervalMs = 5000;
        uint32_t lastReconnectAttemptMs = 0 - reconnectIntervalMs;

        // periodic telemetry publishing (power estimate, RSSI, heap, uptime)
        static constexpr uint32_t telemetryIntervalMs = 10000;
        uint32_t lastTelemetryMs = 0 - telemetryIntervalMs;

        void init();
        void callback(char* topic, byte* payload, unsigned int length);
        void stepShutdown();
        void stepPowerOnMask();
        void reconnect();
        void publishState();
        void publishTelemetry();
        void publishDeviceConfig();
        void publishPaletteState();
        void publishPaletteBoard();
};