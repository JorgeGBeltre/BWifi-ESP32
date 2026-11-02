#include "Infrastructure/Arcade/games_menu.h"
#include "Infrastructure/Arcade/dino_game.h"
#include "Infrastructure/Input/encoder_input.h"
#include "Infrastructure/Arcade/flappy_game.h"
#include "Infrastructure/Arcade/pong_game.h"
#include "Infrastructure/Arcade/snake_game.h"
#include "Infrastructure/Arcade/space_game.h"
#include <Preferences.h>
#include <U8g2lib.h>
#include <math.h>

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern bool runningApp;

extern void pongSetup();
extern void flappySetup();
extern void spaceSetup();
extern void dinoSetup();

static int game_selection = 0;
bool inGame = false;
static const int kTotalGames = 10;

static Preferences s_gamePrefs;
static int s_hiScores[kTotalGames] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static bool s_scoresLoaded = false;

static void loadHighScores() {
  if (s_scoresLoaded) return;
  s_gamePrefs.begin("arcade_hi", true);
  for (int i = 0; i < kTotalGames; i++) {
    char key[8];
    snprintf(key, sizeof(key), "hi%d", i);
    s_hiScores[i] = s_gamePrefs.getInt(key, 0);
  }
  s_gamePrefs.end();
  s_scoresLoaded = true;
}

static void updateHighScore(int idx, int score) {
  if (idx < 0 || idx >= kTotalGames) return;
  if (score > s_hiScores[idx]) {
    s_hiScores[idx] = score;
    s_gamePrefs.begin("arcade_hi", false);
    char key[8];
    snprintf(key, sizeof(key), "hi%d", idx);
    s_gamePrefs.putInt(key, score);
    s_gamePrefs.end();
  }
}

int getArcadeHighScore(int idx) {
  loadHighScores();
  if (idx < 0 || idx >= kTotalGames) return 0;
  return s_hiScores[idx];
}

// ============================================================================
// JUEGO 6: BREAKOUT
// ============================================================================
static float s_padX = 50.0f;
static float s_ballX = 64.0f, s_ballY = 48.0f;
static float s_ballVX = 1.9f, s_ballVY = -1.9f;
static bool s_bricks[3][8];
static int s_brkScore = 0;
static bool s_brkOver = false;

static void breakoutSetup() {
  s_padX = 50.0f;
  s_ballX = 64.0f;
  s_ballY = 48.0f;
  s_ballVX = 1.8f;
  s_ballVY = -1.8f;
  s_brkScore = 0;
  s_brkOver = false;
  for (int r = 0; r < 3; r++)
    for (int c = 0; c < 8; c++)
      s_bricks[r][c] = true;
  encClear();
}

static void breakoutLoop() {
  u8g2.clearBuffer();

  if (s_brkOver) {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(32, 26, "BREAKOUT END");
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(34, 40);
    u8g2.printf("SCORE: %d (HI:%d)", s_brkScore, s_hiScores[5]);
    u8g2.drawStr(24, 56, "[OK] Jugar de nuevo");
    u8g2.sendBuffer();
    if (encOk()) breakoutSetup();
    return;
  }

  int d = encStep();
  if (d != 0) {
    s_padX = constrain(s_padX + d * 8.0f, 2.0f, 102.0f);
  }

  s_ballX += s_ballVX;
  s_ballY += s_ballVY;

  if (s_ballX <= 2 || s_ballX >= 124) s_ballVX = -s_ballVX;
  if (s_ballY <= 10) s_ballVY = -s_ballVY;

  if (s_ballY >= 56 && s_ballY <= 60 && s_ballX >= s_padX - 2 && s_ballX <= s_padX + 26) {
    s_ballVY = -fabsf(s_ballVY);
    float hitOffset = (s_ballX - (s_padX + 12.0f)) / 12.0f;
    s_ballVX = constrain(hitOffset * 2.5f, -2.6f, 2.6f);
  }

  bool anyLeft = false;
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 8; c++) {
      if (!s_bricks[r][c]) continue;
      anyLeft = true;
      int bx = 4 + c * 15;
      int by = 13 + r * 7;
      if (s_ballX >= bx - 1 && s_ballX <= bx + 13 && s_ballY >= by - 1 && s_ballY <= by + 5) {
        s_bricks[r][c] = false;
        s_ballVY = -s_ballVY;
        s_brkScore += 10;
        updateHighScore(5, s_brkScore);
      }
    }
  }

  if (!anyLeft) {
    for (int r = 0; r < 3; r++)
      for (int c = 0; c < 8; c++)
        s_bricks[r][c] = true;
    s_ballY = 45.0f;
    s_ballVY = -fabsf(s_ballVY) * 1.1f;
  }

  if (s_ballY > 63) {
    s_brkOver = true;
  }

  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(2, 7);
  u8g2.printf("SC:%d HI:%d", s_brkScore, s_hiScores[5]);
  u8g2.drawHLine(0, 8, 128);

  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 8; c++) {
      if (s_bricks[r][c]) {
        u8g2.drawBox(4 + c * 15, 13 + r * 7, 13, 5);
      }
    }
  }

  u8g2.drawBox((int)s_padX, 58, 24, 3);
  u8g2.drawDisc((int)s_ballX, (int)s_ballY, 2);
  u8g2.sendBuffer();
}

