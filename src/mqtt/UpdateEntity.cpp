#include "mqtt/UpdateEntity.h"

#include <ArduinoJson.h>
#include <FastLED.h>
#include "config/Config.h"
#include "mqtt/HaDevice.h"
#include "mqtt/MqttTopics.h"
#include "net/Version.h"

namespace {
// Each check blocks rendering for a second or two (TLS handshake), so keep
// them rare. GitHub release downloads aren't API rate-limited.
constexpr uint32_t FIRST_CHECK_DELAY_MS = 30UL * 1000UL;
constexpr uint32_t CHECK_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
}  // namespace

void UpdateEntity::onConnected() {
    JsonDocument doc;
    doc["name"]               = "Firmware";
    doc["unique_id"]          = MQTT_UNIQUE_ID "_firmware";
    doc["device_class"]       = "firmware";
    doc["entity_category"]    = "config";
    doc["state_topic"]        = UPDATE_STATE_TOPIC;
    doc["command_topic"]      = UPDATE_COMMAND_TOPIC;
    doc["payload_install"]    = "install";
    doc["availability_topic"] = AVAILABILITY_TOPIC;
    addHaDeviceInfo(doc);

    String payload;
    serializeJson(doc, payload);
    client.publish(UPDATE_CONFIG_TOPIC, payload.c_str(), true);
    client.subscribe(UPDATE_COMMAND_TOPIC);
    publishState();

    if (!scheduled) {
        nextCheckMs = millis() + FIRST_CHECK_DELAY_MS;
        scheduled = true;
    }
}

bool UpdateEntity::handleMessage(const char* topic, const uint8_t* payload, unsigned int length) {
    if (strcmp(topic, UPDATE_COMMAND_TOPIC) != 0) return false;
    // Acted on in loop(), not here: installing calls client.loop(), which
    // mustn't run inside its own callback.
    if (length == 7 && memcmp(payload, "install", 7) == 0) installRequested = true;
    else if (length == 5 && memcmp(payload, "check", 5) == 0) nextCheckMs = millis();
    return true;
}

void UpdateEntity::loop() {
    if (!client.connected()) return;
    if (installRequested) {
        installRequested = false;
        install();
    } else if (scheduled && (int32_t)(millis() - nextCheckMs) >= 0) {
        nextCheckMs = millis() + CHECK_INTERVAL_MS;
        check();
    }
}

bool UpdateEntity::offering() const {
    return haveLatest && isNewerRelease(latest.version, FW_VERSION);
}

void UpdateEntity::check() {
    FirmwareUpdater::Release release;
    String error;
    if (!FirmwareUpdater::fetchLatest(release, error)) {
        note = "Update check failed: " + error;
        publishState();
        return;
    }
    latest = release;
    haveLatest = true;
    if (offering()) {
        note = String("Signed release ") + latest.version + " is ready to install (signature verified).";
    } else if (strcmp(latest.version, FW_VERSION) == 0) {
        note = String("Up to date with release ") + latest.version + " (signature verified).";
    } else {
        note = String("This build (" FW_VERSION ") is newer than the latest release, ") + latest.version + ".";
    }
    publishState();
}

void UpdateEntity::install() {
    // Always work from a fresh, freshly verified manifest.
    check();
    if (!offering()) {
        note = "Nothing to install: " + note;
        publishState();
        return;
    }

    Serial.printf("Installing %s\n", latest.version);
    FastLED.clear(true);   // strips stay dark while rendering is paused for the download
    publishState(true, 0);
    String error;
    const bool ok = FirmwareUpdater::install(latest, [this](uint8_t pct) {
        publishState(true, pct);
        client.loop();   // keep MQTT alive through the download
    }, error);

    if (ok) {
        note = String("Installed ") + latest.version + "; rebooting into it.";
        publishState(true, 100);
        client.loop();
        delay(500);
        ESP.restart();
    }
    note = "Update failed, still running " FW_VERSION ": " + error;
    publishState();
}

void UpdateEntity::publishState(bool inProgress, int percent) {
    String summary = FirmwareUpdater::rolledBack()
        ? "The last update failed its health check and was rolled back. " + note
        : note;
    if (summary.length() > 255) summary = summary.substring(0, 255);   // HA's limit

    JsonDocument doc;
    doc["installed_version"] = FW_VERSION;
    // HA offers an install whenever these differ, so report "latest" as the
    // running build unless this board would actually install the release.
    const bool offer = offering() || inProgress;
    doc["latest_version"]  = offer ? latest.version : FW_VERSION;
    doc["title"]           = "ESP32-LED firmware";
    doc["release_url"]     = offer ? String(RELEASES_URL "/tag/") + latest.version : String(RELEASES_URL);
    doc["release_summary"] = summary;
    doc["in_progress"]     = inProgress;
    if (percent >= 0) doc["update_percentage"] = percent;
    else              doc["update_percentage"] = nullptr;

    String payload;
    serializeJson(doc, payload);
    client.publish(UPDATE_STATE_TOPIC, payload.c_str(), true);
}
