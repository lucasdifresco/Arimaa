
/*
 * search.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <ctime>
#include "global.h"
#include "timer.h"
#include "bitmap.h"
#include "board.h"
#include "boardmovegen.h"
#include "boardtrees.h"
#include "eval.h"
#include "featuremove.h"
#include "search.h"
#include "searchparams.h"
#include "searchutils.h"
#include "searchthread.h"
#include "timecontrol.h"
#include "searchflags.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;


//SEARCHER INITIALIZATION-----------------------------------------------------------------

Searcher::Searcher()
{
	init();
}

Searcher::Searcher(SearchParams p)
{
	params = p;
	init();
}

void Searcher::init()
{
	doOutput = false;
	mainPla = NPLA;
	fullmoveHash = new ExistsHashTable(params.fullMoveHashExp);

	clockDesiredTime = 0;
	interrupted = false;
	cannotInterrupt = false;

	idpv = new move_t[SearchParams::PV_ARRAY_SIZE];
	idpvLen = 0;

	mainHash = new SearchHashTable(params.mainHashExp);

	fullMv = NULL;
	fullHm = NULL;
	fullMvCapacity = 0;

	searchTree = NULL;
}

Searcher::~Searcher()
{
	delete fullmoveHash;
	delete[] idpv;
	delete mainHash;

	for(int i = 0; i<(int)historyTable.size(); i++)
		delete[] historyTable[i];

	delete[] fullMv;
	delete[] fullHm;

	delete searchTree;
}

//ID AND TOP LEVEL SEARCH-----------------------------------------------------------------

vector<move_t> Searcher::getIDPV()
{
	vector<move_t> moves;
	for(int i = 0; i<idpvLen; i++)
		moves.push_back(idpv[i]);
	return moves;
}

vector<move_t> Searcher::getIDPVFullMoves()
{
	vector<move_t> moves;
	Board copy = mainBoard;
	move_t moveSoFar = ERRORMOVE;
	int ns = 0;
	pla_t pla = copy.player;
	for(int i = 0; i<idpvLen; i++)
	{
		bool suc = copy.makeMoveLegal(idpv[i]);
		DEBUGASSERT(suc);

		moveSoFar = Board::concatMoves(moveSoFar,idpv[i],ns);
		ns = Board::numStepsInMove(moveSoFar);
		if(copy.player != pla)
		{
			moves.push_back(moveSoFar);
			moveSoFar = ERRORMOVE;
			ns = 0;
			pla = copy.player;
		}
	}

	if(ns != 0)
		moves.push_back(moveSoFar);

	return moves;
}

//Get the best move
move_t Searcher::getMove()
{
	step_t steps[4];
	int numSteps = 0;
	for(int i = 0; i<idpvLen; i++)
	{
		bool done = false;
	  move_t move = idpv[i];
		for(int j = 0; j<4; j++)
		{
			step_t step = Board::getStep(move,j);
			if(step == ERRORSTEP)
			{break;}

			steps[numSteps++] = step;
			if(numSteps >= 4 || step == PASSSTEP || step == QPASSSTEP)
			{done = true; break;}
		}
		if(done)
			break;
	}

	switch(numSteps)
	{
	case 0: return PASSMOVE;
	case 1: return Board::getMove(steps[0],PASSSTEP);
	case 2: return Board::getMove(steps[0],steps[1],PASSSTEP);
	case 3: return Board::getMove(steps[0],steps[1],steps[2],PASSSTEP);
	case 4: return Board::getMove(steps[0],steps[1],steps[2],steps[3]);
	default: return ERRORMOVE;
	}
}

void Searcher::searchID(const Board& b, const BoardHistory& hist, bool output)
{
	searchID(b,hist,params.defaultMaxDepth,params.defaultMaxTime,output);
}

void Searcher::searchByTimeControl(const Board& b, const BoardHistory& hist, TimeControl t, bool output)
{
	searchByTimeControl(b,hist,params.defaultMaxDepth,params.defaultMaxTime,t,output);
}

void Searcher::searchByTimeControl(const Board& b, const BoardHistory& hist, int depth, double maxSeconds, TimeControl tc, bool output)
{
	double reserveDelta = tc.reserveCurrent - (SearchParams::TIME_RESERVE_PERMOVE_FACTOR * tc.perMove + SearchParams::TIME_RESERVE_MIN);

	//Figure base amount out from reserve and permove
	double timeToSearch;
	if(reserveDelta < 0)
		timeToSearch = (tc.perMove - tc.alreadyUsed)*SearchParams::TIME_PERMOVE_PROP + reserveDelta*SearchParams::TIME_RESERVE_NEG_PROP;
	else
		timeToSearch = (tc.perMove - tc.alreadyUsed)*SearchParams::TIME_PERMOVE_PROP + reserveDelta*SearchParams::TIME_RESERVE_POS_PROP;

	//Try to make at least some proportion of the permove, even if low reserve
  if(timeToSearch < SearchParams::TIME_FORCED_PROP * tc.perMove)
  	timeToSearch = SearchParams::TIME_FORCED_PROP * tc.perMove;

  //Use a little less
  timeToSearch -= SearchParams::TIME_LAG_ALLOWANCE;

  //Make sure we won't run out of time, leaving a buffer
  double maxTimeFromTC = min((double)(tc.perMoveMax - tc.alreadyUsed),(double)(tc.perMove - tc.alreadyUsed + tc.reserveCurrent));
  if(timeToSearch > maxTimeFromTC-SearchParams::PER_MOVE_MAX_ALLOWANCE)
    timeToSearch = maxTimeFromTC-SearchParams::PER_MOVE_MAX_ALLOWANCE;

  //But take some minimum amount of time anyways
  if(timeToSearch < SearchParams::TIME_MIN)
  	timeToSearch = SearchParams::TIME_MIN;

  //And apply the command line cap
	if(timeToSearch > maxSeconds)
		timeToSearch = maxSeconds;

  //Make sure we won't waste time by overflowing the reserve
  double hardMinTime = tc.reserveMax <= 0 ? 0 : tc.reserveCurrent + tc.perMove - tc.alreadyUsed - (tc.reserveMax - 1);
	double optimisticTime = timeToSearch;
  double hardMaxTime = maxTimeFromTC - SearchParams::PER_MOVE_MAX_ALLOWANCE;
  if(hardMinTime > hardMaxTime)
    hardMinTime = hardMaxTime;
  if(optimisticTime < hardMinTime)
    optimisticTime = hardMinTime;
  if(optimisticTime > hardMaxTime)
    optimisticTime = hardMaxTime;

	searchID(b, hist, depth, hardMinTime, optimisticTime, hardMaxTime, output);
}

void Searcher::interruptExternal()
{
  //Mark as having no time left - effectively allowing interrupt to happen properly from within.
  clockHardMaxTime = 0;
  clockHardMinTime = 0;
  clockDesiredTime = 0;
}

//Helpers-----------------------------------------------------

struct RatedMove
{
	move_t move;
	double val;

	//A move comes earlier in order if it's rating val is higher
	bool operator<(const RatedMove& other) const
	{return val > other.val;}
};

//Iterative deepening search-------------------------------------------------------------

void Searcher::searchID(const Board& b, const BoardHistory& hist, int depth, double seconds)
{
  searchID(b,hist,depth,seconds,false);
}

void Searcher::searchID(const Board& b, const BoardHistory& hist, int depth, double seconds, bool output)
{
  searchID(b,hist,depth,seconds,seconds,seconds,output);
}

void Searcher::searchID(const Board& b, const BoardHistory& hist, int depth,
    double hardMinSeconds, double seconds, double hardMaxSeconds, bool output)
{
  //Ensure the board is well-formed
  if(!b.testConsistency(cout))
  	Global::fatalError("searchID given inconsistent board!");

	//Special case for time
	if(hardMaxSeconds <= 0)
	{
    hardMinSeconds = 100000000.0;
	  seconds = 100000000.0;
    hardMaxSeconds = 100000000.0;
	}

	//Compute desired depth
	int nsInTurn =  4-b.step;

	//Initialize stuff
	doOutput = output;
	mainPla = b.player;
	mainBoard = b;
	mainBoardHistory = hist;
	stats = SearchStats();

	clockTimer.reset();
	clockDesiredTime = seconds;
	clockBaseEval = Eval::LOSE;
	clockOptimisticTime = seconds;
	clockHardMinTime = hardMinSeconds;
	clockHardMaxTime = hardMaxSeconds;
	interrupted = false;

	SearchUtils::endPV(idpv,idpvLen);

	mainHash->clear();
	SearchUtils::ensureHistory(historyTable,historyMax,max(depth,0));
	SearchUtils::clearHistory(historyTable,historyMax);

	//Check if the game is over. If so, we're done!
	Board bCopy = b;
	eval_t gameEndVal = SearchUtils::checkGameEndConditions(bCopy,hist,0);
	if(gameEndVal != 0)
	{
		stats.timeTaken = clockTimer.getSeconds();
		stats.depthReached = 0;
		stats.finalEval = gameEndVal;
		stats.pvString = string();
		return;
	}

	//Initialize search tree, which also spawns off threads ready to help when the search starts
	DEBUGASSERT(searchTree == NULL);
	int maxMSearchDepth =  max(depth,0)+SearchParams::MAX_MSEARCH_DEPTH_OVER_NOMINAL;
	int maxCDepth = maxMSearchDepth + SearchParams::QMAX_CDEPTH;
	searchTree = new SearchTree(this, params.numThreads, maxMSearchDepth, maxCDepth, b, mainBoardHistory);

	eval_t alpha = Eval::LOSE-1;
  eval_t beta = Eval::WIN-1;

	//Qsearch directly if the depth is negative
	if(depth <= 0)
	{
		cannotInterrupt = true;
		bCopy = b;
		directQSearch(bCopy,depth,alpha,beta);
	}
	//Normal main search
	else
	{
		//Generate moves
		vector<move_t> mvVec;
		int startDepth = min(depth,nsInTurn);
		bool doWLPrune = params.enableGoalTree;
		SearchUtils::genFullMoves(b, mainBoardHistory, mvVec, startDepth, doWLPrune, fullmoveHash);

		if(params.randomize)
			SearchUtils::shuffle(params.randSeed,mvVec);

		//BT MOVE ORDERING----------------
		if(params.rootMoveFeatureSet.fset != NULL && !params.stupidPrune)
		{
			FeaturePosData moveFeatureData;
			params.rootMoveFeatureSet.getPosData(b,mainBoardHistory,mainPla,moveFeatureData);

			//Sort moves!
			int size = mvVec.size();
			RatedMove* rmoves = new RatedMove[size];

			for(int m = 0; m < size; m++)
			{
				move_t move = mvVec[m];
				rmoves[m].move = move;
				rmoves[m].val = params.rootMoveFeatureSet.getFeatureSum(
						bCopy,moveFeatureData,mainPla,move,mainBoardHistory,params.rootMoveFeatureWeights);
			}

			std::stable_sort(rmoves,rmoves+size);

			for(int m = 0; m < size; m++)
				mvVec[m] = rmoves[m].move;

			delete[] rmoves;
		}

		//Convert to arrays
		int numMoves = mvVec.size();
		SearchUtils::ensureMoveArray(fullMv,fullHm,fullMvCapacity,numMoves);
		fullNumMoves = numMoves;
		for(int i = 0; i<fullNumMoves; i++)
		{
			fullMv[i] = mvVec[i];
			fullHm[i] = Eval::LOSE - 1;
		}

		//Initial search, unbreakable
		cannotInterrupt = true;
		bCopy = b;
		int numFinished = 0;
		fsearch(bCopy,startDepth*SearchParams::DEPTH_DIV,alpha,beta,numFinished);

		if(doOutput)
		{
		  double timeUsed = currentTimeUsed();
		  double desiredTime = currentDesiredTime();
		  cout << Global::strprintf("ID Depth:  %-5.4g Eval: % 6d Time: (%.2f/%.2f)", stats.depthReached, stats.finalEval, timeUsed, desiredTime)
           << " PV: " << stats.pvString << endl << endl;
    }

		//If win or loss proven, stop now
    if(SearchUtils::isTrustworthyTerminalEval(stats.finalEval,b.step,0,startDepth*SearchParams::DEPTH_DIV))
			depth = startDepth;

		//Iteratively search deeper
		cannotInterrupt = false;
		for(int d = startDepth+1; d <= depth; d++)
		{
			SearchUtils::decayHistory(historyTable,historyMax);

			bCopy = b;
			numFinished = 0;
			fsearch(bCopy,d*SearchParams::DEPTH_DIV,alpha,beta,numFinished);

			if(doOutput && numFinished > 0)
      {
        double timeUsed = currentTimeUsed();
        double desiredTime = currentDesiredTime();
        cout << Global::strprintf("ID Depth:  %-5.4g Eval: % 6d Time: (%.2f/%.2f)", stats.depthReached, stats.finalEval, timeUsed, desiredTime)
             << " PV: " << stats.pvString << endl << endl;
      }

			//Ran out of time, search interrupted
			if(interrupted)
				break;

			//If win or loss proven, stop now. Note that we can't necessarily trust terminal evals that are too deep compared
			//to the rdepth. For example, imagine we have a D5 search:
			//CDepth 0-3 threatens a win in 2 that is a capture, if not defended properly.
			//CDepth 4 cant defend it alone because more than one step needed to defend it.
			//CDepth 5-7 (QD0) fails to guard it because it doesn't see a goal threat and doesn't gen all moves, and doesn't gen capture
			//defenses perfectly
			//CDepth 8-11 (QD1) executes the capture that is the win in 2
			//CDepth 12-15 (QD2) fails to defend in windefsearch
			//So we falsely think that the move was a win!
			if(SearchUtils::isTrustworthyTerminalEval(stats.finalEval,b.step,0,d*SearchParams::DEPTH_DIV))
				break;

			//If we really don't have time left for the next iter, then stop now.
			double used = clockTimer.getSeconds();
			if(params.stopEarlyWhenLittleTime && clockDesiredTime - used < used * SearchParams::TIME_PREV_FACTOR)
			  break;
		}
	}

	//The whole search is done. The search tree destructor signals and waits for the helper threads to
	//terminate completely
	DEBUGASSERT(searchTree != NULL);
	delete searchTree;
	searchTree = NULL;

	//Update data
	stats.timeTaken = clockTimer.getSeconds();
}


//FSEARCH-----------------------------------------------------------------------------

//Perform search and stores best eval found in stats.finalEval, behaving appropriately depending on allowance
//of partial search. Similarly, fills in idpv with best pv so far
//Marks numFinished with how many moves finished before interruption
//Uses and modifies fullHm for ordering
//Whenever a new move is best, stores its eval in fullHm, but for moves not best, stores Eval::LOSE-1.
//And at the end, sorts the moves
void Searcher::fsearch(const Board& b, int rDepth4, eval_t alpha, eval_t beta, int& numFinished)
{
	//Initialize stuff
	numFinished = 0;
	int cDepth = 0;

	if(params.viewing(b)) params.view(b,cDepth,rDepth4/4,alpha,beta);

	//Wipe out our hm array so that it can receive ordering values
	for(int i = 0; i<fullNumMoves; i++)
		fullHm[i] = Eval::LOSE-1;

	//Get thread struct for ourselves, we are the master thread
	SearchThread* curThread = searchTree->getMasterThread();

	//Create the splitpoint and do the search on it
	DEBUGASSERT(searchTree != NULL);
	SplitPoint* spt = searchTree->acquireRootNode();
	spt->init(0,mainBoardHistory.turnMove[b.turnNumber],SplitPoint::MODE_NORMAL,b,b.step == 0,
	    cDepth,rDepth4,alpha,beta,true,ERRORMOVE,ERRORMOVE,fullNumMoves,curThread->id);

	parallelFSearch(curThread,spt);

	//Combine stats
	stats.overwriteAggregates(searchTree->getCombinedStats());

	//Update parameters for time
	clockBaseEval = spt->bestEval;
  updateDesiredTime(spt->bestEval,rDepth4/4+1,0,fullNumMoves);

	//Update final stats data
	numFinished = spt->numMovesDone;
	if(numFinished > 0)
	{
		if(!params.disablePartialSearch || numFinished == spt->numMoves)
		{
			stats.finalEval = spt->bestEval;
			SearchUtils::copyPV(spt->pv,spt->pvLen,idpv,idpvLen);
			stats.pvString = writeMoves(b,getIDPV());
			stats.depthReached = rDepth4/SearchParams::DEPTH_DIV - 1 + (double)numFinished/spt->numMoves;
		}

		if(numFinished == spt->numMoves)
		{
			//Copy hm values back for move ordering
			for(int i = 0; i<spt->numMoves; i++)
				fullHm[i] = spt->hm[i];

			//Reorder moves based on what was best
			SearchUtils::insertionSort(fullMv,fullHm,fullNumMoves);
		}
	}

	searchTree->freeRootNode(spt);
}

void Searcher::directQSearch(const Board& b, int depth, eval_t alpha, eval_t beta)
{
	int fDepth = 0;
	int cDepth = 0;
	int qDepth = max(-depth, SearchParams::QDEPTH_START[b.step]);
	if(qDepth >= SearchParams::QDEPTH_EVAL)
		qDepth = SearchParams::QDEPTH_EVAL;

	if(params.viewing(b)) params.view(b,cDepth,-qDepth,alpha,beta);

	//Get thread struct for ourselves, we are the master thread
	SearchThread* curThread = searchTree->getMasterThread();

	Board copy = b;
	eval_t eval = qSearch(curThread, copy, fDepth, cDepth, qDepth, alpha, beta, SS_MAINLEAF);

	stats = searchTree->getCombinedStats();

	//Update final stats data
	stats.finalEval = eval;
	SearchUtils::copyPV(curThread->pv[0],curThread->pvLen[0],idpv,idpvLen);
	stats.pvString = writeMoves(b,getIDPV());
	stats.depthReached = -qDepth;
}


//MSEARCH-----------------------------------------------------------------------------------

static eval_t maybeFlip(eval_t eval, bool doFlip)
{
	return doFlip ? -eval : eval;
}
#define MSEARCH_RETURN_EVAL(x) {eval = maybeFlip((x),changedPlayer); return;}

void Searcher::mSearch(SearchThread* curThread, SplitPoint* spt, SplitPoint*& newSpt, eval_t& eval)
{
	//By default, if we return without setting these again
	newSpt = NULL;
	eval = Eval::LOSE-1;

	const Board& oldBoard = spt->board;
	int moveIdx = curThread->curSplitPointMoveIdx;
	move_t move = curThread->curSplitPointMove;

	DEBUGASSERT(move != ERRORMOVE);

	//Just in case we have a result, record an empty pv
	curThread->endPV(spt->fDepth + 1);

	//Make sure histories are synced
	DEBUGASSERT(
		curThread->boardHistory.minTurnNumber <= oldBoard.turnNumber &&
		curThread->boardHistory.maxTurnNumber >= oldBoard.turnNumber &&
		curThread->boardHistory.turnBoard[oldBoard.turnNumber].posCurrentHash == (oldBoard.step == 0? oldBoard.posCurrentHash : oldBoard.posStartHash)
	);

	//We have a result, and it's a loss because we're pruning it
	if(moveIdx >= spt->pruneThreshold)
	{
		if(params.viewing(oldBoard)) params.viewMovePruned(oldBoard, move);
		return;
	}

	//Compute appropriate amount of reduction
	int pruneReduction = -1;
	if(moveIdx >= spt->r2Threshold)	pruneReduction = 2;
	else if(moveIdx >= spt->r1Threshold) pruneReduction = 1;
	else if(moveIdx >= spt->pvsThreshold) pruneReduction = 0;

	//Prune reduction cuts the search entirely
	int depthLeft = spt->rDepth4 / SearchParams::DEPTH_DIV;
	if(pruneReduction > 0 && depthLeft - 1 - pruneReduction < 0)
		return;

	//Try the move!
	Board b = oldBoard;
	if(moveIdx == spt->hashMoveIndex)
	{
		bool suc = b.makeMoveLegal(move);
		if(!suc) return;
	}
	else if(moveIdx == spt->killerMoveIndex)
	{
		bool suc = b.makeMoveLegal(move);
		if(!suc) return;
	}
	else
	{
		bool suc = b.makeMoveLegal(move);
		if(!suc)
		{
			ARIMAADEBUG(SearchUtils::reportIllegalMove(b,move));
			return;
		}
	}

	//Cannot return to the same position
	if(b.posCurrentHash == b.posStartHash)
		return;

	bool changedPlayer = (b.player != oldBoard.player);
	int numSteps = changedPlayer ? 4-oldBoard.step : b.step-oldBoard.step;

	//Check if we lose a repetition fight because of this move
	int turnsToLoseByRep = 0;
	if(SearchUtils::losesRepetitionFight(b,curThread->boardHistory,spt->cDepth + numSteps,turnsToLoseByRep))
	{
		eval = Eval::LOSE + spt->cDepth + numSteps + turnsToLoseByRep*4;
		return;
	}

	//Add move to history and keep going
	curThread->boardHistory.reportMove(b,move,oldBoard.step);

	//BEGIN NEW POSITION------------------------------------------------------------------------
	curThread->stats.mNodes++;

	pla_t pla = b.player;
	int fDepth = spt->fDepth + 1;
	int cDepth = spt->cDepth + numSteps;
	int rDepth4 = spt->rDepth4 - (numSteps + max(pruneReduction,0)) * SearchParams::DEPTH_DIV;

	eval_t oldAlpha = spt->alpha;
	eval_t oldBeta = (pruneReduction >= 0) ? spt->alpha+1 : spt->beta;

	eval_t alpha = changedPlayer ? -oldBeta : oldAlpha;
	eval_t beta = changedPlayer ? -oldAlpha : oldBeta;

	//TIME CHECK-------------------------------
	tryCheckTime(curThread);
	if(curThread->isTerminated)
		MSEARCH_RETURN_EVAL(alpha);

	//END CONDITION - GAME RULES---------------------------
	eval_t gameEndVal = SearchUtils::checkGameEndConditions(b,curThread->boardHistory,cDepth);
	if(gameEndVal != 0)
		MSEARCH_RETURN_EVAL(gameEndVal);

	//END CONDITION - WINNING BOUNDS ----------------------

	//If winning in one move puts us below alpha, then we're certainly below alpha
	if(Eval::WIN - cDepth - 1 <= alpha)
		MSEARCH_RETURN_EVAL(alpha);

	//If losing in one move puts us above beta, then we're certainly above beta
	if(Eval::LOSE + cDepth + 1 >= beta)
		MSEARCH_RETURN_EVAL(beta);

	//END CONDITION - GOAL TREE AND ELIM TREE --------------------
	if(b.step == 0 && params.enableGoalTree)
	{
		int goalDist = BoardTrees::goalDist(b,pla,4-b.step);
		if(goalDist <= 4)
			MSEARCH_RETURN_EVAL(Eval::WIN - cDepth - goalDist);

		bool canElim = BoardTrees::canElim(b,pla,4-b.step);
		if(canElim)
			MSEARCH_RETURN_EVAL(Eval::WIN - cDepth - (4-b.step));
	}

	//END CONDITION - REVERSIBLES-----------------------------------
	//TODO
	//Redo reversibility and free capture testing to happen at move generation time for the top level.
	//(why?) Because we don't want some quirk of win/loss scores in the eval to cause
	//an illegal move to be chosen over a surely losing movez
	if(SearchParams::REVERSE_ENABLE && cDepth >= 4 && b.step == 0)
	{
		move_t reverseMove = ERRORMOVE;
		int rev = SearchUtils::isReversible(b, curThread->boardHistory, reverseMove);
		//Full Reverse
		if(rev == 2)
			MSEARCH_RETURN_EVAL(Eval::WIN);
	}

	//END CONDITION - FREECAPTURABLES-------------------------------
	//TODO can we construct a position where we can force the opponent to have only free capturables and such,
	//thereby concluding a win when there is no such win?
	if(SearchParams::FREECAP_PRUNE_ENABLE && cDepth >= 4)
	{
		//TODO weaken this so that we don't lose moves that offer free captures but win the game?
		//especially at the root...
		//what if the freecap move makes a goal threat???
		if(b.step == 0 && rDepth4 < 20)
		{
			int startTurn = b.turnNumber-1;
			if(startTurn >= curThread->boardHistory.minTurnNumber)
			{
				int fc = SearchUtils::isFreeCapturable(curThread->boardHistory.turnBoard[startTurn],
						curThread->boardHistory.turnMove[startTurn],b);
				if(fc > 0)
					MSEARCH_RETURN_EVAL(Eval::WIN);
			}
		}
	}

  //END CONDITION - MAX DEPTH-------------------------------------
  //Switch to quiescence
  if(rDepth4 <= 0)
  {
  	//Should fill in fDepth's pv, as desired
  	int qSearchEval = qSearch(curThread,b,fDepth,cDepth,SearchParams::QDEPTH_START[b.step],alpha,beta,SS_MAINLEAF);
    MSEARCH_RETURN_EVAL(qSearchEval);
  }

  //VIEW CHECK----------------------------------------------------
	if(params.viewing(b)) params.view(b,cDepth,rDepth4/4,alpha,beta);

	//END CONDITION - HASH LOOKUP--------------------------------------
	move_t hashMove = ERRORMOVE;
	if(SearchParams::HASH_ENABLE)
	{
		eval_t hashEval;
		int16_t hashDepth4;
		uint8_t hashFlag;
		//Found a hash?
		if(mainHash->lookup(hashMove,hashEval,hashDepth4,hashFlag,b,cDepth))
		{
			//Does the entry allow for a cutoff?
			if(hashDepth4 > 0 && cDepth >= 4 && (
			    (hashDepth4 >= rDepth4 && SearchUtils::canEvalCutoff(alpha,beta,hashEval,hashFlag)) ||
  				SearchUtils::canTerminalEvalCutoff(alpha,beta,b.step,cDepth,hashDepth4,hashEval,hashFlag)))
			{
				curThread->stats.mHashCuts++;
				if(params.viewing(b)) params.viewHash(b, hashEval, hashFlag, hashDepth4, hashMove);
				MSEARCH_RETURN_EVAL(hashEval);
			}

			//Do not try passes that were generated in the qsearch
			if(hashDepth4 <= 0 && hashMove == PASSMOVE)
				hashMove = ERRORMOVE;
			if(hashMove == QPASSMOVE)
				hashMove = ERRORMOVE;

			if(hashDepth4 <= 0 && SearchParams::HASH_NO_USE_QBM_IN_MAIN)
				hashMove = ERRORMOVE;
			else
				hashMove = SearchUtils::pruneToRegularMove(b,hashMove);
		}
	}
	//Just in case, make sure if we are passing, that it is legal
	if(hashMove == PASSMOVE && !(b.step != 0 && b.posCurrentHash != b.posStartHash))
		hashMove = ERRORMOVE;

	move_t killerMove = ERRORMOVE;
	if(SearchParams::KILLER_ENABLE)
		killerMove = SearchUtils::getKiller(curThread->killerMoves,curThread->killerMovesLen,b,cDepth);

	//Here we go! We actually have a new child splitpoint
	int mode = (pruneReduction >= 0) ? (int)SplitPoint::MODE_REDUCED : (int)SplitPoint::MODE_NORMAL;
	DEBUGASSERT(curThread->lockedSpt == NULL);
	newSpt = curThread->curSplitPointBuffer->acquireSplitPoint(spt);
	newSpt->init(moveIdx,move,mode,b,changedPlayer,cDepth,rDepth4,alpha,beta,false,hashMove,killerMove,
	    SearchParams::MSEARCH_MOVE_CAPACITY,curThread->id);

	return;
}

void Searcher::mSearchDone(SearchThread* curThread, SplitPoint* finishedSpt, SplitPoint*& newSpt, eval_t& eval)
{
	newSpt = NULL;
	eval = Eval::LOSE - 1;

	//TODO turn the arrows into local variables instead of repeated accessing?

	mainHash->record(finishedSpt->board, finishedSpt->cDepth, finishedSpt->rDepth4,
			finishedSpt->bestEval, finishedSpt->bestFlag, finishedSpt->bestMove);

	if(finishedSpt->bestFlag != Flags::FLAG_ALPHA)
		SearchUtils::updateHistoryTable(historyTable,historyMax,finishedSpt->board,finishedSpt->cDepth,finishedSpt->bestMove);

	if(SearchParams::KILLER_ENABLE && (finishedSpt->bestFlag == Flags::FLAG_BETA || finishedSpt->bestFlag == Flags::FLAG_EXACT)
	    && finishedSpt->cDepth < curThread->killerMovesArrayLen)
		SearchUtils::recordKiller(curThread->killerMoves, curThread->killerMovesLen, finishedSpt->cDepth, finishedSpt->pv, finishedSpt->pvLen, finishedSpt->board);

	//Note: rarely, bestIndex can be -1 and bestMove be ERRORMOVE if we just totally failed super low on every
	//legal move at this node (specifically, we got an illegallity terminal eval on every move!)
	DEBUGASSERT(finishedSpt->numMoves <= 0 || (finishedSpt->bestIndex == -1 && finishedSpt->bestMove == ERRORMOVE) || finishedSpt->mv[finishedSpt->bestIndex] == finishedSpt->bestMove);

	if(finishedSpt->cDepth > 0)
	{
		curThread->stats.bestMoveSum += finishedSpt->bestIndex >= 0 ? finishedSpt->bestIndex : 0;
		curThread->stats.bestMoveCount++;
	}

	//Normal search done - return the eval
	if(finishedSpt->searchMode == SplitPoint::MODE_NORMAL)
	{
		eval = finishedSpt->bestEval;
		if(finishedSpt->fDepth < SearchParams::PV_ARRAY_SIZE)
			SearchUtils::copyPV(finishedSpt->pv, finishedSpt->pvLen, curThread->pv[finishedSpt->fDepth], curThread->pvLen[finishedSpt->fDepth]);
		return;
	}

	//Reduced depth search
	DEBUGASSERT(finishedSpt->searchMode == SplitPoint::MODE_REDUCED)

	//If the reduced depth search got the expected result (failed below alpha at the parent)
	//then no further search is needed.
	bool changedPlayer = finishedSpt->changedPlayer;
	eval_t oldAlpha = finishedSpt->parent->alpha; //TODO if the 32 bit read of alpha,beta is not atomic, bad stuff can happen
	eval_t oldBeta = finishedSpt->parent->beta;
	if((!changedPlayer && finishedSpt->bestFlag == Flags::FLAG_ALPHA) || (changedPlayer && finishedSpt->bestFlag == Flags::FLAG_BETA))
	{
    if(params.viewOn && finishedSpt->fDepth < SearchParams::PV_ARRAY_SIZE)
      SearchUtils::copyPV(finishedSpt->pv, finishedSpt->pvLen, curThread->pv[finishedSpt->fDepth], curThread->pvLen[finishedSpt->fDepth]);
		eval = changedPlayer ? -oldAlpha : oldAlpha;
		return;
	}

	int numSteps = changedPlayer ? 4-finishedSpt->parent->board.step : finishedSpt->board.step - finishedSpt->parent->board.step;
	int cDepth = finishedSpt->cDepth;
	int rDepth4 = finishedSpt->parent->rDepth4 - numSteps * SearchParams::DEPTH_DIV;
	eval_t alpha = changedPlayer ? -oldBeta : oldAlpha;
	eval_t beta = changedPlayer ? -oldAlpha : oldBeta;

	if(params.viewing(finishedSpt->board)) params.viewResearch(finishedSpt->board,cDepth,rDepth4/4,alpha,beta);

	DEBUGASSERT(curThread->lockedSpt == NULL);
	newSpt = curThread->curSplitPointBuffer->acquireSplitPoint(finishedSpt->parent);
	//TODO does a killer move here instead of ERRORMOVE help, when we are doing move reductions?
	newSpt->init(finishedSpt->parentMoveIndex,finishedSpt->parentMove,SplitPoint::MODE_NORMAL,
			finishedSpt->board,changedPlayer,cDepth,rDepth4,alpha,beta,false,finishedSpt->bestMove,ERRORMOVE,
			SearchParams::MSEARCH_MOVE_CAPACITY,curThread->id);

	return;
}

//QSEARCH------------------------------------------------------------------------

//RAII style object for acquiring a chunk of a move list buffer to use for a particular fdepth during recursion
struct MoveListAcquirer
{
	public:
	move_t* const mv; //These are the acquired lists, offset appropriately, that you can grab and stuff data into
	int* const hm;    //These are the acquired lists, offset appropriately, that you can grab and stuff data into

	private:
	const int maxCapacity;         //Max capacity of the whole list
	const int initialCapacityUsed; //Capacity prior to this MoveListAquirier
	int& capacityUsed;             //Set to new capacity (or -1 prior to reportUsage when debugging, to flag the possible error)
	int selfCapacity;              //How much capacity used by ourself, set by reportUsage

	public:
	//TODO it's really sketchy that the arimaadebug wraps real code here.
	MoveListAcquirer(move_t* mv, int* hm, int maxCapacity, int& capacityUsed)
	:mv(mv+capacityUsed),hm(hm+capacityUsed),maxCapacity(maxCapacity),
	 initialCapacityUsed(capacityUsed),capacityUsed(capacityUsed),selfCapacity(0)
	{
		ARIMAADEBUG(
		if(capacityUsed == -1)
			Global::fatalError("Probably forgot to call reportUsage");
		capacityUsed = -1;
    selfCapacity = -1;
		)
	}

	~MoveListAcquirer()
	{
		ARIMAADEBUG(
		if(selfCapacity == -1)
		{capacityUsed = initialCapacityUsed; return;}
		if(capacityUsed != initialCapacityUsed + selfCapacity)
			Global::fatalError("MoveListAcquirer: capacities don't match");
		)
		capacityUsed -= selfCapacity;

		ARIMAADEBUG(
		if(capacityUsed < 0)
			Global::fatalError("MoveListAcquirer: capacityUsed < 0");
		if(capacityUsed != initialCapacityUsed)
			Global::fatalError("MoveListAcquirer: capacityUsed not the same after destruction");
		)
	}

	//Can call more than once to report different changes in usage.
	//But should ONLY call when you are the head - if any further acquirers have taken
	//chunks of the list after you, you cannot change.
	void reportUsage(int num)
	{
	  ARIMAADEBUG(
	  if(selfCapacity != -1 && capacityUsed != initialCapacityUsed + selfCapacity)
	    Global::fatalError("MoveListAcquirer: Reported usage but capacities don't match");
	  )

		selfCapacity = max(selfCapacity,num);
		capacityUsed = initialCapacityUsed + selfCapacity;

		ARIMAADEBUG(
		if(capacityUsed > maxCapacity)
			Global::fatalError("MoveListAcquirer: capacityUsed > maxCapacity");
		)
	}
};

int Searcher::qSearch(SearchThread* curThread, Board& b, int fDepth, int cDepth, int qDepth, eval_t alpha, eval_t beta, SearchState qState)
{
	//BEGIN QUIESCENCE SEARCH------------------------
	pla_t pla = b.player;

	//Mark the end of the PV, in case we return
	curThread->endPV(fDepth);

	//QEND CONDITION - TIME CHECK-------------------------------
	tryCheckTime(curThread);
	if(curThread->isTerminated)
		return alpha;

	//Q END CONDITIONS----------------------------------------
	//Do not check end conditions at the main search leaf, since already checked
	if(qState == SS_Q)
	{
		eval_t gameEndVal = SearchUtils::checkGameEndConditionsQSearch(b,cDepth);
		if(gameEndVal != 0)
			return gameEndVal;
	}

	//Q END CONDITIONS - WINNING BOUNDS ----------------------
	//If winning in one move puts us below alpha, then we're certainly below alpha
	if(Eval::WIN - cDepth - 1 <= alpha)
		return alpha;
	//If losing in one move puts us above beta, then we're certainly above beta
	if(Eval::LOSE + cDepth + 1 >= beta)
		return beta;

  //Q END CONDITION - GOAL TREE AND ELIM TREE --------------
	//TODO can you put the hash check before the goal tree check? So if it's hashed, it will be faster?
	//Also in msearch
	if(qState == SS_Q && params.enableGoalTree)
	{
		int goalDist = BoardTrees::goalDist(b,pla,4-b.step);
		if(goalDist <= 4)
			return Eval::WIN - cDepth - goalDist;

		if(BoardTrees::canElim(b,pla,4-b.step))
			return Eval::WIN - cDepth - (4-b.step);
	}

	//Q VIEW CHECK-----------------------------------------------
	if(params.viewing(b)) params.view(b,cDepth,-qDepth,alpha,beta);

	//Q END CONDITION - HASHTABLE--------------------------------
  bool hashFound = false;
  move_t hashMove = ERRORMOVE;
  if(SearchParams::HASH_ENABLE)
  {
    eval_t hashEval;
    int16_t hashDepth4;
    uint8_t hashFlag;
    //Found a hash?
    if(mainHash->lookup(hashMove,hashEval,hashDepth4,hashFlag,b,cDepth))
    {
      hashFound = true;

      //Does the entry allow for a cutoff?
      if(hashDepth4 >= -qDepth && SearchUtils::canEvalCutoff(alpha,beta,hashEval,hashFlag))
      {
      	curThread->stats.qHashCuts++;
      	if(params.viewing(b)) params.viewHash(b, hashEval, hashFlag, hashDepth4, hashMove);
        return hashEval;
      }
    }

    if(hashMove == QPASSMOVE && SearchParams::QDEPTH_NEXT[qDepth] != SearchParams::QDEPTH_EVAL)
    	hashMove = ERRORMOVE;
  }

	//Q EVALUATE-------------------------------------
  if(!SearchParams::Q_ENABLE || qDepth >= SearchParams::QDEPTH_EVAL || !params.qEnable)
	{
  	if(beta < 0 && SearchUtils::isTerminalEval(beta))
  		return beta;
  	if(alpha > 0 && SearchUtils::isTerminalEval(alpha))
  		return alpha;

		int eval = evaluate(curThread,b,alpha,beta,params.viewingEval(b));
		mainHash->record(b, cDepth, -qDepth, eval, Flags::FLAG_EXACT, ERRORMOVE);
		return eval;
	}

	//QSEARCH-------------------------------------------
	int bestIndex = 0;
	eval_t bestEval = Eval::LOSE-1;
	move_t bestMove = ERRORMOVE;
	flag_t bestFlag = Flags::FLAG_ALPHA;

	//Q HASH MOVE AND RECURSE-------------------------------------
	if(hashFound && hashMove != ERRORMOVE)
	{
		//Make the move
		int status = 0;
		tryMove(curThread,hashMove,SearchParams::HASH_SCORE,0,status,bestMove,bestEval,bestIndex,bestFlag,b,fDepth,cDepth,qDepth,alpha,beta);

		if(status == TRYMOVE_BETA)
		{
			//curThread->stats.bestMoveSum += 0;
			curThread->stats.bestMoveCount++;
			mainHash->record(b, cDepth, -qDepth, bestEval, Flags::FLAG_BETA, bestMove);
			if(SearchParams::QKILLER_ENABLE && fDepth < SearchParams::PV_ARRAY_SIZE && cDepth < curThread->killerMovesArrayLen)
				SearchUtils::recordKiller(curThread->killerMoves, curThread->killerMovesLen, cDepth, curThread->pv[fDepth], curThread->pvLen[fDepth], b);
			SearchUtils::updateHistoryTable(historyTable,historyMax,b,cDepth,hashMove);
			return bestEval;
		}
	}

	//Q MOVES-------------------------------------
	MoveListAcquirer moveListAq(curThread->mvList,curThread->hmList,curThread->mvListCapacity,curThread->mvListCapacityUsed);
	move_t* mv = moveListAq.mv;
	int* hm = moveListAq.hm;

	//Q GOAL THREAT PRUNING----------------------
	int num = 0;
	int loseStepsMax = beta - (Eval::LOSE + cDepth);
	if(loseStepsMax <= 0)
		return Eval::LOSE + cDepth;
	int winDefSearchInteriorNodes = 0;
	int stepsToLose = SearchUtils::winDefSearch(b,mv,num,loseStepsMax,winDefSearchInteriorNodes);

	//Expensive, but not counted in our node count, so add it manually.
	curThread->stats.qNodes += winDefSearchInteriorNodes;
	//And also advance the time check counter so we keep to desired time more tightly
	curThread->timeCheckCounter += winDefSearchInteriorNodes;

	if(stepsToLose < 9)
		return Eval::LOSE + cDepth + stepsToLose;

	//Win def search moves generated
	if(num > 0)
	{
		//Fill up hm array because windefsearch won't.
		//TODO add some move ordering here? History ordering is also done below, but maybe some extra ordering here could help
		for(int i = 0; i<num; i++)
			hm[i] = 0;
	}

	//Q MAIN QUIESCENCE MOVE GEN-----------------------------------
	//No windefsearch moves generated - so generate ordinary qsearch moves
	if(num == 0)
		num = SearchUtils::genQuiescenceMoves(b,curThread->boardHistory,cDepth,qDepth,mv,hm);

	//Report move list usage out acquirer object
	moveListAq.reportUsage(num);

	//Q MOVE ORDERING------------------------------
	if(SearchParams::HISTORY_ENABLE)
	{
		SearchUtils::getHistoryScores(historyTable,historyMax,b,pla,cDepth,mv,hm,num); //TODO test this for move order impact
		//for(int i = 0; i<num; i++)
		//	hm[i] += getNSScore(mv[i]);
	}

	if(SearchParams::QKILLER_ENABLE)
	{
		move_t killerMove = SearchUtils::getKiller(curThread->killerMoves,curThread->killerMovesLen,b,cDepth);
		if(killerMove == hashMove)
			killerMove = ERRORMOVE;

		if(killerMove != ERRORMOVE)
		{
			int longestLen = 0;
			int longestIdx = -1;
			for(int i = 0; i<num; i++)
			{
				if(Board::isPrefix(mv[i],killerMove))
				{
					int ns = Board::numStepsInMove(mv[i]);
					if(ns > longestLen)
					{longestLen = ns; longestIdx = i;}
				}
			}
			if(longestIdx >= 0)
				hm[longestIdx] = SearchParams::KILLER_SCORE;
		}
	}

	//QSEARCH! ------------------------------------------------
	int mOffset = (hashFound && hashMove != ERRORMOVE) ? 1 : 0;	//Moves already tried - the hashmove, just for cosmetics
	for(int m = 0; m < num; m++)
	{
		//Sort the next best move to the top
		SearchUtils::selectBest(mv,hm,num,m);
		move_t move = mv[m];

		//Don't search the hashed move twice
		if(move == hashMove)
		{mOffset--; continue;}

		int status = 0;
		tryMove(curThread,move,hm[m],m+mOffset,status,bestMove,bestEval,bestIndex,bestFlag,b,fDepth,cDepth,qDepth,alpha,beta);

		if(status == TRYMOVE_ERR)
		{ARIMAADEBUG({SearchUtils::reportIllegalMove(b,move);}) continue;}

		else if(status == TRYMOVE_BETA)
		{
			curThread->stats.bestMoveSum += bestIndex;
			curThread->stats.bestMoveCount++;
			mainHash->record(b, cDepth, -qDepth, bestEval, Flags::FLAG_BETA, bestMove);
			if(SearchParams::QKILLER_ENABLE && fDepth < SearchParams::PV_ARRAY_SIZE && cDepth < curThread->killerMovesArrayLen)
				SearchUtils::recordKiller(curThread->killerMoves, curThread->killerMovesLen, cDepth, curThread->pv[fDepth], curThread->pvLen[fDepth], b);

			SearchUtils::updateHistoryTable(historyTable,historyMax,b,cDepth,bestMove);

			return bestEval;
		}
	}
	//Record and restore
	mainHash->record(b, cDepth, -qDepth, bestEval, bestFlag, bestMove);

	if(bestFlag != Flags::FLAG_ALPHA)
		SearchUtils::updateHistoryTable(historyTable,historyMax,b,cDepth,bestMove);

	if(SearchParams::QKILLER_ENABLE && bestFlag == Flags::FLAG_EXACT && fDepth < SearchParams::PV_ARRAY_SIZE && cDepth < curThread->killerMovesArrayLen)
		SearchUtils::recordKiller(curThread->killerMoves, curThread->killerMovesLen, cDepth, curThread->pv[fDepth], curThread->pvLen[fDepth], b);

	//Record statistics
	curThread->stats.bestMoveSum += bestIndex;
	curThread->stats.bestMoveCount++;

	//Done
	return bestEval;
}

//TRYMOVE---------------------------------------------------------------------------------

eval_t Searcher::performSearchRecursion(SearchThread* curThread, move_t move, int numSteps, bool changedPlayer,
    Board& bCopy, int fDepth, int cDepth, int rDepth4, eval_t alpha, eval_t beta)
{
	eval_t eval = Eval::LOSE-1;

  //DISABLED - qstate argument to this function removed for now, performSearchRecursion usable only in qsearch
  /*
	if(qState == SS_MAIN)
	{
		Global::fatalError("Cannot msearch from performSearchRecursion!");
		//Recurse
		if(pruneReduction >= 0)
		{
			//Perform reduced depth search as requested
			if(changedPlayer) {eval = -mSearch(curThread,bCopy,fDepth+1,cDepth+numSteps,rDepth4-(numSteps+pruneReduction)*4,-(alpha+1),  -alpha);}
			else              {eval =  mSearch(curThread,bCopy,fDepth+1,cDepth+numSteps,rDepth4-(numSteps+pruneReduction)*4,     alpha, alpha+1);}
			//Failed high? Re-search it!
			if(eval >= alpha+1)
				pruneReduction = -1;
		}
		if(pruneReduction <= -1)
		{
			if(changedPlayer) {eval = -mSearch(curThread,bCopy,fDepth+1,cDepth+numSteps,rDepth4-numSteps*4,-beta,-alpha);}
			else              {eval =  mSearch(curThread,bCopy,fDepth+1,cDepth+numSteps,rDepth4-numSteps*4,alpha,  beta);}
		}
	}
  */

	//QSearch (rDepth4 is really qDepth)
	//else
	//{
		int qDepth = rDepth4;

		//Compute new alpha and beta bounds given bonus for passing
		int newAlpha = alpha;
		int newBeta = beta;
		int bonus = 0;
		if(qDepth == 0 && bCopy.step == 0)
		{
			bool passed = false;
			int passedSteps = 0;
			for(int s = 0; s<4; s++)
			{
				step_t step = Board::getStep(move,s);
				if(step == PASSSTEP)
				{passed = true; passedSteps = numSteps - s; break;}
				if(step == ERRORSTEP)
					break;
			}
			if(passed && passedSteps >= 0)
			{
				bonus = SearchParams::Q0PASS_BONUS[passedSteps];
				newAlpha = SearchUtils::isTerminalEval(alpha) ? alpha : alpha - bonus;
				newBeta = SearchUtils::isTerminalEval(beta) ? beta : beta - bonus;
			}
		}

		//Compute next qdepth
		curThread->stats.qNodes++;
		int nextQDepth = qDepth;
		if(changedPlayer)
			nextQDepth = SearchParams::QDEPTH_NEXT[qDepth];

		//Recurse!
		if(changedPlayer) {eval = -qSearch(curThread,bCopy,fDepth+1,cDepth+numSteps,nextQDepth,-newBeta,-newAlpha,SS_Q);}
		else              {eval =  qSearch(curThread,bCopy,fDepth+1,cDepth+numSteps,nextQDepth, newAlpha, newBeta,SS_Q);}

		//Adjust for passing bonus
		if(qDepth == 0 && move == PASSMOVE && !SearchUtils::isTerminalEval(eval))
			eval += bonus;
	//}
	return eval;
}

