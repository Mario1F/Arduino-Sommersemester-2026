#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include "IGame.h"
#include <avr/pgmspace.h>

// ============================================================
// ChessGame — vereinfachtes Schach auf 16×32 Matrix
//
// Darstellung:
//   Schachbrett: 8×8 Felder, jedes Feld = 2×2 Pixel
//   Gesamt: 16×16 Pixel, zentriert auf der Matrix (Spalten 8–23)
//
//   Jedes 2×2 Feld:
//     [Figurfarbe][Feldfarbe]
//     [Feldfarbe ][Feldfarbe]
//   → obere linke Ecke = Figur (oder Feldfarbe wenn leer)
//   → Rest = Feldfarbe (hell/dunkel je nach Feld)
//
// Figuren (Farb-Codes, Weiß vs. Schwarz):
//   König   = hellblau/dunkelblau
//   Dame    = gelb/dunkelgelb
//   Turm    = grün/dunkelgrün
//   Läufer  = orange/dunkelorange
//   Springer= lila/dunkelviolett
//   Bauer   = weiß/grau
//
// Steuerung (1 Joystick):
//   Joystick = Cursor bewegen
//   Knopf    = Figur auswählen / Zug ausführen
//
// Spielregeln: vereinfacht (keine Rochade, kein En-passant)
//
// RAM:
//   Brett: 64 × uint8_t = 64 Bytes  (Figur + Farbe gepackt)
//   Sonstige Variablen: ~20 Bytes
//   Gesamt: ~84 Bytes
// ============================================================

#define CHESS_COL_OFF  8   // Schachbrett beginnt bei Spalte 8

// Figur-Codes (Bits 0-2 = Typ, Bit 3 = Farbe: 0=Weiß, 1=Schwarz)
#define CH_EMPTY   0
#define CH_PAWN    1
#define CH_ROOK    2
#define CH_KNIGHT  3
#define CH_BISHOP  4
#define CH_QUEEN   5
#define CH_KING    6
#define CH_BLACK   8  // Bit 3 gesetzt = schwarz

#define CH_TYPE(x)   ((x) & 0x07)
#define CH_COLOR(x)  ((x) >> 3)   // 0=weiß, 1=schwarz

class ChessGame : public IGame {
  public:
    ChessGame();
    void reset()                                      override;
    void handleInput(int8_t dx, int8_t dy, bool btn) override;
    void draw(LEDMatrix& matrix)                      override;
    bool isOver() const                               override;

  private:
    uint8_t _board[8][8];       // Figur an jedem Feld
    int8_t  _cursorR, _cursorC; // Cursor-Position
    int8_t  _selR, _selC;       // Ausgewählte Figur (-1 = keine)
    uint8_t _turn;              // 0 = weiß, 1 = schwarz
    bool    _gameOver;
    bool    _whiteKingAlive, _blackKingAlive;

    // Farben in PROGMEM
    static const uint32_t _colors[16] PROGMEM;

    uint32_t _color(uint8_t idx);

    // Schachlogik
    bool _isValidMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _pawnMove  (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _rookMove  (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _knightMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _bishopMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _queenMove (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _kingMove  (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _pathClear (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    void _drawField (LEDMatrix& matrix, int8_t r, int8_t c);
};

#endif
