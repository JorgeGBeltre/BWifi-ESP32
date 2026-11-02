#pragma once

#include <cstdint>

namespace bwifi::domain {

/**
 * @brief Abstract monotonic clock and delay provider.
 */
class IClock {
public:
    virtual ~IClock() = default;

    virtual uint32_t millis() const = 0;
    virtual void delayMs(uint32_t ms) = 0;
};

} // namespace bwifi::domain

