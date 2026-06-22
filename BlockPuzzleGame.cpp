#include "BlockPuzzleGame.h"
#include <string.h>

const uint8_t BlockPuzzleGame::_pieces[BP_PIECES][4] PROGMEM = {
  {0b0001,0,      0,      0     },
  {0b0011,0,      0,      0     },
  {0b0111,0,      0,      0     },
  {0b0001,0b0001, 0,      0     },
  {0b0001,0b0001, 0b0001, 0     },
  {0b0011,0b0011, 0,      0     },
  {0b0011,0b0001, 0b0001, 0     },
  {0b0011,0b0010, 0b0010, 0     },
  {0b0111,0b0010, 0,      0     },
  {0b0011,0b0110, 0,      0     },
  {0b0110,0b0011, 0,      0     },
  {0b1111,0,      0,      0     },
};

const uint8_t BlockPuzzleGame::_pieceSizes[BP_PIECES] PROGMEM = {
  1,1,1,2,3,2,3,3,2,2,2,1
};

// 0-11=Stückfarben, 12=Feldhintergrund, 13=Rahmen (gelb)
const uint32_t BlockPuzzleGame::_colors[BP_PIECES + 2] PROGMEM = {
  0x404040, 0x004488, 0x008844, 0x884400,
  0x880044, 0x886600, 0xAA0000, 0x0044AA,
  0x008888, 0x006600, 0x660000, 0x444400,
  0x060606,  // 12 Feldhintergrund
  0x886600,  // 13 Rahmen: gelb
};

uint8_t BlockPuzzleGame::_randomType() {
  _seed = _seed * 6364 + 1;
  return (_seed >> 4) % BP_PIECES;
}

uint32_t BlockPuzzleGame::_color(uint8_t idx) {
  return pgm_read_dword(&_colors[idx]);
}

uint8_t BlockPuzzleGame::_pieceHeight(uint8_t type) {
  return pgm_read_byte(&_pieceSizes[type]);
}

uint8_t BlockPuzzleGame::_pieceWidth(uint8_t type) {
  uint8_t maxW=0, h=_pieceHeight(type);
  for (uint8_t r=0;r<h;r++) {
    uint8_t row=pgm_read_byte(&_pieces[type][r]);
    uint8_t w=0; while(row>>w) w++;
    if(w>maxW) maxW=w;
  }
  return maxW;
}

BlockPuzzleGame::BlockPuzzleGame() { reset(); }

void BlockPuzzleGame::reset() {
  memset(_board, 0, sizeof(_board));
  _gameOver     = false;
  _gameOverAnim = 0;
  _score        = 0;
  _queueSel     = 0;
  _placing      = false;
  _cursorR      = 0;
  _cursorC      = 0;
  _seed         = (uint16_t)(millis() & 0xFFFF) | 1;
  _fillQueue();
}

void BlockPuzzleGame::_fillQueue() {
  for (uint8_t i=0;i<3;i++) _queue[i]=_randomType();
}

bool BlockPuzzleGame::_canPlace(uint8_t type, int8_t r, int8_t c) {
  uint8_t h=_pieceHeight(type);
  for (uint8_t pr=0;pr<h;pr++) {
    uint8_t row=pgm_read_byte(&_pieces[type][pr]);
    for (uint8_t pc=0;pc<4;pc++) {
      if (!(row&(1<<pc))) continue;
      int8_t br=r+pr, bc=c+pc;
      if (br<0||br>=BP_ROWS||bc<0||bc>=BP_COLS) return false;
      if (_board[br]&(1<<bc)) return false;
    }
  }
  return true;
}

void BlockPuzzleGame::_place(uint8_t type, int8_t r, int8_t c) {
  uint8_t h=_pieceHeight(type);
  for (uint8_t pr=0;pr<h;pr++) {
    uint8_t row=pgm_read_byte(&_pieces[type][pr]);
    for (uint8_t pc=0;pc<4;pc++)
      if (row&(1<<pc)) _board[r+pr]|=(1<<(c+pc));
  }
}

