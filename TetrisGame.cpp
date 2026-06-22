#include "TetrisGame.h"
#include <string.h>

const uint32_t TetrisGame::_colors[9] PROGMEM = {
  0x000000, 0x004040, 0x404000, 0x004400,
  0x440000, 0x300040, 0x402000, 0x001040,
  0x080808,
};

const int8_t TetrisGame::_pieces[7][4][8] PROGMEM = {
  {{ 0,-1, 0, 0, 0, 1, 0, 2},{-1, 0, 0, 0, 1, 0, 2, 0},{ 0,-1, 0, 0, 0, 1, 0, 2},{-1, 0, 0, 0, 1, 0, 2, 0}},
  {{ 0, 0, 0, 1, 1, 0, 1, 1},{ 0, 0, 0, 1, 1, 0, 1, 1},{ 0, 0, 0, 1, 1, 0, 1, 1},{ 0, 0, 0, 1, 1, 0, 1, 1}},
  {{ 0, 0, 0, 1,-1, 1,-1, 2},{-1, 0, 0, 0, 0, 1, 1, 1},{ 0, 0, 0, 1,-1, 1,-1, 2},{-1, 0, 0, 0, 0, 1, 1, 1}},
  {{-1, 0,-1, 1, 0, 1, 0, 2},{ 0, 0, 1, 0, 1, 1, 2, 1},{-1, 0,-1, 1, 0, 1, 0, 2},{ 0, 0, 1, 0, 1, 1, 2, 1}},
  {{ 0, 0, 0, 1, 0, 2,-1, 1},{-1, 0, 0, 0, 1, 0, 0, 1},{ 0, 0, 0, 1, 0, 2, 1, 1},{ 0, 0,-1, 0, 1, 0, 0,-1}},
  {{ 0, 0, 1, 0, 2, 0, 2, 1},{ 0, 0, 0, 1, 0, 2, 1, 0},{ 0, 0, 0, 1,-2, 1,-2, 0},{ 0, 0, 1, 0,-1, 0, 0,-1}},
  {{ 0, 0, 1, 0, 2, 0, 2,-1},{ 0, 0, 0, 1, 0, 2,-1, 0},{ 0, 0,-1, 0,-2, 0,-2, 1},{ 0, 0, 1, 0, 0,-1, 0,-2}},
};

uint32_t TetrisGame::_color(uint8_t idx) {
  return pgm_read_dword(&_colors[idx]);
}

// ── 7-Bag: Seed als Instanzvariable, kein static ─────────────
void TetrisGame::_shuffleBag() {
  for (uint8_t i = 0; i < 7; i++) _bag[i] = i;
  for (uint8_t i = 6; i > 0; i--) {
    _bagSeed = _bagSeed * 6364 + 1;
    uint8_t j = (_bagSeed >> 5) % (i + 1);
    uint8_t tmp = _bag[i]; _bag[i] = _bag[j]; _bag[j] = tmp;
  }
  _bagPos = 0;
}

uint8_t TetrisGame::_nextFromBag() {
  if (_bagPos >= 7) _shuffleBag();
  return _bag[_bagPos++];
}

TetrisGame::TetrisGame() { reset(); }

void TetrisGame::reset() {
  memset(_board,    0, sizeof(_board));
  memset(_rowColor, 0, sizeof(_rowColor));
  _gameOver     = false;
  _gameOverAnim = 0;
  _fallTimer    = 25;
  _fallCount    = 0;
  _level        = 0;
  _linesCleared = 0;
  _inputTimer   = 0;
  // Seed aus Timer → jedes Reset anders, kein static nötig
  _bagSeed      = (uint16_t)(millis() & 0xFFFF) | 1;
  _bagPos       = 7;
  _nextType     = _nextFromBag();
  _spawnPiece();
}

void TetrisGame::_getBlocks(uint8_t type, uint8_t rot, int8_t out[4][2]) {
  for (uint8_t i = 0; i < 4; i++) {
    out[i][0] = pgm_read_byte(&_pieces[type][rot][i*2]);
    out[i][1] = pgm_read_byte(&_pieces[type][rot][i*2+1]);
  }
}

bool TetrisGame::_canPlace(int8_t r, int8_t c, uint8_t type, uint8_t rot) {
  int8_t blocks[4][2];
  _getBlocks(type, rot, blocks);
  for (uint8_t i = 0; i < 4; i++) {
    int8_t br = r + blocks[i][0];
    int8_t bc = c + blocks[i][1];
    if (br < 0 || br >= TET_ROWS) return false;
    if (bc < 0 || bc >= TET_COLS) return false;
    if (_board[br] & (1 << bc))   return false;
  }
  return true;
}

void TetrisGame::_lockPiece() {
  int8_t blocks[4][2];
  _getBlocks(_pieceType, _pieceRot, blocks);
  for (uint8_t i = 0; i < 4; i++) {
    int8_t br = _pieceR + blocks[i][0];
    int8_t bc = _pieceC + blocks[i][1];
    if (br >= 0 && br < TET_ROWS && bc >= 0 && bc < TET_COLS) {
      _board[br]    |= (1 << bc);
      _rowColor[br]  = _pieceColor;
    }
  }
}

