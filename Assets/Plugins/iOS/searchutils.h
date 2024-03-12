/*
 * searchutils.h
 *
 *  Created on: Jun 2, 2011
 *      Author: davidwu
 */

#ifndef SEARCHUTILS_H_
#define SEARCHUTILS_H_

#include "board.h"
#include "boardhistory.h"
#include "eval.h"
#include "searchflags.h"

class ExistsHashTable;
class SearchHashTable;

namespace SearchUtils
{
	//Game end conditions------------------------------------------------------------

	//If won or lost, returns the eval, else returns 0.
	eval_t checkGameEndConditions(const Board& b, const BoardHistory& hist, int cDepth);
	eval_t checkGameEndConditionsQSearch(const Board& b, int cDepth);

	//Is this eval a game-ending eval?
	bool isTerminalEval(eval_t eval);

	//Is this eval a game-ending eval that indicates some sort of illegal move or error?
	bool isIllegalityTerminalEval(eval_t eval);

	//Is this a trustworthy terminal eval? For use only with nonnegative rDepth4 (in msearch or higher).
	bool isTrustworthyTerminalEval(eval_t eval, int currentStep, int cDepth, int rDepth4);

	//Eval flag bounding--------------------------------------------------------------

	//Is the result one that proves a value or bound on this position, given alpha and beta?
	bool canEvalCutoff(eval_t alpha, eval_t beta, eval_t result, flag_t resultFlag);

	//Is the result a terminal eval that proves a value or bound on this position, given alpha and beta?
	//Depending on allowed to be HASH_WL_AB_STRICT, allowed to return the wrong answer so long as
	//it's still a proven win or a loss
	bool canTerminalEvalCutoff(eval_t alpha, eval_t beta, int currentStep, int cDepth, int hashDepth, eval_t result, flag_t resultFlag);

	//Move processing-------------------------------------------------------------------

	//Ensure array big enough, allocate or reallocate it if needed
	void ensureMoveHistoryArray(move_t*& mv, int& capacity, int newCapacity);

	//Ensure move array big enough, allocate or reallocate it if needed
	void ensureMoveArray(move_t*& mv, int*& hm, int& mvCapacity, int newCapacity);

	//Find the best move in the list at or after index and swap it to index
	void selectBest(move_t* mv, int* hm, int num, int index);

	//Randomly shuffle the given moves
	//Expensive, because it creates and initializes an RNG!
	void shuffle(uint64_t seed, vector<move_t>& mvB);
	void shuffle(uint64_t seed, move_t* mvB, int num);

	//Stable insertion sort from largest to smallest by values in hm
	void insertionSort(move_t* mv, int* hm, int numMoves);

	//Output that an illegal move was made, and exit
	void reportIllegalMove(const Board& b, move_t move);

	//Full move gen-------------------------------------------------------------------
	//Generate all full legal moves in the board position
	void genFullMoves(const Board& b, const BoardHistory& hist, vector<move_t>& moves, int maxSteps,
			bool winLossPrune, ExistsHashTable* fullMoveHash);

	void genFullMoveHelper(Board& b, const BoardHistory& hist, pla_t pla, vector<move_t>& moves, move_t moveSoFar,
			int stepIndex, int maxSteps, hash_t posStartHash, bool winLossPrune, ExistsHashTable* fullMoveHash);

	//Regular generation--------------------------------------------------------------

	//Get the normal moves to search and recurse on
	int genRegularMoves(const Board& b, move_t* mv);

	//Finds the step, push, or pull that begins mv.
	move_t pruneToRegularMove(const Board& b, move_t mv);

	//Capture-related generation------------------------------------------------------

	//Get the full capture moves
	int genCaptureMoves(Board& b, int numSteps, move_t* mv, int* hm);

	//Get moves to defend vs capture
	int genCaptureDefMoves(Board& b, int numSteps, move_t* mv, int* hm);

	//QSearch move gen-------------------------------------------------------------------

	//Get the qdepth to begin quiescence at
	int getStartQDepth(const Board& b);

	//Get the Q moves to search and recurse on
	int genQuiescenceMoves(Board& b, const BoardHistory& hist, int cDepth, int qDepth4, move_t* mv, int* hm);

	//Generate moves to try to blockade tempting elephants
	int genBlockadeMoves(const Board& b, int numSteps, move_t* mv, int* hm);

	//Goal and Win gen-------------------------------------------------------------------

	//Perform a win defense search. Finds the shortest and nearly-shortest moves for stopping a loss
	//Current player is defender, assumes pla does not goal threat (else pla could just win).
	//Returns 9 if goal can be stopped, else returns the
	//approx number of steps until the other player goals (0-8)
	//Return early if steps <= stepsMin or steps >= stepsMax
  int winDefSearch(Board& b, move_t* shortestmv, int& shortestnum, int loseStepsMax);
	int winDefSearch(Board& b, move_t* shortestmv, int& shortestnum, int loseStepsMax, int& numInteriorNodes);