// ============================================================================
// JUEGO 7: SIMON MEMORIA
// ============================================================================
static uint8_t s_simonSeq[32];
static int s_simonLen = 1;
static int s_simonStep = 0;
static int s_simonCursor = 0;
static bool s_simonShowing = true;
static int s_simonShowIdx = 0;
static uint32_t s_simonTimer = 0;
static bool s_simonOver = false;

static void simonSetup() {
  for (int i = 0; i < 32; i++) s_simonSeq[i] = (uint8_t)random(0, 4);
  s_simonLen = 1;
  s_simonStep = 0;
  s_simonCursor = 0;
  s_simonShowing = true;
  s_simonShowIdx = 0;
  s_simonTimer = millis();
  s_simonOver = false;
  encClear();
}

static void simonLoop() {
  u8g2.clearBuffer();

  if (s_simonOver) {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(26, 26, "SIMON FALLASTE");
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(28, 40);
    u8g2.printf("NIVEL: %d (HI:%d)", s_simonLen - 1, s_hiScores[6]);
    u8g2.drawStr(24, 56, "[OK] Reintentar");
    u8g2.sendBuffer();
    if (encOk()) simonSetup();
    return;
  }

  int activeLit = -1;
  if (s_simonShowing) {
    uint32_t elapsed = millis() - s_simonTimer;
    if (elapsed < 420) {
      activeLit = s_simonSeq[s_simonShowIdx];
    } else if (elapsed > 600) {
      s_simonShowIdx++;
      s_simonTimer = millis();
      if (s_simonShowIdx >= s_simonLen) {
        s_simonShowing = false;
        s_simonStep = 0;
        encClear();
      }
    }
  } else {
    int d = encStep();
    if (d != 0) {
      s_simonCursor = (s_simonCursor + d) % 4;
      if (s_simonCursor < 0) s_simonCursor += 4;
    }
    if (encOk()) {
      if (s_simonCursor == s_simonSeq[s_simonStep]) {
        s_simonStep++;
        if (s_simonStep >= s_simonLen) {
          updateHighScore(6, s_simonLen);
          if (s_simonLen < 31) s_simonLen++;
          s_simonShowing = true;
          s_simonShowIdx = 0;
          s_simonTimer = millis() + 250;
        }
      } else {
        s_simonOver = true;
      }
    }
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(4, 9);
  u8g2.printf("SIMON NV:%d HI:%d %s", s_simonLen, s_hiScores[6],
              s_simonShowing ? "[MIRA]" : "[TU TURNO]");
  u8g2.drawHLine(0, 11, 128);

  const int bx[4] = {14, 68, 14, 68};
  const int by[4] = {15, 15, 39, 39};
  const char *bLabels[4] = {"1:NW", "2:NE", "3:SW", "4:SE"};

  for (int i = 0; i < 4; i++) {
    bool lit = (activeLit == i);
    bool sel = (!s_simonShowing && s_simonCursor == i);
    if (lit) {
      u8g2.drawRBox(bx[i], by[i], 44, 20, 3);
      u8g2.setDrawColor(0);
      u8g2.drawStr(bx[i] + 10, by[i] + 13, bLabels[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawRFrame(bx[i], by[i], 44, 20, 3);
      if (sel) {
        u8g2.drawRFrame(bx[i] - 2, by[i] - 2, 48, 24, 4);
      }
      u8g2.drawStr(bx[i] + 10, by[i] + 13, bLabels[i]);
    }
  }
  u8g2.sendBuffer();
}

// ============================================================================
// JUEGO 8: TETRIS (Tablero 10x14 vertical, Giro = Mover X, OK = Rotar/Bajar)
// ============================================================================
static uint16_t s_tetGrid[14]; // 10 bits por fila
static int s_tetPiece = 0, s_tetRot = 0;
static int s_tetX = 3, s_tetY = 0;
static int s_tetScore = 0;
static bool s_tetOver = false;
static uint32_t s_tetLastFall = 0;

static const uint16_t kTetShapes[7][4] = {
    {0x0F00, 0x2222, 0x0F00, 0x2222}, // I
    {0x0660, 0x0660, 0x0660, 0x0660}, // O
    {0x0E40, 0x4C40, 0x4E00, 0x4640}, // T
    {0x06C0, 0x4620, 0x06C0, 0x4620}, // S
    {0x0C60, 0x2640, 0x0C60, 0x2640}, // Z
    {0x0E20, 0x44C0, 0x8E00, 0x6440}, // J
    {0x0E80, 0xC440, 0x2E00, 0x4460}  // L
};

static bool tetCell(int p, int rot, int r, int c) {
  uint16_t mask = kTetShapes[p % 7][rot & 3];
  int bit = 15 - (r * 4 + c);
  return (mask & (1U << bit)) != 0;
}

static bool tetCollide(int px, int py, int p, int rot) {
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (!tetCell(p, rot, r, c)) continue;
      int gx = px + c;
      int gy = py + r;
      if (gx < 0 || gx >= 10 || gy >= 14) return true;
      if (gy >= 0 && (s_tetGrid[gy] & (1U << gx))) return true;
    }
  }
  return false;
}

static void tetSpawn() {
  s_tetPiece = random(0, 7);
  s_tetRot = 0;
  s_tetX = 3;
  s_tetY = 0;
  if (tetCollide(s_tetX, s_tetY, s_tetPiece, s_tetRot)) {
    s_tetOver = true;
  }
}

static void tetrisSetup() {
  memset(s_tetGrid, 0, sizeof(s_tetGrid));
  s_tetScore = 0;
  s_tetOver = false;
  s_tetLastFall = millis();
  tetSpawn();
  encClear();
}

static void tetrisLoop() {
  u8g2.clearBuffer();
  if (s_tetOver) {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(28, 26, "TETRIS OVER");
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(28, 40);
    u8g2.printf("SCORE:%d (HI:%d)", s_tetScore, s_hiScores[7]);
    u8g2.drawStr(24, 56, "[OK] Reintentar");
    u8g2.sendBuffer();
    if (encOk()) tetrisSetup();
    return;
  }

  int d = encStep();
  if (d != 0) {
    int stepDir = (d > 0) ? 1 : -1;
    if (!tetCollide(s_tetX + stepDir, s_tetY, s_tetPiece, s_tetRot)) {
      s_tetX += stepDir;
    }
  }

  if (encOk()) {
    int nextRot = (s_tetRot + 1) & 3;
    if (!tetCollide(s_tetX, s_tetY, s_tetPiece, nextRot)) {
      s_tetRot = nextRot;
    } else if (!tetCollide(s_tetX, s_tetY + 1, s_tetPiece, s_tetRot)) {
      s_tetY++;
    }
  }

  if (millis() - s_tetLastFall > 420) {
    s_tetLastFall = millis();
    if (!tetCollide(s_tetX, s_tetY + 1, s_tetPiece, s_tetRot)) {
      s_tetY++;
    } else {
      for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
          if (tetCell(s_tetPiece, s_tetRot, r, c)) {
            int gy = s_tetY + r;
            int gx = s_tetX + c;
            if (gy >= 0 && gy < 14 && gx >= 0 && gx < 10) {
              s_tetGrid[gy] |= (1U << gx);
            }
          }
        }
      }
      // Limpiar líneas completas
      for (int y = 13; y >= 0; y--) {
        if (s_tetGrid[y] == 0x03FF) {
          for (int k = y; k > 0; k--) s_tetGrid[k] = s_tetGrid[k - 1];
          s_tetGrid[0] = 0;
          s_tetScore += 25;
          updateHighScore(7, s_tetScore);
          y++;
        }
      }
      tetSpawn();
    }
  }

  // Dibujar tablero 10x14 (cada celda 4x4 px -> 40x56 px centrado en x=12, y=4)
  u8g2.drawFrame(10, 3, 44, 58);
  for (int y = 0; y < 14; y++) {
    for (int x = 0; x < 10; x++) {
      if (s_tetGrid[y] & (1U << x)) {
        u8g2.drawBox(12 + x * 4, 4 + y * 4, 3, 3);
      }
    }
  }
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (tetCell(s_tetPiece, s_tetRot, r, c)) {
        int gx = s_tetX + c;
        int gy = s_tetY + r;
        if (gy >= 0) u8g2.drawBox(12 + gx * 4, 4 + gy * 4, 3, 3);
      }
    }
  }

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(60, 14, "TETRIS v3.5");
  u8g2.setCursor(60, 26);
  u8g2.printf("SC: %d", s_tetScore);
  u8g2.setCursor(60, 36);
  u8g2.printf("HI: %d", s_hiScores[7]);
  u8g2.drawStr(60, 48, "Gira:Mover");
  u8g2.drawStr(60, 58, "OK:  Rotar");
  u8g2.sendBuffer();
}

