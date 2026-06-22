#ifndef META_GAME_H
#define META_GAME_H

#include "IGame.h"
#include "GameBoard.h"

class MetaGame : public IGame {
  public:
    MetaGame();
    void reset()                                      override;
    void handleInput(int8_t dx, int8_t dy, bool btn) override;
    void draw(LEDMatrix& matrix)                      override;
    bool isOver() const                               override;
    void playEndAnimation(LEDMatrix& matrix);
  private:
    GameState _state;
    int  _checkBoard(int8_t b[3][3], int8_t outWinLine[][2]);
    bool _miniPlayable(int8_t gr, int8_t gc);
    void _snapCursorToFree(int8_t gr, int8_t gc);
    void _updateForced(int8_t movedMR, int8_t movedMC);
    void _moveCursor(int dr, int dc);
    bool _makeMove();
};
#endif
