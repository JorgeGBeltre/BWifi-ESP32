#include "Application/Services/ScreensaverService.h"

namespace bwifi::app {

ScreensaverService::ScreensaverService(domain::IDisplayController& display,
                                       domain::IEncoderInput& encoder,
                                       domain::IClock& clock)
    : display_(display), encoder_(encoder), clock_(clock) {}

bool ScreensaverService::evaluate(uint8_t screensaverIdx) {
    const uint32_t timeoutMs = domain::screensaverTimeoutMsForIndex(screensaverIdx);
    if (timeoutMs == 0) {
        if (dimmed_) {
            display_.setPowerSave(false);
            dimmed_ = false;
        }
        return false;
    }

    const uint32_t idleMs = clock_.millis() - encoder_.lastActivityMs();
    if (idleMs >= timeoutMs) {
        if (!dimmed_) {
            display_.setPowerSave(true);
            dimmed_ = true;
        }
        if (encoder_.isSwPressed() || encoder_.step() != 0 || encoder_.ok() || encoder_.back()) {
            display_.setPowerSave(false);
            dimmed_ = false;
            encoder_.touchActivity();
            encoder_.clear();
        }
        return dimmed_;
    } else if (dimmed_) {
        display_.setPowerSave(false);
        dimmed_ = false;
    }
    return false;
}

} // namespace bwifi::app