	int winDefSearchHelper(Board& b, pla_t pla, int origStepsLeft, int maxSteps, move_t sofar, int sofarNS,
			move_t* shortestmv, int& shortestnum, int loseStepsMax, int& numInteriorNodes);

	//If b.player could win by elim, or must defend against goal, or must defend against elim, get the relevant moves
	//hm is allowed to be NULL
	//If this returns 0, we will lose.
	int genWinConditionMoves(Board& b, move_t* mv, int* hm, int maxSteps);

	//If opponent is threatening elim, gen defenses with maxSteps and return the num. Else, return -1.
	int genElimDefMoves(Board& b, move_t* mv, int* hm, int maxSteps);

	//If opponent is threatening goal, gen defenses with maxSteps and return the num. Else, return -1.
	int genGoalDefMoves(Board& b, move_t* mv, int* hm, int maxSteps);

	//Can we restrict the legal move space because opp goal? Returns true if opp can goal, false else,
	//and if true, returns the bitmaps relevant for defending in maxSteps
	bool getGoalDefRelArea(Board& b, loc_t& rloc, Bitmap& stepRel, Bitmap& pushRel, int maxSteps);

	//Using the goal tree, get the full winning move in the position, if it exists
	move_t getWinningFullMove(Board& b);

	//Reversible Moves and Related ------------------------------------------------------

	//Get the moves that 1-step-reverse a pla piece movement from the opp pushing/pulling our pieces
	int genReverseMoves(const Board& b, const BoardHistory& boardHistory, int cDepth, move_t* mv);

	//Checks if the last move, by the opponent, is reversable.
	//Returns 0 if not reversible, 1 if we can reverse 3 step, 2 if we can reverse fully
	int isReversible(const Board& prev, move_t move, const Board& b, move_t& reverseMove);
	int isReversible(const Board& b, const BoardHistory& hist, move_t& reverseMove);

	//Check if the move that arrived at the current position loses a repetition fight and therefore effectively cannot be made
	bool losesRepetitionFight(const Board& b, const BoardHistory& hist, int cDepth, int& turnsToLoseByRep);

	//Call at step 3 of a search. Not all three steps can be independent. (haizhi step pruning)
	bool isStepPrunable(const Board& b, const BoardHistory& hist, int cDepth);

	//Is this a move that gives a freely capturable piece?
	int isFreeCapturable(const Board& prev, move_t move, Board& finalBoard);

	//Generate all single step reverse moves
	//int getReverseMoves(Board& b, int cDepth, move_t* mvB, int* hmB);

	//Mark suicides with negative value
	//void pruneSuicides(Board& b, int cDepth, move_t* mvB, int* hmB, int num);

	//KILLERS

	//Might not be legal!
	move_t getKiller(const move_t* killerMoves, int killerMovesLen, const Board& b, int cDepth);
	void recordKiller(move_t* killerMoves, int& killerMovesLen, int cDepth, const move_t* pv, int pvLen, const Board& b);

	//HISTORY TABLE---------------------------------------------------
	//Ensure that the history table has been allocated up to the given requested depth of search.
	//Not threadsafe, call only before search!
	void ensureHistory(vector<long long*>& historyTable, vector<long long>& historyMax, int depth);

	//Call whenever we want to clear the history table
	void clearHistory(vector<long long*>& historyTable, vector<long long>& historyMax);

	//Cut all history scores down, call between iterations of iterative deepening
	void decayHistory(vector<long long*>& historyTable, vector<long long>& historyMax);

	//Update the history table
	void updateHistoryTable(vector<long long*>& historyTable, vector<long long>& historyMax,
			const Board& b, int cDepth, move_t move);

	//Fill history scores into the hm array for all moves
	void getHistoryScores(const vector<long long*>& historyTable, const vector<long long>& historyMax,
			const Board& b, pla_t pla, int cDepth, const move_t* mv, int* hm, int len);

	//Get the history score for a single move
	int getHistoryScore(const vector<long long*>& historyTable, const vector<long long>& historyMax,
			const Board& b, pla_t pla, int cDepth, move_t move);

	//PV--------------------------------------------------------------------------------

	void endPV(const move_t* pv, int& pvLen);

	void copyPV(const move_t* srcpv, int srcpvLen, move_t* destpv, int& destpvLen);

	void copyExtendPV(move_t move, const move_t* srcpv, int srcpvLen, move_t* destpv, int& destpvLen);

}



#endif /* SEARCHUTILS_H_ */
