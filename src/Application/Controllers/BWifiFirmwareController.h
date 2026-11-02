#pragma once

#include "Application/Services/ModuleRegistryService.h"
#include "Application/Services/ScreensaverService.h"
#include "Domain/Entities/DeviceSettings.h"
#include "Domain/Interfaces/IClock.h"
#include "Domain/Interfaces/IDisplayController.h"
#include "Domain/Interfaces/IEncoderInput.h"
#include "Domain/Interfaces/IRadioStackManager.h"
#include "Domain/Interfaces/ISettingsRepository.h"
#include "Domain/Interfaces/ISystemDiagnostics.h"

namespace bwifi::app {

/**
 * @brief Main application controller orchestrating the boot sequence,
 * carousel navigation, screenshot hotkeys, module execution, and deterministic radio teardown.
 */
class BWifiFirmwareController {
public:
    enum class State {
        CarouselMenu,
        RunningModule
    };

    BWifiFirmwareController(ModuleRegistryService& registry,
                            ScreensaverService& screensaver,
                            domain::IDisplayController& display,
                            domain::IEncoderInput& encoder,
                            domain::IRadioStackManager& radios,
                            domain::ISettingsRepository& settingsRepo,
                            domain::ISystemDiagnostics& diagnostics,
                            domain::IClock& clock,
                            int& sharedMenuIndex,
                            bool& sharedRunningApp,
                            domain::DeviceSettings& sharedSettings);

    void setup();
    void tick();

    State getState() const { return sharedRunningApp_ ? State::RunningModule : State::CarouselMenu; }

private:
    void handleCarouselTick();
    void handleModuleTick();
    void performExitCleanup(const domain::ModuleDescriptor& desc);

    ModuleRegistryService&       registry_;
    ScreensaverService&          screensaver_;
    domain::IDisplayController&  display_;
    domain::IEncoderInput&       encoder_;
    domain::IRadioStackManager&  radios_;
    domain::ISettingsRepository& settingsRepo_;
    domain::ISystemDiagnostics&  diagnostics_;
    domain::IClock&              clock_;

    int&                         sharedMenuIndex_;
    bool&                        sharedRunningApp_;
    domain::DeviceSettings&      sharedSettings_;
};

} // namespace bwifi::app

