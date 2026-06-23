#include "ChessGame.h"
#include <string.h>
#include <new>

const uint32_t ChessGame::_colors[16] PROGMEM = {
  0x1A1208, 0x0A0804, 0x000000, 0x000000,
  0x000000, 0x000000, 0x003300, 0x505000,
  0xC0C0A0, 0x40C040, 0xC080C0, 0xC09040,
  0xDC1900, 0xFFFFFF, 0x500A00, 0x555555,
};

uint32_t ChessGame::_color(uint8_t idx) {
  return pgm_read_dword(&_colors[idx]);
}

// ── Konstruktor / Reset ───────────────────────────────────────

ChessGame::ChessGame() { _mode = 0; reset(); }

void ChessGame::reset() {
  memset(_board, CH_EMPTY, sizeof(_board));
  memset(_hand,  0,        sizeof(_hand));

  _board[0][0]=CH_ROOK|CH_BLACK; _board[0][1]=CH_KNIGHT|CH_BLACK;
  _board[0][2]=CH_BISHOP|CH_BLACK; _board[0][3]=CH_QUEEN|CH_BLACK;
  _board[0][4]=CH_KING|CH_BLACK; _board[0][5]=CH_BISHOP|CH_BLACK;
  _board[0][6]=CH_KNIGHT|CH_BLACK; _board[0][7]=CH_ROOK|CH_BLACK;
  for (int8_t c=0;c<8;c++) _board[1][c]=CH_PAWN|CH_BLACK;
  for (int8_t c=0;c<8;c++) _board[6][c]=CH_PAWN;
  _board[7][0]=CH_ROOK; _board[7][1]=CH_KNIGHT; _board[7][2]=CH_BISHOP;
  _board[7][3]=CH_QUEEN; _board[7][4]=CH_KING; _board[7][5]=CH_BISHOP;
  _board[7][6]=CH_KNIGHT; _board[7][7]=CH_ROOK;

  _cursorR=7; _cursorC=4;
  _selR=-1; _selC=-1;
  _turn=0;
  _gameOver=false;
  _whiteKingAlive=true; _blackKingAlive=true;
  _epCol=-1; _epRow=-1;
  _checkCount[0]=0; _checkCount[1]=0;
  _castleRights = CR_W_KING|CR_W_QUEEN|CR_B_KING|CR_B_QUEEN;
  _handMode=false; _handType=CH_PAWN;
}

// Modus-Setter (von außen aufgerufen nach Konstruktor)
ChessGame* newChessNormal(void* buf)     { auto* g=new(buf) ChessGame(); g->setMode(0); return g; }
ChessGame* newChessAntichess(void* buf)  { auto* g=new(buf) ChessGame(); g->setMode(1); return g; }
ChessGame* newChessThreeCheck(void* buf) { auto* g=new(buf) ChessGame(); g->setMode(2); return g; }
ChessGame* newCrazyhouse(void* buf)      { auto* g=new(buf) ChessGame(); g->setMode(3); return g; }

// ── Bewegungslogik ────────────────────────────────────────────

bool ChessGame::_pathClear(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  int8_t dr=(tr>fr)?1:(tr<fr)?-1:0;
  int8_t dc=(tc>fc)?1:(tc<fc)?-1:0;
  int8_t r=fr+dr, c=fc+dc;
  while(r!=tr||c!=tc) {
    if(_board[r][c]!=CH_EMPTY) return false;
    r+=dr; c+=dc;
  }
  return true;
}

bool ChessGame::_pawnMove(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  uint8_t fig=_board[fr][fc], col=CH_COLOR(fig);
  int8_t dir=(col==0)?-1:1, start=(col==0)?6:1;
  int8_t dr=tr-fr, dc=tc-fc;
  if(dc==0&&dr==dir&&_board[tr][tc]==CH_EMPTY) return true;
  if(dc==0&&dr==dir*2&&fr==start&&_board[fr+dir][fc]==CH_EMPTY&&_board[tr][tc]==CH_EMPTY) return true;
  if(abs(dc)==1&&dr==dir&&_board[tr][tc]!=CH_EMPTY&&CH_COLOR(_board[tr][tc])!=col) return true;
  // En passant
  if(abs(dc)==1&&dr==dir&&_board[tr][tc]==CH_EMPTY&&tc==_epCol&&fr==_epRow) {
    uint8_t epFig=_board[_epRow][_epCol];
    if(CH_TYPE(epFig)==CH_PAWN&&CH_COLOR(epFig)!=col) return true;
  }
  return false;
}

