#include "Infrastructure/Display/gui_helper.h"
#include "Application/Services/CarouselKinematicsService.h"
#include "Infrastructure/Input/encoder_input.h"
#include <U8g2lib.h>
#include <math.h>
#include <stdio.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern int menu_index;
extern float currentPos;
extern int TOTAL_OPTIONS;
extern const char* menu_labels[];
extern bool attack_confirmed; 

// --- COLECCIÓN DE ICONOS ÚNICOS ---
void drawBolt(int x, int y) { u8g2.drawLine(x+22, y+2, x+10, y+16); u8g2.drawLine(x+10, y+16, x+24, y+16); u8g2.drawLine(x+24, y+16, x+12, y+30); }
void drawFlame(int x, int y) { u8g2.drawTriangle(x+16, y+2, x+8, y+20, x+24, y+20); u8g2.drawDisc(x+16, y+22, 8); u8g2.setDrawColor(0); u8g2.drawDisc(x+16, y+24, 4); u8g2.setDrawColor(1); }
void drawBan(int x, int y) { u8g2.drawCircle(x+16, y+16, 13); u8g2.drawLine(x+7, y+25, x+25, y+7); }
void drawBTJam(int x, int y) { u8g2.drawLine(x+12, y+6, x+12, y+26); u8g2.drawLine(x+12, y+6, x+20, y+11); u8g2.drawLine(x+20, y+11, x+12, y+16); u8g2.drawLine(x+12, y+16, x+20, y+21); u8g2.drawLine(x+20, y+21, x+12, y+26); u8g2.drawLine(x+6, y+6, x+26, y+26); }
void drawRadar(int x, int y) { u8g2.drawCircle(x+16, y+16, 14); u8g2.drawCircle(x+16, y+16, 7); u8g2.drawLine(x+16, y+2, x+16, y+30); u8g2.drawLine(x+2, y+16, x+30, y+16); u8g2.drawLine(x+16, y+16, x+28, y+5); }
void drawSearch(int x, int y) { u8g2.drawCircle(x+14, y+12, 9); u8g2.drawLine(x+20, y+19, x+28, y+28); }
void drawMegaphone(int x, int y) { u8g2.drawTriangle(x+8, y+16, x+24, y+6, x+24, y+26); u8g2.drawBox(x+4, y+14, 6, 6); }
void drawLock(int x, int y) { u8g2.drawFrame(x+8, y+14, 16, 13); u8g2.drawCircle(x+16, y+14, 7, U8G2_DRAW_UPPER_RIGHT|U8G2_DRAW_UPPER_LEFT); u8g2.drawDisc(x+16, y+21, 2); }
void drawAlien(int x, int y) { u8g2.drawBox(x+6, y+10, 20, 10); u8g2.drawBox(x+4, y+12, 24, 6); u8g2.drawBox(x+8, y+22, 4, 4); u8g2.drawBox(x+20, y+22, 4, 4); u8g2.setDrawColor(0); u8g2.drawBox(x+9, y+13, 4, 4); u8g2.drawBox(x+19, y+13, 4, 4); u8g2.setDrawColor(1); }
void drawFolder(int x, int y) { u8g2.drawFrame(x+4, y+8, 24, 18); u8g2.drawBox(x+4, y+4, 10, 4); }
void drawInfo(int x, int y) { u8g2.drawCircle(x+16, y+16, 14); u8g2.drawBox(x+15, y+12, 2, 10); u8g2.drawDisc(x+16, y+8, 2); }
void drawSnakeIcon(int x, int y) { u8g2.drawFrame(x+6, y+6, 20, 20); u8g2.drawBox(x+10, y+10, 4, 4); u8g2.drawBox(x+14, y+10, 4, 4); u8g2.drawBox(x+14, y+14, 4, 4); u8g2.drawDisc(x+22, y+22, 2); }
void drawMonitorIcon(int x, int y) { u8g2.drawFrame(x+4, y+6, 24, 20); u8g2.drawLine(x+6, y+20, x+12, y+10); u8g2.drawLine(x+12, y+10, x+18, y+24); u8g2.drawLine(x+18, y+24, x+24, y+14); }
void drawScanIcon(int x, int y) { u8g2.drawCircle(x+14, y+12, 7); u8g2.drawLine(x+18, y+18, x+24, y+24); u8g2.drawHLine(x+4, y+28, 24); }
void drawControlIcon(int x, int y) { u8g2.drawFrame(x+6, y+10, 20, 12); u8g2.drawBox(x+14, y+22, 4, 4); u8g2.drawCircle(x+16, y+16, 3); }
void drawRemoteIcon(int x, int y) { u8g2.drawFrame(x+10, y+4, 12, 24); u8g2.drawBox(x+13, y+7, 6, 4); u8g2.drawBox(x+13, y+13, 2, 2); u8g2.drawBox(x+17, y+13, 2, 2); u8g2.drawBox(x+13, y+17, 6, 2); u8g2.drawDisc(x+16, y+23, 2); }

