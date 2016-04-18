#include <Arduboy.h>
#include "bitmaps.h"
#include "digits.h"

#define TILE_SIZE 7
#define ROWS 9
#define COLUMNS 15

enum {
  STATE_MENU,
  STATE_SETTINGS,
  STATE_HELP,
  STATE_HELP2,
  STATE_PLAY,
  STATE_LOSE,
  STATE_WIN,
};
byte state;

Arduboy arduboy;
byte selectedX = 0;
byte selectedY = 0;
byte menuPosition = 0;

byte tiles[COLUMNS][ROWS];
/* Each tile is defined by a byte:
  00000000
  1....... tile has a mine
  .1...... tile has been opened
  ..1..... tile has been flagged
  ...x.... unused, waste of valuable space
  ....1111 number of surrounding mines (0 to 8)
*/

bool firstTime;
bool soundEnabled = true;
bool fastMode = true;
byte buttons = 0;
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
  firstTime = true;
  memset(tiles, 0, sizeof(tiles[0][0]) * ROWS * COLUMNS);
  arduboy.clear();
  state = STATE_MENU;
}

void setup() {
  //dbg
  arduboy.beginNoLogo();
  //dbg
  //  arduboy.begin();
  arduboy.setFrameRate(10);
  arduboy.setTextSize(1);
  arduboy.initRandomSeed();
  reset();
}

void drawGrid() {
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if ((tiles[x][y] >> 6) & 1) { // if opened
        if ((tiles[x][y] & 0x0F) > 0) { // if there are surrounding mines, draw the digit
          arduboy.drawBitmap(x * TILE_SIZE + 2, y * TILE_SIZE + 1, digits[(tiles[x][y] & 0x0F) - 1], 3, 5, WHITE);
        }
      } else {
        arduboy.fillRect(x * TILE_SIZE + 2, y * TILE_SIZE + 2 , 4, 4, WHITE);
      }
      if ((tiles[x][y] >> 5) & 1) { // if flagged
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
      if ((tiles[x][y] >> 7) & 1) { // if there's a mine
        arduboy.drawBitmap(x * TILE_SIZE + 2, y * TILE_SIZE + 2, mineTile, 4, 4, BLACK);
      }
    }
  }
}

void getNumberOfSurroundingMines() {
  byte surroundingMines;
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      surroundingMines = 0;
      if (!((tiles[x][y] >> 7) & 1)) { // if there's no mine
        for (int dx = x - 1; dx <= x + 1; dx++) {
          for (int dy = y - 1; dy <= y + 1; dy++) {
            if (dx >= 0 && dx < COLUMNS &&
                dy >= 0 && dy < ROWS    &&
                !(dx == x && dy == y)   &&
                ((tiles[dx][dy] >> 7) & 1)) {
              surroundingMines++;
            }
          }
        }
      }
      tiles[x][y] += surroundingMines;
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

    if (!(tiles[randX][randY] >> 7) & 1) {
      tiles[randX][randY] |= (1 << 7);
      numberOfMines--;
    }
  }
}

