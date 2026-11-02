#pragma once

#include "Application/Services/ModuleRegistryService.h"
#include "Domain/Interfaces/IAppModule.h"

namespace bwifi::infra {

/**
 * @brief Lightweight adapter binding a firmware module's descriptor, setup, and loop callbacks
 * to the pure domain::IAppModule contract.
 */
class CallbackAppModule final : public domain::IAppModule {
public:
    using SetupFn = void (*)();
    using LoopFn  = void (*)();

    constexpr CallbackAppModule(domain::ModuleDescriptor desc,
                                SetupFn setupFn,
                                LoopFn loopFn)
        : desc_(desc), setupFn_(setupFn), loopFn_(loopFn) {}

    domain::ModuleDescriptor descriptor() const override { return desc_; }

    void setup() override {
        if (setupFn_) {
            setupFn_();
        }
    }

    void loop() override {
        if (loopFn_) {
            loopFn_();
        }
    }

private:
    domain::ModuleDescriptor desc_;
    SetupFn                  setupFn_;
    LoopFn                   loopFn_;
};

/**
 * @brief Registers all 22 BWifi-ESP32 v3.5 firmware modules into the application ModuleRegistryService.
 */
void registerAllFirmwareModules(app::ModuleRegistryService& registry);

} // namespace bwifi::infra

