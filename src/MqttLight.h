#ifndef MQTTLIGHT_H
#define MQTTLIGHT_H

#include <Wifi.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include "LEDController.h"
#include "config/Mqtt.h"

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
 
        LEDController* ledController;

        void callback(char* topic, byte* payload, unsigned int length);
        void reconnect();
        void publishState();
};

#endif