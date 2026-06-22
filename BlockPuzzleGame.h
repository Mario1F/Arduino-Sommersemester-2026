#ifndef BLOCK_PUZZLE_GAME_H
#define BLOCK_PUZZLE_GAME_H

#include "IGame.h"
#include <avr/pgmspace.h>

#define BP_ROWS      8
#define BP_COLS      8
#define BP_ROW_OFF   1    // Ein Pixel oben frei für Rahmen
#define BP_COL_OFF  12
#define BP_PIECES   12

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
    uint8_t  _queueSel;
    bool     _placing;
    int8_t   _cursorR, _cursorC;
    bool     _gameOver;
    uint8_t  _gameOverAnim;   // Verlier-Animation
    uint16_t _score;
    uint16_t _seed;

    static const uint8_t  _pieces[BP_PIECES][4]  PROGMEM;
    static const uint8_t  _pieceSizes[BP_PIECES] PROGMEM;
    static const uint32_t _colors[BP_PIECES + 2] PROGMEM;

    uint32_t _color(uint8_t idx);
    uint8_t  _pieceHeight(uint8_t type);
    uint8_t  _pieceWidth(uint8_t type);
    bool     _canPlace(uint8_t type, int8_t r, int8_t c);
    void     _place(uint8_t type, int8_t r, int8_t c);
    uint8_t  _clearLines();
    uint8_t  _randomType();
    void     _fillQueue();
    bool     _anyMovePossible();
    void     _drawPieceAt(LEDMatrix& matrix, uint8_t type,
                          int8_t pixRow, int8_t pixCol, uint32_t color);
};

#endif
