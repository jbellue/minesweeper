#include <Arduboy.h>
#include "bitmaps.h"
#include "digits.h"

#define TILE_SIZE 7
#define ROWS 9
#define COLUMNS 15

#define STATE_MENU 0
#define STATE_HELP 1
#define STATE_PLAY 2
#define STATE_LOSE 3
#define STATE_WIN  4

Arduboy arduboy;
byte selectedX = 0;
byte selectedY = 0;
byte tiles[COLUMNS][ROWS];
bool opened[COLUMNS][ROWS];
bool flagged[COLUMNS][ROWS];

bool soundEnabled = true;
bool fastMode = true;
byte buttons = 0;
byte state;
byte totalMines;

unsigned long startTime;
unsigned long currentTime;

const unsigned char* digits[] = {
  digit_1, digit_2, digit_3, digit_4,
  digit_5, digit_6, digit_7, digit_8
};

bool getButtonDown(byte button) {
  if (arduboy.pressed(button)) {
    if (buttons & button) return false;
    else buttons |= button;
    return true;
  }
  else  {
    if (buttons & button) buttons ^= button;
    return false;
  }
}

void reset() {
  selectedX = 0;
  selectedY = 0;
  memset(tiles, 0, sizeof(tiles[0][0]) * ROWS * COLUMNS);
  memset(opened, 0, sizeof(opened[0][0]) * ROWS * COLUMNS);
  memset(flagged, 0, sizeof(flagged[0][0]) * ROWS * COLUMNS);
  arduboy.clear();
  state = STATE_MENU;
}

void setup() {
  //dbg
  arduboy.beginNoLogo();
  //dbg
  //  arduboy.begin();
  arduboy.setFrameRate(10);
  arduboy.initRandomSeed();
  reset();
}

void drawGrid() {
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if (opened[x][y]) {
        if (tiles[x][y] != 0) {
          arduboy.drawBitmap(x * TILE_SIZE + 2, y * TILE_SIZE + 1, digits[tiles[x][y] - 1], 3, 5, WHITE);
        }
      } else {
        arduboy.fillRect(x * TILE_SIZE + 2, y * TILE_SIZE + 2 , 4, 4, WHITE);
      }
      if (flagged[x][y]) {
        arduboy.fillRect(x * TILE_SIZE + 3, y * TILE_SIZE + 3, 2, 2, BLACK);
      }
    }
  }
  arduboy.drawRect(0, 0, WIDTH - 22, HEIGHT, WHITE);
  for (byte w = TILE_SIZE; w <= WIDTH - 22; w += TILE_SIZE) {
    arduboy.drawFastVLine(w, 0, HEIGHT, WHITE);
  }
  for (byte h = TILE_SIZE; h < HEIGHT; h += TILE_SIZE) {
    arduboy.drawFastHLine(0, h, WIDTH - 22, WHITE);
  }
}

void drawMines() {
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if (tiles[x][y] == 9) {
        arduboy.drawBitmap(x * TILE_SIZE + 2, y * TILE_SIZE + 2, mineTile, 4, 4, BLACK);
      }
    }
  }
}

void getNumberOfSurroundingMines() {
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if (tiles[x][y] != 9) {
        for (int dx = x - 1; dx <= x + 1; dx++) {
          for (int dy = y - 1; dy <= y + 1; dy++) {
            if (dx >= 0 && dx < COLUMNS &&
                dy >= 0 && dy < ROWS    &&
                !(dx == x && dy == y)   &&
                tiles[dx][dy] == 9) {
              tiles[x][y]++;
            }
          }
        }
      }
    }
  }
}

void drawCursor() {
  arduboy.drawRect(selectedX * TILE_SIZE + 1, selectedY * TILE_SIZE + 1, TILE_SIZE - 1 , TILE_SIZE - 1, WHITE);
}

void setMines() {
  byte randX = 0;
  byte randY = 0;
  byte numberOfMines = totalMines;
  while (numberOfMines > 0)
  {
    randX = random(0, COLUMNS);
    randY = random(0, ROWS);
    if (tiles[randX][randY] == 0) {
      tiles[randX][randY] = 9;
      numberOfMines--;
    }
  }
}

void propagate(byte x, byte y) {
  if (flagged[x][y] || opened[x][y]) {
    return;
  }

  if (soundEnabled) arduboy.tunes.tone(587, 20);
  opened[x][y] = true;

  if (tiles[x][y] != 0) {
    return;
  }

  for (int dx = x - 1; dx <= x + 1; dx++) {
    for (int dy = y - 1; dy <= y + 1; dy++) {
      if (dx >= 0 && dx < COLUMNS &&
          dy >= 0 && dy < ROWS    &&
          !(dx == x && dy == y)) {
        propagate(dx, dy);
      }
    }
  }
}

void clickTile(byte x, byte y) {
  if (tiles[x][y] != 9) {
    flagged[x][y] = false;
    propagate(x, y);
  } else {
    //lost
    state = STATE_LOSE;
    if (soundEnabled) {
      arduboy.tunes.tone(587, 40);
      delay(160);
      arduboy.tunes.tone(392, 40);
    }
  }
}

int minesLeft() {
  if (state == STATE_WIN) {
    return 0;
  }
  int ret = totalMines;
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if (flagged[x][y]) {
        ret--;
      }
    }
  }
  return ret;
}

void drawExtras() {
  arduboy.setCursor(110, 1);
  arduboy.print(minesLeft());
  arduboy.setCursor(110, 56);
  arduboy.print((currentTime < 999) ? currentTime : 999);
}

