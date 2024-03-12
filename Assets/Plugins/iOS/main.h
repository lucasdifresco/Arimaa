
/*
 * main.h
 * Author: davidwu
 */
#pragma once
#ifndef MAIN_H_
#define MAIN_H_


#include <map>
#include <string>

using namespace std;

typedef int (*MainFunc)(int, const char* const*);

struct MainFuncEntry
{
	MainFunc func;
	string name;
	string doc;

	MainFuncEntry();
	MainFuncEntry(string name, MainFunc func, string doc);
	MainFuncEntry(const char* name, MainFunc func, const char* doc);
	~MainFuncEntry();
};

//A bunch of top-level routines. Return value is an exit code.
namespace MainFuncs
{
	extern int numCommands;
	extern MainFuncEntry commandArr[];
	extern map<string,MainFuncEntry> commandMap;

	//Getmove program, for bot script------------------------------------
	int getMove(int argc, const char* const* argv);

	//Reset seed--------------------------------------------------------
	int init(int argc, const char* const *argv);

	//Positions----------------------------------------------------------
	int searchPos(int argc, const char* const *argv);
	int searchMoves(int argc, const char* const *argv);
	int evalPos(int argc, const char* const *argv);
	int evalMoves(int argc, const char* const *argv);
	int viewPos(int argc, const char* const *argv);
	int viewMoves(int argc, const char* const *argv);
	int loadPos(int argc, const char* const *argv);
	int loadMoves(int argc, const char* const *argv);

	//Match--------------------------------------------------------------
	int playGame(int argc, const char* const *argv);

	//Stats--------------------------------------------------------------
	int avgBranching(int argc, const char* const *argv);
	int avgGameLength(int argc, const char* const *argv);

	//Move Learning------------------------------------------------------
	int learnMoveOrdering(int argc, const char* const *argv);
	int testMoveOrdering(int argc, const char* const *argv);
	int testEvalOrdering(int argc, const char* const *argv);

	int testGameIterator(int argc, const char* const *argv);
	int viewMoveFeatures(int argc, const char* const *argv);
	int testMoveFeatureSpeed(int argc, const char* const *argv);

	//Goal Pattern Generation--------------------------------------------
	int genGoalPatterns(int argc, const char* const *argv);
	int verifyGoalPatterns(int argc, const char* const *argv);

	//Eval---------------------------------------------------------------
	int compareNumStepsLeftValue(int argc, const char* const *argv);
	int runEvalBounds(int argc, const char* const *argv);
	int runEvalBadGood(int argc, const char* const *argv);
	int optimizeEval(int argc, const char* const *argv);

	//Tests--------------------------------------------------------------
	int runBasicTests(int argc, const char* const *argv);
	int runGoalTest(int argc, const char* const *argv);
	int runCapTest(int argc, const char* const *argv);
	int runElimTest(int argc, const char* const *argv);
	int runSearcherWinDefTest(int argc, const char* const *argv);
	int runBTGradientTest(int argc, const char* const *argv);

	//Misc---------------------------------------------------------------
	int createBenchmark(int argc, const char* const *argv);
	int createEvalBadGoodTest(int argc, const char* const *argv);
	int randomizePosFile(int argc, const char* const *argv);

	int findAndSortGoals(int argc, const char* const *argv);
	int testGoalLoseInOnePatterns(int argc, const char* const *argv);

}


#endif /* MAIN_H_ */
