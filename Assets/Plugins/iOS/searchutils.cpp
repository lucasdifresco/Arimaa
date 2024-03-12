/*
 * searchutils.cpp
 * Author: davidwu
 */
#include "pch.h"

#include "global.h"
#include "rand.h"
#include "board.h"
#include "boardhistory.h"
#include "boardmovegen.h"
#include "boardtrees.h"
#include "boardtreeconst.h"
#include "searchutils.h"
#include "searchparams.h"
#include "search.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;

//If won or lost, returns the eval, else returns 0.
eval_t SearchUtils::checkGameEndConditions(const Board& b, const BoardHistory& hist, int cDepth)
{
	pla_t pla = b.player;
	pla_t opp = OPP(pla);
	if(b.step == 0)
	{
		if(b.isGoal(opp)) {return Eval::LOSE + cDepth;} //Lose if opponent goaled
		if(b.isGoal(pla)) {return Eval::WIN - cDepth;} //Win if opponent made us goal
		if(b.isRabbitless(pla)) {return Eval::LOSE + cDepth;} //Lose if opponent eliminated our rabbits
		if(b.isRabbitless(opp)) {return Eval::WIN - cDepth;} //Win if opponent killed all his own rabbits
		if(b.noMoves(pla)) {return Eval::LOSE + cDepth;} //Lose if we have no moves
		if(cDepth > 0 && BoardHistory::isThirdRepetition(b,hist)) {return Eval::WIN - cDepth;} //Win if opponent made a 3rd repetition
	}
	else
	{
		if(b.isGoal(pla)) {return Eval::WIN - cDepth;}		//Win if we goaled
		if(b.isRabbitless(opp)) {return Eval::WIN - cDepth;}	//Win if we killed all opponent's rabbits
		if(b.noMoves(opp) && b.posCurrentHash != b.posStartHash && !b.isGoal(opp) && !b.isRabbitless(pla)) {return Eval::WIN - cDepth;}	//Win if opp has no moves, pos different, opp no goal, we have rabbits
	}
	return 0;
}

//If won or lost, returns the eval, else returns 0.
eval_t SearchUtils::checkGameEndConditionsQSearch(const Board& b, int cDepth)
{
	pla_t pla = b.player;
	pla_t opp = OPP(pla);
  //We do not check 3rd move reps here. We also allow the opponent not to change the position, since we allow quiescence passes.
	if(b.step == 0)
	{
		if(b.isGoal(opp)) {return Eval::LOSE + cDepth;}		//Lose if opponent goaled
		if(b.isGoal(pla)) {return Eval::WIN - cDepth;}		//Win if opponent made us goal
		if(b.isRabbitless(pla)) {return Eval::LOSE + cDepth;}	//Lose if opponent eliminated our rabbits
		if(b.isRabbitless(opp)) {return Eval::WIN - cDepth;}	//Win if opponent killed all his own rabbits
		//if(b.noMoves(pla)) {return Eval::LOSE + cDepth;}		//Lose if we have no moves
		//if(cDepth > 0 && BoardHistory::isThirdRepetition(b,boardHistory)) {return Eval::WIN - cDepth;} //Win if opponent made a 3rd repetition
	}
	else
	{
		if(b.isGoal(pla)) {return Eval::WIN - cDepth;}		//Win if we goaled
		if(b.isRabbitless(opp)) {return Eval::WIN - cDepth;}	//Win if we killed all opponent's rabbits
		//if(b.noMoves(opp) && b.posCurrentHash != b.posStartHash && !b.isGoal(opp) && !b.isRabbitless(pla)) {return Eval::WIN - cDepth;}	//Win if opp has no moves, pos different, opp no goal, we have rabbits
	}
	return 0;
}

//Is this eval a game-ending eval?
bool SearchUtils::isTerminalEval(eval_t eval)
{
	return (eval <= Eval::LOSE_TERMINAL) || (eval >= Eval::WIN_TERMINAL);
}

//Is this eval a game-ending eval that indicates some sort of illegal move or error?
bool SearchUtils::isIllegalityTerminalEval(eval_t eval)
{
	return eval <= Eval::LOSE || eval >= Eval::WIN;
}

//HELPERS - EVAL FLAG BOUNDING--------------------------------------------------------------

//Is the result one that proves a value or bound on this position, given alpha and beta?
bool SearchUtils::canEvalCutoff(eval_t alpha, eval_t beta, eval_t result, flag_t resultFlag)
{
	return
	(resultFlag == Flags::FLAG_EXACT) ||
	(resultFlag == Flags::FLAG_ALPHA && result <= alpha) ||
	(resultFlag == Flags::FLAG_BETA && result >= beta);
}

//Is this a trustworthy terminal eval? For use only with nonnegative rDepth4 (in msearch or higher).
bool SearchUtils::isTrustworthyTerminalEval(eval_t eval, int currentStep, int cDepth, int rDepth4)
{
  if(!isTerminalEval(eval) || rDepth4 < 0)
    return false;

  int wlDist = eval < 0 ? eval - Eval::LOSE : Eval::WIN - eval;
  int totalDepth = cDepth + rDepth4/4;
  int stepWhenEndOfTotalDepth = (currentStep + rDepth4/4) % 4;
  int trustDist = totalDepth + SearchParams::WL_TRUST_RDEPTH_OFFSET[stepWhenEndOfTotalDepth];
  return wlDist <= trustDist;
}

//Is the result a terminal eval that proves a value or bound on this position, given alpha and beta?
//Depending on allowed to be HASH_WL_AB_STRICT, allowed to return the wrong answer so long as
//it's still a proven win or a loss
bool SearchUtils::canTerminalEvalCutoff(eval_t alpha, eval_t beta, int currentStep, int cDepth, int hashDepth4, eval_t result, flag_t resultFlag)
{
  if(!isTrustworthyTerminalEval(result,currentStep,cDepth,hashDepth4))
	  return false;

  if(SearchParams::HASH_WL_AB_STRICT)
		return canEvalCutoff(alpha,beta,result,resultFlag);

	return !(result < 0 && resultFlag == Flags::FLAG_BETA) &&
         !(result > 0 && resultFlag == Flags::FLAG_ALPHA);
}

//HELPERS - MOVES PROCESSING----------------------------------------------------------------------------

void SearchUtils::ensureMoveHistoryArray(move_t*& mv, int& capacity, int newCapacity)
{
	if(capacity < newCapacity)
	{
		move_t* newmv = new move_t[newCapacity];
		if(mv != NULL)
		{
			for(int i = 0; i<capacity; i++)
				newmv[i] = mv[i];
		}

		move_t* mvTemp = mv;
		mv = newmv;
		capacity = newCapacity;

		if(mvTemp != NULL)
			delete[] mvTemp;
	}
}

void SearchUtils::ensureMoveArray(move_t*& mv, int*& hm, int& capacity, int newCapacity)
{
	if(capacity < newCapacity)
	{
		move_t* newmv = new move_t[newCapacity];
		int* newhm = new int[newCapacity];

		if(mv != NULL)
		{
			DEBUGASSERT(hm != NULL);
			for(int i = 0; i<capacity; i++)
			{
				newmv[i] = mv[i];
				newhm[i] = hm[i];
			}
		}

		move_t* mvTemp = mv;
		int* hmTemp = hm;

		mv = newmv;
		hm = newhm;
		capacity = newCapacity;

		if(mvTemp != NULL)
		{
			delete[] mvTemp;
			delete[] hmTemp;
		}
	}
}

//Find the best move in the list at or after index and swap it to index
void SearchUtils::selectBest(move_t* mv, int* hm, int num, int index)
{
	int bestIndex = index;
	int bestHM = hm[index];
	for(int i = index+1; i<num; i++)
	{
		int hmval = hm[i];
		if(hmval > bestHM)
		{
			bestHM = hmval;
			bestIndex = i;
		}
	}

	move_t utemp = mv[bestIndex];
	mv[bestIndex] = mv[index];
	mv[index] = utemp;

	int temp = hm[bestIndex];
	hm[bestIndex] = hm[index];
	hm[index] = temp;
}

void SearchUtils::shuffle(uint64_t seed, vector<move_t>& mv)
{
	Rand rand(seed);
	int num = mv.size();
	for(int i = num-1; i >= 1; i--)
	{
		int r = rand.nextUInt(i+1);
		move_t mtemp = mv[i];
		mv[i] = mv[r];
		mv[r] = mtemp;
	}
}

void SearchUtils::shuffle(uint64_t seed, move_t* mv, int num)
{
	Rand rand(seed);
	for(int i = num-1; i >= 1; i--)
	{
		int r = rand.nextUInt(i+1);
		move_t mtemp = mv[i];
		mv[i] = mv[r];
		mv[r] = mtemp;
	}
}

//Stable insertion sort from largest to smallest by values in hm
void SearchUtils::insertionSort(move_t* mv, int* hm, int numMoves)
{
	for(int i = 1; i < numMoves; i++)
	{
		move_t tempmv = mv[i];
		int temphm = hm[i];
		int j;
		for(j = i; j > 0 && hm[j-1] < temphm; j--)
		{
			mv[j] = mv[j-1];
			hm[j] = hm[j-1];
		}
		mv[j] = tempmv;
		hm[j] = temphm;
	}
}