bool ChessGame::_rookMove(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  if(fr!=tr&&fc!=tc) return false;
  return _pathClear(fr,fc,tr,tc);
}
bool ChessGame::_knightMove(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  int8_t dr=abs(tr-fr),dc=abs(tc-fc);
  return (dr==2&&dc==1)||(dr==1&&dc==2);
}
bool ChessGame::_bishopMove(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  if(abs(tr-fr)!=abs(tc-fc)) return false;
  return _pathClear(fr,fc,tr,tc);
}
bool ChessGame::_queenMove(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  return _rookMove(fr,fc,tr,tc)||_bishopMove(fr,fc,tr,tc);
}
bool ChessGame::_kingMove(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  return abs(tr-fr)<=1&&abs(tc-fc)<=1;
}

// ── Rochade ───────────────────────────────────────────────────

bool ChessGame::_canCastle(uint8_t side) {
  if(!(_castleRights & side)) return false;
  bool isKing = (side==CR_W_KING||side==CR_B_KING);
  uint8_t row  = (side&(CR_B_KING|CR_B_QUEEN)) ? 0 : 7;
  uint8_t col1 = isKing ? 5 : 1;
  uint8_t col2 = isKing ? 6 : 3;
  for(uint8_t c=col1;c<=col2;c++) if(_board[row][c]!=CH_EMPTY) return false;
  // Felder dürfen nicht angegriffen sein (vereinfacht: König-Felder prüfen)
  uint8_t kc = 4, kc1 = isKing ? 5 : 3, kc2 = isKing ? 6 : 2;
  // Gegner-Farbe
  uint8_t opp = (side&(CR_W_KING|CR_W_QUEEN)) ? 1 : 0;
  // Prüfe ob König-Startfeld, Zwischenfeld, Zielfeld angegriffen
  uint8_t checkCols[3] = {kc, kc1, kc2};
  for(uint8_t ci=0; ci<3; ci++) {
    uint8_t c = checkCols[ci];
    for(int8_t r2=0;r2<8;r2++) for(int8_t c2=0;c2<8;c2++) {
      uint8_t f=_board[r2][c2];
      if(f==CH_EMPTY||CH_COLOR(f)!=opp) continue;
      if(_isValidMove(r2,c2,(int8_t)row,(int8_t)c)) return false;
    }
  }
  return true;
}

// ── Schach-Erkennung (Three-Check + Antichess) ────────────────

bool ChessGame::_isInCheck(uint8_t color) {
  // Königsposition suchen
  int8_t kr=-1,kc=-1;
  for(int8_t r=0;r<8&&kr==-1;r++) for(int8_t c=0;c<8;c++)
    if(_board[r][c]==(CH_KING|(color?CH_BLACK:0))) { kr=r;kc=c;break; }
  if(kr<0) return false;
  uint8_t opp=1-color;
  for(int8_t r=0;r<8;r++) for(int8_t c=0;c<8;c++) {
    uint8_t f=_board[r][c];
    if(f==CH_EMPTY||CH_COLOR(f)!=opp) continue;
    if(_isValidMove(r,c,kr,kc)) return true;
  }
  return false;
}

// Antichess: gibt es irgendeinen Schlagzug für `color`?
bool ChessGame::_canCapture(uint8_t color) {
  for(int8_t fr=0;fr<8;fr++) for(int8_t fc=0;fc<8;fc++) {
    uint8_t f=_board[fr][fc];
    if(f==CH_EMPTY||CH_COLOR(f)!=color) continue;
    for(int8_t tr=0;tr<8;tr++) for(int8_t tc=0;tc<8;tc++) {
      if(_board[tr][tc]==CH_EMPTY||CH_COLOR(_board[tr][tc])==color) continue;
      if(_isValidMove(fr,fc,tr,tc)) return true;
    }
  }
  return false;
}