// ============================================================================
// JUEGO 9: ASTEROIDS (Giro = Rotar Nave 360°, OK = Disparar)
// ============================================================================
struct AstRock {
  float x, y, vx, vy;
  int r;
  bool active;
};
struct AstShot {
  float x, y, vx, vy;
  int life;
};

static float s_shipAngle = 0.0f;
static AstRock s_rocks[6];
static AstShot s_shots[4];
static int s_astScore = 0;
static bool s_astOver = false;

static void spawnRocks() {
  for (int i = 0; i < 6; i++) {
    s_rocks[i].x = (i % 2 == 0) ? random(4, 28) : random(100, 124);
    s_rocks[i].y = random(4, 60);
    float a = random(0, 360) * 0.0174533f;
    s_rocks[i].vx = cosf(a) * 0.85f;
    s_rocks[i].vy = sinf(a) * 0.85f;
    s_rocks[i].r = 6;
    s_rocks[i].active = true;
  }
}

static void asteroidsSetup() {
  s_shipAngle = -1.57f;
  s_astScore = 0;
  s_astOver = false;
  for (int i = 0; i < 4; i++) s_shots[i].life = 0;
  spawnRocks();
  encClear();
}

static void asteroidsLoop() {
  u8g2.clearBuffer();
  if (s_astOver) {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(24, 26, "ASTEROIDS END");
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(28, 40);
    u8g2.printf("SCORE:%d (HI:%d)", s_astScore, s_hiScores[8]);
    u8g2.drawStr(24, 56, "[OK] Reintentar");
    u8g2.sendBuffer();
    if (encOk()) asteroidsSetup();
    return;
  }

  int d = encStep();
  if (d != 0) {
    s_shipAngle += d * 0.32f;
  }

  if (encOkDown() || encOk()) {
    for (int i = 0; i < 4; i++) {
      if (s_shots[i].life <= 0) {
        s_shots[i].x = 64.0f + cosf(s_shipAngle) * 6.0f;
        s_shots[i].y = 32.0f + sinf(s_shipAngle) * 6.0f;
        s_shots[i].vx = cosf(s_shipAngle) * 3.2f;
        s_shots[i].vy = sinf(s_shipAngle) * 3.2f;
        s_shots[i].life = 26;
        break;
      }
    }
  }

  for (int i = 0; i < 4; i++) {
    if (s_shots[i].life > 0) {
      s_shots[i].x += s_shots[i].vx;
      s_shots[i].y += s_shots[i].vy;
      s_shots[i].life--;
      u8g2.drawDisc((int)s_shots[i].x, (int)s_shots[i].y, 1);
    }
  }

  bool anyRock = false;
  for (int i = 0; i < 6; i++) {
    if (!s_rocks[i].active) continue;
    anyRock = true;
    s_rocks[i].x += s_rocks[i].vx;
    s_rocks[i].y += s_rocks[i].vy;
    if (s_rocks[i].x < 0) s_rocks[i].x += 128;
    if (s_rocks[i].x >= 128) s_rocks[i].x -= 128;
    if (s_rocks[i].y < 0) s_rocks[i].y += 64;
    if (s_rocks[i].y >= 64) s_rocks[i].y -= 64;

    float dxS = s_rocks[i].x - 64.0f;
    float dyS = s_rocks[i].y - 32.0f;
    if ((dxS * dxS + dyS * dyS) < (float)((s_rocks[i].r + 3) * (s_rocks[i].r + 3))) {
      s_astOver = true;
    }

    for (int j = 0; j < 4; j++) {
      if (s_shots[j].life > 0) {
        float dx = s_rocks[i].x - s_shots[j].x;
        float dy = s_rocks[i].y - s_shots[j].y;
        if ((dx * dx + dy * dy) <= (float)((s_rocks[i].r + 2) * (s_rocks[i].r + 2))) {
          s_shots[j].life = 0;
          s_astScore += 15;
          updateHighScore(8, s_astScore);
          if (s_rocks[i].r > 3) {
            s_rocks[i].r = 3;
            s_rocks[i].vx = -s_rocks[i].vy * 1.3f;
            s_rocks[i].vy = s_rocks[i].vx * 1.3f;
          } else {
            s_rocks[i].active = false;
          }
          break;
        }
      }
    }

    if (s_rocks[i].active) {
      u8g2.drawCircle((int)s_rocks[i].x, (int)s_rocks[i].y, s_rocks[i].r);
    }
  }

  if (!anyRock) spawnRocks();

  // Dibujar nave vectorial central
  int nx = 64 + (int)(cosf(s_shipAngle) * 7.0f);
  int ny = 32 + (int)(sinf(s_shipAngle) * 7.0f);
  int lx = 64 + (int)(cosf(s_shipAngle + 2.5f) * 5.0f);
  int ly = 32 + (int)(sinf(s_shipAngle + 2.5f) * 5.0f);
  int rx = 64 + (int)(cosf(s_shipAngle - 2.5f) * 5.0f);
  int ry = 32 + (int)(sinf(s_shipAngle - 2.5f) * 5.0f);
  u8g2.drawTriangle(nx, ny, lx, ly, rx, ry);

  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(2, 6);
  u8g2.printf("SC:%d HI:%d", s_astScore, s_hiScores[8]);
  u8g2.sendBuffer();
}

