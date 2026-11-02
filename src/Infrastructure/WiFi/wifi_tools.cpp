#include "Infrastructure/WiFi/wifi_tools.h"
#include "Infrastructure/Input/encoder_input.h"
#include "Infrastructure/Config/settings_menu.h"
#include <SPIFFS.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <time.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern bool runningApp;

struct ProbeEntry {
  uint8_t mac[6];
  char ssid[21];
  int8_t rssi;
  uint8_t ch;
  bool isRandomMac;
};

static const int kMaxProbes = 14;
static ProbeEntry s_probes[kMaxProbes];
static volatile int s_probeCount = 0;
static volatile uint32_t s_totalProbePkts = 0;
static volatile uint32_t s_rndMacCount = 0;
static volatile uint32_t s_realMacCount = 0;

// 0: Menu, 1: Probe+MAC Rnd, 2: Heatmap, 3: Wardrive/Kismet, 4: Rogue AP/Evil Twin, 5: Hidden SSID Unmask, 6: Karma Detector, 7: PCAP Capture
static int s_subMode = 0;
static int s_menuIdx = 0;
static int s_probeScroll = 0;
static int s_currentCh = 1;
static uint32_t s_lastHopMs = 0;

// Heatmap
static uint8_t s_chApCount[14];
static int8_t s_chBestRssi[14];
static uint8_t s_chPeakScore[14];
static int s_bestFreeCh = 1;
static int s_busiestCh = 6;
static uint32_t s_lastHeatmapScanMs = 0;

// Wardriving
static uint32_t s_wardriveTotalLogged = 0;
static uint32_t s_wardriveScans = 0;
static uint32_t s_lastWardriveScanMs = 0;
static uint8_t s_seenBssids[32][6];
static int s_seenBssidCount = 0;

// Rogue AP / Evil Twin Detector
struct RogueAlert {
  char ssid[19];
  char detail[24];
};
static RogueAlert s_rogues[8];
static int s_rogueCount = 0;
static int s_totalScannedAps = 0;
static uint32_t s_lastRogueScanMs = 0;

// Hidden SSID Unmasker & Karma Detector
struct HiddenEntry {
  uint8_t bssid[6];
  char ssid[21];
  uint8_t ch;
  bool unmasked;
};
static HiddenEntry s_hiddenList[10];
static volatile int s_hiddenCount = 0;

struct KarmaTracker {
  uint8_t bssid[6];
  char lastSsid1[16];
  char lastSsid2[16];
  uint8_t distinctSsids;
};
static KarmaTracker s_karmaList[8];
static volatile int s_karmaCount = 0;
static volatile bool s_karmaAlert = false;
static char s_karmaCulpritMac[18] = "Ninguno";

// Captura PCAP en cola circular ligera desde ISR -> SPIFFS
struct PcapSlot {
  uint8_t len;
  uint8_t data[64];
  volatile bool ready;
};
static PcapSlot s_pcapRing[4];
static uint32_t s_pcapSavedCount = 0;
static uint32_t s_pcapBytes = 0;

