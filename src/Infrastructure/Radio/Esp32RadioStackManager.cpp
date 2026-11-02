#include "Infrastructure/Radio/Esp32RadioStackManager.h"
#include "Infrastructure/EspNow/slave_control.h"
#include <WiFi.h>
#include <esp_wifi.h>

namespace bwifi::infra {

Esp32RadioStackManager::Esp32RadioStackManager(RF24& radio1, RF24& radio2, AsyncWebServer& webServer)
    : radio1_(radio1), radio2_(radio2), webServer_(webServer) {}

void Esp32RadioStackManager::initRadios() {
    ::slaveControlSetup();
    radio1_.begin();
    radio2_.begin();
}

void Esp32RadioStackManager::stopAllRadios() {
    if (WiFi.getMode() != WIFI_OFF) {
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_promiscuous_rx_cb(nullptr);
        WiFi.mode(WIFI_OFF);
    }
    radio1_.stopListening();
    radio2_.stopListening();
    radio1_.stopConstCarrier();
    radio2_.stopConstCarrier();
}

void Esp32RadioStackManager::stopWebServer() {
    webServer_.end();
    WiFi.softAPdisconnect(true);
}

bool Esp32RadioStackManager::isNrf1Connected() const {
    return radio1_.isChipConnected();
}

bool Esp32RadioStackManager::isNrf2Connected() const {
    return radio2_.isChipConnected();
}

} // namespace bwifi::infra

