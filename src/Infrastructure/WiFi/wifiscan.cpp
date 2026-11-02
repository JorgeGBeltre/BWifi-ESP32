#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include "Infrastructure/WiFi/wifiscan.h"
#include "Infrastructure/Input/encoder_input.h"

extern String target_ssid;
extern int target_channel;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

int selectedNetwork = 0;
int totalNetworks = 0;
bool viewingDetails = false;
int detailScrollY = 0;
int maxScrollDetails = 0;
int marqueeOffset = 0;

void drawWiFiWaves(int radius) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(25, 12, "BUSCANDO REDES");
    u8g2.drawDisc(64, 55, 2);
    for (int i = 0; i < 3; i++) {
        int r = (radius + (i * 15)) % 45;
        if (r > 2) u8g2.drawCircle(64, 55, r, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
    }
    u8g2.sendBuffer();
}

// función para conectar desde el Scanneer
void attemptConnection(String ssid) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(10, 25, "CONECTANDO A:");
    u8g2.setCursor(10, 40); u8g2.print(ssid);
    u8g2.drawFrame(10, 50, 108, 6);
    u8g2.sendBuffer();

    WiFi.begin(ssid.c_str());
    
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        u8g2.drawBox(12, 52, (timeout * 104) / 20, 2);
        u8g2.sendBuffer();
        timeout++;
    }

    u8g2.clearBuffer();
    if (WiFi.status() == WL_CONNECTED) {
        u8g2.drawRFrame(5, 15, 118, 40, 5);
        u8g2.drawStr(25, 35, "¡CONECTADO!");
        u8g2.setCursor(20, 48); u8g2.print(WiFi.localIP());
        target_ssid = ssid; // Guardamos para IP Scanner
    } else {
        u8g2.drawStr(20, 35, "ERROR: TIEMPO AGOTADO");
    }
    u8g2.sendBuffer();
    delay(2000);
    encClear();
}

void wifiscanSetup() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    for(int r = 0; r < 45; r += 3) { drawWiFiWaves(r); delay(20); }
    totalNetworks = WiFi.scanNetworks(false, true, false, 300);
    selectedNetwork = 0;
    viewingDetails = false;
    detailScrollY = 0;
    marqueeOffset = 0;
    encClear();
}

