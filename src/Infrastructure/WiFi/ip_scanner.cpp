#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <WiFiUdp.h>
#include <U8g2lib.h>
#include <time.h>
#include "Infrastructure/WiFi/ip_scanner.h"
#include "Infrastructure/Input/encoder_input.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern bool runningApp;

String found_ips[20];
int ip_count = 0;
int ip_selection = 0;
int action_selection = 0;
bool scanning = false;
bool in_action_menu = false;
bool is_attacking = false;
int current_scan_num = 1;
WiFiUDP udp;
static bool s_ntpRequested = false;

String getDeviceRole(IPAddress ip) {
    WiFiClient client;
    if (client.connect(ip, 62078, 120) || client.connect(ip, 7000, 120)) { client.stop(); return "Apple (iOS/Mac)"; }
    if (client.connect(ip, 135, 120) || client.connect(ip, 445, 120)) { client.stop(); return "PC Windows"; }
    if (client.connect(ip, 8008, 120) || client.connect(ip, 8009, 120)) { client.stop(); return "Android/Google"; }
    if (client.connect(ip, 80, 120) || client.connect(ip, 443, 120)) { client.stop(); return "Servidor/Router"; }
    if (client.connect(ip, 5000, 120)) { client.stop(); return "Samsung/NAS"; }
    return "Genérico/Activo";
}

void ipScannerSetup() {
    ip_count = 0;
    ip_selection = 0;
    action_selection = 0;
    current_scan_num = 1;
    in_action_menu = false;
    is_attacking = false;
    scanning = (WiFi.status() == WL_CONNECTED);
    if (scanning && !s_ntpRequested) {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        s_ntpRequested = true;
    }
    encClear();
}

