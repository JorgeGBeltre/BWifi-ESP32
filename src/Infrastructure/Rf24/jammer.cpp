#include "Infrastructure/Rf24/jammer.h"
#include "Infrastructure/Input/encoder_input.h"
#include <RF24.h>
#include <U8g2lib.h>

extern RF24 jam1; 
extern RF24 jam2;
extern U8G2 u8g2;
extern bool runningApp;

int jamChannel = 1;
bool isAttacking = false;

const byte noise_payload[] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};

void jammerSetup() {
    isAttacking = false;
    jam1.begin();
    jam2.begin();
    
    jam1.setAddressWidth(3);
    jam1.setRetries(0, 0);
    jam1.setDataRate(RF24_2MBPS); 
    jam1.setAutoAck(false);
    jam1.stopListening();

    jam2.setAddressWidth(3);
    jam2.setRetries(0, 0);
    jam2.setDataRate(RF24_2MBPS);
    jam2.setAutoAck(false);
    jam2.stopListening();
    encClear();
}

void jammerLoop() {
    if (encBack()) { 
        isAttacking = false; 
        jam1.stopConstCarrier(); 
        jam2.stopConstCarrier(); 
        runningApp = false;
        return; 
    }
    
    int step = encStep();
    if (step != 0) {
        jamChannel = constrain(jamChannel + step, 1, 14);
        if (isAttacking) {
            int freq = (jamChannel * 5) + 2;
            jam1.startConstCarrier(RF24_PA_MAX, (uint8_t)freq);
        }
    }
    
    if (encOk()) { 
        isAttacking = !isAttacking; 
        int freq = (jamChannel * 5) + 2;
        if(isAttacking) { 
            jam1.startConstCarrier(RF24_PA_MAX, (uint8_t)freq);
        } else { 
            jam1.stopConstCarrier(); 
            jam2.stopConstCarrier(); 
        }
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 10, "JAMMER PRO (SATURAR)");
    u8g2.setCursor(0, 25); u8g2.print("Canal WiFi: "); u8g2.print(jamChannel);

    if (isAttacking) {
        for (int i = 20; i < 108; i += 4) {
            int h = random(5, 30); 
            u8g2.drawVLine(i, 60 - h, h);
        }
        u8g2.drawStr(30, 62, "ATAQUE ACTIVO");
    } else {
        u8g2.drawStr(0, 50, "ESTADO: LISTO");
        u8g2.drawFrame(20, 35, 88, 15);
    }
    u8g2.sendBuffer();

    if (isAttacking) {
        int freq = (jamChannel * 5) + 2;
        jam2.setChannel(freq);
        for(int i = 0; i < 20; i++) {
            jam2.startWrite(&noise_payload, sizeof(noise_payload), true);
        }
    }
}
