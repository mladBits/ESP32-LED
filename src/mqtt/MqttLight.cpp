#include "MqttLight.h"
#include "MqttTopics.h"
#include "animation/AnimationDirection.h"

MqttLight::MqttLight(PubSubClient& clientRef, const char* user, const char*pass)
: client(clientRef), mqttUser(user), mqttPass(pass), pm(), ar(pm) {
}

void MqttLight::begin() {
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
}

static uint8_t lerpHueShortest(uint8_t a, uint8_t b, uint8_t t /*0..255*/) {
    int16_t diff = (int16_t)b - (int16_t)a;

    // wrap to shortest path on the 0..255 hue circle
    if (diff > 127) diff -= 256;
    if (diff < -128) diff += 256;

    // linear interpolation along that shortest diff
    return (uint8_t)(a + ((diff * (int16_t)t) >> 8));
}

void MqttLight::loop() {
    if (!client.connected()) reconnect(); 
    client.loop();
 
    if (brightness < targetBrightness) brightness++;
    else if (brightness > targetBrightness) brightness--;
    
    const bool effectiveOn = powerRequestedOn && (brightness > 0);

    //if fully off; stop doing work
    if (!effectiveOn) {
        if (brightness == 0 && prevBrightness != 0) {
            ledController->setBrightness(0);
            ledController->clear();
            ledController->show();
            prevBrightness = 0;
            prevHue = 0xFF;
            prevSat = 0xFF;
        }
        return;
    }

    ledController->setBrightness(brightness);

    if (effectWanted == EffectMode::ANIMATION && wantedAnimation) {
        ledController->animate();

        // optional: don't let solid-mode cache block future solid updates
        prevHue = 0xFF;
        prevSat = 0xFF;
    } else {
        const uint32_t now = millis();
        if (solidColorBlending) {
            uint32_t elapsed = now - solidBlendStartMs;

            uint8_t t;
            if (elapsed >= solidBlendDurationMs) {
                t = 255;
                solidColorBlending = false;
            } else {
                t = (uint8_t)((elapsed * 255UL) / solidBlendDurationMs);
            }

            solidHueCur = lerpHueShortest(solidHueFrom, solidHueTo, t);
            solidSatCur = lerp8by8(solidSatFrom, solidSatTo, t);
        } else {
            solidHueCur = solidHueTo;
            solidSatCur = solidSatTo;
        }

        // render blended color; do NOT pass brightness as V (brightness handled by setBrightness)
        if (brightness != prevBrightness || solidHueCur != prevHue || solidSatCur != prevSat) {
            ledController->applyHsv(solidHueCur, solidSatCur, 255);
            prevHue = solidHueCur;
            prevSat = solidSatCur;
            prevBrightness = brightness;
        }
    }

    ledController->show();
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
    const Animation* const* animations = ar.list();
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

            float hDegF = color["h"].as<float>();
            float sPctF = color["s"].as<float>();

            int hDeg = (int)lroundf(hDegF);
            int sPct = (int)lroundf(sPctF);

            hue = haHueDegToFast(hDeg);
            sat = haSatPctToFast(sPct);
            
            effectWanted = EffectMode::SOLID;
            wantedAnimation = nullptr;

            // start solid color blend from current displayed color to new target
            solidHueFrom = solidHueCur;
            solidSatFrom = solidSatCur;

            solidHueTo = hue;
            solidSatTo = sat;

            solidBlendStartMs = millis();
            solidColorBlending = true;
        }

        // if effect is adjusted.
        if (doc["effect"].is<const char*>()) {
            const char* effect = doc["effect"];
            Animation* anim = ar.getByName(effect);
            if (anim) {
                effectWanted = EffectMode::ANIMATION;
                wantedAnimation = anim;

                ledController->setAnimation(wantedAnimation);
            }
        }

        // if effect direction is adjusted (not supported by home assistant).
        if (doc["effect_dir"].is<const char*>()) {
            AnimationDirection direction = AnimationDirectionHelper::parse(doc["effect_dir"]);
            ledController->updateDirection(direction);
        }

        // if brightness is adjusted.
        if (b) {
            int br = doc["brightness"].as<int>();
            if (br < 0) br = 0;
            if (br > 255) br = 255;
            targetBrightness = (uint8_t)br;

            if (targetBrightness > 0) lastOnBrightness = targetBrightness;
        }

        // turn off leds from an on state.
        if (ledController->isOn && !isTargetStateOn) {
            if (targetBrightness > 0) lastOnBrightness = targetBrightness;

            ledController->isOn = false;
            targetBrightness = 0;
            // turn on leds from an off state.
        } else if (!ledController->isOn && isTargetStateOn) {
            ledController->isOn = true;
            // default brightness when turning on/off
            if (!b) {
                targetBrightness = lastOnBrightness;
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
    color["h"] = fastHueToHaDeg(hue);
    color["s"] = fastSatToHaPct(sat);
    
    char buffer[128];
    serializeJson(doc, buffer, sizeof(buffer));
    client.publish(STATE_TOPIC, buffer, true);
}
