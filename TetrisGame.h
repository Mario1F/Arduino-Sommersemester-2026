#ifndef TETRIS_GAME_H
#define TETRIS_GAME_H

#include "IGame.h"
#include <avr/pgmspace.h>

// Spielfeld: 16 Zeilen × 12 Spalten (zentriert, Spalten 10-21)
// Tetrominos: I,O,S,Z,T,L,J (7 Typen, 4 Rotationen)
// 7-Bag-Randomizer: alle 7 Typen kommen bevor sich etwas wiederholt
// Game Over: nur wenn ein neu gespawnter Block sofort blockiert IST
//            (d.h. das Feld ist wirklich bis ganz oben voll)

#define TET_ROWS    16
#define TET_COLS    12
#define TET_COL_OFF 10

class TetrisGame : public IGame {
  public:
    TetrisGame();
    void reset()                                      override;
    void handleInput(int8_t dx, int8_t dy, bool btn) override;
    void draw(LEDMatrix& matrix)                      override;
    bool isOver() const                               override;

  private:
    uint16_t _board[TET_ROWS];
    uint8_t  _rowColor[TET_ROWS];

    int8_t  _pieceR, _pieceC;
    uint8_t _pieceType;
    uint8_t _pieceRot;
    uint8_t _pieceColor;
    uint8_t _nextType;

    // 7-Bag
    uint8_t _bag[7];
    uint8_t _bagPos;

    bool    _gameOver;
    uint8_t _fallTimer;
    uint8_t _fallCount;
    uint8_t _level;
    uint16_t _linesCleared;
    uint8_t _inputTimer;

    static const uint32_t _colors[9] PROGMEM;
    static const int8_t   _pieces[7][4][8] PROGMEM;

    uint32_t _color(uint8_t idx);
    void     _shuffleBag();
    uint8_t  _nextFromBag();
    void     _getBlocks(uint8_t type, uint8_t rot, int8_t out[4][2]);
    bool     _canPlace(int8_t r, int8_t c, uint8_t type, uint8_t rot);
    void     _lockPiece();
    void     _clearLines();
    void     _spawnPiece();
    void     _drawCell(LEDMatrix& matrix, int8_t r, int8_t c, uint8_t colorIdx);
};

#endif
