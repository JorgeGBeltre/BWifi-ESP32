#include "Infrastructure/Arcade/space_game.h"
#include "Infrastructure/Input/encoder_input.h"
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// Estructura básica para naves y balas
struct Entity { float x, y; bool active; };

static Entity player;
static Entity bullet;
static Entity enemies[12]; 
static int enemyDir = 1;
static float enemySpeed = 0.4;
static int score = 0;
static bool gameOver = false;

void spaceSetup() {
    player = {60.0, 55.0, true};
    bullet = {0, 0, false};
    score = 0;
    enemySpeed = 0.4;
    enemyDir = 1;
    gameOver = false;

    for(int i=0; i<12; i++) {
        enemies[i] = { (float)(20 + (i%4)*25), (float)(10 + (i/4)*12), true };
    }
    encClear();
}

void spaceLoop() {
    u8g2.clearBuffer();

    if (!gameOver) {
        // --- CONTROLES CON ENCODER HW-040 (GIRO MUEVE IZQ/DER, SW DISPARA) ---
        int step = encStep();
        if (step != 0) {
            player.x = constrain(player.x + (step * 6.0f), 5.0f, 115.0f);
        }
        
        if ((encOkDown() || encOk()) && !bullet.active) {
            bullet = {player.x + 3, player.y - 4, true};
        }

        // --- MOVIMIENTO BALA ---
        if (bullet.active) {
            bullet.y -= 5;
            if (bullet.y < 0) bullet.active = false;
        }

        // --- MOVIMIENTO ENEMIGOS (IA DE HORDA) ---
        bool touchEdge = false;
        for(int i=0; i<12; i++) {
            if (!enemies[i].active) continue;
            
            enemies[i].x += enemyDir * enemySpeed;
            
            if (enemies[i].x > 120 || enemies[i].x < 5) touchEdge = true;
            
            if (enemies[i].y >= player.y - 2) gameOver = true;
        }

        if (touchEdge) {
            enemyDir *= -1;
            for(int i=0; i<12; i++) {
                if(enemies[i].active) enemies[i].y += 5;
            }
            if (enemySpeed < 2.5) enemySpeed += 0.08; 
        }

        // --- COLISIONES BALA-ENEMIGO ---
        if (bullet.active) {
            for(int i=0; i<12; i++) {
                if (enemies[i].active && bullet.x >= enemies[i].x && bullet.x <= enemies[i].x + 6 &&
                    bullet.y >= enemies[i].y && bullet.y <= enemies[i].y + 6) {
                    enemies[i].active = false;
                    bullet.active = false;
                    score += 10;
                }
            }
        }

        // --- VERIFICAR VICTORIA (RESPAWN) ---
        bool allDead = true;
        for(int i=0; i<12; i++) if(enemies[i].active) allDead = false;
        if (allDead) spaceSetup(); 

        // --- DIBUJAR ELEMENTOS ---
        u8g2.drawTriangle(player.x, player.y+6, player.x+6, player.y+6, player.x+3, player.y); 
        if (bullet.active) u8g2.drawVLine(bullet.x, bullet.y, 4); 

        for(int i=0; i<12; i++) {
            if (enemies[i].active) {
                u8g2.drawFrame(enemies[i].x, enemies[i].y, 6, 5);
                u8g2.drawPixel(enemies[i].x, enemies[i].y);
                u8g2.drawPixel(enemies[i].x+5, enemies[i].y);
                u8g2.drawPixel(enemies[i].x+2, enemies[i].y+6);
                u8g2.drawPixel(enemies[i].x+3, enemies[i].y+6);
            }
        }

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.setCursor(2, 8); u8g2.print("SCORE: "); u8g2.print(score);

    } else {
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(30, 30, "SYSTEM BREACHED");
        u8g2.setCursor(45, 45); u8g2.print("SCORE: "); u8g2.print(score);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(25, 60, "OK: REINTENTAR");
        if (encOk()) { spaceSetup(); }
    }
    u8g2.sendBuffer();
}
