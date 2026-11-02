#pragma once

#include <array>
#include <cstddef>
#include "Domain/Interfaces/IAppModule.h"

namespace bwifi::app {

/**
 * @brief Registry and dispatcher for the 22 firmware modules.
 */
class ModuleRegistryService {
public:
    static constexpr size_t kMaxModules = static_cast<size_t>(domain::ModuleId::Count);

    ModuleRegistryService() = default;

    bool registerModule(domain::IAppModule& module);
    domain::IAppModule* getModule(int index) const;
    size_t count() const { return count_; }

private:
    std::array<domain::IAppModule*, kMaxModules> modules_{};
    size_t                                       count_ = 0;
};

} // namespace bwifi::app