void SearchUtils::reportIllegalMove(const Board& b, move_t move)
{
	ostringstream err;
	err << "Illegal move in search: " << writeMove(b,move) << endl;
	err << b;
	Global::fatalError(err.str());
}


//HELPERS - FULL MOVE GEN--------------------------------------------------------------------

void SearchUtils::genFullMoveHelper(Board& b, const BoardHistory& hist,
		pla_t pla, vector<move_t>& moves, move_t moveSoFar,
		int nsSoFar, int maxSteps, hash_t posStartHash, bool winLossPrune, ExistsHashTable* fullMoveHash)
{
	if(nsSoFar > 0 && (b.posCurrentHash == posStartHash))
		return;

	if(fullMoveHash->lookup(b))
		return;
	fullMoveHash->record(b);

	if(maxSteps <= 0 || b.player != pla || (b.step != 0 && isTerminalEval(checkGameEndConditions(b,hist,nsSoFar))))
	{
		moves.push_back(moveSoFar);
		return;
	}
	move_t mv[256];

	int num = 0;
	if(winLossPrune)
		num = genWinConditionMoves(b,mv,NULL,4-b.step);
	if(num == 0 || num == -1)
	{
		num = 0;
		if(b.step < 3)
			num += BoardMoveGen::genPushPulls(b,pla,mv+num);
		num += BoardMoveGen::genSteps(b,pla,mv+num);
		if(b.step != 0)
			mv[num++] = PASSMOVE;
	}

	for(int i = 0; i<num; i++)
	{
		Board copy = b;
		copy.makeMove(mv[i]);
		int ns = Board::numStepsInMove(mv[i]);
		move_t newMoveSoFar = Board::concatMoves(moveSoFar,mv[i],nsSoFar);
		genFullMoveHelper(copy,hist,pla,moves,newMoveSoFar,nsSoFar+ns,maxSteps-ns,posStartHash,winLossPrune,fullMoveHash);
	}
}

void SearchUtils::genFullMoves(const Board& board, const BoardHistory& hist,
		vector<move_t>& moves, int maxSteps,
		bool winLossPrune, ExistsHashTable* fullMoveHash)
{
	Board b = board;

	if(winLossPrune)
	{
		move_t winningMove = getWinningFullMove(b);
		if(winningMove != ERRORMOVE)
		{moves.push_back(winningMove); return;}
	}

	if(winLossPrune && (BoardTrees::goalDist(b,b.player,maxSteps) <= 4 || BoardTrees::canElim(b,b.player,maxSteps)))
		winLossPrune = false;

	fullMoveHash->clear();
	genFullMoveHelper(b, hist, b.player, moves, ERRORMOVE, 0, maxSteps, b.posCurrentHash, winLossPrune, fullMoveHash);

	if(winLossPrune && moves.size() == 0)
	{
		fullMoveHash->clear();
		genFullMoveHelper(b, hist, b.player, moves, ERRORMOVE, 0, maxSteps, b.posCurrentHash, false, fullMoveHash);
	}
}

//HELPERS - REGULAR GEN--------------------------------------------------------------------------

int SearchUtils::genRegularMoves(const Board& b, move_t* mv)
{
	pla_t pla = b.player;
	int num = 0;
	//Generate pushpulls if enough steps left
	if(b.step < 3)
	{num += BoardMoveGen::genPushPulls(b,pla,mv);}

	//Generate steps
	num += BoardMoveGen::genSteps(b,pla,mv+num);
	return num;
}

//Finds the step, push, or pull that begins mv.
move_t SearchUtils::pruneToRegularMove(const Board& b, move_t mv)
{
	int ns = Board::numStepsInMove(mv);
	if(ns <= 1)
		return mv;

	step_t step0 = Board::getStep(mv,0);
	step_t step1 = Board::getStep(mv,1);
	loc_t k0 = Board::K0INDEX[step0];
	loc_t k1 = Board::K0INDEX[step1];

	//Ensure one pla and one opp adjacent
	if(!Board::ISADJACENT[k0][k1] || b.owners[k0] == NPLA || b.owners[k1] == NPLA || b.owners[k0] == b.owners[k1])
		return Board::getMove(step0);

	//Since there's an opp involved, if it's only 2 steps, it must be a valid push or pull.
	if(ns <= 2)
		return mv;

	//Extract the push or pull from the longer sequence of steps

	//Pull
	if(b.owners[k0] == b.player)
	{
		//Catch the case where some third piece does a push that makes it look like we pulled
		if(Board::K1INDEX[step1] != k0 || b.pieces[k0] <= b.pieces[k1])
			return Board::getMove(step0);
		return Board::getMove(step0,step1);
	}
	//Push
	else
	{
		//Must be legal push
		return Board::getMove(step0,step1);
	}
}

//HELPERS - CAPTURE GEN---------------------------------------------------------------------------

int SearchUtils::genCaptureMoves(Board& b, int numSteps, move_t* mv, int* hm)
{
	int num = 0;
	for(int trapIndex = 0; trapIndex < 4; trapIndex++)
	{
		loc_t kt = Board::TRAPLOCS[trapIndex];
		num += BoardTrees::genCapsFull(b,b.player,numSteps,2,true,kt,mv+num,hm+num);
	}
	return num;
}

int SearchUtils::genCaptureDefMoves(Board& b, int numSteps, move_t* mv, int* hm)
{
	int num = 0;
	pla_t pla = b.player;
	pla_t opp = OPP(pla);
  Bitmap pStrongerMaps[2][NUMTYPES];
  b.initializeStrongerMaps(pStrongerMaps);

  bool oppCanCap[4];
  Bitmap capMaps[4];
  Bitmap wholeCapMap;
  piece_t biggestThreats[4];

  //Try all the traps to detect captures
  for(int trapIndex = 0; trapIndex < 4; trapIndex++)
	{
		loc_t kt = Board::TRAPLOCS[trapIndex];
		int capDist;
		oppCanCap[trapIndex] = BoardTrees::canCaps(b,opp,4,kt,capMaps[trapIndex],capDist);
		if(oppCanCap[trapIndex])
			wholeCapMap |= capMaps[trapIndex];

		piece_t biggestThreat = EMP;
		Bitmap capMap = capMaps[trapIndex];
		while(capMap.hasBits())
		{
			loc_t loc = capMap.nextBit();
			if(b.pieces[loc] > biggestThreat)
				biggestThreat = b.pieces[loc];
		}
		biggestThreats[trapIndex] = biggestThreat;
	}

  //Gen all trap defenses
  int shortestGoodDef[4]; //Shortest "good" defense. No need to try runaways unless they're shorter.
  for(int trapIndex = 0; trapIndex < 4; trapIndex++)
	{
  	if(!oppCanCap[trapIndex])
			continue;
		loc_t kt = Board::TRAPLOCS[trapIndex];
  	int defNum = BoardTrees::genTrapDefs(b,pla,kt,numSteps,pStrongerMaps,mv+num);
		shortestGoodDef[trapIndex] = BoardTrees::shortestGoodTrapDef(b,pla,kt,mv+num,defNum,wholeCapMap);

	  //Prune trap defenses to include nothing longer than the shortest good one
		int newDefNum = 0;
		for(int i = 0; i<defNum; i++)
		{
			if(Board::numStepsInMove(mv[num+i]) <= shortestGoodDef[trapIndex])
			{
				mv[num+newDefNum] = mv[num+i];
				newDefNum++;
			}
		}

		for(int i = 0; i<newDefNum; i++)
			hm[num+i] = biggestThreats[trapIndex];

		num += newDefNum;
	}

	//Runaways defense
  for(int trapIndex = 0; trapIndex < 4; trapIndex++)
	{
  	if(!oppCanCap[trapIndex])
			continue;
  	Bitmap capMap = capMaps[trapIndex];

		//Already a good defense - no need to go more
		if(shortestGoodDef[trapIndex] <= 1)
			continue;
		int maxRunawaySteps = shortestGoodDef[trapIndex]-1 < numSteps ? shortestGoodDef[trapIndex]-1 : numSteps;

		while(capMap.hasBits())
		{
			loc_t ploc = capMap.nextBit();

			//No use running away on trap - was covered when we tried to add defenders
			if(Board::ISTRAP[ploc])
				continue;

			int runawayNum = BoardTrees::genRunawayDefs(b,pla,ploc,maxRunawaySteps,mv+num);
			for(int i = 0; i<runawayNum; i++)
				hm[num+i] = b.pieces[ploc];
			num += runawayNum;
		}
	}

  return num;
}

//The below function relies on this being LR
static const int EDGE_ADJ_1[64] =
{
 0, 0, 1, 2, 5, 6, 7, 7,
 8, 8, 9,10,13,14,15,15,
16,16,17,18,21,22,23,23,
24,24,25,26,29,30,31,31,
32,32,33,34,37,38,39,39,
40,40,41,42,45,46,47,47,
48,48,49,50,53,54,55,55,
56,56,57,58,61,62,63,63,
};
//The below function relies on this being UD
static const int EDGE_ADJ_2[64] =
{
 0, 1, 2, 3, 4, 5 ,6, 7,
 0, 1, 2, 3, 4, 5, 6, 7,
 8, 9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,
40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,
56,57,58,59,60,61,62,63,
56,57,58,59,60,61,62,63,
};