// ── Hauptzugvalidierung ───────────────────────────────────────

bool ChessGame::_isValidMove(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  if(tr<0||tr>7||tc<0||tc>7) return false;
  uint8_t from=_board[fr][fc], to=_board[tr][tc];
  if(from==CH_EMPTY) return false;
  if(to!=CH_EMPTY&&CH_COLOR(to)==CH_COLOR(from)) return false;

  // Rochade: König zieht 2 Felder
  if(CH_TYPE(from)==CH_KING&&abs(tc-fc)==2&&fr==tr) {
    uint8_t col=CH_COLOR(from);
    if(tc==6) return _canCastle(col==0?CR_W_KING:CR_B_KING);
    if(tc==2) return _canCastle(col==0?CR_W_QUEEN:CR_B_QUEEN);
    return false;
  }

  switch(CH_TYPE(from)) {
    case CH_PAWN:   return _pawnMove  (fr,fc,tr,tc);
    case CH_ROOK:   return _rookMove  (fr,fc,tr,tc);
    case CH_KNIGHT: return _knightMove(fr,fc,tr,tc);
    case CH_BISHOP: return _bishopMove(fr,fc,tr,tc);
    case CH_QUEEN:  return _queenMove (fr,fc,tr,tc);
    case CH_KING:   return _kingMove  (fr,fc,tr,tc);
    default:        return false;
  }
}

// Antichess: Zug nur gültig wenn kein Schlagzug möglich war, ODER dieser Zug schlägt
bool ChessGame::_isValidMoveAntichess(int8_t fr,int8_t fc,int8_t tr,int8_t tc) {
  if(!_isValidMove(fr,fc,tr,tc)) return false;
  if(_canCapture(_turn)) {
    // Muss schlagen: Zielfeld muss besetzt sein (oder en-passant)
    bool isCapture = (_board[tr][tc]!=CH_EMPTY&&CH_COLOR(_board[tr][tc])!=_turn);
    // En passant ist auch ein Schlag
    if(!isCapture && CH_TYPE(_board[fr][fc])==CH_PAWN && tc==_epCol && fr==_epRow)
      isCapture=true;
    return isCapture;
  }
  return true;
}

// ── Input-Handler ─────────────────────────────────────────────

