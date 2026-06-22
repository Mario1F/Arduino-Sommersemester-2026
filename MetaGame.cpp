#include "MetaGame.h"
#include <string.h>

MetaGame::MetaGame() { reset(); }

void MetaGame::reset() {
  memset(&_state, 0, sizeof(_state));
  _state.cursorGR = 1; _state.cursorGC = 1;
  _state.cursorMR = 1; _state.cursorMC = 1;
  _state.forcedGR = -1; _state.forcedGC = -1;
  _state.currentPlayer = 1;
  _state.gameOver = false;
  _state.winner = 0;
  _state.hasWinLine = false;
}

void MetaGame::handleInput(int8_t dx, int8_t dy, bool btn) {
  if (_state.gameOver) return;
  if (dx != 0 || dy != 0) _moveCursor(dy, dx);
  if (btn) _makeMove();
}

void MetaGame::draw(LEDMatrix& matrix) {
  GameBoard board(matrix);
  board.draw(_state);
}

bool MetaGame::isOver() const { return _state.gameOver; }

void MetaGame::playEndAnimation(LEDMatrix& matrix) {
  GameBoard board(matrix);
  if (_state.winner == 1 || _state.winner == 2) board.winAnimation(_state);
  else board.tieAnimation();
}

int MetaGame::_checkBoard(int8_t b[3][3], int8_t outWinLine[][2]) {
  for (int r=0;r<3;r++) {
    if (b[r][0]&&b[r][0]==b[r][1]&&b[r][1]==b[r][2]) {
      if (outWinLine){outWinLine[0][0]=r;outWinLine[0][1]=0;outWinLine[1][0]=r;outWinLine[1][1]=1;outWinLine[2][0]=r;outWinLine[2][1]=2;}
      return b[r][0];
    }
  }
  for (int c=0;c<3;c++) {
    if (b[0][c]&&b[0][c]==b[1][c]&&b[1][c]==b[2][c]) {
      if (outWinLine){outWinLine[0][0]=0;outWinLine[0][1]=c;outWinLine[1][0]=1;outWinLine[1][1]=c;outWinLine[2][0]=2;outWinLine[2][1]=c;}
      return b[0][c];
    }
  }
  if (b[0][0]&&b[0][0]==b[1][1]&&b[1][1]==b[2][2]) {
    if (outWinLine){outWinLine[0][0]=0;outWinLine[0][1]=0;outWinLine[1][0]=1;outWinLine[1][1]=1;outWinLine[2][0]=2;outWinLine[2][1]=2;}
    return b[0][0];
  }
  if (b[0][2]&&b[0][2]==b[1][1]&&b[1][1]==b[2][0]) {
    if (outWinLine){outWinLine[0][0]=0;outWinLine[0][1]=2;outWinLine[1][0]=1;outWinLine[1][1]=1;outWinLine[2][0]=2;outWinLine[2][1]=0;}
    return b[0][2];
  }
  for (int r=0;r<3;r++) for (int c=0;c<3;c++) if (!b[r][c]) return 0;
  return 3;
}

bool MetaGame::_miniPlayable(int8_t gr, int8_t gc) {
  if (_state.miniWon[gr][gc]!=0) return false;
  for (int8_t mr=0;mr<3;mr++) for (int8_t mc=0;mc<3;mc++) if (_state.board[gr][gc][mr][mc]==0) return true;
  return false;
}

void MetaGame::_snapCursorToFree(int8_t gr, int8_t gc) {
  _state.cursorGR=gr; _state.cursorGC=gc;
  if (_state.board[gr][gc][1][1]==0){_state.cursorMR=1;_state.cursorMC=1;return;}
  for (int8_t mr=0;mr<3;mr++) for (int8_t mc=0;mc<3;mc++)
    if (_state.board[gr][gc][mr][mc]==0){_state.cursorMR=mr;_state.cursorMC=mc;return;}
}

void MetaGame::_updateForced(int8_t movedMR, int8_t movedMC) {
  if (_miniPlayable(movedMR,movedMC)){_state.forcedGR=movedMR;_state.forcedGC=movedMC;_snapCursorToFree(movedMR,movedMC);}
  else {
    _state.forcedGR=-1;_state.forcedGC=-1;
    for (int8_t gr=0;gr<3;gr++) for (int8_t gc=0;gc<3;gc++)
      if (_miniPlayable(gr,gc)){_snapCursorToFree(gr,gc);return;}
  }
}

void MetaGame::_moveCursor(int dr, int dc) {
  if (_state.gameOver) return;
  if (_state.forcedGR!=-1) {
    int8_t gr=_state.forcedGR,gc=_state.forcedGC,mr=_state.cursorMR,mc=_state.cursorMC;
    for (int s=0;s<8;s++){mr=(mr+dr+3)%3;mc=(mc+dc+3)%3;
      if (_state.board[gr][gc][mr][mc]==0){_state.cursorMR=mr;_state.cursorMC=mc;break;}}
  } else {
    int8_t newMR=_state.cursorMR,newMC=_state.cursorMC,newGR=_state.cursorGR,newGC=_state.cursorGC;
    for (int s=0;s<81;s++){
      newMR+=dr;newMC+=dc;
      if (newMR<0){newMR=2;newGR--;}if (newMR>2){newMR=0;newGR++;}
      if (newMC<0){newMC=2;newGC--;}if (newMC>2){newMC=0;newGC++;}
      newGR=(newGR+3)%3;newGC=(newGC+3)%3;
      if (_miniPlayable(newGR,newGC)&&_state.board[newGR][newGC][newMR][newMC]==0){
        _state.cursorGR=newGR;_state.cursorGC=newGC;_state.cursorMR=newMR;_state.cursorMC=newMC;break;}
    }
  }
}

bool MetaGame::_makeMove() {
  if (_state.gameOver) return false;
  int8_t gr=_state.cursorGR,gc=_state.cursorGC,mr=_state.cursorMR,mc=_state.cursorMC;
  if (_state.miniWon[gr][gc]!=0) return false;
  if (_state.board[gr][gc][mr][mc]!=0) return false;
  if (_state.forcedGR!=-1&&(gr!=_state.forcedGR||gc!=_state.forcedGC)) return false;
  _state.board[gr][gc][mr][mc]=_state.currentPlayer;
  int mResult=_checkBoard(_state.board[gr][gc],nullptr);
  if (mResult!=0) _state.miniWon[gr][gc]=mResult;
  int8_t gBoard[3][3];
  for (int8_t r=0;r<3;r++) for (int8_t c=0;c<3;c++)
    gBoard[r][c]=(_state.miniWon[r][c]==3)?0:_state.miniWon[r][c];
  int gResult=_checkBoard(gBoard,_state.winCells);
  if (gResult==1||gResult==2){_state.winner=gResult;_state.gameOver=true;_state.hasWinLine=true;return true;}
  if (gResult==3){_state.winner=3;_state.gameOver=true;return true;}
  _updateForced(mr,mc);
  _state.currentPlayer=(_state.currentPlayer==1)?2:1;
  return true;
}
