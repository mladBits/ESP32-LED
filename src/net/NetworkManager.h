#pragma once

#include <WiFi.h>
#include "config/WifiCreds.h"

class NetworkManager {
public:
    // Blocking initial connect — used from setup() only, where there is
    // nothing to render yet.
    static void connect() {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        Serial.println("Connecting to WiFi...");

        int retryCount = 0;
        while (WiFi.status() != WL_CONNECTED && retryCount < 30) {
            delay(1000);
            Serial.println(".");
            retryCount++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to WiFi!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\nFailed to connect to WiFi!");
        }
    }

    // Non-blocking recovery — call from loop(). Nudges a reconnect at most
    // every reconnectIntervalMs and returns immediately, so animations and
    // OTA keep running while WiFi is down.
    static void ensureConnected() {
        if (WiFi.status() == WL_CONNECTED) return;

        static uint32_t lastAttemptMs = 0 - reconnectIntervalMs;
        const uint32_t now = millis();
        if (now - lastAttemptMs < reconnectIntervalMs) return;
        lastAttemptMs = now;

        Serial.println("WiFi disconnected, attempting reconnect...");
        WiFi.reconnect();
    }

private:
    static constexpr uint32_t reconnectIntervalMs = 10000;
};