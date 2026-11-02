#include "Infrastructure/Bluetooth/ble_trackers.h"
#include "Infrastructure/Input/encoder_input.h"
#include "Infrastructure/Config/settings_menu.h"
#include <BLEAdvertising.h>
#include <BLEBeacon.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEUtils.h>
#include <SPIFFS.h>
#include <U8g2lib.h>
#include <math.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern bool runningApp;

struct TrackerHit {
  char type[14];
  char mac[18];
  int rssi;
  float distMeters;
  uint32_t seenMs;
};

static const int kMaxTrackers = 10;
static TrackerHit s_hits[kMaxTrackers];
static int s_hitCount = 0;
static int s_scrollIdx = 0;

// 0: Submenu, 1: Detector AirTag/Tile, 2: Emisor iBeacon, 3: Medidor Distancia BLE (m)
static int s_subMode = 0;
static int s_menuChoice = 0;
static bool s_beaconRunning = false;
static uint16_t s_beaconMinor = 1;
static uint32_t s_lastScanMs = 0;

static char s_closestMac[18] = "Ninguno";
static int s_closestRssi = -100;
static float s_closestDistM = 99.9f;

// Estimador log-distance path loss: d = 10 ^ ((TxPower - RSSI) / (10 * n))
static float estimateBleDistanceMeters(int rssi, int txPowerAt1m = -59, float pathLossN = 2.2f) {
  if (rssi >= 0 || rssi < -110) return 99.9f;
  float ratio = (float)(txPowerAt1m - rssi) / (10.0f * pathLossN);
  float d = powf(10.0f, ratio);
  if (d < 0.1f) d = 0.1f;
  if (d > 99.9f) d = 99.9f;
  return d;
}

static void recordTrackerHit(const char *type, const char *mac, int rssi) {
  float distM = estimateBleDistanceMeters(rssi);
  for (int i = 0; i < s_hitCount; i++) {
    if (strcmp(s_hits[i].mac, mac) == 0) {
      s_hits[i].rssi = rssi;
      s_hits[i].distMeters = distM;
      s_hits[i].seenMs = millis();
      return;
    }
  }

  int idx = s_hitCount;
  if (idx < kMaxTrackers) {
    s_hitCount++;
  } else {
    for (int i = 0; i < kMaxTrackers - 1; i++) {
      s_hits[i] = s_hits[i + 1];
    }
    idx = kMaxTrackers - 1;
  }

  strncpy(s_hits[idx].type, type, sizeof(s_hits[idx].type) - 1);
  s_hits[idx].type[sizeof(s_hits[idx].type) - 1] = '\0';
  strncpy(s_hits[idx].mac, mac, sizeof(s_hits[idx].mac) - 1);
  s_hits[idx].mac[sizeof(s_hits[idx].mac) - 1] = '\0';
  s_hits[idx].rssi = rssi;
  s_hits[idx].distMeters = distM;
  s_hits[idx].seenMs = millis();

  if (SPIFFS.begin(true)) {
    rotateLogIfNeeded("/trackers.txt", 16384);
    File f = SPIFFS.open("/trackers.txt", FILE_APPEND);
    if (f) {
      f.printf("[%lus] %s | MAC: %s | RSSI: %d dBm (~%.1fm)\n",
               (unsigned long)(millis() / 1000), type, mac, rssi, distM);
      f.close();
    }
  }
}