//Make and undo move, recurse, return status and evaluation, record PV, and record stats
//Doesn't record hash or push or pop situations
void Searcher::tryMove(SearchThread* curThread, move_t move, int hm, int index,
	int& status, move_t& bestMove, eval_t& bestEval, int& bestIndex, flag_t& bestFlag,
	Board& b, int fDepth, int cDepth, int rDepth4, eval_t& alpha, eval_t& beta)
{
	//Eval of this move - to be computed
	eval_t eval = Eval::LOSE-1;
	//DISABLED - pruneReduction argument to this function removed for now
	//if(rDepth4-4-pruneReduction*4 < 0)
	//{
	//	if(params.viewing(b)) params.viewMovePruned(b, move);
	//	status = TRYMOVE_CONT;
	//	return;
	//}

	if(move == QPASSMOVE)
	{
	  //DISABLED - qstate argument to this function removed for now, tryMove usable only in qsearch
		//if(qState == SS_MAIN) Global::fatalError("qpss in main search!");
		eval = qSearch(curThread,b,fDepth+1,cDepth,SearchParams::QDEPTH_EVAL,alpha,beta,SS_Q);
	}
	else
	{
		//Attempt move
		Board bCopy = b;
		bool suc = bCopy.makeMoveLegal(move);
		if(!suc) {status = TRYMOVE_ERR; return;}

		//Cannot return to the same position
		if(bCopy.posCurrentHash == bCopy.posStartHash)
		{
			if(params.viewing(b)) params.viewMovePruned(b, move);
			status = TRYMOVE_CONT;
			return;
		}

		//Compute new basic params
		bool changedPlayer = (bCopy.player != b.player);
		int numSteps = changedPlayer ? 4-b.step : bCopy.step-b.step;

		//Check if we lose a repetition fight because of this move
		int turnsToLoseByRep = 0;
		if(SearchUtils::losesRepetitionFight(bCopy,curThread->boardHistory,cDepth+numSteps,turnsToLoseByRep))
		{
			eval = Eval::LOSE + cDepth + numSteps + turnsToLoseByRep*4;
			curThread->endPV(fDepth+1);
		}
		//All good, just recurse
		else
		{
			curThread->boardHistory.reportMove(bCopy,move,b.step);
			eval = performSearchRecursion(curThread,move, numSteps, changedPlayer, bCopy, fDepth, cDepth, rDepth4, alpha, beta);
		}
	}

	if(params.viewing(b))
		params.viewMove(b,move,hm,eval,bestEval,curThread->getPV(fDepth+1),curThread->getPVLen(fDepth+1),curThread->stats);

	//Beta cutoff
	if(eval >= beta)
	{
		//Unconditional copy extend, for killer moves
		curThread->copyExtendPVFromNextFDepth(move, fDepth);

		//Update statistics
		curThread->stats.betaCuts++;

		//Done
		status = TRYMOVE_BETA;
		bestIndex = index;
		bestMove = move;
		bestEval = eval;
		bestFlag = Flags::FLAG_BETA;
		return;
	}

	//Update best and alpha
	if(eval > bestEval)
	{
		bestMove = move;
		bestEval = eval;
		bestIndex = index;

		if(eval > alpha)
		{
			alpha = eval;
			bestFlag = Flags::FLAG_EXACT;
			curThread->copyExtendPVFromNextFDepth(move, fDepth);
		}
		//View
		else if(params.viewOn)
			curThread->copyExtendPVFromNextFDepth(move, fDepth);
	}

	status = TRYMOVE_CONT;
	return;
}


