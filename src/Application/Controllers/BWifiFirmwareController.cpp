#include "Application/Controllers/BWifiFirmwareController.h"
#include "Application/Services/CarouselKinematicsService.h"

namespace bwifi::app {

BWifiFirmwareController::BWifiFirmwareController(ModuleRegistryService& registry,
                                                 ScreensaverService& screensaver,
                                                 domain::IDisplayController& display,
                                                 domain::IEncoderInput& encoder,
                                                 domain::IRadioStackManager& radios,
                                                 domain::ISettingsRepository& settingsRepo,
                                                 domain::ISystemDiagnostics& diagnostics,
                                                 domain::IClock& clock,
                                                 int& sharedMenuIndex,
                                                 bool& sharedRunningApp,
                                                 domain::DeviceSettings& sharedSettings)
    : registry_(registry),
      screensaver_(screensaver),
      display_(display),
      encoder_(encoder),
      radios_(radios),
      settingsRepo_(settingsRepo),
      diagnostics_(diagnostics),
      clock_(clock),
      sharedMenuIndex_(sharedMenuIndex),
      sharedRunningApp_(sharedRunningApp),
      sharedSettings_(sharedSettings) {}

void BWifiFirmwareController::setup() {
    diagnostics_.incrementBootCount();
    display_.begin();
    encoder_.init();
    settingsRepo_.load(sharedSettings_);
    display_.applySettings(sharedSettings_);

    for (int pct = 0; pct <= 100; pct += 10) {
        display_.renderSplashProgress(pct);
        clock_.delayMs(15);
    }

    radios_.initRadios();
    display_.runBootGuards();
    encoder_.clear();
}

void BWifiFirmwareController::tick() {
    encoder_.update();

    if (!sharedRunningApp_) {
        handleCarouselTick();
    } else {
        handleModuleTick();
    }
}

void BWifiFirmwareController::handleCarouselTick() {
    if (screensaver_.evaluate(sharedSettings_.screensaverIdx)) {
        return;
    }

    const int delta = encoder_.step();
    const int total = static_cast<int>(registry_.count());
    if (delta != 0 && total > 0) {
        sharedMenuIndex_ = CarouselKinematicsService::wrapIndex(sharedMenuIndex_ + delta, total);
    }

    display_.renderCarouselMenu();

    // Quick gesture (Hold SW + Rotate) in main menu: Save 1-bit OLED screenshot to /screen.bmp
    if (encoder_.aux()) {
        display_.saveScreenshotBmp("/screen.bmp");
        display_.renderToast("BMP GUARDADO!", 450);
        encoder_.clear();
    }

    // Short press (OK) launches the selected module
    if (encoder_.ok()) {
        encoder_.clear();
        sharedRunningApp_ = true;
        if (domain::IAppModule* mod = registry_.getModule(sharedMenuIndex_)) {
            mod->setup();
        }
    }
}

void BWifiFirmwareController::handleModuleTick() {
    domain::IAppModule* mod = registry_.getModule(sharedMenuIndex_);
    if (mod) {
        mod->loop();
    }

    const domain::ModuleDescriptor desc = mod ? mod->descriptor() : domain::ModuleDescriptor{};
    const bool backRequested = encoder_.back() && desc.allowGlobalBackExit;

    if (!sharedRunningApp_ || backRequested) {
        if (mod) {
            mod->onExit();
        }
        performExitCleanup(desc);
    }
}

void BWifiFirmwareController::performExitCleanup(const domain::ModuleDescriptor& desc) {
    encoder_.clear();
    sharedRunningApp_ = false;
    if (desc.requiresWebServerStopOnExit) {
        radios_.stopWebServer();
    }
    radios_.stopAllRadios();
}

} // namespace bwifi::app