static void IRAM_ATTR wifiAuditPromiscCb(void *buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
  const uint8_t *frame = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  if (len < 26) return;

  uint8_t subtype = frame[0] & 0xFC;

  // Modo 7: Encolar trama para archivo .pcap
  if (s_subMode == 7) {
    for (int i = 0; i < 4; i++) {
      if (!s_pcapRing[i].ready) {
        uint8_t capLen = (len > 64) ? 64 : (uint8_t)len;
        memcpy(s_pcapRing[i].data, frame, capLen);
        s_pcapRing[i].len = capLen;
        s_pcapRing[i].ready = true;
        break;
      }
    }
    return;
  }

  // Modo 5 (Hidden SSID Unmasker) y Modo 6 (Karma Detector)
  if (s_subMode == 5 || s_subMode == 6) {
    // Beacon (0x80) o Probe Response (0x50) -> tags empiezan en offset 36
    if ((subtype == 0x80 || subtype == 0x50) && len >= 38) {
      const uint8_t *bssid = &frame[16];
      uint8_t tagId = frame[36];
      uint8_t ssidLen = frame[37];
      if (tagId == 0 && ssidLen <= 32 && (38 + ssidLen) <= len) {
        bool isHidden = (ssidLen == 0) || (frame[38] == 0);
        char ssidBuf[21];
        if (!isHidden) {
          uint8_t cLen = (ssidLen > 20) ? 20 : ssidLen;
          for (uint8_t k = 0; k < cLen; k++) {
            char c = (char)frame[38 + k];
            ssidBuf[k] = (c >= 32 && c <= 126) ? c : '?';
          }
          ssidBuf[cLen] = '\0';
        } else {
          strcpy(ssidBuf, "<OCULTO>");
        }

        if (s_subMode == 5) {
          for (int i = 0; i < s_hiddenCount; i++) {
            if (memcmp(s_hiddenList[i].bssid, bssid, 6) == 0) {
              if (!isHidden && !s_hiddenList[i].unmasked) {
                strncpy(s_hiddenList[i].ssid, ssidBuf, 20);
                s_hiddenList[i].ssid[20] = '\0';
                s_hiddenList[i].unmasked = true;
              }
              return;
            }
          }
          if (isHidden && s_hiddenCount < 10) {
            int idx = s_hiddenCount++;
            memcpy(s_hiddenList[idx].bssid, bssid, 6);
            strcpy(s_hiddenList[idx].ssid, "<OCULTO>");
            s_hiddenList[idx].ch = pkt->rx_ctrl.channel;
            s_hiddenList[idx].unmasked = false;
          }
        }

        if (s_subMode == 6 && subtype == 0x50 && !isHidden) {
          for (int i = 0; i < s_karmaCount; i++) {
            if (memcmp(s_karmaList[i].bssid, bssid, 6) == 0) {
              if (strncmp(s_karmaList[i].lastSsid1, ssidBuf, 15) != 0 &&
                  strncmp(s_karmaList[i].lastSsid2, ssidBuf, 15) != 0) {
                strncpy(s_karmaList[i].lastSsid2, s_karmaList[i].lastSsid1, 15);
                strncpy(s_karmaList[i].lastSsid1, ssidBuf, 15);
                s_karmaList[i].distinctSsids++;
                if (s_karmaList[i].distinctSsids >= 3) {
                  s_karmaAlert = true;
                  snprintf(s_karmaCulpritMac, sizeof(s_karmaCulpritMac),
                           "%02X:%02X:%02X:%02X:%02X:%02X",
                           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                }
              }
              return;
            }
          }
          if (s_karmaCount < 8) {
            int idx = s_karmaCount++;
            memcpy(s_karmaList[idx].bssid, bssid, 6);
            strncpy(s_karmaList[idx].lastSsid1, ssidBuf, 15);
            s_karmaList[idx].lastSsid1[15] = '\0';
            s_karmaList[idx].lastSsid2[0] = '\0';
            s_karmaList[idx].distinctSsids = 1;
          }
        }
      }
    }
    // Association Request (0x00) -> tags empiezan en offset 28, BSSID en frame[16]
    if (s_subMode == 5 && subtype == 0x00 && len >= 30) {
      const uint8_t *bssid = &frame[16];
      uint8_t tagId = frame[28];
      uint8_t ssidLen = frame[29];
      if (tagId == 0 && ssidLen > 0 && ssidLen <= 32 && (30 + ssidLen) <= len && frame[30] != 0) {
        char ssidBuf[21];
        uint8_t cLen = (ssidLen > 20) ? 20 : ssidLen;
        for (uint8_t k = 0; k < cLen; k++) {
          char c = (char)frame[30 + k];
          ssidBuf[k] = (c >= 32 && c <= 126) ? c : '?';
        }
        ssidBuf[cLen] = '\0';
        for (int i = 0; i < s_hiddenCount; i++) {
          if (memcmp(s_hiddenList[i].bssid, bssid, 6) == 0) {
            strncpy(s_hiddenList[i].ssid, ssidBuf, 20);
            s_hiddenList[i].ssid[20] = '\0';
            s_hiddenList[i].unmasked = true;
            return;
          }
        }
      }
    }
    return;
  }

  // Modo 1: Probe Request (0x40) + Detector de MAC Aleatoria (bit 0x02 localmente administrado)
  if (type != WIFI_PKT_MGMT || subtype != 0x40) return;

  s_totalProbePkts++;
  const uint8_t *srcMac = &frame[10];
  bool isRnd = ((srcMac[0] & 0x02) != 0);

  uint8_t tagId = frame[24];
  uint8_t ssidLen = frame[25];

  char parsedSsid[21];
  if (tagId == 0 && ssidLen > 0 && ssidLen <= 32 && (26 + ssidLen) <= len) {
    uint8_t copyLen = (ssidLen > 20) ? 20 : ssidLen;
    for (uint8_t i = 0; i < copyLen; i++) {
      char c = (char)frame[26 + i];
      parsedSsid[i] = (c >= 32 && c <= 126) ? c : '?';
    }
    parsedSsid[copyLen] = '\0';
  } else {
    strcpy(parsedSsid, "<Broadcast>");
  }

  for (int i = 0; i < s_probeCount; i++) {
    if (memcmp(s_probes[i].mac, srcMac, 6) == 0) {
      if (parsedSsid[0] != '<') {
        strncpy(s_probes[i].ssid, parsedSsid, 20);
        s_probes[i].ssid[20] = '\0';
      }
      s_probes[i].rssi = pkt->rx_ctrl.rssi;
      s_probes[i].ch = pkt->rx_ctrl.channel;
      return;
    }
  }

  if (isRnd) s_rndMacCount++;
  else s_realMacCount++;

  if (s_probeCount < kMaxProbes) {
    int idx = s_probeCount++;
    memcpy(s_probes[idx].mac, srcMac, 6);
    strncpy(s_probes[idx].ssid, parsedSsid, 20);
    s_probes[idx].ssid[20] = '\0';
    s_probes[idx].rssi = pkt->rx_ctrl.rssi;
    s_probes[idx].ch = pkt->rx_ctrl.channel;
    s_probes[idx].isRandomMac = isRnd;
  } else {
    for (int i = 0; i < kMaxProbes - 1; i++) {
      s_probes[i] = s_probes[i + 1];
    }
    memcpy(s_probes[kMaxProbes - 1].mac, srcMac, 6);
    strncpy(s_probes[kMaxProbes - 1].ssid, parsedSsid, 20);
    s_probes[kMaxProbes - 1].ssid[20] = '\0';
    s_probes[kMaxProbes - 1].rssi = pkt->rx_ctrl.rssi;
    s_probes[kMaxProbes - 1].ch = pkt->rx_ctrl.channel;
    s_probes[kMaxProbes - 1].isRandomMac = isRnd;
  }
}

static void startPromiscEngine() {
  s_currentCh = 1;
  s_lastHopMs = millis();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(20);
  wifi_promiscuous_filter_t filt;
  filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&wifiAuditPromiscCb);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(s_currentCh, WIFI_SECOND_CHAN_NONE);
}