uint8_t BlockPuzzleGame::_clearLines() {
  uint8_t cleared=0;
  uint8_t fullRow=(1<<BP_COLS)-1;
  for (uint8_t r=0;r<BP_ROWS;r++)
    if (_board[r]==fullRow) { _board[r]=0; cleared++; }
  for (uint8_t c=0;c<BP_COLS;c++) {
    bool full=true;
    for (uint8_t r=0;r<BP_ROWS;r++)
      if (!(_board[r]&(1<<c))) { full=false; break; }
    if (full) { for(uint8_t r=0;r<BP_ROWS;r++) _board[r]&=~(1<<c); cleared++; }
  }
  return cleared;
}

bool BlockPuzzleGame::_anyMovePossible() {
  for (uint8_t qi=0;qi<3;qi++)
    for (int8_t r=0;r<BP_ROWS;r++)
      for (int8_t c=0;c<BP_COLS;c++)
        if (_canPlace(_queue[qi],r,c)) return true;
  return false;
}

void BlockPuzzleGame::handleInput(int8_t dx, int8_t dy, bool btn) {
  if (_gameOver) return;

  if (!_placing) {
    if (dx==-1 && _queueSel>0) _queueSel--;
    if (dx== 1 && _queueSel<2) _queueSel++;
    if (btn) {
      uint8_t type=_queue[_queueSel];
      uint8_t h=_pieceHeight(type), w=_pieceWidth(type);
      _cursorR=(BP_ROWS/2)-h/2; _cursorC=(BP_COLS/2)-w/2;
      if (_cursorR<0) _cursorR=0;
      if (_cursorC<0) _cursorC=0;
      if (_cursorR+h>BP_ROWS) _cursorR=BP_ROWS-h;
      if (_cursorC+w>BP_COLS) _cursorC=BP_COLS-w;
      _placing=true;
    }
  } else {
    uint8_t type=_queue[_queueSel];
    uint8_t h=_pieceHeight(type), w=_pieceWidth(type);
    if (dx==-1 && _cursorC>0)           _cursorC--;
    if (dx== 1 && _cursorC+w<BP_COLS)   _cursorC++;
    if (dy==-1 && _cursorR>0)           _cursorR--;
    if (dy== 1 && _cursorR+h<BP_ROWS)   _cursorR++;
    if (btn) {
      if (_canPlace(type,_cursorR,_cursorC)) {
        _place(type,_cursorR,_cursorC);
        uint8_t cleared=_clearLines();
        _score+=cleared*10+1;
        _queue[_queueSel]=_randomType();
        _placing=false;
        _queueSel=(_queueSel+1)%3;
        if (!_anyMovePossible()) _gameOver=true;
      }
    }
  }
}

void BlockPuzzleGame::_drawPieceAt(LEDMatrix& matrix, uint8_t type,
                                    int8_t pixRow, int8_t pixCol, uint32_t color) {
  uint8_t h=_pieceHeight(type);
  for (uint8_t pr=0;pr<h;pr++) {
    uint8_t row=pgm_read_byte(&_pieces[type][pr]);
    for (uint8_t pc=0;pc<4;pc++)
      if (row&(1<<pc))
        matrix.setPixel(pixRow+pr, pixCol+pc, color);
  }
}

