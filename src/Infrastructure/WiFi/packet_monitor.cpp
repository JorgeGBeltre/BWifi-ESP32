#include "Infrastructure/WiFi/packet_monitor.h"
#include "Infrastructure/Input/encoder_input.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <U8g2lib.h>

// Referencias a variables globales del main
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern int target_channel;

// Estructuras y variables privadas del módulo
struct PktData { uint8_t total; uint8_t data; }; 
PktData history[128];
volatile uint32_t count_all = 0;
volatile uint32_t count_data = 0;
unsigned long lastPktUpdate = 0;
unsigned long lastHopTime = 0;
bool autoHop = true;

// Callback de interrupción para captura de paquetes
void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    count_all++;
    if (type == WIFI_PKT_DATA) count_data++;
}

void monitorSetup() {
    WiFi.mode(WIFI_STA); 
    WiFi.disconnect();
    wifi_promiscuous_filter_t filt = { .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL };
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
    autoHop = true;
    
    for(int i=0; i<128; i++) { 
        history[i].total = 0; 
        history[i].data = 0; 
    }
    encClear();
}

void monitorLoop() {
    // --- 1. LÓGICA DE CANALES (AUTOMÁTICO O MANUAL CON ENCODER HW-040) ---
    int step = encStep();
    if (step != 0) {
        autoHop = false;
        target_channel = ((target_channel - 1 + step) % 13);
        if (target_channel < 0) target_channel += 13;
        target_channel += 1;
        esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
        count_all = 0;
        count_data = 0;
    } else if (autoHop) {
        if (millis() - lastHopTime > 1500) {
            lastHopTime = millis(); 
            target_channel++; 
            if (target_channel > 13) target_channel = 1;
            esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
        }
    }

    // --- 2. CONTROL DE MODO (AUTO/MANUAL) CON PULSADOR SW (OK) ---
    if (encOk()) {
        autoHop = !autoHop;
    }

    // --- 3. PROCESAMIENTO DE DATOS PARA LA GRÁFICA ---
    if (millis() - lastPktUpdate > 100) {
        lastPktUpdate = millis();
        for (int i = 0; i < 127; i++) history[i] = history[i+1];
        
        history[127].total = map(constrain(count_all, 0, 15), 0, 15, 0, 60);
        history[127].data = map(constrain(count_data, 0, 10), 0, 10, 0, 60);
        
        count_all = 0; 
        count_data = 0;
    }

    // --- 4. DIBUJO EN PANTALLA ---
    u8g2.clearBuffer();
    
    // Dibujar las barras del historial
    for (int i = 0; i < 128; i++) {
        u8g2.drawVLine(i, 63 - history[i].total, history[i].total);
        if (history[i].data > 2) {
            u8g2.drawVLine(i, 63 - history[i].data, history[i].data);
            if(i > 0) u8g2.drawVLine(i-1, 63 - history[i].data, history[i].data);
        }
    }

    // Encabezado con información de canal y modo
    u8g2.setDrawColor(0); 
    u8g2.drawBox(0, 0, 128, 12); 
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_4x6_tf);
    
    u8g2.setCursor(2, 9); 
    u8g2.print("CH:"); u8g2.print(target_channel);
    
    u8g2.setCursor(40, 9); 
    if (autoHop) {
        u8g2.print("MODO: AUTO SCAN");
    } else {
        u8g2.print("MODO: MANUAL (GIRO)");
    }
    
    u8g2.sendBuffer();
}
