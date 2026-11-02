#include "Infrastructure/Arcade/pong_game.h"
#include "Infrastructure/Input/encoder_input.h"
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// Variables de posición y física
static float ballX = 64, ballY = 32;
static float ballDX = 2.0, ballDY = 1.0;
static float speedMultiplier = 1.0;
static int paddleY = 24;
static int cpuY = 24;
static int scorePlayer = 0;
static int scoreCPU = 0;

void pongSetup() {
    ballX = 64; ballY = 32;
    ballDX = 2.0; ballDY = 1.0;
    speedMultiplier = 1.0;
    scorePlayer = 0; scoreCPU = 0;
    encClear();
}

void pongLoop() {
    u8g2.clearBuffer();

    // Dibujar Red central
    for(int i=0; i<64; i+=4) u8g2.drawVLine(64, i, 2);

    // --- LÓGICA JUGADOR CON ROTARY ENCODER HW-040 ---
    int step = encStep();
    if (step != 0) {
        paddleY = constrain(paddleY + (step * 5), 0, 48);
    }

    // --- LÓGICA IA CPU ---
    if (ballX > 50) { 
        if (ballY > cpuY + 8 && cpuY < 48) cpuY += 2;
        if (ballY < cpuY + 8 && cpuY > 0) cpuY -= 2;
    }

    // --- FÍSICA Y ACELERACIÓN ---
    ballX += ballDX * speedMultiplier;
    ballY += ballDY * speedMultiplier;

    if (speedMultiplier < 3.5) {
        speedMultiplier += 0.0005; 
    }

    if (ballY <= 0 || ballY >= 63) {
        ballDY = -ballDY;
        speedMultiplier += 0.02; 
    }

    // --- COLISIONES CON PALETAS ---
    if (ballX <= 10 && ballY >= paddleY && ballY <= paddleY + 16) {
        ballDX = -ballDX;
        ballX = 11;
        speedMultiplier += 0.05;
    }

    if (ballX >= 118 && ballY >= cpuY && ballY <= cpuY + 16) {
        ballDX = -ballDX;
        ballX = 117;
        speedMultiplier += 0.05;
    }

    // --- SISTEMA DE PUNTUACIÓN ---
    if (ballX < 0) { 
        scoreCPU++; 
        ballX = 64; ballY = 32;
        ballDX = 2.0; 
        speedMultiplier = 1.0;
    }
    if (ballX > 127) { 
        scorePlayer++; 
        ballX = 64; ballY = 32;
        ballDX = -2.0; 
        speedMultiplier = 1.0;
    }

    // --- DIBUJAR ELEMENTOS ---
    u8g2.drawFrame(4, paddleY, 4, 16);
    u8g2.drawFrame(120, cpuY, 4, 16);
    u8g2.drawBox((int)ballX, (int)ballY, 3, 3);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(50, 10); u8g2.print(scorePlayer);
    u8g2.setCursor(75, 10); u8g2.print(scoreCPU);

    u8g2.setCursor(105, 10); u8g2.print("x"); u8g2.print(speedMultiplier, 1);

    u8g2.sendBuffer();
}
