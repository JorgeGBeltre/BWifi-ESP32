#pragma once

#include <RF24.h>
#include "Domain/Interfaces/ISystemDiagnostics.h"

namespace bwifi::infra {

/**
 * @brief Concrete ISystemDiagnostics adapter collecting ESP32 heap, SPIFFS,
 * RTC boot count, nRF24 chip presence, and encoder button status.
 */
class EspSystemDiagnostics : public domain::ISystemDiagnostics {
public:
    EspSystemDiagnostics(RF24& radio1, RF24& radio2, uint32_t& bootCount);

    void incrementBootCount() override;
    domain::HardwareDiagnostics collectDiagnostics() const override;

private:
    RF24&     radio1_;
    RF24&     radio2_;
    uint32_t& bootCount_;
};

} // namespace bwifi::infra

