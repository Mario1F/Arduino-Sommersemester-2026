#include "BlockPuzzleGame.h"
#include <string.h>

// Piece-Bitmaps: jede Zeile = 4 Bit (Spalten 0-3), Höhe in _pieceSizes
const uint8_t BlockPuzzleGame::_pieces[BP_PIECES][4] PROGMEM = {
  {0b0001, 0,      0,      0     },  //  0: 1×1
  {0b0011, 0,      0,      0     },  //  1: 1×2
  {0b0111, 0,      0,      0     },  //  2: 1×3
  {0b0001, 0b0001, 0,      0     },  //  3: 2×1
  {0b0001, 0b0001, 0b0001, 0     },  //  4: 3×1
  {0b0011, 0b0011, 0,      0     },  //  5: 2×2
  {0b0011, 0b0001, 0b0001, 0     },  //  6: L
  {0b0011, 0b0010, 0b0010, 0     },  //  7: J
  {0b0111, 0b0010, 0,      0     },  //  8: T
  {0b0011, 0b0110, 0,      0     },  //  9: S
  {0b0110, 0b0011, 0,      0     },  // 10: Z
  {0b1111, 0,      0,      0     },  // 11: 1×4
};

const uint8_t BlockPuzzleGame::_pieceSizes[BP_PIECES] PROGMEM = {
  1, 1, 1, 2, 3, 2, 3, 3, 2, 2, 2, 1
};

// Index 0-11 = Stückfarben, 12 = Feldhintergrund, 13 = Rahmen
const uint32_t BlockPuzzleGame::_colors[BP_PIECES + 2] PROGMEM = {
  0x404040, 0x004488, 0x008844, 0x884400,
  0x880044, 0x886600, 0xAA0000, 0x0044AA,
  0x008888, 0x006600, 0x660000, 0x444400,
  0x060606,  // 12 Feldhintergrund
  0x181818,  // 13 Rahmen
};

static uint16_t _bpSeed = 1234;

uint8_t BlockPuzzleGame::_randomType() {
  _bpSeed = _bpSeed * 6364 + 1;
  return (_bpSeed >> 4) % BP_PIECES;
}

uint32_t BlockPuzzleGame::_color(uint8_t idx) {
  return pgm_read_dword(&_colors[idx]);
}

uint8_t BlockPuzzleGame::_pieceHeight(uint8_t type) {
  return pgm_read_byte(&_pieceSizes[type]);
}

uint8_t BlockPuzzleGame::_pieceWidth(uint8_t type) {
  uint8_t maxW = 0, h = _pieceHeight(type);
  for (uint8_t r = 0; r < h; r++) {
    uint8_t row = pgm_read_byte(&_pieces[type][r]);
    uint8_t w = 0; while (row >> w) w++;
    if (w > maxW) maxW = w;
  }
  return maxW;
}

BlockPuzzleGame::BlockPuzzleGame() { reset(); }

void BlockPuzzleGame::reset() {
  memset(_board, 0, sizeof(_board));
  _gameOver = false;
  _score    = 0;
  _queueSel = 0;
  _placing  = false;
  _cursorR  = 0;
  _cursorC  = 0;
  _fillQueue();
}

void BlockPuzzleGame::_fillQueue() {
  for (uint8_t i = 0; i < 3; i++) _queue[i] = _randomType();
}

bool BlockPuzzleGame::_canPlace(uint8_t type, int8_t r, int8_t c) {
  uint8_t h = _pieceHeight(type);
  for (uint8_t pr = 0; pr < h; pr++) {
    uint8_t row = pgm_read_byte(&_pieces[type][pr]);
    for (uint8_t pc = 0; pc < 4; pc++) {
      if (!(row & (1 << pc))) continue;
      int8_t br = r + pr, bc = c + pc;
      if (br < 0 || br >= BP_ROWS) return false;
      if (bc < 0 || bc >= BP_COLS) return false;
      if (_board[br] & (1 << bc))  return false;
    }
  }
  return true;
}

void BlockPuzzleGame::_place(uint8_t type, int8_t r, int8_t c) {
  uint8_t h = _pieceHeight(type);
  for (uint8_t pr = 0; pr < h; pr++) {
    uint8_t row = pgm_read_byte(&_pieces[type][pr]);
    for (uint8_t pc = 0; pc < 4; pc++)
      if (row & (1 << pc)) _board[r + pr] |= (1 << (c + pc));
  }
}

