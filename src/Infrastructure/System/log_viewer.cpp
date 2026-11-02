#include <Arduino.h>
#include <U8g2lib.h>
#include "FS.h"
#include "SPIFFS.h"
#include "Infrastructure/Input/encoder_input.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

int logScroll = 0;
int maxLogScroll = 0; 
static bool confirmErase = false;

void logViewerLoop() {
    u8g2.clearBuffer();
    
    // --- ENCABEZADO FIJO ---
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 10, "HISTORIAL DE LOGS");
    u8g2.drawLine(0, 13, 128, 13);

    if (!SPIFFS.begin(true)) {
        u8g2.drawStr(0, 35, "Error SPIFFS");
    } else {
        File file = SPIFFS.open("/logins.txt", FILE_READ);
        if (!file || file.size() == 0) {
            confirmErase = false;
            u8g2.drawStr(0, 35, "No hay registros.");
            u8g2.drawStr(0, 50, "[HOLD] para salir");
        } else {
            // --- NAVEGACIÓN FLUIDA CON ENCODER HW-040 ---
            int step = encStep();
            if (step != 0) {
                if (confirmErase) {
                    confirmErase = false;
                } else {
                    logScroll = constrain(logScroll - (step * 10), maxLogScroll, 0);
                }
            }

            // --- RENDERIZADO CON CLIPPING ---
            int currentY = 25 + logScroll;
            int totalHeight = 0;

            while (file.available()) {
                String line = file.readStringUntil('\n');
                line.trim();

                if (currentY > 15 && currentY < 75) {
                    if (line.length() > 21) {
                        u8g2.setCursor(0, currentY); u8g2.print(line.substring(0, 21));
                        currentY += 10;
                        if (currentY > 15 && currentY < 75) {
                            u8g2.setCursor(0, currentY); u8g2.print(line.substring(21));
                        }
                    } else {
                        u8g2.setCursor(0, currentY); u8g2.print(line);
                    }
                }
                currentY += 12;
                totalHeight += (line.length() > 21) ? 22 : 12;
            }
            maxLogScroll = -(totalHeight - 40); 
            if (maxLogScroll > 0) maxLogScroll = 0;
            file.close();

            // --- CONFIRMACIÓN DE BORRADO CON ENCODER HW-040 ---
            if (confirmErase || encAux()) {
                confirmErase = true;
                u8g2.setDrawColor(0);
                u8g2.drawBox(5, 18, 118, 36);
                u8g2.setDrawColor(1);
                u8g2.drawFrame(5, 18, 118, 36);
                u8g2.drawStr(10, 30, "¿BORRAR REGISTROS?");
                u8g2.drawStr(10, 42, "OK: SI | GIRO: NO");

                if (encBack()) {
                    confirmErase = false;
                    encClear();
                } else if (encOk()) {
                    SPIFFS.remove("/logins.txt");
                    logScroll = 0;
                    confirmErase = false;
                    u8g2.clearBuffer();
                    u8g2.drawStr(30, 35, "LOGS ELIMINADOS");
                    u8g2.sendBuffer();
                    delay(1000);
                    encClear();
                }
            } else if (encOk()) {
                confirmErase = true;
            }
        }
    }

    u8g2.sendBuffer();
}
