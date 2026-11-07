#pragma once

#include <Arduino.h>

// Shared animation clock. Once SNTP has set the time this is epoch
// milliseconds (low 32 bits), so every board computes the same animation phase
// at the same moment and effects line up across devices. Until the first sync
// it is just this board's own time and animations run unsynced.
//
// For an animation to stay in sync:
//  - derive all motion from now(), never from millis() or from accumulators
//    integrated frame by frame (use beatsinIntegral16 in AnimMath.h);
//  - pass timebase() to FastLED's beat*()/beatsin*() helpers, which otherwise
//    read this board's millis().
namespace AnimClock {
    // Starts SNTP. Call once at boot; it retries and resyncs on its own.
    void begin();
    // Latches the clock for one frame so every strip renders the same instant.
    void beginFrame();
    // Shared time of the current frame, in ms.
    uint32_t now();
    // Timebase argument that makes FastLED's beat functions follow now().
    uint32_t timebase();
    // True once SNTP has set the clock; until then boards are not in sync.
    bool synced();
}
