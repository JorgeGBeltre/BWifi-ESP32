#pragma once

#include <ESPAsyncWebServer.h>
#include <RF24.h>
#include "Domain/Interfaces/IRadioStackManager.h"

namespace bwifi::infra {

/**
 * @brief Concrete IRadioStackManager adapter managing dual nRF24L01+ radios,
 * ESP-NOW slave link, Wi-Fi promiscuous/SoftAP state, and AsyncWebServer lifecycle.
 */
class Esp32RadioStackManager : public domain::IRadioStackManager {
public:
    Esp32RadioStackManager(RF24& radio1, RF24& radio2, AsyncWebServer& webServer);

    void initRadios() override;
    void stopAllRadios() override;
    void stopWebServer() override;

    bool isNrf1Connected() const override;
    bool isNrf2Connected() const override;

private:
    RF24&           radio1_;
    RF24&           radio2_;
    AsyncWebServer& webServer_;
};

} // namespace bwifi::infra

