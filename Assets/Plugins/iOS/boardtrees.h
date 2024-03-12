
/*
 * boardtrees.h
 * Author: David Wu
 *
 * Board functions for complex detections and move generation
 */

#ifndef BOARDTREES_H_
#define BOARDTREES_H_

#include "bitmap.h"
#include "board.h"

namespace BoardTrees
{
	//CAP TREE-------------------------------------------------------------------------
	//All the basic canCaps/genCaps functions should be safe with tempSteps

	bool canCaps(Board& b, pla_t pla, int steps);
	int genCaps(Board& b, pla_t pla, int steps, move_t* mv, int* hm);

	bool canCaps(Board& b, pla_t pla, int steps, int minSteps, loc_t kt);
	bool canCaps(Board& b, pla_t pla, int steps, loc_t kt, Bitmap& capMap, int& capDist);

	int genCaps(Board& b, pla_t pla, int steps, int minSteps, loc_t kt, move_t* mv, int* hm);

	int genCapsOption(Board& b, pla_t pla, int steps, int minSteps, bool suicideExtend, loc_t kt, move_t* mv, int* hm, int* stepsUsed = NULL);

	int genCapsFull(Board& b, pla_t pla, int steps, int minSteps, bool suicideExtend, loc_t kt, move_t* mv, int* hm);

	//CAP DEFENSE----------------------------------------------------------------------

	int genTrapDefs(Board& b, pla_t pla, loc_t kt, int numSteps, const Bitmap pStrongerMaps[2][NUMTYPES], move_t* mv);

	int shortestGoodTrapDef(Board& b, pla_t pla, loc_t kt, move_t* mv, int num, const Bitmap& currentWholeCapMap);

	int genRunawayDefs(Board& b, pla_t pla, loc_t ploc, int numSteps, move_t* mv);

	int genDefsUsing(Board& b, pla_t pla, loc_t ploc, loc_t targ, int numSteps, move_t* mv,
			move_t moveSoFar, int stepsSoFar, bool simpleWalk, Bitmap& simpleWalkLocs, bool simpleWalkIsGood=true);

	//ELIMINATION---------------------------------------------------------------------
	bool canElim(Board& b, pla_t pla, int numSteps);

	//GOAL TREE------------------------------------------------------------------------
	int goalDist(Board& b, pla_t pla, int steps);
	int goalDist(Board& b, pla_t pla, int steps, loc_t rloc);

	//ugly helpers used in both cap gen and goal dist
	int genUFPPPEShared(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
	int genBlockedPPShared(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
	int genMoveStrongAdjCentShared(Board& b, pla_t pla, loc_t dest, loc_t eloc, loc_t forbidden, move_t* mv, int* hm, int hmval);

	//GOAL THREAT---------------------------------------------------------------------

	int genGoalThreats(Board& b, pla_t pla, int numSteps, move_t* mv);
	int genGoalThreatsRec(Board& b, pla_t pla, Bitmap relevant, int numStepsLeft, move_t* mv,
			move_t moveSoFar, int numStepsSoFar);
}


#endif /* BOARDTREES_H_ */
