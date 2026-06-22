#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include <avr/pgmspace.h>
#include "LEDMatrix.h"

#define BOARD_START_ROW  2
#define BOARD_START_COL 10

struct GameState {
  int8_t board[3][3][3][3];
  int8_t miniWon[3][3];
  int8_t cursorGR, cursorGC;
  int8_t cursorMR, cursorMC;
  int8_t forcedGR, forcedGC;
  int8_t currentPlayer;
  int8_t winner;
  int8_t winCells[3][2];
  bool   gameOver;
  bool   hasWinLine;
};

class GameBoard {
  public:
    GameBoard(LEDMatrix& matrix);
    void draw(const GameState& state);
    void winAnimation(const GameState& state);
    void tieAnimation();

  private:
    LEDMatrix& _matrix;
    static const uint32_t _colors[11] PROGMEM;
    static const uint8_t CI_OFF       = 0;
    static const uint8_t CI_GRID      = 1;
    static const uint8_t CI_PLAYER1   = 2;
    static const uint8_t CI_PLAYER2   = 3;
    static const uint8_t CI_CURSOR_P1 = 4;
    static const uint8_t CI_CURSOR_P2 = 5;
    static const uint8_t CI_ACTIVE    = 6;
    static const uint8_t CI_WON_P1    = 7;
    static const uint8_t CI_WON_P2    = 8;
    static const uint8_t CI_TIE       = 9;
    static const uint8_t CI_FLASH     = 10;
    uint32_t _color(uint8_t idx);
    void _getCellPixel(int8_t gr, int8_t gc, int8_t mr, int8_t mc, int& outRow, int& outCol);
    void _setCell(int8_t gr, int8_t gc, int8_t mr, int8_t mc, uint32_t color);
    void _fillMiniField(int8_t gr, int8_t gc, uint32_t color);
    void _drawGridLines();
};

#endif
