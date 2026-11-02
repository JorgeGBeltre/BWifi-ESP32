#pragma once

namespace bwifi::domain {

/**
 * @brief Abstract port for dual nRF24L01+ SPI radios, ESP-NOW peer control,
 * Wi-Fi promiscuous/SoftAP teardown, and AsyncWebServer lifecycle.
 */
class IRadioStackManager {
public:
    virtual ~IRadioStackManager() = default;

    virtual void initRadios() = 0;
    virtual void stopAllRadios() = 0;
    virtual void stopWebServer() = 0;

    virtual bool isNrf1Connected() const = 0;
    virtual bool isNrf2Connected() const = 0;
};

} // namespace bwifi::domain

