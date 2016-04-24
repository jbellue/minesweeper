#include <Arduboy.h>
#include <ArduboyPlaytune.h>

#include "bitmaps.h"
#include "digits.h"

#define DEBUG

#define DEBOUNCE_TIME 75

#define TILE_SIZE 7
#define ROWS 9
#define COLUMNS 15

#ifdef DEBUG
#define HIGH_SCORE_FILE_NAME 4
#else
#define HIGH_SCORE_FILE_NAME 3
#endif


typedef struct
{
  const char* name;
  const byte mines;
} level;

level levels[] = {
  {"easy",   15},
  {"medium", 25},
  {"hard",   35}
};

enum {
  STATE_MENU,
  STATE_SETTINGS,
  STATE_HELP,
  STATE_HELP2,
  STATE_PLAY,
  STATE_LOSE,
  STATE_WIN,
  STATE_HIGHSCORE,
  STATE_CLEAR_HIGHSCORES,
};
byte state;

char text_buffer[32]; //General string buffer

Arduboy arduboy;
AbPrinter text(arduboy);
ArduboyPlaytune tunes;

byte selectedX = 0;
byte selectedY = 0;
byte menuPosition = 0;

char initials[3];     //Initials used in high score

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

bool fastMode = true;
byte buttons = 0;
byte totalMines;

unsigned long startTime;
unsigned long currentTime;
unsigned long lastClickedTime = 0;

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
#ifdef DEBUG
  arduboy.boot();
  arduboy.audio.begin();
#else
  arduboy.begin();
#endif
  arduboy.setFrameRate(10);
  tunes.initChannel(PIN_SPEAKER_1);
  tunes.initChannel(PIN_SPEAKER_2);
  text.setSize(1);
  arduboy.initRandomSeed();
  reset();
}

bool isOpen(byte x, byte y) {
  return ((tiles[x][y] >> 6) & 1);
}
bool isFlagged(byte x, byte y) {
  return ((tiles[x][y] >> 5) & 1);
}
bool isMined(byte x, byte y) {
  return ((tiles[x][y] >> 7) & 1);
}
byte getSurroundingMines(byte x, byte y) {
  return (tiles[x][y] & 0x0F);
}
void setMine(byte x, byte y) {
  tiles[x][y] |= (1 << 7);
}
void setOpen(byte x, byte y) {
  tiles[x][y] |= (1 << 6);
}
void unsetFlag(byte x, byte y) {
  tiles[x][y] &= ~(1 << 5);
}
void setFlag(byte x, byte y) {
  tiles[x][y] |= (1 << 5);
}