//Pla is the holder, as usual
static int isEleMostlyImmoStatus(const Board& b, pla_t pla, loc_t loc)
{
  pla_t opp = OPP(pla);
  loc_t edgeAdj1 = EDGE_ADJ_1[loc];
  loc_t edgeAdj2 = EDGE_ADJ_2[loc];
  int edgeCount = 0;
  if(CS1(loc))
  { bool blocked = true;
    if     (b.owners[loc-8] == NPLA) {blocked = !b.isTrapSafe2(opp,loc-8);}
    else if(b.owners[loc-8] == pla)  {blocked = b.pieces[loc-8] >= b.pieces[loc] || !b.isTrapSafe2(opp,loc-8) || !b.isOpen(loc-8);}
    else                             {blocked = !b.isOpenToStep(loc-8,loc) && !b.isOpenToPush(loc-8);}
    if(!blocked)
    {if(loc-8 == edgeAdj2) edgeCount+=1;
     else return 2;
     if(edgeCount > 1) {return 2;}}
  }
  if(CW1(loc))
  { bool blocked = true;
    if     (b.owners[loc-1] == NPLA) {blocked = !b.isTrapSafe2(opp,loc-1);}
    else if(b.owners[loc-1] == pla)  {blocked = b.pieces[loc-1] >= b.pieces[loc] || !b.isTrapSafe2(opp,loc-1) || !b.isOpen(loc-1);}
    else                             {blocked = !b.isOpenToStep(loc-1,loc) && !b.isOpenToPush(loc-1);}
    if(!blocked)
    {if(loc-1 == edgeAdj1 && Board::ISEDGE[loc]) edgeCount+=1;
     else return 2;
     if(edgeCount > 1) {return 2;}}
  }
  if(CE1(loc))
  { bool blocked = true;
    if     (b.owners[loc+1] == NPLA) {blocked = !b.isTrapSafe2(opp,loc+1);}
    else if(b.owners[loc+1] == pla)  {blocked = b.pieces[loc+1] >= b.pieces[loc] || !b.isTrapSafe2(opp,loc+1) || !b.isOpen(loc+1);}
    else                             {blocked = !b.isOpenToStep(loc+1,loc) && !b.isOpenToPush(loc+1);}
    if(!blocked)
    {if(loc+1 == edgeAdj1 && Board::ISEDGE[loc]) edgeCount+=1;
     else return 2;
     if(edgeCount > 1) {return 2;}}
  }
  if(CN1(loc))
  { bool blocked = true;
    if     (b.owners[loc+8] == NPLA) {blocked = !b.isTrapSafe2(opp,loc+8);}
    else if(b.owners[loc+8] == pla)  {blocked = b.pieces[loc+8] >= b.pieces[loc] || !b.isTrapSafe2(opp,loc+8) || !b.isOpen(loc+8);}
    else                             {blocked = !b.isOpenToStep(loc+8,loc) && !b.isOpenToPush(loc+8);}
    if(!blocked)
    {if(loc+8 == edgeAdj2) edgeCount+=1;
     else return 2;
     if(edgeCount > 1) {return 2;}}
  }
  return edgeCount;
}

static int genBlockadeMovesRec(const Board& b, pla_t pla, loc_t oppEleLoc, move_t moveSoFar, int nsSoFar, int maxSteps, int& bestSoFar, uint64_t posStartHash, move_t* mv, int* hm, int totalNum)
{
  move_t submv[256];
  if((nsSoFar > 0 && b.posCurrentHash == posStartHash) || pla != b.player)
    return totalNum;

  int num = 0;
  if(b.step < 3 && maxSteps > 1)
    num += BoardMoveGen::genPushPullsInvolving(b,pla,submv+num,Board::DISK[2][oppEleLoc]);
  num += BoardMoveGen::genStepsInto(b,pla,submv+num,Board::DISK[2][oppEleLoc]);

  for(int i = 0; i<num; i++)
  {
    Board copy = b;
    copy.makeMove(submv[i]);
    int ns = Board::numStepsInMove(submv[i]);
    move_t newMoveSoFar = Board::concatMoves(moveSoFar,submv[i],nsSoFar);

    int immoStatus = isEleMostlyImmoStatus(copy,pla,oppEleLoc);
    if(immoStatus < bestSoFar)
    {bestSoFar = immoStatus; totalNum = 0;}
    if(immoStatus <= bestSoFar)
    {mv[totalNum] = newMoveSoFar; hm[totalNum] = 0; totalNum++; if(totalNum >= 128) return totalNum;}
    else if(maxSteps > 0 && copy.step != 0)
    {
      totalNum = genBlockadeMovesRec(copy,pla,oppEleLoc,newMoveSoFar,nsSoFar+ns,maxSteps-ns,bestSoFar,posStartHash,mv,hm,totalNum);
      if(totalNum >= 128) return totalNum;
    }
  }
  return totalNum;
}

static const int numPiecesToConsiderBlockade[64] =
{
   4,5,6,6,6,6,5,4,
   5,6,6,0,0,6,6,5,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   5,6,6,0,0,6,6,5,
   4,5,6,6,6,6,5,4,
};

static int genBlockadeMoves(const Board& b, move_t* mv, int* hm)
{
  pla_t pla = b.player;
  pla_t opp = OPP(pla);
  loc_t oppEleLoc = b.findElephant(opp);

  if(oppEleLoc == ERRORSQUARE || numPiecesToConsiderBlockade[oppEleLoc] == 0)
    return 0;
  if((Board::DISK[2][oppEleLoc] & b.pieceMaps[pla][0]).countBits() < numPiecesToConsiderBlockade[oppEleLoc])
    return 0;
  if(isEleMostlyImmoStatus(b,pla,oppEleLoc) == 0)
    return 0;

  move_t mvtemp[128];
  int hmtemp[128];

  int bestSoFar = 1;
  hash_t posStartHash = (b.step == 0 ? b.posCurrentHash : b.posStartHash);
  for(int i = 1; i<=4-b.step; i++)
  {
    int num = genBlockadeMovesRec(b,pla,oppEleLoc,ERRORMOVE,0,i,bestSoFar,posStartHash,mv,hm,0);
    if(num > 0)
    {
      if(bestSoFar == 0 || i == (4-b.step))
        return num;

      //Otherwise, bestSoFar is 1.
      //Search the next iters for a complete blockade
      bestSoFar = 0;
      for(i++; i<=4-b.step; i++)
      {
        int numComp = genBlockadeMovesRec(b,pla,oppEleLoc,ERRORMOVE,0,i,bestSoFar,posStartHash,mvtemp,hmtemp,0);
        if(numComp > 0)
        {
          //Found some, so copy back
          for(int j = 0; j<numComp; j++)
          {mv[j] = mvtemp[j]; hm[j] = hmtemp[j];}
          return numComp;
        }
      }

      //Return what we have
      return num;
    }
  }
  return 0;
}


//HELPERS - QUIESCENCE GEN----------------------------------------------------------------

int SearchUtils::genQuiescenceMoves(Board& b, const BoardHistory& hist, int cDepth, int qDepth, move_t* mv, int* hm)
{
	int num = 0;
	int numSteps = 4-b.step;

	if(qDepth == 1)
	{
		num += genCaptureMoves(b,numSteps,mv+num,hm+num);
    num += genBlockadeMoves(b,mv+num,hm+num);
	}
	else //qDepth == 0
	{
		num += genCaptureMoves(b,numSteps,mv+num,hm+num);
		num += genCaptureDefMoves(b,numSteps,mv+num,hm+num);
		num += genBlockadeMoves(b,mv+num,hm+num);
	  /*
		int goalThreatNum = BoardTrees::genGoalThreats(b, pla, numSteps, mv+num);
		for(int i = 0; i<goalThreatNum; i++)
			hm[num+i] = 0;
		num += goalThreatNum;
	  */
	}

	//TODO test if generating reverse moves actually helps. It increases the branching factor and makes the search unstable,
	//but maybe it pays off due to the occasional better eval?
	int rnum = genReverseMoves(b,hist,cDepth,mv+num);
	for(int i = 0; i<rnum; i++)
		hm[num+i] = 0;
	num += rnum;

	//Generate the pass move
	if(qDepth == 1)
	{mv[num] = QPASSMOVE; hm[num] = 0; num++;}
	else
	{mv[num] = PASSMOVE; hm[num] = 0; num++;}

	return num;
}

//HELPERS - WIN RELATED GEN-------------------------------------------------------------------------

static void expandRelArea(const Board& b, pla_t pla, const Bitmap relArea, Bitmap& stepRel, Bitmap& pushRel, int maxSteps);

//Returns number of interior nodes in search
int SearchUtils::winDefSearch(Board& b, move_t* shortestmv, int& shortestnum, int loseStepsMax)
{
  int numInteriorNodes = 0;
  return winDefSearch(b,shortestmv,shortestnum,loseStepsMax,numInteriorNodes);
}

