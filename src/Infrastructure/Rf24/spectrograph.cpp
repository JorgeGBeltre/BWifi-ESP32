#include <Arduino.h>
#include <U8g2lib.h>
#include <RF24.h>
#include "Infrastructure/Input/encoder_input.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern RF24 jam1;
extern RF24 jam2;
extern bool runningApp;

#define CHANNELS 125
uint8_t values[CHANNELS];
uint8_t peak_heights[CHANNELS]; 

#define WF_ROWS 36
#define WF_BYTES_PER_ROW 16
static uint8_t s_waterfall[WF_ROWS][WF_BYTES_PER_ROW];
// 0: Espectro Barras, 1: Waterfall 2D, 2: RF Spectrum Fingerprint, 3: MouseJack / ESB Sniffer + Replay
static int s_rfViewMode = 0;

// Variables para MouseJack / ESB Sniffer + Replay (LAB)
static uint8_t s_esbLastCh = 24;
static uint8_t s_esbLastFrame[16];
static bool s_esbHasCapture = false;
static uint32_t s_esbFrameCount = 0;
static uint32_t s_esbReplayCount = 0;

const int UMBRAL_ENERGIA_TOTAL = 800; 
const int REPETICIONES_LECTURA = 40;  

static void configureSpectrumListen() {
    jam1.begin();
    jam1.setAutoAck(false);
    jam1.setDataRate(RF24_1MBPS);
    jam1.disableCRC();
    jam1.startListening();

    jam2.begin();
    jam2.setAutoAck(false);
    jam2.setDataRate(RF24_1MBPS);
    jam2.disableCRC();
    jam2.startListening();
}

static void configureEsbSnifferMode() {
    const uint8_t esbPromiscAddr[3] = {0xAA, 0x00, 0x00};
    jam1.begin();
    jam1.stopListening();
    jam1.setAutoAck(false);
    jam1.setDataRate(RF24_2MBPS);
    jam1.disableCRC();
    jam1.setAddressWidth(3);
    jam1.setPayloadSize(16);
    jam1.openReadingPipe(1, esbPromiscAddr);
    jam1.openWritingPipe(esbPromiscAddr);
    jam1.startListening();
}

void spectrographSetup() {
    s_rfViewMode = 0;
    configureSpectrumListen();
    memset(values, 0, sizeof(values));
    memset(peak_heights, 0, sizeof(peak_heights));
    memset(s_waterfall, 0, sizeof(s_waterfall));
}

