
/*
 * search.h
 * Author: davidwu
 *
 *  ----- SEARCHER NOTES ------
 *
 * Eval >= Eval::WIN or <= Eval::LOSE are considered error evals and are not hashtabled.
 *
 * All of the ways I can think of right now to lose or win:
 * By goal/goal tree (correct number of steps)
 * By elim/elim tree (correct number of steps)
 * By windefsearch (correct number of steps)
 * By immo in msearch or msearch leaf (correct number of steps)
 * By failing to have any generated move (should not happen) (win in -1)
 * By various static pruning in MSearch (reversals, freecaps) (win in 0)
 * By losing a predicted repetition fight (win in the length of the pv terminating in the repeated move (filled to the full turn))
 * By having the opponent make a 3rd repetition in msearch or msearch leaf (win in the length of the pv including the repetition)
 * By the hashtable returning a trusted WL score.
 *
 * Also returning to the posstarthash is outright pruned (upon trying it) from the movelist.
 *
 * ---Search Structure----
 * Front Search for the first turn of search.
 * Main Search with tactical fight extensions and heavy pruning
 * Quiescence Search
 *  - Trades, Goals, Fights each have different max added depths
 *  - Qsearch is completely determined by the added depth so far.
 *
 * Boundary between main and qsearch:
 *
 * Main remaining 1:
 *  Recurses...
 * Main remaining 0:
 *  End conditions
 *  Conditions that prune on bad moves from 1
 *  Goal tree
 * Qsearch 0
 *
 *
 * ---Depths---
 * FDepth is the depth in the number of recurses of search - the number of moves, not steps
 * CDepth is the depth in the number of steps we have made on the board.
 * RDepth is the depth remaining to be searched * 4.
 * QDepth is the mode of qsearch:
 *   0 = Cap + GoalDef + CapDef + GoalThreat
 *   1 = Cap + GoalDef
 *   2 = Evaluate
 * HDepth is the key we store in the hashtable (RDepth or -QDepth)
 *
 * Example:
 *  C   R   Q   H
 *  0  36   -  36
 *  1  32   -  32
 *  ...
 *  7   8   -   8
 *  8   4   -   4
 *  9   -   1  -1
 * 10   -   1  -1
 * 11   -   1  -1
 * 12   -   1  -1
 * 13   -   3  -3
 *
 *
 */

#ifndef SEARCH_H
#define SEARCH_H

#include "timer.h"
#include "bitmap.h"
#include "board.h"
#include "boardhistory.h"
#include "eval.h"
#include "timecontrol.h"
#include "searchflags.h"
#include "searchparams.h"
#include "searchstats.h"

class QState;
class SearchHashTable;
class ExistsHashTable;

struct SplitPoint;
struct SearchThread;
class SearchTree;

class Searcher
{
	public:
	//CONSTANT============================================================================

	//QSEARCH --------------------------------------------------------------------
	typedef int SearchState;
	private:
  static const SearchState SS_MAIN = 0;
  static const SearchState SS_MAINLEAF = 1;
  static const SearchState SS_Q = 2;

	//UNCHANGEABLE DURING SEARCH===========================================================

	public:
	SearchParams params; //Parameters and options for the searcher
	bool doOutput;       //Do we output top-level data about the search?
	private:

	//TOP-LEVEL MUTABLE ====================================================================

	//BOARD AND HISTORY ------------------------------------------------------------
	pla_t mainPla;   //The player whose we are searching for, at the top level.
	Board mainBoard; //The board we started searching on
	BoardHistory mainBoardHistory; //The history up to the start of search. Should NOT be modified!

  //STATISTICS ------------------------------------------------------------------
	public:
	SearchStats stats; //Stats combined and updated at end of search
	private:

	//FSEARCH ---------------------------------------------------------------------
	//Buffer for generating full moves.
	move_t* fullMv;
	int* fullHm;
	int fullMvCapacity;
	int fullNumMoves;
	ExistsHashTable* fullmoveHash;

	//TIME CONTROLS --------------------------------------------------------------
	ClockTimer clockTimer;

	//Current max time - if time goes over this, the search is over. Can be changed as search goes.
	//TODO atomic accesses or other synchronization needed?
	volatile double clockDesiredTime;
	volatile eval_t clockBaseEval;

