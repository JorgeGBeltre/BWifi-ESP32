#include "Application/Services/ModuleRegistryService.h"

namespace bwifi::app {

bool ModuleRegistryService::registerModule(domain::IAppModule& module) {
    if (count_ >= kMaxModules) {
        return false;
    }
    modules_[count_++] = &module;
    return true;
}

domain::IAppModule* ModuleRegistryService::getModule(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= count_) {
        return nullptr;
    }
    return modules_[static_cast<size_t>(index)];
}

} // namespace bwifi::app