// ============================================================================
// JUEGO 10: 2048 (Tablero 4x4, Giro elige dirección N/E/S/W, OK desliza)
// ============================================================================
static uint16_t s_grid2048[4][4];
static int s_dir2048 = 0; // 0: UP, 1: RIGHT, 2: DOWN, 3: LEFT
static int s_score2048 = 0;

static void addRandomTile2048() {
  int emptyR[16], emptyC[16], cnt = 0;
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      if (s_grid2048[r][c] == 0) {
        emptyR[cnt] = r;
        emptyC[cnt] = c;
        cnt++;
      }
    }
  }
  if (cnt > 0) {
    int pick = random(0, cnt);
    s_grid2048[emptyR[pick]][emptyC[pick]] = (random(0, 10) == 0) ? 4 : 2;
  }
}

static void game2048Setup() {
  memset(s_grid2048, 0, sizeof(s_grid2048));
  s_score2048 = 0;
  s_dir2048 = 0;
  addRandomTile2048();
  addRandomTile2048();
  encClear();
}

static bool slideLine2048(uint16_t line[4]) {
  uint16_t tmp[4] = {0, 0, 0, 0};
  int pos = 0;
  for (int i = 0; i < 4; i++) {
    if (line[i] != 0) tmp[pos++] = line[i];
  }
  for (int i = 0; i < 3; i++) {
    if (tmp[i] != 0 && tmp[i] == tmp[i + 1]) {
      tmp[i] *= 2;
      s_score2048 += tmp[i];
      updateHighScore(9, s_score2048);
      for (int j = i + 1; j < 3; j++) tmp[j] = tmp[j + 1];
      tmp[3] = 0;
    }
  }
  bool changed = false;
  for (int i = 0; i < 4; i++) {
    if (line[i] != tmp[i]) changed = true;
    line[i] = tmp[i];
  }
  return changed;
}