//TODO optimize this to make this as fast as possible. Currently, it makes a noticeable slowdown.
int SearchUtils::winDefSearch(Board& b, move_t* shortestmv, int& shortestnum, int loseStepsMax, int& numInteriorNodes)
{
	int numStepsLeft = 4-b.step;
	int bestStepsToLose = 0;
	for(int maxSteps = 1; maxSteps <= numStepsLeft; maxSteps++)
	{
		shortestnum = 0;
		int stepsToLose = winDefSearchHelper(b,b.player,numStepsLeft,maxSteps,ERRORMOVE,0,shortestmv,shortestnum,loseStepsMax,numInteriorNodes);
		if(stepsToLose == 9 || stepsToLose >= loseStepsMax)
			return stepsToLose;

		if(stepsToLose > bestStepsToLose)
			bestStepsToLose = stepsToLose;
	}
	return bestStepsToLose;
}

int SearchUtils::winDefSearchHelper(Board& b, pla_t pla, int origStepsLeft, int maxSteps, move_t sofar, int sofarNS,
		move_t* shortestmv, int& shortestnum, int loseStepsMax, int& numInteriorNodes)
{
	//If we have no rabbits (such as we sacked them to stop goal), then we lose.
	if(b.pieceCounts[pla][RAB] == 0)
		return origStepsLeft;

	//Can't go any further - test if opponent can win
	if(maxSteps <= 0)
	{
		pla_t opp = OPP(pla);

		//TODO use loseStepsMax to bound the goal dist test, rather than just passing in 4.
		int oppGoalDist = BoardTrees::goalDist(b,opp,4);
		//Opponent can still goal? - we lose in number of steps we had, plus number of steps for opp to goal
		if(oppGoalDist < 5)
			return origStepsLeft + oppGoalDist;

		//Guaranteed to be above the max now?
		if(origStepsLeft+4 >= loseStepsMax)
			return loseStepsMax;

		//Opponent can elim? - assume it takes 4 steps to elim
		if(BoardTrees::canElim(b,opp,4))
			return origStepsLeft + 4;

		//Successly stopped opp win - add move
		shortestmv[shortestnum] = sofar;
		shortestnum++;
		return 9;
	}

	//Generate win defense moves
	move_t mv[256];
	int hm[256];
	int num = genWinConditionMoves(b,mv,hm,maxSteps);

	//No defenses - we lose right away...
	if(num == 0)
		return origStepsLeft;

	//No win threat! We're done!
	if(num == -1)
	{
		if(sofar != ERRORMOVE) {shortestmv[shortestnum] = sofar; shortestnum++;}
		return 9;
	}

	//Else we need to recurse and try all the possible moves
	numInteriorNodes++;
	int bestStepsToLose = 0;
	for(int m = 0; m<num; m++)
	{
		move_t move = mv[m];
		Board copy = b;
		copy.makeMove(move);

		//Can't undo the position
		if(copy.posCurrentHash == copy.posStartHash)
			continue;

		//TODO maybe gain a speedup by checking not only if we returned to the start, but if we made other sorts of transpositions.
		//A small hashtable perhaps?

		int ns = Board::numStepsInMove(move);
		move_t nextSoFar = Board::concatMoves(sofar,move,sofarNS);
		int stepsToLose = winDefSearchHelper(copy,pla,origStepsLeft,maxSteps-ns,nextSoFar,sofarNS+ns,
				shortestmv,shortestnum,loseStepsMax,numInteriorNodes);
		if(stepsToLose > bestStepsToLose)
		{
			bestStepsToLose = stepsToLose;
			if(stepsToLose >= loseStepsMax)  //Beta pruning
				return stepsToLose;
		}
	}
	return bestStepsToLose;
}

int SearchUtils::genWinConditionMoves(Board& b, move_t* mv, int* hm, int maxSteps)
{
	int num = genGoalDefMoves(b,mv,hm,maxSteps);
	if(num >= 0)
		return num;

	num = genElimDefMoves(b,mv,hm,maxSteps);
	if(num >= 0)
		return num;

	return -1;
}

int SearchUtils::genElimDefMoves(Board& b, move_t* mv, int* hm, int maxSteps)
{
	pla_t pla = b.player;
	pla_t opp = OPP(pla);

	if(!BoardTrees::canElim(b,opp,4))
		return -1;

	if(b.pieceCounts[pla][RAB] == 0)
		return 0;

	int num = 0;
	Bitmap stepRel;
	Bitmap pushRel;
	loc_t traps[2];
	if(b.pieceCounts[pla][RAB] == 1)
	{
		Bitmap rMap = b.pieceMaps[pla][RAB];
		loc_t rloc = rMap.nextBit();
		if(rloc == ERRORSQUARE)
			Global::fatalError("genElimDefMoves: PieceCount 1 but no rabbit in bitmap");
		loc_t trap = Board::CLOSEST_TRAP[rloc];
		Bitmap relArea = Board::DISK[6][rloc] | Board::DISK[6][trap];
		expandRelArea(b,pla,relArea,stepRel,pushRel,maxSteps);

		traps[0] = trap;
		traps[1] = trap;
	}
	else if(b.pieceCounts[pla][RAB] == 2)
	{
		Bitmap rMap = b.pieceMaps[pla][RAB];
		Bitmap relArea;
		int i = 0;
		while(rMap.hasBits())
		{
			loc_t rloc = rMap.nextBit();
			loc_t trap = Board::CLOSEST_TRAP[rloc];
			relArea |= Board::DISK[3][rloc] | Board::DISK[3][trap];
			traps[i++] = trap;
			if(i > 2)
				Global::fatalError("genElimDefMoves: PieceCount 2 but more than 2 in bitmap");
		}
		expandRelArea(b,pla,relArea,stepRel,pushRel,maxSteps);
	}

	//Generate pushpulls if enough steps left
	if(maxSteps > 1)
		num += BoardMoveGen::genPushPullsInvolving(b,pla,mv,pushRel);

	//Generate steps
	//TODO can we strengthen this condition of step == 3 to just always do it when only 1 maxstep left?
	if(b.step == 3 && maxSteps <= 1)
		num += BoardMoveGen::genStepsInto(b,pla,mv+num,stepRel);
	else
		num += BoardMoveGen::genStepsInvolving(b,pla,mv+num,stepRel);

	//Order the moves
	if(hm != NULL)
	{
		for(int i = 0; i<num; i++)
		{
			move_t move = mv[i];
			int h = 0;
			int ns = Board::numStepsInMove(move);
			for(int j = 0; j<ns; j++)
			{
				step_t step = Board::getStep(move,j);
				loc_t k0 = Board::K0INDEX[step];

				if(b.owners[k0] == pla)
					h = min(Board::MANHATTANDIST[traps[0]][k0],Board::MANHATTANDIST[traps[1]][k0]);
			}
			hm[i] = h;
		}
	}
	return num;
}

int SearchUtils::genGoalDefMoves(Board& b, move_t* mv, int* hm, int maxSteps)
{
	pla_t pla = b.player;
	int num = 0;
	loc_t rloc = ERRORSQUARE;
	Bitmap stepRel = Bitmap::BMPONES;
	Bitmap pushRel = Bitmap::BMPONES;
	if(!getGoalDefRelArea(b,rloc,stepRel,pushRel,maxSteps))
		return -1;

	//Generate pushpulls if enough steps left
	if(maxSteps > 1)
		num += BoardMoveGen::genPushPullsInvolving(b,pla,mv,pushRel);

	//Generate steps
	if(b.step == 3 && maxSteps <= 1)
		num += BoardMoveGen::genStepsInto(b,pla,mv+num,stepRel);
	else
		num += BoardMoveGen::genStepsInvolving(b,pla,mv+num,stepRel);

	//Order the moves
	if(hm != NULL)
	{
		for(int i = 0; i<num; i++)
		{
			move_t move = mv[i];
			int h = 0;
			int ns = Board::numStepsInMove(move);
			for(int j = 0; j<ns; j++)
			{
				step_t step = Board::getStep(move,j);
				loc_t k0 = Board::K0INDEX[step];
				loc_t k1 = Board::K1INDEX[step];
				if(k0 == rloc && Board::GOALYDIST[OPP(pla)][k1] > Board::GOALYDIST[OPP(pla)][k0])
				{h = 10; break;}
				else if(k0 == rloc && Board::GOALYDIST[OPP(pla)][k1] < Board::GOALYDIST[OPP(pla)][k0])
				{h = 0; break;}
				else if(k0 == rloc)
				{h = 5; break;}

				if(b.owners[k0] == pla)
				{
					if(Board::MANHATTANDIST[rloc][k1] < Board::MANHATTANDIST[rloc][k0])
						h = 9-Board::MANHATTANDIST[rloc][k1] < 1 ? 1 : 9-Board::MANHATTANDIST[rloc][k1];
					else
						h = 0;
					break;
				}
			}
			hm[i] = h;
		}
	}
	return num;
}

