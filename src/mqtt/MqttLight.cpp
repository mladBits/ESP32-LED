#include "MqttLight.h"
#include <Preferences.h>
#include "MqttTopics.h"
#include "animation/AnimationDirection.h"
#include "animation/AnimClock.h"
#include "config/Credentials.h"
#include "led/ColorMath.h"
#include "mqtt/HaDevice.h"
#include "net/FirmwareUpdater.h"

namespace {
constexpr const char* STATE_NAMESPACE = "lightstate";
constexpr const char* STATE_KEY = "state";
// 2: colours are saved for every palette (1 saved them only for "Custom";
// named palettes came from a built-in list). Same layout, so 1 still loads.
constexpr uint8_t STATE_VERSION = 2;
// Save this long after the last change: a dragged brightness slider becomes
// one flash write, not dozens.
constexpr uint32_t STATE_SAVE_DELAY_MS = 5000;

const char* resetReasonName() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "power_on";
        case ESP_RST_SW:        return "software";      // OTA, install, restart()
        case ESP_RST_PANIC:     return "crash";
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:       return "watchdog";
        case ESP_RST_BROWNOUT:  return "brownout";
        case ESP_RST_EXT:       return "reset_pin";
        case ESP_RST_DEEPSLEEP: return "deep_sleep";
        default:                return "other";
    }
}
}  // namespace

MqttLight::MqttLight(PubSubClient& clientRef, const char* user, const char*pass)
: client(clientRef), updates(clientRef), mqttUser(user), mqttPass(pass) {
    // The strips start on RainbowColors_p (Strip.h); keep the colours in step.
    for (int i = 0; i < 16; i++) paletteColors[i] = CRGB(RainbowColors_p[i]);
}

void MqttLight::begin() {
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
    updates.setBeforeInstall([this] { flushState(); });
    restoreState();
}

void MqttLight::loop() {
    if (!client.connected()) reconnect();
    client.loop();
    updates.loop();   // release checks; installs block here and reboot

    const uint32_t tnow = millis();
    if (client.connected() && tnow - lastTelemetryMs >= telemetryIntervalMs) {
        lastTelemetryMs = tnow;
        publishTelemetry();
    }

    saveStateIfDue(false);

    // Power-down collapse owns the strips until it finishes, then hands off to
    // the normal blacked-out idle below.
    if (shuttingDown) {
        stepShutdown();
        return;
    }

    stepBrightnessRamp();
    
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

// Moves brightness toward targetBrightness one step per BRIGHTNESS_STEP_MS
// (0..255 in ~1.3 s), on the clock rather than one step per loop, so fades keep
// their speed whatever the frame rate and catch up after a stall.
void MqttLight::stepBrightnessRamp() {
    const uint32_t now = millis();
    if (brightness == targetBrightness) {
        lastRampMs = now;
        return;
    }
    uint32_t steps = (now - lastRampMs) / BRIGHTNESS_STEP_MS;
    if (!steps) return;
    lastRampMs += steps * BRIGHTNESS_STEP_MS;
    const uint8_t gap = brightness < targetBrightness ? targetBrightness - brightness
                                                      : brightness - targetBrightness;
    if (steps > gap) steps = gap;
    if (brightness < targetBrightness) brightness += steps;
    else                               brightness -= steps;
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

    addHaDeviceInfo(doc);

    String payload;
    serializeJson(doc, payload);
    client.publish(CONFIG_TOPIC, payload.c_str(), true);
}

void MqttLight::publishPaletteState() {
    JsonDocument doc;
    doc["palette"] = currentPaletteName;

    char buffer[96];
    serializeJson(doc, buffer, sizeof(buffer));
    client.publish(PALETTE_STATE_TOPIC, buffer, true);
}

// Announces the board to palette-service, which owns the palette library and
// publishes this board's Palette dropdown in HA (on the same device, under
// the unique_id the board itself used to publish it with). Retained, so the
// service finds the board after its own restarts.
void MqttLight::publishPaletteBoard() {
    JsonDocument doc;
    doc["unique_id"] = MQTT_UNIQUE_ID;
    doc["name"]      = MQTT_NAME;
    doc["palette"]   = currentPaletteName;

    char buffer[160];
    serializeJson(doc, buffer, sizeof(buffer));
    client.publish(PALETTE_BOARD_TOPIC, buffer, true);
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
    // true = this boot started on probation: it is a newly installed image
    // that rolls back unless it reaches MQTT. ota_state is the running
    // slot's state now: "pending" while on the bootloader's probation,
    // "valid" once confirmed.
    doc["rollback_armed"] = FirmwareUpdater::onProbationAtBoot();
    doc["ota_state"]      = FirmwareUpdater::otaState();
    doc["reset_reason"]   = resetReasonName();

    char buffer[384];
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

        Serial.print("Subscribing to ");
        Serial.println(SET_TOPIC);
        client.subscribe(SET_TOPIC);

        Serial.print("Subscribing to ");
        Serial.println(PALETTE_DATA_TOPIC);
        client.subscribe(PALETTE_DATA_TOPIC);

        updates.onConnected();

        // publish default state.
        publishState();
        publishPaletteState();
        // After subscribing to PALETTE_DATA_TOPIC: the service answers with
        // the current palette's colours.
        publishPaletteBoard();
        client.publish(AVAILABILITY_TOPIC, "online", true);

        // Reaching the broker is the health check that confirms a newly
        // installed firmware (else it's rolled back).
        FirmwareUpdater::markHealthy();
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
    if (updates.handleMessage(topic, payload, length)) return;   // not JSON

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
            direction = AnimationDirectionHelper::parse(doc["effect_dir"]);
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
        markStateDirty();
        publishState();
    } else if (strncmp(topic, PALETTE_DATA_TOPIC, sizeof(PALETTE_DATA_TOPIC)) == 0) {
        // The palette to show: exactly 16 "#RRGGBB" entries and its name, which
        // the board reports back as the palette state (no name: "Custom").
        JsonArray colors = doc["colors"].as<JsonArray>();
        if (colors.isNull() || colors.size() != 16) {
            Serial.println("Palette data rejected: need a \"colors\" array of 16 entries");
            return;
        }
        const char* name = doc["name"] | "Custom";
        // Too long to save whole: a cut name would match no dropdown option.
        if (!*name || strlen(name) >= sizeof(SavedState::palette)) name = "Custom";

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
        memcpy(paletteColors, entries, sizeof paletteColors);
        currentPaletteName = name;
        markStateDirty();
        publishPaletteState();
    }
}

