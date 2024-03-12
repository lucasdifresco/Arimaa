/*
 * searchparams.h
 *
 *  Created on: Jun 2, 2011
 *      Author: davidwu
 */

#ifndef SEARCHPARAMS_H_
#define SEARCHPARAMS_H_

#include <string>
#include <vector>
#include "learner.h"
#include "evalparams.h"
#include "eval.h"
#include "searchflags.h"

struct SearchStats;

using namespace std;

struct SearchParams
{
	//CONSTANT==============================================================================

	//MULTITHREADING----------------------------------------------------------
	static const int MAX_THREADS = 16; //Max number of threads allowed

	//MISC--------------------------------------------------------------------
	static const int EARLY_TRADE_TURN_MAX = 4; //Max turn on which to avoid early trades
	static const int PV_ARRAY_SIZE; //Size of PV arrays for principal variation storage //TODO why is this param unique? Fails with undefined ref in searchutils.cpp on unix
	static const int MSEARCH_MOVE_CAPACITY = 256; //How big to ensure the arrays for msearch, per splitpoint?
	static const int QSEARCH_MOVE_CAPACITY = 2048; //How big to ensure the arrays for qsearch, per fdepth?

	//LIMITS-------------------------------------------------------------------
	static constexpr double MAX_SEARCH_TIME = 3*24*60*60; //The max time to search or if time is specified, 3 days
	static const int AUTO_DEPTH = 96; //The depth to search if unsecified
	static const int DEPTH_DIV = 4; //The factor by which to multiply/divide depths

	//TIME CHECK----------------------------------------------------------------
	static const int TIME_CHECK_PERIOD = 4096; //Check time only every this many mnodes or qnodes

	//TIME POLICY---------------------------------------------------------------
	static const int TIME_RESERVE_MIN = 8;       //Min base time we aim to leave in reserve
	static const int TIME_RESERVE_PERMOVE_FACTOR = 1; //Proportion of permove that we desire in the reserve. //TODO this used to be 1.75 but wasn't really because it was truncated to int
	static constexpr  double TIME_RESERVE_POS_PROP = 0.12; //Prop of reserve time we should use when above desired
	static constexpr  double TIME_RESERVE_NEG_PROP = 0.16; //Prop of (neg) reserve time we should use, when below desired
	static constexpr  double TIME_PERMOVE_PROP = 0.95; //Prop of permove time to use
	static constexpr  double TIME_FORCED_PROP = 0.75;  //Try to spend at least this prop of the permove.
	static const int TIME_LAG_ALLOWANCE = 1;    //And then spend this many fewer seconds than we intended.
	static const int PER_MOVE_MAX_ALLOWANCE = 5;  //And spend at most this many seconds fewer than perMoveMax or total time counting reserve
	static const int TIME_MIN = 3;                //But spend at least this amount of time no matter what

	static constexpr  double TIME_PREV_FACTOR = 0.0;   //Stop deepening if less than this factor times the time used so far is available.

	//HASH TABLE----------------------------------------------------------------
	static const bool HASH_ENABLE = true;
	static const int DEFAULT_MAIN_HASH_EXP = 24; //Size of hashtable is 2**MAIN_HASH_EXP
	static const bool HASH_WL_AB_STRICT = true; //Don't return hashtable proven win/losses unless they actually bound given alpha/beta.
	static const bool HASH_NO_USE_QBM_IN_MAIN = false; //Don't use qsearch best moves in main search
	static const int DEFAULT_FULLMOVE_HASH_EXP = 21; //Size of hashtable for finding full moves at root is 2**FULLMOVE_HASH_EXP


	//QUIESCENCE-----------------------------------------------------------------
	static const bool Q_ENABLE = true;

	//Table indicating how qdepth transitions
	static const int QDEPTH_NEXT[3];  //Next qdepth given the current qdepth (switch to it when the player changes)
	static const int QDEPTH_EVAL;     //Stop search and eval at this qdepth
	static const int QDEPTH_START[4]; //Maps b.step -> which qdepth to start on
	static const int QMAX_FDEPTH;     //Maximum number of steps that qsearch can go deep (for allocating arrays, etc)
	static const int QMAX_CDEPTH;     //Maximum number of steps that qsearch can go deep (for allocating arrays, etc)
	static const int Q0PASS_BONUS[5];  //Maps numstepspassed -> eval bonus for passing, in qdepth 0
	static const int STEPS_LEFT_BONUS[5]; //Maps numstepsleft -> bonus for player in eval (qdepth2, after qpass)

	//How much deeper could an msearch go beyond the nominal depth passed in at the top level?
	static const int MAX_MSEARCH_DEPTH_OVER_NOMINAL;

  //When is a terminal eval of a lower rdepth search trustworthy in a higher one? That is, when can we
  //be sure that the shallower search was thorough, and a deeper search will discover no more?
  //Depends very much on the qsearch and winDefSearch.
	//Maps [b.step when rdepth = 0 (transition to qsearch)] -> number of steps greater than nominal msearch depth that can be trusted
	static const int WL_TRUST_RDEPTH_OFFSET[4];

	//REVERSIBLE MOVE PRUNING-----------------------------------------------------
	static const bool REVERSE_ENABLE = true;

	//REPETITION FIGHT LOGIC-------------------------------------------------------
	static const bool ENABLE_REP_FIGHT_CHECK = true;

	//FREECAPTURABLE MOVE PRUNING--------------------------------------------------
	static const bool FREECAP_PRUNE_ENABLE = true;

	//ORDERING---------------------------------------------------------------------
	static const int HASH_SCORE = 0x3FFFFFFF; //A big int (1 billion)