void wifiscanLoop() {
    if (viewingDetails && encBack()) {
        viewingDetails = false;
        detailScrollY = 0;
        return;
    }

    u8g2.clearBuffer();
    
    if (!viewingDetails) {
        int step = encStep();
        if (totalNetworks > 0) {
            if (step != 0) {
                selectedNetwork = (selectedNetwork + step) % totalNetworks;
                if (selectedNetwork < 0) selectedNetwork += totalNetworks;
                marqueeOffset = 0;
            }
            if (encOk()) { viewingDetails = true; detailScrollY = 0; }
        } else if (encOk()) {
            wifiscanSetup();
            return;
        }

        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(0, 10, "REDES ENCONTRADAS:");
        
        if (totalNetworks == 0) {
            u8g2.drawStr(10, 35, "No hay redes...");
            u8g2.drawStr(10, 50, "OK: Re-escanear");
        } else {
            static unsigned long lastMarqueeUpdate = 0;
            if (millis() - lastMarqueeUpdate > 150) {
                marqueeOffset++;
                lastMarqueeUpdate = millis();
            }

            for (int i = 0; i < 4; i++) {
                int idx = (selectedNetwork / 4 * 4) + i;
                if (idx < totalNetworks) {
                    int y = 25 + (i * 12);
                    u8g2.setCursor(0, y);
                    u8g2.print(idx == selectedNetwork ? ">" : " ");

                    u8g2.setFont(u8g2_font_open_iconic_all_1x_t);
                    if (WiFi.encryptionType(idx) == WIFI_AUTH_OPEN) u8g2.drawGlyph(10, y, 122); 
                    else u8g2.drawGlyph(10, y, 123);

                    u8g2.setFont(u8g2_font_6x10_tr);
                    String ssid = WiFi.SSID(idx);
                    
                    if (idx == selectedNetwork && ssid.length() > 10) {
                        int len = ssid.length();
                        int shift = marqueeOffset % (len + 5);
                        String displaySSID = (ssid + "     " + ssid).substring(shift, shift + 10);
                        u8g2.setCursor(20, y); u8g2.print(displaySSID);
                    } else {
                        u8g2.setCursor(20, y); 
                        u8g2.print(ssid.length() > 10 ? ssid.substring(0, 9) + ".." : ssid);
                    }

                    int rssi = WiFi.RSSI(idx);
                    u8g2.setCursor(88, y); u8g2.print(rssi);

                    int bars = (rssi > -55) ? 4 : (rssi > -70) ? 3 : (rssi > -85) ? 2 : 1;
                    for (int b = 0; b < 4; b++) {
                        int barH = (b + 1) * 2;
                        if (b < bars) u8g2.drawBox(115 + (b * 3), y, 2, -barH);
                        else u8g2.drawFrame(115 + (b * 3), y, 2, -barH);
                    }
                }
            }
        }
    } else {
        // --- PANTALLA DE DETALLES ---
        int totalLines = 6;
        maxScrollDetails = -((totalLines * 12) - 30);

        int step = encStep();
        if (step != 0) {
            detailScrollY = constrain(detailScrollY - (step * 8), maxScrollDetails, 0);
        }

        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(0, 12, "DETALLES DE RED:");
        u8g2.drawHLine(0, 15, 128);

        int yOff = 28 + detailScrollY;
        u8g2.setFont(u8g2_font_6x10_tr);

        auto printLine = [&](String label, String value) {
            if (yOff > 16 && yOff < 52) { 
                u8g2.setCursor(0, yOff); 
                u8g2.print(label);
                u8g2.print(value);
            }
            yOff += 12;
        };

        String ssid = WiFi.SSID(selectedNetwork);
        bool isOpen = (WiFi.encryptionType(selectedNetwork) == WIFI_AUTH_OPEN);

        printLine("SSID: ", ssid);
        printLine("BSSID: ", WiFi.BSSIDstr(selectedNetwork));
        printLine("CH: ", String(WiFi.channel(selectedNetwork)));
        printLine("RSSI: ", String(WiFi.RSSI(selectedNetwork)) + " dBm");
        
        String auth;
        switch(WiFi.encryptionType(selectedNetwork)) {
            case WIFI_AUTH_OPEN: auth = "Abierta"; break;
            case WIFI_AUTH_WPA2_PSK: auth = "WPA2-PSK"; break;
            default: auth = "Protegida"; break;
        }
        printLine("SEG: ", auth);

        // --- BARRA INFERIOR DE ACCIONES (ADAPTADA A ENCODER HW-040) ---
        u8g2.setDrawColor(0); u8g2.drawBox(0, 52, 128, 12); u8g2.setDrawColor(1);
        u8g2.drawHLine(0, 52, 128);
        u8g2.setFont(u8g2_font_5x7_tr);
        
        if (isOpen) {
            u8g2.drawStr(5, 61, "ABIERTA: [OK] CONECTAR");
            if (encOk()) {
                attemptConnection(ssid);
                viewingDetails = false;
            }
        } else {
            u8g2.drawStr(5, 61, "OK: CLONAR | HOLD: VOLVER");
            if (encOk() || encAux()) {
                target_ssid = ssid;
                target_channel = WiFi.channel(selectedNetwork);
                u8g2.clearBuffer();
                u8g2.drawStr(20, 35, "¡DATOS COPIADOS!");
                u8g2.sendBuffer();
                delay(1200);
                encClear();
                viewingDetails = false;
            }
        }
    }

    u8g2.sendBuffer();
}
