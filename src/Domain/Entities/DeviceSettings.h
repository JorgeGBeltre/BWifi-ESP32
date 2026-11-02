#pragma once

#include <cstdint>

namespace bwifi::domain {

enum class OperatingProfile : uint8_t {
    Lab   = 0,
    Audit = 1,
    Demo  = 2
};

enum class UiTheme : uint8_t {
    Classic = 0,
    Inverse = 1,
    Retro   = 2
};

inline const char* operatingProfileToString(OperatingProfile profile) {
    switch (profile) {
        case OperatingProfile::Audit: return "AUDIT";
        case OperatingProfile::Demo:  return "DEMO";
        case OperatingProfile::Lab:
        default:                      return "LAB";
    }
}

inline const char* uiThemeToString(UiTheme theme) {
    switch (theme) {
        case UiTheme::Inverse: return "INVERSO";
        case UiTheme::Retro:   return "RETRO";
        case UiTheme::Classic:
        default:               return "CLASICO";
    }
}

inline uint8_t contrastByteForIndex(uint8_t brightnessIdx) {
    static constexpr uint8_t kContrastTable[4] = {45, 115, 185, 255};
    return kContrastTable[brightnessIdx < 4 ? brightnessIdx : 3];
}

inline uint32_t screensaverTimeoutMsForIndex(uint8_t screensaverIdx) {
    static constexpr uint32_t kScreensaverMs[4] = {0, 30000, 60000, 120000};
    return kScreensaverMs[screensaverIdx % 4];
}

/**
 * @brief Pure domain entity representing persistent NVS device configuration.
 * Decoupled from Arduino/ESP-IDF frameworks to support host-side compilation and testing.
 */
struct DeviceSettings {
    uint8_t          brightnessIdx   = 3;     // 0: 25%, 1: 50%, 2: 75%, 3: 100%
    bool             invertEncoder   = true;  // true = inverted, false = normal
    uint8_t          screensaverIdx  = 0;     // 0: OFF, 1: 30s, 2: 60s, 3: 120s
    OperatingProfile profileMode     = OperatingProfile::Lab;
    bool             pinEnabled      = false; // 4-digit startup PIN (1234)
    bool             showLegalNotice = false; // Ethical/Laboratory notice on boot
    UiTheme          themeMode       = UiTheme::Classic;
};

} // namespace bwifi::domain

