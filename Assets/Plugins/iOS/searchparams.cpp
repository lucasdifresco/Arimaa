/*
 * searchparams.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <cstdio>
#include <vector>
#include <string>
#include "global.h"
#include "learner.h"
#include "searchflags.h"
#include "searchparams.h"
#include "searchutils.h"
#include "searchstats.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;

const int SearchParams::PV_ARRAY_SIZE = 64;

SearchParams::SearchParams()
{
	name = string("SearchParams");
	defaultMaxDepth = AUTO_DEPTH;
	defaultMaxTime = MAX_SEARCH_TIME;
	init();
}

SearchParams::SearchParams(int depth, double time)
{
	name = string("SearchParamsD") + Global::intToString(depth) + "T" + Global::doubleToString(time);
	defaultMaxDepth = depth;
	defaultMaxTime = time;
	init();
}

SearchParams::SearchParams(string nm)
{
	name = nm;
	defaultMaxDepth = AUTO_DEPTH;
	defaultMaxTime = MAX_SEARCH_TIME;
	init();
}

SearchParams::SearchParams(string nm, int depth, double time)
{
	name = nm;
	defaultMaxDepth = depth;
	defaultMaxTime = time;
	init();
}

void SearchParams::init()
{
  stopEarlyWhenLittleTime = false;

	randomize = false;
	randDelta = 0;
	randSeed = 0;
	numThreads = 1;

	disablePartialSearch = false;
	avoidEarlyTrade = false;
	earlyTradePenalty = 0;

	fullMoveHashExp = DEFAULT_FULLMOVE_HASH_EXP;
	mainHashExp = DEFAULT_MAIN_HASH_EXP;

	qEnable = true;
	enableGoalTree = true;

	rootMoveFeatureSet = ArimaaFeatureSet();
	rootMinDontPrune = 10;
	rootFancyPrune = false;
	rootFixedPrune = false;
	rootPruneAllButProp = 1.0;
	rootPruneSoft = false;
	stupidPrune = false;

	treeMoveFeatureSet = ArimaaFeatureSet();

	useEvalParams = false;
	evalParams = EvalParams();

	viewOn = false;
	viewPrintMoves = false;
	viewPrintBetterMoves = false;
	viewEvaluate = false;

}

SearchParams::~SearchParams()
{
}

//Threading-------------------------

//How many parallel threads to use for the search?
void SearchParams::setNumThreads(int num)
{
	if(num <= 0 || num > 64)
		Global::fatalError("Invalid number of threads: " + Global::intToString(num));
	numThreads = num;
}

//Randomization--------------------

//Do we randomize the pv among equally strong moves? And do we add any deltas to the evals?
void SearchParams::setRandomize(bool r, eval_t rDelta, uint64_t seed)
{
	if(rDelta < 0 || rDelta > 16383)
		Global::fatalError(string("Invalid rDelta: ") + Global::intToString(rDelta));

	randomize = r;
	randDelta = rDelta;
	randSeed = seed;
}

void SearchParams::setRandomSeed(uint64_t seed)
{
	randSeed = seed;
}


//Move feature ordering ---------------------------

//Add features and weights for root move ordering
void SearchParams::initRootMoveFeatures(const BradleyTerry& bt)
{
	rootMoveFeatureSet = bt.afset;
	rootMoveFeatureWeights = bt.logGamma;
}

//Use root move ordering for pruning too. Prune after the first prop of moves
void SearchParams::setRootFixedPrune(bool b, double prop, bool soft)
{
	if(rootMoveFeatureSet.fset == NULL && b == true)
		Global::fatalError("Tried to set root fixed pruning without feature set!");
	stupidPrune = false;
	rootFixedPrune = b;
	rootFancyPrune = false;
  rootPruneSoft = soft;
	rootPruneAllButProp = prop;
}

void SearchParams::setStupidPrune(bool b, double prop)
{
  stupidPrune = b;
  rootFixedPrune = b;
  rootFancyPrune = false;
  rootPruneSoft = false;
  rootPruneAllButProp = prop;
}

//Use root move ordering for fancier pruning
void SearchParams::setRootFancyPrune(bool b)
{
	if(rootMoveFeatureSet.fset == NULL && b == true)
		Global::fatalError("Tried to set root fancy pruning without feature set!");
  stupidPrune = false;
  rootFixedPrune = false;
  rootFancyPrune = b;
}

void SearchParams::initTreeMoveFeatures(const BradleyTerry& bt)
{
	treeMoveFeatureSet = bt.afset;
	treeMoveFeatureWeights = bt.logGamma;
}

//Search options ------------------------------

void SearchParams::setAvoidEarlyTrade(bool avoid, eval_t penalty)
{
	avoidEarlyTrade = avoid;
	earlyTradePenalty = penalty;
}

void SearchParams::setDefaultMaxDepthTime(int depth, double time)
{
	defaultMaxDepth = depth;
	defaultMaxTime = time;
}

void SearchParams::setDisablePartialSearch(bool b)
{
	disablePartialSearch = b;
}

//VIEW--------------------------------------------------------------------------------------

void SearchParams::initView(const Board& b, const string& moveStr, bool printBetterMoves, bool printMoves)
{
	viewOn = true;
	viewStartBoard = b;
	cout << "---VIEWING---------------------- " << endl;

	vector<move_t> moves = readMoveSequence(moveStr);

	for(int i = 0; i<(int)moves.size(); i++)
	{
		move_t move = moves[i];
		cout << writeMove(viewStartBoard,move) << " " << endl;

		bool suc = viewStartBoard.makeMoveLegal(move);
		if(!suc)
			Global::fatalError("Viewing illegal move!");
	}
	viewPrintBetterMoves = printBetterMoves;
	viewPrintMoves = printMoves;
	viewEvaluate = true;

	cout << viewStartBoard << endl;
	cout << "------------------------------" << endl;
}

bool SearchParams::viewing(const Board& b) const
{
	return viewOn && b.sitCurrentHash == viewStartBoard.sitCurrentHash;
}

bool SearchParams::viewingEval(const Board& b) const
{
	if(!viewEvaluate)
		return false;

	return viewing(b);
}

void SearchParams::view(const Board& b, int cDepth, int rDepth, int alpha, int beta) const
{
	cout << "---VIEW------------" << endl;
	cout << "rDepth " << rDepth << " cDepth " << cDepth << endl;
	cout << "AlphaBeta (" << alpha << "," << beta << ")"<< endl;
}

void SearchParams::viewResearch(const Board& b, int cDepth, int rDepth, int alpha, int beta) const
{
	cout << "---VIEW RE-SEARCH------------" << endl;
	cout << "rDepth " << rDepth << " cDepth " << cDepth << endl;
	cout << "AlphaBeta (" << alpha << "," << beta << ")"<< endl;
}

void SearchParams::viewMove(const Board& b, move_t move, int hm,
		eval_t eval, eval_t bestEval, move_t* pv, int pvLen, const SearchStats& stats) const
{
	if(!viewPrintMoves && !viewPrintBetterMoves)
		return;

	if(viewPrintBetterMoves && !viewPrintMoves && eval <= bestEval)
		return;

	cout << writeMove(b,move) << " ";
	printf(" %5d %5d %7lld  ", eval, bestEval, stats.mNodes + stats.qNodes);
	Board copy = b;
	copy.makeMove(move);
	if(pv != NULL)
		cout << writeMoves(copy, pv, pvLen);

	if(hm == HASH_SCORE)
		cout << " (hashmove)";
	else if(hm == KILLER_SCORE)
		cout << " (killermove)";
	else
		cout << " (order " << hm << ")";
	cout << endl;
}

void SearchParams::viewMovePruned(const Board& b, move_t move) const
{
	if(!viewPrintMoves)
		return;

	cout << writeMove(b,move) << " pruned " << endl;
}

void SearchParams::viewHash(const Board& b, eval_t hashEval, flag_t hashFlag, int hashDepth4, move_t hashMove) const
{
	cout <<
			"HashEval: " << hashEval <<
			" HashFlag: " << Flags::FLAG_NAMES[hashFlag] <<
			" HashDepth4: " << hashDepth4 <<
			" Hashmove: " << writeMove(b,hashMove) <<
			" Hash: " << b.sitCurrentHash << endl;
	cout << "-----------------" << endl;
}

//QSEARCH--------------------------------------------------------------------------------------

const int SearchParams::QDEPTH_NEXT[3] = {1,2,2};
const int SearchParams::WL_TRUST_RDEPTH_OFFSET[4] = {8,11,10,9};
const int SearchParams::QDEPTH_EVAL = 2;
const int SearchParams::QDEPTH_START[4] = {1,0,0,0};
const int SearchParams::QMAX_FDEPTH = 8;
const int SearchParams::QMAX_CDEPTH = 8;
const int SearchParams::Q0PASS_BONUS[5] = {0,100,200,300,400};
const int SearchParams::STEPS_LEFT_BONUS[5] = {-200,-100,0,100,200};

//This is at least 3, because we with, say, a nominal search depth of 5,
//we could make a pass move in the search and reach cdepth 8.
const int SearchParams::MAX_MSEARCH_DEPTH_OVER_NOMINAL = 3;


