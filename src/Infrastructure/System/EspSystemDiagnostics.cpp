#include "Infrastructure/System/EspSystemDiagnostics.h"
#include "Infrastructure/Input/encoder_input.h"
#include <Arduino.h>
#include <SPIFFS.h>

namespace bwifi::infra {

EspSystemDiagnostics::EspSystemDiagnostics(RF24& radio1, RF24& radio2, uint32_t& bootCount)
    : radio1_(radio1), radio2_(radio2), bootCount_(bootCount) {}

void EspSystemDiagnostics::incrementBootCount() {
    bootCount_++;
}

domain::HardwareDiagnostics EspSystemDiagnostics::collectDiagnostics() const {
    domain::HardwareDiagnostics diag;
    diag.nrf1Connected        = radio1_.isChipConnected();
    diag.nrf2Connected        = radio2_.isChipConnected();
    diag.spiffsMounted        = SPIFFS.begin(true);
    diag.spiffsUsedKb         = diag.spiffsMounted ? (SPIFFS.usedBytes() / 1024) : 0;
    diag.spiffsTotalKb        = diag.spiffsMounted ? (SPIFFS.totalBytes() / 1024) : 0;
    diag.freeHeapKb           = ESP.getFreeHeap() / 1024;
    diag.bootCount            = bootCount_;
    diag.encoderButtonPressed = ::encSwPressed();
    return diag;
}

} // namespace bwifi::infra

