#pragma once

#include <stdint.h>

// WiFi, MQTT and OTA settings, kept in the board's NVS storage, which
// survives reboots and firmware updates.
//
// A build made with the gitignored local headers (WifiCreds.h, Mqtt.h)
// treats their values as the master copy: it uses them directly and saves
// them to NVS whenever NVS differs, so one normal OTA update provisions a
// board. A build without the headers (CI, releases) uses whatever NVS holds,
// so its firmware file contains no secrets.
struct Credentials {
    char     wifiSsid[33];        // 802.11 max 32
    char     wifiPassword[65];    // WPA2 max 63, or 64 hex
    char     mqttServer[65];
    uint16_t mqttPort;
    char     mqttUser[65];
    char     mqttPassword[129];
    char     otaPassword[129];

    bool fromBuild;   // values came from this build's headers (and were saved)
    bool stored;      // NVS holds a complete set: a header-free build could boot
};

extern Credentials credentials;

// Fills `credentials`. Call first thing in setup(), before WiFi or MQTT.
void loadCredentials();
