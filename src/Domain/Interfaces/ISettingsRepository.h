#pragma once

#include <cstddef>
#include "Domain/Entities/DeviceSettings.h"

namespace bwifi::domain {

/**
 * @brief Abstract port for persistent NVS settings storage and SPIFFS log rotation.
 */
class ISettingsRepository {
public:
    virtual ~ISettingsRepository() = default;

    virtual void load(DeviceSettings& out) = 0;
    virtual void save(const DeviceSettings& settings) = 0;
    virtual void rotateLogIfNeeded(const char* path, size_t maxBytes = 24576) = 0;
};

} // namespace bwifi::domain

