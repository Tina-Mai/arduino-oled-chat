// DODGEFALL — ultra-simple joystick game for UNO R4 WiFi + SSD1306 OLED
// Wiring: OLED on I2C (0x3C), Joystick VRx->A0, VRy->A1 (unused), SW->D2
// Move left/right to dodge falling blocks. Press to restart.

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Pins ---
const int VRX_PIN = A0;   // left/right
const int VRY_PIN = A1;   // (unused in this game)
const int SW_PIN  = 2;    // push (active-LOW)

// --- Player ---
int px;                      // player x (mapped from joystick)
const int py = SCREEN_HEIGHT - 8;
const int PW = 12;           // player width
const int PH = 4;            // player height

// --- Obstacles ---
struct Rock {
  int x, y;      // top-left
  uint8_t w, h;  // size
  uint8_t sp;    // speed (pixels per frame tick)
};
const uint8_t NROCK = 5;
Rock rocks[NROCK];

// --- Game state ---
enum GState { MENU, PLAY, DEAD };
GState state = MENU;

unsigned long lastFrame = 0;
const uint16_t FRAME_MS = 25;  // ~40 FPS

unsigned long lastTick = 0;
const uint16_t TICK_MS = 40;   // movement / difficulty tick

uint32_t score = 0;
uint16_t survivalMs = 0;       // for score

// input handling
bool lastBtn = false;

// helpers
int clampInt(int v, int lo, int hi) { if (v < lo) return lo; if (v > hi) return hi; return v; }
bool btnPressed() { return digitalRead(SW_PIN) == LOW; }
bool btnPressedEdge() {
  bool b = btnPressed();
  bool edge = (b && !lastBtn);
  lastBtn = b;
  return edge;
}

// simple PRNG seed (don’t use A4/A5: they’re I2C)
void seedRng() {
  randomSeed( (uint32_t)micros() ^ (uint32_t)analogRead(A2) ^ (uint32_t)analogRead(A3) );
}

void spawnRock(Rock &r, uint8_t minSp, uint8_t maxSp) {
  r.w = random(6, 16);
  r.h = random(4, 10);
  r.x = random(0, SCREEN_WIDTH - r.w);
  r.y = - (int)random(8, 30);            // start just above screen
  r.sp = random(minSp, maxSp + 1);
}

void resetGame() {
  score = 0;
  survivalMs = 0;
  // initial slower speeds; scale up during play
  for (uint8_t i = 0; i < NROCK; ++i) spawnRock(rocks[i], 1, 2);
  state = PLAY;
}

bool rectOverlap(int ax,int ay,int aw,int ah,int bx,int by,int bw,int bh) {
  return !(ax+aw <= by /* oops swapped? */);
}

// proper rect overlap
bool collide(int ax,int ay,int aw,int ah,int bx,int by,int bw,int bh) {
  bool sep = (ax + aw <= bx) || (bx + bw <= ax) || (ay + ah <= by) || (by + bh <= ay);
  return !sep;
}

int mapAxisX() {
  int v = analogRead(VRX_PIN); // 0..1023
  const int DEAD = 40;         // deadzone around center
  if (abs(v - 512) < DEAD) v = 512;
  // map 0..1023 -> 0..(SCREEN_WIDTH - PW)
  int m = map(clampInt(v, 0, 1023), 0, 1023, 0, SCREEN_WIDTH - PW);
  return m;
}

void drawHUD() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 2);
  display.print("Score: ");
  display.print(score);
}

void drawPlayer() {
  // a small “ship”: base + nose
  display.fillRect(px, py, PW, PH, SSD1306_WHITE);
  display.drawFastVLine(px + PW/2, py - 3, 3, SSD1306_WHITE); // tiny nose
}

void drawRocks() {
  for (uint8_t i = 0; i < NROCK; ++i) {
    display.fillRect(rocks[i].x, rocks[i].y, rocks[i].w, rocks[i].h, SSD1306_WHITE);
  }
}

void updateRocks(uint8_t speedBoost) {
  for (uint8_t i = 0; i < NROCK; ++i) {
    rocks[i].y += rocks[i].sp + (speedBoost > 0 ? 1 : 0);
    if (rocks[i].y > SCREEN_HEIGHT) {
      // passed screen — respawn and add points
      score += 10;
      // gradually increase difficulty by nudging speeds upward
      uint8_t minSp = 1 + (score / 80);       // every ~8 rocks -> +1
      uint8_t maxSp = 2 + (score / 60);
      if (minSp > 5) minSp = 5;
      if (maxSp > 7) maxSp = 7;
      spawnRock(rocks[i], minSp, maxSp);
    }
  }
}

bool checkHit() {
  for (uint8_t i = 0; i < NROCK; ++i) {
    if (collide(px, py, PW, PH, rocks[i].x, rocks[i].y, rocks[i].w, rocks[i].h)) return true;
  }
  return false;
}

void drawMenu() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 14);
  display.println(F("DODGEFALL"));
  display.setTextSize(1);
  display.setCursor(18, 36);
  display.println(F("Tilt to move, avoid blocks"));
  display.setCursor(22, 48);
  display.println(F("Press stick to start"));
  display.display();
}

void drawGame() {
  display.clearDisplay();
  drawHUD();
  drawRocks();
  drawPlayer();
  display.display();
}

void drawGameOver() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(22, 16);
  display.println(F("Game Over"));
  display.setTextSize(1);
  display.setCursor(18, 40);
  display.print(F("Score: "));
  display.println(score);
  display.setCursor(10, 52);
  display.println(F("Press stick to retry"));
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(SW_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) { delay(1000); }
  }
  display.clearDisplay();
  display.display();

  seedRng();
  px = (SCREEN_WIDTH - PW) / 2;
  drawMenu();
}

void loop() {
  unsigned long now = millis();

  // Fixed frame rate
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  switch (state) {
    case MENU:
      if (btnPressedEdge()) { resetGame(); }
      // show a tiny blinking cursor hint
      px = mapAxisX();
      display.clearDisplay();
      display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(10, 14);
      display.println(F("DODGEFALL"));
      display.setTextSize(1);
      display.setCursor(16, 36);
      display.println(F("Press stick to start"));
      // draw player preview
      display.fillRect(px, py, PW, PH, SSD1306_WHITE);
      display.display();
      break;

    case PLAY: {
      // input
      px = mapAxisX();

      // timed movement / difficulty & score
      if (now - lastTick >= TICK_MS) {
        lastTick = now;
        survivalMs += TICK_MS;
        if (survivalMs >= 200) { // every 0.2s, +1 score
          survivalMs = 0;
          score += 1;
        }
        // subtle speed boost over time
        uint8_t boost = (score % 50 == 0 && score > 0) ? 1 : 0;
        updateRocks(boost);
      }

      // collision
      if (checkHit()) {
        state = DEAD;
        drawGameOver();
        break;
      }

      // draw
      drawGame();
    } break;

    case DEAD:
      if (btnPressedEdge()) { resetGame(); }
      // keep showing game-over screen
      break;
  }
}
