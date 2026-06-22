#include "ChessGame.h"
#include <string.h>

// Farb-Indizes:
//  0-1:  helles/dunkles Feld
//  6:    Cursor (grün)
//  7:    Auswahl (gelb)
//  8-15: Figurfarben
//
// Pixel-Position im 2x2-Feld:
//   oben links  = Bauer
//   oben rechts = Springer / Laufer
//   unten links = Turm
//   unten rechts= Dame / Konig
const uint32_t ChessGame::_colors[16] PROGMEM = {
  0x1A1208,  //  0 helles Feld  (warmbeige)
  0x0A0804,  //  1 dunkles Feld (dunkelbraun)
  0x000000,  //  2 (reserviert)
  0x000000,  //  3 (reserviert)
  0x000000,  //  4 (reserviert)
  0x000000,  //  5 (reserviert)
  0x003300,  //  6 Cursor (gruen)
  0x505000,  //  7 Auswahl (gelb)
  0xC0C0A0,  //  8 Bauer        weiss -> hellgrau
  0x40C040,  //  9 Turm         weiss -> gruen
  0xC080C0,  // 10 Springer     weiss -> lila
  0xC09040,  // 11 Laufer       weiss -> orange
  0xDC1900,  // 12 Dame         weiss -> rot
  0xFFFFFF,  // 13 Konig        weiss -> weiss
  0x500A00,  // 14 Dame         schwarz -> dunkelrot
  0x555555,  // 15 Konig        schwarz -> grau
};

uint32_t ChessGame::_color(uint8_t idx) {
  return pgm_read_dword(&_colors[idx]);
}

ChessGame::ChessGame() { reset(); }

void ChessGame::reset() {
  memset(_board, CH_EMPTY, sizeof(_board));

  // Schwarze Figuren (Zeile 0-1)
  _board[0][0] = CH_ROOK   | CH_BLACK;
  _board[0][1] = CH_KNIGHT | CH_BLACK;
  _board[0][2] = CH_BISHOP | CH_BLACK;
  _board[0][3] = CH_QUEEN  | CH_BLACK;
  _board[0][4] = CH_KING   | CH_BLACK;
  _board[0][5] = CH_BISHOP | CH_BLACK;
  _board[0][6] = CH_KNIGHT | CH_BLACK;
  _board[0][7] = CH_ROOK   | CH_BLACK;
  for (int8_t c = 0; c < 8; c++) _board[1][c] = CH_PAWN | CH_BLACK;

  // Weiße Figuren (Zeile 6-7)
  for (int8_t c = 0; c < 8; c++) _board[6][c] = CH_PAWN;
  _board[7][0] = CH_ROOK;
  _board[7][1] = CH_KNIGHT;
  _board[7][2] = CH_BISHOP;
  _board[7][3] = CH_QUEEN;
  _board[7][4] = CH_KING;
  _board[7][5] = CH_BISHOP;
  _board[7][6] = CH_KNIGHT;
  _board[7][7] = CH_ROOK;

  _cursorR = 7; _cursorC = 4;
  _selR = -1;   _selC = -1;
  _turn = 0;    // Weiß beginnt
  _gameOver = false;
  _whiteKingAlive = true;
  _blackKingAlive = true;
}

void ChessGame::handleInput(int8_t dx, int8_t dy, bool btn) {
  if (_gameOver) return;

  // Cursor bewegen
  if (dy == -1 && _cursorR > 0) _cursorR--;
  if (dy ==  1 && _cursorR < 7) _cursorR++;
  if (dx == -1 && _cursorC > 0) _cursorC--;
  if (dx ==  1 && _cursorC < 7) _cursorC++;

  if (!btn) return;

  // Knopf: Auswählen oder Zug ausführen
  uint8_t fig = _board[_cursorR][_cursorC];

  if (_selR == -1) {
    // Noch nichts ausgewählt: Figur des aktuellen Spielers auswählen
    if (fig != CH_EMPTY && CH_COLOR(fig) == _turn) {
      _selR = _cursorR; _selC = _cursorC;
    }
  } else {
    // Bereits ausgewählt: Zug versuchen
    if (_cursorR == _selR && _cursorC == _selC) {
      // Gleiche Position → Auswahl aufheben
      _selR = -1; _selC = -1;
    } else if (_isValidMove(_selR, _selC, _cursorR, _cursorC)) {
      // Zug ausführen
      uint8_t captured = _board[_cursorR][_cursorC];

      // König geschlagen?
      if (CH_TYPE(captured) == CH_KING) {
        if (CH_COLOR(captured) == 0) _whiteKingAlive = false;
        else                          _blackKingAlive = false;
        _gameOver = true;
      }

      _board[_cursorR][_cursorC] = _board[_selR][_selC];
      _board[_selR][_selC] = CH_EMPTY;

      // Bauer-Promotion (erreicht gegnerische Grundlinie)
      uint8_t moved = _board[_cursorR][_cursorC];
      if (CH_TYPE(moved) == CH_PAWN) {
        if (_turn == 0 && _cursorR == 0) _board[_cursorR][_cursorC] = CH_QUEEN;
        if (_turn == 1 && _cursorR == 7) _board[_cursorR][_cursorC] = CH_QUEEN | CH_BLACK;
      }

      _selR = -1; _selC = -1;
      if (!_gameOver) _turn = 1 - _turn;
    } else {
      // Ungültiger Zug: eigene Figur neu auswählen
      if (fig != CH_EMPTY && CH_COLOR(fig) == _turn) {
        _selR = _cursorR; _selC = _cursorC;
      } else {
        _selR = -1; _selC = -1;
      }
    }
  }
}

