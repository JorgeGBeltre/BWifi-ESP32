#pragma once

#include <cstdint>

namespace bwifi::infra {

struct BoardConfig {
    static constexpr const char* kFirmwareVersion = "3.5";

    // I2C SSD1306 128x64 OLED Display
    static constexpr uint8_t  kOledSclPin     = 22;
    static constexpr uint8_t  kOledSdaPin     = 21;
    static constexpr uint32_t kOledBusClockHz = 800000;

    // Dual nRF24L01+ PA+LNA SPI Radios (VSPI: SCK=18, MISO=19, MOSI=23)
    static constexpr uint8_t  kNrf1CePin      = 5;
    static constexpr uint8_t  kNrf1CsnPin     = 17;
    static constexpr uint8_t  kNrf2CePin      = 16;
    static constexpr uint8_t  kNrf2CsnPin     = 4;
    static constexpr uint32_t kNrfSpiSpeedHz  = 16000000;

    // HW-040 (KY-040) Quadrature Rotary Encoder
    static constexpr uint8_t  kEncClkPin      = 26;
    static constexpr uint8_t  kEncDtPin       = 33;
    static constexpr uint8_t  kEncSwPin       = 32;
    static constexpr uint8_t  kBtnBackPin     = 25;
    static constexpr uint8_t  kBtnAuxPin      = 27;

    // Web Dashboard HTTP Port
    static constexpr uint16_t kWebServerPort  = 80;
};

} // namespace bwifi::infra

