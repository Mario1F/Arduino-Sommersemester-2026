#include "SnakeGame.h"
#include <string.h>

const uint32_t SnakeGame::_colors[6] PROGMEM = {
  0x000000, 0x181818, 0x00FF00, 0x005000, 0xFF2000, 0xFF0060,
};

uint32_t SnakeGame::_color(uint8_t idx) {
  return pgm_read_dword(&_colors[idx]);
}

// Seed als Instanzvariable — kein static → kein Reset-Bug
uint16_t SnakeGame::_rng() {
  _seed = _seed * 6364 + 1;
  return _seed;
}

SnakeGame::SnakeGame() { reset(); }

void SnakeGame::reset() {
  _gameOver    = false;
  _gameOverAnim = 0;
  _len         = 3;
  _dr = 0; _dc = 1;
  _nextDr = 0; _nextDc = 1;
  _moveTimer   = 6;
  _moveCount   = 0;
  _grew        = false;
  _foodCount   = 0;
  _spawnTimer  = 0;
  // Seed aus Timer → jedes Spiel anders
  _seed = (uint16_t)(millis() & 0xFFFF) | 1;

  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++) {
    _foodActive[i] = false;
    _food[i][0] = -1; _food[i][1] = -1;
  }

  int8_t startRow = (SNAKE_ROW_MIN + SNAKE_ROW_MAX) / 2;
  int8_t startCol = (SNAKE_COL_MIN + SNAKE_COL_MAX) / 2;
  _body[0][0] = startRow; _body[0][1] = startCol;
  _body[1][0] = startRow; _body[1][1] = startCol - 1;
  _body[2][0] = startRow; _body[2][1] = startCol - 2;

  // Starte sofort mit SNAKE_MIN_FOOD Aepfeln
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

  uint8_t slot = 0;
  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++)
    if (!_foodActive[i]) { slot = i; break; }

  int8_t r, c;
  uint8_t tries = 0;
  do {
    r = SNAKE_ROW_MIN + (_rng() >> 4) % (SNAKE_ROW_MAX - SNAKE_ROW_MIN + 1);
    c = SNAKE_COL_MIN + (_rng() >> 4) % (SNAKE_COL_MAX - SNAKE_COL_MIN + 1);
    tries++;
    // Abbruch nach 99 Versuchen (tries==100 würde noch einmal testen)
  } while (tries < 99 && (_onSnake(r, c) || _onFood(r, c)));

  _food[slot][0]    = r;
  _food[slot][1]    = c;
  _foodActive[slot] = true;
  _foodCount++;
}

void SnakeGame::handleInput(int8_t dx, int8_t dy, bool /*btn*/) {
  if (_gameOver) return;

  if (dy == -1 && _dr != 1)  { _nextDr=-1; _nextDc= 0; }
  if (dy ==  1 && _dr != -1) { _nextDr= 1; _nextDc= 0; }
  if (dx == -1 && _dc != 1)  { _nextDr= 0; _nextDc=-1; }
  if (dx ==  1 && _dc != -1) { _nextDr= 0; _nextDc= 1; }

  _moveCount++;
  if (_moveCount >= _moveTimer) { _moveCount = 0; _step(); }

  // Apfel-Spawn-Timer
  if (_foodCount < SNAKE_MAX_FOOD) {
    _spawnTimer++;
    if (_spawnTimer >= SNAKE_FOOD_SPAWN_TICKS) { _spawnTimer = 0; _spawnFood(); }
  } else {
    _spawnTimer = 0;
  }
}

void SnakeGame::_step() {
  _dr = _nextDr; _dc = _nextDc;
  int8_t newRow = _body[0][0] + _dr;
  int8_t newCol = _body[0][1] + _dc;

  // Wrap oben/unten
  if (newRow < SNAKE_ROW_MIN) newRow = SNAKE_ROW_MAX;
  if (newRow > SNAKE_ROW_MAX) newRow = SNAKE_ROW_MIN;

  // Wand links/rechts = Tod
  if (newCol < SNAKE_COL_MIN || newCol > SNAKE_COL_MAX) { _gameOver = true; return; }

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

  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++) {
    if (!_foodActive[i]) continue;
    if (newRow == _food[i][0] && newCol == _food[i][1]) {
      _grew = true;
      if (_moveTimer > 3) _moveTimer--;
      _foodActive[i] = false;
      _foodCount--;
      _food[i][0] = -1; _food[i][1] = -1;
      // Sofort nachspawnen wenn unter Minimum
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

  // Waende
  for (int r = 0; r < MATRIX_ROWS; r++) {
    matrix.setPixel(r, 0,             _color(C_WALL));
    matrix.setPixel(r, MATRIX_COLS-1, _color(C_WALL));
  }

  // Aepfel
  for (uint8_t i = 0; i < SNAKE_MAX_FOOD; i++) {
    if (!_foodActive[i]) continue;
    matrix.setPixel(_food[i][0], _food[i][1],
                    _color((i % 2 == 0) ? C_FOOD : C_FOOD2));
  }

  // Schlange
  for (uint8_t i = _len - 1; i > 0; i--)
    matrix.setPixel(_body[i][0], _body[i][1], _color(C_BODY));
  matrix.setPixel(_body[0][0], _body[0][1], _color(C_HEAD));

  // Verlier-Animation: Schlange segmentweise von hinten rot/dunkel
  if (_gameOver) {
    if (_gameOverAnim < _len) _gameOverAnim++;
    for (uint8_t i = 0; i < _gameOverAnim; i++) {
      uint32_t col = (i % 2 == 0) ? 0x440000 : 0x200000;
      matrix.setPixel(_body[_len - 1 - i][0], _body[_len - 1 - i][1], col);
    }
  }

  matrix.show();
}

bool SnakeGame::isOver() const { return _gameOver; }
