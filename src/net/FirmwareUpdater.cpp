#include "net/FirmwareUpdater.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include "esp_ota_ops.h"
#include "mbedtls/base64.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"

#include "config/Config.h"
#include "config/ReleaseKey.h"
#include "net/ReleaseFormat.h"
#include "net/RootCAs.h"

// An image that hasn't reached MQTT this long after booting is rolled back.
#ifndef FW_HEALTH_TIMEOUT_MS
#define FW_HEALTH_TIMEOUT_MS (10UL * 60UL * 1000UL)
#endif

// An image on probation that has booted more often than this without being
// confirmed (crash or reset loop) is rolled back straight away.
constexpr uint8_t MAX_PROBATION_BOOTS = 3;

// Arduino core hook: returning true stops the core confirming a newly
// installed image at boot, leaving it pending verification until
// markHealthy(). With a rollback-capable bootloader, a reset before then
// boots the previous image instead.
bool verifyRollbackLater() { return true; }

namespace {

// Two ways an image can be on probation:
//  - Bootloader: the bootloader supports rollback and marked the image
//    pending verification;
//  - App: the bootloader doesn't (it predates rollback support and is only
//    replaced by a USB flash), so the firmware that installed the image
//    recorded the slot it left in NVS, and this firmware switches back to it.
enum class Probation { None, Bootloader, App };
Probation probation = Probation::None;
bool probationAtBoot = false;
bool lastRolledBack = false;
esp_ota_img_states_t bootState = ESP_OTA_IMG_UNDEFINED;
bool bootStateKnown = false;

constexpr const char* NVS_NAMESPACE = "fwupdate";

void rollBack(const char* why) {
    Serial.printf("Rolling back firmware %s: %s\n", FW_VERSION, why);
    if (probation == Probation::Bootloader) {
        esp_ota_mark_app_invalid_rollback_and_reboot();
    } else {
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, false);
        const String prev = prefs.getString("prev", "");
        prefs.putBool("probation", false);
        prefs.putBool("rolled_back", true);
        prefs.end();
        const esp_partition_t* target =
            esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, prev.c_str());
        // set_boot_partition checks the image there is intact before switching.
        if (target && target != esp_ota_get_running_partition() &&
            esp_ota_set_boot_partition(target) == ESP_OK) {
            esp_restart();
        }
    }
    Serial.println("No previous image to return to; staying on this one");
    probation = Probation::None;
}

// HTTPS GET with redirects (GitHub hops through two hosts) and the embedded
// root CAs. Leaves the response open on `http` for the caller to read.
bool httpsGet(WiFiClientSecure& tls, HTTPClient& http, const String& url, String& error) {
    tls.setCACert(GITHUB_ROOT_CAS);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setTimeout(15000);
    if (!http.begin(tls, url)) {
        error = "bad URL";
        return false;
    }
    const int code = http.GET();
    if (code == HTTP_CODE_OK) return true;
    error = code < 0 ? http.errorToString(code) : "HTTP " + String(code);
    http.end();
    return false;
}

bool verifySignature(const char* version, uint32_t size, const char* sha256Hex,
                     const uint8_t* sig, size_t sigLen) {
    char message[256];
    const int n = formatSignedMessage(message, sizeof message, MQTT_CLIENT_ID, version, size, sha256Hex);
    if (n <= 0 || n >= (int)sizeof message) return false;

    uint8_t hash[32];
    if (mbedtls_sha256_ret((const uint8_t*)message, n, hash, 0) != 0) return false;

    mbedtls_pk_context key;
    mbedtls_pk_init(&key);
    // PEM input to mbedTLS must include the terminating NUL.
    const bool ok =
        mbedtls_pk_parse_public_key(&key, (const uint8_t*)RELEASE_PUBLIC_KEY_PEM,
                                    sizeof RELEASE_PUBLIC_KEY_PEM) == 0 &&
        mbedtls_pk_verify(&key, MBEDTLS_MD_SHA256, hash, sizeof hash, sig, sigLen) == 0;
    mbedtls_pk_free(&key);
    return ok;
}

bool copyString(char* out, size_t outLen, const char* in) {
    if (!in || strlen(in) >= outLen) return false;
    strcpy(out, in);
    return true;
}

}  // namespace

void FirmwareUpdater::begin() {
    bootStateKnown = esp_ota_get_state_partition(esp_ota_get_running_partition(), &bootState) == ESP_OK;

    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    lastRolledBack = esp_ota_get_last_invalid_partition() != nullptr || prefs.getBool("rolled_back", false);
    const bool appProbation = prefs.getBool("probation", false);

    if (bootStateKnown && bootState == ESP_OTA_IMG_PENDING_VERIFY) {
        probation = Probation::Bootloader;
        if (appProbation) prefs.putBool("probation", false);   // the bootloader has it covered
    } else if (appProbation) {
        probation = Probation::App;
        const uint8_t boots = prefs.getUChar("boots", 0) + 1;
        prefs.putUChar("boots", boots);
        if (boots > MAX_PROBATION_BOOTS) {
            prefs.end();
            rollBack("rebooted repeatedly without reaching MQTT");
            return;
        }
    }
    prefs.end();

    probationAtBoot = probation != Probation::None;
    if (probationAtBoot) {
        Serial.printf("Firmware %s on probation (%s): confirmed once MQTT connects, else rolled back after %lus\n",
                      FW_VERSION, probation == Probation::Bootloader ? "bootloader" : "app",
                      (unsigned long)(FW_HEALTH_TIMEOUT_MS / 1000));
    }
}

void FirmwareUpdater::armProbation() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool("probation", true);
    prefs.putString("prev", esp_ota_get_running_partition()->label);
    prefs.putUChar("boots", 0);
    prefs.putBool("rolled_back", false);
    prefs.end();
}