void ChessGame::handleInput(int8_t dx, int8_t dy, bool btn) {
  if(_gameOver) return;

  // ── Crazyhouse Hand-Modus: Joystick blättert durch Figuren ──
  if(_mode==3 && _handMode) {
    // Links/Rechts: andere Figur in der Hand wählen
    if(dx!=0) {
      // Nächste/vorherige Figur mit Anzahl > 0 suchen
      static const uint8_t types[5]={CH_PAWN,CH_ROOK,CH_KNIGHT,CH_BISHOP,CH_QUEEN};
      int8_t cur=-1;
      for(uint8_t i=0;i<5;i++) if(types[i]==_handType){cur=i;break;}
      int8_t next=cur+dx;
      // Überspringen wenn keine Figur vorhanden
      for(uint8_t tries=0;tries<5;tries++){
        if(next<0) next=4;
        if(next>4) next=0;
        if(_hand[_turn][next]>0){_handType=types[next];break;}
        next+=dx;
      }
    }
    // Oben: Hand-Modus abbrechen
    if(dy==-1) { _handMode=false; return; }
    // Button: Figur auf Cursor-Position einsetzen
    if(btn) {
      if(_board[_cursorR][_cursorC]==CH_EMPTY) {
        // Bauer darf nicht auf Grundlinie
        if(_handType==CH_PAWN&&(_cursorR==0||_cursorR==7)) return;
        uint8_t tidx=_handType-1;
        if(_hand[_turn][tidx]>0) {
          _board[_cursorR][_cursorC]=_handType|(_turn?CH_BLACK:0);
          _hand[_turn][tidx]--;
          _handMode=false;
          _epCol=-1; _epRow=-1;
          if(_isInCheck(1-_turn)) {
            _checkCount[1-_turn]++;
            if(_checkCount[1-_turn]>=3) _gameOver=true;
          }
          if(!_gameOver) _turn=1-_turn;
        }
      }
    }
    // Im Hand-Modus Cursor noch bewegen (ohne Button)
    if(!btn) {
      if(dy==-1&&_cursorR>0) _cursorR--;
      if(dy== 1&&_cursorR<7) _cursorR++;
      if(dx==-1&&_cursorC>0) _cursorC--;
      if(dx== 1&&_cursorC<7) _cursorC++;
    }
    return;
  }

  // Cursor bewegen (normaler Modus)
  if(dy==-1&&_cursorR>0) _cursorR--;
  if(dy== 1&&_cursorR<7) _cursorR++;
  if(dx==-1&&_cursorC>0) _cursorC--;
  if(dx== 1&&_cursorC<7) _cursorC++;

  if(!btn) return;

  // ── Crazyhouse: leeres Feld → Hand-Menü öffnen ───────────
  if(_mode==3) {
    bool hasAny=false;
    for(uint8_t i=0;i<5;i++) if(_hand[_turn][i]>0){hasAny=true;break;}
    if(_board[_cursorR][_cursorC]==CH_EMPTY && _selR==-1 && hasAny) {
      // Erste verfügbare Figur in der Hand als Startauswahl
      static const uint8_t types[5]={CH_PAWN,CH_ROOK,CH_KNIGHT,CH_BISHOP,CH_QUEEN};
      for(uint8_t i=0;i<5;i++){
        if(_hand[_turn][i]>0){_handType=types[i];break;}
      }
      _handMode=true;
      return;
    }
  }

  uint8_t fig=_board[_cursorR][_cursorC];

  if(_selR==-1) {
    if(fig!=CH_EMPTY&&CH_COLOR(fig)==_turn)
      { _selR=_cursorR; _selC=_cursorC; }
  } else {
    if(_cursorR==_selR&&_cursorC==_selC) {
      _selR=-1; _selC=-1;
    } else {
      // Zugvalidierung je nach Modus
      bool valid = (_mode==1)
        ? _isValidMoveAntichess(_selR,_selC,_cursorR,_cursorC)
        : _isValidMove(_selR,_selC,_cursorR,_cursorC);

      if(valid) {
        uint8_t moving   = _board[_selR][_selC];
        uint8_t captured = _board[_cursorR][_cursorC];

        // ── En passant ausführen ──────────────────────────
        bool wasEP=false;
        if(CH_TYPE(moving)==CH_PAWN&&_cursorC==_epCol&&_board[_cursorR][_cursorC]==CH_EMPTY
           &&abs(_cursorC-_selC)==1&&abs(_cursorR-_selR)==1) {
          _board[_epRow][_epCol]=CH_EMPTY;
          wasEP=true;
          // Crazyhouse: geschlagener Bauer kommt in die Hand
          if(_mode==3) _hand[_turn][0]++;
        }

        // ── Rochade ausführen ─────────────────────────────
        bool wasCastle=false;
        if(CH_TYPE(moving)==CH_KING&&abs(_cursorC-_selC)==2) {
          wasCastle=true;
          uint8_t row=_selR;
          if(_cursorC==6) { // Königsseite
            _board[row][5]=_board[row][7]; _board[row][7]=CH_EMPTY;
          } else { // Damenseite
            _board[row][3]=_board[row][0]; _board[row][0]=CH_EMPTY;
          }
        }

        // ── Normale Schlaglogik ───────────────────────────
        if(!wasEP&&captured!=CH_EMPTY) {
          if(CH_TYPE(captured)==CH_KING) {
            // Antichess: König schlagen = Sieg
            if(CH_COLOR(captured)==0) _whiteKingAlive=false;
            else                       _blackKingAlive=false;
            _gameOver=true;
          }
          // Crazyhouse: geschlagene Figur in Hand aufnehmen
          if(_mode==3&&CH_TYPE(captured)!=CH_KING) {
            uint8_t tidx=CH_TYPE(captured)-1;
            if(tidx<5) _hand[_turn][tidx]++;
          }
        }

        // ── Zug ausführen ─────────────────────────────────
        _board[_cursorR][_cursorC]=moving;
        _board[_selR][_selC]=CH_EMPTY;

        // ── Rochade: Rechte entziehen ─────────────────────
        if(CH_TYPE(moving)==CH_KING) {
          if(_turn==0) _castleRights&=~(CR_W_KING|CR_W_QUEEN);
          else         _castleRights&=~(CR_B_KING|CR_B_QUEEN);
        }
        if(CH_TYPE(moving)==CH_ROOK) {
          if(_selR==7&&_selC==0) _castleRights&=~CR_W_QUEEN;
          if(_selR==7&&_selC==7) _castleRights&=~CR_W_KING;
          if(_selR==0&&_selC==0) _castleRights&=~CR_B_QUEEN;
          if(_selR==0&&_selC==7) _castleRights&=~CR_B_KING;
        }

        // ── En-passant-Marker aktualisieren ──────────────
        _epCol=-1; _epRow=-1;
        if(CH_TYPE(moving)==CH_PAWN&&abs(_cursorR-_selR)==2) {
          _epCol=_cursorC; _epRow=_cursorR;
        }

        // ── Bauer-Promotion ───────────────────────────────
        uint8_t moved=_board[_cursorR][_cursorC];
        if(CH_TYPE(moved)==CH_PAWN) {
          if(_turn==0&&_cursorR==0) _board[_cursorR][_cursorC]=CH_QUEEN;
          if(_turn==1&&_cursorR==7) _board[_cursorR][_cursorC]=CH_QUEEN|CH_BLACK;
        }

        // ── Three-Check: Schach-Erkennung ─────────────────
        if((_mode==2||_mode==3)&&!_gameOver) {
          if(_isInCheck(1-_turn)) {
            _checkCount[1-_turn]++;
            if(_checkCount[1-_turn]>=3) _gameOver=true;
          }
        }

        // ── Antichess: Gewinn wenn keine Figuren mehr ─────
        if(_mode==1&&!_gameOver) {
          bool hasW=false, hasB=false;
          for(int8_t r=0;r<8;r++) for(int8_t c=0;c<8;c++) {
            uint8_t f=_board[r][c];
            if(f!=CH_EMPTY) { if(CH_COLOR(f)==0) hasW=true; else hasB=true; }
          }
          if(!hasW||!hasB) _gameOver=true;
        }

        (void)wasCastle;
        _selR=-1; _selC=-1;
        if(!_gameOver) _turn=1-_turn;

      } else {
        if(fig!=CH_EMPTY&&CH_COLOR(fig)==_turn)
          { _selR=_cursorR; _selC=_cursorC; }
        else
          { _selR=-1; _selC=-1; }
      }
    }
  }
}