static void runTrackerScan(bool distanceAllMode) {
  if (!BLEDevice::getInitialized()) {
    BLEDevice::init("");
  }
  BLEScan *scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(80);
  scan->setWindow(70);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawRBox(0, 0, 128, 12, 2);
  u8g2.setDrawColor(0);
  u8g2.drawStr(8, 9, distanceAllMode ? "RADAR DISTANCIA BLE" : "ESCANEANDO TRACKERS");
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(8, 28, "Midiendo RSSI -> Metros");
  u8g2.drawStr(8, 40, "AirTag / SmartTag / Tile");
  u8g2.drawCircle(64, 52, 8);
  u8g2.drawDisc(64, 52, 2);
  u8g2.sendBuffer();

  BLEScanResults results = scan->start(2, false);
  int count = results.getCount();
  int bestRssi = -110;

  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice dev = results.getDevice(i);
    String mac = dev.getAddress().toString().c_str();
    mac.toUpperCase();
    int rssi = dev.getRSSI();

    if (rssi > bestRssi) {
      bestRssi = rssi;
      strncpy(s_closestMac, mac.c_str(), sizeof(s_closestMac) - 1);
      s_closestMac[sizeof(s_closestMac) - 1] = '\0';
      s_closestRssi = rssi;
      s_closestDistM = estimateBleDistanceMeters(rssi);
    }

    const char *detectedType = nullptr;

    if (dev.haveManufacturerData()) {
      std::string mfg = dev.getManufacturerData();
      if (mfg.length() >= 4) {
        uint16_t companyId = (uint8_t)mfg[0] | ((uint16_t)(uint8_t)mfg[1] << 8);
        uint8_t subType = (uint8_t)mfg[2];
        if (companyId == 0x004C) {
          if (subType == 0x12 || subType == 0x07 || subType == 0x10) {
            detectedType = "AIRTAG/FINDMY";
          } else if (subType == 0x02) {
            detectedType = "iBEACON APPLE";
          }
        } else if (companyId == 0x0075) {
          detectedType = "SMARTTAG/SAM";
        }
      }
    }

    if (!detectedType && dev.haveServiceUUID()) {
      String uuid = dev.getServiceUUID().toString().c_str();
      uuid.toLowerCase();
      if (uuid.indexOf("feed") >= 0 || uuid.indexOf("feec") >= 0) {
        detectedType = "TILE TRACKER";
      } else if (uuid.indexOf("fd5a") >= 0) {
        detectedType = "SMARTTAG";
      } else if (uuid.indexOf("fe2c") >= 0 || uuid.indexOf("fd69") >= 0) {
        detectedType = "GOOGLE FINDMY";
      }
    }

    if (detectedType != nullptr) {
      recordTrackerHit(detectedType, mac.c_str(), rssi);
    }
  }
  scan->clearResults();
}

static void startIBeacon() {
  if (!BLEDevice::getInitialized()) {
    BLEDevice::init("BWifi-iBeacon");
  }
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->stop();

  BLEBeacon beacon;
  beacon.setManufacturerId(0x4C00);
  beacon.setProximityUUID(BLEUUID("E2C56DB5-DFFB-48D2-B060-D0F5A71096E0"));
  beacon.setMajor(1);
  beacon.setMinor(s_beaconMinor);
  beacon.setSignalPower(-59);

  BLEAdvertisementData advData;
  advData.setFlags(0x04);
  std::string strServiceData = "";
  strServiceData += (char)26;
  strServiceData += (char)0xFF;
  strServiceData += beacon.getData();
  advData.addData(strServiceData);

  adv->setAdvertisementData(advData);
  adv->setAdvertisementType(ADV_TYPE_NONCONN_IND);
  adv->start();
  s_beaconRunning = true;
}

static void stopIBeacon() {
  if (BLEDevice::getInitialized()) {
    BLEDevice::getAdvertising()->stop();
  }
  s_beaconRunning = false;
}

void bleTrackersSetup() {
  s_subMode = 0;
  s_menuChoice = 0;
  s_beaconRunning = false;
  encClear();
}

