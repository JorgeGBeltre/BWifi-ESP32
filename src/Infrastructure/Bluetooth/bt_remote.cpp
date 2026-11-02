#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#define private protected
#include <BleKeyboard.h>
#undef private
#include <SPIFFS.h>
#include <U8g2lib.h>
#include "Infrastructure/Input/encoder_input.h"

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern bool runningApp;

static const uint8_t kComboHidDescriptor[] = {
  USAGE_PAGE(1),      0x01,
  USAGE(1),           0x06,
  COLLECTION(1),      0x01,
  REPORT_ID(1),       0x01,
  USAGE_PAGE(1),      0x07,
  USAGE_MINIMUM(1),   0xE0,
  USAGE_MAXIMUM(1),   0xE7,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,
  REPORT_SIZE(1),     0x01,
  REPORT_COUNT(1),    0x08,
  HIDINPUT(1),        0x02,
  REPORT_COUNT(1),    0x01,
  REPORT_SIZE(1),     0x08,
  HIDINPUT(1),        0x01,
  REPORT_COUNT(1),    0x05,
  REPORT_SIZE(1),     0x01,
  USAGE_PAGE(1),      0x08,
  USAGE_MINIMUM(1),   0x01,
  USAGE_MAXIMUM(1),   0x05,
  HIDOUTPUT(1),       0x02,
  REPORT_COUNT(1),    0x01,
  REPORT_SIZE(1),     0x03,
  HIDOUTPUT(1),       0x01,
  REPORT_COUNT(1),    0x06,
  REPORT_SIZE(1),     0x08,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x65,
  USAGE_PAGE(1),      0x07,
  USAGE_MINIMUM(1),   0x00,
  USAGE_MAXIMUM(1),   0x65,
  HIDINPUT(1),        0x00,
  END_COLLECTION(0),

  USAGE_PAGE(1),      0x0C,
  USAGE(1),           0x01,
  COLLECTION(1),      0x01,
  REPORT_ID(1),       0x02,
  USAGE_PAGE(1),      0x0C,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,
  REPORT_SIZE(1),     0x01,
  REPORT_COUNT(1),    0x10,
  USAGE(1),           0xB5,
  USAGE(1),           0xB6,
  USAGE(1),           0xB7,
  USAGE(1),           0xCD,
  USAGE(1),           0xE2,
  USAGE(1),           0xE9,
  USAGE(1),           0xEA,
  USAGE(2),           0x23, 0x02,
  USAGE(2),           0x94, 0x01,
  USAGE(2),           0x92, 0x01,
  USAGE(2),           0x2A, 0x02,
  USAGE(2),           0x21, 0x02,
  USAGE(2),           0x26, 0x02,
  USAGE(2),           0x24, 0x02,
  USAGE(2),           0x83, 0x01,
  USAGE(2),           0x8A, 0x01,
  HIDINPUT(1),        0x02,
  END_COLLECTION(0),

  USAGE_PAGE(1),      0x01,
  USAGE(1),           0x02,
  COLLECTION(1),      0x01,
  USAGE(1),           0x01,
  COLLECTION(1),      0x00,
  REPORT_ID(1),       0x03,
  USAGE_PAGE(1),      0x09,
  USAGE_MINIMUM(1),   0x01,
  USAGE_MAXIMUM(1),   0x03,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,
  REPORT_COUNT(1),    0x03,
  REPORT_SIZE(1),     0x01,
  HIDINPUT(1),        0x02,
  REPORT_COUNT(1),    0x01,
  REPORT_SIZE(1),     0x05,
  HIDINPUT(1),        0x03,
  USAGE_PAGE(1),      0x01,
  USAGE(1),           0x30,
  USAGE(1),           0x31,
  USAGE(1),           0x38,
  LOGICAL_MINIMUM(1), 0x81,
  LOGICAL_MAXIMUM(1), 0x7F,
  REPORT_SIZE(1),     0x08,
  REPORT_COUNT(1),    0x03,
  HIDINPUT(1),        0x06,
  END_COLLECTION(0),
  END_COLLECTION(0)
};

