#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include <Adafruit_NeoPixel.h>

#define MATRIX_ROWS 16
#define MATRIX_COLS 32
#define NUM_LEDS    512

class LEDMatrix {
  public:
    LEDMatrix(uint8_t pin = 6, uint8_t brightness = 20);
    void begin();
    void setPixel(int row, int col, uint32_t color);
    void setRow(int row, uint32_t color);
    void setCol(int col, uint32_t color);
    void clear();
    void show();
    void setBrightness(uint8_t brightness);
    uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);
    Adafruit_NeoPixel& getStrip();
  private:
    Adafruit_NeoPixel _strip;
    int _getLedIndex(int row, int col);
};

#endif
