#pragma once

#include "Domain/Interfaces/IEncoderInput.h"
#include "Infrastructure/Input/encoder_input.h"

namespace bwifi::infra {

/**
 * @brief Concrete IEncoderInput adapter wrapping the 7-state Buxton Gray-code ISR driver.
 */
class Hw040EncoderInput : public domain::IEncoderInput {
public:
    void init() override { ::encoderInit(); }
    void update() override { ::encoderUpdate(); }
    void clear() override { ::encClear(); }

    int step() override { return ::encStep(); }
    bool up() override { return ::encUp(); }
    bool down() override { return ::encDown(); }
    bool ok() override { return ::encOk(); }
    bool okDown() override { return ::encOkDown(); }
    bool back() override { return ::encBack(); }
    bool aux() override { return ::encAux(); }
    bool isSwPressed() const override { return ::encSwPressed(); }

    void setInvertDirection(bool invert) override { ::encSetInvertDir(invert); }
    bool getInvertDirection() const override { return ::encGetInvertDir(); }
    uint32_t lastActivityMs() const override { return ::encLastActivityMs(); }
    void touchActivity() override { ::encTouchActivity(); }
};

} // namespace bwifi::infra