static void expandRelArea(const Board& b, pla_t pla, const Bitmap relArea, Bitmap& stepRel, Bitmap& pushRel, int maxSteps)
{
	//peo - pla emp opp
	//TO - unsafe trap with opp
	//TP - unsafe trap with pla
	//x expand, doesn't include actual, only adjacent
	//S0 - what we care about to stop the goal
	//Sn,Zn

	//Z[n+1] = S[n] | (S[n] & TO)x
	//S[n+1] = Sn | Snpex | Snexpx | (Snexpx & TP)x | Zn | Znx | Znexpx | Znexoxp | Znexoxpx

	//Step n+1: care about steps involving sn, pushpulls involving Zn

	if(maxSteps == 1)
	{
		stepRel = relArea;
		pushRel = Bitmap();
		return;
	}

	pla_t opp = OPP(pla);
	Bitmap plaMap = b.pieceMaps[pla][0];
	Bitmap oppMap = b.pieceMaps[opp][0];
	Bitmap empMap = ~(plaMap | oppMap);

	//Unsafe traps with pla/opp
	Bitmap tplaMap;
	Bitmap toppMap;
	if     (b.owners[18] == pla && b.trapGuardCounts[pla][0] < 2) {tplaMap.setOn(18);}
	else if(b.owners[18] == opp && b.trapGuardCounts[opp][0] < 2) {toppMap.setOn(18);}
	if     (b.owners[21] == pla && b.trapGuardCounts[pla][1] < 2) {tplaMap.setOn(21);}
	else if(b.owners[21] == opp && b.trapGuardCounts[opp][1] < 2) {toppMap.setOn(21);}
	if     (b.owners[42] == pla && b.trapGuardCounts[pla][2] < 2) {tplaMap.setOn(42);}
	else if(b.owners[42] == opp && b.trapGuardCounts[opp][2] < 2) {toppMap.setOn(42);}
	if     (b.owners[45] == pla && b.trapGuardCounts[pla][3] < 2) {tplaMap.setOn(45);}
	else if(b.owners[45] == opp && b.trapGuardCounts[opp][3] < 2) {toppMap.setOn(45);}

	//Z[n+1] = S | (S & TO)x
	//S[n+1] = S | Spex | (Spx & TP)x | Sexpx | (Sexpx & TP)x | Z | Zx | Zexpx | Zexox | Zexoxpx | Zoxpex | Zoxpexpxp | (Zpx & TP)x
	//
	// Not included because sacrificial and "stupid": (Spx & TE)x | Spxex & TE | (Zex & TP)xPx
	// Probably some others

	//Step n+1: care about steps involving Sn, pushpulls involving Zn

	Bitmap s = relArea;
	Bitmap z = Bitmap();
	for(int i = 0; i<maxSteps-1; i++)
	{
		Bitmap newz = s | Bitmap::adj(s & toppMap);
		Bitmap news = s;
		Bitmap spex = Bitmap::adj(s & (plaMap | empMap));
		Bitmap sexpx = Bitmap::adj(Bitmap::adj(s & empMap) & plaMap);
		Bitmap spexWithsexpx = spex | sexpx;
		news |= spexWithsexpx | Bitmap::adj(spexWithsexpx & tplaMap);

		news |= z;
		news |= Bitmap::adj(z);
		Bitmap zex = Bitmap::adj(z & empMap);
		news |= Bitmap::adj(zex & plaMap);
		news |= Bitmap::adj(Bitmap::adj(z & plaMap) & tplaMap);
		Bitmap zexox = Bitmap::adj(zex & oppMap);
		news |= zexox;
		news |= Bitmap::adj(zexox & plaMap);
		Bitmap zoxpex = Bitmap::adj(Bitmap::adj(z & oppMap) & (plaMap | empMap));
		news |= zoxpex;
		news |= Bitmap::adj(zoxpex & plaMap) & plaMap;

		s = news;
		z = newz;
	}
	stepRel = s;
	pushRel = z;
}

bool SearchUtils::getGoalDefRelArea(Board& b, loc_t& rloc, Bitmap& stepRel, Bitmap& pushRel, int maxSteps)
{
	//Pla trying to stop opp goal
	pla_t pla = b.player;
	loc_t opp = OPP(pla);
	Bitmap rMap = b.pieceMaps[opp][RAB];
	rMap &= Board::PGOALMASKS[4][opp];

	bool found = false;
	rloc = ERRORSQUARE;
	int gDist = 5;
	while(rMap.hasBits())
	{
	  loc_t k = rMap.nextBit();
		gDist = BoardTrees::goalDist(b,opp,4,k);
		if(gDist < 5)
		{
			found = true;
			rloc = k;
			break;
		}
	}

	if(!found)
	{
		stepRel = Bitmap::BMPONES;
		pushRel = Bitmap::BMPONES;
		return false;
	}

	if(gDist <= 0)
	{
		stepRel = Bitmap::BMPZEROS;
		pushRel = Bitmap::BMPZEROS;
		pushRel.setOn(rloc);
		return true;
	}

	Bitmap relArea;
	if(b.goalTreeMove != ERRORMOVE)
	{
		loc_t rabDest = rloc;
		int nsInMove = Board::numStepsInMove(b.goalTreeMove);
		for(int i = 0; i<nsInMove; i++)
		{
			step_t step = Board::getStep(b.goalTreeMove,i);
			loc_t src = Board::K0INDEX[step];
			loc_t dest = Board::K1INDEX[step];
			relArea |= Board::DISK[1][src];
			//if(Board::CHECKCAPSAT[src] != ERRORSQUARE)
			//	relArea |= Board::DISK[1][Board::CHECKCAPSAT[src]];

			if(src == rloc)
				rabDest = dest;
		}
		relArea |= Board::GPRUNEMASKS[opp][rabDest][gDist-nsInMove];
		relArea &= Board::GPRUNEMASKS[opp][rloc][gDist];
	}
	else
		relArea = Board::GPRUNEMASKS[opp][rloc][gDist];

	expandRelArea(b,pla,relArea,stepRel,pushRel,maxSteps);
	return true;
}

move_t SearchUtils::getWinningFullMove(Board& b)
{
	pla_t pla = b.player;
	int numSteps = 4-b.step;
	if(BoardTrees::goalDist(b,pla,numSteps) < 5)
	{
		Board copy = b;
		move_t moveSoFar = ERRORMOVE;
		int soFarLen = 0;
		while(BoardTrees::goalDist(copy,pla,numSteps-soFarLen) > 0)
		{
			int ns = Board::numStepsInMove(copy.goalTreeMove);
			moveSoFar = Board::concatMoves(moveSoFar,copy.goalTreeMove,soFarLen);
			soFarLen += ns;

			bool suc = copy.makeMoveLegal(copy.goalTreeMove);
			if(!suc)
				return ERRORMOVE;

			if(soFarLen > numSteps || ns == 0)
				return ERRORMOVE;
		}

		if(soFarLen > numSteps)
			return ERRORMOVE;

		if(copy.isGoal(pla))
			return moveSoFar;
	}
	return ERRORMOVE;
}

//HELPERS - REVERSIBLES------------------------------------------------------------------------------

int SearchUtils::genReverseMoves(const Board& b, const BoardHistory& boardHistory, int cDepth, move_t* mv)
{
	//Ensure start of turn and deep enough
	if(b.step != 0 || cDepth < 4)
		return 0;

	//Ensure we have history
	int lastTurn = b.turnNumber-1;
	if(lastTurn < boardHistory.minTurnNumber)
		return 0;

	//Arrays to track pieces and where they went
	loc_t src[8];
	loc_t dest[8];

	move_t lastMove = boardHistory.turnMove[lastTurn];
	int numChanges = boardHistory.turnBoard[lastTurn].getChanges(lastMove,src,dest);

	pla_t pla = b.player;
	pla_t opp = OPP(pla);
	int numMoves = 0;
	for(int ch = 0; ch<numChanges; ch++)
	{
	  loc_t k0 = src[ch];
	  loc_t k1 = dest[ch];

		if(k1 != ERRORSQUARE && b.owners[k1] == pla &&
				Board::MANHATTANDIST[k1][k0] == 1 &&
				b.isRabOkay(pla,k1,k0) &&
				b.isTrapSafe2(pla,k0))
		{
			if(b.owners[k0] == NPLA)
			{
				if(b.isThawed(k1))
					mv[numMoves++] = Board::getMove(Board::STEPINDEX[k1][k0]);
				else if(b.step < 2)
				{
					for(int i = 0; i<4; i++)
					{
						if(!Board::ADJOKAY[i][k1])
							continue;
						loc_t loc = k1 + Board::ADJOFFSETS[i];
						if(loc != k0 && ISE(loc) && b.isTrapSafe3(pla,loc))
						{
							if(CS1(loc) && ISP(loc S) && b.pieces[loc S] != RAB && b.isThawed(loc S) && b.wouldBeUF(pla,loc S,loc,loc S,k1)) {mv[numMoves++] = Board::getMove(loc S MN,Board::STEPINDEX[k1][k0],loc MS); break;}
							if(CW1(loc) && ISP(loc W)                           && b.isThawed(loc W) && b.wouldBeUF(pla,loc W,loc,loc W,k1)) {mv[numMoves++] = Board::getMove(loc W ME,Board::STEPINDEX[k1][k0],loc MW); break;}
							if(CE1(loc) && ISP(loc E)                           && b.isThawed(loc E) && b.wouldBeUF(pla,loc E,loc,loc E,k1)) {mv[numMoves++] = Board::getMove(loc E MW,Board::STEPINDEX[k1][k0],loc ME); break;}
							if(CN1(loc) && ISP(loc N) && b.pieces[loc N] != RAB && b.isThawed(loc N) && b.wouldBeUF(pla,loc N,loc,loc N,k1)) {mv[numMoves++] = Board::getMove(loc N MS,Board::STEPINDEX[k1][k0],loc MN); break;}
						}
					}
				}
			}
			else if(b.step < 3 && b.owners[k0] == opp && b.pieces[k0] < b.pieces[k1] && b.isThawed(k1))
			{
				if(CS1(k0) && k0 S != k1 && ISE(k0 S)) mv[numMoves++] = Board::getMove(k0 MS,Board::STEPINDEX[k1][k0]);
				if(CW1(k0) && k0 W != k1 && ISE(k0 W)) mv[numMoves++] = Board::getMove(k0 MW,Board::STEPINDEX[k1][k0]);
				if(CE1(k0) && k0 E != k1 && ISE(k0 E)) mv[numMoves++] = Board::getMove(k0 ME,Board::STEPINDEX[k1][k0]);
				if(CN1(k0) && k0 N != k1 && ISE(k0 N)) mv[numMoves++] = Board::getMove(k0 MN,Board::STEPINDEX[k1][k0]);
			}
		}
	}

	return numMoves;
}

