#include "Infrastructure/Input/encoder_input.h"
#include "soc/gpio_struct.h"

// Máquina de estados determinista de código Gray para rotary encoder HW-040 (KY-040)
// Emite exactamente 1 paso por detent físico al retornar al reposo (11),
// filtrando rebotes mecánicos sin saltos dobles.
#define R_START     0x00
#define R_CW_BEGIN  0x01
#define R_CW_NEXT   0x02
#define R_CW_FINAL  0x03
#define R_CCW_BEGIN 0x04
#define R_CCW_NEXT  0x05
#define R_CCW_FINAL 0x06

#define DIR_NONE    0x00
#define DIR_CW      0x10
#define DIR_CCW     0x20

// Columnas indexadas por pinState = (CLK << 1) | DT:
// 00 (0), 01 (1), 10 (2), 11 (3 = reposo en detent)
static const uint8_t DRAM_ATTR kStateTable[7][4] = {
    // R_START (0x00)
    { R_START,     R_CW_BEGIN,  R_CCW_BEGIN, R_START },
    // R_CW_BEGIN (0x01)
    { R_CW_NEXT,   R_CW_BEGIN,  R_CW_FINAL,  R_START },
    // R_CW_NEXT (0x02)
    { R_CW_NEXT,   R_CW_BEGIN,  R_CW_FINAL,  R_START },
    // R_CW_FINAL (0x03)
    { R_CW_NEXT,   R_START,     R_CW_FINAL,  R_START | DIR_CW },
    // R_CCW_BEGIN (0x04)
    { R_CCW_NEXT,  R_CCW_FINAL, R_CCW_BEGIN, R_START },
    // R_CCW_NEXT (0x05)
    { R_CCW_NEXT,  R_CCW_FINAL, R_CCW_BEGIN, R_START },
    // R_CCW_FINAL (0x06)
    { R_CCW_NEXT,  R_CCW_FINAL, R_START,     R_START | DIR_CCW }
};

static portMUX_TYPE s_encMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint8_t s_encState = R_START;
static volatile uint8_t s_lastPinState = 0x03;
static volatile int16_t s_stepCount = 0;
static volatile uint32_t s_lastStepMs = 0;
static volatile int8_t s_lastStepDir = 0;
static volatile uint32_t s_lastUpMs = 0;
static volatile uint32_t s_lastDownMs = 0;
static volatile bool s_rotatedWhileSwLow = false;
static volatile bool s_runtimeInvertDir = (ENC_INVERT_DIR != 0);
static volatile uint32_t s_lastActivityMs = 0;

// Estado del pulsador SW (GPIO 32) con soporte por interrupción + polling
static volatile uint32_t s_swIsrFallMs = 0;
static volatile uint32_t s_swIsrRiseMs = 0;
static volatile bool s_swIsrEdgePending = false;

static bool s_swLastRaw = HIGH;
static bool s_swStable = HIGH;
static uint32_t s_swLastDebounceMs = 0;
static uint32_t s_swPressStartMs = 0;
static bool s_swLongHandled = false;
static bool s_okEvent = false;
static bool s_okDownEvent = false;
static bool s_backEvent = false;
static bool s_auxEvent = false;

static bool s_backPinLast = HIGH;
static uint32_t s_backPinLastMs = 0;

// Lectura ultrarrápida de registros GPIO en RAM (sin sobrecarga de digitalRead)
static inline uint8_t IRAM_ATTR readPinFast(uint8_t pin) {
    if (pin < 32) {
        return (uint8_t)((GPIO.in >> pin) & 1U);
    }
    return (uint8_t)((GPIO.in1.val >> (pin - 32)) & 1U);
}

