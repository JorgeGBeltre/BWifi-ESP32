#pragma once

#include <string>

namespace bwifi::domain {

/**
 * @brief Pure domain entity representing the selected Wi-Fi target and active RF state.
 */
struct AuditTarget {
    std::string ssid             = "Ninguna";
    int         channel          = 1;
    bool        isTotalAttacking = false;
    bool        isHybridActive   = false;
};

} // namespace bwifi::domain

