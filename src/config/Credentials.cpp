#include "config/Credentials.h"

#include <Arduino.h>
#include <Preferences.h>

// Local builds include the gitignored headers; CI and release builds don't.
#if __has_include("config/WifiCreds.h")
#include "config/WifiCreds.h"
static const char* const BUILD_WIFI_SSID     = WIFI_SSID;
static const char* const BUILD_WIFI_PASSWORD = WIFI_PASSWORD;
static const char* const BUILD_OTA_PASSWORD  = OTA_PASSWORD;
#else
static const char* const BUILD_WIFI_SSID     = nullptr;
static const char* const BUILD_WIFI_PASSWORD = nullptr;
static const char* const BUILD_OTA_PASSWORD  = nullptr;
#endif

#if __has_include("config/Mqtt.h")
#include "config/Mqtt.h"
static const char* const BUILD_MQTT_SERVER   = MQTT_SERVER;
static const char* const BUILD_MQTT_USER     = MQTT_USER;
static const char* const BUILD_MQTT_PASSWORD = MQTT_PASSWORD;
static const bool        BUILD_HAS_MQTT_PORT = true;
static const uint16_t    BUILD_MQTT_PORT     = MQTT_PORT;
#else
static const char* const BUILD_MQTT_SERVER   = nullptr;
static const char* const BUILD_MQTT_USER     = nullptr;
static const char* const BUILD_MQTT_PASSWORD = nullptr;
static const bool        BUILD_HAS_MQTT_PORT = false;
static const uint16_t    BUILD_MQTT_PORT     = 0;
#endif

Credentials credentials = {};

namespace {

// NVS namespace and keys (max 15 characters each).
constexpr const char* NVS_NAMESPACE = "creds";

// A value from this build wins and is saved if NVS differs (writing only on
// change spares the flash); otherwise the stored value is used. Returns
// whether NVS now holds the key.
bool syncString(Preferences& prefs, const char* key, const char* fromBuild,
                char* out, size_t outLen) {
    if (fromBuild) {
        if (!prefs.isKey(key) || prefs.getString(key, "") != fromBuild) {
            if (prefs.putString(key, fromBuild) == 0) {
                Serial.printf("Credentials: failed to save %s\n", key);
            }
        }
        strlcpy(out, fromBuild, outLen);
    } else if (prefs.isKey(key)) {
        prefs.getString(key, out, outLen);
    }
    return prefs.isKey(key);
}

}  // namespace

void loadCredentials() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);

    bool stored = true;
    stored &= syncString(prefs, "wifi_ssid", BUILD_WIFI_SSID,     credentials.wifiSsid,     sizeof credentials.wifiSsid);
    stored &= syncString(prefs, "wifi_pass", BUILD_WIFI_PASSWORD, credentials.wifiPassword, sizeof credentials.wifiPassword);
    stored &= syncString(prefs, "mqtt_host", BUILD_MQTT_SERVER,   credentials.mqttServer,   sizeof credentials.mqttServer);
    stored &= syncString(prefs, "mqtt_user", BUILD_MQTT_USER,     credentials.mqttUser,     sizeof credentials.mqttUser);
    stored &= syncString(prefs, "mqtt_pass", BUILD_MQTT_PASSWORD, credentials.mqttPassword, sizeof credentials.mqttPassword);
    stored &= syncString(prefs, "ota_pass",  BUILD_OTA_PASSWORD,  credentials.otaPassword,  sizeof credentials.otaPassword);

    if (BUILD_HAS_MQTT_PORT) {
        if (!prefs.isKey("mqtt_port") || prefs.getUShort("mqtt_port") != BUILD_MQTT_PORT) {
            prefs.putUShort("mqtt_port", BUILD_MQTT_PORT);
        }
        credentials.mqttPort = BUILD_MQTT_PORT;
    } else {
        credentials.mqttPort = prefs.getUShort("mqtt_port", 1883);
    }
    stored &= prefs.isKey("mqtt_port");

    prefs.end();

    credentials.fromBuild = BUILD_WIFI_SSID != nullptr;
    credentials.stored = stored;
    Serial.printf("Credentials: %s; stored set %s\n",
                  credentials.fromBuild ? "from this build" : "from NVS",
                  stored ? "complete" : "INCOMPLETE (provision with a build that has the headers)");
}