void MqttLight::markStateDirty() {
    stateDirty = true;
    stateChangedMs = millis();
}

void MqttLight::flushState() {
    saveStateIfDue(true);
}

MqttLight::SavedState MqttLight::captureState() const {
    SavedState s = {};   // zero-filled, so unused bytes compare equal
    s.version    = STATE_VERSION;
    s.on         = ledController && ledController->isOn;
    s.brightness = lastOnBrightness;
    s.hue        = hue;
    s.sat        = sat;
    s.animated   = effectWanted == EffectMode::ANIMATION && wantedAnimation;
    s.direction  = (uint8_t)direction;
    if (s.animated) strlcpy(s.effect, wantedAnimation->getName(), sizeof s.effect);
    strlcpy(s.palette, currentPaletteName.c_str(), sizeof s.palette);
    for (int i = 0; i < 16; i++) {
        s.colors[3 * i]     = paletteColors[i].r;
        s.colors[3 * i + 1] = paletteColors[i].g;
        s.colors[3 * i + 2] = paletteColors[i].b;
    }
    return s;
}

void MqttLight::saveStateIfDue(bool force) {
    if (!stateDirty || !ledController) return;
    if (!force && millis() - stateChangedMs < STATE_SAVE_DELAY_MS) return;
    stateDirty = false;

    const SavedState s = captureState();
    if (haveSaved && memcmp(&s, &lastSaved, sizeof s) == 0) return;
    Preferences prefs;
    prefs.begin(STATE_NAMESPACE, false);
    const bool ok = prefs.putBytes(STATE_KEY, &s, sizeof s) == sizeof s;
    prefs.end();
    if (ok) {
        lastSaved = s;
        haveSaved = true;
    }
}

// Puts the light back the way it was saved: palette, direction, colour,
// effect, and power (coming on with the usual power-on reveal). Runs before
// MQTT connects, so the lights return even without the broker; the first
// publishState() after connecting tells HA. Nothing saved yet keeps the
// defaults (off).
void MqttLight::restoreState() {
    SavedState s = {};
    Preferences prefs;
    prefs.begin(STATE_NAMESPACE, true);
    const bool ok = prefs.getBytesLength(STATE_KEY) == sizeof s &&
                    prefs.getBytes(STATE_KEY, &s, sizeof s) == sizeof s &&
                    (s.version == STATE_VERSION || s.version == 1);
    prefs.end();
    if (!ok || !ledController) return;
    lastSaved = s;
    haveSaved = true;
    s.effect[sizeof s.effect - 1] = '\0';
    s.palette[sizeof s.palette - 1] = '\0';

    if (s.palette[0]) {
        // Version 1 named palettes came from the old built-in list, so their
        // colours weren't saved: keep the name and the default colours until
        // palette-service resends them (it does when the board announces).
        if (s.version >= 2 || strcmp(s.palette, "Custom") == 0) {
            CRGBPalette16 palette;
            for (int i = 0; i < 16; i++) {
                paletteColors[i] = CRGB(s.colors[3 * i], s.colors[3 * i + 1], s.colors[3 * i + 2]);
                palette.entries[i] = paletteColors[i];
            }
            ledController->updatePalette(palette, true);
        }
        currentPaletteName = s.palette;
    }

    direction = s.direction == (uint8_t)Inward ? Inward : Outward;
    ledController->updateDirection(direction);

    hue = solidHueTo = solidHueCur = solidHueFrom = s.hue;
    sat = solidSatTo = solidSatCur = solidSatFrom = s.sat;

    if (s.animated) {
        if (Animation* anim = ar.getByName(s.effect)) {
            effectWanted = EffectMode::ANIMATION;
            wantedAnimation = anim;
            ledController->setAnimation(anim);
        }
    }

    if (s.brightness) lastOnBrightness = s.brightness;
    // After a brownout reset, come back off rather than straight back into
    // the load that may have caused it (avoids a reset loop). Everything else
    // is restored, so turning it on from HA resumes the saved look.
    const bool afterBrownout = esp_reset_reason() == ESP_RST_BROWNOUT;
    if (s.on && !afterBrownout) {
        ledController->isOn = true;
        targetBrightness = brightness = lastOnBrightness;
        poweringOn = true;
        powerOnStartMs = millis();
    }
    Serial.printf("Restored light state: %s, %s, palette %s\n",
                  s.on ? (afterBrownout ? "on (kept off after a brownout)" : "on") : "off",
                  s.animated ? s.effect : "solid colour", currentPaletteName.c_str());
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
