#include "Infrastructure/Display/U8g2DisplayController.h"
#include "Infrastructure/Input/encoder_input.h"
#include "Infrastructure/Display/gui_helper.h"
#include "Infrastructure/Config/settings_menu.h"
#include <Arduino.h>

namespace bwifi::infra {

U8g2DisplayController::U8g2DisplayController(U8G2_SSD1306_128X64_NONAME_F_HW_I2C& display,
                                             uint32_t busClockHz)
    : display_(display), busClockHz_(busClockHz) {}

void U8g2DisplayController::begin() {
    display_.setBusClock(busClockHz_);
    display_.begin();
}

void U8g2DisplayController::applySettings(const domain::DeviceSettings& settings) {
    display_.setContrast(domain::contrastByteForIndex(settings.brightnessIdx));
    ::encSetInvertDir(settings.invertEncoder);
    display_.sendF("c", (settings.themeMode == domain::UiTheme::Inverse) ? 0xA7 : 0xA6);
}

void U8g2DisplayController::setPowerSave(bool enabled) {
    display_.setPowerSave(enabled ? 1 : 0);
}

void U8g2DisplayController::renderSplashProgress(int percent) {
    display_.clearBuffer();
    display_.setFont(u8g2_font_10x20_tr);
    display_.drawStr(16, 28, "BWifi 3.5");
    display_.setFont(u8g2_font_6x12_tr);
    display_.drawStr(24, 44, "MULTI-TOOL RF");
    display_.drawHLine(24, 48, 80);
    display_.drawFrame(14, 58, 100, 3);
    display_.drawBox(16, 59, (percent * 96) / 100, 1);
    display_.sendBuffer();
}

void U8g2DisplayController::renderCarouselMenu() {
    ::drawBruceMenu();
}

void U8g2DisplayController::renderToast(const char* message, uint32_t durationMs) {
    display_.setFont(u8g2_font_5x7_tr);
    display_.drawRBox(18, 22, 92, 18, 3);
    display_.setDrawColor(0);
    display_.drawStr(24, 34, message);
    display_.setDrawColor(1);
    display_.sendBuffer();
    ::delay(durationMs);
}

void U8g2DisplayController::runBootGuards() {
    ::settingsRunBootGuards();
}

bool U8g2DisplayController::saveScreenshotBmp(const char* path) {
    return ::saveOledScreenshotBmp(path);
}

} // namespace bwifi::infra