static void stopPromiscEngine() {
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(NULL);
}

static void runChannelHeatmapScan() {
  for (int ch = 1; ch <= 13; ch++) {
    s_chApCount[ch] = 0;
    s_chBestRssi[ch] = -100;
  }

  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks(false, true, false, 90);
  for (int i = 0; i < n; i++) {
    int ch = WiFi.channel(i);
    int rssi = WiFi.RSSI(i);
    if (ch >= 1 && ch <= 13) {
      if (s_chApCount[ch] < 50) s_chApCount[ch]++;
      if (rssi > s_chBestRssi[ch]) s_chBestRssi[ch] = (int8_t)rssi;
    }
  }
  WiFi.scanDelete();

  int minScore = 9999;
  int maxScore = -1;
  s_bestFreeCh = 1;
  s_busiestCh = 6;

  for (int ch = 1; ch <= 13; ch++) {
    int rssiWeight = (s_chBestRssi[ch] > -100) ? (105 + s_chBestRssi[ch]) / 3 : 0;
    int score = s_chApCount[ch] * 8 + rssiWeight;
    if (score > 36) score = 36;
    if (score > s_chPeakScore[ch]) s_chPeakScore[ch] = (uint8_t)score;
    else if (s_chPeakScore[ch] > 1) s_chPeakScore[ch]--;

    if (score < minScore && (ch == 1 || ch == 6 || ch == 11)) {
      minScore = score;
      s_bestFreeCh = ch;
    }
    if (score > maxScore) {
      maxScore = score;
      s_busiestCh = ch;
    }
  }
}

