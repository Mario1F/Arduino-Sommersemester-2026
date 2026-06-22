#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include "IGame.h"
#include <avr/pgmspace.h>

// Spielfeld: Zeilen 0-15, Spalten 1-30 (Waende auf 0 und 31)
// Oben/unten: Wrap-Around
// Links/rechts: Wand = Tod
// Aepfel: 3 Minimum, 5 Maximum, spawnen mit der Zeit nach

#define SNAKE_MAX_LEN    60
#define SNAKE_MAX_FOOD    5
#define SNAKE_MIN_FOOD    3

#define SNAKE_ROW_MIN    0
#define SNAKE_ROW_MAX   15
#define SNAKE_COL_MIN    1
#define SNAKE_COL_MAX   30

// Ticks zwischen Apfel-Spawns (bei 16ms/Frame = ca. 3s)
#define SNAKE_FOOD_SPAWN_TICKS  180

class SnakeGame : public IGame {
  public:
    SnakeGame();
    void reset()                                      override;
    void handleInput(int8_t dx, int8_t dy, bool btn) override;
    void draw(LEDMatrix& matrix)                      override;
    bool isOver() const                               override;

  private:
    int8_t  _body[SNAKE_MAX_LEN][2];
    int8_t  _food[SNAKE_MAX_FOOD][2];
    bool    _foodActive[SNAKE_MAX_FOOD];   // ist dieser Apfel-Slot aktiv?
    uint8_t _foodCount;                    // aktuell aktive Aepfel
    uint8_t _len;
    int8_t  _dr, _dc;
    int8_t  _nextDr, _nextDc;
    bool    _gameOver;
    bool    _grew;
    uint8_t _moveTimer;
    uint8_t _moveCount;
    uint8_t _spawnTimer;                   // zaehlt hoch bis SNAKE_FOOD_SPAWN_TICKS

    static const uint32_t _colors[6] PROGMEM;
    static const uint8_t C_OFF   = 0;
    static const uint8_t C_WALL  = 1;
    static const uint8_t C_HEAD  = 2;
    static const uint8_t C_BODY  = 3;
    static const uint8_t C_FOOD  = 4;
    static const uint8_t C_FOOD2 = 5;

    uint32_t _color(uint8_t idx);
    void     _spawnFood();
    void     _step();
    bool     _selfCollision();
    bool     _onSnake(int8_t r, int8_t c);
    bool     _onFood(int8_t r, int8_t c);
    uint16_t _rng();
};

#endif
