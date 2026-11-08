#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <functional>
#include "net/FirmwareUpdater.h"

// Home Assistant `update` entity for signed GitHub releases. Checks the latest
// release shortly after connecting and every few hours, offers it only if its
// signature verifies and it is newer than the running build, and installs it
// when HA sends "install". Sending "check" to the command topic re-checks now.
class UpdateEntity {
public:
    explicit UpdateEntity(PubSubClient& client) : client(client) {}

    void onConnected();   // discovery, subscription, state; schedules the first check
    // True if the message was for this entity (call before other handling).
    bool handleMessage(const char* topic, const uint8_t* payload, unsigned int length);
    void loop();
    // Runs just before an install starts (e.g. to save state before the reboot).
    void setBeforeInstall(std::function<void()> fn) { beforeInstall = std::move(fn); }

private:
    PubSubClient& client;
    std::function<void()> beforeInstall;
    FirmwareUpdater::Release latest = {};
    bool     haveLatest = false;
    bool     scheduled = false;
    bool     installRequested = false;
    uint32_t nextCheckMs = 0;
    String   note = "Not checked for updates yet.";   // outcome of the last check/install

    bool offering() const;
    void check();
    void install();
    void publishState(bool inProgress = false, int percent = -1);
};
