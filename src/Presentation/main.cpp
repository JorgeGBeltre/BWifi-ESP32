#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <RF24.h>
#include <U8g2lib.h>

#include "Application/Controllers/BWifiFirmwareController.h"
#include "Application/Services/ModuleRegistryService.h"
#include "Application/Services/ScreensaverService.h"
#include "Domain/Entities/DeviceSettings.h"

#include "Infrastructure/Config/BoardConfig.h"
#include "Infrastructure/Config/NvsSettingsRepository.h"
#include "Infrastructure/Display/U8g2DisplayController.h"
#include "Infrastructure/Input/Hw040EncoderInput.h"
#include "Infrastructure/Modules/FirmwareModules.h"
#include "Infrastructure/Radio/Esp32RadioStackManager.h"
#include "Infrastructure/System/EspSystemDiagnostics.h"
#include "Infrastructure/Time/ArduinoClock.h"

using namespace bwifi;

// --- SHARED FIRMWARE STATE (USED BY SUBSYSTEM MODULES) ---
RTC_DATA_ATTR bool forceBleMode = false;
bool isTotalAttacking = false;
bool isHybridActive = false;
String target_ssid = "Ninguna";
int target_channel = 1;

extern uint32_t g_bootCount;

int menu_index = 0;
float currentPos = 0;
bool runningApp = false;
int TOTAL_OPTIONS = static_cast<int>(domain::ModuleId::Count);

const char* menu_labels[] = {
    "WIFI SCANNER",    // 0
    "WIFI AUDIT PRO",  // 1
    "ANALIZADOR",      // 2
    "BT SCANNER",      // 3
    "BLE TRACKERS",    // 4
    "PACKET MONITOR",  // 5
    "MODO CENTINELA",  // 6
    "JAMMER CANAL",    // 7
    "BARRIDO TOTAL",   // 8
    "BT JAMMER",       // 9
    "BEACON SPAM",     // 10
    "BLE SPAM (POP)",  // 11
    "MODO HIBRIDO",    // 12
    "EVIL PORTAL",     // 13
    "IP SCANNER",      // 14
    "CONTROL ESCLAVO", // 15
    "WEB DASHBOARD",   // 16
    "BT REMOTE",       // 17
    "LEER LOGS",       // 18
    "ARCADE",          // 19
    "AJUSTES / TEST",  // 20
    "ABOUT"            // 21
};

// --- HARDWARE PERIPHERAL INSTANCES ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0, U8X8_PIN_NONE, infra::BoardConfig::kOledSclPin, infra::BoardConfig::kOledSdaPin);
RF24 jam1(
    infra::BoardConfig::kNrf1CePin, infra::BoardConfig::kNrf1CsnPin, infra::BoardConfig::kNrfSpiSpeedHz);
RF24 jam2(
    infra::BoardConfig::kNrf2CePin, infra::BoardConfig::kNrf2CsnPin, infra::BoardConfig::kNrfSpiSpeedHz);
AsyncWebServer asyncServer(infra::BoardConfig::kWebServerPort);

// --- CLEAN ARCHITECTURE COMPOSITION ROOT ---
static domain::DeviceSettings        g_domainSettings;
static infra::ArduinoClock           g_clock;
static infra::Hw040EncoderInput      g_encoder;
static infra::U8g2DisplayController  g_display{u8g2, infra::BoardConfig::kOledBusClockHz};
static infra::Esp32RadioStackManager g_radios{jam1, jam2, asyncServer};
static infra::NvsSettingsRepository  g_settingsRepo;
static infra::EspSystemDiagnostics   g_diagnostics{jam1, jam2, g_bootCount};

static app::ModuleRegistryService    g_moduleRegistry;
static app::ScreensaverService       g_screensaver{g_display, g_encoder, g_clock};
static app::BWifiFirmwareController  g_controller{
    g_moduleRegistry,
    g_screensaver,
    g_display,
    g_encoder,
    g_radios,
    g_settingsRepo,
    g_diagnostics,
    g_clock,
    menu_index,
    runningApp,
    g_domainSettings
};

void setup() {
    Serial.begin(115200);
    infra::registerAllFirmwareModules(g_moduleRegistry);
    g_controller.setup();
}

void loop() {
    g_controller.tick();
}

