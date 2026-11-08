#include "MqttLight.h"
#include "MqttTopics.h"
#include "animation/AnimationDirection.h"
#include "animation/AnimClock.h"
#include "config/Credentials.h"
#include "led/ColorMath.h"

MqttLight::MqttLight(PubSubClient& clientRef, const char* user, const char*pass)
: client(clientRef), mqttUser(user), mqttPass(pass), pm(), ar(pm) {
}

void MqttLight::begin() {
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
}

void MqttLight::loop() {
    if (!client.connected()) reconnect();
    client.loop();

    const uint32_t tnow = millis();
    if (client.connected() && tnow - lastTelemetryMs >= telemetryIntervalMs) {
        lastTelemetryMs = tnow;
        publishTelemetry();
    }

    // Power-down collapse owns the strips until it finishes, then hands off to
    // the normal blacked-out idle below.
    if (shuttingDown) {
        stepShutdown();
        return;
    }

    if (brightness < targetBrightness) brightness++;
    else if (brightness > targetBrightness) brightness--;
    
    const bool effectiveOn = (brightness > 0);

    //if fully off; black out once, then stop doing work
    if (!effectiveOn) {
        if (!blackedOut) {
            ledController->setBrightness(0);
            ledController->clear();
            ledController->show();
            blackedOut = true;
            prevBrightness = 0;
            prevHue = 0xFF;
            prevSat = 0xFF;
        }
        return;
    }
    blackedOut = false; // any render tick re-arms the blackout

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
        // While powering on, repaint every tick so the expanding reveal window
        // always has fresh solid content to uncover (the cache would otherwise
        // skip the repaint and leave revealed pixels black).
        if (poweringOn || brightness != prevBrightness || solidHueCur != prevHue || solidSatCur != prevSat) {
            ledController->applyHsv(solidHueCur, solidSatCur, 255);
            prevHue = solidHueCur;
            prevSat = solidSatCur;
            prevBrightness = brightness;
        }
    }

    // Carve the freshly-rendered frame with the expanding power-on window.
    if (poweringOn) stepPowerOnMask();

    ledController->show();
}

// Drives the one-shot power-down collapse. Runs every tick while shuttingDown
// is set; on completion it forces the strips fully dark and re-arms the normal
// blacked-out idle (matching the fully-off branch in loop()).
void MqttLight::stepShutdown() {
    const uint32_t elapsed = millis() - shutdownStartMs;

    if (elapsed >= shutdownDurationMs) {
        shuttingDown = false;
        brightness = 0;
        targetBrightness = 0;
        ledController->setBrightness(0);
        ledController->clear();
        ledController->show();
        blackedOut = true;
        prevBrightness = 0;
        prevHue = 0xFF;
        prevSat = 0xFF;
        return;
    }

    // Hold the pre-off brightness so the collapsing colours stay visible; the
    // effect fades itself to black.
    const uint8_t progress = (uint8_t)((elapsed * 255UL) / shutdownDurationMs);
    ledController->setBrightness(brightness);
    ledController->shutdownCollapse(progress);
    ledController->show();
}

// Applies the expanding power-on window to the already-rendered frame. Runs
// each tick while poweringOn is set (after the normal render, before show());
// clears the flag once the reveal has fully opened.
void MqttLight::stepPowerOnMask() {
    const uint32_t elapsed = millis() - powerOnStartMs;
    if (elapsed >= powerOnDurationMs) {
        poweringOn = false;
        return;
    }
    const uint8_t progress = (uint8_t)((elapsed * 255UL) / powerOnDurationMs);
    ledController->powerOnReveal(progress);
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
    // null marks the light as the device's main entity: HA shows it under the
    // device name instead of prefixing the device name onto a duplicate.
    doc["name"]         = nullptr;
    doc["unique_id"]    = MQTT_UNIQUE_ID;
    doc["schema"]       = "json";
    doc["stat_t"]       = "~/state";
    doc["cmd_t"]        = "~/set";
    doc["avty_t"]       = "~/availability";
    doc["bri_scl"]      = 255;
    doc["effect"]       = true;

    // Brightness is implied by any colour mode other than onoff.
    JsonArray supportedColors = doc["sup_clrm"].to<JsonArray>();
    supportedColors.add("hs");

    JsonArray effectList = doc["effect_list"].to<JsonArray>();
    const Animation* const* animations = ar.list();
    for (uint8_t i = 0; i < ar.getSize(); i++) {
       effectList.add(animations[i]->getName());
    }

    addDeviceInfo(doc);

    String payload;
    serializeJson(doc, payload);
    client.publish(CONFIG_TOPIC, payload.c_str(), true);
}

