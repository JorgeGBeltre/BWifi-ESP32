#pragma once

#include <U8g2lib.h>
#include "Domain/Interfaces/IDisplayController.h"

namespace bwifi::infra {

/**
 * @brief Concrete IDisplayController adapter wrapping U8G2 SSD1306 128x64 I2C OLED.
 */
class U8g2DisplayController : public domain::IDisplayController {
public:
    explicit U8g2DisplayController(U8G2_SSD1306_128X64_NONAME_F_HW_I2C& display,
                                   uint32_t busClockHz = 800000);

    void begin() override;
    void applySettings(const domain::DeviceSettings& settings) override;
    void setPowerSave(bool enabled) override;

    void renderSplashProgress(int percent) override;
    void renderCarouselMenu() override;
    void renderToast(const char* message, uint32_t durationMs) override;
    void runBootGuards() override;
    bool saveScreenshotBmp(const char* path = "/screen.bmp") override;

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C& display_;
    uint32_t                             busClockHz_;
};

} // namespace bwifi::infra

