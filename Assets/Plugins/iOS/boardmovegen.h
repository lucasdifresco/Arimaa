
/*
 * boardmovegen.h
 * Author: David Wu
 */

#ifndef BOARDMOVEGEN_H_
#define BOARDMOVEGEN_H_

#include "bitmap.h"
#include "board.h"

namespace BoardMoveGen
{
	//MOVEMENT GENERATION--------------------------------------------------------------
  int genPushPulls(const Board& b, pla_t pla, move_t* mv);
  int genSteps(const Board& b, pla_t pla, move_t* mv);
	bool canPushPulls(const Board& b, pla_t pla);
	bool canSteps(const Board& b, pla_t pla);
	bool noMoves(const Board& b, pla_t pla);

  int genPushPullsInvolving(const Board& b, pla_t pla, move_t* mv, Bitmap relArea);
  int genPushesInto(const Board& b, pla_t pla, move_t* mv, Bitmap relArea);
  int genStepsIntoOutTSWF(const Board& b, pla_t pla, move_t* mv, Bitmap relArea); //Into, or involving if the piece is not trapSafe1 or (dominated and not guarded 2)
  int genStepsInvolving(const Board& b, pla_t pla, move_t* mv, Bitmap relArea);
  int genStepsInto(const Board& b, pla_t pla, move_t* mv, Bitmap relArea);

  int genSimpleChainMoves(Board& b, pla_t pla, int numSteps, move_t* mv);
  int genSimpleChainMoves(Board& b, pla_t pla, int numSteps, move_t* mv, Bitmap relPlaPieces);
  int genSimpleChainMoves(Board& b, pla_t pla, loc_t ploc, int numSteps, move_t* mv, move_t prefix, int prefixLen, loc_t prohibitedLoc);

  int genLocalComboMoves(const Board& b, pla_t pla, int numSteps, move_t* mv);
  int genLocalComboMoves(const Board& b, pla_t pla, int numSteps, move_t* mv, move_t prefix, int prefixLen, Bitmap relevantArea);

	//QMOVEMENT GENERATION-------------------------------------------------------------
  int genQuiescenceMoves(const Board& b, pla_t pla, int maxSteps, move_t* mv, int* hm);

}

#endif /* BOARDMOVEGEN_H_ */
