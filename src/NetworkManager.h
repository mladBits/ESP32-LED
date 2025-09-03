#pragma once

#include <WiFi.h>
#include "config/WifiCreds.h"

class NetworkManager {
public:
    static void connect() {
        WiFi.mode(WIFI_STA);
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
};