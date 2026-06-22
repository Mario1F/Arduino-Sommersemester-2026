#include "GameManager.h"

#define END_DISPLAY_MS 2000

// Abstand zwischen Icon-Mittelpunkten (Pixel)
#define ICON_STRIDE    8

// Bildschirm-Mittelpunkt, auf den das ausgewählte Icon zentriert wird
#define SCREEN_CENTER  15

static const uint32_t MENU_COLORS[GAME_COUNT] = {
  0x8800AA,  // Meta-TicTacToe  lila
  0x0055CC,  // Tetris           blau
  0xAA8800,  // Chess            gold
  0x006600,  // BlockPuzzle      grün
  0x00AA00,  // Snake            hellgrün
};

GameManager::GameManager(LEDMatrix& matrix)
  : _matrix(matrix), _game(nullptr)
  , _currentGame(GAME_META_TTT), _playing(false), _menuCursor(0)
  , _scrollTarget(0), _scrollCurrent(0)
{}

void GameManager::begin() { _playing = false; }

void GameManager::update(int8_t dx, int8_t dy, bool btn) {
  if (!_playing) {
    if (dx == -1 && _menuCursor > 0) {
      _menuCursor--;
      _scrollTarget = (int16_t)_menuCursor * ICON_STRIDE * 4;
    }
    if (dx ==  1 && _menuCursor < GAME_COUNT - 1) {
      _menuCursor++;
      _scrollTarget = (int16_t)_menuCursor * ICON_STRIDE * 4;
    }
    if (btn) { _loadGame((GameID)_menuCursor); _playing = true; return; }
    _drawMenu();
  } else {
    _game->handleInput(dx, dy, btn);
    _game->draw(_matrix);
    if (_game->isOver()) {
      if (_currentGame == GAME_META_TTT)
        static_cast<MetaGame*>(_game)->playEndAnimation(_matrix);
      delay(END_DISPLAY_MS);
      _unloadGame();
      _playing = false;
    }
  }
}

void GameManager::_loadGame(GameID id) {
  _currentGame = id;
  switch (id) {
    case GAME_META_TTT:    _game = new (&_buf.metaGame)        MetaGame();        break;
    case GAME_TETRIS:      _game = new (&_buf.tetrisGame)      TetrisGame();      break;
    case GAME_CHESS:       _game = new (&_buf.chessGame)       ChessGame();       break;
    case GAME_BLOCKPUZZLE: _game = new (&_buf.blockPuzzleGame) BlockPuzzleGame(); break;
    case GAME_SNAKE:       _game = new (&_buf.snakeGame)       SnakeGame();       break;
    default:               _game = nullptr; return;
  }
}

void GameManager::_unloadGame() {
  if (_game != nullptr) { _game->~IGame(); _game = nullptr; }
}

void GameManager::resetCurrentGame() {
  if (_playing && _game != nullptr) {
    _game->reset();
  } else {
    // Im Menü: Menü-Cursor zurücksetzen
    _menuCursor    = 0;
    _scrollTarget  = 0;
    _scrollCurrent = 0;
  }
}