// --- NUEVOS DISEÑOS EXCLUSIVOS ---
void drawPortalIcon(int x, int y) { // EVIL PORTAL: Puerta con remolino
    u8g2.drawFrame(x+8, y+4, 16, 24); 
    u8g2.drawCircle(x+16, y+14, 4); 
    u8g2.drawCircle(x+16, y+14, 2);
    u8g2.drawHLine(x+4, y+28, 24);
}
void drawDashboardIcon(int x, int y) { // WEB DASHBOARD: Monitor de PC
    u8g2.drawFrame(x+4, y+6, 24, 16); 
    u8g2.drawBox(x+12, y+22, 8, 2); 
    u8g2.drawHLine(x+8, y+24, 16);
    u8g2.drawPixel(x+6, y+8); // Detalle pantalla
}
void drawIPScannerIcon(int x, int y) { // IP SCANNER: Lupa con "red"
    u8g2.drawCircle(x+12, y+12, 8); 
    u8g2.drawLine(x+12, y+8, x+12, y+16);
    u8g2.drawLine(x+8, y+12, x+16, y+12);
    u8g2.drawLine(x+18, y+18, x+26, y+26);
}

void drawCentinelaIcon(int x, int y) {
    if (attack_confirmed && (millis() / 250) % 2 == 0) return;
    u8g2.drawFrame(x+8, y+4, 16, 18);          
    u8g2.drawTriangle(x+8, y+22, x+24, y+22, x+16, y+30); 
    u8g2.drawCircle(x+16, y+14, 4);            
    u8g2.drawDisc(x+16, y+14, 1);              
    u8g2.drawLine(x+4, y+10, x+8, y+10);       
    u8g2.drawLine(x+24, y+10, x+28, y+10);
}

