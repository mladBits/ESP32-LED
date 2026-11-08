#pragma once

#include <ArduinoJson.h>
#include "config/Config.h"

// Device block shared by every entity, grouping them under one HA device that
// shows the running firmware version, plus the origin HA logs for discovery.
inline void addHaDeviceInfo(JsonDocument& doc) {
    JsonObject device = doc["device"].to<JsonObject>();
    JsonArray ids = device["identifiers"].to<JsonArray>();
    ids.add(MQTT_UNIQUE_ID);
    device["name"]       = MQTT_NAME;
    device["sw_version"] = FW_VERSION;

    JsonObject origin = doc["origin"].to<JsonObject>();
    origin["name"]       = "ESP32-LED";
    origin["sw_version"] = FW_VERSION;
}