static inline void IRAM_ATTR pollQuadratureFast() {
    uint8_t clk = readPinFast(ENC_CLK);
    uint8_t dt  = readPinFast(ENC_DT);
    uint8_t pinState = (uint8_t)((clk << 1) | dt);

    if (pinState == s_lastPinState && pinState != 0x03) {
        return;
    }
    s_lastPinState = pinState;

    uint8_t nextState = kStateTable[s_encState & 0x07][pinState];
    s_encState = (uint8_t)(nextState & 0x07);

    uint8_t dir = (uint8_t)(nextState & 0x30);
    if (dir == DIR_NONE) {
        return;
    }

    int8_t step = (dir == DIR_CW) ? 1 : -1;

    if (s_runtimeInvertDir) {
        step = -step;
    }

    uint32_t nowMs = millis();
    uint32_t dtMs = nowMs - s_lastStepMs;

    // Filtro de asentamiento mecánico (< 5 ms del mismo sentido)
    if (dtMs < 5 && step == s_lastStepDir) {
        return;
    }

    s_lastStepMs = nowMs;
    s_lastActivityMs = nowMs;
    s_lastStepDir = step;

    if (readPinFast(ENC_SW) == 0) {
        s_rotatedWhileSwLow = true;
    }

    int16_t next = s_stepCount + step;
    if (next > 24) next = 24;
    if (next < -24) next = -24;
    s_stepCount = next;

    if (step > 0) {
        s_lastDownMs = nowMs;
    } else {
        s_lastUpMs = nowMs;
    }
}

void IRAM_ATTR encoderIsr() {
    portENTER_CRITICAL_ISR(&s_encMux);
    pollQuadratureFast();
    portEXIT_CRITICAL_ISR(&s_encMux);
}

void IRAM_ATTR swIsr() {
    uint32_t nowMs = millis();
    uint8_t level = readPinFast(ENC_SW);
    if (level == 0) {
        if (nowMs - s_swIsrFallMs >= ENC_DEBOUNCE_MS) {
            s_swIsrFallMs = nowMs;
            s_swIsrEdgePending = true;
        }
    } else {
        s_swIsrRiseMs = nowMs;
    }
}

void encoderInit() {
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT, INPUT_PULLUP);
    pinMode(ENC_SW, INPUT_PULLUP);
#if BTN_BACK_PIN >= 0
    pinMode(BTN_BACK_PIN, INPUT_PULLUP);
#endif
#if BTN_AUX_PIN >= 0
    pinMode(BTN_AUX_PIN, INPUT_PULLUP);
#endif

    uint8_t clk = readPinFast(ENC_CLK);
    uint8_t dt  = readPinFast(ENC_DT);
    s_lastPinState = (uint8_t)((clk << 1) | dt);
    s_encState = R_START;
    s_stepCount = 0;

    attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderIsr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_DT), encoderIsr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_SW), swIsr, CHANGE);
}

void encoderUpdate() {
    // Asegurar sincronización de cuadratura incluso tras secciones críticas de WiFi/BLE
    portENTER_CRITICAL(&s_encMux);
    pollQuadratureFast();
    portEXIT_CRITICAL(&s_encMux);

    uint32_t now = millis();

    // 1. Lectura con anti-rebote ágil (12 ms) del pulsador integrado SW (GPIO 32)
    bool rawSw = (readPinFast(ENC_SW) != 0);
    if (rawSw != s_swLastRaw) {
        s_swLastDebounceMs = now;
        s_swLastRaw = rawSw;
    }

    if ((now - s_swLastDebounceMs) >= ENC_DEBOUNCE_MS) {
        if (rawSw != s_swStable) {
            s_swStable = rawSw;
            s_swIsrEdgePending = false;
            if (s_swStable == LOW) {
                // Flanco de bajada (presionado)
                s_swPressStartMs = now;
                s_lastActivityMs = now;
                s_swLongHandled = false;
                s_rotatedWhileSwLow = false;
                s_okDownEvent = true;
            } else {
                // Flanco de subida (soltado)
                s_lastActivityMs = now;
                if (!s_swLongHandled && !s_rotatedWhileSwLow) {
                    s_okEvent = true;
                }
            }
        }
    } else if (s_swIsrEdgePending && rawSw == HIGH && s_swStable == HIGH) {
        // Si hubo un tap ultrarrápido capturado por ISR durante un frame pesado de OLED/WiFi
        uint32_t pulseWidth = s_swIsrRiseMs - s_swIsrFallMs;
        if (pulseWidth >= ENC_DEBOUNCE_MS && pulseWidth < ENC_LONG_PRESS_MS) {
            s_swIsrEdgePending = false;
            s_okDownEvent = true;
            s_okEvent = true;
        }
    }

    // Detección de pulsación larga (BACK) mientras SW se mantiene presionado
    if (s_swStable == LOW && !s_swLongHandled) {
        if (s_rotatedWhileSwLow) {
            s_auxEvent = true;
            s_swLongHandled = true;
        } else if ((now - s_swPressStartMs) >= ENC_LONG_PRESS_MS) {
            s_backEvent = true;
            s_swLongHandled = true;
            s_okDownEvent = false;
        }
    }

#if BTN_BACK_PIN >= 0
    // 2. Respaldo opcional: botón físico BACK si está configurado
    bool rawBack = (readPinFast(BTN_BACK_PIN) != 0);
    if (rawBack != s_backPinLast && (now - s_backPinLastMs) >= 30) {
        s_backPinLastMs = now;
        s_backPinLast = rawBack;
        if (rawBack == LOW) {
            s_backEvent = true;
        }
    }
#endif
}

