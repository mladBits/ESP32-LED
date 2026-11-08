#pragma once

#include <Arduino.h>
#include <functional>

// Installs signed releases from GitHub (see scripts/release.py), and guards
// every newly installed image, from a release or a PC upload, with the
// bootloader's rollback.
namespace FirmwareUpdater {

// This board's entry in a release's manifest.json. fetchLatest() only fills
// one in after its signature has verified.
struct Release {
    char     version[24];
    char     file[80];
    uint32_t size;
    char     sha256[65];
    uint8_t  sig[80];    // DER ECDSA P-256 signature
    size_t   sigLen;
};

// Rollback guard. A new image boots "on probation": markHealthy() confirms it
// once MQTT is connected; if that hasn't happened within the timeout, or it
// keeps rebooting, the board returns to the image that ran before it. Uses
// the bootloader's rollback where available, else switches slots itself.
// Call begin() first thing in setup() and loop() every tick.
void begin();
void markHealthy();
void loop();
// Call right after activating a newly written image (before rebooting into
// it), whether it came from a release or a PC upload.
void armProbation();
bool onProbationAtBoot();   // this boot started on probation (rollback is armed)
bool rolledBack();          // the previous update failed its health check and was undone
const char* otaState();     // the running slot's OTA state now ("valid" once confirmed), for diagnostics

// Fetches the latest release's manifest and verifies this board's entry.
bool fetchLatest(Release& out, String& error);

// Downloads `release` into the spare OTA slot, checking its size and SHA-256
// against the signed values, and activates it only if they match. progress()
// gets 0..100. On success the caller should reboot into it.
bool install(const Release& release, const std::function<void(uint8_t)>& progress, String& error);

}  // namespace FirmwareUpdater