// ── Schachlogik ───────────────────────────────────────────────

bool ChessGame::_pathClear(int8_t fr, int8_t fc, int8_t tr, int8_t tc) {
  int8_t dr = (tr > fr) ? 1 : (tr < fr) ? -1 : 0;
  int8_t dc = (tc > fc) ? 1 : (tc < fc) ? -1 : 0;
  int8_t r = fr + dr, c = fc + dc;
  while (r != tr || c != tc) {
    if (_board[r][c] != CH_EMPTY) return false;
    r += dr; c += dc;
  }
  return true;
}

bool ChessGame::_pawnMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc) {
  uint8_t fig  = _board[fr][fc];
  uint8_t col  = CH_COLOR(fig);
  int8_t  dir  = (col == 0) ? -1 : 1;  // Weiß geht nach oben (Zeile sinkt)
  int8_t  start = (col == 0) ? 6 : 1;

  int8_t dr = tr - fr, dc = tc - fc;

  // Geradeaus: 1 Schritt
  if (dc == 0 && dr == dir && _board[tr][tc] == CH_EMPTY) return true;
  // Geradeaus: 2 Schritte vom Start
  if (dc == 0 && dr == dir*2 && fr == start &&
      _board[fr + dir][fc] == CH_EMPTY && _board[tr][tc] == CH_EMPTY) return true;
  // Schlagen diagonal
  if (abs(dc) == 1 && dr == dir &&
      _board[tr][tc] != CH_EMPTY && CH_COLOR(_board[tr][tc]) != col) return true;
  return false;
}

bool ChessGame::_rookMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc) {
  if (fr != tr && fc != tc) return false;
  return _pathClear(fr, fc, tr, tc);
}

bool ChessGame::_knightMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc) {
  int8_t dr = abs(tr - fr), dc = abs(tc - fc);
  return (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
}

bool ChessGame::_bishopMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc) {
  if (abs(tr - fr) != abs(tc - fc)) return false;
  return _pathClear(fr, fc, tr, tc);
}

bool ChessGame::_queenMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc) {
  return _rookMove(fr, fc, tr, tc) || _bishopMove(fr, fc, tr, tc);
}

bool ChessGame::_kingMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc) {
  return abs(tr - fr) <= 1 && abs(tc - fc) <= 1;
}

bool ChessGame::_isValidMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc) {
  if (tr < 0 || tr > 7 || tc < 0 || tc > 7) return false;
  uint8_t from = _board[fr][fc];
  uint8_t to   = _board[tr][tc];
  if (from == CH_EMPTY) return false;
  // Eigene Figur nicht schlagen
  if (to != CH_EMPTY && CH_COLOR(to) == CH_COLOR(from)) return false;

  switch (CH_TYPE(from)) {
    case CH_PAWN:   return _pawnMove  (fr, fc, tr, tc);
    case CH_ROOK:   return _rookMove  (fr, fc, tr, tc);
    case CH_KNIGHT: return _knightMove(fr, fc, tr, tc);
    case CH_BISHOP: return _bishopMove(fr, fc, tr, tc);
    case CH_QUEEN:  return _queenMove (fr, fc, tr, tc);
    case CH_KING:   return _kingMove  (fr, fc, tr, tc);
    default:        return false;
  }
}

// ── Draw ─────────────────────────────────────────────────────