static void runRogueApScan() {
  s_rogueCount = 0;
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks(false, false, false, 110);
  s_totalScannedAps = (n > 0) ? n : 0;

  for (int i = 0; i < n; i++) {
    String ssidA = WiFi.SSID(i);
    if (ssidA.length() == 0) continue;
    uint8_t *bssidA = WiFi.BSSID(i);
    int chA = WiFi.channel(i);
    bool openA = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);

    for (int j = i + 1; j < n; j++) {
      if (ssidA == WiFi.SSID(j)) {
        uint8_t *bssidB = WiFi.BSSID(j);
        int chB = WiFi.channel(j);
        bool openB = (WiFi.encryptionType(j) == WIFI_AUTH_OPEN);
        if (!bssidA || !bssidB) continue;

        // Si cambia la encriptación (downgrade WPA -> OPEN) o tienen OUI distinto en diferente canal
        bool diffSecurity = (openA != openB);
        bool diffOuiOrCh = (memcmp(bssidA, bssidB, 3) != 0) || (chA != chB);

        if ((diffSecurity || diffOuiOrCh) && s_rogueCount < 8) {
          strncpy(s_rogues[s_rogueCount].ssid, ssidA.c_str(), 18);
          s_rogues[s_rogueCount].ssid[18] = '\0';
          snprintf(s_rogues[s_rogueCount].detail, sizeof(s_rogues[s_rogueCount].detail),
                   "CH%d vs CH%d %s", chA, chB, diffSecurity ? "DOWNGRADE!" : "CLON BSSID");
          s_rogueCount++;
          break;
        }
      }
    }
  }
  WiFi.scanDelete();
}

static void runWardriveStep() {
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks(false, true, false, 100);
  s_wardriveScans++;

  if (n > 0 && SPIFFS.begin(true)) {
    rotateLogIfNeeded("/wardrive.csv", 24576);
    bool exists = SPIFFS.exists("/wardrive.csv");
    File f = SPIFFS.open("/wardrive.csv", FILE_APPEND);
    if (f) {
      if (!exists) {
        f.println("timestamp,ssid,bssid,ch,rssi,enc,kismet_type");
      }
      time_t nowSec = time(nullptr);
      for (int i = 0; i < n; i++) {
        uint8_t *bssid = WiFi.BSSID(i);
        if (!bssid) continue;
        bool alreadySeen = false;
        for (int j = 0; j < s_seenBssidCount; j++) {
          if (memcmp(s_seenBssids[j], bssid, 6) == 0) {
            alreadySeen = true;
            break;
          }
        }
        if (!alreadySeen) {
          if (s_seenBssidCount < 32) {
            memcpy(s_seenBssids[s_seenBssidCount++], bssid, 6);
          } else {
            memmove(&s_seenBssids[0], &s_seenBssids[1], 31 * 6);
            memcpy(s_seenBssids[31], bssid, 6);
          }
          String ssid = WiFi.SSID(i);
          ssid.replace(",", " ");
          if (ssid.length() == 0) ssid = "<HIDDEN>";
          const char *enc = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "WPA2";
          unsigned long ts = (nowSec > 1700000000) ? (unsigned long)nowSec : (unsigned long)(millis() / 1000);
          f.printf("%lu,%s,%s,%d,%d,%s,infrastructure\n",
                   ts,
                   ssid.c_str(),
                   WiFi.BSSIDstr(i).c_str(),
                   WiFi.channel(i),
                   WiFi.RSSI(i),
                   enc);
          s_wardriveTotalLogged++;
        }
      }
      f.close();
    }
  }
  WiFi.scanDelete();
}