void FirmwareUpdater::markHealthy() {
#ifndef FW_TEST_NEVER_HEALTHY
    if (probation == Probation::Bootloader) {
        esp_ota_mark_app_valid_cancel_rollback();
    } else if (probation == Probation::App) {
        Preferences prefs;
        prefs.begin(NVS_NAMESPACE, false);
        prefs.putBool("probation", false);
        prefs.putUChar("boots", 0);
        prefs.end();
    }
    if (probation != Probation::None) Serial.println("Firmware confirmed healthy; rollback cancelled");
    probation = Probation::None;
#endif
}

void FirmwareUpdater::loop() {
    if (probation != Probation::None && millis() > FW_HEALTH_TIMEOUT_MS) {
        rollBack("never reached MQTT");
    }
}

bool FirmwareUpdater::onProbationAtBoot() { return probationAtBoot; }

bool FirmwareUpdater::rolledBack() { return lastRolledBack; }

const char* FirmwareUpdater::bootOtaState() {
    if (!bootStateKnown) return "unknown";
    switch (bootState) {
        case ESP_OTA_IMG_NEW:            return "new";
        case ESP_OTA_IMG_PENDING_VERIFY: return "pending";
        case ESP_OTA_IMG_VALID:          return "valid";
        case ESP_OTA_IMG_INVALID:        return "invalid";
        case ESP_OTA_IMG_ABORTED:        return "aborted";
        default:                         return "undefined";
    }
}

bool FirmwareUpdater::fetchLatest(Release& out, String& error) {
    WiFiClientSecure tls;
    HTTPClient http;
    if (!httpsGet(tls, http, RELEASES_URL "/latest/download/manifest.json", error)) {
        error = "release check: " + error;
        return false;
    }
    // getString() rather than the raw stream: it copes with chunked responses.
    const String body = http.getString();
    http.end();

    JsonDocument filter;
    filter["version"] = true;
    filter["boards"][MQTT_CLIENT_ID] = true;
    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, body, DeserializationOption::Filter(filter));
    if (err) {
        error = String("manifest: ") + err.c_str();
        return false;
    }

    JsonObject entry = doc["boards"][MQTT_CLIENT_ID];
    const char* sigB64 = entry["sig"].as<const char*>();
    Release r = {};
    r.size = entry["size"] | 0u;
    if (entry.isNull() || !copyString(r.version, sizeof r.version, doc["version"].as<const char*>()) ||
        !copyString(r.file, sizeof r.file, entry["file"].as<const char*>()) ||
        !copyString(r.sha256, sizeof r.sha256, entry["sha256"].as<const char*>()) || strlen(r.sha256) != 64 ||
        r.size == 0 || !sigB64 ||
        mbedtls_base64_decode(r.sig, sizeof r.sig, &r.sigLen, (const uint8_t*)sigB64, strlen(sigB64)) != 0) {
        error = "manifest has no usable entry for " MQTT_CLIENT_ID;
        return false;
    }
    if (!verifySignature(r.version, r.size, r.sha256, r.sig, r.sigLen)) {
        error = String("release ") + r.version + " has an invalid signature";
        return false;
    }
    out = r;
    return true;
}

bool FirmwareUpdater::install(const Release& r, const std::function<void(uint8_t)>& progress, String& error) {
    // Re-verify rather than trust the caller's copy.
    if (!verifySignature(r.version, r.size, r.sha256, r.sig, r.sigLen)) {
        error = "invalid signature";
        return false;
    }

    WiFiClientSecure tls;
    HTTPClient http;
    if (!httpsGet(tls, http, String(RELEASES_URL "/download/") + r.version + "/" + r.file, error)) {
        error = "download: " + error;
        return false;
    }
    if (http.getSize() != (int)r.size) {
        error = "download size " + String(http.getSize()) + " doesn't match the signed " + String(r.size);
        http.end();
        return false;
    }
    if (!Update.begin(r.size)) {
        error = String("no room for the image: ") + Update.errorString();
        http.end();
        return false;
    }

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts_ret(&sha, 0);

    WiFiClient* stream = http.getStreamPtr();
    static uint8_t buf[2048];
    uint32_t done = 0, lastDataMs = millis();
    uint8_t lastReported = 0;
    progress(0);
    while (done < r.size && error.isEmpty()) {
        const size_t avail = stream->available();
        if (!avail) {
            if (!http.connected() || millis() - lastDataMs > 20000) error = "download stalled";
            else delay(1);
            continue;
        }
        const size_t want = min(min(avail, sizeof buf), (size_t)(r.size - done));
        const size_t n = stream->readBytes(buf, want);
        if (n == 0) continue;
        lastDataMs = millis();
        if (Update.write(buf, n) != n) {
            error = String("flash write: ") + Update.errorString();
            break;
        }
        mbedtls_sha256_update_ret(&sha, buf, n);
        done += n;
        const uint8_t pct = (uint8_t)((uint64_t)done * 100 / r.size);
        if (pct >= lastReported + 5) {
            lastReported = pct - pct % 5;
            progress(lastReported);
        }
    }
    http.end();

    uint8_t digest[32];
    char hex[65];
    mbedtls_sha256_finish_ret(&sha, digest);
    mbedtls_sha256_free(&sha);
    toLowerHex(digest, sizeof digest, hex);

    if (error.isEmpty() && done != r.size) error = "incomplete download";
    if (error.isEmpty() && strcmp(hex, r.sha256) != 0) error = "image doesn't match its signed SHA-256";
    if (!error.isEmpty()) {
        Update.abort();   // never activated: the running image stays
        return false;
    }
    if (!Update.end()) {
        error = String("install: ") + Update.errorString();
        return false;
    }
    armProbation();
    progress(100);
    return true;
}
