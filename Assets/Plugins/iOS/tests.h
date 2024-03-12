
/*
 * tests.h
 * Author: davidwu
 */

#ifndef TESTS_H
#define TESTS_H

#include <vector>
#include "board.h"
#include "gamerecord.h"

using namespace std;

namespace Tests
{
	void runEvalFeatures(const char* filename);

	//Error tests-------------

	void runBasicTests(uint64_t seed);

	void testGoalTree(const vector<GameRecord>& games, int trustDepth, int testDepth, int numRandomPerturbations, uint64_t seed);

	void testCapTree(const vector<GameRecord>& games, int trustDepth, int testDepth);

	void testElimTree(const vector<GameRecord>& games);

	//Misc Tests---------------

	void testBTGradient();

	void testPatterns();



}

#endif
