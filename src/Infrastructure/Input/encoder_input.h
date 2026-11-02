#ifndef ENCODER_INPUT_H
#define ENCODER_INPUT_H

#include <Arduino.h>

// --- MAPEO DE PINES ROTARY ENCODER HW-040 ---
// Reutiliza los GPIOs de entrada originales sin interferir con SPI (nRF24) ni I2C (OLED)
#define ENC_CLK 26          // Canal A (antes BTN_UP)
#define ENC_DT 33           // Canal B (antes BTN_DOWN)
#define ENC_SW 32           // Pulsador integrado del eje (antes BTN_OK)
#define BTN_BACK_PIN 25     // Opcional: botón físico BACK de respaldo
#define BTN_AUX_PIN 27      // Opcional: botón físico AUX/ERASE de respaldo

// Cambiar a 1 si se desea invertir el sentido de giro por software
#define ENC_INVERT_DIR 0

// Tiempos de respuesta del pulsador SW (ms) optimizados para fluidez
#define ENC_DEBOUNCE_MS 12
#define ENC_LONG_PRESS_MS 500

void encoderInit();
void encoderUpdate();
void encClear();

// Lectura de rotación (CW = +N / siguiente, CCW = -N / anterior)
int encStep();
bool encUp();
bool encDown();
bool encUpHeld(uint32_t windowMs = 140);
bool encDownHeld(uint32_t windowMs = 140);

// Lectura de eventos de pulsador
bool encOk();       // Pulsación corta en SW
bool encOkDown();   // Flanco de bajada inmediato en SW (para juegos arcade)
bool encBack();     // Pulsación larga en SW (>= 500 ms) o botón físico 25
bool encAux();      // Giro con SW presionado o botón físico 27
bool encSwPressed();// Indica si SW está físicamente presionado (para feedback visual)

// Configuración en caliente y marca de actividad (para Screensaver / Settings NVS)
void encSetInvertDir(bool invert);
bool encGetInvertDir();
uint32_t encLastActivityMs();
void encTouchActivity();

#endif
