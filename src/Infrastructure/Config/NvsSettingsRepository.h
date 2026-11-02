#pragma once

#include "Domain/Interfaces/ISettingsRepository.h"
#include "Infrastructure/Config/settings_menu.h"

namespace bwifi::infra {

/**
 * @brief Adapter implementing domain::ISettingsRepository over ESP32 Preferences (NVS) and SPIFFS.
 * Synchronizes cleanly between pure domain::DeviceSettings and AppSettings g_settings.
 */
class NvsSettingsRepository : public domain::ISettingsRepository {
public:
    void load(domain::DeviceSettings& out) override {
        ::settingsLoad();
        out = toDomain(::g_settings);
    }

    void save(const domain::DeviceSettings& settings) override {
        ::g_settings = fromDomain(settings);
        ::settingsSave();
    }

    void rotateLogIfNeeded(const char* path, size_t maxBytes = 24576) override {
        ::rotateLogIfNeeded(path, maxBytes);
    }

    static domain::DeviceSettings toDomain(const AppSettings& s) {
        domain::DeviceSettings d;
        d.brightnessIdx   = s.brightnessIdx;
        d.invertEncoder   = s.invertEncoder;
        d.screensaverIdx  = s.screensaverIdx;
        d.profileMode     = (s.profileMode <= 2)
                                ? static_cast<domain::OperatingProfile>(s.profileMode)
                                : domain::OperatingProfile::Lab;
        d.pinEnabled      = s.pinEnabled;
        d.showLegalNotice = s.showLegalNotice;
        d.themeMode       = (s.themeMode <= 2)
                                ? static_cast<domain::UiTheme>(s.themeMode)
                                : domain::UiTheme::Classic;
        return d;
    }

    static AppSettings fromDomain(const domain::DeviceSettings& d) {
        AppSettings s;
        s.brightnessIdx   = d.brightnessIdx;
        s.invertEncoder   = d.invertEncoder;
        s.screensaverIdx  = d.screensaverIdx;
        s.profileMode     = static_cast<uint8_t>(d.profileMode);
        s.pinEnabled      = d.pinEnabled;
        s.showLegalNotice = d.showLegalNotice;
        s.themeMode       = static_cast<uint8_t>(d.themeMode);
        return s;
    }
};

} // namespace bwifi::infra

