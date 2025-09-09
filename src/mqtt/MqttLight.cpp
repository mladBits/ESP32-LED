#include "MqttLight.h"
#include "MqttTopics.h"
#include "animation/AnimationDirection.h"

int brightness = 0;
int targetBrightness = 0;
uint8_t hue = 32;
uint8_t sat = 180;
uint8_t prevHue = -1;
uint8_t prevSat = -1;
uint8_t prevBrightness = -1;

MqttLight::MqttLight(PubSubClient& clientRef, const char* user, const char*pass)
: client(clientRef), mqttUser(user), mqttPass(pass), pm(), ar(pm) {
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
            ledController->applyHsv(hue, sat, brightness);
            prevHue = hue;
            prevSat = sat;
            prevBrightness = brightness;
        }
    } else {
        ledController->animate();
    }
}

void MqttLight::setController(LEDController* controller) {
    ledController = controller;
}

void MqttLight::publishPaletteList() {
    JsonDocument doc;
    pm.buildPaletteJson(doc);

    String payload;
    serializeJson(doc, payload);
    client.publish(PALETTE_LIST_TOPIC, payload.c_str(), true);
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
    
    String payload;
    serializeJson(doc, payload);
    client.publish(CONFIG_TOPIC, payload.c_str(), true);
}

void MqttLight::reconnect() {
    unsigned long start = millis();
    while (!client.connected()) {
        if (client.connect(MQTT_CLIENT_ID, mqttUser, mqttPass)) {
            Serial.println("Connected to MQTT Broker");

            publishDeviceConfig();
            publishPaletteList();

            Serial.print("Subscribing to ");
            Serial.println(SET_TOPIC);
            client.subscribe(SET_TOPIC);

            Serial.print("Subscribing to ");
            Serial.println(PALETTE_SET_TOPIC);
            client.subscribe(PALETTE_SET_TOPIC);

            // publish default state.
            publishState();
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
    } else {
        Serial.println("");
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

    if (strncmp(topic, SET_TOPIC, sizeof(SET_TOPIC)) == 0) {
        bool b = doc["brightness"].is<int>();

        // target on/off state.
        bool isTargetStateOn = ledController->isOn;
        if (doc["state"].is<const char*>()) {
            const char* stateStr = doc["state"];
            isTargetStateOn = (stateStr && strncmp(stateStr, "ON", 2) == 0);    
        }

        // if color is adjusted.
        if (doc["color"].is<JsonObject>()) {
            JsonObject color = doc["color"];
            hue = (uint8_t)((color["h"].as<int>() * 255L) / 360L);
            sat = (uint8_t)((color["s"].as<int>() * 255L) / 100L);
            ledController->isAnimationActive(false);
            currentAnimation = nullptr;
        }

        // if effect is adjusted.
        if (doc["effect"].is<const char*>()) {
            const char* effect = doc["effect"];
            ledController->registerAnimation(&ar, effect);
            ledController->isAnimationActive(true);
        }

        // if effect direction is adjusted (not supported by home assistant).
        if (doc["effect_dir"].is<const char*>()) {
            AnimationDirection direction = AnimationDirectionHelper::parse(doc["effect_dir"]);
            ledController->updateDirection(direction);
        }

        // if brightness is adjusted.
        if (b) {
            targetBrightness = doc["brightness"];
        }

        // turn off leds from an on state.
        if (ledController->isOn && !isTargetStateOn) {
            ledController->isOn = false;
            ledController->isAnimationActive(false);
            targetBrightness = 0;
            // turn on leds from an off state.
        } else if (!ledController->isOn && isTargetStateOn) {
            ledController->isOn = true;
            // default brightness when turning on/off
            if (!b) {
                targetBrightness = 150;
            }
        }
        publishState();
    } else if(strncmp(topic, PALETTE_SET_TOPIC, sizeof(PALETTE_SET_TOPIC)) == 0) {
        if (doc["palette"].is<const char*>()) {
            const char* palette = doc["palette"];
            const CRGBPalette16* targetPalette = pm.getPaletteByName(palette);
            if (!targetPalette) {
                Serial.print("Invalid palette: ");
                Serial.println(palette);
                return;
            }
            ledController->updatePalette(*targetPalette);
        }   
    }
}

void MqttLight::publishState() {
    JsonDocument doc;
    doc["state"] = ledController->isOn ? "ON" : "OFF";
    doc["brightness"] = targetBrightness;

    JsonObject color = doc["color"].to<JsonObject>();
    color["h"] = (hue * 360) / 255;
    color["s"] = (sat * 100) / 255;
    
    char buffer[128];
    unsigned int n = serializeJson(doc, buffer, sizeof(buffer));
    client.publish(STATE_TOPIC, buffer, true);
}