	static const bool HISTORY_ENABLE = true;
	static const int HISTORY_MAX_DEPTH = 64;
	static const int HISTORY_LEN = 256*5; //[256 pla steps, 256*4 steps+push/pull dir]
	static const int HISTORY_SCORE_MAX = 10000;

	static const int MOVEWEIGHTS_SCALE = 50; //Amount to multiply bt move weights by

	static const bool KILLER_ENABLE = true;
	static const bool QKILLER_ENABLE = true;
	static const int KILLER_SCORE = 20000;

	//PVS
	static const bool PVS_ENABLE = false;


	//PARAMETERS============================================================================

	//NAME-------------------------------------------------------------------------
	string name;

	//MULTITHREADING---------------------------------------------------------------
	int numThreads; //Number of threads to run simultaneously

	//SEARCH PARAMS ---------------------------------------------------------------
	int defaultMaxDepth;   //Default max depth to search
	double defaultMaxTime; //Default max time to search

	double stopEarlyWhenLittleTime; //Stop early when completing an iteration but there is not much time left

	bool randomize;    //Randomize among equal branches and enable randDelta as well
	eval_t randDelta;  //Add random vals in [-randDelta,randDelta] three times to evals. Gives a bell-shaped curve with stdev ~ randDelta.
	uint64_t randSeed; //Random seed for any randomization

	bool disablePartialSearch; //If this is set, don't accept a new IDPV until the *entire* the next iterative deepen is done.
	bool avoidEarlyTrade; //Try to avoid early trades, if board.turn is low.
	eval_t earlyTradePenalty; //Penalty for making an early trade

	//These two parameters need to be set BEFORE creating the searcher!!
	int fullMoveHashExp; //Size of hashtable for root move generation is 2**this, defaults to DEFAULT_FULLMOVE_HASH_EXP
	int mainHashExp; //Size of main hashtable is 2**this, defaults to DEFAULT_MAIN_HASH_EXP

	//Enable qsearch?
	bool qEnable;
	//Enable goal tree
	bool enableGoalTree;

	//ROOT MOVE ORDERING AND PRUNING-----------------------------------------------

	ArimaaFeatureSet rootMoveFeatureSet;
	vector<double> rootMoveFeatureWeights;

	//Always leave this many moves unpruned at least - only prune after the first this many
	int rootMinDontPrune;

	//Enable either type of pruning
	bool rootFancyPrune;
	bool rootFixedPrune;
	bool stupidPrune;

	//Fixed pruning
	double rootPruneAllButProp;
	bool rootPruneSoft;

	//TREE INTERNAL MOVE ORDERING AND PRUNING------------------------------------------
	ArimaaFeatureSet treeMoveFeatureSet;
	vector<double> treeMoveFeatureWeights;

	//EVAL PARAMETERS---------------------------------------------------------------
	bool useEvalParams;
	EvalParams evalParams;

	//VIEWING---------------------------------------------------------------------
	bool viewOn;            //Are we viewing a position?
	Board viewStartBoard;   //The board position we want to view
	bool viewPrintMoves;    //Do we want to view the generated moves for the position?
	bool viewPrintBetterMoves; //Do we want to view the generated moves that improved the score?
	bool viewEvaluate;      //Do we want to view the evaluations?

	//METHODS---------------------------------------------------------------------

	SearchParams();
	SearchParams(int depth, double time);
	SearchParams(string name);
	SearchParams(string name, int depth, double time);
	~SearchParams();

	private:
	void init();


	//PARAMETER SPECIFCATION---------------------------------------------------------------------------
	public:

	//Threading----------------------------------

	//How many parallel threads to use for the search?
	void setNumThreads(int num);

	//Randomization---------------------------------

	//Do we randomize the pv among equally strong moves?  And do we add any random deltas to the evals?
	//The eval will receive a roughly bell-shaped curve with stdev rDelta
	void setRandomize(bool r, eval_t rDelta, uint64_t seed);

	void setRandomSeed(uint64_t seed);

	//Root move ordering-------------------------------

	//Add features and weights for root move ordering
	void initRootMoveFeatures(const BradleyTerry& bt);

	//Use root move ordering for pruning too. Prune after the first prop of moves
	void setRootFixedPrune(bool b, double prop, bool soft);

	//Use root move ordering for fancier pruning
	void setRootFancyPrune(bool b);

	//Prune even without an ordering
	void setStupidPrune(bool b, double prop);

	//Tree move ordering--------------------------

	//Add features and weights for tree move ordering
	void initTreeMoveFeatures(const BradleyTerry& bt);

	//Search options----------------------------------

	//Set the searcher to avoid early trades
	void setAvoidEarlyTrade(bool avoid, eval_t penalty);

	//Re-set the default max depth and time, negative indicates unbounded time.
	void setDefaultMaxDepthTime(int depth, double time);

	//Do not use the results of partially completed iterative deepen?
	void setDisablePartialSearch(bool b);

	//View----------------------------------------------

	//Indicate a board position to examine in the next search.
	void initView(const Board& b, const string& moveStr, bool printBetterMoves, bool printMoves);


	//USAGE--------------------------------------------------------------------------------------------

	//View----------------------------------------------
	//Utility functions for search
	bool viewing(const Board& b) const;
	bool viewingEval(const Board& b) const;

	void view(const Board& b, int cDepth, int rDepth, int alpha, int beta) const;
	void viewResearch(const Board& b, int cDepth, int rDepth, int alpha, int beta) const;
	void viewMove(const Board& b, move_t move, int hm,
			eval_t eval, eval_t bestEval, move_t* pv, int pvLen, const SearchStats& stats) const;
	void viewMovePruned(const Board& b, move_t move) const;
	void viewHash(const Board& b, eval_t hashEval, flag_t hashFlag, int hashDepth, move_t hashMove) const;

};


#endif /* SEARCHPARAMS_H_ */