// Zeichnet ein einzelnes Icon zentriert auf screenCol
void GameManager::_drawIcon(uint8_t idx, int8_t cc) {
  bool     sel   = (idx == _menuCursor);
  uint32_t color = MENU_COLORS[idx];
  uint8_t  dim   = sel ? 1 : 5;
  uint8_t  R = ((color>>16)&0xFF)/dim;
  uint8_t  G = ((color>> 8)&0xFF)/dim;
  uint8_t  B = ( color     &0xFF)/dim;
  uint32_t dc = ((uint32_t)R<<16)|((uint32_t)G<<8)|B;

  // Auswahl-Balken oben
  if (sel)
    for (int8_t c = cc-2; c <= cc+2; c++) _matrix.setPixel(2, c, color);

  switch (idx) {
    case 0: // Meta-TicTacToe: 3×3 Gitter
      for (int8_t r = 4; r <= 10; r++)
        for (int8_t c = cc-3; c <= cc+3; c++)
          if ((r % 3 == 1) || ((c - cc + 3) % 3 == 0))
            _matrix.setPixel(r, c, dc);
      break;

    case 1: // Tetris: T-Tetromino
      _matrix.setPixel(5, cc,   dc);
      _matrix.setPixel(6, cc-1, dc);
      _matrix.setPixel(6, cc,   dc);
      _matrix.setPixel(6, cc+1, dc);
      _matrix.setPixel(7, cc-1, dc);
      _matrix.setPixel(7, cc+1, dc);
      _matrix.setPixel(8, cc-1, dc);
      _matrix.setPixel(8, cc+1, dc);
      _matrix.setPixel(9, cc-1, dc);
      _matrix.setPixel(9, cc,   dc);
      _matrix.setPixel(9, cc+1, dc);
      break;

    case 2: // Chess: Schachbrett 4×4
      for (int8_t r = 5; r <= 10; r++)
        for (int8_t c = cc-2; c <= cc+2; c++)
          _matrix.setPixel(r, c, ((r+c)%2==0) ? dc : (dc>>2 & 0x3F3F3F));
      break;

    case 3: // BlockPuzzle: zwei 2×2 Blöcke + Einzelblock
      _matrix.setPixel(5, cc-1, dc); _matrix.setPixel(5, cc,   dc);
      _matrix.setPixel(6, cc-1, dc); _matrix.setPixel(6, cc,   dc);
      _matrix.setPixel(7, cc+1, dc); _matrix.setPixel(7, cc+2, dc);
      _matrix.setPixel(8, cc+1, dc); _matrix.setPixel(8, cc+2, dc);
      _matrix.setPixel(9, cc-1, dc);
      break;

    case 4: // Snake: gewundene Schlange + Apfel
      // Schlangenkörper
      _matrix.setPixel(5, cc-2, dc);
      _matrix.setPixel(6, cc-2, dc); _matrix.setPixel(6, cc-1, dc); _matrix.setPixel(6, cc, dc);
      _matrix.setPixel(7, cc,   dc);
      _matrix.setPixel(8, cc,   dc); _matrix.setPixel(8, cc-1, dc); _matrix.setPixel(8, cc-2, dc);
      _matrix.setPixel(9, cc-2, dc);
      // Kopf (heller)
      _matrix.setPixel(5, cc-1, color); _matrix.setPixel(5, cc, color);
      // Apfel
      _matrix.setPixel(5, cc+2, sel ? 0xFF2000 : 0x3F0800);
      _matrix.setPixel(6, cc+2, sel ? 0xCC1800 : 0x300600);
      break;
  }
}

void GameManager::_drawMenu() {
  // Scroll-Animation: schrittweise annähern (läuft bei jedem draw()-Aufruf)
  if (_scrollCurrent < _scrollTarget) {
    _scrollCurrent += 4;
    if (_scrollCurrent > _scrollTarget) _scrollCurrent = _scrollTarget;
  } else if (_scrollCurrent > _scrollTarget) {
    _scrollCurrent -= 4;
    if (_scrollCurrent < _scrollTarget) _scrollCurrent = _scrollTarget;
  }

  // Pixel-Offset = scrollCurrent / 4
  int16_t offset = _scrollCurrent >> 2;

  _matrix.clear();

  // Kopfleiste
  for (int c = 0; c < MATRIX_COLS; c++) _matrix.setPixel(0, c, 0x100010);

  // Alle Icons zeichnen (auch teilweise außerhalb sichtbar)
  for (uint8_t i = 0; i < GAME_COUNT; i++) {
    // Absolute Position des Icon-Mittelpunkts im virtuellen Raum
    int16_t iconCenter = (int16_t)i * ICON_STRIDE;
    // Auf dem Bildschirm: zentriert = SCREEN_CENTER, verschoben um offset
    int8_t  screenCol  = (int8_t)(SCREEN_CENTER + iconCenter - offset);

    // Nur zeichnen wenn Icon (±4 Pixel) überhaupt auf dem Bildschirm liegt
    if (screenCol < -4 || screenCol > MATRIX_COLS + 4) continue;

    _drawIcon(i, screenCol);
  }

  // Scrollbar unten: zeigt Position (Punkt blinkt)
  // Kleine Dots für jedes Spiel, aktives leuchtet auf
  for (uint8_t i = 0; i < GAME_COUNT; i++) {
    uint32_t dotColor = (i == _menuCursor)
      ? ((millis()/300)%2==0 ? 0x00CCCC : 0x004444)
      : 0x080808;
    int8_t dotCol = 14 + (int8_t)i * 2;  // Dots bei Spalten 14,16,18,20,22
    _matrix.setPixel(15, dotCol, dotColor);
  }

  _matrix.show();
}
