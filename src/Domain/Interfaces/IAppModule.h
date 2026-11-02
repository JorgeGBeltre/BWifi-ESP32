#pragma once

#include "Domain/Entities/ModuleDescriptor.h"

namespace bwifi::domain {

/**
 * @brief Abstract interface implemented by each of the 22 firmware modules.
 */
class IAppModule {
public:
    virtual ~IAppModule() = default;

    virtual ModuleDescriptor descriptor() const = 0;
    virtual void setup() = 0;
    virtual void loop() = 0;
    virtual void onExit() {}
};

} // namespace bwifi::domain

