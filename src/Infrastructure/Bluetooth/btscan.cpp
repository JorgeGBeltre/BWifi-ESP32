#include <Arduino.h>
#include <U8g2lib.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "Infrastructure/Bluetooth/btscan.h"
#include "Infrastructure/Input/encoder_input.h"

extern U8G2 u8g2;

BLEScan* pBLEScan;
BLEScanResults foundDevices; 
int btSelected = 0;
int btFound = 0;
bool hasScanResults = false;

void btscanSetup() {
    if (!BLEDevice::getInitialized()) {
        BLEDevice::init(""); 
    }
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true); 
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    hasScanResults = false;
    btFound = 0;
    btSelected = 0;
    encClear();
}

// Función auxiliar para la animación del radar
void drawRadar(int angle) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(35, 12, "ESCANEANDO BT");
    
    u8g2.drawCircle(64, 40, 20);
    u8g2.drawCircle(64, 40, 10);
    u8g2.drawCircle(64, 40, 2);
    
    float rad = angle * 0.0174533;
    int x = 64 + cos(rad) * 20;
    int y = 40 + sin(rad) * 20;
    u8g2.drawLine(64, 40, x, y);
    
    u8g2.sendBuffer();
}

void btscanLoop() {
    if (!hasScanResults) {
        for(int a = 0; a < 360; a += 20) {
            drawRadar(a);
            delay(10);
        }

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        BLEScanResults* results = pBLEScan->start(5, false);
        if (results) {
            foundDevices = *results; 
            btFound = foundDevices.getCount();
            pBLEScan->clearResults(); 
        }
#else
        foundDevices = pBLEScan->start(5, false);
        btFound = foundDevices.getCount();
        pBLEScan->clearResults();
#endif
        hasScanResults = true;
        btSelected = 0;
        encClear();
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 10, "DISPOSITIVOS BT:");

    int step = encStep();
    if (btFound > 0) {
        if (step != 0) {
            btSelected = (btSelected + step) % btFound;
            if (btSelected < 0) btSelected += btFound;
        }

        for (int i = 0; i < 3; i++) {
            int idx = (btSelected / 3 * 3) + i;
            if (idx < btFound) {
                BLEAdvertisedDevice dev = foundDevices.getDevice(idx);
                int y = 25 + (i * 14);
                u8g2.setCursor(0, y);
                u8g2.print(idx == btSelected ? ">" : " ");
                String name = dev.getName().c_str();
                if (name == "") name = dev.getAddress().toString().c_str();
                u8g2.print(name.substring(0, 12));
                u8g2.setCursor(95, y);
                u8g2.print(dev.getRSSI()); u8g2.print("dB");
            }
        }
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(0, 63, "OK: RE-ESCANEAR | HOLD: SALIR");
    } else { 
        u8g2.drawStr(10, 40, "No se hallaron..."); 
    }

    if (encOk()) { hasScanResults = false; btFound = 0; }
    u8g2.sendBuffer();
}