//Pruning thresholds
static const double fancyThresholdPVS = 0.050;
static const int fancyBaseThresholdPVS = 5;
static const int fancyDepthArrayMax = 16;
static const double fancyThresholdR1Prop[fancyDepthArrayMax+1] = {
1, 1,1,1,1, 0.200,0.170,0.150,0.130, 0.115,0.110,0.105,0.100, 0.100,0.100,0.100,0.100
};
static const double fancyThresholdR2Prop[fancyDepthArrayMax+1] = {
1, 1,1,1,1, 1.000,0.400,0.350,0.300, 0.270,0.255,0.240,0.230, 0.230,0.230,0.230,0.230
};

int Searcher::genFSearchMoves(const SearchThread* curThread, const Board& b, int cDepth, int rDepth4,
		move_t* mv, int* hm,
		int& pvsThreshold, int& r1Threshold, int& r2Threshold, int& pruneThreshold)
{
	DEBUGASSERT(cDepth == 0);
	for(int i = 0; i<fullNumMoves; i++)
	{
		mv[i] = fullMv[i];
		hm[i] = fullHm[i];
	}

	//BT PRUNING AND PVS------------------------------------------------------------------------
	int numMoves = fullNumMoves;
	pvsThreshold = numMoves;
	r1Threshold = numMoves;
	r2Threshold = numMoves;
	pruneThreshold = numMoves;

	if(SearchParams::PVS_ENABLE)
	{
		pvsThreshold = (int)(fancyThresholdPVS * numMoves) + fancyBaseThresholdPVS;
	}
	if(params.rootMoveFeatureSet.fset != NULL || params.stupidPrune)
	{
		if(params.rootFancyPrune)
		{
			int d = rDepth4/SearchParams::DEPTH_DIV;
			if(d > fancyDepthArrayMax)
				d = fancyDepthArrayMax;
			r1Threshold = (int)(fancyThresholdR1Prop[d] * numMoves)+params.rootMinDontPrune;
			r2Threshold = (int)(fancyThresholdR2Prop[d] * numMoves)+params.rootMinDontPrune;
		}
		else if(params.rootFixedPrune)
		{
			if(params.rootPruneSoft)
				r1Threshold = (int)(params.rootPruneAllButProp * max(0,numMoves-params.rootMinDontPrune))+params.rootMinDontPrune;
			else
				pruneThreshold = (int)(params.rootPruneAllButProp * max(0,numMoves-10))+params.rootMinDontPrune;
		}
	}
	return numMoves;
}