void TetrisGame::_clearLines() {
  uint8_t cleared = 0;
  for (int8_t r = TET_ROWS - 1; r >= 0; r--) {
    uint16_t fullMask = (1 << TET_COLS) - 1;
    if ((_board[r] & fullMask) == fullMask) {
      for (int8_t rr = r; rr > 0; rr--) {
        _board[rr]    = _board[rr-1];
        _rowColor[rr] = _rowColor[rr-1];
      }
      _board[0] = 0; _rowColor[0] = 0;
      cleared++; r++;
    }
  }
  if (cleared > 0) {
    _linesCleared += cleared;
    _level     = _linesCleared / 5;
    _fallTimer = (_level * 2 < 17) ? 25 - _level * 2 : 8;
  }
}

void TetrisGame::_spawnPiece() {
  _pieceType  = _nextType;
  _nextType   = _nextFromBag();
  _pieceColor = _pieceType + 1;
  _pieceRot   = 0;
  _pieceR     = 0;
  _pieceC     = TET_COLS / 2 - 1;
  if (!_canPlace(_pieceR, _pieceC, _pieceType, _pieceRot))
    _gameOver = true;
}

void TetrisGame::handleInput(int8_t dx, int8_t dy, bool btn) {
  if (_gameOver) return;
  if (_inputTimer > 0) { _inputTimer--; }
  else {
    if (dx == -1 && _canPlace(_pieceR, _pieceC-1, _pieceType, _pieceRot)) { _pieceC--; _inputTimer=4; }
    if (dx ==  1 && _canPlace(_pieceR, _pieceC+1, _pieceType, _pieceRot)) { _pieceC++; _inputTimer=4; }
    if (dy ==  1 && _canPlace(_pieceR+1, _pieceC, _pieceType, _pieceRot)) { _pieceR++; _inputTimer=2; }
    if (btn) {
      uint8_t nr = (_pieceRot+1)%4;
      if (_canPlace(_pieceR, _pieceC, _pieceType, nr)) { _pieceRot=nr; _inputTimer=8; }
    }
  }
  _fallCount++;
  if (_fallCount >= _fallTimer) {
    _fallCount = 0;
    if (_canPlace(_pieceR+1, _pieceC, _pieceType, _pieceRot)) {
      _pieceR++;
    } else {
      _lockPiece(); _clearLines(); _spawnPiece();
    }
  }
}

void TetrisGame::draw(LEDMatrix& matrix) {
  matrix.clear();

  // Verlier-Animation: Reihen von unten rot auffüllen
  if (_gameOver) {
    if (_gameOverAnim < TET_ROWS) _gameOverAnim++;
    // Normales Board zeichnen
    for (int8_t r = 0; r < TET_ROWS; r++)
      for (int8_t c = 0; c < TET_COLS; c++)
        if (_board[r] & (1 << c))
          matrix.setPixel(r, c + TET_COL_OFF, _color(_rowColor[r]));
    // Rote Welle von unten
    for (uint8_t i = 0; i < _gameOverAnim; i++) {
      int8_t r = TET_ROWS - 1 - i;
      uint32_t col = (i % 2 == 0) ? 0x440000 : 0x200000;
      for (int8_t c = 0; c < TET_COLS; c++)
        matrix.setPixel(r, c + TET_COL_OFF, col);
    }
    matrix.show();
    return;
  }

  // Rahmen
  for (int8_t r = 0; r < TET_ROWS; r++) {
    matrix.setPixel(r, TET_COL_OFF-1,        _color(8));
    matrix.setPixel(r, TET_COL_OFF+TET_COLS, _color(8));
  }

  // Board
  for (int8_t r = 0; r < TET_ROWS; r++)
    for (int8_t c = 0; c < TET_COLS; c++)
      if (_board[r] & (1 << c))
        matrix.setPixel(r, c + TET_COL_OFF, _color(_rowColor[r]));

  int8_t blocks[4][2];
  _getBlocks(_pieceType, _pieceRot, blocks);

  // Ghost
  int8_t ghostR = _pieceR;
  while (_canPlace(ghostR+1, _pieceC, _pieceType, _pieceRot)) ghostR++;
  if (ghostR != _pieceR) {
    for (uint8_t i = 0; i < 4; i++) {
      int8_t br = ghostR + blocks[i][0], bc = _pieceC + blocks[i][1];
      if (br >= 0 && br < TET_ROWS && bc >= 0 && bc < TET_COLS)
        if (!(_board[br] & (1 << bc)))
          matrix.setPixel(br, bc + TET_COL_OFF, 0x080808);
    }
  }

  // Aktives Stück (heller)
  for (uint8_t i = 0; i < 4; i++) {
    int8_t br = _pieceR + blocks[i][0], bc = _pieceC + blocks[i][1];
    if (br >= 0 && br < TET_ROWS && bc >= 0 && bc < TET_COLS) {
      uint32_t c = _color(_pieceColor);
      uint8_t r8=((c>>16)&0xFF); r8=r8>127?255:r8*2;
      uint8_t g8=((c>> 8)&0xFF); g8=g8>127?255:g8*2;
      uint8_t b8=( c     &0xFF); b8=b8>127?255:b8*2;
      matrix.setPixel(br, bc+TET_COL_OFF, ((uint32_t)r8<<16)|((uint32_t)g8<<8)|b8);
    }
  }

  // Level-Anzeige
  uint8_t lvlShow = (_level > 4) ? 5 : _level + 1;
  for (uint8_t i = 0; i < lvlShow && i < 5; i++)
    matrix.setPixel(0, i*2, 0x004040);

  matrix.show();
}

bool TetrisGame::isOver() const { return _gameOver; }
