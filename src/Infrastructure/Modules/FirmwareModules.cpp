#include "Infrastructure/Modules/FirmwareModules.h"
#include "Infrastructure/System/about_info.h"
#include "Infrastructure/WiFi/beacon_spam.h"
#include "Infrastructure/Bluetooth/ble_spam.h"
#include "Infrastructure/Bluetooth/ble_trackers.h"
#include "Infrastructure/Bluetooth/bt_jammer.h"
#include "Infrastructure/Bluetooth/bt_remote.h"
#include "Infrastructure/Bluetooth/btscan.h"
#include "Infrastructure/WiFi/centinela.h"
#include "Infrastructure/WiFi/evil_portal.h"
#include "Infrastructure/Arcade/games_menu.h"
#include "Infrastructure/Rf24/hybrid_attack.h"
#include "Infrastructure/WiFi/ip_scanner.h"
#include "Infrastructure/Rf24/jammer.h"
#include "Infrastructure/System/log_viewer.h"
#include "Infrastructure/WiFi/packet_monitor.h"
#include "Infrastructure/Config/settings_menu.h"
#include "Infrastructure/EspNow/slave_control.h"
#include "Infrastructure/Arcade/snake_game.h"
#include "Infrastructure/Rf24/spectrograph.h"
#include "Infrastructure/Rf24/total_jammer.h"
#include "Infrastructure/Web/web_dashboard.h"
#include "Infrastructure/Web/web_server.h"
#include "Infrastructure/WiFi/wifi_tools.h"
#include "Infrastructure/WiFi/wifiscan.h"

namespace bwifi::infra {

namespace {

void arcadeModuleSetup() {
    ::inGame = false;
    ::resetSnake();
}

using domain::ModuleCategory;
using domain::ModuleDescriptor;
using domain::ModuleId;

static CallbackAppModule s_modules[] = {
    // 00: WIFI SCANNER
    CallbackAppModule(
        ModuleDescriptor{ModuleId::WifiScanner, "WIFI SCANNER", ModuleCategory::WiFi, true, false},
        &::wifiscanSetup, &::wifiscanLoop),
    // 01: WIFI AUDIT PRO
    CallbackAppModule(
        ModuleDescriptor{ModuleId::WifiAuditPro, "WIFI AUDIT PRO", ModuleCategory::WiFi, true, false},
        &::wifiToolsSetup, &::wifiToolsLoop),
    // 02: ANALIZADOR
    CallbackAppModule(
        ModuleDescriptor{ModuleId::Analyzer, "ANALIZADOR", ModuleCategory::Rf24, true, false},
        &::spectrographSetup, &::spectrographLoop),
    // 03: BT SCANNER
    CallbackAppModule(
        ModuleDescriptor{ModuleId::BtScanner, "BT SCANNER", ModuleCategory::Bluetooth, true, false},
        &::btscanSetup, &::btscanLoop),
    // 04: BLE TRACKERS
    CallbackAppModule(
        ModuleDescriptor{ModuleId::BleTrackers, "BLE TRACKERS", ModuleCategory::Bluetooth, true, false},
        &::bleTrackersSetup, &::bleTrackersLoop),
    // 05: PACKET MONITOR
    CallbackAppModule(
        ModuleDescriptor{ModuleId::PacketMonitor, "PACKET MONITOR", ModuleCategory::WiFi, true, false},
        &::monitorSetup, &::monitorLoop),
    // 06: MODO CENTINELA
    CallbackAppModule(
        ModuleDescriptor{ModuleId::SentinelMode, "MODO CENTINELA", ModuleCategory::WiFi, true, false},
        nullptr, &::centinelaLoop),
    // 07: JAMMER CANAL
    CallbackAppModule(
        ModuleDescriptor{ModuleId::ChannelJammer, "JAMMER CANAL", ModuleCategory::Rf24, true, false},
        &::jammerSetup, &::jammerLoop),
    // 08: BARRIDO TOTAL
    CallbackAppModule(
        ModuleDescriptor{ModuleId::TotalJammer, "BARRIDO TOTAL", ModuleCategory::Rf24, true, false},
        &::totalJammerSetup, &::totalJammerLoop),
    // 09: BT JAMMER
    CallbackAppModule(
        ModuleDescriptor{ModuleId::BtJammer, "BT JAMMER", ModuleCategory::Bluetooth, true, false},
        &::btJammerSetup, &::btJammerLoop),
    // 10: BEACON SPAM
    CallbackAppModule(
        ModuleDescriptor{ModuleId::BeaconSpam, "BEACON SPAM", ModuleCategory::WiFi, true, false},
        nullptr, &::beaconSpamLoop),
    // 11: BLE SPAM (POP)
    CallbackAppModule(
        ModuleDescriptor{ModuleId::BleSpam, "BLE SPAM (POP)", ModuleCategory::Bluetooth, true, false},
        nullptr, &::bleSpamLoop),
    // 12: MODO HIBRIDO
    CallbackAppModule(
        ModuleDescriptor{ModuleId::HybridMode, "MODO HIBRIDO", ModuleCategory::Rf24, true, false},
        nullptr, &::hybridAttackLoop),
    // 13: EVIL PORTAL
    CallbackAppModule(
        ModuleDescriptor{ModuleId::EvilPortal, "EVIL PORTAL", ModuleCategory::WiFi, true, false},
        nullptr, &::evilPortalLoop),
    // 14: IP SCANNER (manages its own hierarchical back navigation)
    CallbackAppModule(
        ModuleDescriptor{ModuleId::IpScanner, "IP SCANNER", ModuleCategory::WiFi, false, false},
        &::ipScannerSetup, &::ipScannerLoop),
    // 15: CONTROL ESCLAVO
    CallbackAppModule(
        ModuleDescriptor{ModuleId::SlaveControl, "CONTROL ESCLAVO", ModuleCategory::EspNow, true, false},
        &::slaveControlSetup, &::slaveControlLoop),
    // 16: WEB DASHBOARD (requires stopping AsyncWebServer & SoftAP on exit)
    CallbackAppModule(
        ModuleDescriptor{ModuleId::WebDashboard, "WEB DASHBOARD", ModuleCategory::Web, true, true},
        &::startWebServer, &::webDashboardLoop),
    // 17: BT REMOTE
    CallbackAppModule(
        ModuleDescriptor{ModuleId::BtRemote, "BT REMOTE", ModuleCategory::Bluetooth, true, false},
        nullptr, &::btRemoteLoop),
    // 18: LEER LOGS
    CallbackAppModule(
        ModuleDescriptor{ModuleId::LogViewer, "LEER LOGS", ModuleCategory::System, true, false},
        nullptr, &::logViewerLoop),
    // 19: ARCADE
    CallbackAppModule(
        ModuleDescriptor{ModuleId::Arcade, "ARCADE", ModuleCategory::Arcade, true, false},
        &arcadeModuleSetup, &::gamesLoop),
    // 20: AJUSTES / TEST
    CallbackAppModule(
        ModuleDescriptor{ModuleId::SettingsTest, "AJUSTES / TEST", ModuleCategory::System, true, false},
        &::settingsSetup, &::settingsLoop),
    // 21: ABOUT
    CallbackAppModule(
        ModuleDescriptor{ModuleId::About, "ABOUT", ModuleCategory::System, true, false},
        nullptr, &::aboutLoop)
};

} // namespace

void registerAllFirmwareModules(app::ModuleRegistryService& registry) {
    for (auto& mod : s_modules) {
        registry.registerModule(mod);
    }
}

} // namespace bwifi::infra