void propagate(byte x, byte y) {
  // stop propagation if it's flagged or already opened
  if (((tiles[x][y] >> 5) & 1) || ((tiles[x][y] >> 6) & 1)) {
    return;
  }

  if (soundEnabled && state != STATE_LOSE) arduboy.tunes.tone(587, 20);

  tiles[x][y] |= (1 << 6); // open tile
  if ((tiles[x][y] & 0x0F) > 0) {
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
  if (!(tiles[x][y] >> 7) & 1) {
    tiles[x][y] &= ~(1 << 5); // unflag tile
    propagate(x, y);
  } else {
    state = STATE_LOSE;
  }
}

int minesLeft() {
  if (state == STATE_WIN) {
    return 0;
  }
  int ret = totalMines;
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if ((tiles[x][y] >> 5) & 1) { // if flagged
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

void settings() {
  arduboy.drawBitmap(0, 1, settingsTitle, 58, 18, WHITE);
  arduboy.drawRoundRect(10, menuPosition * 11 + 20, 112, 11, 5, WHITE);

  arduboy.setCursor(24, 22);
  arduboy.print(F("sounds"));
  arduboy.setCursor(24, 33);
  arduboy.print(F("fast mode"));
  arduboy.setCursor(24, 44);
  arduboy.print(F("help"));
  arduboy.setCursor(24, 55);
  arduboy.print(F("back"));

  if (arduboy.pressed(UP_BUTTON)) {
    if (menuPosition == 0) menuPosition = 3;
    else menuPosition--;
  }
  else if (arduboy.pressed(DOWN_BUTTON)) {
    if (menuPosition == 3) menuPosition = 0;
    else menuPosition++;
  }

  if (soundEnabled) arduboy.fillCircle(15, 25, 3, WHITE);
  if (fastMode) arduboy.fillCircle(15, 36, 3, WHITE);

  if (getButtonDown(A_BUTTON)) {
    menuPosition = 0;
    state = STATE_MENU;
  }
  if (getButtonDown(B_BUTTON)) {
    if (menuPosition == 0) {
      soundEnabled = !soundEnabled;
      if (soundEnabled) arduboy.tunes.tone(587, 40);
    }
    else if (menuPosition == 1) {
      fastMode = !fastMode;
      if (soundEnabled) arduboy.tunes.tone(587, 40);
    }
    else if (menuPosition == 2) {
      menuPosition = 0;
      state = STATE_HELP;
    }
    else {
      menuPosition = 0;
      state = STATE_MENU;
    }
  }
}

void helpFastMode() {
  arduboy.drawBitmap(0, 1, fastModeTitle, 74, 13, WHITE);

  arduboy.setCursor(0, 17);
  arduboy.print(F("Flagging an open tile"));
  arduboy.setCursor(0, 27);
  arduboy.print(F("opens all non-flagged"));
  arduboy.setCursor(0, 37);
  arduboy.print(F("tiles around."));
  arduboy.setCursor(43, 53);
  arduboy.print(F("Careful!"));

  if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON)) {
    state = STATE_SETTINGS;
  }
}

void helpControls() {
  //drawing the arduboy
  arduboy.drawBitmap(76, 0, arduboyBMP, 52, 64, WHITE);
  arduboy.drawBitmap(0, 1, controlsTitle, 66, 13, WHITE);

  arduboy.setCursor(8, 21);
  arduboy.print(F("toggle flag"));
  arduboy.setCursor(8, 36);
  arduboy.print(F("move cursor"));
  arduboy.setCursor(1, 51);
  arduboy.print(F("click a tile"));

  // sound icon in the screen.
  if (soundEnabled) {
    arduboy.drawBitmap(101, 8, sound, 14, 12, WHITE);
  } else {
    arduboy.drawBitmap(103, 8, noSound, 12, 12, WHITE);
  }

  if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON)) {
    state = STATE_HELP2;
  }
}

void menu() {
  arduboy.drawBitmap(3, 3, titleImage, 122, 12, WHITE);
  arduboy.drawRoundRect(10, menuPosition * 11 + 21, 112, 10, 5, WHITE);

  arduboy.setCursor(15, 22);
  arduboy.print(F("easy   (20 mines)"));
  arduboy.setCursor(15, 33);
  arduboy.print(F("medium (30 mines)"));
  arduboy.setCursor(15, 44);
  arduboy.print(F("hard   (40 mines)"));
  arduboy.setCursor(15, 55);
  arduboy.print(F("settings and help"));

  if (arduboy.pressed(UP_BUTTON)) {
    if (menuPosition == 0) menuPosition = 3;
    else menuPosition--;
  }
  else if (arduboy.pressed(DOWN_BUTTON)) {
    if (menuPosition == 3) menuPosition = 0;
    else menuPosition++;
  }

  if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON)) {
    if (menuPosition == 3) {
      state = STATE_SETTINGS;
      menuPosition = 0;
    }
    else
    {
      totalMines = 20 + menuPosition * 10; // 20 - 30 - 40
      setMines();
      getNumberOfSurroundingMines();
      startTime = millis();
      state = STATE_PLAY;
    }
  }
}

void checkVictory() {
  byte ret = 0;
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if ((tiles[x][y] >> 6) & 1) {
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
          !((tiles[dx][dy] >> 5) & 1)) {
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
  else if (state == STATE_SETTINGS) {
    settings();
  }
  else if (state == STATE_HELP) {
    helpControls();
  }
  else if (state == STATE_HELP2) {
    helpFastMode();
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
      if ((tiles[selectedX][selectedY] >> 5) & 1) { // if flagged
        if (soundEnabled) arduboy.tunes.tone(800, 50);
        tiles[selectedX][selectedY] &= ~(1 << 5); //unflag
      } else if (!((tiles[selectedX][selectedY] >> 6) & 1)) { // if not opened
        if (soundEnabled) arduboy.tunes.tone(980, 50);
        tiles[selectedX][selectedY] |= (1 << 5); // flag
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
    if (firstTime) {
      firstTime = false;
      if (soundEnabled) {
        arduboy.tunes.tone(587, 40);
        delay(160);
        arduboy.tunes.tone(392, 40);
      }
    }
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