int Searcher::genMSearchMoves(SearchThread* curThread, const Board& b, int cDepth, int rDepth4, int numSpecialMoves,
		move_t* mv, int* hm,
		int& pvsThreshold, int& r1Threshold, int& r2Threshold, int& pruneThreshold)
{
	//TODO the disabled genWinConditionMoves actually seems to slow down the program a bit
	//due to the extra board copy and overhead. Also, there is some funny interaction with
	//the transition to qsearch. What to do about it?
	//GOAL THREAT PRUNING----------------------
	//int winNum = -1;
	//Board bCopy = b;
	//winNum = SearchUtils::genWinConditionMoves(bCopy,mv);

	//No way to stop imminent loss?
	//if(winNum == 0)
	//	return 0;

	//NORMAL MOVE GEN--------------------------
	//int num = winNum;
	//if(winNum == -1)
	int num = SearchUtils::genRegularMoves(b,mv);

	//Generate the pass move
	if(b.step != 0 && b.posCurrentHash != b.posStartHash)
	{mv[num] = PASSMOVE; hm[num] = 0; num++;}

	//MOVE ORDERING------------------------------
	for(int i = 0; i<num; i++)
		hm[i] = 0;

	if(SearchParams::HISTORY_ENABLE)
	{
		SearchUtils::getHistoryScores(historyTable,historyMax,b,b.player,cDepth,mv,hm,num);

		if(params.treeMoveFeatureSet.fset != NULL)
		{
			int turnsFromStart = b.turnNumber - curThread->mainBoardTurnNumber;
			if((int)curThread->featurePosDataHash.size() < turnsFromStart+1)
			{
				curThread->featurePosDataHash.resize(turnsFromStart+1);
				curThread->featurePosData.resize(turnsFromStart+1);
			}

			const Board& turnBoard = curThread->boardHistory.turnBoard[b.turnNumber];
			move_t turnMove = curThread->boardHistory.turnMove[b.turnNumber];
			FeaturePosData& moveFeatureData = curThread->featurePosData[turnsFromStart];
			if(curThread->featurePosDataHash[turnsFromStart] != turnBoard.sitCurrentHash)
			{
				params.treeMoveFeatureSet.getPosData(turnBoard,curThread->boardHistory,turnBoard.player,moveFeatureData);
				curThread->featurePosDataHash[turnsFromStart] = turnBoard.sitCurrentHash;
			}

			for(int i = 0; i<num; i++)
			{
				hm[i] += (int)(SearchParams::MOVEWEIGHTS_SCALE * params.treeMoveFeatureSet.getFeatureSum(
						turnBoard,moveFeatureData,b.player,Board::concatMoves(turnMove,mv[i],b.step),curThread->boardHistory,params.treeMoveFeatureWeights));
			}
		}
		else
		{
			for(int i = 0; i<num; i++)
				hm[i] += Board::numStepsInMove(mv[i]);
		}
	}

	int endNum = numSpecialMoves+num;
	pvsThreshold = endNum;
	r1Threshold = endNum;
	r2Threshold = endNum;
	pruneThreshold = endNum;

	return num;
}


