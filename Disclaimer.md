# Legal Disclaimer & Ethical Use Policy (`BWifi-ESP32`)

> **IMPORTANT — FOR AUTHORIZED SECURITY AUDITING, EDUCATIONAL RESEARCH, AND LABORATORY ENVIRONMENTS (`LAB MODE`) ONLY**

---

## 1. Purpose and Intended Scope

**BWifi-ESP32 (BWifiKill v3.5)** is an open-source hardware and firmware research platform designed exclusively for:

1. **Academic & Educational Research**: Understanding IEEE 802.11 management frames, Bluetooth Low Energy (BLE) advertising protocols, Enhanced ShockBurst (ESB) 2.4 GHz packet structures, and embedded real-time signal processing on Espressif SoCs.
2. **Defensive Wireless Auditing**: Detecting rogue access points (*Evil Twin*), WiFi Pineapple *Karma* behavior, hidden SSIDs, MAC address randomization, and unauthorized physical tracking tags (*Apple AirTag*, *Samsung SmartTag*, *Tile*, *Google Find My*).
3. **RF Spectrum Diagnostics**: Visualizing 2.4 GHz ISM band channel occupancy (`2.400 GHz – 2.525 GHz`), identifying protocol interference (*WiFi*, *BLE*, *Zigbee*), and selecting optimal non-overlapping channels (`1`, `6`, `11`).
4. **Authorized Laboratory Testing (`LAB MODE`)**: Evaluating network resilience and intrusion detection systems (IDS/WIDS) strictly within isolated laboratory environments or on networks/devices owned by the operator or covered by explicit, written authorization.

---

## 2. Regulatory & Legal Compliance

The operation of radio frequency (RF) transmitters and active wireless auditing tools is strictly regulated by national and international telecommunications authorities, including but not limited to:

- **United States**: Federal Communications Commission (**FCC**) — Title 47 CFR Part 15 & Communications Act of 1934 (Section 333).
- **European Union**: Radio Equipment Directive (**RED 2014/53/EU**) & national spectrum agencies.
- **Dominican Republic**: Instituto Dominicano de las Telecomunicaciones (**INDOTEL**) — Ley General de Telecomunicaciones No. 153-98.
- **International**: International Telecommunication Union (**ITU**) Radio Regulations.

### Strictly Prohibited Actions

 Under no circumstances may this firmware or hardware be used to:
- Transmit continuous carrier waves, RF interference, or deauthentication frames against public, commercial, or third-party wireless networks without prior written consent.
- Intercept, capture, or collect credentials, personal data, or network traffic from unauthorized third-party users.
- Deploy captive portals (*Evil Portal*) or HID macro scripts (*Ducky Script*) on systems you do not own or lack explicit written permission to test.

---

## 3. Built-In Hardware & Software Safeguards

To promote responsible operation and prevent accidental or unauthorized activation, **BWifi-ESP32 v3.5** incorporates native firmware safeguards configurable under **`21. SETTINGS / TEST` (`AJUSTES / TEST`)**:

| Safeguard Feature | NVS Key | Description |
|---|---|---|
| **Boot Legal Notice (`AVISO LAB`)** | `legal` | Displays a mandatory ethical use acknowledgment screen on boot before granting access to the main carousel. |
| **Startup PIN Lock (`PIN INICIO`)** | `pin` | Protects the device at power-on with a 4-digit rotary encoder PIN (`1234`) to prevent unauthorized physical use. |
| **Operating Profile (`PERFIL MODO`)** | `prof` | Allows switching the operational context between **`LAB`** (Laboratory), **`AUDIT`** (Passive Defensive Auditing), and **`DEMO`** (UI Demonstration). |
| **Hardware Self-Test (`SELF-TEST HW`)** | — | Verifies SPI radio connectivity (`jam1`, `jam2`), `SPIFFS` storage health, and available `Heap` before operation. |

---

## 4. Limitation of Liability

THE SOFTWARE AND HARDWARE DESIGNS ARE PROVIDED **"AS IS"**, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT.

IN NO EVENT SHALL THE AUTHOR (**JORGE GASPAR BELTRE RIVERA**) OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, WITHOUT LIMITATION, LEGAL PENALTIES, EQUIPMENT DAMAGE, DATA LOSS, OR SERVICE INTERRUPTION) ARISING OUT OF THE USE, MISUSE, OR INABILITY TO USE THIS SOFTWARE OR HARDWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

By compiling, flashing, or operating **BWifi-ESP32**, you acknowledge that you have read this [Disclaimer](Disclaimer.md), understand your local laws and regulations, and accept full sole responsibility for your actions.