static void initPcapFile() {
  for (int i = 0; i < 4; i++) s_pcapRing[i].ready = false;
  s_pcapSavedCount = 0;
  s_pcapBytes = 0;
  if (!SPIFFS.begin(true)) return;
  rotateLogIfNeeded("/capture.pcap", 24576);
  if (!SPIFFS.exists("/capture.pcap")) {
    File f = SPIFFS.open("/capture.pcap", FILE_WRITE);
    if (f) {
      // Cabecera global libpcap (24 bytes): magic 0xa1b2c3d4, v2.4, snaplen 64, DLT_IEEE802_11 (105)
      const uint32_t hdr[6] = {0xA1B2C3D4, 0x00040002, 0, 0, 64, 105};
      f.write((const uint8_t *)hdr, 24);
      s_pcapBytes = 24;
      f.close();
    }
  } else {
    File f = SPIFFS.open("/capture.pcap", FILE_READ);
    if (f) {
      s_pcapBytes = f.size();
      f.close();
    }
  }
}

static void flushPcapRingToSpiffs() {
  if (s_pcapBytes >= 24576) return;
  for (int i = 0; i < 4; i++) {
    if (s_pcapRing[i].ready) {
      File f = SPIFFS.open("/capture.pcap", FILE_APPEND);
      if (f) {
        uint32_t sec = millis() / 1000;
        uint32_t usec = (millis() % 1000) * 1000;
        uint32_t inclLen = s_pcapRing[i].len;
        uint32_t origLen = s_pcapRing[i].len;
        uint32_t pktHdr[4] = {sec, usec, inclLen, origLen};
        f.write((const uint8_t *)pktHdr, 16);
        f.write(s_pcapRing[i].data, inclLen);
        f.close();
        s_pcapSavedCount++;
        s_pcapBytes += (16 + inclLen);
      }
      s_pcapRing[i].ready = false;
    }
  }
}

void wifiToolsSetup() {
  s_subMode = 0;
  s_menuIdx = 0;
  for (int i = 0; i < 14; i++) s_chPeakScore[i] = 0;
  encClear();
}