// Device block shared by every entity, grouping them under one HA device that
// shows the running firmware version, plus the origin HA logs for discovery.
void MqttLight::addDeviceInfo(JsonDocument& doc) {
    JsonObject device = doc["device"].to<JsonObject>();
    JsonArray ids = device["identifiers"].to<JsonArray>();
    ids.add(MQTT_UNIQUE_ID);
    device["name"]       = MQTT_NAME;
    device["sw_version"] = FW_VERSION;

    JsonObject origin = doc["origin"].to<JsonObject>();
    origin["name"]       = "ESP32-LED";
    origin["sw_version"] = FW_VERSION;
}

// Advertises palettes as a Home Assistant `select` entity so they can be
// chosen from a dropdown, grouped under the same device as the light.
void MqttLight::publishPaletteSelectConfig() {
    JsonDocument doc;
    doc["name"]             = "Palette";
    doc["unique_id"]        = MQTT_UNIQUE_ID "_palette";
    doc["command_topic"]    = PALETTE_SET_TOPIC;
    // HA `select` publishes the raw option; wrap it into the JSON the callback expects.
    doc["command_template"] = "{\"palette\": \"{{ value }}\"}";
    doc["state_topic"]      = PALETTE_STATE_TOPIC;
    doc["value_template"]   = "{{ value_json.palette }}";
    doc["availability_topic"] = AVAILABILITY_TOPIC;

    JsonArray options = doc["options"].to<JsonArray>();
    pm.addPaletteOptions(options);
    // Shown as state while an AI-generated palette (PALETTE_DATA_TOPIC) is
    // active; picking it from HA matches no named palette and does nothing.
    options.add("Custom");

    addDeviceInfo(doc);

    String payload;
    serializeJson(doc, payload);
    client.publish(PALETTE_SELECT_CONFIG_TOPIC, payload.c_str(), true);
}

void MqttLight::publishPaletteState() {
    JsonDocument doc;
    doc["palette"] = currentPaletteName;

    char buffer[64];
    serializeJson(doc, buffer, sizeof(buffer));
    client.publish(PALETTE_STATE_TOPIC, buffer, true);
}

// Publishes a small JSON telemetry frame for external metrics collection
// (e.g. mqtt2prometheus -> Prometheus -> Grafana). Non-retained: these are
// transient samples, not state to replay to new subscribers. current_ma uses
// the board's LED supply voltage (LED_VOLTS build flag).
void MqttLight::publishTelemetry() {
    const uint32_t powerMw = ledController ? ledController->estimatePowerMw() : 0;

    JsonDocument doc;
    doc["power_mw"]   = powerMw;
    doc["current_ma"] = powerMw / LED_VOLTS;    // I = P / V
    doc["brightness"] = brightness;
    doc["leds"]       = ledController ? ledController->getTotalLeds() : 0;
    doc["rssi"]       = WiFi.RSSI();
    doc["heap"]       = ESP.getFreeHeap();
    doc["uptime_s"]   = millis() / 1000;
    // false = SNTP hasn't set the clock yet, so animations aren't synced
    // with the other boards.
    doc["clock_synced"] = AnimClock::synced();
    // true = NVS holds a complete set of credentials, so a header-free
    // (release) build would boot and connect on this board.
    doc["creds_stored"] = credentials.stored;

    char buffer[256];
    serializeJson(doc, buffer, sizeof(buffer));
    client.publish(TELEMETRY_TOPIC, buffer);
}

