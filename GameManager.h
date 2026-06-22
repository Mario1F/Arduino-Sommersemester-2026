#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "IGame.h"
#include "LEDMatrix.h"
#include "GameBoard.h"
#include "MetaGame.h"
#include "TetrisGame.h"
#include "ChessGame.h"
#include "BlockPuzzleGame.h"
#include "SnakeGame.h"
#include <new>

enum GameID : uint8_t {
  GAME_META_TTT    = 0,
  GAME_TETRIS      = 1,
  GAME_CHESS       = 2,
  GAME_BLOCKPUZZLE = 3,
  GAME_SNAKE       = 4,
  GAME_COUNT       = 5
};

union GameBuffer {
  MetaGame        metaGame;
  TetrisGame      tetrisGame;
  ChessGame       chessGame;
  BlockPuzzleGame blockPuzzleGame;
  SnakeGame       snakeGame;
  GameBuffer()  {}
  ~GameBuffer() {}
};

class GameManager {
  public:
    GameManager(LEDMatrix& matrix);
    void begin();
    void update(int8_t dx, int8_t dy, bool btn);
    void resetCurrentGame();  // D2-Taster: aktuelles Spiel neu starten
  private:
    LEDMatrix& _matrix;
    GameBuffer _buf;
    IGame*     _game;
    GameID     _currentGame;
    bool       _playing;
    uint8_t    _menuCursor;

    // Scroll-Animation
    int16_t    _scrollTarget;  // Ziel-Offset in Pixeln (× 4 für Subpixel-Glättung)
    int16_t    _scrollCurrent; // Aktueller Offset (× 4)

    void _loadGame(GameID id);
    void _unloadGame();
    void _drawMenu();
    void _drawIcon(uint8_t gameIdx, int8_t screenCol);
};

#endif