class BleComboControl : public BleKeyboard {
private:
  BLECharacteristic *inputMouse = nullptr;

public:
  BleComboControl(std::string name, std::string mfg, uint8_t bat)
      : BleKeyboard(name, mfg, bat) {}

  void beginCombo() {
    BLEDevice::init(deviceName);
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);

    hid = new BLEHIDDevice(pServer);
    inputKeyboard = hid->inputReport(0x01);
    outputKeyboard = hid->outputReport(0x01);
    inputMediaKeys = hid->inputReport(0x02);
    inputMouse = hid->inputReport(0x03);

    outputKeyboard->setCallbacks(this);
    hid->manufacturer()->setValue(deviceManufacturer);
    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
    hid->hidInfo(0x00, 0x01);

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    hid->reportMap((uint8_t *)kComboHidDescriptor, sizeof(kComboHidDescriptor));
    hid->startServices();
    onStarted(pServer);

    advertising = pServer->getAdvertising();
    advertising->setAppearance(HID_KEYBOARD);
    advertising->addServiceUUID(hid->hidService()->getUUID());
    advertising->setScanResponse(false);
    advertising->start();
    hid->setBatteryLevel(batteryLevel);
  }

  void moveMouse(int8_t dx, int8_t dy, int8_t wheel = 0, uint8_t buttons = 0) {
    if (this->isConnected() && inputMouse != nullptr) {
      uint8_t m[4] = {buttons, (uint8_t)dx, (uint8_t)dy, (uint8_t)wheel};
      inputMouse->setValue(m, 4);
      inputMouse->notify();
    }
  }

  void clickMouse(uint8_t buttonMask) {
    moveMouse(0, 0, 0, buttonMask);
    delay(25);
    moveMouse(0, 0, 0, 0);
  }
};

BleComboControl bleKeyboard("Wireless Kbd 3000", "Microsoft", 100);
bool keyboardInitialized = false;

static int remoteSubMode = 0; // 0: Media, 1: Mouse, 2: Reels/PPT, 3: Ducky Script, 4: Idioma ES/EN, 5: Gamepad
static bool modeSelected = false;
static int mouseAction = 0;
static bool s_layoutEs = true; // true = ES (Español), false = EN (US)
static int s_duckyLinesExecuted = 0;

// Mapea caracteres frecuentes para layout Español cuando el host está en ES
static void typeWithLayout(const String &text) {
  for (size_t i = 0; i < text.length(); i++) {
    char c = text[i];
    if (!s_layoutEs) {
      bleKeyboard.print(c);
    } else {
      // En teclado ES sobre hid US, ':' es Shift+'.', '/' es Shift+'7', '-' es '/'
      if (c == ':') bleKeyboard.print('>');
      else if (c == '/') bleKeyboard.print('&');
      else if (c == '-') bleKeyboard.print('/');
      else bleKeyboard.print(c);
    }
    delay(12);
  }
}

static void ensureDefaultDuckyFile() {
  if (!SPIFFS.begin(true)) return;
  if (!SPIFFS.exists("/payload.txt")) {
    File f = SPIFFS.open("/payload.txt", FILE_WRITE);
    if (f) {
      f.println("REM BWifi Ducky Script");
      f.println("DELAY 400");
      f.println("GUI r");
      f.println("DELAY 300");
      f.println("STRING notepad");
      f.println("ENTER");
      f.println("DELAY 500");
      f.println("STRING Hola desde BWifi-ESP32 v3.5 Ducky Parser!");
      f.println("ENTER");
      f.close();
    }
  }
}