	//Time parameters
	double clockOptimisticTime;
  double clockHardMinTime;
	double clockHardMaxTime;

	//Flag this search as uninterruptable - it MUST finish.
	bool cannotInterrupt;

	//IDPV REPORTING---------------------------------------------------------------
	move_t* idpv;  //Holds pv found during an interative deepening
	int idpvLen;   //Length of idpv

	//THREAD-MUTABLE SHARED=====================================================================

	//HASHTABLE-------------------------------------------------------------------
	SearchHashTable* mainHash;

	//MOVE ORDERING AND PRUNING--------------------------------------------------
	//History Heuristic
	vector<long long*> historyTable;      //[cDepth][historyIndex of move]
	vector<long long> historyMax;         //[cDepth]

	//Time Controls
	bool interrupted; //TODO atomic read and writes for this?

	//THREADING DATA============================================================================
	public:
	SearchTree* searchTree;

	//METHODS=================================================================================

	//CONSTRUCTION----------------------------------------------------------------
	public:
	Searcher();
	Searcher(SearchParams params);
	~Searcher();

	private:
	void init();

	//ACCESSORS----------------------------------------------------------------------------------
	public:

	//Get the best move
	move_t getMove();

	//Get the PV
	vector<move_t> getIDPV();

	//Get the PV, but join all of the moves in it into full-turn moves as much as possible
	//(only the last move might not be full)
	vector<move_t> getIDPVFullMoves();

	//MAIN INTERFACE-----------------------------------------------------------------------------

	//Perform a search, using the default max depth, time
	void searchID(const Board& b, const BoardHistory& hist, bool output = false);

	//Perform a search, given a certain time control
	void searchByTimeControl(const Board& b, const BoardHistory& hist, TimeControl t, bool output = false);

	//Perform a search, given a certain time control up to a max of depth using a max time of maxSeconds
	//Negative maxSeconds indicates unbounded time and ignores time control.
	void searchByTimeControl(const Board& b, const BoardHistory& hist, int depth, double maxSeconds, TimeControl t, bool output = false);

	//Perform a search up to a max of depth using a max time of seconds, optionally outputting data
	//Negative seconds indicates unbounded time.
	void searchID(const Board& b, const BoardHistory& hist, int depth, double seconds);
  void searchID(const Board& b, const BoardHistory& hist, int depth, double seconds, bool output);
  void searchID(const Board& b, const BoardHistory& hist, int depth, double hardMinTime, double optimisticSeconds, double hardMaxTime, bool output);

  //Try to stop the search. Safe to call from other threads.
  void interruptExternal();

	//TOP LEVEL SEARCH--------------------------------------------------------------------------------------

	//Perform search and stores best eval found in stats.finalEval, behaving appropriately depending on allowance
	//of partial search. Similarly, fills in idpv with best pv so far
	//Marks numFinished with how many moves finished before interruption
	//Uses and modifies fullHm for ordering
	//Whenever a new move is best, stores its eval in fullHm, but for moves not best, stores Eval::LOSE-1.
	//And at the end, sorts the moves
	void fsearch(const Board& b, int rDepth4, eval_t alpha, eval_t beta, int& numFinished);

	//Perform qsearch directly from the top level, without going through msearch first
	void directQSearch(const Board& b, int depth, eval_t alpha, eval_t beta);

	//SEARCH HELPERS----------------------------------------------------------------------------------------
	//Search a move of the given SplitPoint.
	//The particular move searched is whatever curThread has selected.
	//newSpt will be null if there is an immediate eval, else it will be the child splitpoint.
	void mSearch(SearchThread* curThread, SplitPoint* spt, SplitPoint*& newSpt, eval_t& eval);

	//Call when the given SplitPoint is finished (all moves searched, or beta cutoff).
	//This function will provide either a new splitpoint to search, or provide null for the new splitpoint
	//and instead give an eval to pass back to the parent
	void mSearchDone(SearchThread* curThread, SplitPoint* finishedSpt, SplitPoint*& newSpt, eval_t& eval);

	//Perform a quiesence search of the position
	eval_t qSearch(SearchThread* curThread, Board& b, int fDepth, int cDepth, int qDepth4, eval_t alpha, eval_t beta, SearchState qState);

	//Perform a quiesence search of the position after at least one pass in an earlier qsearch
	//eval_t qSearchPassed(SearchThread* curThread, Board& b, int fDepth, int cDepth, int qDepth4, eval_t alpha, eval_t beta, SearchState qState);