void Searcher::tryCheckTime(SearchThread* curThread)
{
	curThread->timeCheckCounter++;

	if(!cannotInterrupt && curThread->timeCheckCounter >= SearchParams::TIME_CHECK_PERIOD)
	{
	  curThread->timeCheckCounter = 0;
	  double desiredTime = clockDesiredTime;
	  double usedTime = clockTimer.getSeconds();
		if(usedTime > desiredTime)
		{
			interrupted = true;
			searchTree->timeout(curThread);
		}
	}
}

void Searcher::updateDesiredTime(eval_t currentEval, int rDepth, int movesDone, int movesTotal)
{
  eval_t baseEval = clockBaseEval;

  bool baseTerminal = SearchUtils::isTerminalEval(baseEval);
  bool currentTerminal = SearchUtils::isTerminalEval(currentEval);

  //Our current best eval is a loss - search as long as we can.
  if(currentEval < 0 && currentTerminal)
  {clockDesiredTime = clockHardMaxTime; return;}

  double factor = 1.0;

  //Either eval is terminal - search the basic amount of time.
  if(baseTerminal || currentTerminal)
  {}
  else
  {
    eval_t evalDiff = currentEval - baseEval;

    //Search longer if we're down, less if we're not.
    if(evalDiff > -300)
    {}
    else if(evalDiff > -600)
    {factor *= 1.0 + (-300-evalDiff)/300.0 * 0.1;} //Linear 1.0 -> 1.1
    else if(evalDiff > -1300)
    {factor *= 1.1 + (-600-evalDiff)/350.0 * 0.1;} //Linear 1.1 -> 1.3
    else if(evalDiff > -2200)
    {factor *= 1.3 + (-1300-evalDiff)/450.0 * 0.1;} //Linear 1.3 -> 1.5
    else
    {factor *= 1.5;}
  }

  //Based on number of moves finished
  if(movesTotal <= 0)
    factor *= 1.0;
  else if(movesDone <= 0)
    factor *= 1.1;
  else if(movesDone <= 2 + movesTotal * 2/1000)
    factor *= 1.25;
  else if(movesDone <= 3 + movesTotal * 4/1000)
    factor *= 1.2;
  else if(movesDone <= 4 + movesTotal * 1/100)
    factor *= 1.16;
  else if(movesDone <= 5 + movesTotal * 2/100)
    factor *= 1.12;
  else if(movesDone <= 6 + movesTotal * 3/100)
    factor *= 1.09;
  else if(movesDone <= 7 + movesTotal * 5/100)
    factor *= 1.06;
  else if(movesDone <= 8 + movesTotal * 7/100)
    factor *= 1.03;
  else if(movesDone <= 9 + movesTotal * 10/100)
    factor *= 1.00;
  else if(movesDone <= 10 + movesTotal * 20/100)
    factor *= 0.94;
  else if(movesDone <= 11+ movesTotal * 35/100)
    factor *= 0.90;
  else if(movesDone <= 12 + movesTotal * 50/100)
    factor *= 0.88;
  else if(movesDone <= 13 + movesTotal * 80/100)
    factor *= 0.86;
  else
    factor *= 0.87;

  //Based on depth
  int depth = rDepth;
  if(depth <= 5)
    factor *= 1.22;
  else if(depth <= 6)
    factor *= 1.16;
  else if(depth <= 7)
    factor *= 1.09;
  else if(depth <= 8)
    factor *= 1.08;
  else if(depth <= 9)
    factor *= 1.02;
  else if(depth <= 10)
    factor *= 1.0;
  else if(depth <= 11)
    factor *= 0.98;
  else
    factor *= 0.98;

  clockDesiredTime = min(clockHardMaxTime, max(clockHardMinTime, clockOptimisticTime * factor));
}

