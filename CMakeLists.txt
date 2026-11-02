cmake_minimum_required(VERSION 3.14)
project(bwifi_esp32 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HOST_BUILD        "Build pure Domain+Application for host (no Arduino/ESP-IDF)" ON)
option(BWIFI_BUILD_TESTS "Build host unit tests (requires HOST_BUILD)"                 OFF)

set(BWIFI_APPLICATION_SOURCES
    src/Application/Services/CarouselKinematicsService.cpp
    src/Application/Services/ScreensaverService.cpp
    src/Application/Services/ModuleRegistryService.cpp
    src/Application/Controllers/BWifiFirmwareController.cpp
)

set(BWIFI_INFRASTRUCTURE_SOURCES
    src/Infrastructure/Config/settings_menu.cpp
    src/Infrastructure/Display/U8g2DisplayController.cpp
    src/Infrastructure/Display/gui_helper.cpp
    src/Infrastructure/Input/encoder_input.cpp
    src/Infrastructure/Radio/Esp32RadioStackManager.cpp
    src/Infrastructure/System/EspSystemDiagnostics.cpp
    src/Infrastructure/System/log_viewer.cpp
    src/Infrastructure/System/about_info.cpp
    src/Infrastructure/Modules/FirmwareModules.cpp
    src/Infrastructure/WiFi/wifiscan.cpp
    src/Infrastructure/WiFi/wifi_tools.cpp
    src/Infrastructure/WiFi/packet_monitor.cpp
    src/Infrastructure/WiFi/centinela.cpp
    src/Infrastructure/WiFi/beacon_spam.cpp
    src/Infrastructure/WiFi/evil_portal.cpp
    src/Infrastructure/WiFi/ip_scanner.cpp
    src/Infrastructure/Bluetooth/btscan.cpp
    src/Infrastructure/Bluetooth/ble_trackers.cpp
    src/Infrastructure/Bluetooth/bt_jammer.cpp
    src/Infrastructure/Bluetooth/ble_spam.cpp
    src/Infrastructure/Bluetooth/bt_remote.cpp
    src/Infrastructure/Bluetooth/bad_bt.cpp
    src/Infrastructure/Rf24/spectrograph.cpp
    src/Infrastructure/Rf24/jammer.cpp
    src/Infrastructure/Rf24/total_jammer.cpp
    src/Infrastructure/Rf24/hybrid_attack.cpp
    src/Infrastructure/EspNow/slave.control.cpp
    src/Infrastructure/Web/web_dashboard.cpp
    src/Infrastructure/Arcade/games_menu.cpp
    src/Infrastructure/Arcade/snake_game.cpp
    src/Infrastructure/Arcade/pong_game.cpp
    src/Infrastructure/Arcade/flappy_game.cpp
    src/Infrastructure/Arcade/space_game.cpp
    src/Infrastructure/Arcade/dino_game.cpp
)

add_library(bwifi_core STATIC ${BWIFI_APPLICATION_SOURCES})
target_include_directories(bwifi_core PUBLIC src src/Infrastructure/Config .)

if(NOT HOST_BUILD)
    target_sources(bwifi_core PRIVATE ${BWIFI_INFRASTRUCTURE_SOURCES})
    add_executable(bwifi_firmware src/Presentation/main.cpp)
    target_link_libraries(bwifi_firmware PRIVATE bwifi_core)
endif()