void ipScannerLoop() {
    u8g2.clearBuffer();

    if (WiFi.status() != WL_CONNECTED) {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(15, 25, "! SIN CONEXION !");
        u8g2.drawStr(10, 45, "Usa WIFI SCAN primero");
        if (encBack()) runningApp = false;
        u8g2.sendBuffer();
        return;
    }

    if (scanning) {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(10, 12, "ESCANEANDO RED...");
        
        IPAddress local = WiFi.localIP();
        IPAddress target(local[0], local[1], local[2], current_scan_num);

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.setCursor(10, 28);
        u8g2.print("Probando: "); u8g2.print(target.toString());

        int progress = map(current_scan_num, 1, 254, 0, 108);
        u8g2.drawFrame(10, 35, 108, 8);
        u8g2.drawBox(10, 35, progress, 8);

        u8g2.setCursor(10, 55);
        u8g2.print("Hallados: "); u8g2.print(ip_count);
        u8g2.drawStr(70, 55, "[HOLD: PARAR]");
        u8g2.sendBuffer();

        if (Ping.ping(target, 1)) {
            if (ip_count < 20) {
                found_ips[ip_count] = target.toString();
                if (target == WiFi.gatewayIP()) found_ips[ip_count] += " (GW)";
                ip_count++;
            }
        }
        current_scan_num++;
        if (current_scan_num > 254 || encBack()) {
            scanning = false;
            encClear();
        }
        return;
    }

    if (!in_action_menu) {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(2, 9, "EQUIPOS:");
        u8g2.setCursor(55, 9); u8g2.print(ip_count);
        u8g2.drawHLine(0, 11, 128);

        if (ip_count == 0) {
            u8g2.drawStr(10, 35, "Ninguno hallado");
            if (encOk()) ipScannerSetup();
            if (encBack()) runningApp = false;
        } else {
            int d = encStep();
            if (d != 0) {
                ip_selection = (ip_selection + d) % ip_count;
                if (ip_selection < 0) ip_selection += ip_count;
            }
            if (encOk()) { in_action_menu = true; action_selection = 0; encClear(); }
            if (encBack()) { runningApp = false; encClear(); return; }

            int start_idx = (ip_selection / 4) * 4;
            for (int i = 0; i < 4; i++) {
                int idx = start_idx + i;
                if (idx >= ip_count) break;
                int y = 23 + (i * 10);
                if (idx == ip_selection) {
                    u8g2.drawRBox(0, y - 8, 128, 10, 2);
                    u8g2.setDrawColor(0);
                    u8g2.drawStr(5, y, found_ips[idx].c_str());
                    u8g2.setDrawColor(1);
                } else {
                    u8g2.drawStr(5, y, found_ips[idx].c_str());
                }
            }
        }
    } else {
        String cleanIP = found_ips[ip_selection];
        if (cleanIP.indexOf(" ") != -1) cleanIP = cleanIP.substring(0, cleanIP.indexOf(" "));
        IPAddress targetIP;
        targetIP.fromString(cleanIP);

        if (!is_attacking) {
            u8g2.setFont(u8g2_font_6x10_tr);
            u8g2.setCursor(5, 10); u8g2.print("OBJ: "); u8g2.print(cleanIP);
            u8g2.drawHLine(0, 12, 128);

            const int kActCount = 5;
            const char* actions[kActCount] = {
                "1. PING TEST",
                "2. UDP FLOOD",
                "3. PORT SCAN",
                "4. DHCP AUDIT + NTP",
                "5. <- VOLVER"
            };
            int d = encStep();
            if (d != 0) {
                action_selection = (action_selection + d) % kActCount;
                if (action_selection < 0) action_selection += kActCount;
            }

            int firstVis = action_selection - 2;
            if (firstVis > kActCount - 4) firstVis = kActCount - 4;
            if (firstVis < 0) firstVis = 0;

            u8g2.setFont(u8g2_font_5x7_tr);
            for (int row = 0; row < 4; row++) {
                int i = firstVis + row;
                int y = 24 + (row * 10);
                if (i == action_selection) {
                    u8g2.drawBox(5, y - 7, 118, 9);
                    u8g2.setDrawColor(0);
                    u8g2.drawStr(10, y, actions[i]);
                    u8g2.setDrawColor(1);
                } else {
                    u8g2.drawStr(10, y, actions[i]);
                }
            }

            if (encOk()) {
                if (action_selection == 4) in_action_menu = false;
                else is_attacking = true;
                encClear();
            }
            if (encBack()) { in_action_menu = false; encClear(); }
        } else {
            u8g2.setFont(u8g2_font_6x10_tr);
            if (action_selection == 0) {
                u8g2.drawStr(10, 15, "EJECUTANDO PING...");
                if (Ping.ping(targetIP, 1)) {
                    u8g2.drawStr(10, 35, "Respuesta: OK!");
                    u8g2.setFont(u8g2_font_5x7_tr);
                    u8g2.setCursor(10, 48);
                    u8g2.print("Tiempo: "); u8g2.print(Ping.averageTime()); u8g2.print(" ms");
                } else {
                    u8g2.drawStr(10, 35, "ESTADO: PERDIDO");
                }
            } else if (action_selection == 1) {
                u8g2.drawStr(10, 15, "ATAQUE UDP ACTIVO!");
                u8g2.setFont(u8g2_font_5x7_tr);
                u8g2.setCursor(10, 30); u8g2.print("Destino: "); u8g2.print(cleanIP);
                u8g2.drawStr(10, 45, "Enviando rafagas...");
                
                for(int i=0; i<15; i++) {
                    udp.beginPacket(targetIP, 80);
                    udp.printf("FLOOD_TEST_PACKET_%d", i);
                    udp.endPacket();
                }
            } else if (action_selection == 2) {
                u8g2.drawStr(10, 15, "ANALIZANDO PUERTOS");
                u8g2.drawStr(10, 30, "Detectado:");
                String role = getDeviceRole(targetIP);
                u8g2.setFont(u8g2_font_6x10_tr);
                u8g2.drawStr(10, 45, role.c_str());
            } else if (action_selection == 3) {
                u8g2.drawStr(6, 12, "AUDITORIA DHCP & NTP");
                u8g2.drawHLine(0, 14, 128);
                u8g2.setFont(u8g2_font_5x7_tr);
                u8g2.setCursor(4, 25);
                u8g2.print("GW/DHCP: "); u8g2.print(WiFi.gatewayIP());
                u8g2.setCursor(4, 35);
                u8g2.print("DNS:     "); u8g2.print(WiFi.dnsIP());
                bool dnsMatch = (WiFi.dnsIP() == WiFi.gatewayIP()) || (WiFi.dnsIP()[0] == 8) || (WiFi.dnsIP()[0] == 1);
                u8g2.setCursor(4, 45);
                u8g2.print(dnsMatch ? "Estado DHCP: LEGITIMO" : "! ALERTA: DNS EXTERNO !");
                time_t nowSec = time(nullptr);
                u8g2.setCursor(4, 54);
                if (nowSec > 1700000000) {
                    struct tm *ti = gmtime(&nowSec);
                    u8g2.printf("NTP UTC: %02d:%02d:%02d OK", ti->tm_hour, ti->tm_min, ti->tm_sec);
                } else {
                    configTime(0, 0, "pool.ntp.org");
                    u8g2.print("NTP: Sincronizando...");
                }
            }

            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(15, 63, "[OK/HOLD: DETENER]");
            if (encOk() || encBack()) { is_attacking = false; encClear(); }
        }
    }
    u8g2.sendBuffer();
}