void wifiToolsLoop() {
  const int kMenuCount = 8;
  if (s_subMode == 0) {
    int d = encStep();
    if (d != 0) {
      s_menuIdx = (s_menuIdx + d) % kMenuCount;
      if (s_menuIdx < 0) s_menuIdx += kMenuCount;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setDrawColor(0);
    u8g2.drawStr(10, 9, "WIFI AUDIT PRO v3.5");
    u8g2.setDrawColor(1);

    const char *items[kMenuCount] = {
        "1. PROBE + MAC RND",
        "2. CHANNEL HEATMAP",
        "3. WARDRIVE + KISMET",
        "4. ROGUE AP / EVIL TWIN",
        "5. HIDDEN SSID UNMASK",
        "6. KARMA / PINEAPPLE",
        "7. PCAP CAPTURE (.pcap)",
        "8. <- VOLVER AL MENU"};

    int firstVis = s_menuIdx - 2;
    if (firstVis > kMenuCount - 4) firstVis = kMenuCount - 4;
    if (firstVis < 0) firstVis = 0;

    u8g2.setFont(u8g2_font_5x7_tr);
    for (int row = 0; row < 4; row++) {
      int i = firstVis + row;
      if (i >= kMenuCount) break;
      int y = 24 + row * 11;
      if (i == s_menuIdx) {
        u8g2.drawRBox(3, y - 8, 122, 10, 2);
        u8g2.setDrawColor(0);
        u8g2.drawStr(7, y, items[i]);
        u8g2.setDrawColor(1);
      } else {
        u8g2.drawStr(7, y, items[i]);
      }
    }
    u8g2.sendBuffer();

    if (encOk()) {
      if (s_menuIdx == 0) {
        s_subMode = 1;
        s_probeCount = 0;
        s_totalProbePkts = 0;
        s_rndMacCount = 0;
        s_realMacCount = 0;
        s_probeScroll = 0;
        startPromiscEngine();
      } else if (s_menuIdx == 1) {
        s_subMode = 2;
        s_lastHeatmapScanMs = 0;
      } else if (s_menuIdx == 2) {
        s_subMode = 3;
        s_lastWardriveScanMs = 0;
      } else if (s_menuIdx == 3) {
        s_subMode = 4;
        s_lastRogueScanMs = 0;
      } else if (s_menuIdx == 4) {
        s_subMode = 5;
        s_hiddenCount = 0;
        startPromiscEngine();
      } else if (s_menuIdx == 5) {
        s_subMode = 6;
        s_karmaCount = 0;
        s_karmaAlert = false;
        strcpy(s_karmaCulpritMac, "Ninguno");
        startPromiscEngine();
      } else if (s_menuIdx == 6) {
        s_subMode = 7;
        initPcapFile();
        startPromiscEngine();
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

  // --- MODO 1: PROBE SNIFFER + MAC RANDOMIZER DETECTOR ---
  if (s_subMode == 1) {
    if (millis() - s_lastHopMs > 280) {
      s_currentCh = (s_currentCh % 13) + 1;
      esp_wifi_set_channel(s_currentCh, WIFI_SECOND_CHAN_NONE);
      s_lastHopMs = millis();
    }

    int d = encStep();
    int total = s_probeCount;
    if (d != 0 && total > 0) {
      s_probeScroll = constrain(s_probeScroll + d, 0, max(0, total - 1));
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(2, 8);
    u8g2.printf("PROBES | RND:%lu REAL:%lu", (unsigned long)s_rndMacCount, (unsigned long)s_realMacCount);
    u8g2.drawHLine(0, 10, 128);

    if (total == 0) {
      u8g2.drawStr(8, 30, "Escuchando Probes 802.11");
      u8g2.drawStr(8, 42, "Detectando MAC Random...");
    } else {
      int idx = total - 1 - s_probeScroll;
      if (idx < 0) idx = 0;
      ProbeEntry e = s_probes[idx];
      char macStr[20];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
               e.mac[0], e.mac[1], e.mac[2], e.mac[3], e.mac[4], e.mac[5]);
      u8g2.setCursor(2, 22);
      u8g2.printf("[%s] %s", e.isRandomMac ? "RND" : "REAL", macStr);
      u8g2.setCursor(2, 34);
      u8g2.printf("SSID: %s", e.ssid);
      u8g2.setCursor(2, 46);
      u8g2.printf("CH:%d | RSSI:%ddBm (%d/%d)", e.ch, e.rssi, idx + 1, total);
    }

    u8g2.drawHLine(0, 53, 128);
    u8g2.drawStr(4, 62, "Gira:Ver | [HOLD] Volver");
    u8g2.sendBuffer();

    if (encBack()) {
      stopPromiscEngine();
      s_subMode = 0;
      encClear();
    }
    return;
  }

  // --- MODO 2: CHANNEL HEATMAP ---
  if (s_subMode == 2) {
    if (s_lastHeatmapScanMs == 0 || (millis() - s_lastHeatmapScanMs > 2500) || encOk()) {
      runChannelHeatmapScan();
      s_lastHeatmapScanMs = millis();
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(2, 8);
    u8g2.printf("HEATMAP | LIBRE:CH%d SAT:CH%d", s_bestFreeCh, s_busiestCh);
    u8g2.drawHLine(0, 10, 128);

    for (int ch = 1; ch <= 13; ch++) {
      int x = 3 + (ch - 1) * 9;
      int h = s_chApCount[ch] * 7 + ((s_chBestRssi[ch] > -100) ? 4 : 0);
      if (h > 36) h = 36;
      if (h > 0) u8g2.drawBox(x, 49 - h, 6, h);
      else u8g2.drawPixel(x + 2, 48);

      int pk = s_chPeakScore[ch];
      if (pk > 36) pk = 36;
      u8g2.drawHLine(x, 49 - pk, 6);

      if (ch % 2 == 1) {
        u8g2.setCursor(x, 57);
        u8g2.print(ch);
      }
    }
    u8g2.drawHLine(0, 49, 128);
    u8g2.sendBuffer();

    if (encBack()) {
      s_subMode = 0;
      encClear();
    }
    return;
  }

  // --- MODO 3: WARDRIVING + KISMET CSV ---
  if (s_subMode == 3) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setDrawColor(0);
    u8g2.drawStr(8, 9, "WARDRIVE / KISMET CSV");
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(4, 24);
    u8g2.printf("Barridos realizados: %lu", (unsigned long)s_wardriveScans);
    u8g2.setCursor(4, 35);
    u8g2.printf("Redes unicas (CSV):  %lu", (unsigned long)s_wardriveTotalLogged);
    u8g2.setCursor(4, 46);
    u8g2.print("Formato: Kismet + NTP");
    u8g2.drawHLine(0, 52, 128);
    u8g2.drawStr(6, 61, "Descarga en WEB DASHBOARD");
    u8g2.sendBuffer();

    if (s_lastWardriveScanMs == 0 || (millis() - s_lastWardriveScanMs > 2800)) {
      runWardriveStep();
      s_lastWardriveScanMs = millis();
    }

    if (encBack()) {
      s_subMode = 0;
      encClear();
    }
    return;
  }

  // --- MODO 4: ROGUE AP / EVIL TWIN DETECTOR ---
  if (s_subMode == 4) {
    if (s_lastRogueScanMs == 0 || (millis() - s_lastRogueScanMs > 3500) || encOk()) {
      runRogueApScan();
      s_lastRogueScanMs = millis();
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    if (s_rogueCount > 0) {
      u8g2.drawRBox(0, 0, 128, 12, 2);
      u8g2.setDrawColor(0);
      u8g2.drawStr(10, 9, "! EVIL TWIN / CLON !");
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(6, 9, "DETECTOR EVIL TWIN");
      u8g2.drawHLine(0, 11, 128);
    }

    u8g2.setFont(u8g2_font_5x7_tr);
    if (s_rogueCount == 0) {
      u8g2.setCursor(6, 26);
      u8g2.printf("APs verificados: %d", s_totalScannedAps);
      u8g2.drawStr(6, 38, "Estado: OK (Sin clones");
      u8g2.drawStr(6, 47, "ni downgrade WPA->OPEN)");
    } else {
      for (int i = 0; i < min(s_rogueCount, 2); i++) {
        int y = 23 + i * 16;
        u8g2.setCursor(2, y);
        u8g2.printf("SSID: %s", s_rogues[i].ssid);
        u8g2.setCursor(2, y + 8);
        u8g2.printf(" -> %s", s_rogues[i].detail);
      }
    }
    u8g2.drawHLine(0, 53, 128);
    u8g2.drawStr(4, 62, "[OK] Escanear [HOLD] Salir");
    u8g2.sendBuffer();

    if (encBack()) {
      s_subMode = 0;
      encClear();
    }
    return;
  }

  // --- MODO 5: HIDDEN SSID UNMASKER ---
  if (s_subMode == 5) {
    if (millis() - s_lastHopMs > 300) {
      s_currentCh = (s_currentCh % 13) + 1;
      esp_wifi_set_channel(s_currentCh, WIFI_SECOND_CHAN_NONE);
      s_lastHopMs = millis();
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(2, 8);
    u8g2.printf("HIDDEN SSID CH:%02d VISTOS:%d", s_currentCh, (int)s_hiddenCount);
    u8g2.drawHLine(0, 10, 128);

    if (s_hiddenCount == 0) {
      u8g2.drawStr(6, 28, "Buscando redes ocultas");
      u8g2.drawStr(6, 40, "y ProbeResp / AssocReq...");
    } else {
      for (int i = 0; i < min((int)s_hiddenCount, 3); i++) {
        int y = 21 + i * 11;
        u8g2.setCursor(2, y);
        u8g2.printf("%02X:%02X:%02X %s",
                    s_hiddenList[i].bssid[3], s_hiddenList[i].bssid[4], s_hiddenList[i].bssid[5],
                    s_hiddenList[i].ssid);
      }
    }
    u8g2.drawHLine(0, 53, 128);
    u8g2.drawStr(6, 62, "[HOLD] Volver al Submenu");
    u8g2.sendBuffer();

    if (encBack()) {
      stopPromiscEngine();
      s_subMode = 0;
      encClear();
    }
    return;
  }

  // --- MODO 6: KARMA / PINEAPPLE DETECTOR ---
  if (s_subMode == 6) {
    if (millis() - s_lastHopMs > 300) {
      s_currentCh = (s_currentCh % 13) + 1;
      esp_wifi_set_channel(s_currentCh, WIFI_SECOND_CHAN_NONE);
      s_lastHopMs = millis();
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    if (s_karmaAlert) {
      u8g2.drawRBox(0, 0, 128, 12, 2);
      u8g2.setDrawColor(0);
      u8g2.drawStr(10, 9, "! KARMA AP DETECTADO !");
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(8, 9, "DETECTOR WIFI KARMA");
      u8g2.drawHLine(0, 11, 128);
    }

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(4, 24);
    u8g2.printf("BSSIDs analizados: %d (CH%d)", (int)s_karmaCount, s_currentCh);
    u8g2.setCursor(4, 36);
    u8g2.printf("Sospechoso: %s", s_karmaCulpritMac);
    u8g2.drawStr(4, 47, s_karmaAlert ? "Responde a multiples SSIDs!" : "Estado: Normal (Sin Karma)");
    u8g2.drawHLine(0, 53, 128);
    u8g2.drawStr(8, 62, "[HOLD] Volver al Submenu");
    u8g2.sendBuffer();

    if (encBack()) {
      stopPromiscEngine();
      s_subMode = 0;
      encClear();
    }
    return;
  }

  // --- MODO 7: CAPTURA .PCAP (WIRESHARK) ---
  if (s_subMode == 7) {
    if (millis() - s_lastHopMs > 350) {
      s_currentCh = (s_currentCh % 13) + 1;
      esp_wifi_set_channel(s_currentCh, WIFI_SECOND_CHAN_NONE);
      s_lastHopMs = millis();
    }

    flushPcapRingToSpiffs();

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setDrawColor(0);
    u8g2.drawStr(10, 9, "PACKET CAPTURE .PCAP");
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(4, 24);
    u8g2.printf("Canal actual: CH %02d", s_currentCh);
    u8g2.setCursor(4, 35);
    u8g2.printf("Tramas guardadas: %lu", (unsigned long)s_pcapSavedCount);
    u8g2.setCursor(4, 46);
    u8g2.printf("Archivo: %lu / 24576 B", (unsigned long)s_pcapBytes);
    u8g2.drawHLine(0, 52, 128);
    u8g2.drawStr(4, 61, "Descarga /capture.pcap Web");
    u8g2.sendBuffer();

    if (encBack()) {
      stopPromiscEngine();
      s_subMode = 0;
      encClear();
    }
  }
}
