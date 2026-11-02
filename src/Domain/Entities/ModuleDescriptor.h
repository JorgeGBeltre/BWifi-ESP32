#pragma once

#include <cstdint>

namespace bwifi::domain {

enum class ModuleId : uint8_t {
    WifiScanner   = 0,
    WifiAuditPro  = 1,
    Analyzer      = 2,
    BtScanner     = 3,
    BleTrackers   = 4,
    PacketMonitor = 5,
    SentinelMode  = 6,
    ChannelJammer = 7,
    TotalJammer   = 8,
    BtJammer      = 9,
    BeaconSpam    = 10,
    BleSpam       = 11,
    HybridMode    = 12,
    EvilPortal    = 13,
    IpScanner     = 14,
    SlaveControl  = 15,
    WebDashboard  = 16,
    BtRemote      = 17,
    LogViewer     = 18,
    Arcade        = 19,
    SettingsTest  = 20,
    About         = 21,
    Count         = 22
};

enum class ModuleCategory : uint8_t {
    WiFi      = 0,
    Bluetooth = 1,
    Rf24      = 2,
    EspNow    = 3,
    Web       = 4,
    Arcade    = 5,
    System    = 6
};

struct ModuleDescriptor {
    ModuleId       id                           = ModuleId::WifiScanner;
    const char*    label                        = "";
    ModuleCategory category                     = ModuleCategory::System;
    bool           allowGlobalBackExit          = true;
    bool           requiresWebServerStopOnExit  = false;
};

} // namespace bwifi::domain