void drawGrid() {
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if (isOpen(x, y)) {
        if (getSurroundingMines(x, y) > 0) { // if there are surrounding mines, draw the digit
          arduboy.drawBitmap(x * TILE_SIZE + 2, y * TILE_SIZE + 1, digits[getSurroundingMines(x, y) - 1], 3, 5, WHITE);
        }
      } else {
        arduboy.fillRect(x * TILE_SIZE + 2, y * TILE_SIZE + 2 , 4, 4, WHITE);
      }
      if (isFlagged(x, y)) { // if flagged
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
#ifdef DEBUG
  drawMines();
#endif
}

void drawMines() {
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if (isMined(x, y)) { // if there's a mine
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
      if (!isMined(x, y)) { // if there's no mine
        for (int dx = x - 1; dx <= x + 1; dx++) {
          for (int dy = y - 1; dy <= y + 1; dy++) {
            if (dx >= 0 && dx < COLUMNS &&
                dy >= 0 && dy < ROWS    &&
                !(dx == x && dy == y)   &&
                (isMined(dx, dy))) {
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

    if (!isMined(randX, randY)) {
      setMine(randX, randY);
      numberOfMines--;
    }
  }
}

void propagate(byte x, byte y) {
  // stop propagation if it's flagged or already opened
  if (isFlagged(x, y) || isOpen(x, y)) {
    return;
  }

  if (state != STATE_LOSE) tunes.tone(587, 20);

  setOpen(x, y);
  if (getSurroundingMines(x, y) > 0) {
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
  if (!isMined(x, y)) {
    unsetFlag(x, y);
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
      if (isFlagged(x, y)) {
        ret--;
      }
    }
  }
  return ret;
}

void drawExtras() {
  text.setCursor(110, 1);
  text.print(minesLeft());
  text.setCursor(110, 56);
  text.print(currentTime);
}

void drawGame() {
  drawGrid();
  drawCursor();
  drawExtras();
}

void settings() {
  arduboy.drawBitmap(0, 1, settingsTitle, 58, 18, WHITE);
  arduboy.drawRoundRect(10, menuPosition * 11 + 20, 112, 11, 5, WHITE);

  text.setCursor(24, 22);
  text.print(F("sounds"));
  text.setCursor(24, 33);
  text.print(F("fast mode"));
  text.setCursor(24, 44);
  text.print(F("help"));
  text.setCursor(24, 55);
  text.print(F("back"));

  if (getButtonDown(UP_BUTTON)) {
    if (menuPosition == 0) menuPosition = 3;
    else menuPosition--;
  }
  else if (getButtonDown(DOWN_BUTTON)) {
    if (menuPosition == 3) menuPosition = 0;
    else menuPosition++;
  }

  if (ArduboyAudio::enabled()) arduboy.fillCircle(15, 25, 3, WHITE);
  if (fastMode) arduboy.fillCircle(15, 36, 3, WHITE);

  if (getButtonDown(A_BUTTON)) {
    menuPosition = 0;
    state = STATE_MENU;
  }
  if (getButtonDown(B_BUTTON)) {
    if (menuPosition == 0) {
      if (ArduboyAudio::enabled()) {
        ArduboyAudio::off();
      }
      else {
        ArduboyAudio::on();
      }
      ArduboyAudio::saveOnOff();
      tunes.tone(587, 40);
    }
    else if (menuPosition == 1) {
      fastMode = !fastMode;
      tunes.tone(587, 40);
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

  text.setCursor(0, 17);
  text.print(F("Flagging an open tile"));
  text.setCursor(0, 27);
  text.print(F("opens all non-flagged"));
  text.setCursor(0, 37);
  text.print(F("tiles around."));
  text.setCursor(43, 53);
  text.print(F("Careful!"));

  if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON)) {
    state = STATE_SETTINGS;
  }
}

void clearHighscoreConfirm() {
  text.setSize(2);
  text.setCursor(22, 0);
  text.print(F("WARNING"));
  text.setSize(1);

  text.setCursor(10, 22);
  text.print(F("Delete highscores?"));
  arduboy.drawRoundRect(38, menuPosition * 11 + 35, 51, 11, 5, WHITE);
  if (firstTime) {
    text.setCursor(43, 37);
    text.print(F("Confirm"));
  }
  else {
    text.setCursor(48, 37);
    text.print(F("Sure?"));

  }
  text.setCursor(46, 48);
  text.print(F("Cancel"));

  if (getButtonDown(UP_BUTTON) || getButtonDown(DOWN_BUTTON)) {
    menuPosition = (menuPosition == 0) ? 1 : 0;
  }
  if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON)) {
    if (menuPosition == 0) {
      if (firstTime) {
        firstTime = false;
        return;
      }
      else {
        clearHighScores(HIGH_SCORE_FILE_NAME);
        firstTime = true;
      }
    }
    state = STATE_MENU;
  }
}

void helpControls() {
  //drawing the arduboy
  arduboy.drawBitmap(76, 0, arduboyBMP, 52, 64, WHITE);
  arduboy.drawBitmap(0, 1, controlsTitle, 66, 13, WHITE);

  text.setCursor(8, 21);
  text.print(F("toggle flag"));
  text.setCursor(8, 36);
  text.print(F("move cursor"));
  text.setCursor(1, 51);
  text.print(F("click a tile"));

  // sound icon in the screen.
  if (ArduboyAudio::enabled()) {
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

  for (byte i = 0; i < 3; i++) {
    text.setCursor(15, (11 * i) + 22);
    sprintf(text_buffer, "%-6s (%d mines)", levels[i].name, levels[i].mines);
    text.print(text_buffer);
  }

  text.setCursor(15, 55);
  text.print(F("settings and help"));

  if (getButtonDown(UP_BUTTON)) {
    if (menuPosition == 0) menuPosition = 3;
    else menuPosition--;
  }
  else if (getButtonDown(DOWN_BUTTON)) {
    if (menuPosition == 3) menuPosition = 0;
    else menuPosition++;
  }
  else if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON)) {
    if (menuPosition == 3) {
      state = STATE_SETTINGS;
      menuPosition = 0;
    }
    else
    {
      totalMines = levels[menuPosition].mines;
      setMines();
      getNumberOfSurroundingMines();
      startTime = millis();
      state = STATE_PLAY;
    }
  }
  else if (getButtonDown(LEFT_BUTTON) || getButtonDown(RIGHT_BUTTON)) {
    menuPosition = 0;
    state = STATE_HIGHSCORE;
  }
}

void checkVictory() {
  byte ret = 0;
  for (byte x = 0; x < COLUMNS; x++) {
    for (byte y = 0; y < ROWS; y++) {
      if (isOpen(x, y)) {
        ret++;
      }
    }
  }
  if (ret == (COLUMNS * ROWS) - totalMines) { //yay
    state = STATE_WIN;
    if (ArduboyAudio::enabled()) {
      tunes.tone(587, 40);
      delay(160);
      tunes.tone(782, 40);
      delay(160);
      tunes.tone(977, 40);
    }
  }
}

void clickAllSurrounding(byte x, byte y) {
  for (int dx = x - 1; dx <= x + 1; dx++) {
    for (int dy = y - 1; dy <= y + 1; dy++) {
      if (dx >= 0 && dx < COLUMNS &&
          dy >= 0 && dy < ROWS    &&
          !(dx == x && dy == y)   &&
          !isFlagged(dx, dy)) {
        clickTile(dx, dy);
      }
    }
  }
}

bool clickButton(byte btn) {
  if (arduboy.pressed(btn) && millis() > lastClickedTime + DEBOUNCE_TIME) {
    lastClickedTime = millis();
    return true;
  }
  else {
    return false;
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
    if (((millis() - startTime) / 1000) < 999) {
      currentTime = ((millis() - startTime) / 1000);
    }
    else {
      currentTime = 999;
    }

    drawGame();
    if (clickButton(RIGHT_BUTTON) && selectedX < COLUMNS - 1) {
      selectedX++;
    }
    else if (clickButton(LEFT_BUTTON) && selectedX > 0) {
      selectedX--;
    }
    if (clickButton(UP_BUTTON) && selectedY > 0) {
      selectedY--;
    }
    else if (clickButton(DOWN_BUTTON) && selectedY < ROWS - 1) {
      selectedY++;
    }
    if (getButtonDown(A_BUTTON)) {
      clickTile(selectedX, selectedY);
    }
    else if (getButtonDown(B_BUTTON)) {
      if (isFlagged(selectedX, selectedY)) {
        tunes.tone(800, 50);
        unsetFlag(selectedX, selectedY);
      } else if (!isOpen(selectedX, selectedY)) {
        tunes.tone(980, 50);
        setFlag(selectedX, selectedY);
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
      enterHighScore(HIGH_SCORE_FILE_NAME, menuPosition);

      reset();
      state = STATE_HIGHSCORE;
    }
  }
  else if (state == STATE_LOSE) {
    if (firstTime) {
      firstTime = false;
      if (ArduboyAudio::enabled()) {
        tunes.tone(587, 40);
        delay(160);
        tunes.tone(392, 40);
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
  else if (state == STATE_HIGHSCORE) {
    displayHighScores(HIGH_SCORE_FILE_NAME);
  }
  else if (state == STATE_CLEAR_HIGHSCORES) {
    clearHighscoreConfirm();
  }
  arduboy.display();
}

// Function by nootropic design to add high scores that I stole and hacked
// it's more reactive than the original one because I removed the delay,
// but now you can't leave a button pressed... To make it usable, I limited
// the entry to A-Z. Sorry, cyborgs will have to use a human alias.
void enterInitials() {
  byte index = 0;

  arduboy.clear();

  initials[0] = ' ';
  initials[1] = ' ';
  initials[2] = ' ';

  while (true) {
    arduboy.display();
    arduboy.clear();

    text.setCursor(25, 0);
    text.print(F("NEW BEST TIME"));
    text.setCursor((currentTime > 99) ? 55 : 58, 15);
    text.print(currentTime);
    arduboy.drawChar(55, 35, initials[0], WHITE, BLACK, 1);
    arduboy.drawChar(63, 35, initials[1], WHITE, BLACK, 1);
    arduboy.drawChar(71, 35, initials[2], WHITE, BLACK, 1);
    for (byte i = 0; i < 3; i++) {
      arduboy.drawLine(54 + (i * 8), 43, 54 + (i * 8) + 6, 43, 1);
    }
    arduboy.fillTriangle(56 + (index * 8), 32, 56 + (index * 8) + 2, 32, 56 + (index * 8) + 1, 31, 1);
    arduboy.fillTriangle(56 + (index * 8), 45, 56 + (index * 8) + 2, 45, 56 + (index * 8) + 1, 46, 1);

    if (getButtonDown(LEFT_BUTTON) || getButtonDown(B_BUTTON)) {
      if (index > 0) {
        index--;
      }
      tunes.tone(1046, 50);
    }
    else if (getButtonDown(RIGHT_BUTTON)) {
      if (index < 2) {
        index++;
      }
      tunes.tone(1046, 50);
    }

    if (clickButton(DOWN_BUTTON)) {
      initials[index]++;
      tunes.tone(523, 25);
      // A-Z
      // A-Z 0-9 :-? !-/ ' '
      if (initials[index] == '0') {
        initials[index] = ' ';
      } else if (initials[index] == '!') {
        initials[index] = 'A';
      } else if (initials[index] == '[')  {
        initials[index] = '0';
      } else if (initials[index] == '@') {
        initials[index] = '!';
      }
    }
    else if (clickButton(UP_BUTTON)) {
      initials[index]--;
      tunes.tone(523, 25);
      if (initials[index] == ' ') {
        initials[index] = '?';
      } else if (initials[index] == '/') {
        initials[index] = 'Z';
      } else if (initials[index] == 31) {
        initials[index] = '/';
      } else if (initials[index] == '@') {
        initials[index] = ' ';
      }
    }

    if (getButtonDown(A_BUTTON)) {
      tunes.tone(1046, 50);
      if (index < 2) {
        index++;
      }
      else {
        return;
      }
    }
  }
}

void clearHighScores(byte file) {
  // Each block of EEPROM has 3 high scores, and each high score entry
  // is 5 bytes long:  3 bytes for initials and two bytes for score.
  int maxSize = file * 3 * 5 + 15;
  for (int i = file * 3 * 5; i < maxSize ; i++) {
    EEPROM.write(i, 0);
  }
}

void enterHighScore(byte file, byte level) {
  // Each block of EEPROM has 3 high scores, and each high score entry
  // is 5 bytes long:  3 bytes for initials and two bytes for score.
  int address = file * 3 * 5;
  byte hi, lo;
  unsigned int tmpScore = 0;

  // Best time processing
  hi = EEPROM.read(address + (5 * level));
  lo = EEPROM.read(address + (5 * level) + 1);
  //  if ((hi == 0xFF) && (lo == 0xFF)) {
  if ((hi == 0x00) && (lo == 0x00)) {
    // The values are uninitialized, so treat this entry
    // as a score of 999 (max time)
    tmpScore = 999;
  }
  else {
    tmpScore = (hi << 8) | lo;
  }
  if (currentTime < tmpScore) {
    enterInitials();

    // write score and initials to current slot
    EEPROM.write(address + (5 * level), ((currentTime >> 8) & 0xFF));
    EEPROM.write(address + (5 * level) + 1, (currentTime & 0xFF));
    EEPROM.write(address + (5 * level) + 2, initials[0]);
    EEPROM.write(address + (5 * level) + 3, initials[1]);
    EEPROM.write(address + (5 * level) + 4, initials[2]);

    initials[0] = ' ';
    initials[1] = ' ';
    initials[2] = ' ';

    return;
  }
}

//Function by nootropic design to display highscores
void displayHighScores(byte file) {
  // Each block of EEPROM has 3 high scores (easy, medium, hard),
  // and each entry is 5 bytes long:
  // 3 bytes for initials and two bytes for time.
  int address = file * 3 * 5;
  byte hi, lo;
  arduboy.clear();
  text.setCursor(34, 5);
  text.print(F("BEST TIMES"));
  arduboy.display();

  for (int i = 0; i < 3; i++) {
    hi = EEPROM.read(address + (5 * i));
    lo = EEPROM.read(address + (5 * i) + 1);

    if ((hi == 0x00) && (lo == 0x00)) {
      currentTime = 999;
    }
    else {
      currentTime = (hi << 8) | lo;
    }

    initials[0] = (char)EEPROM.read(address + (5 * i) + 2);
    initials[1] = (char)EEPROM.read(address + (5 * i) + 3);
    initials[2] = (char)EEPROM.read(address + (5 * i) + 4);

    if (currentTime < 999) {
      sprintf(text_buffer, "%-6s %c%c%c (%u)", levels[i].name, initials[0], initials[1], initials[2], currentTime);
      text.setCursor(22, 22 + (i * 12));
      text.print(text_buffer);

      arduboy.display();
    }
  }
  while (true) {
    if (getButtonDown(DOWN_BUTTON + UP_BUTTON)) {
      state = STATE_CLEAR_HIGHSCORES;
      return;
    }
    else if (getButtonDown(A_BUTTON) || getButtonDown(B_BUTTON) ||
             getButtonDown(LEFT_BUTTON) || getButtonDown(RIGHT_BUTTON)) {
      state = STATE_MENU;
      return;
    }
  }
}