static void executeDuckyScriptFromSpiffs() {
  ensureDefaultDuckyFile();
  File f = SPIFFS.open("/payload.txt", FILE_READ);
  if (!f) {
    typeWithLayout("Hola desde BWifi-ESP32 v3.5!");
    bleKeyboard.write(KEY_RETURN);
    s_duckyLinesExecuted = 1;
    return;
  }

  s_duckyLinesExecuted = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("REM")) continue;

    if (line.startsWith("DELAY ")) {
      int ms = line.substring(6).toInt();
      delay(constrain(ms, 10, 5000));
    } else if (line.startsWith("STRING ")) {
      typeWithLayout(line.substring(7));
    } else if (line == "ENTER") {
      bleKeyboard.write(KEY_RETURN);
    } else if (line == "TAB") {
      bleKeyboard.write(KEY_TAB);
    } else if (line == "SPACE") {
      bleKeyboard.write(' ');
    } else if (line == "UP" || line == "UPARROW") {
      bleKeyboard.write(KEY_UP_ARROW);
    } else if (line == "DOWN" || line == "DOWNARROW") {
      bleKeyboard.write(KEY_DOWN_ARROW);
    } else if (line.startsWith("GUI")) {
      bleKeyboard.press(KEY_LEFT_GUI);
      if (line.length() > 4) {
        char k = line[4];
        bleKeyboard.press(k);
      }
      delay(80);
      bleKeyboard.releaseAll();
    }
    s_duckyLinesExecuted++;
    delay(25);
  }
  f.close();
}

