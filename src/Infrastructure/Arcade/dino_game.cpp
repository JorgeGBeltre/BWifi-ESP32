#include "Infrastructure/Arcade/dino_game.h"
#include "Infrastructure/Input/encoder_input.h"
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

// Física y Animación
static float dinoY = 48.0;
static float dinoVelocity = 0.0;
static float currentGravity = 0.5;
static const float dinoJump = -6.5;
static bool isJumping = false;
static bool isDucking = false;
static int animFrame = 0;

// Obstáculos
static float obstX = 128.0;
static float obstY = 42.0;    
static int obstType = 0;      
static float gameSpeed = 3.2; 
static int score = 0;
static bool gameOver = false;

void dinoSetup() {
    dinoY = 48.0;
    dinoVelocity = 0.0;
    currentGravity = 0.5;
    isJumping = false;
    isDucking = false;
    obstX = 128.0;
    obstY = 42.0;
    obstType = 0;
    gameSpeed = 3.2;
    score = 0;
    gameOver = false;
    encClear();
}

void drawDino(int x, int y, bool ducking, int frame) {
    if (!ducking) {
        u8g2.drawBox(x, y, 8, 8);         
        u8g2.drawBox(x + 4, y - 4, 6, 4); 
        u8g2.drawPixel(x + 8, y - 3);     
        if (frame == 0) { u8g2.drawVLine(x + 1, y + 8, 2); u8g2.drawVLine(x + 5, y + 8, 2); }
        else { u8g2.drawVLine(x + 2, y + 8, 2); u8g2.drawVLine(x + 6, y + 8, 2); }
    } else {
        u8g2.drawBox(x, y + 4, 12, 4);    
        u8g2.drawBox(x + 8, y + 2, 6, 3); 
        u8g2.drawPixel(x + 12, y + 3);    
    }
}

void drawRealCactus(int x, int y) {
    u8g2.drawBox(x, y, 4, 16);
    u8g2.drawBox(x - 3, y + 5, 2, 6);
    u8g2.drawHLine(x - 2, y + 9, 2);
    u8g2.drawBox(x + 5, y + 2, 2, 6);
    u8g2.drawHLine(x + 4, y + 6, 1);
}

void dinoLoop() {
    u8g2.clearBuffer();

    if (!gameOver) {
        // --- ENTRADA CON ENCODER HW-040 ---
        // Saltar con clic en SW o giro CCW (UP)
        if ((encOkDown() || encUp()) && !isJumping && !isDucking) {
            dinoVelocity = dinoJump;
            isJumping = true;
        }

        // Agachado / Fast-Fall al girar CW (DOWN)
        bool downPressed = encDownHeld(220);
        float effectiveGravity = currentGravity;

        if (downPressed) {
            if (isJumping) {
                effectiveGravity = currentGravity * 2.0;
                isDucking = false;
            } else {
                isDucking = true;
                isJumping = false;
            }
        } else {
            if (!isJumping) {
                isDucking = false;
            }
        }

        // --- FÍSICA APLICADA ---
        if (!isDucking) {
            dinoVelocity += effectiveGravity;
            dinoY += dinoVelocity;
        }

        if (dinoY >= 48.0) { 
            dinoY = 48.0; 
            dinoVelocity = 0; 
            isJumping = false; 
        }

        // --- MOVIMIENTO Y DIFICULTAD ---
        obstX -= gameSpeed;
        if (obstX < -20) {
            obstX = 128;
            score++;
            
            if (gameSpeed < 9.5) {
                gameSpeed += 0.18;
                if (currentGravity < 0.85) {
                    currentGravity += 0.02; 
                }
            }
            
            if (score > 8 && random(0, 10) > 7) {
                obstType = 1; 
                obstY = 32.0; 
            } else {
                obstType = 0; 
                obstY = 42.0;
            }
        }

        if ((millis() / 120) % 2 == 0) animFrame = 0; else animFrame = 1;

        // --- COLISIONES ---
        if (obstType == 0) {
            if (obstX < 25 && obstX > 8) {
                if (dinoY > 38) gameOver = true;
            }
        } else {
            if (obstX < 23 && obstX > 10) {
                if (!isDucking) gameOver = true; 
            }
        }

        // --- DIBUJAR ---
        u8g2.drawHLine(0, 58, 128);
        drawDino(15, (int)dinoY, isDucking, animFrame);
        
        if (obstType == 0) {
            drawRealCactus((int)obstX, (int)obstY);
        } else {
            u8g2.drawBox((int)obstX, (int)obstY, 10, 4); 
            if (animFrame == 0) u8g2.drawTriangle(obstX+2, obstY, obstX+8, obstY, obstX+5, obstY-5); 
            else u8g2.drawTriangle(obstX+2, obstY+4, obstX+8, obstY+4, obstX+5, obstY+9);
        }

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.setCursor(2, 8); u8g2.print("SCORE: "); u8g2.print(score);
        u8g2.setCursor(95, 8); u8g2.print("v:"); u8g2.print(gameSpeed, 1);

    } else {
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(35, 30, "EXTINCTION");
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(25, 60, "OK: REINTENTAR");
        if (encOk()) { dinoSetup(); }
    }
    u8g2.sendBuffer();
}