void BlockPuzzleGame::draw(LEDMatrix& matrix) {
  matrix.clear();

  // ── Rahmen: gelb, heller (BP_ROW_OFF=1 → oben Platz) ────
  uint32_t frameCol = 0xAA8800;  // helles Gelb
  for (int r=BP_ROW_OFF-1; r<=BP_ROW_OFF+BP_ROWS; r++) {
    matrix.setPixel(r, BP_COL_OFF-1,       frameCol);
    matrix.setPixel(r, BP_COL_OFF+BP_COLS, frameCol);
  }
  for (int c=BP_COL_OFF-1; c<=BP_COL_OFF+BP_COLS; c++) {
    matrix.setPixel(BP_ROW_OFF-1,       c, frameCol);
    matrix.setPixel(BP_ROW_OFF+BP_ROWS, c, frameCol);
  }

  // ── Spielfeld ────────────────────────────────────────────
  for (uint8_t r=0;r<BP_ROWS;r++)
    for (uint8_t c=0;c<BP_COLS;c++)
      matrix.setPixel(r+BP_ROW_OFF, c+BP_COL_OFF,
                      (_board[r]&(1<<c)) ? 0x303030 : _color(12));

  // ── Verlier-Animation: Feld von oben nach unten rot füllen
  if (_gameOver) {
    if (_gameOverAnim < BP_ROWS) _gameOverAnim++;
    for (uint8_t r=0; r<_gameOverAnim; r++) {
      uint32_t col=(r%2==0)?0x440000:0x200000;
      for (uint8_t c=0;c<BP_COLS;c++)
        matrix.setPixel(r+BP_ROW_OFF, c+BP_COL_OFF, col);
    }
    matrix.show();
    return;
  }

  // ── Vorschau im Spielfeld (PLACE-Phase) ──────────────────
  if (_placing) {
    uint8_t type=_queue[_queueSel];
    bool canP=_canPlace(type,_cursorR,_cursorC);
    uint32_t bc=canP?_color(type):0x300000;
    uint8_t h=_pieceHeight(type);
    for (uint8_t pr=0;pr<h;pr++) {
      uint8_t row=pgm_read_byte(&_pieces[type][pr]);
      for (uint8_t pc=0;pc<4;pc++) {
        if (!(row&(1<<pc))) continue;
        int8_t br=_cursorR+pr, bcc=_cursorC+pc;
        if (br>=0&&br<BP_ROWS&&bcc>=0&&bcc<BP_COLS)
          matrix.setPixel(br+BP_ROW_OFF, bcc+BP_COL_OFF, bc);
      }
    }
  }

  // ── Queue-Anzeige unten (Zeilen 11-15) ───────────────────
  for (int c=0;c<24;c++) matrix.setPixel(10, c, 0x0A0A0A);

  for (uint8_t qi=0;qi<3;qi++) {
    uint8_t  type   = _queue[qi];
    bool     isSel  = (qi==_queueSel);
    bool     isHeld = (isSel && _placing);
    int8_t   slotCol= qi*8;

    // Slot-Hintergrund
    uint32_t bg=isSel?0x0A0A06:0x040404;
    for (int8_t r=11;r<16;r++)
      for (int8_t c=slotCol;c<slotCol+7;c++)
        matrix.setPixel(r, c, bg);

    // Auswahl-Indicator
    if (isSel) {
      uint32_t indCol=isHeld?0x004400:0x444400;
      for (int8_t c=slotCol;c<slotCol+7;c++)
        matrix.setPixel(11, c, indCol);
    }

    // Block zentriert im Slot
    uint8_t h=_pieceHeight(type), w=_pieceWidth(type);
    int8_t startR=12+(3-(int8_t)h)/2;
    int8_t startC=slotCol+(7-(int8_t)w)/2;
    uint32_t pieceColor=_color(type);
    if (!isSel) {
      uint8_t R=((pieceColor>>16)&0xFF)/3;
      uint8_t G=((pieceColor>> 8)&0xFF)/3;
      uint8_t B=( pieceColor     &0xFF)/3;
      pieceColor=((uint32_t)R<<16)|((uint32_t)G<<8)|B;
    }
    _drawPieceAt(matrix, type, startR, startC, pieceColor);
  }

  // Trennlinien
  for (int8_t r=10;r<16;r++) {
    matrix.setPixel(r,  7, 0x0A0A0A);
    matrix.setPixel(r, 15, 0x0A0A0A);
  }

  matrix.show();
}

bool BlockPuzzleGame::isOver() const { return _gameOver; }
