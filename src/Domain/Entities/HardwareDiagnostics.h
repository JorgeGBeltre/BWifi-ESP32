#pragma once

#include <cstddef>
#include <cstdint>

namespace bwifi::domain {

/**
 * @brief Pure domain entity representing hardware self-test diagnostics and memory telemetry.
 */
struct HardwareDiagnostics {
    bool     nrf1Connected        = false;
    bool     nrf2Connected        = false;
    bool     spiffsMounted        = false;
    size_t   spiffsUsedKb         = 0;
    size_t   spiffsTotalKb        = 0;
    uint32_t freeHeapKb           = 0;
    uint32_t bootCount            = 0;
    bool     encoderButtonPressed = false;
};

} // namespace bwifi::domain