static void moveBoard2048(int dir) {
  bool moved = false;
  uint16_t line[4];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (dir == 0) line[j] = s_grid2048[j][i];
      else if (dir == 1) line[j] = s_grid2048[i][3 - j];
      else if (dir == 2) line[j] = s_grid2048[3 - j][i];
      else line[j] = s_grid2048[i][j];
    }
    if (slideLine2048(line)) moved = true;
    for (int j = 0; j < 4; j++) {
      if (dir == 0) s_grid2048[j][i] = line[j];
      else if (dir == 1) s_grid2048[i][3 - j] = line[j];
      else if (dir == 2) s_grid2048[3 - j][i] = line[j];
      else s_grid2048[i][j] = line[j];
    }
  }
  if (moved) addRandomTile2048();
}

static void game2048Loop() {
  int d = encStep();
  if (d != 0) {
    s_dir2048 = (s_dir2048 + d) % 4;
    if (s_dir2048 < 0) s_dir2048 += 4;
  }
  if (encOk()) {
    moveBoard2048(s_dir2048);
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr);
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      int x = 2 + c * 21;
      int y = 2 + r * 15;
      u8g2.drawFrame(x, y, 20, 14);
      if (s_grid2048[r][c] > 0) {
        u8g2.setCursor(x + 2, y + 10);
        u8g2.print(s_grid2048[r][c]);
      }
    }
  }

  const char *dirNames[4] = {"ARRIBA ^", "DERECHA >", "ABAJO v", "IZQ <"};
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(88, 12, "2048");
  u8g2.setCursor(88, 23);
  u8g2.printf("SC:%d", s_score2048);
  u8g2.setCursor(88, 33);
  u8g2.printf("HI:%d", s_hiScores[9]);
  u8g2.drawStr(88, 46, dirNames[s_dir2048]);
  u8g2.drawStr(88, 58, "[OK]MOV");
  u8g2.sendBuffer();
}