uint8_t BlockPuzzleGame::_clearLines() {
  uint8_t cleared = 0;
  uint8_t fullRow = (1 << BP_COLS) - 1;

  // Volle Zeilen löschen
  for (uint8_t r = 0; r < BP_ROWS; r++)
    if (_board[r] == fullRow) { _board[r] = 0; cleared++; }

  // Volle Spalten löschen
  for (uint8_t c = 0; c < BP_COLS; c++) {
    bool full = true;
    for (uint8_t r = 0; r < BP_ROWS; r++)
      if (!(_board[r] & (1 << c))) { full = false; break; }
    if (full) {
      for (uint8_t r = 0; r < BP_ROWS; r++) _board[r] &= ~(1 << c);
      cleared++;
    }
  }
  return cleared;
}

bool BlockPuzzleGame::_anyMovePossible() {
  for (uint8_t qi = 0; qi < 3; qi++) {
    if (_queue[qi] == 0xFF) continue;  // bereits verbraucht
    for (int8_t r = 0; r < BP_ROWS; r++)
      for (int8_t c = 0; c < BP_COLS; c++)
        if (_canPlace(_queue[qi], r, c)) return true;
  }
  return false;
}

void BlockPuzzleGame::handleInput(int8_t dx, int8_t dy, bool btn) {
  if (_gameOver) return;

  if (!_placing) {
    // ── SELECT-Phase: Queue-Block auswählen ──────────────────
    if (dx == -1 && _queueSel > 0) _queueSel--;
    if (dx ==  1 && _queueSel < 2) _queueSel++;

    if (btn) {
      // Block nehmen → in PLACE-Phase wechseln
      uint8_t type = _queue[_queueSel];
      uint8_t h    = _pieceHeight(type);
      uint8_t w    = _pieceWidth(type);
      // Cursor in die Mitte des Feldes, clamp ans Feldende
      _cursorR = (BP_ROWS / 2) - h / 2;
      _cursorC = (BP_COLS / 2) - w / 2;
      if (_cursorR < 0)              _cursorR = 0;
      if (_cursorC < 0)              _cursorC = 0;
      if (_cursorR + h > BP_ROWS)    _cursorR = BP_ROWS - h;
      if (_cursorC + w > BP_COLS)    _cursorC = BP_COLS - w;
      _placing = true;
    }
  } else {
    // ── PLACE-Phase: Block im Feld positionieren ─────────────
    uint8_t type = _queue[_queueSel];
    uint8_t h    = _pieceHeight(type);
    uint8_t w    = _pieceWidth(type);

    if (dx == -1 && _cursorC > 0)           _cursorC--;
    if (dx ==  1 && _cursorC + w < BP_COLS) _cursorC++;
    if (dy == -1 && _cursorR > 0)           _cursorR--;
    if (dy ==  1 && _cursorR + h < BP_ROWS) _cursorR++;

    if (btn) {
      if (_canPlace(type, _cursorR, _cursorC)) {
        // Block platzieren
        _place(type, _cursorR, _cursorC);
        uint8_t cleared = _clearLines();
        _score += cleared * 10 + 1;

        // Diesen Slot mit neuem Typ befüllen
        _queue[_queueSel] = _randomType();

        // Zurück zu SELECT
        _placing = false;

        // Nächsten verfügbaren Slot auswählen
        for (uint8_t i = 0; i < 3; i++) {
          _queueSel = (_queueSel + 1) % 3;
          break;
        }

        if (!_anyMovePossible()) _gameOver = true;
      }
      // Wenn nicht platzierbar: nichts tun (Block bleibt in der Hand)
    }
  }
}

// Zeichnet ein Piece-Bitmap an eine Pixel-Position auf der Matrix
void BlockPuzzleGame::_drawPieceAt(LEDMatrix& matrix, uint8_t type,
                                    int8_t pixRow, int8_t pixCol, uint32_t color) {
  uint8_t h = _pieceHeight(type);
  for (uint8_t pr = 0; pr < h; pr++) {
    uint8_t row = pgm_read_byte(&_pieces[type][pr]);
    for (uint8_t pc = 0; pc < 4; pc++)
      if (row & (1 << pc))
        matrix.setPixel(pixRow + pr, pixCol + pc, color);
  }
}