//TODO optimize this and other freecap functions by sharing the src and dest changes array?
//Checks if making move in prev resulting in b is reversable.
//Returns 0 if not reversible, 1 if we can reverse 3 step, 2 if we can reverse fully
int SearchUtils::isReversible(const Board& prev, move_t move, const Board& b, move_t& reverseMove)
{
	//By default
	reverseMove = ERRORMOVE;

	//Arrays to track pieces and where they went
	int num = 0;
	loc_t src[8];
	loc_t dest[8];
	num = prev.getChanges(move,src,dest);

	pla_t pla = b.player;

	//Ensure that too many changes did not occur
	if(num > 2)
	  return 0;

	//One change
	if(num == 1)
	{
	  loc_t k0 = dest[0];
	  loc_t k1 = src[0];
		//Ensure that the change was a player piece, it is unfrozen, and the dest is empty
		if(b.owners[k0] != pla || b.isFrozen(k0) || b.owners[k1] != NPLA)
		  return 0;

		//One step away
		if(Board::MANHATTANDIST[k1][k0] == 1)
		{
			//Can reverse if no rabbit restrictions
			if(b.isRabOkay(pla,k0,k1))
			  return 2;
		}
		//Two steps away
		else if(Board::MANHATTANDIST[k1][k0] == 2)
		{
			int dx = (k1&7) - (k0&7);
			int dy = ((k1>>3) - (k0>>3))*8;
			int dk = (dx+dy)/2;
			//Diagonal step
			if(dx != 0 && dy != 0)
			{
				//Try both paths back
				if(b.owners[k0+dx] == NPLA && b.isTrapSafe2(pla,k0+dx) && b.wouldBeUF(pla,k0,k0+dx,k0) && b.isRabOkay(pla,k0+dx,k1))
				  return 2;
				if(b.owners[k0+dy] == NPLA && b.isTrapSafe2(pla,k0+dy) && b.wouldBeUF(pla,k0,k0+dy,k0) && b.isRabOkay(pla,k0,k0+dy))
				  return 2;

				//Try both paths with a piece swap:
				if(b.owners[k0+dx] == pla && b.pieces[k0+dx] == b.pieces[k0] && b.isTrapSafe2(pla,k1) &&
				   b.wouldBeUF(pla,k0,k0,k0+dx) && b.isRabOkay(pla,k0+dx,k1))
				  return 2;
				if(b.owners[k0+dy] == pla && b.pieces[k0+dy] == b.pieces[k0] && b.isTrapSafe2(pla,k1) &&
				   b.wouldBeUF(pla,k0,k0,k0+dy) && b.isRabOkay(pla,k0,k0+dy))
				  return 2;
			}
			//Double straight step
			else if(dk != 0)
			{
				//Try stepping back
				if(b.owners[k0+dk] == NPLA && b.isTrapSafe2(pla,k0+dk) && b.wouldBeUF(pla,k0,k0+dk,k0) && b.isRabOkay(pla,k0,k0+dk))
				  return 2;
			}
		}
		return 0;
	}
	//Two changes
	else if(num == 2)
	{
		//Ensure both changes are a single step
		if(Board::MANHATTANDIST[src[0]][dest[0]] != 1 || Board::MANHATTANDIST[src[1]][dest[1]] != 1)
		  return 0;

		//Ensure at least one piece is a player
		if(b.owners[dest[0]] != pla && b.owners[dest[1]] != pla)
		  return 0;

		//If both pieces are players
		if(b.owners[dest[0]] == pla && b.owners[dest[1]] == pla)
		{
			loc_t k0 = dest[0];
			loc_t j0 = dest[1];
			loc_t k1 = src[0];
			loc_t j1 = src[1];
			//Try stepping both back
			if(b.isThawed(k0) && b.isThawed(j0) && b.isRabOkay(pla,k0,k1) && b.isRabOkay(pla,j0,j1))
			{
				if(k1 == j0 && b.owners[j1] == NPLA && b.wouldBeUF(pla,k0,k0,k1))
				  return 2;
				if(j1 == k0 && b.owners[k1] == NPLA && b.wouldBeUF(pla,j0,j0,j1))
				  return 2;
				if(b.owners[k1] == NPLA && b.owners[j1] == NPLA && (b.wouldBeUF(pla,k0,k0,j0) || b.wouldBeUF(pla,j0,j0,k0)))
				  return 2;
			}
		}
		//One player one opponent
		else
		{
			loc_t k0,k1,o0,o1;
			if(b.owners[dest[0]] == pla){k0 = dest[0]; o0 = dest[1]; k1 = src[0]; o1 = src[1];}
			else                        {k0 = dest[1]; o0 = dest[0]; k1 = src[1]; o1 = src[0];}

			//Try pushing/pulling the opponent back
			if(b.pieces[k0] > b.pieces[o0] && b.isThawed(k0) && ((k1 == o0 && b.owners[o1] == NPLA) || (o1 == k0 && b.owners[k1] == NPLA)))
			  return 2;

			//Can only step back our own piece
			if(b.isThawed(k0) && b.owners[k1] == NPLA && b.isRabOkay(pla,k0,k1))
			{reverseMove = Board::getMove(Board::STEPINDEX[k0][k1]); return 1;}
		}
		return 0;
	}
	return 0;
}

//Checks if the last move, by the opponent, is reversable.
//Returns 0 if not reversible, 1 if we can reverse 3 step, 2 if we can reverse fully
int SearchUtils::isReversible(const Board& b, const BoardHistory& boardHistory, move_t& reverseMove)
{
	//By default
	reverseMove = ERRORMOVE;

	//Make sure we call only when current step is 0
	if(b.step != 0)
		return 0;

	//Make sure last turn exists
	int lastTurn = b.turnNumber-1;
	if(lastTurn < boardHistory.minTurnNumber)
		return 0;

	//Ensure no pieces died last turn
	if(boardHistory.turnPieceCount[GOLD][lastTurn] + boardHistory.turnPieceCount[SILV][lastTurn] !=
		b.pieceCounts[GOLD][0] + b.pieceCounts[SILV][0])
	  return 0;

	return isReversible(boardHistory.turnBoard[lastTurn], boardHistory.turnMove[lastTurn], b, reverseMove);
}

bool SearchUtils::losesRepetitionFight(const Board& b, const BoardHistory& boardHistory, int cDepth, int& turnsToLose)
{
	if(!SearchParams::ENABLE_REP_FIGHT_CHECK || cDepth < 8 || b.step != 0)
		return false;

	hash_t currentHash = b.sitCurrentHash;
	int currentTurn = b.turnNumber;
	int startTurn = b.turnNumber - (cDepth/4);

	DEBUGASSERT(currentTurn-1 <= boardHistory.maxTurnNumber && startTurn >= boardHistory.minTurnNumber);

	//Walk backwards to see if we repeat any position occuring within the current search
	int repTurn = -1;
	for(int t = currentTurn-2; t >= startTurn; t -= 2)
	{
		//Found a rep?
		if(currentHash == boardHistory.turnSitHash[t])
		{
			repTurn = t;
			break;
		}
	}

	if(repTurn == -1)
		return false;

	//Now see if we will lose this repetition fight
	//Loop forward over the repetitions and check who will have to deviate first
	int pieceCount = b.pieceCounts[GOLD][0] + b.pieceCounts[SILV][0];
	for(int t = repTurn; t < currentTurn; t++)
	{
		//If there is any instance in the past, we lose.
		bool foundPastInstance = false;
		hash_t hash = boardHistory.turnSitHash[t];
		for(int pt = t-2; pt >= boardHistory.minTurnNumber; pt--)
		{
			const Board& oldBoard = boardHistory.turnBoard[pt];
			if(pieceCount != oldBoard.pieceCounts[GOLD][0] + oldBoard.pieceCounts[SILV][0])
				break;
			if(boardHistory.turnSitHash[pt] == hash)
			{
				foundPastInstance = true;
				break;
			}
		}
		if(foundPastInstance)
		{
			turnsToLose = t - repTurn;
			return true;
		}

		//Next!
		t++;
		if(t >= currentTurn)
			break;

		//If there is any instance in the past for the opponent, he loses and we win
		foundPastInstance = false;
		hash = boardHistory.turnSitHash[t];
		for(int pt = t-2; pt >= boardHistory.minTurnNumber; pt--)
		{
			const Board& oldBoard = boardHistory.turnBoard[pt];
			if(pieceCount != oldBoard.pieceCounts[GOLD][0] + oldBoard.pieceCounts[SILV][0])
				break;
			if(boardHistory.turnSitHash[pt] == hash)
			{
				foundPastInstance = true;
				break;
			}
		}
		if(foundPastInstance)
			return false;
	}

	//Everything was okay. Then we lose because we can't repeat it yet again.
	turnsToLose = currentTurn - repTurn;
	return true;
}

