#pragma once

#include <Wifi.h>
#include <FastLED.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

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
        PaletteManager pm;
        AnimationRegistry ar;

        PubSubClient& client;
        
        const char* mqttUser;
        const char* mqttPass;
 
        LEDController* ledController;
        Animation* currentAnimation;

        void init();
        void callback(char* topic, byte* payload, unsigned int length);
        void reconnect();
        void publishState();
        void publishDeviceConfig();
        void publishPaletteList();
};