// ── Draw ─────────────────────────────────────────────────────

void ChessGame::_drawField(LEDMatrix& matrix, int8_t r, int8_t c) {
  int8_t pixR=r*2, pixC=c*2+CHESS_COL_OFF;
  bool light=((r+c)%2==0);
  bool isCursor=(_cursorR==r&&_cursorC==c);
  bool isSel=(_selR==r&&_selC==c);
  bool isTarget=(_selR!=-1&&((_mode==1)?_isValidMoveAntichess(_selR,_selC,r,c):_isValidMove(_selR,_selC,r,c)));
  bool isEP=false;
  if(_selR!=-1&&_epCol==c) {
    uint8_t sf=_board[_selR][_selC];
    if(CH_TYPE(sf)==CH_PAWN&&CH_COLOR(sf)==_turn) {
      int8_t dir=(_turn==0)?-1:1;
      if(r==_epRow+dir&&abs(c-_selC)==1&&_board[r][c]==CH_EMPTY) isEP=true;
    }
  }

  uint32_t fc;
  if     (isSel)    fc=_color(7);
  else if(isCursor) fc=_color(6);
  else if(isEP)     fc=0x003030;
  else if(isTarget) fc=light?0x002020:0x001010;
  else              fc=light?_color(0):_color(1);

  matrix.setPixel(pixR,  pixC,  fc); matrix.setPixel(pixR,  pixC+1,fc);
  matrix.setPixel(pixR+1,pixC,  fc); matrix.setPixel(pixR+1,pixC+1,fc);

  uint8_t fig=_board[r][c];
  if(fig==CH_EMPTY) return;
  uint8_t type=CH_TYPE(fig); bool isW=(CH_COLOR(fig)==0);
  uint32_t figColor; int8_t fpR,fpC;
  if(isW) {
    switch(type){
      case CH_PAWN:figColor=_color(8);break; case CH_ROOK:figColor=_color(9);break;
      case CH_KNIGHT:figColor=_color(10);break; case CH_BISHOP:figColor=_color(11);break;
      case CH_QUEEN:figColor=_color(12);break; case CH_KING:figColor=_color(13);break;
      default:figColor=0xFFFFFF;
    }
  } else {
    uint32_t wc=0;
    switch(type){
      case CH_PAWN:wc=_color(8);break; case CH_ROOK:wc=_color(9);break;
      case CH_KNIGHT:wc=_color(10);break; case CH_BISHOP:wc=_color(11);break;
      case CH_QUEEN:figColor=_color(14);goto place;
      case CH_KING:figColor=_color(15);goto place;
      default:wc=0x404040;
    }
    figColor=(((wc>>16)&0xFF)/3)<<16|(((wc>>8)&0xFF)/3)<<8|((wc&0xFF)/3);
  }
  place:
  switch(type){
    case CH_PAWN:fpR=0;fpC=0;break;
    case CH_KNIGHT:case CH_BISHOP:fpR=0;fpC=1;break;
    case CH_ROOK:fpR=1;fpC=0;break;
    default:fpR=1;fpC=1;break;
  }
  matrix.setPixel(pixR+fpR,pixC+fpC,figColor);
}