/*
Dependencies:
push *
pull *
step to unfreeze subsequent stepping piece
vacate square for subsequent stepping piece
move same piece twice *
support a trap that the second piece to move was supporting
*/

//Call at step 3 of a search. Not all three steps can be independent.
bool SearchUtils::isStepPrunable(const Board& b, const BoardHistory& boardHistory, int cDepth)
{
	//Ensure proper step
	if(b.step != 3)
	  return false;

	//Make sure this turn exists
	int lastTurn = b.turnNumber;
	if(lastTurn < boardHistory.minTurnNumber)
		return 0;

	//Ensure no pieces died this turn
	if(boardHistory.turnPieceCount[GOLD][lastTurn] + boardHistory.turnPieceCount[SILV][lastTurn] !=
		b.pieceCounts[GOLD][0] + b.pieceCounts[SILV][0])
	  return 0;

	//Arrays to track pieces and where they went
	int num = 0;
	loc_t src[8];
	loc_t dest[8];
	const Board& startBoard = boardHistory.turnBoard[lastTurn];
	num = startBoard.getChanges(boardHistory.turnMove[lastTurn],src,dest);

	pla_t pla = b.player;

	//Ensure all same player
	if(num != 3 || b.owners[dest[0]] != pla || b.owners[dest[1]] != pla || b.owners[dest[2]] != pla)
	  return false;

	//Check for dependencies:

	//Empty square to step other piece on
	if(src[0] == dest[1] || src[0] == dest[2] || src[1] == dest[2])
	  return false;

	//Leave trap for other piece to guard
	if(Board::ADJACENTTRAP[src[0]] != ERRORSQUARE && (Board::ADJACENTTRAP[src[0]] == Board::ADJACENTTRAP[dest[1]] || Board::ADJACENTTRAP[src[0]] == Board::ADJACENTTRAP[dest[2]]))
	  return false;
	if(Board::ADJACENTTRAP[src[1]] != ERRORSQUARE && Board::ADJACENTTRAP[src[1]] == Board::ADJACENTTRAP[dest[2]])
	  return false;


	//Step off trap that other piece leaves
	if(Board::ISTRAP[src[0]] && (Board::ADJACENTTRAP[src[1]] == src[0] || Board::ADJACENTTRAP[src[2]] == src[0]))
	  return false;
	if(Board::ISTRAP[src[1]] && Board::ADJACENTTRAP[src[2]] == src[1])
	  return false;

	//Unfreeze other piece
	if(startBoard.isFrozen(src[1]) || startBoard.isFrozen(src[2]))
	  return false;
	if(!startBoard.wouldBeUF(pla,src[2],src[2],src[0]))
	  return false;

	return true;
}


//Returns 0 if no free capture, 1 if we can capture something all but one step was spent on, 2 if we can capture an all-step spend
int SearchUtils::isFreeCapturable(const Board& b, move_t move, Board& finalBoard)
{
	//Arrays to track pieces and where they went
  loc_t src[8];
  loc_t dest[8];
  int num = b.getChanges(move,src,dest);

  //Ensure that too many or too few changes did not occur
  if(num > 2 || num <= 0)
    return 0;

  pla_t pla = OPP(b.player);
  pla_t opp = b.player;
  bool freeCapFound = false;

  for(int i = 0; i<num; i++)
  {
    loc_t oloc = dest[i];
    //Captured piece or a push/pull happened
    if(oloc == ERRORSQUARE || finalBoard.owners[oloc] != opp)
      return 0;

    //Ensure it is next to an opp unsafe trap
    loc_t kt = Board::ADJACENTTRAP[oloc];
    if(kt != ERRORSQUARE && !finalBoard.isTrapSafe2(opp,kt))
    {
			//Check if it can now be captured.
			Bitmap capMap;
			int capDist;
			if(BoardTrees::canCaps(finalBoard,pla,4,kt,capMap,capDist))
			{
				if(capMap.isOne(oloc))
				{
					freeCapFound = true;
					continue;
				}
			}
    }

    //We found a step that isn't capturable, make sure it only moved 1 space
    if(!Board::ISADJACENT[src[i]][dest[i]])
    	return 0;
  }

  if(freeCapFound)
  	return num == 1 ? 2 : 1;

  return 0;
}

//KILLER MOVES---------------------------------------------------

//Might not be legal!
move_t SearchUtils::getKiller(const move_t* killerMoves, int killerMovesLen, const Board& b, int cDepth)
{
	//Only do killer moves at step 0, so that we don't repeatedly try them over and over if they happen to not be good.
	//TODO try killer moves upon entry to qsearch also?
	if(b.step != 0)
		return ERRORMOVE;

	if(cDepth >= killerMovesLen)
		return ERRORMOVE;

	return killerMoves[cDepth];
}

void SearchUtils::recordKiller(move_t* killerMoves, int& killerMovesLen, int cDepth, const move_t* pv, int pvLen, const Board& b)
{
	//Only do killer moves at step 0, so that we don't repeatedly try them over and over if they happen to not be good.
	if(b.step != 0)
		return;

	if(pvLen <= 0)
		return;

	if(killerMovesLen <= cDepth)
	{
	  int oldLen = killerMovesLen;
	  int newLen = cDepth+1;
	  for(int i = oldLen; i<newLen; i++)
	    killerMoves[i] = ERRORMOVE;
	  killerMovesLen = newLen;
	}

	move_t move = pv[0];
	if(move == QPASSMOVE || move == PASSMOVE)
	{killerMoves[cDepth] = ERRORMOVE; return;}

	int ns = Board::numRealStepsInMove(move);
	for(int i = 1; i<pvLen; i++)
	{
		if(ns >= 4)
			break;
		move_t pvMove = pv[i];
		if(pvMove == QPASSMOVE || move == PASSMOVE)
			break;

		move = Board::concatMoves(move,pvMove,ns);
		ns = Board::numRealStepsInMove(move);
	}

	killerMoves[cDepth] = move;
}


//HELPERS - HISTORY---------------------------------------------------------------------

//TODO make this distinguish pushpulls from consecutive nearby steps more perfectly
static int getHistoryIndex(move_t move)
{
	int ns = Board::numStepsInMove(move);

	step_t step0 = Board::getStep(move,0);
	if(ns <= 1)
		return (int)step0;

	step_t step1 = Board::getStep(move,1);
	if(step1 == PASSSTEP || step1 == QPASSSTEP || Board::K1INDEX[step1] != Board::K0INDEX[step0] ||
			Board::K1INDEX[step0] == Board::K0INDEX[step1])
		return (int)step0;

	return (int)step0 + (int)step1/64*256+256;
}

static int getHistoryDepth(int cDepth)
{
	return cDepth;
}

//HISTORY TABLE---------------------------------------------------
//Ensure that the history table has been allocated up to the given requested depth of search.
//Not threadsafe, call only before search!
void SearchUtils::ensureHistory(vector<long long*>& historyTable, vector<long long>& historyMax, int depth)
{
	if(depth > SearchParams::HISTORY_MAX_DEPTH)
		depth = SearchParams::HISTORY_MAX_DEPTH;

	while((int)historyTable.size() < depth + SearchParams::QMAX_CDEPTH)
	{
		long long* arr = new long long[SearchParams::HISTORY_LEN];
		for(int j = 0; j<SearchParams::HISTORY_LEN; j++)
			arr[j] = 0;
		historyTable.push_back(arr);
		historyMax.push_back(0);
	}
}

//Call whenever we want to clear the history table
void SearchUtils::clearHistory(vector<long long*>& historyTable, vector<long long>& historyMax)
{
	int size = historyTable.size();
	for(int i = 0; i<size; i++)
	{
		historyMax[i] = 0;
		for(int j = 0; j<SearchParams::HISTORY_LEN; j++)
			historyTable[i][j] = 0;
	}
}

//Cut all history scores down, call between iterations of iterative deepening
void SearchUtils::decayHistory(vector<long long*>& historyTable, vector<long long>& historyMax)
{
	int size = historyTable.size();
	for(int i = 0; i<size; i++)
	{
		historyMax[i] = (historyMax[i]+1)/2;
		for(int j = 0; j<SearchParams::HISTORY_LEN; j++)
			historyTable[i][j] = (historyTable[i][j]+1)/6;
	}
}

void SearchUtils::updateHistoryTable(vector<long long*>& historyTable, vector<long long>& historyMax,
		const Board& b, int cDepth, move_t move)
{
	if(move == ERRORMOVE)
		return;

	int dIndex = getHistoryDepth(cDepth);
	if((int)historyTable.size() <= dIndex)
		return;

	int hIndex = getHistoryIndex(move);
	long long val = ++(historyTable[dIndex][hIndex]);
	if(val > historyMax[dIndex])
		historyMax[dIndex] = val;
}