void encClear() {
    portENTER_CRITICAL(&s_encMux);
    s_stepCount = 0;
    s_encState = R_START;
    s_rotatedWhileSwLow = false;
    portEXIT_CRITICAL(&s_encMux);

    s_swIsrEdgePending = false;
    s_okEvent = false;
    s_okDownEvent = false;
    s_backEvent = false;
    s_auxEvent = false;
}

int encStep() {
    encoderUpdate();
    int16_t steps = 0;
    portENTER_CRITICAL(&s_encMux);
    steps = s_stepCount;
    s_stepCount = 0;
    portEXIT_CRITICAL(&s_encMux);
    return (int)steps;
}

bool encUp() {
    encoderUpdate();
    bool triggered = false;
    portENTER_CRITICAL(&s_encMux);
    if (s_stepCount < 0) {
        s_stepCount++;
        triggered = true;
    }
    portEXIT_CRITICAL(&s_encMux);
    return triggered;
}

bool encDown() {
    encoderUpdate();
    bool triggered = false;
    portENTER_CRITICAL(&s_encMux);
    if (s_stepCount > 0) {
        s_stepCount--;
        triggered = true;
    }
    portEXIT_CRITICAL(&s_encMux);
    return triggered;
}

bool encUpHeld(uint32_t windowMs) {
    if (encUp()) {
        return true;
    }
    uint32_t lastMs;
    portENTER_CRITICAL(&s_encMux);
    lastMs = s_lastUpMs;
    portEXIT_CRITICAL(&s_encMux);
    return (lastMs != 0) && ((millis() - lastMs) <= windowMs);
}

bool encDownHeld(uint32_t windowMs) {
    if (encDown()) {
        return true;
    }
    uint32_t lastMs;
    portENTER_CRITICAL(&s_encMux);
    lastMs = s_lastDownMs;
    portEXIT_CRITICAL(&s_encMux);
    return (lastMs != 0) && ((millis() - lastMs) <= windowMs);
}

bool encOk() {
    encoderUpdate();
    if (s_okEvent) {
        s_okEvent = false;
        return true;
    }
    return false;
}

bool encOkDown() {
    encoderUpdate();
    if (s_okDownEvent) {
        s_okDownEvent = false;
        return true;
    }
    return false;
}

bool encBack() {
    encoderUpdate();
    if (s_backEvent) {
        s_backEvent = false;
        return true;
    }
    return false;
}

bool encAux() {
    encoderUpdate();
    if (s_auxEvent) {
        s_auxEvent = false;
        return true;
    }
#if BTN_AUX_PIN >= 0
    return (readPinFast(BTN_AUX_PIN) == 0);
#else
    return false;
#endif
}

bool encSwPressed() {
    return (readPinFast(ENC_SW) == 0);
}

void encSetInvertDir(bool invert) {
    portENTER_CRITICAL(&s_encMux);
    s_runtimeInvertDir = invert;
    portEXIT_CRITICAL(&s_encMux);
}

bool encGetInvertDir() {
    return s_runtimeInvertDir;
}

uint32_t encLastActivityMs() {
    return s_lastActivityMs;
}

void encTouchActivity() {
    s_lastActivityMs = millis();
}