// Crazyhouse: Hand-Anzeige
// Normal: kleine Farb-Dots links/rechts vom Brett zeigen was man hat
// Hand-Modus aktiv: Auswahl-Leiste unten mit Figuren und Anzahl
void ChessGame::_drawHandUI(LEDMatrix& matrix) {
  static const uint8_t typeOrder[5]={CH_PAWN,CH_ROOK,CH_KNIGHT,CH_BISHOP,CH_QUEEN};

  // ── Immer: kleine Dots neben dem Brett zeigen Figuren in der Hand ──
  for(uint8_t p=0;p<2;p++) {
    int8_t col = p ? 25 : 6;
    for(uint8_t t=0;t<5;t++) {
      if(_hand[p][t]==0) continue;
      uint32_t fc;
      switch(typeOrder[t]) {
        case CH_PAWN:   fc=_color(8);  break;
        case CH_ROOK:   fc=_color(9);  break;
        case CH_KNIGHT: fc=_color(10); break;
        case CH_BISHOP: fc=_color(11); break;
        default:        fc=_color(12); break;
      }
      if(p==1) fc=(((fc>>16)&0xFF)/3)<<16|(((fc>>8)&0xFF)/3)<<8|((fc&0xFF)/3);
      matrix.setPixel(t*3, col, fc);
    }
  }

  // ── Hand-Modus aktiv: Auswahl-Menü unten (Zeilen 12-15) ──────────
  if(!_handMode) return;

  // Hintergrund-Balken
  for(int8_t r=11;r<16;r++)
    for(int8_t c=0;c<32;c++)
      matrix.setPixel(r,c,0x050505);

  // Trennlinie
  for(int8_t c=0;c<32;c++) matrix.setPixel(11,c,0x303000);

  // Titel-Pixel: zeigt wer dran ist (links)
  matrix.setPixel(11,0, _turn==0?0xFFFFFF:0x003030);

  // Figuren der aktuellen Hand anzeigen — je 5 Slots bei Spalten 3,9,15,21,27
  static const int8_t slotCols[5]={3,9,15,21,27};
  for(uint8_t t=0;t<5;t++) {
    uint8_t cnt=_hand[_turn][t];
    bool isSel=(_handType==typeOrder[t]);

    // Slot-Hintergrund
    uint32_t bg=isSel?0x181808:0x080808;
    for(int8_t r=12;r<16;r++)
      for(int8_t c=slotCols[t]-2;c<=slotCols[t]+2;c++)
        matrix.setPixel(r,c,bg);

    if(cnt==0) {
      // Leer: dunkles X
      matrix.setPixel(13,slotCols[t]-1,0x100000);
      matrix.setPixel(13,slotCols[t]+1,0x100000);
      matrix.setPixel(14,slotCols[t],  0x100000);
      continue;
    }

    // Figur-Farbe
    uint32_t fc;
    switch(typeOrder[t]) {
      case CH_PAWN:   fc=_color(8);  break;
      case CH_ROOK:   fc=_color(9);  break;
      case CH_KNIGHT: fc=_color(10); break;
      case CH_BISHOP: fc=_color(11); break;
      default:        fc=_color(12); break;
    }
    if(!isSel) {
      // Gedimmt wenn nicht ausgewählt
      fc=(((fc>>16)&0xFF)/3)<<16|(((fc>>8)&0xFF)/3)<<8|((fc&0xFF)/3);
    }

    // Figur-Pixel (Zeile 13)
    matrix.setPixel(13,slotCols[t],fc);
    // Anzahl-Pixel darunter (Zeile 14-15): 1 Punkt pro Figur
    for(uint8_t n=0;n<cnt&&n<3;n++)
      matrix.setPixel(15,slotCols[t]-1+n,isSel?0xAA8800:0x333300);

    // Auswahl-Rahmen
    if(isSel) {
      for(int8_t c=slotCols[t]-2;c<=slotCols[t]+2;c++) {
        matrix.setPixel(12,c,0xAA8800);
        matrix.setPixel(15,c,0x554400);
      }
      matrix.setPixel(13,slotCols[t]-2,0xAA8800);
      matrix.setPixel(13,slotCols[t]+2,0xAA8800);
      matrix.setPixel(14,slotCols[t]-2,0xAA8800);
      matrix.setPixel(14,slotCols[t]+2,0xAA8800);
    }
  }

  // Hinweis: Pfeil hoch = abbrechen (rechts außen)
  matrix.setPixel(13,30,0x200020);
  matrix.setPixel(12,30,0x300030);
}

