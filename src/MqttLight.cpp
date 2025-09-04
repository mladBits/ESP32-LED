#include "MqttLight.h"


int brightness = 0;
int targetBrightness = 0;
bool isOn = false;
uint8_t hue = 0;
uint8_t sat = 255;
uint8_t prevHue = -1;
uint8_t prevSat = -1;
uint8_t prevBrightness = -1;

namespace {
    char stateTopic[64];
    char commandTopic[64];
    char lightConfigTopic[64];
    char paletteCommandTopic[64];
    char paletteStateTopic[64];
}

MqttLight::MqttLight(PubSubClient& clientRef, const char* user, const char*pass)
: client(clientRef), mqttUser(user), mqttPass(pass), pm(), ar(pm) {
}

void MqttLight::begin() {
    snprintf(stateTopic, sizeof(stateTopic), "%s/state", LIGHT_CONFIG_TOPIC);
    snprintf(commandTopic, sizeof(commandTopic), "%s/set", LIGHT_CONFIG_TOPIC);
    snprintf(lightConfigTopic, sizeof(lightConfigTopic), "%s/config", LIGHT_CONFIG_TOPIC);
    //snprintf(paletteCommandTopic, sizeof(paletteCommandTopic), "%s/palette/set", LIGHT_CONFIG_TOPIC);
    //snprintf(paletteStateTopic, sizeof(paletteStateTopic), "%s/palette/state", LIGHT_CONFIG_TOPIC);

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
            ledController->apply_hsv(hue, sat, brightness);
            prevHue = hue;
            prevSat = sat;
            prevBrightness = brightness;
        }
    } else {
        ledController->animate(currentAnimation);
    }
}

void MqttLight::setController(LEDController* controller) {
    ledController = controller;
}

void MqttLight::publishDeviceConfig() {
    JsonDocument doc;
    doc["~"]            = LIGHT_CONFIG_TOPIC;
    doc["name"]         = MQTT_NAME;
    doc["unique_id"]    = MQTT_UNIQUE_ID;
    doc["schema"]       = "json";
    doc["stat_t"]       = "~/state";
    doc["cmd_t"]        = "~/set";
    doc["brightness"]   = true;
    doc["bri_scl"]      = 255;
    doc["clrm"]         = true;
    doc["effect"]       = true;

    JsonArray supportedColors = doc["sup_clrm"].to<JsonArray>();
    supportedColors.add("hs");

    JsonArray effectList = doc["effect_list"].to<JsonArray>();
    Animation* const* animations = ar.list();
    for (uint8_t i = 0; i < ar.getSize(); i++) {
       effectList.add(animations[i]->getName());
    }
    
    char buffer[512];
    unsigned int n = serializeJson(doc, buffer, sizeof(buffer));
    client.publish(lightConfigTopic, buffer, true);
}

void MqttLight::reconnect() {
    unsigned long start = millis();
    while (!client.connected()) {
        if (client.connect(MQTT_CLIENT_ID, mqttUser, mqttPass)) {
            Serial.println("Connected to MQTT Broker");

            publishDeviceConfig();

            Serial.print("Subscribing to ");
            Serial.println(commandTopic);
            client.subscribe(commandTopic);

           // Serial.print("Subscribing to ");
           // Serial.println(paletteCommandTopic);
            //client.subscribe(paletteCommandTopic);

        } else {
            Serial.printf("MQTT connect failed, rc=%d. Retrying...\n", client.state());
            if (millis() - start > 30000) { // stop retrying after 30s
                Serial.println("MQTT connection timeout");
                break;
            }
            delay(2000);
        }
    }
}

void logTopicAndPayload(char *topic, byte* payload, unsigned int length) {
    char buf[64];
    Serial.print("Topic: ");
    Serial.print(topic);
    if (length < sizeof(buf)) {
        memcpy(buf, payload, length);
        buf[length] = '\0';
        Serial.print(", Payload: ");
        Serial.println(buf);
    }
}

void MqttLight::callback(char* topic, byte* payload, unsigned int length) {
    logTopicAndPayload(topic, payload, length);

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        Serial.print("JSON parse failed: ");
        Serial.println(err.c_str());
        return;
    }

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
        ledController->isAnimationActive(false);
        currentAnimation = nullptr;
        targetBrightness = 0;
    }

    if (doc["brightness"].is<int>()) {
        targetBrightness = doc["brightness"];
    }

    if (doc["color"].is<JsonObject>()) {
        JsonObject color = doc["color"];
        hue = (uint8_t)((color["h"].as<int>() * 255L) / 360L);
        sat = (uint8_t)((color["s"].as<int>() * 255L) / 100L);
        ledController->isAnimationActive(false);
        currentAnimation = nullptr;
    }

    if (doc["effect"].is<const char*>()) {
        const char* effect = doc["effect"];
        currentAnimation = ar.getByName(effect);
        ledController->isAnimationActive(true);
    }

    publishState();
}

void MqttLight::publishState() {
    JsonDocument doc;
    doc["state"] = isOn ? "ON" : "OFF";
    doc["brightness"] = targetBrightness;

    JsonObject color = doc["color"].to<JsonObject>();
    color["h"] = (hue * 360) / 255;
    color["s"] = (sat * 100) / 255;
    
    char buffer[128];
    unsigned int n = serializeJson(doc, buffer, sizeof(buffer));
    client.publish(stateTopic, buffer, true);
}
