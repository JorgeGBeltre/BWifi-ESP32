#pragma once

#include "Domain/Entities/HardwareDiagnostics.h"

namespace bwifi::domain {

/**
 * @brief Abstract port for reading hardware diagnostics, SPIFFS metrics, and boot counters.
 */
class ISystemDiagnostics {
public:
    virtual ~ISystemDiagnostics() = default;

    virtual void incrementBootCount() = 0;
    virtual HardwareDiagnostics collectDiagnostics() const = 0;
};

} // namespace bwifi::domain

