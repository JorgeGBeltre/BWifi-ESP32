#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include <Arduino.h>

struct AppSettings {
  uint8_t brightnessIdx;    // 0: 25%, 1: 50%, 2: 75%, 3: 100%
  bool invertEncoder;       // true = invertido, false = normal
  uint8_t screensaverIdx;   // 0: OFF, 1: 30s, 2: 60s, 3: 120s
  uint8_t profileMode;      // 0: LAB, 1: AUDIT, 2: DEMO
  bool pinEnabled;          // Bloqueo PIN 1234 al inicio
  bool showLegalNotice;     // Aviso de uso ético al inicio
  uint8_t themeMode;        // 0: CLASICO, 1: INVERSO, 2: RETRO
};

extern AppSettings g_settings;

void settingsLoad();
void settingsSave();
void settingsApplyHardware();
void settingsRunBootGuards();
bool settingsHandleScreensaver();
void settingsSetup();
void settingsLoop();

void rotateLogIfNeeded(const char *path, size_t maxBytes = 24576);
bool saveOledScreenshotBmp(const char *path = "/screen.bmp");

#endif
