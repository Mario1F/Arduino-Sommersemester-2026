#include "SnakeGame.h"
#include <string.h>

const uint32_t SnakeGame::_colors[6] PROGMEM = {
  0x000000,  // 0 OFF
  0x181818,  // 1 Wand: dunkelgrau
  0x00FF00,  // 2 Kopf: hellgruen
  0x005000,  // 3 Koerper: dunkelgruen
  0xFF2000,  // 4 Futter 1: orange-rot
  0xFF0060,  // 5 Futter 2: pink
};

uint32_t SnakeGame::_color(uint8_t idx) {
  return pgm_read_dword(&_colors[idx]);
}

static uint16_t _snakeSeed = 42;

uint16_t SnakeGame::_rng() {
  _snakeSeed = _snakeSeed * 6364 + 1;
  return _snakeSeed;
}

SnakeGame::SnakeGame() { reset(); }

void SnakeGame::reset() {
  _gameOver   = false;
  _len        = 3;
  _dr         = 0; _dc = 1;
  _nextDr     = 0; _nextDc = 1;
  _moveTimer  = 6;
  _moveCount  = 0;
  _grew       = false;
  _foodCount  = 0;
  _spawnTimer = 0;

  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++) {
    _foodActive[i] = false;
    _food[i][0] = -1; _food[i][1] = -1;
  }

  int8_t startRow = (SNAKE_ROW_MIN + SNAKE_ROW_MAX) / 2;
  int8_t startCol = (SNAKE_COL_MIN + SNAKE_COL_MAX) / 2;
  _body[0][0] = startRow; _body[0][1] = startCol;
  _body[1][0] = startRow; _body[1][1] = startCol - 1;
  _body[2][0] = startRow; _body[2][1] = startCol - 2;

  // Starte mit genau SNAKE_MIN_FOOD Aepfeln
  for (uint8_t i = 0; i < SNAKE_MIN_FOOD; i++) _spawnFood();
}

bool SnakeGame::_onSnake(int8_t r, int8_t c) {
  for (uint8_t i = 0; i < _len; i++)
    if (_body[i][0] == r && _body[i][1] == c) return true;
  return false;
}

bool SnakeGame::_onFood(int8_t r, int8_t c) {
  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++)
    if (_foodActive[i] && _food[i][0] == r && _food[i][1] == c) return true;
  return false;
}

void SnakeGame::_spawnFood() {
  if (_foodCount >= SNAKE_MAX_FOOD) return;

  // Freien Slot suchen
  uint8_t slot = 0;
  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++) {
    if (!_foodActive[i]) { slot = i; break; }
  }

  // Zufaellige freie Position suchen (nicht auf Schlange, nicht auf anderem Apfel)
  int8_t r, c;
  uint8_t tries = 0;
  do {
    r = SNAKE_ROW_MIN + (_rng() >> 4) % (SNAKE_ROW_MAX - SNAKE_ROW_MIN + 1);
    c = SNAKE_COL_MIN + (_rng() >> 4) % (SNAKE_COL_MAX - SNAKE_COL_MIN + 1);
    tries++;
  } while ((_onSnake(r, c) || _onFood(r, c)) && tries < 100);

  if (tries < 100) {
    _food[slot][0]  = r;
    _food[slot][1]  = c;
    _foodActive[slot] = true;
    _foodCount++;
  }
}

void SnakeGame::handleInput(int8_t dx, int8_t dy, bool /*btn*/) {
  if (_gameOver) return;

  // Richtung (keine 180°-Kehrtwendung)
  if (dy == -1 && _dr != 1)  { _nextDr = -1; _nextDc =  0; }
  if (dy ==  1 && _dr != -1) { _nextDr =  1; _nextDc =  0; }
  if (dx == -1 && _dc != 1)  { _nextDr =  0; _nextDc = -1; }
  if (dx ==  1 && _dc != -1) { _nextDr =  0; _nextDc =  1; }

  // Bewegungs-Timer
  _moveCount++;
  if (_moveCount >= _moveTimer) {
    _moveCount = 0;
    _step();
  }

  // Apfel-Spawn-Timer (nur wenn unter Maximum)
  if (_foodCount < SNAKE_MAX_FOOD) {
    _spawnTimer++;
    if (_spawnTimer >= SNAKE_FOOD_SPAWN_TICKS) {
      _spawnTimer = 0;
      _spawnFood();
    }
  } else {
    _spawnTimer = 0;
  }
}

void SnakeGame::_step() {
  _dr = _nextDr; _dc = _nextDc;

  int8_t newRow = _body[0][0] + _dr;
  int8_t newCol = _body[0][1] + _dc;

  // Oben/unten: Wrap-Around
  if (newRow < SNAKE_ROW_MIN) newRow = SNAKE_ROW_MAX;
  if (newRow > SNAKE_ROW_MAX) newRow = SNAKE_ROW_MIN;

  // Links/rechts: Wand = Tod
  if (newCol < SNAKE_COL_MIN || newCol > SNAKE_COL_MAX) {
    _gameOver = true;
    return;
  }

  // Koerper verschieben
  uint8_t moveLen = _grew ? _len : _len - 1;
  if (_grew && _len < SNAKE_MAX_LEN) _len++;
  _grew = false;

  for (uint8_t i = moveLen; i > 0; i--) {
    _body[i][0] = _body[i-1][0];
    _body[i][1] = _body[i-1][1];
  }
  _body[0][0] = newRow;
  _body[0][1] = newCol;

  if (_selfCollision()) { _gameOver = true; return; }

  // Apfel essen
  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++) {
    if (!_foodActive[i]) continue;
    if (newRow == _food[i][0] && newCol == _food[i][1]) {
      _grew = true;
      if (_moveTimer > 3) _moveTimer--;
      _foodActive[i] = false;
      _foodCount--;
      _food[i][0] = -1; _food[i][1] = -1;

      // Sofort neuen Apfel spawnen wenn unter Minimum
      if (_foodCount < SNAKE_MIN_FOOD) _spawnFood();
      break;
    }
  }
}

bool SnakeGame::_selfCollision() {
  for (uint8_t i = 1; i < _len; i++)
    if (_body[0][0] == _body[i][0] && _body[0][1] == _body[i][1]) return true;
  return false;
}

void SnakeGame::draw(LEDMatrix& matrix) {
  matrix.clear();

  // Waende links (Spalte 0) und rechts (Spalte 31)
  for (int r = 0; r < MATRIX_ROWS; r++) {
    matrix.setPixel(r, 0,             _color(C_WALL));
    matrix.setPixel(r, MATRIX_COLS-1, _color(C_WALL));
  }

  // Aktive Aepfel
  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++) {
    if (!_foodActive[i]) continue;
    uint8_t ci = (i % 2 == 0) ? C_FOOD : C_FOOD2;
    matrix.setPixel(_food[i][0], _food[i][1], _color(ci));
  }

  // Schlange (Koerper zuerst, dann Kopf drueber)
  for (uint8_t i = _len - 1; i > 0; i--)
    matrix.setPixel(_body[i][0], _body[i][1], _color(C_BODY));
  matrix.setPixel(_body[0][0], _body[0][1], _color(C_HEAD));

  // Game Over: Schlange blinkt rot
  if (_gameOver) {
    static uint8_t b = 0; b++;
    if (b < 10)
      for (uint8_t i = 0; i < _len; i++)
        matrix.setPixel(_body[i][0], _body[i][1], 0xFF0000);
    if (b > 20) b = 0;
  }

  matrix.show();
}

bool SnakeGame::isOver() const { return _gameOver; }
