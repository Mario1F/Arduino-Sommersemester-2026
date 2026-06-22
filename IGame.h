#ifndef IGAME_H
#define IGAME_H

#include "LEDMatrix.h"

class IGame {
  public:
    virtual ~IGame() {}
    virtual void reset() = 0;
    virtual void handleInput(int8_t dx, int8_t dy, bool btn) = 0;
    virtual void draw(LEDMatrix& matrix) = 0;
    virtual bool isOver() const = 0;
};

#endif
