#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include "IGame.h"
#include <avr/pgmspace.h>

#define CHESS_COL_OFF  8

#define CH_EMPTY   0
#define CH_PAWN    1
#define CH_ROOK    2
#define CH_KNIGHT  3
#define CH_BISHOP  4
#define CH_QUEEN   5
#define CH_KING    6
#define CH_BLACK   8

#define CH_TYPE(x)   ((x) & 0x07)
#define CH_COLOR(x)  ((x) >> 3)

// Rochade-Bits in _castleRights
#define CR_W_KING  0x01   // Weiß Königsseite
#define CR_W_QUEEN 0x02   // Weiß Damenseite
#define CR_B_KING  0x04   // Schwarz Königsseite
#define CR_B_QUEEN 0x08   // Schwarz Damenseite

// Crazyhouse: Hand-Bitfeld (3 Bit pro Figurtyp, 6 Typen = 18 Bit → 3 Bytes pro Spieler)
// Wir speichern Anzahl jedes Typs 0-7 → 3 Bit reicht
// Layout _hand[spieler]: Bits 0-2=Bauern, 3-5=Türme, 6-8=Springer,
//                         9-11=Läufer, 12-14=Damen (max 2), 15-17=reserviert
// Vereinfacht: uint8_t _hand[2][5] = Anzahl jedes Typs (Pawn/Rook/Knight/Bishop/Queen)
// 2×5 = 10 Bytes — zu groß. Stattdessen: jede Figur max 1 Bit (je hatten wir sie oder nicht)
// → uint8_t _hand[2]: Bits 0-4 = PAWN,ROOK,KNIGHT,BISHOP,QUEEN (1=vorhanden)
// Realistisch: mehrere gleiche Typen möglich, daher uint8_t pro Typ & Spieler
// ABER: 2×5 = 10 Bytes ist noch immer ok (ChessGame bleibt 86 Bytes << 151)

class ChessGame : public IGame {
  public:
    ChessGame();
    void reset()                                      override;
    void setMode(uint8_t mode) { _mode = mode; }
    void handleInput(int8_t dx, int8_t dy, bool btn) override;
    void draw(LEDMatrix& matrix)                      override;
    bool isOver() const                               override;

  private:
    uint8_t _board[8][8];          // 64 Bytes
    int8_t  _cursorR, _cursorC;    //  2
    int8_t  _selR, _selC;          //  2
    uint8_t _turn;                 //  1
    bool    _gameOver;             //  1
    bool    _whiteKingAlive, _blackKingAlive; // 2

    // En passant
    int8_t  _epCol, _epRow;        //  2

    // Three-Check: Schach-Zähler pro Spieler
    uint8_t _checkCount[2];        //  2  (weiß=0, schwarz=1)

    // Rochade: welche Seiten noch erlaubt
    uint8_t _castleRights;         //  1  (Bits: CR_W_KING/QUEEN, CR_B_KING/QUEEN)

    // Crazyhouse: Figuren in der Hand (Anzahl pro Typ, Index=Figurtyp-1)
    // [0]=Bauer [1]=Turm [2]=Springer [3]=Läufer [4]=Dame
    uint8_t _hand[2][5];           // 10

    // Crazyhouse UI: Figur aus Hand einsetzen
    bool    _handMode;             //  1  true = Hand-Auswahl aktiv
    uint8_t _handType;             //  1  ausgewählter Figurtyp (1-5)

    // Spielmodus (alle in einer Klasse, kein extra RAM für neue Varianten nötig)
    // Wird über #define gesetzt → kein RAM-Overhead
    // Modi: 0=Normal, 1=Antichess, 2=ThreeCheck, 3=Crazyhouse
    uint8_t _mode;                 //  1
    // vtable: 2
    // GESAMT: ~92 Bytes << 151 (SnakeGame)

    static const uint32_t _colors[16] PROGMEM;

    uint32_t _color(uint8_t idx);

    // Bewegungslogik
    bool _isValidMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _isValidMoveAntichess(int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _pawnMove  (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _rookMove  (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _knightMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _bishopMove(int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _queenMove (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _kingMove  (int8_t fr, int8_t fc, int8_t tr, int8_t tc);
    bool _pathClear (int8_t fr, int8_t fc, int8_t tr, int8_t tc);

    // Rochade
    bool _canCastle(uint8_t side);   // side: CR_W_KING etc.

    // Three-Check / Antichess Hilfsfunktionen
    bool _isInCheck(uint8_t color);
    bool _canCapture(uint8_t color); // Antichess: gibt es irgendein Schlagzug?

    void _drawField(LEDMatrix& matrix, int8_t r, int8_t c);
    void _drawHandUI(LEDMatrix& matrix);
};

// Konstruktor-Helfer: Modus mitgeben
ChessGame* newChessNormal(void* buf);
ChessGame* newChessAntichess(void* buf);
ChessGame* newChessThreeCheck(void* buf);
ChessGame* newCrazyhouse(void* buf);

#endif
