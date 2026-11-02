#pragma once

#include <cstdint>
#include "Domain/Entities/DeviceSettings.h"

namespace bwifi::domain {

/**
 * @brief Abstract port for the 128x64 SSD1306 OLED display, carousel rendering, and BMP screenshots.
 */
class IDisplayController {
public:
    virtual ~IDisplayController() = default;

    virtual void begin() = 0;
    virtual void applySettings(const DeviceSettings& settings) = 0;
    virtual void setPowerSave(bool enabled) = 0;

    virtual void renderSplashProgress(int percent) = 0;
    virtual void renderCarouselMenu() = 0;
    virtual void renderToast(const char* message, uint32_t durationMs) = 0;
    virtual void runBootGuards() = 0;
    virtual bool saveScreenshotBmp(const char* path = "/screen.bmp") = 0;
};

} // namespace bwifi::domain