	//Evaluate the actual board position
	eval_t evaluate(SearchThread* curThread, Board& b, eval_t alpha, eval_t beta, bool print);

	//MOVES------------------------------------------------------------------------------------------------

	static const int TRYMOVE_ERR = 0;
	static const int TRYMOVE_BETA = 1;
	static const int TRYMOVE_CONT = 2;
	void tryMove(SearchThread* curThread, move_t move, int hm, int index,
		int& status, move_t& bestMove, eval_t& bestEval, int& bestIndex, flag_t& bestFlag,
		Board& b, int fDepth, int cDepth, int rDepth4, eval_t& alpha, eval_t& beta);

	eval_t performSearchRecursion(SearchThread* curThread, move_t move, int numSteps, bool changedPlayer,
			Board& bCopy, int fDepth, int cDepth, int rDepth4, eval_t alpha, eval_t beta);

	//THREADING HELPERS-------------------------------------------------------------------------------
	//Sets the thresholds for pruning
	int genFSearchMoves(const SearchThread* curThread, const Board& b, int cDepth, int rDepth4,
			move_t* mv, int* hm,
			int& pvsThreshold, int& r1Threshold, int& r2Threshold, int& pruneThreshold);

	//Sets the thresholds for pruning. Pruning thresholds indicate that some change should take effect AFTER
	//some number of moves have been searched, including the number of special moves (like hashmove).
	int genMSearchMoves(SearchThread* curThread, const Board& b, int cDepth, int rDepth4,
			int numSpecialMoves,
			move_t* mv, int* hm,
			int& pvsThreshold, int& r1Threshold, int& r2Threshold, int& pruneThreshold);

	move_t getRootMove(move_t* mv, int* hm, int numMoves, int idx);

	move_t getMove(move_t* mv, int* hm, int numMoves, int idx);

	//Time-----------------------------------------------------------------------------

	//Maybe perform the timecheck, setting interrupted if out of time
	void tryCheckTime(SearchThread* curThread);
	//Update the time we should search for, based on the given root eval, movesTotal is ignored if movesDone is 0
	void updateDesiredTime(eval_t currentEval, int rDepth, int movesDone, int movesTotal);
	//Check how much time we've searched
	double currentTimeUsed();
	//Check what the current desired time is
	double currentDesiredTime();

	//THREADING-----------------------------------------------------------------------

	//Free SplitPoint and handle buffer return
	void freeSplitPoint(SearchThread* curThread, SplitPoint* spt);

	//Begin a parallel search on a root split point. Call this after retrieving the split point from the search tree
	//and calling init on it
	void parallelFSearch(SearchThread* curThread, SplitPoint* spt);

	//INTERNAL TO searchthread.cpp---------------------------------------------

	void mainLoop(SearchThread* curThread);
	void work(SearchThread* curThread);
};

struct SearchHashEntry
{
	//Note: for this to work, hash_t should be 64 bits!
	volatile uint64_t key;
	volatile uint64_t data;

	SearchHashEntry();

	void record(hash_t hash, int16_t depth4, eval_t eval, flag_t flag, move_t move);
	bool lookup(hash_t hash, int16_t& depth4, eval_t& eval, flag_t& flag, move_t& move);
};

//Thread safe, uses lockless xor scheme to ensure data integrity
class SearchHashTable
{
	public:
	int exponent;
	hash_t size;
	hash_t mask;
	SearchHashEntry* entries;

	SearchHashTable(int exponent); //Size will be (2 ** sizeExp)
	~SearchHashTable();

	static int getExp(uint64_t maxMem);

	void clear();

	void record(const Board& b, int cDepth, int16_t depth4, eval_t eval, flag_t flag, move_t move);
	bool lookup(move_t& hashMove, eval_t& hashEval, int16_t& hashDepth4, flag_t& hashFlag, const Board& b, int cDepth);

};

//NOT THREADSAFE!!!
class ExistsHashTable
{
	public:
	int exponent;
	hash_t size;
	hash_t mask;
	hash_t* hashKeys;

	ExistsHashTable(int exponent); //Size will be (2 ** sizeExp)
	~ExistsHashTable();

	void clear();

	bool lookup(const Board& b);
	void record(const Board& b);

};

#endif