void ChessGame::draw(LEDMatrix& matrix) {
  matrix.clear();

  for(int8_t r=0;r<8;r++) for(int8_t c=0;c<8;c++) _drawField(matrix,r,c);

  // Spieler-Indicator
  matrix.setPixel(7,7,  _turn==0?0xFFFFFF:0x0A0A0A);
  matrix.setPixel(7,24, _turn==1?0x003030:0x0A0A0A);

  // Three-Check: Zähler als Punkte oben links/rechts
  if(_mode==2||_mode==3) {
    for(uint8_t i=0;i<_checkCount[0]&&i<3;i++) matrix.setPixel(0,i,0xFF0000);
    for(uint8_t i=0;i<_checkCount[1]&&i<3;i++) matrix.setPixel(0,31-i,0xFF0000);
  }

  // Crazyhouse: Hand anzeigen
  if(_mode==3) _drawHandUI(matrix);

  // Antichess: kleiner Hinweis — Muss-Schlagen leuchtet Cursor rot
  // (wird über isTarget in _drawField abgedeckt)

  if(_gameOver) {
    static uint8_t b=0; b++;
    uint32_t flash;
    if(_mode==1) flash=0x002000;       // Antichess: grün
    else         flash=_whiteKingAlive?0x000020:0x200000;
    if(b<20)
      for(int r=0;r<16;r++) for(int c=CHESS_COL_OFF;c<CHESS_COL_OFF+16;c++)
        matrix.setPixel(r,c,flash);
    if(b>40) b=0;
  }
  matrix.show();
}

bool ChessGame::isOver() const { return _gameOver; }
