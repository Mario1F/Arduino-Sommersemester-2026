#include "GameBoard.h"

const uint32_t GameBoard::_colors[11] PROGMEM = {
  0x000000, 0x14140A, 0xDC1900, 0x0046DC,
  0xDC1900, 0x0046DC, 0x3C1E00, 0x500A00,
  0x00194F, 0x006400, 0xFFB400,
};

GameBoard::GameBoard(LEDMatrix& matrix) : _matrix(matrix) {}

uint32_t GameBoard::_color(uint8_t idx) {
  return pgm_read_dword(&_colors[idx]);
}

void GameBoard::_getCellPixel(int8_t gr, int8_t gc, int8_t mr, int8_t mc,
                               int& outRow, int& outCol) {
  outRow = BOARD_START_ROW + gr * 4 + mr;
  outCol = BOARD_START_COL + gc * 4 + mc;
}

void GameBoard::_setCell(int8_t gr, int8_t gc, int8_t mr, int8_t mc, uint32_t color) {
  int r, c;
  _getCellPixel(gr, gc, mr, mc, r, c);
  _matrix.setPixel(r, c, color);
}

void GameBoard::_fillMiniField(int8_t gr, int8_t gc, uint32_t color) {
  for (int8_t mr = 0; mr < 3; mr++)
    for (int8_t mc = 0; mc < 3; mc++)
      _setCell(gr, gc, mr, mc, color);
}

void GameBoard::_drawGridLines() {
  for (int i = 0; i < 2; i++) {
    int lineCol = BOARD_START_COL + (i + 1) * 4 - 1;
    for (int row = BOARD_START_ROW; row < BOARD_START_ROW + 11; row++)
      _matrix.setPixel(row, lineCol, _color(CI_GRID));
    int lineRow = BOARD_START_ROW + (i + 1) * 4 - 1;
    for (int col = BOARD_START_COL; col < BOARD_START_COL + 11; col++)
      _matrix.setPixel(lineRow, col, _color(CI_GRID));
  }
}

void GameBoard::draw(const GameState& state) {
  _matrix.clear();
  _drawGridLines();
  for (int8_t gr = 0; gr < 3; gr++) {
    for (int8_t gc = 0; gc < 3; gc++) {
      bool isForced = (state.forcedGR == gr && state.forcedGC == gc);
      if (state.miniWon[gr][gc] == 1) {
        bool inWin = false;
        if (state.hasWinLine)
          for (int w = 0; w < 3; w++)
            if (state.winCells[w][0] == gr && state.winCells[w][1] == gc) inWin = true;
        _fillMiniField(gr, gc, _color(inWin ? CI_FLASH : CI_WON_P1));
        continue;
      }
      if (state.miniWon[gr][gc] == 2) {
        bool inWin = false;
        if (state.hasWinLine)
          for (int w = 0; w < 3; w++)
            if (state.winCells[w][0] == gr && state.winCells[w][1] == gc) inWin = true;
        _fillMiniField(gr, gc, _color(inWin ? CI_FLASH : CI_WON_P2));
        continue;
      }
      if (state.miniWon[gr][gc] == 3) { _fillMiniField(gr, gc, _color(CI_TIE)); continue; }
      for (int8_t mr = 0; mr < 3; mr++) {
        for (int8_t mc = 0; mc < 3; mc++) {
          int8_t val = state.board[gr][gc][mr][mc];
          uint32_t color;
          if      (val == 1) color = _color(CI_PLAYER1);
          else if (val == 2) color = _color(CI_PLAYER2);
          else if (isForced) color = _color(CI_ACTIVE);
          else               color = _color(CI_OFF);
          _setCell(gr, gc, mr, mc, color);
        }
      }
    }
  }
  if (!state.gameOver) {
    int8_t cGR = state.cursorGR, cGC = state.cursorGC;
    int8_t cMR = state.cursorMR, cMC = state.cursorMC;
    if (state.miniWon[cGR][cGC] == 0 && state.board[cGR][cGC][cMR][cMC] == 0) {
      uint8_t cc = (state.currentPlayer == 1) ? CI_CURSOR_P1 : CI_CURSOR_P2;
      _setCell(cGR, cGC, cMR, cMC, _color(cc));
    }
  }
  _matrix.show();
}

void GameBoard::winAnimation(const GameState& state) {
  uint8_t winColorIdx = (state.winner == 1) ? CI_PLAYER1 : CI_PLAYER2;
  for (int i = 0; i < 6; i++) {
    uint8_t colorIdx = (i % 2 == 0) ? CI_FLASH : winColorIdx;
    for (int w = 0; w < 3; w++)
      _fillMiniField(state.winCells[w][0], state.winCells[w][1], _color(colorIdx));
    _matrix.show();
    delay(300);
  }
  for (int w = 0; w < 3; w++)
    _fillMiniField(state.winCells[w][0], state.winCells[w][1], _color(CI_OFF));
  _matrix.show();
  delay(200);
  draw(state);
}

void GameBoard::tieAnimation() {
  const int8_t order[9][2] = {
    {0,0},{0,1},{0,2},{1,0},{1,1},{1,2},{2,0},{2,1},{2,2}
  };
  _matrix.clear();
  _drawGridLines();
  for (int i = 0; i < 9; i++) {
    _fillMiniField(order[i][0], order[i][1], _color(CI_TIE));
    _matrix.show();
    delay(120);
  }
  for (int i = 0; i < 4; i++) {
    _matrix.clear(); _drawGridLines(); _matrix.show(); delay(200);
    for (int8_t gr = 0; gr < 3; gr++)
      for (int8_t gc = 0; gc < 3; gc++)
        _fillMiniField(gr, gc, _color(CI_TIE));
    _drawGridLines(); _matrix.show(); delay(300);
  }
}
