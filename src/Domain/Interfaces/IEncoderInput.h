#pragma once

#include <cstdint>

namespace bwifi::domain {

/**
 * @brief Abstract port for the HW-040 rotary encoder and push-button events.
 */
class IEncoderInput {
public:
    virtual ~IEncoderInput() = default;

    virtual void init() = 0;
    virtual void update() = 0;
    virtual void clear() = 0;

    virtual int step() = 0;
    virtual bool up() = 0;
    virtual bool down() = 0;
    virtual bool ok() = 0;
    virtual bool okDown() = 0;
    virtual bool back() = 0;
    virtual bool aux() = 0;
    virtual bool isSwPressed() const = 0;

    virtual void setInvertDirection(bool invert) = 0;
    virtual bool getInvertDirection() const = 0;
    virtual uint32_t lastActivityMs() const = 0;
    virtual void touchActivity() = 0;
};

} // namespace bwifi::domain

