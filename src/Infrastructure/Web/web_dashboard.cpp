#include "Infrastructure/Web/web_dashboard.h"
#include <U8g2lib.h>
#include <WiFi.h>

// Referencia externa a la pantalla
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

void webDashboardLoop() {
    uint64_t chipID = ESP.getEfuseMac();
    String apSSID = "BWESP-" + String((uint16_t)(chipID >> 32), DEC) +
                    String((uint32_t)chipID, DEC);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(15, 14, "ADMIN WEB ACTIVO");
    u8g2.drawHLine(0, 18, 128);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(2, 32);
    u8g2.print("SSID: ");
    u8g2.print(apSSID);
    u8g2.setCursor(2, 45);
    u8g2.print("PASS: 12345678");
    u8g2.setCursor(2, 58);
    u8g2.print("IP:   192.168.4.1");
    u8g2.sendBuffer();
}
