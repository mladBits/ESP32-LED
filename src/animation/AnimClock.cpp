#include "AnimClock.h"

#include <sys/time.h>
#include <time.h>
#include "esp_sntp.h"

// Override with -DNTP_SERVER=\"192.168.1.1\" to use a LAN time server.
#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

namespace {
    uint32_t frameNow = 0;
    uint32_t frameTimebase = 0;

    uint32_t readSharedMillis() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return (uint32_t)((uint64_t)tv.tv_sec * 1000ULL + (uint64_t)tv.tv_usec / 1000ULL);
    }
}

void AnimClock::begin() {
    // Slew later corrections gradually instead of stepping the clock, so
    // resyncs don't make animations jump (the first sync, from 1970, steps).
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
    // Board crystals drift apart by tens of ms per hour; resync every 15 min
    // (the core's default is every 3 hours).
    sntp_set_sync_interval(15UL * 60UL * 1000UL);
    configTime(0, 0, NTP_SERVER);
}

void AnimClock::beginFrame() {
    const uint32_t local = millis();
    frameNow = readSharedMillis();
    // FastLED's beat functions compute (millis() - timebase).
    frameTimebase = local - frameNow;
}

uint32_t AnimClock::now() { return frameNow; }

uint32_t AnimClock::timebase() { return frameTimebase; }

bool AnimClock::synced() {
    // Any date after 2020 means SNTP has set the clock at least once since
    // power-up (system time survives soft resets such as OTA reboots).
    return time(nullptr) > 1577836800;
}
