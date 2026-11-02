#pragma once

#include <Arduino.h>
#include "Domain/Interfaces/IClock.h"

namespace bwifi::infra {

/**
 * @brief Concrete IClock adapter backed by Arduino millis() and delay().
 */
class ArduinoClock : public domain::IClock {
public:
    uint32_t millis() const override {
        return ::millis();
    }

    void delayMs(uint32_t ms) override {
        ::delay(ms);
    }
};

} // namespace bwifi::infra