void drawWifiAuditIcon(int x, int y) { // WIFI AUDIT PRO: Antena + barras de espectro
    u8g2.drawDisc(x+10, y+8, 2);
    u8g2.drawLine(x+10, y+10, x+10, y+26);
    u8g2.drawCircle(x+10, y+8, 5, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
    u8g2.drawBox(x+16, y+20, 3, 7);
    u8g2.drawBox(x+20, y+15, 3, 12);
    u8g2.drawBox(x+24, y+10, 3, 17);
}

void drawTrackerIcon(int x, int y) { // BLE TRACKERS: Tag / Radar AirTag
    u8g2.drawCircle(x+16, y+16, 12);
    u8g2.drawCircle(x+16, y+16, 6);
    u8g2.drawDisc(x+16, y+16, 2);
    u8g2.drawLine(x+16, y+2, x+16, y+6);
    u8g2.drawLine(x+16, y+26, x+16, y+30);
    u8g2.drawLine(x+2, y+16, x+6, y+16);
    u8g2.drawLine(x+26, y+16, x+30, y+16);
}

void drawSettingsIcon(int x, int y) { // AJUSTES / TEST: Sliders de configuración
    u8g2.drawHLine(x+5, y+9, 22);
    u8g2.drawBox(x+10, y+6, 4, 7);
    u8g2.drawHLine(x+5, y+17, 22);
    u8g2.drawBox(x+18, y+14, 4, 7);
    u8g2.drawHLine(x+5, y+25, 22);
    u8g2.drawBox(x+12, y+22, 4, 7);
}

static void drawMenuItemIcon(int idx, int x, int y) {
    switch(idx) {
        case 0:  drawScanIcon(x, y); break;      // WIFI SCANNER
        case 1:  drawWifiAuditIcon(x, y); break; // WIFI AUDIT PRO
        case 2:  drawBolt(x, y); break;          // ANALIZADOR
        case 3:  drawSearch(x, y); break;        // BT SCANNER
        case 4:  drawTrackerIcon(x, y); break;   // BLE TRACKERS
        case 5:  drawMonitorIcon(x, y); break;   // PACKET MONITOR
        case 6:  drawCentinelaIcon(x, y); break; // MODO CENTINELA
        case 7:  drawBan(x, y); break;           // JAMMER CANAL
        case 8:  drawRadar(x, y); break;         // BARRIDO TOTAL
        case 9:  drawBTJam(x, y); break;         // BT JAMMER
        case 10: drawMegaphone(x, y); break;     // BEACON SPAM
        case 11: drawFlame(x, y); break;         // BLE SPAM
        case 12: drawAlien(x, y); break;         // MODO HIBRIDO
        case 13: drawPortalIcon(x, y); break;    // EVIL PORTAL
        case 14: drawIPScannerIcon(x, y); break; // IP SCANNER
        case 15: drawControlIcon(x, y); break;   // CONTROL ESCLAVO
        case 16: drawDashboardIcon(x, y); break; // WEB DASHBOARD
        case 17: drawRemoteIcon(x, y); break;    // BT REMOTE
        case 18: drawFolder(x, y); break;        // LEER LOGS
        case 19: drawSnakeIcon(x, y); break;     // ARCADE
        case 20: drawSettingsIcon(x, y); break;  // AJUSTES / TEST
        case 21: drawInfo(x, y); break;          // ABOUT
    }
}

// --- CARRUSEL ROTATIVO DE ALTA FLUIDEZ (3 ICONOS SIMULTÁNEOS + FÍSICA CIRCULAR) ---
void drawBruceMenu() {
    static uint32_t s_lastFrameMs = 0;
    uint32_t nowMs = millis();
    uint32_t dtMs = (s_lastFrameMs == 0) ? 14 : (nowMs - s_lastFrameMs);
    s_lastFrameMs = nowMs;
    if (dtMs > 80) dtMs = 80;

    currentPos = bwifi::app::CarouselKinematicsService::advancePosition(
        menu_index, currentPos, TOTAL_OPTIONS, dtMs);

    u8g2.clearBuffer();

    // 1. Ventana de recorte para el área central del carrusel (evita pisar header/footer)
    u8g2.setClipWindow(3, 14, 124, 49);

    // 2. Renderizar los iconos visibles en el carrusel horizontal con arco suave
    for (int i = 0; i < TOTAL_OPTIONS; i++) {
        float rel = bwifi::app::CarouselKinematicsService::shortestRingDelta(
            (float)i, currentPos, TOTAL_OPTIONS);

        if (fabsf(rel) < 1.65f) {
            int icon_x = 48 + (int)lroundf(rel * 40.0f);
            int arc_drop = (int)lroundf(fabsf(rel) * 3.0f);
            int icon_y = 15 + arc_drop;
            drawMenuItemIcon(i, icon_x, icon_y);
        }
    }

    u8g2.setMaxClipWindow();

    // 3. Marco central de enfoque (con respuesta táctil visual al presionar SW)
    if (encSwPressed()) {
        u8g2.drawRFrame(44, 14, 40, 35, 6);
        u8g2.drawRFrame(45, 15, 38, 33, 5);
    } else {
        u8g2.drawRFrame(45, 15, 38, 33, 5);
    }

    // Indicadores laterales de giro (< y >)
    u8g2.drawTriangle(2, 31, 6, 27, 6, 35);
    u8g2.drawTriangle(125, 31, 121, 27, 121, 35);

    // 4. Encabezado superior con contador de posición [XX/19]
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 0, 128, 13);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(5, 10, "BWifiKill v3.5");

    char idxBuf[10];
    snprintf(idxBuf, sizeof(idxBuf), "%02d/%02d", menu_index + 1, TOTAL_OPTIONS);
    u8g2.drawRBox(93, 1, 31, 11, 2);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(96, 9, idxBuf);
    u8g2.setDrawColor(1);
    u8g2.drawHLine(0, 13, 128);

    // 5. Etiqueta inferior tipo píldora (actualización instantánea al girar el HW-040)
    u8g2.setFont(u8g2_font_6x12_tr);
    int lw = u8g2.getStrWidth(menu_labels[menu_index]);
    int pillW = lw + 10;
    if (pillW > 120) pillW = 120;
    int pillX = 64 - (pillW / 2);
    u8g2.drawRBox(pillX, 50, pillW, 12, 3);
    u8g2.setDrawColor(0);
    u8g2.drawStr(64 - (lw / 2), 60, menu_labels[menu_index]);
    u8g2.setDrawColor(1);

    // 6. Barra de progreso continua en el borde inferior (y = 63)
    int progW = ((menu_index + 1) * 120) / (TOTAL_OPTIONS > 0 ? TOTAL_OPTIONS : 1);
    u8g2.drawHLine(4, 63, progW);

    u8g2.drawRFrame(0, 0, 128, 64, 5);
    u8g2.sendBuffer();
}
