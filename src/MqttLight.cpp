#include "MqttLight.h"
#include <Arduino.h>
#include <ArduinoJson.h>

int brightness = 0;
int targetBrightness = 0;
bool isOn = false;
uint8_t hue = 0;
uint8_t sat = 255;
uint8_t prevHue = -1;
uint8_t prevSat = -1;
uint8_t prevBrightness = -1;

MqttLight::MqttLight(PubSubClient& clientRef, const char* user, const char*pass)
    : client(clientRef), mqttUser(user), mqttPass(pass) {
}

void MqttLight::begin() {
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
}

void MqttLight::loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
 
    if (brightness < targetBrightness) brightness++;
    else if (brightness > targetBrightness) brightness--;
        
    if (!ledController->isAnimationActive()) {
        if (brightness != prevBrightness || hue != prevHue || sat != prevSat) {
            ledController->update(hue, sat, brightness);
            prevHue = hue;
            prevSat = sat;
            prevBrightness = brightness;
        }
    }
}

void MqttLight::setController(LEDController* controller) {
    ledController = controller;
}

void MqttLight::reconnect() {
    String configPayload = R"({
        "~": "homeassistant/light/esp32_peg_led",
        "name": "ESP32 Peg LED",
        "unique_id": "esp32_led_5",
        "schema": "json",
        "stat_t": "~/state",
        "cmd_t": "~/set",
        "brightness": true,
        "bri_scl": "255",
        "clrm": true,
        "sup_clrm": ["hs"]
    })";

    while(!client.connected()) {
        if (client.connect("esp32_peg_led_test", mqttUser, mqttPass)) {
            Serial.println("Connected to MQTT Broker!");
            
            client.publish(lightConfigTopic, configPayload.c_str(), true);
            
            client.subscribe(commandTopic);
        } else {
            delay(5000);
        }
    }
}



void MqttLight::callback(char* topic, byte* payload, unsigned int length) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        Serial.print("JSON parse failed: ");
        Serial.println(err.c_str());
        return;
    }

    payload[length] = '\0';
    String cmd = String((char*)payload);

    Serial.print(topic);
    Serial.print(", payload:");
    Serial.println(cmd);

    char buf[64];
    if (doc["state"].is<const char*>()) {
        const char* stateStr = doc["state"];
        isOn = (stateStr && strncmp(stateStr, "ON", 2) == 0);        
    }   

    if (isOn) {
        if (doc["brightness"].is<int>()) {
            targetBrightness = doc["brightness"];
        } else {
            targetBrightness = 150;
        }
    } else {
        targetBrightness = 0;
    }

    if (doc["brightness"].is<int>()) {
        targetBrightness = doc["brightness"];
    }

    if (doc["color"].is<JsonObject>()) {
        JsonObject color = doc["color"];
        hue = (uint8_t)((color["h"].as<int>() * 255L) / 360L);
        sat = (uint8_t)((color["s"].as<int>() * 255L) / 100L);
    }
    publishState();
}

void MqttLight::publishState() {
    JsonDocument doc;
    doc["state"] = isOn ? "ON" : "OFF";
    doc["brightness"] = brightness;

    JsonObject color = doc["color"].to<JsonObject>();
    color["h"] = (hue * 360) / 255;
    color["s"] = (sat * 100) / 255;
    
    char buffer[128];
    unsigned int n = serializeJson(doc, buffer);
    buffer[n] = '\0';
    client.publish(stateTopic, buffer, true);
}
