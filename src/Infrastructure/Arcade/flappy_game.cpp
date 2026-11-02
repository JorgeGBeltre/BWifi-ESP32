#include "Infrastructure/Arcade/flappy_game.h"
#include "Infrastructure/Input/encoder_input.h"
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// Física del pájaro (Ajustada para micro-saltos)
static float birdY = 32.0;
static float velocity = 0.0;
static float gravity = 0.15;
static float jumpImpulse = -1.8;

// Variables de los tubos y dificultad
static float pipeX = 128;
static float pipeSpeed = 1.8;
static int gapY = 32;
static int gapSize = 28;
static int score = 0;
static bool gameOver = false;

static unsigned long lastDifficultyBoost = 0;

void flappySetup() {
    birdY = 32.0;
    velocity = 0.0;
    pipeX = 128;
    pipeSpeed = 1.8;
    gapY = random(18, 42); 
    score = 0;
    gameOver = false;
    lastDifficultyBoost = millis();
    encClear();
}

void flappyLoop() {
    u8g2.clearBuffer();

    if (!gameOver) {
        // --- ENTRADA (Pulso o giro hacia arriba en el HW-040) ---
        if (encOkDown() || encUp()) {
            velocity = jumpImpulse; 
        }

        // --- FÍSICA ---
        velocity += gravity;
        birdY += velocity;

        // --- DIFICULTAD PROGRESIVA ---
        if (millis() - lastDifficultyBoost > 5000) {
            if (pipeSpeed < 4.5) pipeSpeed += 0.2;
            lastDifficultyBoost = millis();
        }

        // --- MOVIMIENTO DE TUBOS ---
        pipeX -= pipeSpeed; 
        if (pipeX < -15) {
            pipeX = 128;
            gapY = random(18, 42);
            score++;
        }

        // --- COLISIONES ---
        if (birdY < 0 || birdY > 63) gameOver = true;

        if (pipeX < 25 && pipeX > 10) {
            if (birdY < gapY - (gapSize/2) || birdY > gapY + (gapSize/2)) {
                gameOver = true;
            }
        }

        // --- DIBUJAR ---
        u8g2.drawBox(15, (int)birdY, 4, 4); 
        u8g2.drawBox((int)pipeX, 0, 12, gapY - (gapSize/2)); 
        u8g2.drawBox((int)pipeX, gapY + (gapSize/2), 12, 64); 

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.setCursor(2, 8);
        u8g2.print("SCORE: "); u8g2.print(score);
        u8g2.setCursor(100, 8);
        u8g2.print("v"); u8g2.print(pipeSpeed, 1);

    } else {
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(35, 30, "GAME OVER");
        u8g2.setCursor(45, 45);
        u8g2.print("SCORE: "); u8g2.print(score);
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(25, 60, "OK: REINTENTAR");

        if (encOk()) {
            flappySetup();
        }
    }
    u8g2.sendBuffer();
}
