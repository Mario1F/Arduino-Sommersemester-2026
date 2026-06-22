#include "LEDMatrix.h"

LEDMatrix::LEDMatrix(uint8_t pin, uint8_t brightness)
  : _strip(NUM_LEDS, pin, NEO_GRB + NEO_KHZ800)
{
  _strip.setBrightness(brightness);
}

void LEDMatrix::begin() {
  _strip.begin();
  _strip.clear();
  _strip.show();
}

int LEDMatrix::_getLedIndex(int row, int col) {
  const int ledsPerCol   = 8;
  const int ledsPerStrip = 256;
  if (row < 8) {
    if (col % 2 == 0) return col * ledsPerCol + row;
    else              return col * ledsPerCol + (7 - row);
  } else {
    int localRow   = row - 8;
    int flippedCol = 31 - col;
    if (flippedCol % 2 == 0) return ledsPerStrip + flippedCol * ledsPerCol + (7 - localRow);
    else                     return ledsPerStrip + flippedCol * ledsPerCol + localRow;
  }
}

void LEDMatrix::setPixel(int row, int col, uint32_t color) {
  if (row < 0 || row >= MATRIX_ROWS) return;
  if (col < 0 || col >= MATRIX_COLS) return;
  int idx = _getLedIndex(row, col);
  if (idx < 0 || idx >= NUM_LEDS) return;
  _strip.setPixelColor(idx, color);
}

void LEDMatrix::setRow(int row, uint32_t color) {
  if (row < 0 || row >= MATRIX_ROWS) return;
  for (int col = 0; col < MATRIX_COLS; col++) setPixel(row, col, color);
}

void LEDMatrix::setCol(int col, uint32_t color) {
  if (col < 0 || col >= MATRIX_COLS) return;
  for (int row = 0; row < MATRIX_ROWS; row++) setPixel(row, col, color);
}

void LEDMatrix::clear()                  { _strip.clear(); }
void LEDMatrix::show()                   { _strip.show(); }
void LEDMatrix::setBrightness(uint8_t b) { _strip.setBrightness(b); }
uint32_t LEDMatrix::rgb(uint8_t r, uint8_t g, uint8_t b) { return _strip.Color(r,g,b); }
Adafruit_NeoPixel& LEDMatrix::getStrip() { return _strip; }
