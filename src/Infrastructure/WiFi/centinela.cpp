#include "Infrastructure/WiFi/centinela.h"
#include "Infrastructure/Input/encoder_input.h"
#include <WiFi.h>          
#include <esp_wifi.h>      
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern bool runningApp;

bool attack_confirmed = false; 

static unsigned long attack_packets = 0;   
static unsigned long last_sec_packets = 0; 
static unsigned long last_check_time = 0;
static int selected_ch = 1;
static bool channel_fixed = false;
static uint32_t last_packet_time = 0;

void centinela_sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return; 
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t* payload = pkt->payload;

    if (payload[0] != 0x80 && payload[0] != 0x40) {
        attack_packets++;
        last_sec_packets++; 
        last_packet_time = millis();
    }
}

void runCentinelaSetup() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    wifi_promiscuous_filter_t my_filter = { .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT };
    esp_wifi_set_promiscuous_filter(&my_filter);
    esp_wifi_set_promiscuous_rx_cb(centinela_sniffer_callback);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(selected_ch, WIFI_SECOND_CHAN_NONE);
}

void centinelaLoop() {
    if (!channel_fixed) {
        int step = encStep();
        if (step != 0) {
            selected_ch = ((selected_ch - 1 + step) % 13);
            if (selected_ch < 0) selected_ch += 13;
            selected_ch += 1;
        }

        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(15, 12, "MODO CENTINELA V3.3");
        u8g2.drawStr(10, 35, "Canal de Escucha:");
        u8g2.setCursor(55, 58);
        u8g2.setFont(u8g2_font_10x20_tr);
        u8g2.print(selected_ch);
        u8g2.sendBuffer();

        if (encOk()) { runCentinelaSetup(); channel_fixed = true; attack_packets = 0; encClear(); }
        if (encBack()) { runningApp = false; encClear(); }
        return;
    }

    unsigned long now = millis();
    static int current_bar_width = 0;

    if (now - last_check_time > 1000) {
        if (last_sec_packets > 80) attack_confirmed = true;
        else if (now - last_packet_time > 2000) attack_confirmed = false;
        
        if (last_sec_packets < 5) current_bar_width = 0;
        else current_bar_width = map(last_sec_packets, 5, 250, 8, 118);
        
        if (current_bar_width > 118) current_bar_width = 118;

        last_sec_packets = 0; 
        last_check_time = now;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);

    if (attack_confirmed) {
        u8g2.drawBox(0, 0, 128, 14); 
        u8g2.setDrawColor(0);        
        u8g2.drawStr(12, 11, "!ATAQUE DETECTADO!");
        u8g2.setDrawColor(1);        
    } else {
        u8g2.drawStr(5, 11, "CH:"); 
        u8g2.setCursor(25, 11); u8g2.print(selected_ch);
        u8g2.drawStr(45, 11, "|ESTADO: OK");
        u8g2.drawHLine(0, 13, 128);
    }

    u8g2.setCursor(5, 28);
    u8g2.print("Pkts: "); u8g2.print(attack_packets);
    
    u8g2.setCursor(5, 40);
    u8g2.print("Intensidad RF:");

    u8g2.drawFrame(5, 45, 118, 10); 
    if (current_bar_width > 0) {
        u8g2.drawBox(5, 45, current_bar_width, 10); 
    }

    u8g2.setCursor(5, 62);
    if (attack_confirmed) u8g2.print("ALERTA DE SEGURIDAD");
    else u8g2.print("Vigilando frecuencia...");

    u8g2.sendBuffer();

    if (encOk() || encBack()) {
        esp_wifi_set_promiscuous(false);
        channel_fixed = false;
        encClear();
    }
}