void BlockPuzzleGame::draw(LEDMatrix& matrix) {
  matrix.clear();

  // ── Rahmen ums Spielfeld ─────────────────────────────────
  uint32_t frameCol = _color(13);
  for (int r = BP_ROW_OFF - 1; r <= BP_ROW_OFF + BP_ROWS; r++) {
    matrix.setPixel(r, BP_COL_OFF - 1,        frameCol);
    matrix.setPixel(r, BP_COL_OFF + BP_COLS,  frameCol);
  }
  for (int c = BP_COL_OFF - 1; c <= BP_COL_OFF + BP_COLS; c++) {
    matrix.setPixel(BP_ROW_OFF - 1,       c, frameCol);
    matrix.setPixel(BP_ROW_OFF + BP_ROWS, c, frameCol);
  }

  // ── Spielfeld ────────────────────────────────────────────
  for (uint8_t r = 0; r < BP_ROWS; r++)
    for (uint8_t c = 0; c < BP_COLS; c++)
      matrix.setPixel(r + BP_ROW_OFF, c + BP_COL_OFF,
                      (_board[r] & (1 << c)) ? 0x303030 : _color(12));

  // ── Vorschau-Cursor im Spielfeld (PLACE-Phase) ───────────
  if (_placing && !_gameOver) {
    uint8_t type = _queue[_queueSel];
    bool    canP = _canPlace(type, _cursorR, _cursorC);
    uint32_t bc  = canP ? _color(type) : 0x300000;
    uint8_t h    = _pieceHeight(type);
    for (uint8_t pr = 0; pr < h; pr++) {
      uint8_t row = pgm_read_byte(&_pieces[type][pr]);
      for (uint8_t pc = 0; pc < 4; pc++) {
        if (!(row & (1 << pc))) continue;
        int8_t br = _cursorR + pr, bcc = _cursorC + pc;
        if (br >= 0 && br < BP_ROWS && bcc >= 0 && bcc < BP_COLS)
          matrix.setPixel(br + BP_ROW_OFF, bcc + BP_COL_OFF, bc);
      }
    }
  }

  // ── Queue-Anzeige unten (Zeilen 11-15) ───────────────────
  // Trennlinie
  for (int c = 0; c < 24; c++) matrix.setPixel(10, c, 0x0A0A0A);

  // 3 Queue-Felder: je 8 Spalten breit (0-7, 8-15, 16-23)
  for (uint8_t qi = 0; qi < 3; qi++) {
    uint8_t  type   = _queue[qi];
    bool     isSel  = (qi == _queueSel);
    bool     isHeld = (isSel && _placing);

    int8_t   slotCol = qi * 8;  // linke Kante des Slots

    // Hintergrund des Slots
    uint32_t bg = isSel ? 0x0A0A06 : 0x040404;
    for (int8_t r = 11; r < 16; r++)
      for (int8_t c = slotCol; c < slotCol + 7; c++)
        matrix.setPixel(r, c, bg);

    // Auswahl-Indicator (helle Linie oben im Slot)
    if (isSel) {
      uint32_t indCol = isHeld ? 0x004400 : 0x440044;
      for (int8_t c = slotCol; c < slotCol + 7; c++)
        matrix.setPixel(11, c, indCol);
    }

    // Block zentriert im Slot zeichnen (Zeilen 12-15)
    uint8_t  h = _pieceHeight(type);
    uint8_t  w = _pieceWidth(type);
    int8_t   startR = 12 + (3 - (int8_t)h) / 2;
    int8_t   startC = slotCol + (7 - (int8_t)w) / 2;

    uint32_t pieceColor = _color(type);
    if (!isSel) {
      // Nicht ausgewählte Blöcke gedimmt
      uint8_t R = ((pieceColor>>16)&0xFF)/3;
      uint8_t G = ((pieceColor>> 8)&0xFF)/3;
      uint8_t B = ( pieceColor     &0xFF)/3;
      pieceColor = ((uint32_t)R<<16)|((uint32_t)G<<8)|B;
    }

    _drawPieceAt(matrix, type, startR, startC, pieceColor);
  }

  // Trennlinien zwischen Queue-Slots
  for (int8_t r = 10; r < 16; r++) {
    matrix.setPixel(r, 7,  0x0A0A0A);
    matrix.setPixel(r, 15, 0x0A0A0A);
  }

  matrix.show();
}

bool BlockPuzzleGame::isOver() const { return _gameOver; }