double Searcher::currentTimeUsed()
{
  return clockTimer.getSeconds();
}

double Searcher::currentDesiredTime()
{
  return clockDesiredTime;
}


move_t Searcher::getRootMove(move_t* mv, int* hm, int numMoves, int idx)
{
	DEBUGASSERT(idx < numMoves);
	return mv[idx];
}

move_t Searcher::getMove(move_t* mv, int* hm, int numMoves, int idx)
{
	DEBUGASSERT(idx < numMoves);
	SearchUtils::selectBest(mv,hm,numMoves,idx);
	return mv[idx];
}

//HELPERS - EVALUATION----------------------------------------------------------------------

eval_t Searcher::evaluate(SearchThread* curThread, Board& b, eval_t alpha, eval_t beta, bool print)
{
	curThread->stats.evalCalls++;

	eval_t eval;
	if(params.useEvalParams)
		eval = Eval::evaluateWithParams(b,mainPla,alpha,beta,params.evalParams,print);
	else
		eval = Eval::evaluate(b,alpha,beta,print);

	if(params.avoidEarlyTrade && mainBoard.turnNumber <= SearchParams::EARLY_TRADE_TURN_MAX)
	{
		if(b.pieceCounts[mainPla][0] != mainBoard.pieceCounts[mainPla][0])
		{
			if(mainPla == b.player) eval -= params.earlyTradePenalty;
			else eval += params.earlyTradePenalty;
		}
	}

	if(params.randomize && params.randDelta > 0)
	{
		int d = params.randDelta*2+1;
		int d2 = d*d;

		uint64_t randSeed = params.randSeed;
		uint64_t hash = b.sitCurrentHash;
		hash = hash * 0xC849B09232AFC387ULL;
		hash ^= (hash >> 32) & 0x00000000FFFFFFFFULL;
		hash += randSeed;
		hash ^= 0x7331F00DCAFEBABEULL;
		hash = hash * 0x25AFE4F5678219C1ULL;
		hash ^= (hash >> 32) & 0x00000000FFFFFFFFULL;

		int randvalues = hash % d2;
		int randvalues2 = ((hash >> 32) & 0x00000000FFFFFFFFULL) % d;
		eval += (randvalues % d)-params.randDelta;
		eval += (randvalues/d)-params.randDelta;
		eval += randvalues2-params.randDelta;
	}

	return eval;
}