void drawGameMenu() {
  loadHighScores();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawRBox(0, 0, 128, 12, 2);
  u8g2.setDrawColor(0);
  u8g2.drawStr(14, 9, "ARCADE MODE (10)");
  u8g2.setDrawColor(1);

  const char *games[kTotalGames] = {
      "1. SNAKE",
      "2. PONG",
      "3. FLAPPY",
      "4. INVADERS",
      "5. DINO RUN",
      "6. BREAKOUT",
      "7. SIMON MEMORIA",
      "8. TETRIS",
      "9. ASTEROIDS",
      "10. 2048 PUZZLE"};

  int firstVis = game_selection - 2;
  if (firstVis > kTotalGames - 4) firstVis = kTotalGames - 4;
  if (firstVis < 0) firstVis = 0;

  u8g2.setFont(u8g2_font_5x7_tr);
  for (int row = 0; row < 4; row++) {
    int i = firstVis + row;
    if (i >= kTotalGames) break;
    int y = 23 + (row * 10);
    if (i == game_selection) {
      u8g2.drawRBox(6, y - 7, 116, 9, 2);
      u8g2.setDrawColor(0);
      u8g2.drawStr(12, y, games[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(12, y, games[i]);
    }
  }

  u8g2.drawHLine(0, 54, 128);
  u8g2.setCursor(6, 62);
  u8g2.printf("Record #%d: %d pts", game_selection + 1, s_hiScores[game_selection]);
  u8g2.sendBuffer();
}

void gamesLoop() {
  if (!inGame) {
    int d = encStep();
    if (d != 0) {
      game_selection = (game_selection + d) % kTotalGames;
      if (game_selection < 0)
        game_selection += kTotalGames;
    }

    drawGameMenu();

    if (encOk()) {
      inGame = true;
      encClear();
      if (game_selection == 0)
        resetSnake();
      else if (game_selection == 1)
        pongSetup();
      else if (game_selection == 2)
        flappySetup();
      else if (game_selection == 3)
        spaceSetup();
      else if (game_selection == 4)
        dinoSetup();
      else if (game_selection == 5)
        breakoutSetup();
      else if (game_selection == 6)
        simonSetup();
      else if (game_selection == 7)
        tetrisSetup();
      else if (game_selection == 8)
        asteroidsSetup();
      else if (game_selection == 9)
        game2048Setup();
    }

    if (encBack()) {
      runningApp = false;
      encClear();
    }
  } else {
    if (game_selection == 0)
      snakeLoop();
    else if (game_selection == 1)
      pongLoop();
    else if (game_selection == 2)
      flappyLoop();
    else if (game_selection == 3)
      spaceLoop();
    else if (game_selection == 4)
      dinoLoop();
    else if (game_selection == 5)
      breakoutLoop();
    else if (game_selection == 6)
      simonLoop();
    else if (game_selection == 7)
      tetrisLoop();
    else if (game_selection == 8)
      asteroidsLoop();
    else if (game_selection == 9)
      game2048Loop();

    if (encBack()) {
      inGame = false;
      encClear();
    }
  }
}