// Single connection attempt, rate-limited to one every reconnectIntervalMs.
// Animations, brightness fades, and OTA keep running while the broker is
// unreachable; loop() retries until it returns. connect() itself blocks on the
// TCP connect and then the CONNACK wait (both timeouts kept short in main.cpp),
// so don't attempt it while WiFi is down.
void MqttLight::reconnect() {
    if (WiFi.status() != WL_CONNECTED) return;

    const uint32_t now = millis();
    if (now - lastReconnectAttemptMs < reconnectIntervalMs) return;
    lastReconnectAttemptMs = now;

    if (client.connect(MQTT_CLIENT_ID, mqttUser, mqttPass,
                       AVAILABILITY_TOPIC, 0, true, "offline")) {
        Serial.println("Connected to MQTT Broker");

        publishDeviceConfig();
        publishPaletteList();
        publishPaletteSelectConfig();

        Serial.print("Subscribing to ");
        Serial.println(SET_TOPIC);
        client.subscribe(SET_TOPIC);

        Serial.print("Subscribing to ");
        Serial.println(PALETTE_SET_TOPIC);
        client.subscribe(PALETTE_SET_TOPIC);

        Serial.print("Subscribing to ");
        Serial.println(PALETTE_DATA_TOPIC);
        client.subscribe(PALETTE_DATA_TOPIC);

        // publish default state.
        publishState();
        publishPaletteState();
        client.publish(AVAILABILITY_TOPIC, "online", true);
    } else {
        Serial.printf("MQTT connect failed, rc=%d. Retrying in %lus...\n",
                      client.state(), (unsigned long)(reconnectIntervalMs / 1000));
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
            if (br > 0) lastOnBrightness = (uint8_t)br;

            // While off (and not being turned on), only remember the level for
            // the next turn-on; applying it would light the strips while the
            // reported state stays OFF.
            if (ledController->isOn || isTargetStateOn) targetBrightness = (uint8_t)br;
        }

        // turn off leds from an on state.
        if (ledController->isOn && !isTargetStateOn) {
            if (targetBrightness > 0) lastOnBrightness = targetBrightness;

            ledController->isOn = false;
            targetBrightness = 0;

            // Play the power-down collapse instead of a plain fade. brightness
            // is left where it is so the effect renders the live colours before
            // fading itself to black.
            shuttingDown = true;
            shutdownStartMs = millis();
            poweringOn = false;     // cancel a bloom still in progress
            // turn on leds from an off state.
        } else if (!ledController->isOn && isTargetStateOn) {
            ledController->isOn = true;
            shuttingDown = false;   // cancel any in-progress power-down
            // default brightness when turning on/off
            if (!b) {
                targetBrightness = lastOnBrightness;
            }

            // Burst on from the centre instead of a slow fade: jump straight to
            // the target brightness and let the spatial reveal be the show.
            brightness = targetBrightness;
            poweringOn = true;
            powerOnStartMs = millis();
        }
        publishState();
    } else if(strncmp(topic, PALETTE_SET_TOPIC, sizeof(PALETTE_SET_TOPIC)) == 0) {
        if (doc["palette"].is<const char*>()) {
            const char* palette = doc["palette"];
            // "Custom" is only ever entered via PALETTE_DATA_TOPIC; selecting it
            // from HA falls through the lookup below and is a no-op.
            const CRGBPalette16* targetPalette = pm.getPaletteByName(palette);
            if (!targetPalette) {
                Serial.print("Invalid palette: ");
                Serial.println(palette);
                return;
            }
            ledController->updatePalette(*targetPalette);
            currentPaletteName = palette;
            publishPaletteState();
        }
    } else if (strncmp(topic, PALETTE_DATA_TOPIC, sizeof(PALETTE_DATA_TOPIC)) == 0) {
        // Raw palette: exactly 16 "#RRGGBB" entries. Applied directly, never saved —
        // it lives in strip RAM until replaced or the device reboots.
        JsonArray colors = doc["colors"].as<JsonArray>();
        if (colors.isNull() || colors.size() != 16) {
            Serial.println("Palette data rejected: need a \"colors\" array of 16 entries");
            return;
        }

        CRGB entries[16];
        for (int i = 0; i < 16; i++) {
            uint8_t r, g, b;
            const char* hex = colors[i].as<const char*>();
            if (!parseHexColor(hex, r, g, b)) {
                Serial.printf("Palette data rejected: bad color at index %d\n", i);
                return;
            }
            entries[i] = CRGB(r, g, b);
        }

        ledController->updatePalette(CRGBPalette16(
            entries[0],  entries[1],  entries[2],  entries[3],
            entries[4],  entries[5],  entries[6],  entries[7],
            entries[8],  entries[9],  entries[10], entries[11],
            entries[12], entries[13], entries[14], entries[15]
        ));
        currentPaletteName = "Custom";
        publishPaletteState();
    }
}

void MqttLight::publishState() {
    JsonDocument doc;
    doc["state"] = ledController->isOn ? "ON" : "OFF";
    doc["brightness"] = targetBrightness;

    // HA's JSON light ignores "color" in a state message without "color_mode".
    doc["color_mode"] = "hs";
    JsonObject color = doc["color"].to<JsonObject>();
    color["h"] = fastHueToHaDeg(hue);
    color["s"] = fastSatToHaPct(sat);

    // null clears HA's effect once back on a solid colour.
    if (effectWanted == EffectMode::ANIMATION && wantedAnimation) {
        doc["effect"] = wantedAnimation->getName();
    } else {
        doc["effect"] = nullptr;
    }

    char buffer[192];
    serializeJson(doc, buffer, sizeof(buffer));
    client.publish(STATE_TOPIC, buffer, true);
}