void bleTrackersLoop() {
  if (s_subMode == 0) {
    const int kMenuCount = 4;
    int d = encStep();
    if (d != 0) {
      s_menuChoice = (s_menuChoice + d) % kMenuCount;
      if (s_menuChoice < 0) s_menuChoice += kMenuCount;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setDrawColor(0);
    u8g2.drawStr(8, 9, "BLE TRACKERS & RADAR");
    u8g2.setDrawColor(1);

    const char *opts[kMenuCount] = {
        "1. DETECTOR AIRTAG/TILE",
        "2. EMISOR iBEACON",
        "3. MEDIDOR DISTANCIA (m)",
        "4. <- VOLVER AL MENU"};

    u8g2.setFont(u8g2_font_5x7_tr);
    for (int i = 0; i < kMenuCount; i++) {
      int y = 23 + i * 11;
      if (i == s_menuChoice) {
        u8g2.drawRBox(4, y - 8, 120, 10, 2);
        u8g2.setDrawColor(0);
        u8g2.drawStr(8, y, opts[i]);
        u8g2.setDrawColor(1);
      } else {
        u8g2.drawStr(8, y, opts[i]);
      }
    }
    u8g2.sendBuffer();

    if (encOk()) {
      if (s_menuChoice == 0) {
        s_subMode = 1;
        s_lastScanMs = 0;
      } else if (s_menuChoice == 1) {
        s_subMode = 2;
        startIBeacon();
      } else if (s_menuChoice == 2) {
        s_subMode = 3;
        s_lastScanMs = 0;
      } else {
        runningApp = false;
      }
      encClear();
    }
    if (encBack()) {
      runningApp = false;
      encClear();
    }
    return;
  }

  if (s_subMode == 1) {
    if (s_lastScanMs == 0 || (millis() - s_lastScanMs > 5000) || encOk()) {
      runTrackerScan(false);
      s_lastScanMs = millis();
    }

    int d = encStep();
    if (d != 0 && s_hitCount > 0) {
      s_scrollIdx = (s_scrollIdx + d) % s_hitCount;
      if (s_scrollIdx < 0) s_scrollIdx += s_hitCount;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(2, 9, "RADAR TAGS:");
    u8g2.setCursor(70, 9);
    u8g2.print(s_hitCount);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(88, 9, "[OK:SCAN]");
    u8g2.drawHLine(0, 11, 128);

    if (s_hitCount == 0) {
      u8g2.drawStr(8, 28, "Sin AirTag/SmartTag/Tile");
      u8g2.drawStr(8, 40, "cercanos detectados.");
    } else {
      TrackerHit &h = s_hits[s_scrollIdx];
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.setCursor(2, 22);
      u8g2.printf("> %s", h.type);

      u8g2.setFont(u8g2_font_5x7_tr);
      u8g2.setCursor(2, 33);
      u8g2.printf("MAC: %s", h.mac);

      int prox = map(constrain(h.rssi, -95, -35), -95, -35, 4, 96);
      u8g2.setCursor(2, 44);
      u8g2.printf("%ddBm (~%.1fm)", h.rssi, h.distMeters);
      u8g2.drawFrame(78, 38, 46, 7);
      u8g2.drawBox(80, 40, (prox * 42) / 96, 3);
    }

    u8g2.drawHLine(0, 52, 128);
    u8g2.drawStr(4, 61, "Gira:Ver | [HOLD] Volver");
    u8g2.sendBuffer();

    if (encBack()) {
      s_subMode = 0;
      encClear();
    }
    return;
  }

  if (s_subMode == 2) {
    int d = encStep();
    if (d != 0) {
      int m = (int)s_beaconMinor + d;
      if (m < 1) m = 99;
      if (m > 99) m = 1;
      s_beaconMinor = (uint16_t)m;
      if (s_beaconRunning) {
        startIBeacon();
      }
    }

    if (encOk()) {
      if (s_beaconRunning) stopIBeacon();
      else startIBeacon();
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setDrawColor(0);
    u8g2.drawStr(18, 9, "EMISOR iBEACON");
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 23, "UUID: E2C56DB5-DFFB-48D2");
    u8g2.setCursor(4, 34);
    u8g2.printf("Major: 1 | Minor: %u (Gira)", (unsigned)s_beaconMinor);
    u8g2.setCursor(4, 45);
    u8g2.printf("Estado: %s", s_beaconRunning ? "EMITIENDO BLE" : "PAUSADO");

    u8g2.drawHLine(0, 52, 128);
    u8g2.drawStr(4, 61, "[OK] ON/OFF  [HOLD] Volver");
    u8g2.sendBuffer();

    if (encBack()) {
      stopIBeacon();
      s_subMode = 0;
      encClear();
    }
    return;
  }

  if (s_subMode == 3) {
    if (s_lastScanMs == 0 || (millis() - s_lastScanMs > 3000) || encOk()) {
      runTrackerScan(true);
      s_lastScanMs = millis();
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setDrawColor(0);
    u8g2.drawStr(8, 9, "BLE DISTANCE ESTIMATOR");
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(4, 23);
    u8g2.printf("Mas cercano: %s", s_closestMac);
    u8g2.setCursor(4, 34);
    u8g2.printf("RSSI: %d dBm", s_closestRssi);

    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.setCursor(4, 48);
    u8g2.printf("Dist: ~%.2f metros", s_closestDistM);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawHLine(0, 53, 128);
    u8g2.drawStr(4, 62, "[OK] Medir  [HOLD] Volver");
    u8g2.sendBuffer();

    if (encBack()) {
      s_subMode = 0;
      encClear();
    }
  }
}
