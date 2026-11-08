#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// The message scripts/release.py signs for each board image. Must match it
// byte for byte, or no release will verify.
static inline int formatSignedMessage(char* out, size_t outLen, const char* board,
                                      const char* version, uint32_t size, const char* sha256Hex) {
    return snprintf(out, outLen, "esp32-led-firmware\nboard=%s\nversion=%s\nsize=%lu\nsha256=%s\n",
                    board, version, (unsigned long)size, sha256Hex);
}

// Lowercase hex, as Python's hexdigest(); `out` needs 2*n+1 bytes.
static inline void toLowerHex(const uint8_t* bytes, size_t n, char* out) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i]     = digits[bytes[i] >> 4];
        out[2 * i + 1] = digits[bytes[i] & 0x0F];
    }
    out[2 * n] = '\0';
}
