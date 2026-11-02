#include "Infrastructure/System/about_info.h"
#include "Infrastructure/Input/encoder_input.h"
#include <U8g2lib.h>
#include <esp_system.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern bool runningApp;
RTC_DATA_ATTR uint32_t g_bootCount = 0;

static int about_scroll = 0;
struct MatrixDrop {
    int x;
    float y;
    float speed;
    char val;
};

static MatrixDrop drops[12];

static const char *resetReasonName(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON: return "POWER_ON";
        case ESP_RST_SW:      return "SW_RESET";
        case ESP_RST_PANIC:   return "EXCEPTION";
        case ESP_RST_WDT:     return "WATCHDOG";
        case ESP_RST_BROWNOUT:return "BROWNOUT";
        default:              return "NORMAL";
    }
}

void initMatrix() {
    for (int i = 0; i < 12; i++) {
        drops[i].x = random(0, 128);
        drops[i].y = (float)random(-64, 0);
        drops[i].speed = (float)random(10, 32) / 10.0f;
        drops[i].val = (random(0, 2) == 0) ? (char)random(48, 58) : (char)random(65, 71);
    }
}

void aboutLoop() {
    static bool firstRun = true;
    if (firstRun) {
        initMatrix();
        firstRun = false;
    }

    if (encBack() || encOk()) {
        about_scroll = 0;
        firstRun = true;
        runningApp = false;
        encClear();
        return;
    }

    int step = encStep();
    if (step != 0) {
        about_scroll = constrain(about_scroll + (step * 10), 0, 110);
    }

    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.setDrawColor(1);
    for (int i = 0; i < 12; i++) {
        u8g2.drawGlyph(drops[i].x, (int)drops[i].y, drops[i].val);
        drops[i].y += drops[i].speed;
        if (drops[i].y > 64) {
            drops[i].y = (float)random(-20, 0);
            drops[i].x = random(0, 128);
            drops[i].val = (random(0, 2) == 0) ? (char)random(48, 58) : (char)random(65, 71);
        }
    }

    u8g2.setDrawColor(0);
    u8g2.drawBox(8, 18, 112, 44);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(8, 18, 112, 44);

    u8g2.setFont(u8g2_font_5x7_tr);
    int textX = 13;
    char ramLine[28];
    char bootLine[28];
    snprintf(ramLine, sizeof(ramLine), "Free RAM: %u KB", ESP.getFreeHeap() / 1024);
    snprintf(bootLine, sizeof(bootLine), "Boots:%lu Rst:%s", (unsigned long)g_bootCount, resetReasonName(esp_reset_reason()));

    u8g2.drawStr(textX, 30 - about_scroll, "System: BWifiKill v3.5");
    u8g2.drawStr(textX, 42 - about_scroll, "HW: ESP32 + 2x nRF24+");
    u8g2.drawStr(textX, 54 - about_scroll, "Input: Encoder HW-040");
    u8g2.drawStr(textX, 66 - about_scroll, ramLine);
    u8g2.drawStr(textX, 78 - about_scroll, bootLine);
    u8g2.drawStr(textX, 90 - about_scroll, "Build: " __DATE__);
    u8g2.drawStr(textX, 102 - about_scroll, "Mode: Dual RF + BLE5");
    u8g2.drawStr(textX, 114 - about_scroll, "Author: Jorge Beltre");

    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 0, 128, 17);
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(5, 12, "> TERMINAL_INFO");
    u8g2.drawHLine(0, 16, 128);

    if ((millis() / 450) % 2 == 0)
        u8g2.drawBox(110, 4, 6, 9);

    u8g2.sendBuffer();
}
