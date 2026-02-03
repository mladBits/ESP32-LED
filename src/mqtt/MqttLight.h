#pragma once

#include <Wifi.h>
#include <FastLED.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "HomeAssistantColor.h"
#include "animation/Animation.h"
#include "animation/AnimationRegistry.h"
#include "LEDController.h"
#include "config/Mqtt.h"
#include "PaletteManager.h"


class MqttLight {
    public:
        MqttLight(PubSubClient& client, const char* mqttUser, const char* mqttPass);
        void begin();
        void loop();
        void setController(LEDController* controller);
    
    private:
        enum class EffectMode : uint8_t {SOLID, ANIMATION };

        PaletteManager pm;
        AnimationRegistry ar;

        PubSubClient& client;
        
        const char* mqttUser;
        const char* mqttPass;
 
        LEDController* ledController;
        
        bool powerRequestedOn = true;
        uint8_t brightness = 0;          // current brightness
        uint8_t targetBrightness = 0;    // fade target
        uint8_t lastOnBrightness = 150;

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


        void init();
        void callback(char* topic, byte* payload, unsigned int length);
        void reconnect();
        void publishState();
        void publishDeviceConfig();
        void publishPaletteList();
};