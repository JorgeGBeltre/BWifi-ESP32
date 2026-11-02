#pragma once

#include <cstdint>
#include "Domain/Entities/DeviceSettings.h"
#include "Domain/Interfaces/IClock.h"
#include "Domain/Interfaces/IDisplayController.h"
#include "Domain/Interfaces/IEncoderInput.h"

namespace bwifi::app {

/**
 * @brief Application service that evaluates user inactivity and manages OLED power-save dimming.
 */
class ScreensaverService {
public:
    ScreensaverService(domain::IDisplayController& display,
                       domain::IEncoderInput& encoder,
                       domain::IClock& clock);

    /**
     * @return true if the display is currently dimmed in screensaver mode (caller should skip frame rendering).
     */
    bool evaluate(uint8_t screensaverIdx);
    bool isDimmed() const { return dimmed_; }

private:
    domain::IDisplayController& display_;
    domain::IEncoderInput&      encoder_;
    domain::IClock&             clock_;
    bool                        dimmed_ = false;
};

} // namespace bwifi::app