static void runMouseJackEsbLoop() {
    // Barrer canales típicos de ratones/teclados inalámbricos 2.4 GHz (2..84)
    for (int step = 0; step < 14; step++) {
        uint8_t ch = 2 + ((s_esbLastCh + step * 3) % 82);
        jam1.setChannel(ch);
        delayMicroseconds(260);
        if (jam1.testRPD() && jam1.available()) {
            uint8_t candidate[16];
            jam1.read(candidate, 16);
            // Filtrar ruido puro (debe tener entropía / bytes no todos 0x00 ni 0xFF)
            int nonZero = 0, nonFF = 0;
            for (int k = 0; k < 10; k++) {
                if (candidate[k] != 0x00) nonZero++;
                if (candidate[k] != 0xFF) nonFF++;
            }
            if (nonZero >= 3 && nonFF >= 3) {
                memcpy(s_esbLastFrame, candidate, 16);
                s_esbLastCh = ch;
                s_esbHasCapture = true;
                s_esbFrameCount++;
            }
        }
    }

    // Gesto secundario (Mantener SW + Girar o botón Aux): Replay de la última trama ESB capturada
    if (encAux() && s_esbHasCapture) {
        jam1.stopListening();
        jam1.setChannel(s_esbLastCh);
        for (int r = 0; r < 5; r++) {
            jam1.write(s_esbLastFrame, 16);
        }
        jam1.startListening();
        s_esbReplayCount++;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setDrawColor(0);
    u8g2.drawStr(6, 9, "MOUSEJACK / ESB LAB");
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(2, 22);
    u8g2.printf("ESB Capturas:%lu | Replay:%lu", (unsigned long)s_esbFrameCount, (unsigned long)s_esbReplayCount);

    if (!s_esbHasCapture) {
        u8g2.drawStr(2, 35, "Escuchando 2Mbps ESB...");
        u8g2.drawStr(2, 46, "Mueve raton/mando 2.4GHz");
    } else {
        u8g2.setCursor(2, 33);
        u8g2.printf("CH:%02d ADDR:%02X:%02X:%02X:%02X:%02X",
                    s_esbLastCh,
                    s_esbLastFrame[0], s_esbLastFrame[1], s_esbLastFrame[2],
                    s_esbLastFrame[3], s_esbLastFrame[4]);
        u8g2.setCursor(2, 44);
        u8g2.printf("PLD: %02X %02X %02X %02X %02X %02X",
                    s_esbLastFrame[5], s_esbLastFrame[6], s_esbLastFrame[7],
                    s_esbLastFrame[8], s_esbLastFrame[9], s_esbLastFrame[10]);
    }

    u8g2.drawHLine(0, 52, 128);
    u8g2.drawStr(2, 61, "[OK]Modo [SW+Giro]Replay");
    u8g2.sendBuffer();
}

void spectrographLoop() {
    if (encOk()) {
        s_rfViewMode = (s_rfViewMode + 1) % 4;
        if (s_rfViewMode == 3) {
            configureEsbSnifferMode();
        } else if (s_rfViewMode == 0) {
            configureSpectrumListen();
        }
        encClear();
    }

    if (encBack()) {
        jam1.stopListening();
        jam2.stopListening();
        runningApp = false;
        encClear();
        return;
    }

    if (s_rfViewMode == 3) {
        runMouseJackEsbLoop();
        return;
    }

    long total_energy = 0;

    for (int i = 0; i < 62; i++) {
        if ((i & 7) == 0) {
            encoderUpdate();
            if (encBack()) {
                jam1.stopListening();
                jam2.stopListening();
                runningApp = false;
                return;
            }
            if (encOk()) {
                s_rfViewMode = (s_rfViewMode + 1) % 4;
                if (s_rfViewMode == 3) {
                    configureEsbSnifferMode();
                    return;
                }
            }
        }
        int c1 = 0, c2 = 0;
        for (int j = 0; j < REPETICIONES_LECTURA; j++) {
            jam1.setChannel(i);
            if (jam1.testRPD() || jam1.testCarrier()) c1++;

            jam2.setChannel(i + 63);
            if (jam2.testRPD() || jam2.testCarrier()) c2++;
        }
        values[i] = c1;
        values[i + 63] = c2;
        total_energy += (c1 + c2);
    }

    // Actualizar matriz Waterfall
    for (int r = WF_ROWS - 1; r > 0; r--) {
        memcpy(s_waterfall[r], s_waterfall[r - 1], WF_BYTES_PER_ROW);
    }
    memset(s_waterfall[0], 0, WF_BYTES_PER_ROW);
    for (int x = 0; x < CHANNELS; x++) {
        if (values[x] >= 3) {
            s_waterfall[0][x >> 3] |= (uint8_t)(1U << (x & 7));
        }
    }

    u8g2.clearBuffer();

    if (s_rfViewMode == 2) {
        // --- MODO 2: RF SPECTRUM FINGERPRINT ---
        int wifiEnergy = 0, bleAdvEnergy = 0, zigbeeSpikes = 0, narrowHops = 0;
        for (int c = 1; c < 82; c++) {
            if (values[c] > 2) {
                // Si hay energía continua en lóbulos anchos (>= 8 MHz) -> firma WiFi OFDM/DSSS
                if (c >= 4 && c <= 78 && values[c - 2] > 1 && values[c + 2] > 1) {
                    wifiEnergy += values[c];
                } else {
                    narrowHops += values[c];
                }
            }
        }
        bleAdvEnergy = values[2] * 3 + values[26] * 3 + values[80] * 3 + (narrowHops / 2);
        // Canales Zigbee 11, 15, 20, 25 -> nRF24 canales 5, 25, 50, 75
        zigbeeSpikes = (values[5] + values[25] + values[50] + values[75]) * 2;

        const char *domClass = "SILENCIO RF";
        if (total_energy > 25) {
            if (wifiEnergy >= bleAdvEnergy && wifiEnergy >= zigbeeSpikes) domClass = "WIFI 802.11 (20MHz)";
            else if (bleAdvEnergy >= zigbeeSpikes) domClass = "BLE / BT FHSS HOP";
            else domClass = "ZIGBEE / ESB 2.4G";
        }

        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawRBox(0, 0, 128, 12, 2);
        u8g2.setDrawColor(0);
        u8g2.drawStr(8, 9, "RF FINGERPRINT IA");
        u8g2.setDrawColor(1);

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.setCursor(2, 22);
        u8g2.printf("Firma: %s", domClass);

        int wBar = constrain(wifiEnergy / 2, 0, 70);
        int bBar = constrain(bleAdvEnergy / 2, 0, 70);
        int zBar = constrain(zigbeeSpikes / 2, 0, 70);

        u8g2.drawStr(2, 33, "WiFi:");
        u8g2.drawFrame(38, 27, 74, 6);
        u8g2.drawBox(40, 29, wBar, 2);

        u8g2.drawStr(2, 42, "BT/BLE:");
        u8g2.drawFrame(38, 36, 74, 6);
        u8g2.drawBox(40, 38, bBar, 2);

        u8g2.drawStr(2, 51, "Zig/ESB:");
        u8g2.drawFrame(38, 45, 74, 6);
        u8g2.drawBox(40, 47, zBar, 2);

        u8g2.drawHLine(0, 53, 128);
        u8g2.drawStr(4, 62, "[OK] MouseJack [HOLD]Salir");
        u8g2.sendBuffer();
        return;
    }

    if (total_energy > UMBRAL_ENERGIA_TOTAL) {
        if ((millis() / 150) % 2 == 0) {
            u8g2.drawBox(0, 0, 128, 12);
            u8g2.setDrawColor(0);
            u8g2.setFont(u8g2_font_6x10_tr);
            u8g2.drawStr(15, 9, "! ATAQUE DETECTADO !");
            u8g2.setDrawColor(1);
        }
    } else {
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(2, 7, (s_rfViewMode == 1) ? "2.4GHz WATERFALL" : "2.4GHz ESPECTRO");
        u8g2.setCursor(90, 7);
        u8g2.print("[OK]MODO");
    }
    u8g2.drawHLine(0, 9, 128);

    if (s_rfViewMode == 0) {
        for (int x = 0; x < CHANNELS; x++) {
            int bar_h = map(values[x], 0, REPETICIONES_LECTURA, 0, 42);
            if (bar_h > 42) bar_h = 42;
            if (bar_h > 0) {
                u8g2.drawVLine(x + 1, 53 - bar_h, bar_h);
            }
            if (bar_h >= peak_heights[x]) {
                peak_heights[x] = bar_h; 
            } else {
                if (millis() % 2 == 0 && peak_heights[x] > 0) peak_heights[x]--; 
            }
            if (peak_heights[x] > 0) {
                u8g2.drawPixel(x + 1, 53 - peak_heights[x]);
            }
        }
    } else {
        for (int r = 0; r < WF_ROWS; r++) {
            int y = 11 + r;
            for (int x = 0; x < CHANNELS; x++) {
                if (s_waterfall[r][x >> 3] & (1U << (x & 7))) {
                    u8g2.drawPixel(x + 1, y);
                }
            }
        }
        for (int x = 0; x < CHANNELS; x++) {
            int h = map(values[x], 0, REPETICIONES_LECTURA, 0, 6);
            if (h > 6) h = 6;
            if (h > 0) u8g2.drawVLine(x + 1, 53 - h, h);
        }
    }

    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawHLine(0, 54, 128);
    u8g2.drawStr(2, 62, "2.40G");
    u8g2.drawStr(52, 62, "2.45G");
    u8g2.drawStr(104, 62, "2.52G");

    u8g2.sendBuffer();
}