void ChessGame::_drawField(LEDMatrix& matrix, int8_t r, int8_t c) {
  // 2×2 Pixel pro Feld, Startpixel = (r*2, c*2 + CHESS_COL_OFF)
  int8_t pixR  = r * 2;
  int8_t pixC  = c * 2 + CHESS_COL_OFF;
  bool   light = ((r + c) % 2 == 0);

  // Cursor und Auswahl-Highlight
  bool isCursor = (_cursorR == r && _cursorC == c);
  bool isSel    = (_selR    == r && _selC    == c);
  // Gültiger Zug von Auswahl aus
  bool isTarget = (_selR != -1 && _isValidMove(_selR, _selC, r, c));

  uint32_t fieldColor;
  if      (isSel)    fieldColor = _color(7);           // Auswahl: gelb
  else if (isCursor) fieldColor = _color(6);           // Cursor: grün
  else if (isTarget) fieldColor = light ? 0x002020 : 0x001010;  // Ziel: blaugrün
  else               fieldColor = light ? _color(0) : _color(1);

  // Alle 4 Pixel mit Feldfarbe füllen
  matrix.setPixel(pixR,   pixC,   fieldColor);
  matrix.setPixel(pixR,   pixC+1, fieldColor);
  matrix.setPixel(pixR+1, pixC,   fieldColor);
  matrix.setPixel(pixR+1, pixC+1, fieldColor);

  // Figur zeichnen — Position im 2x2-Feld je nach Typ:
  //   oben links  (pixR,   pixC  ) = Bauer
  //   oben rechts (pixR,   pixC+1) = Springer, Laufer
  //   unten links (pixR+1, pixC  ) = Turm
  //   unten rechts(pixR+1, pixC+1) = Dame, Konig
  uint8_t fig = _board[r][c];
  if (fig != CH_EMPTY) {
    uint8_t  type    = CH_TYPE(fig);
    bool     isWhite = (CH_COLOR(fig) == 0);
    uint32_t figColor;
    int8_t   fpR, fpC;  // Figur-Pixel-Offset

    // Farbe bestimmen
    if (isWhite) {
      switch (type) {
        case CH_PAWN:   figColor = _color(8);  break;
        case CH_ROOK:   figColor = _color(9);  break;
        case CH_KNIGHT: figColor = _color(10); break;
        case CH_BISHOP: figColor = _color(11); break;
        case CH_QUEEN:  figColor = _color(12); break;  // rot
        case CH_KING:   figColor = _color(13); break;  // weiss
        default:        figColor = 0xFFFFFF;
      }
    } else {
      // Schwarz: eigene gedaempfte Farben (1/3 der hellen)
      uint32_t wc;
      switch (type) {
        case CH_PAWN:   wc = _color(8);  break;
        case CH_ROOK:   wc = _color(9);  break;
        case CH_KNIGHT: wc = _color(10); break;
        case CH_BISHOP: wc = _color(11); break;
        case CH_QUEEN:  figColor = _color(14); goto place;  // dunkelrot, direkt
        case CH_KING:   figColor = _color(15); goto place;  // grau, direkt
        default:        wc = 0x404040;
      }
      figColor = (((wc >> 16) & 0xFF) / 3) << 16 |
                 (((wc >>  8) & 0xFF) / 3) <<  8 |
                 (( wc        & 0xFF) / 3);
    }

    place:
    // Position im 2x2-Feld nach Typ
    switch (type) {
      case CH_PAWN:                    fpR = 0; fpC = 0; break;  // oben links
      case CH_KNIGHT: case CH_BISHOP:  fpR = 0; fpC = 1; break;  // oben rechts
      case CH_ROOK:                    fpR = 1; fpC = 0; break;  // unten links
      case CH_QUEEN:  case CH_KING:    fpR = 1; fpC = 1; break;  // unten rechts
      default:                         fpR = 0; fpC = 0; break;
    }

    matrix.setPixel(pixR + fpR, pixC + fpC, figColor);
  }
}

void ChessGame::draw(LEDMatrix& matrix) {
  matrix.clear();

  // Brett zeichnen (8×8 Felder × 2×2 Pixel)
  for (int8_t r = 0; r < 8; r++)
    for (int8_t c = 0; c < 8; c++)
      _drawField(matrix, r, c);

  // Spieler-Anzeige: kleiner Punkt außerhalb des Bretts
  // Weiß = Spalte 7 (links vom Brett), Schwarz = Spalte 24 (rechts)
  uint32_t turnColor = (_turn == 0) ? 0xFFFFFF : 0x003030;
  matrix.setPixel(7, 7,  _turn == 0 ? 0xFFFFFF : 0x0A0A0A);
  matrix.setPixel(7, 24, _turn == 1 ? 0x003030 : 0x0A0A0A);
  (void)turnColor;

  // Game Over: Brett blinkt
  if (_gameOver) {
    static uint8_t b = 0; b++;
    uint32_t flash = _whiteKingAlive ? 0x000020 : 0x200000;
    if (b < 20) {
      for (int r = 0; r < 16; r++)
        for (int c = CHESS_COL_OFF; c < CHESS_COL_OFF + 16; c++)
          matrix.setPixel(r, c, flash);
    }
    if (b > 40) b = 0;
  }

  matrix.show();
}

bool ChessGame::isOver() const { return _gameOver; }