void drawGame() {
  drawGrid();
  drawCursor();
  drawExtras();
}

void helpAndSound() {
  arduboy.setTextSize(1);

  arduboy.setCursor(3, 2);
  arduboy.print(F("sounds"));
  arduboy.setCursor(44, 2);
  arduboy.print(F("mute"));

  //drawing the arduboy
  arduboy.drawBitmap(76, 0, arduboyBMP, 52, 64, WHITE);

  arduboy.setCursor(8, 21);
  arduboy.print(F("toggle flag"));

  arduboy.setCursor(8, 36);
  arduboy.print(F("move cursor"));

  arduboy.setCursor(1, 51);
  arduboy.print(F("click a tile"));

  // sound "menu"
  if (soundEnabled) {
    arduboy.drawRoundRect(0, 0, 41, 11, 1, WHITE);
    arduboy.drawBitmap(101, 8, sound, 14, 12, WHITE);
  } else {
    arduboy.drawRoundRect(40, 0, 30, 11, 1, WHITE);
    arduboy.drawBitmap(103, 8, noSound, 12, 12, WHITE);
  }

  if (getButtonDown(RIGHT_BUTTON) || getButtonDown(LEFT_BUTTON)) {
    soundEnabled = !soundEnabled;
    if (soundEnabled) arduboy.tunes.tone(587, 40);
  }

  if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON)) {
    state = STATE_MENU;
  }
}

void menu() {
  arduboy.drawBitmap(10, 2, titleImage, 109, 18, WHITE);
  arduboy.drawRoundRect(10, selectedY * 11 + 21, 112, 10, 2, WHITE);

  arduboy.setTextSize(1);
  arduboy.setCursor(15, 22);
  arduboy.print(F("easy   (20 mines)"));
  arduboy.setCursor(15, 33);
  arduboy.print(F("medium (30 mines)"));
  arduboy.setCursor(15, 44);
  arduboy.print(F("hard   (40 mines)"));
  arduboy.setCursor(15, 55);
  arduboy.print(F("help & sound"));

  if (arduboy.pressed(UP_BUTTON)) {
    if (selectedY == 0) selectedY = 3;
    else selectedY--;
  }
  else if (arduboy.pressed(DOWN_BUTTON)) {
    if (selectedY == 3) selectedY = 0;
    else selectedY++;
  }

  if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON)) {
    if (selectedY == 3) {
      state = STATE_HELP;
    }
    else
    {
      totalMines = 20 + selectedY * 10; // 20 - 30 - 40
      setMines();
      getNumberOfSurroundingMines();
      startTime = millis();
      state = STATE_PLAY;
    }
    selectedY = 0;
  }
}

void checkVictory() {
  byte ret = 0;
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if (opened[x][y]) {
        ret++;
      }
    }
  }
  if (ret == (COLUMNS * ROWS) - totalMines) { //yay
    state = STATE_WIN;
    if (soundEnabled) {
      arduboy.tunes.tone(587, 40);
      delay(160);
      arduboy.tunes.tone(782, 40);
      delay(160);
      arduboy.tunes.tone(977, 40);
    }
  }
}

void clickAllSurrounding(byte x, byte y) {
  for (int dx = x - 1; dx <= x + 1; dx++) {
    for (int dy = y - 1; dy <= y + 1; dy++) {
      if (dx >= 0 && dx < COLUMNS &&
          dy >= 0 && dy < ROWS    &&
          !(dx == x && dy == y)   &&
          !flagged[dx][dy]) {
        clickTile(dx, dy);
      }
    }
  }
}

void loop() {
  if (!(arduboy.nextFrame())) return;
  arduboy.clear();

  if (state == STATE_MENU) {
    menu();
  }
  else if (state == STATE_HELP) {
    helpAndSound();
  }
  else if (state == STATE_PLAY) {
    currentTime = (millis() - startTime) / 1000;
    drawGame();

    if (arduboy.pressed(RIGHT_BUTTON) && selectedX < 14) {
      selectedX++;
    }
    else if (arduboy.pressed(LEFT_BUTTON) && selectedX > 0) {
      selectedX--;
    }
    else if (arduboy.pressed(UP_BUTTON) && selectedY > 0) {
      selectedY--;
    }
    else if (arduboy.pressed(DOWN_BUTTON) && selectedY < 8) {
      selectedY++;
    }

    if (getButtonDown(A_BUTTON)) {
      clickTile(selectedX, selectedY);
    }
    else if (getButtonDown(B_BUTTON)) {
      if (flagged[selectedX][selectedY]) {
        if (soundEnabled) arduboy.tunes.tone(800, 50);
        flagged[selectedX][selectedY] = false;
      } else if (!opened[selectedX][selectedY]) {
        if (soundEnabled) arduboy.tunes.tone(980, 50);
        flagged[selectedX][selectedY] = true;
      } else if (fastMode) {
        clickAllSurrounding(selectedX, selectedY);
      }
    }
    checkVictory();

  }
  else if (state == STATE_WIN) {
    drawGame();
    arduboy.drawBitmap(109, 14, win, 18, 30, WHITE);
    if (getButtonDown(A_BUTTON)) {
      reset();
      state = STATE_MENU;
    }
  }
  else if (state == STATE_LOSE) {
    drawGame();
    drawMines();
    arduboy.drawBitmap(108, 13, dead, 20, 31, WHITE);
    if (getButtonDown(A_BUTTON)) {
      reset();
      state = STATE_MENU;
    }
  }

  arduboy.display();
}