void SearchUtils::getHistoryScores(const vector<long long*>& historyTable, const vector<long long>& historyMax,
		const Board& b, pla_t pla, int cDepth, const move_t* mv, int* hm, int len)
{
	int dIndex = getHistoryDepth(cDepth);
	DEBUGASSERT((int)historyTable.size() > dIndex)

	long long histMax = historyMax[dIndex];
	if(histMax == 0)
		return;

	long long* histTable = historyTable[dIndex];
	for(int m = 0; m < len; m++)
	{
		move_t move = mv[m];

		int hIndex = getHistoryIndex(move);
		long long score = histTable[hIndex];
		if(score > histMax)
			score = histMax;
		hm[m] += (int)(score*SearchParams::HISTORY_SCORE_MAX/histMax);
	}
}

int SearchUtils::getHistoryScore(const vector<long long*>& historyTable, const vector<long long>& historyMax,
		const Board& b, pla_t pla, int cDepth, move_t move)
{
	int dIndex = getHistoryDepth(cDepth);
	DEBUGASSERT((int)historyTable.size() > dIndex)

	long long histMax = historyMax[dIndex];
	if(histMax == 0)
		return 0;

	int hIndex = getHistoryIndex(move);
	long long score = historyTable[dIndex][hIndex];
	if(score > histMax)
		score = histMax;
	return (int)(score*SearchParams::HISTORY_SCORE_MAX/histMax);
}




/*

//Requires startingBoard array
void Searcher::pruneSuicides(Board& b, int cDepth, move_t* mvB, int* hmB, int num)
{
	pla_t pla = b.player;
	for(int i = 0; i < num; i++)
	{
		//Find first pla move
		int s = 0;
		step_t step;
		for(int s = 0; s<4; s++)
		{
			step = Board::getStep(mvB[i],s);
			if(step == ERRORSTEP || step == PASSSTEP)
			{s = 4; break;}
			if(b.owners[Board::K0INDEX[step]] == pla)
			{break;}
		}
		if(s == 4)
		{continue;}

		if(!b.isTrapSafe2(pla,Board::K1INDEX[step]))
		{hmB[i] = SUICIDEVAL;}
		else
		{
			loc_t kt = Board::CHECKCAPSAT[i];
			if(kt != ERRORSQUARE && b.owners[kt] == pla && !b.isTrapSafe2(pla,kt) && startingBoard[cDepth/4].owners[kt] != pla)
			{
				hmB[i] = SUICIDEVAL;
			}
		}
	}
}
*/


//PV-------------------------------------------------------------------------------

void SearchUtils::endPV(const move_t* pv, int& pvLen)
{
	pvLen = 0;
}

void SearchUtils::copyPV(const move_t* srcpv, int srcpvLen, move_t* destpv, int& destpvLen)
{
	for(int i = 0; i<srcpvLen; i++)
		destpv[i] = srcpv[i];
	destpvLen = srcpvLen;
}

void SearchUtils::copyExtendPV(move_t move, const move_t* srcpv, int srcpvLen, move_t* destpv, int& destpvLen)
{
	destpv[0] = move;
	int newLen = min(srcpvLen+1,SearchParams::PV_ARRAY_SIZE);
	for(int i = 1; i<newLen; i++)
		destpv[i] = srcpv[i-1];
	destpvLen = newLen;
}


//HASHTABLE-------------------------------------------------------------------------------------------

SearchHashEntry::SearchHashEntry()
{
	data = 0;
	key = 0;
}

void SearchHashEntry::record(hash_t hash, int16_t depth, eval_t eval, flag_t flag, move_t move)
{
	//Assert that eval uses only only 21 bits (except for sign extension)
	DEBUGASSERT((eval & 0xFFE00000) == 0 || (eval & 0xFFE00000) == 0xFFE00000);
	//Assert that flag uses only 2 bits
	DEBUGASSERT((flag & 0xFFFFFFFC) == 0);
	//Assert that depth uses only 9 bits (except for sign extension)
	DEBUGASSERT((depth & 0xFFFFFE00) == 0 || (depth & 0xFFFFFE00) == 0xFFFFFE00);

	//Put it all together!
	uint32_t subWord = ((uint32_t)eval << 11) | (((uint32_t)depth & 0x1FF) << 2) | (uint32_t)flag;
	uint64_t rData = ((uint64_t)move << 32) | subWord;

	data = rData;
	key = hash ^ rData;
}

bool SearchHashEntry::lookup(hash_t hash, int16_t& depth4, eval_t& eval, flag_t& flag, move_t& move)
{
	uint64_t rData = data;
	uint64_t rKey = key;

	//Verify that hash key matches, using lockless xor scheme
	if((rKey ^ rData) != hash)
		return false;

	uint32_t subWord = (uint32_t) rData;
	flag_t theFlag = (flag_t)(subWord & 0x3);

	//Ensure there's supposed to be an entry here
	if(theFlag == Flags::FLAG_NONE)
		return false;

	flag = theFlag;
	move = (move_t)(rData >> 32);
	depth4 = (int16_t)(((int32_t)(subWord << 21)) >> 23);
	eval = (eval_t)((int32_t)subWord >> 11);

	return true;
}

int SearchHashTable::getExp(uint64_t maxMem)
{
  uint64_t maxEntries = maxMem / sizeof(SearchHashEntry);
  int shift = 0;
  while(shift < 63 && (1ULL << (shift+1)) <= maxEntries)
    shift++;
  return max(shift,10);
}

SearchHashTable::SearchHashTable(int exp)
{
	exponent = exp;
	size = ((hash_t)1) << exponent;
	mask = size-1;
	entries = new SearchHashEntry[size];
}

SearchHashTable::~SearchHashTable()
{
	delete[] entries;
}

void SearchHashTable::clear()
{
	for(hash_t i = 0; i<size; i++)
		entries[i] = SearchHashEntry();
}

bool SearchHashTable::lookup(move_t& hashMove, eval_t& hashEval, int16_t& hashDepth4, flag_t& hashFlag, const Board& b, int cDepth)
{
	//Compute appropriate slot
  hash_t hash = b.sitCurrentHash;
  int hashSlot = (int)(hash & mask);

  //Attempt to look up the entry
  if(entries[hashSlot].lookup(hash,hashDepth4,hashEval,hashFlag,hashMove))
  {
  	//Adjust for terminal eval
    if(SearchUtils::isTerminalEval(hashEval))
    {
    	int wlDepth = hashEval > 0 ? Eval::WIN - hashEval : hashEval - Eval::LOSE;
    	wlDepth += cDepth;
    	hashEval = hashEval > 0 ? Eval::WIN - wlDepth : Eval::LOSE + wlDepth;
    }
    return true;
  }
  return false;
}

void SearchHashTable::record(const Board& b, int cDepth, int16_t depth4, eval_t eval, flag_t flag, move_t move)
{
	//Don't ever record evals that were obtained through some sort of illegal move check
	if(SearchUtils::isIllegalityTerminalEval(eval))
		return;

	//Adjust for win loss scores
	if(SearchUtils::isTerminalEval(eval))
	{
		int wlDepth = eval > 0 ? Eval::WIN - eval : eval - Eval::LOSE;
		wlDepth -= cDepth;
		if(wlDepth < 0)
			wlDepth = 0; //Just in case
		eval = eval > 0 ? Eval::WIN - wlDepth : Eval::LOSE + wlDepth;
	}

	//Record the hash!
  hash_t hash = b.sitCurrentHash;
	int hashSlot = (int)(hash & mask);
	entries[hashSlot].record(hash,depth4,eval,flag,move);
}

ExistsHashTable::ExistsHashTable(int exp)
{
	if(exp > 21)
		Global::fatalError("Cannot have ExistsHashTable size with exp > 21!");

	exponent = exp;
	size = ((hash_t)1) << exponent;
	mask = size-1;
	hashKeys = new hash_t[size];
	clear();
}

ExistsHashTable::~ExistsHashTable()
{
	delete[] hashKeys;
}

void ExistsHashTable::clear()
{
	//For entry 0, put a hash value that is nonzero everywhere (so it couldn't possibly go there under)
	//normal operation.
	//For all other entries, put a hash value that is zero everywhere, which couldn't possibly go there under
	//normal operation
	hashKeys[0] = 0xFFFFFFFFFFFFFFFFULL;
	for(hash_t i = 1; i<size; i++)
		hashKeys[i] = 0;
}

bool ExistsHashTable::lookup(const Board& b)
{
	hash_t hash = b.sitCurrentHash;
	int hashIndex = (int)(hash & mask);
	int hashIndex2 = (int)((hash >> exponent) & mask);
	int hashIndex3 = (int)((hash >> (exponent*2)) & mask);
	if(hashKeys[hashIndex] == hash || hashKeys[hashIndex2] == hash || hashKeys[hashIndex3] == hash)
		return true;

	return false;
}

void ExistsHashTable::record(const Board& b)
{
  hash_t hash = b.sitCurrentHash;
	int hashIndex = (int)(hash & mask);
	int hashIndex2 = (int)((hash >> exponent) & mask);
	int hashIndex3 = (int)((hash >> (exponent*2)) & mask);
	hashKeys[hashIndex] = hash;
	hashKeys[hashIndex2] = hash;
	hashKeys[hashIndex3] = hash;
}

