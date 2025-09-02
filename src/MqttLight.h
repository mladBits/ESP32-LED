#ifndef MQTTLIGHT_H
#define MQTTLIGHT_H

#include <Wifi.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include "SolidColorState.h"
#include "LEDController.h"

class MqttLight {
    public:
        MqttLight(PubSubClient& client, const char* mqttUser, const char* mqttPass);
        void begin();
        void loop();
        void setController(LEDController* controller);
    
    private:
        PubSubClient& client;
        const char* mqttUser;
        const char* mqttPass;

        const char* stateTopic          = "homeassistant/light/esp32_peg_led/state";
        const char* commandTopic        = "homeassistant/light/esp32_peg_led/set";
        const char* lightConfigTopic    = "homeassistant/light/esp32_peg_led/config";

        LEDController* ledController;

        void callback(char* topic, byte* payload, unsigned int length);
        void reconnect();
        void publishState();

};

#endif