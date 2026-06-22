#ifndef BLOCK_PUZZLE_GAME_H
#define BLOCK_PUZZLE_GAME_H

#include "IGame.h"
#include <avr/pgmspace.h>

// ============================================================
// BlockPuzzleGame (Block Blast)
//
// Layout auf 16×32 Matrix:
//   Zeilen 0-7:  Spielfeld 8×8 (ab Spalte 12)
//   Zeile 12-15: Queue-Anzeige — 3 Blöcke nebeneinander
//                Jeder Block hat einen Cursor-Bereich
//
// Steuerung:
//   Phase SELECT: Joystick links/rechts = Block in Queue wählen
//                 Knopf = ausgewählten Block nehmen (-> Phase PLACE)
//   Phase PLACE:  Joystick = Cursor im Spielfeld bewegen
//                 Knopf = Block platzieren (-> Phase SELECT)
//
// Reset-Taster D2: Doppelklick innerhalb 5s = Neustart
// ============================================================

#define BP_ROWS      8
#define BP_COLS      8
#define BP_ROW_OFF   0   // Spielfeld beginnt bei Zeile 0
#define BP_COL_OFF  12   // Spielfeld beginnt bei Spalte 12
#define BP_PIECES   12

// Queue-Display: 3 Blöcke in Zeilen 11-15, je 8 Spalten breit
// Block 0: Spalten 0-7, Block 1: Spalten 8-15, Block 2: Spalten 16-23
#define BP_QUEUE_ROW  11
#define BP_QUEUE_W     8

class BlockPuzzleGame : public IGame {
  public:
    BlockPuzzleGame();
    void reset()                                      override;
    void handleInput(int8_t dx, int8_t dy, bool btn) override;
    void draw(LEDMatrix& matrix)                      override;
    bool isOver() const                               override;

  private:
    uint8_t  _board[BP_ROWS];
    uint8_t  _queue[3];
    uint8_t  _queueSel;    // welcher Block in Queue ausgewählt (0-2)
    bool     _placing;     // false=SELECT-Phase, true=PLACE-Phase
    int8_t   _cursorR, _cursorC;
    bool     _gameOver;
    uint16_t _score;

    static const uint8_t  _pieces[BP_PIECES][4]    PROGMEM;
    static const uint8_t  _pieceSizes[BP_PIECES]   PROGMEM;
    static const uint32_t _colors[BP_PIECES + 2]   PROGMEM;

    uint32_t _color(uint8_t idx);
    uint8_t  _pieceHeight(uint8_t type);
    uint8_t  _pieceWidth(uint8_t type);
    bool     _canPlace(uint8_t type, int8_t r, int8_t c);
    void     _place(uint8_t type, int8_t r, int8_t c);
    uint8_t  _clearLines();
    uint8_t  _randomType();
    void     _fillQueue();
    bool     _anyMovePossible();
    void     _drawPieceAt(LEDMatrix& matrix, uint8_t type, int8_t pixRow, int8_t pixCol,
                          uint32_t color);
};

#endif