void btRemoteLoop() {
  if (!keyboardInitialized) {
    bleKeyboard.beginCombo();
    keyboardInitialized = true;
    ensureDefaultDuckyFile();
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  if (!bleKeyboard.isConnected()) {
    u8g2.drawStr(10, 12, "BT REMOTE + MOUSE");
    u8g2.drawHLine(0, 14, 128);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(10, 28, "Estado: ESPERANDO...");
    u8g2.drawStr(10, 40, "Busca: Wireless Kbd 3000");
    u8g2.drawStr(10, 56, "[HOLD] Volver al Menu");

    if (encBack()) {
      modeSelected = false;
      runningApp = false;
      encClear();
      return;
    }
  } else {
    if (!modeSelected) {
      const int kTotalModes = 6;
      int d = encStep();
      if (d != 0) {
        remoteSubMode = (remoteSubMode + d) % kTotalModes;
        if (remoteSubMode < 0) remoteSubMode += kTotalModes;
      }

      if (encOk()) {
        if (remoteSubMode == 4) {
          s_layoutEs = !s_layoutEs;
          encClear();
        } else {
          modeSelected = true;
          mouseAction = 0;
          encClear();
        }
      }

      if (encBack()) {
        runningApp = false;
        encClear();
        return;
      }

      u8g2.drawRBox(0, 0, 128, 12, 2);
      u8g2.setDrawColor(0);
      u8g2.drawStr(8, 9, "BLE HID COMBO v3.5");
      u8g2.setDrawColor(1);

      char layoutLine[26];
      snprintf(layoutLine, sizeof(layoutLine), "5. LAYOUT TECLADO: %s", s_layoutEs ? "ES" : "EN");

      const char *modes[kTotalModes] = {
          "1. CONTROL MEDIA",
          "2. BLE MOUSE (GIRO)",
          "3. NAV / REELS / PPT",
          "4. DUCKY (/payload.txt)",
          layoutLine,
          "6. BLE GAMEPAD / STICK"};

      int firstVis = remoteSubMode - 2;
      if (firstVis > kTotalModes - 4) firstVis = kTotalModes - 4;
      if (firstVis < 0) firstVis = 0;

      u8g2.setFont(u8g2_font_5x7_tr);
      for (int row = 0; row < 4; row++) {
        int i = firstVis + row;
        if (i >= kTotalModes) break;
        int y = 24 + row * 10;
        if (i == remoteSubMode) {
          u8g2.drawRBox(4, y - 7, 120, 9, 2);
          u8g2.setDrawColor(0);
          u8g2.drawStr(8, y, modes[i]);
          u8g2.setDrawColor(1);
        } else {
          u8g2.drawStr(8, y, modes[i]);
        }
      }

      u8g2.drawHLine(0, 54, 128);
      u8g2.drawStr(6, 62, "[OK] Entrar  [HOLD] Salir");

    } else {
      if (remoteSubMode == 0) {
        u8g2.drawStr(14, 11, "MODO: MULTIMEDIA");
        u8g2.drawHLine(0, 13, 128);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(6, 26, "GIRA: Subir/Bajar Volumen");
        u8g2.drawStr(6, 38, "OK:   Play / Pause");
        u8g2.drawStr(6, 58, "HOLD: Volver Selector");

        int step = encStep();
        if (step < 0) {
          for (int i = 0; i < -step; i++) bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        } else if (step > 0) {
          for (int i = 0; i < step; i++) bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        }
        if (encOk()) bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);

      } else if (remoteSubMode == 1) {
        const char *actNames[5] = {
            "MOVER EJE X (H)",
            "MOVER EJE Y (V)",
            "CLIC IZQUIERDO",
            "CLIC DERECHO",
            "RUEDA SCROLL"};

        u8g2.drawStr(12, 11, "MODO: BLE MOUSE");
        u8g2.drawHLine(0, 13, 128);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.setCursor(4, 25);
        u8g2.printf("Accion: %s", actNames[mouseAction]);
        u8g2.drawStr(4, 37, "GIRA: Mover / Clic / Scroll");
        u8g2.drawStr(4, 48, "OK:   Cambiar Eje / Clic");
        u8g2.drawStr(4, 59, "HOLD: Volver Selector");

        if (encOk()) {
          mouseAction = (mouseAction + 1) % 5;
        }

        int step = encStep();
        if (step != 0) {
          int8_t delta = (int8_t)constrain(step * 18, -120, 120);
          if (mouseAction == 0) {
            bleKeyboard.moveMouse(delta, 0, 0, 0);
          } else if (mouseAction == 1) {
            bleKeyboard.moveMouse(0, delta, 0, 0);
          } else if (mouseAction == 2) {
            bleKeyboard.clickMouse(0x01);
          } else if (mouseAction == 3) {
            bleKeyboard.clickMouse(0x02);
          } else if (mouseAction == 4) {
            bleKeyboard.moveMouse(0, 0, (int8_t)(-step * 2), 0);
          }
        }

      } else if (remoteSubMode == 2) {
        u8g2.drawStr(10, 11, "MODO: REELS / PPT");
        u8g2.drawHLine(0, 13, 128);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(6, 26, "GIRA: Siguiente / Anterior");
        u8g2.drawStr(6, 38, "OK:   Pausa / Espacio");
        u8g2.drawStr(6, 58, "HOLD: Volver Selector");

        int step = encStep();
        if (step > 0) {
          bleKeyboard.write(KEY_DOWN_ARROW);
        } else if (step < 0) {
          bleKeyboard.write(KEY_UP_ARROW);
        }
        if (encOk()) {
          bleKeyboard.write(' ');
        }

      } else if (remoteSubMode == 3) {
        u8g2.drawStr(8, 11, "DUCKY SCRIPT SPIFFS");
        u8g2.drawHLine(0, 13, 128);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.setCursor(6, 25);
        u8g2.printf("Archivo: /payload.txt (%s)", s_layoutEs ? "ES" : "EN");
        u8g2.drawStr(6, 36, "[OK] Ejecutar Ducky Script");
        u8g2.setCursor(6, 47);
        u8g2.printf("Lineas ejecutadas: %d", s_duckyLinesExecuted);
        u8g2.drawStr(6, 59, "HOLD: Volver Selector");

        if (encOk()) {
          u8g2.drawStr(6, 47, "EJECUTANDO DUCKY...     ");
          u8g2.sendBuffer();
          executeDuckyScriptFromSpiffs();
        }
      } else if (remoteSubMode == 5) {
        u8g2.drawStr(10, 11, "MODO: BLE GAMEPAD");
        u8g2.drawHLine(0, 13, 128);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(6, 26, "GIRA: Stick Izq / Der (A/D)");
        u8g2.drawStr(6, 38, "OK:   Boton Accion (SPACE)");
        u8g2.drawStr(6, 58, "HOLD: Volver Selector");

        int step = encStep();
        if (step < 0) bleKeyboard.write(KEY_LEFT_ARROW);
        else if (step > 0) bleKeyboard.write(KEY_RIGHT_ARROW);
        if (encOkDown() || encOk()) bleKeyboard.write(' ');
      }

      if (encBack()) {
        modeSelected = false;
        encClear();
      }
    }
  }

  u8g2.sendBuffer();
}
