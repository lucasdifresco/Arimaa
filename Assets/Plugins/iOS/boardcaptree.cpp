
/*
 * boardcaptree.cpp
 * Author: davidwu
 *
 * Capture detection and generation functions
 */
#include "pch.h"

#include <iostream>
#include "global.h"
#include "bitmap.h"
#include "board.h"
#include "boardtrees.h"
#include "boardtreeconst.h"

using namespace std;

//TODO add 3 step and 4 step captures where we GUARD THE TRAP to avoid sacrificing a piece during a capture

//Cap Tree Helpers
static bool couldPushToTrap(Board& b, pla_t pla, loc_t eloc, loc_t kt, loc_t uf1, loc_t uf2);
static bool canSteps1S(Board& b, pla_t pla, loc_t k);
static int genSteps1S(Board& b, pla_t pla, loc_t k, move_t* mv, int* hm, int hmval);
static bool canPushE(Board& b, loc_t eloc, loc_t fadjloc);
static int genPushPE(Board& b, loc_t ploc, loc_t eloc, loc_t fadjloc, move_t* mv, int* hm, int hmval);
static int genPullPE(Board& b, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canPushc(Board& b, pla_t pla, loc_t eloc);
static int genPush(Board& b, pla_t pla, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canPullc(Board& b, pla_t pla, loc_t eloc);
static int genPull(Board& b, pla_t pla, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canPPPE(Board& b, loc_t ploc, loc_t eloc);
static bool canUF(Board& b, pla_t pla, loc_t ploc);
static int genUF(Board& b, pla_t pla, loc_t ploc, move_t* mv, int* hm, int hmval);
static bool canUFAt(Board& b, pla_t pla, loc_t loc);
static int genUFAt(Board& b, pla_t pla, loc_t loc, move_t* mv, int* hm, int hmval);
static bool canUFPPPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc);
static int genUFPPPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canUF2(Board& b, pla_t pla, loc_t ploc);
static int genUF2(Board& b, pla_t pla, loc_t ploc, move_t* mv, int* hm, int hmval);
static bool canPPCapAndUFc(Board& b, pla_t pla, loc_t eloc, loc_t ploc);
static int genPPCapAndUF(Board& b, pla_t pla, loc_t eloc, loc_t ploc, move_t* mv, int* hm, int hmval);
static bool canGetPlaTo2S(Board& b, pla_t pla, loc_t k, loc_t floc);
static int genGetPlaTo2S(Board& b, pla_t pla, loc_t k, loc_t floc, move_t* mv, int* hm, int hmval);
static bool canUFUFPPPE(Board& b, pla_t pla, loc_t ploc, loc_t plocdest, loc_t ploc2, loc_t eloc);
static int genUFUFPPPE(Board& b, pla_t pla, loc_t ploc, loc_t plocdest, loc_t ploc2, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canStepStepPPPE(Board& b, pla_t pla, loc_t dest, loc_t dest2, loc_t ploc, loc_t eloc);
static int genStepStepPPPE(Board& b, pla_t pla, loc_t dest, loc_t dest2, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canPushPPPE(Board& b, pla_t pla, loc_t floc, loc_t ploc, loc_t eloc);
static int genPushPPPE(Board& b, pla_t pla, loc_t floc, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canUF2PPPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc);
static int genUF2PPPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canGetPlaTo2SPPPE(Board& b, pla_t pla, loc_t k, loc_t ploc, loc_t eloc);
static int genGetPlaTo2SPPPE(Board& b, pla_t pla, loc_t k, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canPP(Board& b, pla_t pla, loc_t eloc);
static int genPP(Board& b, pla_t pla, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canPPTo(Board& b, pla_t pla, loc_t eloc, loc_t dest);
static int genPPTo(Board& b, pla_t pla, loc_t eloc, loc_t dest, move_t* mv, int* hm, int hmval);
static bool canMoveStrongAdjCent(Board& b, pla_t pla, loc_t dest, loc_t eloc, loc_t forbidden);
static int genMoveStrongAdjCent(Board& b, pla_t pla, loc_t dest, loc_t eloc, loc_t forbidden, move_t* mv, int* hm, int hmval);
static bool canAdvanceUFStep(Board& b, pla_t pla, loc_t curploc, loc_t futploc);
static int genAdvanceUFStep(Board& b, pla_t pla, loc_t curploc, loc_t futploc, move_t* mv, int* hm, int hmval);
static bool canUFStepWouldBeUF(Board& b, pla_t pla, loc_t ploc, loc_t wloc, loc_t kt);
static int genUFStepWouldBeUF(Board& b, pla_t pla, loc_t ploc, loc_t wloc, loc_t kt,move_t* mv, int* hm, int hmval);
static bool canStepStepc(Board& b, pla_t pla, loc_t dest, loc_t dest2);
static int genStepStep(Board& b, pla_t pla, loc_t dest, loc_t dest2, move_t* mv, int* hm, int hmval);


//2 STEP CAPTURES----------------------------------------------------------------------
static bool canPPIntoTrap2C(Board& b, pla_t pla, loc_t eloc, loc_t kt);
static int genPPIntoTrap2C(Board& b, pla_t pla, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);
static bool canRemoveDef2C(Board& b, pla_t pla, loc_t eloc);
static int genRemoveDef2C(Board& b, pla_t pla, loc_t eloc, move_t* mv, int* hm, int hmval);

//3 STEP CAPTURES----------------------------------------------------------------------
static bool canPPIntoTrapTE3C(Board& b, pla_t pla, loc_t eloc, loc_t kt);
static int genPPIntoTrapTE3C(Board& b, pla_t pla, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);
static bool canPPIntoTrapTP3C(Board& b, pla_t pla, loc_t eloc, loc_t kt);
static int genPPIntoTrapTP3C(Board& b, pla_t pla, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);
static bool canRemoveDef3C(Board& b, pla_t pla, loc_t eloc, loc_t kt);
static int genRemoveDef3C(Board& b, pla_t pla, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);

//4-STEP CAPTURES---------------------------------------------------------------------

//0 DEFENDERS-----------
static bool can0PieceTrail(Board& b, pla_t pla, loc_t eloc, loc_t tr, loc_t kt);
static int gen0PieceTrail(Board& b, pla_t pla, loc_t eloc, loc_t tr, loc_t kt, move_t* mv, int* hm, int hmval);

//2 DEFENDERS------------
static bool can2PieceAdj(Board& b, pla_t pla, loc_t eloc1, loc_t eloc2, loc_t kt);
static int gen2PieceAdj(Board& b, pla_t pla, loc_t eloc1, loc_t eloc2, loc_t kt, move_t* mv, int* hm, int hmval);

//4-STEP CAPTURES, 1 DEFENDER-----
static bool canMoveStrong2S(Board& b, pla_t pla, loc_t sloc, loc_t eloc);
static int genMoveStrong2S(Board& b, pla_t pla, loc_t sloc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canSwapPla(Board& b, pla_t pla, loc_t sloc, loc_t eloc);
static int genSwapPla(Board& b, pla_t pla, loc_t sloc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canSwapOpp(Board& b, pla_t pla, loc_t sloc, loc_t eloc, loc_t kt);
static int genSwapOpp(Board& b, pla_t pla, loc_t sloc, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);
static bool canPPIntoTrapTE4C(Board& b, pla_t pla, loc_t eloc, loc_t kt);
static int genPPIntoTrapTE4C(Board& b, pla_t pla, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);
static bool canPPIntoTrapTP4C(Board& b, pla_t pla, loc_t eloc, loc_t kt);
static int genPPIntoTrapTP4C(Board& b, pla_t pla, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);
static bool canRemoveDef4C(Board& b, pla_t pla, loc_t eloc, loc_t kt);
static int genRemoveDef4C(Board& b, pla_t pla, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);
static bool canEmpty2(Board& b, pla_t pla, loc_t loc, loc_t floc, loc_t ploc);
static int genEmpty2(Board& b, pla_t pla, loc_t loc, loc_t floc, loc_t ploc, move_t* mv, int* hm, int hmval);
static bool canUFBlockedPP(Board& b, pla_t pla, loc_t ploc, loc_t eloc);
static int genUFBlockedPP(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canBlockedPP(Board& b, pla_t pla, loc_t ploc, loc_t eloc);
static int genBlockedPP(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval);
static bool canBlockedPP2(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t kt);
static int genBlockedPP2(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t kt, move_t* mv, int* hm, int hmval);
static loc_t findAdjOpp(Board& b, pla_t pla, loc_t k);
static bool canAdvancePullCap(Board& b, pla_t pla, loc_t ploc, loc_t tr);
static int genAdvancePullCap(Board& b, pla_t pla, loc_t ploc, loc_t tr, move_t* mv, int* hm, int hmval);


//TOP-LEVEL FUNCTIONS-----------------------------------------------------------------

bool BoardTrees::canCaps(Board& b, pla_t pla, int steps)
{
	return
	canCaps(b,pla,steps,2,18) ||
	canCaps(b,pla,steps,2,21) ||
	canCaps(b,pla,steps,2,42) ||
	canCaps(b,pla,steps,2,45);
}

int BoardTrees::genCaps(Board& b, pla_t pla, int steps,
move_t* mv, int* hm)
{
	int num = 0;
	num += genCaps(b,pla,steps,2,18,mv+num,hm+num);
	num += genCaps(b,pla,steps,2,21,mv+num,hm+num);
	num += genCaps(b,pla,steps,2,42,mv+num,hm+num);
	num += genCaps(b,pla,steps,2,45,mv+num,hm+num);
	return num;
}

int BoardTrees::genCapsFull(Board& b, pla_t pla, int steps, int minSteps, bool suicideExtend, loc_t kt, move_t* mv, int* hm)
{
	int stepsUsed = 0;
	int num = genCapsOption(b,pla,steps,minSteps,suicideExtend,kt,mv,hm,&stepsUsed);

	if(steps == 2)
		return num;

	//Cumulate into full capture moves. First, split the full captures into complete ones and incomplete ones.
	int numMvBuf = 0;
	int goodNum = 0;
	move_t* mvBuf = new move_t[num];
	int* hmBuf = new int[num];
	//move_t mvBuf[num];
	//int hmBuf[num];
	int minStepsInBufMove = 4;
	for(int i = 0; i<num; i++)
	{
		int ns = Board::numStepsInMove(mv[i]);
		if(ns < stepsUsed)
		{
			mvBuf[numMvBuf] = mv[i];
			hmBuf[numMvBuf] = hm[i];
			numMvBuf++;
			if(ns < minStepsInBufMove)
				minStepsInBufMove = ns;
		}
		else
		{
			mv[goodNum] = mv[i];
			hm[goodNum] = hm[i];
			goodNum++;
		}
	}

	//Okay, these are all good, so pack them away
	int totalNum = 0;
	mv += goodNum;
	hm += goodNum;
	totalNum += goodNum;

	//Now, finish off all the incomplete ones
	for(int i = 0; i<numMvBuf; i++)
	{
		move_t move = mvBuf[i];
		int ns = Board::numStepsInMove(move);

		Board copy = b;
		copy.makeMove(move);

		if(copy.pieceCounts[OPP(pla)][0] < b.pieceCounts[OPP(pla)][0])
		{
			mv[0] = move;
			hm[0] = hmBuf[i];
			mv++;
			hm++;
			totalNum++;
		}
		else
		{
			DEBUGASSERT(!(stepsUsed-ns == 1 || ns >= 4));
			int newNum = genCapsFull(copy,pla,stepsUsed-ns,minStepsInBufMove-ns,false,kt,mv,hm);
			for(int k = 0; k<newNum; k++) {
				mv[k] = Board::concatMoves(move,mv[k],ns);
			}
			mv += newNum;
			hm += newNum;
			totalNum += newNum;
		}
	}

	delete[] mvBuf;
	delete[] hmBuf;
	return totalNum;
}

static bool isOnlySuicides(Board& b, pla_t pla, move_t* mv, int* hm, int& num)
{
	int newNum = 0;
	bool onlySuicides = true;
	for(int i = 0; i<num; i++)
	{
		loc_t src[8];
		loc_t dest[8];
		bool moveHasSuicide = false;
		piece_t suicideStr = EMP;
		int numChanges = b.getChanges(mv[i],src,dest);
		for(int j = 0; j<numChanges; j++)
		{
			if(b.owners[src[j]] == pla && dest[j] == ERRORSQUARE)
			{moveHasSuicide = true; suicideStr = b.pieces[src[j]];}
		}
		if(!moveHasSuicide)
		{onlySuicides = false;}

		if(!moveHasSuicide || suicideStr < hm[i])
		{mv[newNum] = mv[i]; hm[newNum] = hm[i]; newNum++;}
	}
	num = newNum;
	return onlySuicides;
}

//Goes a little further and generates captures using more steps than the minimum possible
int BoardTrees::genCapsOption(Board& b, pla_t pla, int steps, int minSteps, bool suicideExtend, loc_t kt,
move_t* mv, int* hm, int* stepsUsed)
{
	int num = 0;
	if(steps <= 1)
		return num;

	if(minSteps <= 2)
	{
		int newNum = genCaps(b,pla,2,2,kt,mv+num,hm+num);
		if(steps <= 2 || (num > 0 && !(suicideExtend && isOnlySuicides(b,pla,mv+num,hm+num,newNum))))
		{if(stepsUsed != NULL) *stepsUsed = 2; return num+newNum;}
		num += newNum;
	}

	if(minSteps <= 3)
	{
		int newNum = genCaps(b,pla,3,3,kt,mv+num,hm+num);
		if(steps <= 3 || (num > 0 && !(suicideExtend && isOnlySuicides(b,pla,mv+num,hm+num,newNum))))
		{if(stepsUsed != NULL) *stepsUsed = 3; return num+newNum;}
		num += newNum;
	}

	int newNum = genCaps(b,pla,4,4,kt,mv+num,hm+num);

	isOnlySuicides(b,pla,mv+num,hm+num,newNum);
	if(stepsUsed != NULL) *stepsUsed = 4;
	return num+newNum;
}

bool BoardTrees::canCaps(Board& b, pla_t pla, int steps, loc_t kt, Bitmap& capMap, int& capDist)
{
#ifdef CHECK_CAPTREE_CONSISTENCY
	assert(b.testConsistency(cout));
#endif

	pla_t opp = OPP(pla);

	bool oppS = ISO(kt S);
	bool oppW = ISO(kt W);
	bool oppE = ISO(kt E);
	bool oppN = ISO(kt N);

	capDist = 5;
	int defNum = oppS + oppW + oppE + oppN;
	pla_t trapOwner = b.owners[kt];

	//If 3 defenders or elephant guarding, capture impossible.
	//If fewer than 4 steps, the only feasible case is with 1 defender
	//If fewer than 2 steps, impossible to capture anything.
	//If more than 4 steps, we do not generate anything, since we only generate up to 4 step caps.
	if(
	defNum >= 3 ||
	(steps < 4 && defNum != 1) ||
	steps < 2 ||
	steps > 4 ||
	(oppS && b.pieces[kt S] == ELE) ||
	(oppW && b.pieces[kt W] == ELE) ||
	(oppE && b.pieces[kt E] == ELE) ||
	(oppN && b.pieces[kt N] == ELE)
	)
	{return false;}

	if(steps >= 2)
	{
	  capDist = 2;
		if(defNum == 1)
		{
			if(trapOwner != opp)
			{
				if     (oppS) {if(canPPIntoTrap2C(b,pla,kt S,kt)) {capMap.setOn(kt S); return true;}}
				else if(oppW) {if(canPPIntoTrap2C(b,pla,kt W,kt)) {capMap.setOn(kt W); return true;}}
				else if(oppE) {if(canPPIntoTrap2C(b,pla,kt E,kt)) {capMap.setOn(kt E); return true;}}
				else          {if(canPPIntoTrap2C(b,pla,kt N,kt)) {capMap.setOn(kt N); return true;}}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {if(canRemoveDef2C(b,pla,kt S)){capMap.setOn(kt); return true;}}
				else if(oppW) {if(canRemoveDef2C(b,pla,kt W)){capMap.setOn(kt); return true;}}
				else if(oppE) {if(canRemoveDef2C(b,pla,kt E)){capMap.setOn(kt); return true;}}
				else          {if(canRemoveDef2C(b,pla,kt N)){capMap.setOn(kt); return true;}}
			}
		}
	}
	if(steps >= 3)
	{
    capDist = 3;
		if(defNum == 1)
		{
			if(trapOwner == NPLA)
			{
				if     (oppS) {if(canPPIntoTrapTE3C(b,pla,kt S,kt)){capMap.setOn(kt S); return true;}}
				else if(oppW) {if(canPPIntoTrapTE3C(b,pla,kt W,kt)){capMap.setOn(kt W); return true;}}
				else if(oppE) {if(canPPIntoTrapTE3C(b,pla,kt E,kt)){capMap.setOn(kt E); return true;}}
				else          {if(canPPIntoTrapTE3C(b,pla,kt N,kt)){capMap.setOn(kt N); return true;}}
			}
			else if(trapOwner == pla)
			{
				if     (oppS) {if(canPPIntoTrapTP3C(b,pla,kt S,kt)){capMap.setOn(kt S); return true;}}
				else if(oppW) {if(canPPIntoTrapTP3C(b,pla,kt W,kt)){capMap.setOn(kt W); return true;}}
				else if(oppE) {if(canPPIntoTrapTP3C(b,pla,kt E,kt)){capMap.setOn(kt E); return true;}}
				else          {if(canPPIntoTrapTP3C(b,pla,kt N,kt)){capMap.setOn(kt N); return true;}}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {if(canRemoveDef3C(b,pla,kt S,kt)){capMap.setOn(kt); return true;}}
				else if(oppW) {if(canRemoveDef3C(b,pla,kt W,kt)){capMap.setOn(kt); return true;}}
				else if(oppE) {if(canRemoveDef3C(b,pla,kt E,kt)){capMap.setOn(kt); return true;}}
				else          {if(canRemoveDef3C(b,pla,kt N,kt)){capMap.setOn(kt); return true;}}
			}
		}
	}
	if(steps >= 4)
	{
    capDist = 4;
		if(defNum == 0)
		{
			bool foundCap = false;
			if(ISO(kt SS)) {if(can0PieceTrail(b,pla,kt SS,kt S,kt)){capMap.setOn(kt SS); foundCap = true;}}
			if(ISO(kt WW)) {if(can0PieceTrail(b,pla,kt WW,kt W,kt)){capMap.setOn(kt WW); foundCap = true;}}
			if(ISO(kt EE)) {if(can0PieceTrail(b,pla,kt EE,kt E,kt)){capMap.setOn(kt EE); foundCap = true;}}
			if(ISO(kt NN)) {if(can0PieceTrail(b,pla,kt NN,kt N,kt)){capMap.setOn(kt NN); foundCap = true;}}

			if(ISO(kt SW))
			{if(can0PieceTrail(b,pla,kt SW,kt S,kt)
			  ||can0PieceTrail(b,pla,kt SW,kt W,kt)){capMap.setOn(kt SW); foundCap = true;}}
			if(ISO(kt SE))
			{if(can0PieceTrail(b,pla,kt SE,kt S,kt)
			  ||can0PieceTrail(b,pla,kt SE,kt E,kt)){capMap.setOn(kt SE); foundCap = true;}}
			if(ISO(kt NW))
			{if(can0PieceTrail(b,pla,kt NW,kt W,kt)
			  ||can0PieceTrail(b,pla,kt NW,kt N,kt)){capMap.setOn(kt NW); foundCap = true;}}
			if(ISO(kt NE))
			{if(can0PieceTrail(b,pla,kt NE,kt E,kt)
			  ||can0PieceTrail(b,pla,kt NE,kt N,kt)){capMap.setOn(kt NE); foundCap = true;}}

			if(foundCap)
				return true;
		}
		else if(defNum == 1)
		{
			if(trapOwner == NPLA)
			{
				if     (oppS) {if(canPPIntoTrapTE4C(b,pla,kt S,kt)){capMap.setOn(kt S); return true;}}
				else if(oppW) {if(canPPIntoTrapTE4C(b,pla,kt W,kt)){capMap.setOn(kt W); return true;}}
				else if(oppE) {if(canPPIntoTrapTE4C(b,pla,kt E,kt)){capMap.setOn(kt E); return true;}}
				else          {if(canPPIntoTrapTE4C(b,pla,kt N,kt)){capMap.setOn(kt N); return true;}}
			}
			else if(trapOwner == pla)
			{
				if     (oppS) {if(canPPIntoTrapTP4C(b,pla,kt S,kt)){capMap.setOn(kt S); return true;}}
				else if(oppW) {if(canPPIntoTrapTP4C(b,pla,kt W,kt)){capMap.setOn(kt W); return true;}}
				else if(oppE) {if(canPPIntoTrapTP4C(b,pla,kt E,kt)){capMap.setOn(kt E); return true;}}
				else          {if(canPPIntoTrapTP4C(b,pla,kt N,kt)){capMap.setOn(kt N); return true;}}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {if(canRemoveDef4C(b,pla,kt S,kt)){capMap.setOn(kt); return true;}}
				else if(oppW) {if(canRemoveDef4C(b,pla,kt W,kt)){capMap.setOn(kt); return true;}}
				else if(oppE) {if(canRemoveDef4C(b,pla,kt E,kt)){capMap.setOn(kt); return true;}}
				else          {if(canRemoveDef4C(b,pla,kt N,kt)){capMap.setOn(kt); return true;}}
			}
		}
		else //if(defNum == 2)
		{
			if(oppS)
			{
				if(oppW)
				{if(can2PieceAdj(b,pla,kt S,kt W,kt)
				  ||can2PieceAdj(b,pla,kt W,kt S,kt)){if(ISO(kt)) {capMap.setOn(kt);} else{capMap.setOn(kt S); capMap.setOn(kt W);} return true;}}
				else if(oppE)
				{if(can2PieceAdj(b,pla,kt S,kt E,kt)
				  ||can2PieceAdj(b,pla,kt E,kt S,kt)){if(ISO(kt)) {capMap.setOn(kt);} else{capMap.setOn(kt S); capMap.setOn(kt E);} return true;}}
				else if(oppN)
				{if(can2PieceAdj(b,pla,kt S,kt N,kt)
				  ||can2PieceAdj(b,pla,kt N,kt S,kt)){if(ISO(kt)) {capMap.setOn(kt);} else{capMap.setOn(kt S); capMap.setOn(kt N);} return true;}}
			}
			else if(oppW)
			{
				if(oppE)
				{if(can2PieceAdj(b,pla,kt W,kt E,kt)
				  ||can2PieceAdj(b,pla,kt E,kt W,kt)){if(ISO(kt)) {capMap.setOn(kt);} else{capMap.setOn(kt W); capMap.setOn(kt E);} return true;}}
				else if(oppN)
				{if(can2PieceAdj(b,pla,kt W,kt N,kt)
				  ||can2PieceAdj(b,pla,kt N,kt W,kt)){if(ISO(kt)) {capMap.setOn(kt);} else{capMap.setOn(kt W); capMap.setOn(kt N);} return true;}}
			}
			else
			{if(can2PieceAdj(b,pla,kt E,kt N,kt)
			  ||can2PieceAdj(b,pla,kt N,kt E,kt)){if(ISO(kt)) {capMap.setOn(kt);} else{capMap.setOn(kt E); capMap.setOn(kt N);} return true;}}
		}
	}

	return false;
}

//TODO avoid code duplication and combine this with the above, using a dummy bitmap
//Test speed!
bool BoardTrees::canCaps(Board& b, pla_t pla, int steps, int minSteps, loc_t kt)
{
#ifdef CHECK_CAPTREE_CONSISTENCY
	assert(b.testConsistency(cout));
#endif

	pla_t opp = OPP(pla);

	bool oppS = ISO(kt S);
	bool oppW = ISO(kt W);
	bool oppE = ISO(kt E);
	bool oppN = ISO(kt N);

	int defNum = oppS + oppW + oppE + oppN;
	pla_t trapOwner = b.owners[kt];

	//If 3 defenders or elephant guarding, capture impossible.
	//If fewer than 4 steps, the only feasible case is with 1 defender
	//If fewer than 2 steps, impossible to capture anything.
	//If more than 4 steps, we do not generate anything, since we only generate up to 4 step caps.
	if(
	defNum >= 3 ||
	(steps < 4 && defNum != 1) ||
	steps < 2 ||
	steps > 4 ||
	(oppS && b.pieces[kt S] == ELE) ||
	(oppW && b.pieces[kt W] == ELE) ||
	(oppE && b.pieces[kt E] == ELE) ||
	(oppN && b.pieces[kt N] == ELE)
	)
	{return false;}

	if(steps >= 2 && minSteps <= 2)
	{
		if(defNum == 1)
		{
			if(trapOwner != opp)
			{
				if     (oppS) {RIF(canPPIntoTrap2C(b,pla,kt S,kt))}
				else if(oppW) {RIF(canPPIntoTrap2C(b,pla,kt W,kt))}
				else if(oppE) {RIF(canPPIntoTrap2C(b,pla,kt E,kt))}
				else          {RIF(canPPIntoTrap2C(b,pla,kt N,kt))}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {RIF(canRemoveDef2C(b,pla,kt S))}
				else if(oppW) {RIF(canRemoveDef2C(b,pla,kt W))}
				else if(oppE) {RIF(canRemoveDef2C(b,pla,kt E))}
				else          {RIF(canRemoveDef2C(b,pla,kt N))}
			}
		}
	}
	if(steps >= 3 && minSteps <= 3)
	{
		if(defNum == 1)
		{
			if(trapOwner == NPLA)
			{
				if     (oppS) {RIF(canPPIntoTrapTE3C(b,pla,kt S,kt))}
				else if(oppW) {RIF(canPPIntoTrapTE3C(b,pla,kt W,kt))}
				else if(oppE) {RIF(canPPIntoTrapTE3C(b,pla,kt E,kt))}
				else          {RIF(canPPIntoTrapTE3C(b,pla,kt N,kt))}
			}
			else if(trapOwner == pla)
			{
				if     (oppS) {RIF(canPPIntoTrapTP3C(b,pla,kt S,kt))}
				else if(oppW) {RIF(canPPIntoTrapTP3C(b,pla,kt W,kt))}
				else if(oppE) {RIF(canPPIntoTrapTP3C(b,pla,kt E,kt))}
				else          {RIF(canPPIntoTrapTP3C(b,pla,kt N,kt))}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {RIF(canRemoveDef3C(b,pla,kt S,kt))}
				else if(oppW) {RIF(canRemoveDef3C(b,pla,kt W,kt))}
				else if(oppE) {RIF(canRemoveDef3C(b,pla,kt E,kt))}
				else          {RIF(canRemoveDef3C(b,pla,kt N,kt))}
			}
		}
	}
	if(steps >= 4 && minSteps <= 4)
	{
		if(defNum == 0)
		{
			if(ISO(kt SS))
			{RIF(can0PieceTrail(b,pla,kt SS,kt S,kt))}
			if(ISO(kt WW))
			{RIF(can0PieceTrail(b,pla,kt WW,kt W,kt))}
			if(ISO(kt EE))
			{RIF(can0PieceTrail(b,pla,kt EE,kt E,kt))}
			if(ISO(kt NN))
			{RIF(can0PieceTrail(b,pla,kt NN,kt N,kt))}

			if(ISO(kt SW))
			{RIF(can0PieceTrail(b,pla,kt SW,kt S,kt))
			 RIF(can0PieceTrail(b,pla,kt SW,kt W,kt))}
			if(ISO(kt SE))
			{RIF(can0PieceTrail(b,pla,kt SE,kt S,kt))
			 RIF(can0PieceTrail(b,pla,kt SE,kt E,kt))}
			if(ISO(kt NW))
			{RIF(can0PieceTrail(b,pla,kt NW,kt W,kt))
			 RIF(can0PieceTrail(b,pla,kt NW,kt N,kt))}
			if(ISO(kt NE))
			{RIF(can0PieceTrail(b,pla,kt NE,kt E,kt))
			 RIF(can0PieceTrail(b,pla,kt NE,kt N,kt))}
		}
		else if(defNum == 1)
		{
			if(trapOwner == NPLA)
			{
				if     (oppS) {RIF(canPPIntoTrapTE4C(b,pla,kt S,kt))}
				else if(oppW) {RIF(canPPIntoTrapTE4C(b,pla,kt W,kt))}
				else if(oppE) {RIF(canPPIntoTrapTE4C(b,pla,kt E,kt))}
				else          {RIF(canPPIntoTrapTE4C(b,pla,kt N,kt))}
			}
			else if(trapOwner == pla)
			{
				if     (oppS) {RIF(canPPIntoTrapTP4C(b,pla,kt S,kt))}
				else if(oppW) {RIF(canPPIntoTrapTP4C(b,pla,kt W,kt))}
				else if(oppE) {RIF(canPPIntoTrapTP4C(b,pla,kt E,kt))}
				else          {RIF(canPPIntoTrapTP4C(b,pla,kt N,kt))}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {RIF(canRemoveDef4C(b,pla,kt S,kt))}
				else if(oppW) {RIF(canRemoveDef4C(b,pla,kt W,kt))}
				else if(oppE) {RIF(canRemoveDef4C(b,pla,kt E,kt))}
				else          {RIF(canRemoveDef4C(b,pla,kt N,kt))}
			}
		}
		else //if(defNum == 2)
		{
			if(oppS)
			{
				if(oppW)
				{RIF(can2PieceAdj(b,pla,kt S,kt W,kt))
				 RIF(can2PieceAdj(b,pla,kt W,kt S,kt))}
				else if(oppE)
				{RIF(can2PieceAdj(b,pla,kt S,kt E,kt))
				 RIF(can2PieceAdj(b,pla,kt E,kt S,kt))}
				else if(oppN)
				{RIF(can2PieceAdj(b,pla,kt S,kt N,kt))
				 RIF(can2PieceAdj(b,pla,kt N,kt S,kt))}
			}
			else if(oppW)
			{
				if(oppE)
				{RIF(can2PieceAdj(b,pla,kt W,kt E,kt))
				 RIF(can2PieceAdj(b,pla,kt E,kt W,kt))}
				else if(oppN)
				{RIF(can2PieceAdj(b,pla,kt W,kt N,kt))
				 RIF(can2PieceAdj(b,pla,kt N,kt W,kt))}
			}
			else
			{RIF(can2PieceAdj(b,pla,kt E,kt N,kt))
			 RIF(can2PieceAdj(b,pla,kt N,kt E,kt))}
		}
	}

	return false;
}

int BoardTrees::genCaps(Board& b, pla_t pla, int steps, int minSteps, loc_t kt,
move_t* mv, int* hm)
{
#ifdef CHECK_CAPTREE_CONSISTENCY
	assert(b.testConsistency(cout));
#endif

	int num = 0;
	pla_t opp = OPP(pla);

	bool oppS = ISO(kt S);
	bool oppW = ISO(kt W);
	bool oppE = ISO(kt E);
	bool oppN = ISO(kt N);

	int defNum = oppS + oppW + oppE + oppN;
	pla_t trapOwner = b.owners[kt];

	//If 3 defenders or elephant guarding, capture impossible.
	//If fewer than 4 steps, the only feasible case is with 1 defender
	//If fewer than 2 steps, impossible to capture anything.
	//If more than 4 steps, we do not generate anything, since we only generate up to 4 step caps.
	if(
	defNum >= 3 ||
	(steps < 4 && defNum != 1) ||
	steps < 2 ||
	steps > 4 ||
	(oppS && b.pieces[kt S] == ELE) ||
	(oppW && b.pieces[kt W] == ELE) ||
	(oppE && b.pieces[kt E] == ELE) ||
	(oppN && b.pieces[kt N] == ELE)
	)
	{return 0;}

	if(steps >= 2 && minSteps <= 2)
	{
		if(defNum == 1)
		{
			if(trapOwner != opp)
			{
				if     (oppS) {num += genPPIntoTrap2C(b,pla,kt S,kt,mv+num,hm+num,b.pieces[kt S]);}
				else if(oppW) {num += genPPIntoTrap2C(b,pla,kt W,kt,mv+num,hm+num,b.pieces[kt W]);}
				else if(oppE) {num += genPPIntoTrap2C(b,pla,kt E,kt,mv+num,hm+num,b.pieces[kt E]);}
				else          {num += genPPIntoTrap2C(b,pla,kt N,kt,mv+num,hm+num,b.pieces[kt N]);}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {num += genRemoveDef2C(b,pla,kt S,mv+num,hm+num,b.pieces[kt]);}
				else if(oppW) {num += genRemoveDef2C(b,pla,kt W,mv+num,hm+num,b.pieces[kt]);}
				else if(oppE) {num += genRemoveDef2C(b,pla,kt E,mv+num,hm+num,b.pieces[kt]);}
				else          {num += genRemoveDef2C(b,pla,kt N,mv+num,hm+num,b.pieces[kt]);}
			}
		}
		if(num > 0)
		{return num;}
	}

	if(steps >= 3 && minSteps <= 3)
	{
		if(defNum == 1)
		{
			if(trapOwner == NPLA)
			{
				if     (oppS) {num += genPPIntoTrapTE3C(b,pla,kt S,kt,mv+num,hm+num,b.pieces[kt S]);}
				else if(oppW) {num += genPPIntoTrapTE3C(b,pla,kt W,kt,mv+num,hm+num,b.pieces[kt W]);}
				else if(oppE) {num += genPPIntoTrapTE3C(b,pla,kt E,kt,mv+num,hm+num,b.pieces[kt E]);}
				else          {num += genPPIntoTrapTE3C(b,pla,kt N,kt,mv+num,hm+num,b.pieces[kt N]);}
			}
			else if(trapOwner == pla)
			{
				if     (oppS) {num += genPPIntoTrapTP3C(b,pla,kt S,kt,mv+num,hm+num,b.pieces[kt S]);}
				else if(oppW) {num += genPPIntoTrapTP3C(b,pla,kt W,kt,mv+num,hm+num,b.pieces[kt W]);}
				else if(oppE) {num += genPPIntoTrapTP3C(b,pla,kt E,kt,mv+num,hm+num,b.pieces[kt E]);}
				else          {num += genPPIntoTrapTP3C(b,pla,kt N,kt,mv+num,hm+num,b.pieces[kt N]);}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {num += genRemoveDef3C(b,pla,kt S,kt,mv+num,hm+num,b.pieces[kt]);}
				else if(oppW) {num += genRemoveDef3C(b,pla,kt W,kt,mv+num,hm+num,b.pieces[kt]);}
				else if(oppE) {num += genRemoveDef3C(b,pla,kt E,kt,mv+num,hm+num,b.pieces[kt]);}
				else          {num += genRemoveDef3C(b,pla,kt N,kt,mv+num,hm+num,b.pieces[kt]);}
			}
		}
		if(num > 0)
		{return num;}
	}

	if(steps >= 4 && minSteps <= 4)
	{
		if(defNum == 0)
		{
			if(ISO(kt SS))
			{num += gen0PieceTrail(b,pla,kt SS,kt S,kt,mv+num,hm+num,b.pieces[kt SS]);}
			if(ISO(kt WW))
			{num += gen0PieceTrail(b,pla,kt WW,kt W,kt,mv+num,hm+num,b.pieces[kt WW]);}
			if(ISO(kt EE))
			{num += gen0PieceTrail(b,pla,kt EE,kt E,kt,mv+num,hm+num,b.pieces[kt EE]);}
			if(ISO(kt NN))
			{num += gen0PieceTrail(b,pla,kt NN,kt N,kt,mv+num,hm+num,b.pieces[kt NN]);}

			if(ISO(kt SW))
			{num += gen0PieceTrail(b,pla,kt SW,kt S,kt,mv+num,hm+num,b.pieces[kt SW]);
			 num += gen0PieceTrail(b,pla,kt SW,kt W,kt,mv+num,hm+num,b.pieces[kt SW]);}
			if(ISO(kt SE))
			{num += gen0PieceTrail(b,pla,kt SE,kt S,kt,mv+num,hm+num,b.pieces[kt SE]);
			 num += gen0PieceTrail(b,pla,kt SE,kt E,kt,mv+num,hm+num,b.pieces[kt SE]);}
			if(ISO(kt NW))
			{num += gen0PieceTrail(b,pla,kt NW,kt W,kt,mv+num,hm+num,b.pieces[kt NW]);
			 num += gen0PieceTrail(b,pla,kt NW,kt N,kt,mv+num,hm+num,b.pieces[kt NW]);}
			if(ISO(kt NE))
			{num += gen0PieceTrail(b,pla,kt NE,kt E,kt,mv+num,hm+num,b.pieces[kt NE]);
			 num += gen0PieceTrail(b,pla,kt NE,kt N,kt,mv+num,hm+num,b.pieces[kt NE]);}
		}
		else if(defNum == 1)
		{
			if(trapOwner == NPLA)
			{
				if     (oppS) {num += genPPIntoTrapTE4C(b,pla,kt S,kt,mv+num,hm+num,b.pieces[kt S]);}
				else if(oppW) {num += genPPIntoTrapTE4C(b,pla,kt W,kt,mv+num,hm+num,b.pieces[kt W]);}
				else if(oppE) {num += genPPIntoTrapTE4C(b,pla,kt E,kt,mv+num,hm+num,b.pieces[kt E]);}
				else          {num += genPPIntoTrapTE4C(b,pla,kt N,kt,mv+num,hm+num,b.pieces[kt N]);}
			}
			else if(trapOwner == pla)
			{
				if     (oppS) {num += genPPIntoTrapTP4C(b,pla,kt S,kt,mv+num,hm+num,b.pieces[kt S]);}
				else if(oppW) {num += genPPIntoTrapTP4C(b,pla,kt W,kt,mv+num,hm+num,b.pieces[kt W]);}
				else if(oppE) {num += genPPIntoTrapTP4C(b,pla,kt E,kt,mv+num,hm+num,b.pieces[kt E]);}
				else          {num += genPPIntoTrapTP4C(b,pla,kt N,kt,mv+num,hm+num,b.pieces[kt N]);}
			}
			else //if(trapOwner == opp)
			{
				if     (oppS) {num += genRemoveDef4C(b,pla,kt S,kt,mv+num,hm+num,b.pieces[kt]);}
				else if(oppW) {num += genRemoveDef4C(b,pla,kt W,kt,mv+num,hm+num,b.pieces[kt]);}
				else if(oppE) {num += genRemoveDef4C(b,pla,kt E,kt,mv+num,hm+num,b.pieces[kt]);}
				else          {num += genRemoveDef4C(b,pla,kt N,kt,mv+num,hm+num,b.pieces[kt]);}
			}
		}
		else //if(defNum == 2)
		{
			if(oppS)
			{
				if(oppW)
				{num += gen2PieceAdj(b,pla,kt S,kt W,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt S]);
				 num += gen2PieceAdj(b,pla,kt W,kt S,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt W]);}
				else if(oppE)
				{num += gen2PieceAdj(b,pla,kt S,kt E,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt S]);
				 num += gen2PieceAdj(b,pla,kt E,kt S,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt E]);}
				else if(oppN)
				{num += gen2PieceAdj(b,pla,kt S,kt N,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt S]);
				 num += gen2PieceAdj(b,pla,kt N,kt S,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt N]);}
			}
			else if(oppW)
			{
				if(oppE)
				{num += gen2PieceAdj(b,pla,kt W,kt E,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt W]);
				 num += gen2PieceAdj(b,pla,kt E,kt W,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt E]);}
				else if(oppN)
				{num += gen2PieceAdj(b,pla,kt W,kt N,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt W]);
				 num += gen2PieceAdj(b,pla,kt N,kt W,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt N]);}
			}
			else
			{num += gen2PieceAdj(b,pla,kt E,kt N,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt E]);
			 num += gen2PieceAdj(b,pla,kt N,kt E,kt,mv+num,hm+num,ISO(kt) ? b.pieces[kt] : b.pieces[kt N]);}
		}

		if(num > 0)
		  return num;
	}

	return num;
}

//BOARD UTILITY FUNCTIONS-------------------------------------------------------------

//Checks if a piece could push the enemy on to the trap, if the trap were empty, and any friendly pieces on uf1 or uf2 were unfrozen
static bool couldPushToTrap(Board& b, pla_t pla, loc_t eloc, loc_t kt, loc_t uf1, loc_t uf2)
{
	if(eloc S != kt && ISP(eloc S) && GT(eloc S, eloc) && (eloc S == uf1 || eloc S == uf2 || b.isThawedC(eloc S))) {return true;}
	if(eloc W != kt && ISP(eloc W) && GT(eloc W, eloc) && (eloc W == uf1 || eloc W == uf2 || b.isThawedC(eloc W))) {return true;}
	if(eloc E != kt && ISP(eloc E) && GT(eloc E, eloc) && (eloc E == uf1 || eloc E == uf2 || b.isThawedC(eloc E))) {return true;}
	if(eloc N != kt && ISP(eloc N) && GT(eloc N, eloc) && (eloc N == uf1 || eloc N == uf2 || b.isThawedC(eloc N))) {return true;}

	return false;
}

//Generate all steps of this piece into any open surrounding space
static bool canSteps1S(Board& b, pla_t pla, loc_t k)
{
	if(CS1(k) && ISE(k S) && b.isRabOkayS(pla,k)) {return true;}
	if(CW1(k) && ISE(k W))                        {return true;}
	if(CE1(k) && ISE(k E))                        {return true;}
	if(CN1(k) && ISE(k N) && b.isRabOkayN(pla,k)) {return true;}
	return false;
}

//Generate all steps of this piece into any open surrounding space
static int genSteps1S(Board& b, pla_t pla, loc_t k,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	if(CS1(k) && ISE(k S) && b.isRabOkayS(pla,k)) {ADDMOVEPP(Board::getMove(k MS),hmval)}
	if(CW1(k) && ISE(k W))                        {ADDMOVEPP(Board::getMove(k MW),hmval)}
	if(CE1(k) && ISE(k E))                        {ADDMOVEPP(Board::getMove(k ME),hmval)}
	if(CN1(k) && ISE(k N) && b.isRabOkayN(pla,k)) {ADDMOVEPP(Board::getMove(k MN),hmval)}
	return num;
}

//Can ploc push eloc anywhere?
//Assumes there is a ploc adjacent to eloc unfrozen and big enough.
//Does not push adjacent to to floc
static bool canPushE(Board& b, loc_t eloc, loc_t fadjloc)
{
	if(CS1(eloc) && ISE(eloc S) && !b.isAdjacent(eloc S,fadjloc)) {return true;}
	if(CW1(eloc) && ISE(eloc W) && !b.isAdjacent(eloc W,fadjloc)) {return true;}
	if(CE1(eloc) && ISE(eloc E) && !b.isAdjacent(eloc E,fadjloc)) {return true;}
	if(CN1(eloc) && ISE(eloc N) && !b.isAdjacent(eloc N,fadjloc)) {return true;}

	return false;
}

//Can ploc push eloc anywhere?
//Assumes ploc is unfrozen and big enough.
//Does not push on to floc
static int genPushPE(Board& b, loc_t ploc, loc_t eloc, loc_t fadjloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	int pstep = Board::STEPINDEX[ploc][eloc];

	if(CS1(eloc) && ISE(eloc S) && !b.isAdjacent(eloc S,fadjloc)) {ADDMOVEPP(Board::getMove(eloc MS,pstep),hmval)}
	if(CW1(eloc) && ISE(eloc W) && !b.isAdjacent(eloc W,fadjloc)) {ADDMOVEPP(Board::getMove(eloc MW,pstep),hmval)}
	if(CE1(eloc) && ISE(eloc E) && !b.isAdjacent(eloc E,fadjloc)) {ADDMOVEPP(Board::getMove(eloc ME,pstep),hmval)}
	if(CN1(eloc) && ISE(eloc N) && !b.isAdjacent(eloc N,fadjloc)) {ADDMOVEPP(Board::getMove(eloc MN,pstep),hmval)}

	return num;
}


//Can ploc pull eloc anywhere?
//Assumes ploc is unfrozen and big enough.
//The can counterpart to this is simply isOpen(ploc)
static int genPullPE(Board& b, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval)
{
	int num = 0;
	int estep = Board::STEPINDEX[eloc][ploc];

	if(CS1(ploc) && ISE(ploc S)) {ADDMOVEPP(Board::getMove(ploc MS,estep),hmval)}
	if(CW1(ploc) && ISE(ploc W)) {ADDMOVEPP(Board::getMove(ploc MW,estep),hmval)}
	if(CE1(ploc) && ISE(ploc E)) {ADDMOVEPP(Board::getMove(ploc ME,estep),hmval)}
	if(CN1(ploc) && ISE(ploc N)) {ADDMOVEPP(Board::getMove(ploc MN,estep),hmval)}

	return num;
}

//Can this piece be pushed by anthing?
static bool canPushc(Board& b, pla_t pla, loc_t eloc)
{
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CW1(eloc) && ISE(eloc W)) {return true;}
		if(CE1(eloc) && ISE(eloc E)) {return true;}
		if(CN1(eloc) && ISE(eloc N)) {return true;}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc S)) {return true;}
		if(CE1(eloc) && ISE(eloc E)) {return true;}
		if(CN1(eloc) && ISE(eloc N)) {return true;}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc S)) {return true;}
		if(CW1(eloc) && ISE(eloc W)) {return true;}
		if(CN1(eloc) && ISE(eloc N)) {return true;}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CS1(eloc) && ISE(eloc S)) {return true;}
		if(CW1(eloc) && ISE(eloc W)) {return true;}
		if(CE1(eloc) && ISE(eloc E)) {return true;}
	}
	return false;
}

//Can this piece be pushed by anthing?
static int genPush(Board& b, pla_t pla, loc_t eloc, move_t* mv, int* hm, int hmval)
{
	int num = 0;
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CW1(eloc) && ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW,eloc S MN),hmval)}
		if(CE1(eloc) && ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME,eloc S MN),hmval)}
		if(CN1(eloc) && ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN,eloc S MN),hmval)}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS,eloc W ME),hmval)}
		if(CE1(eloc) && ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME,eloc W ME),hmval)}
		if(CN1(eloc) && ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN,eloc W ME),hmval)}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS,eloc E MW),hmval)}
		if(CW1(eloc) && ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW,eloc E MW),hmval)}
		if(CN1(eloc) && ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN,eloc E MW),hmval)}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CS1(eloc) && ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS,eloc N MS),hmval)}
		if(CW1(eloc) && ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW,eloc N MS),hmval)}
		if(CE1(eloc) && ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME,eloc N MS),hmval)}
	}
	return num;
}

//Can this piece be pulled by anthing?
static bool canPullc(Board& b, pla_t pla, loc_t eloc)
{
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CS2(eloc) && ISE(eloc SS)) {return true;}
		if(CW1(eloc) && ISE(eloc SW)) {return true;}
		if(CE1(eloc) && ISE(eloc SE)) {return true;}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc SW)) {return true;}
		if(CW2(eloc) && ISE(eloc WW)) {return true;}
		if(CN1(eloc) && ISE(eloc NW)) {return true;}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc SE)) {return true;}
		if(CE2(eloc) && ISE(eloc EE)) {return true;}
		if(CN1(eloc) && ISE(eloc NE)) {return true;}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CW1(eloc) && ISE(eloc NW)) {return true;}
		if(CE1(eloc) && ISE(eloc NE)) {return true;}
		if(CN2(eloc) && ISE(eloc NN)) {return true;}
	}
	return false;
}

//Can this piece be pulled by anthing?
static int genPull(Board& b, pla_t pla, loc_t eloc, move_t* mv, int* hm, int hmval)
{
	int num = 0;
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CS2(eloc) && ISE(eloc SS)) {ADDMOVEPP(Board::getMove(eloc S MS,eloc MS),hmval)}
		if(CW1(eloc) && ISE(eloc SW)) {ADDMOVEPP(Board::getMove(eloc S MW,eloc MS),hmval)}
		if(CE1(eloc) && ISE(eloc SE)) {ADDMOVEPP(Board::getMove(eloc S ME,eloc MS),hmval)}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc SW)) {ADDMOVEPP(Board::getMove(eloc W MS,eloc MW),hmval)}
		if(CW2(eloc) && ISE(eloc WW)) {ADDMOVEPP(Board::getMove(eloc W MW,eloc MW),hmval)}
		if(CN1(eloc) && ISE(eloc NW)) {ADDMOVEPP(Board::getMove(eloc W MN,eloc MW),hmval)}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc SE)) {ADDMOVEPP(Board::getMove(eloc E MS,eloc ME),hmval)}
		if(CE2(eloc) && ISE(eloc EE)) {ADDMOVEPP(Board::getMove(eloc E ME,eloc ME),hmval)}
		if(CN1(eloc) && ISE(eloc NE)) {ADDMOVEPP(Board::getMove(eloc E MN,eloc ME),hmval)}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CW1(eloc) && ISE(eloc NW)) {ADDMOVEPP(Board::getMove(eloc N MW,eloc MN),hmval)}
		if(CE1(eloc) && ISE(eloc NE)) {ADDMOVEPP(Board::getMove(eloc N ME,eloc MN),hmval)}
		if(CN2(eloc) && ISE(eloc NN)) {ADDMOVEPP(Board::getMove(eloc N MN,eloc MN),hmval)}
	}
	return num;
}

//Assumes UF and big enough. Generates all pushes and pulls of eloc by ploc.
static bool canPPPE(Board& b, loc_t ploc, loc_t eloc)
{
	return b.isOpen(ploc) || b.isOpen(eloc);
}

//Does not check if the piece is already unfrozen
//Assumes a player piece at ploc
static bool canUF(Board& b, pla_t pla, loc_t ploc)
{
	if(CS1(ploc) && ISE(ploc S))
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawedC(ploc SS) && b.isRabOkayN(pla,ploc SS)) {return true;}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW)                             ) {return true;}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE)                             ) {return true;}
	}
	if(CW1(ploc) && ISE(ploc W))
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.isRabOkayN(pla,ploc SW)) {return true;}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawedC(ploc WW)                             ) {return true;}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.isRabOkayS(pla,ploc NW)) {return true;}
	}
	if(CE1(ploc) && ISE(ploc E))
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.isRabOkayN(pla,ploc SE)) {return true;}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawedC(ploc EE)                             ) {return true;}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.isRabOkayS(pla,ploc NE)) {return true;}
	}
	if(CN1(ploc) && ISE(ploc N))
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW)                             ) {return true;}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE)                             ) {return true;}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawedC(ploc NN) && b.isRabOkayS(pla,ploc NN)) {return true;}
	}
	return false;
}

//Does not check if the piece is already unfrozen
//Assumes a player piece at ploc
static int genUF(Board& b, pla_t pla, loc_t ploc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	if(CS1(ploc) && ISE(ploc S))
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawedC(ploc SS) && b.isRabOkayN(pla,ploc SS)) {ADDMOVEPP(Board::getMove(ploc SS MN),hmval)}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW)                             ) {ADDMOVEPP(Board::getMove(ploc SW ME),hmval)}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE)                             ) {ADDMOVEPP(Board::getMove(ploc SE MW),hmval)}
	}
	if(CW1(ploc) && ISE(ploc W))
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.isRabOkayN(pla,ploc SW)) {ADDMOVEPP(Board::getMove(ploc SW MN),hmval)}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawedC(ploc WW)                             ) {ADDMOVEPP(Board::getMove(ploc WW ME),hmval)}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.isRabOkayS(pla,ploc NW)) {ADDMOVEPP(Board::getMove(ploc NW MS),hmval)}
	}
	if(CE1(ploc) && ISE(ploc E))
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.isRabOkayN(pla,ploc SE)) {ADDMOVEPP(Board::getMove(ploc SE MN),hmval)}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawedC(ploc EE)                             ) {ADDMOVEPP(Board::getMove(ploc EE MW),hmval)}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.isRabOkayS(pla,ploc NE)) {ADDMOVEPP(Board::getMove(ploc NE MS),hmval)}
	}
	if(CN1(ploc) && ISE(ploc N))
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW)                             ) {ADDMOVEPP(Board::getMove(ploc NW ME),hmval)}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE)                             ) {ADDMOVEPP(Board::getMove(ploc NE MW),hmval)}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawedC(ploc NN) && b.isRabOkayS(pla,ploc NN)) {ADDMOVEPP(Board::getMove(ploc NN MS),hmval)}
	}
	return num;
}

static bool canUFAt(Board& b, pla_t pla, loc_t loc)
{
	if(CS1(loc) && ISP(loc S) && b.isThawedC(loc S) && b.isRabOkayN(pla,loc S)) {return true;}
	if(CW1(loc) && ISP(loc W) && b.isThawedC(loc W)                           ) {return true;}
	if(CE1(loc) && ISP(loc E) && b.isThawedC(loc E)                           ) {return true;}
	if(CN1(loc) && ISP(loc N) && b.isThawedC(loc N) && b.isRabOkayS(pla,loc N)) {return true;}
	return false;
}

static int genUFAt(Board& b, pla_t pla, loc_t loc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	if(CS1(loc) && ISP(loc S) && b.isThawedC(loc S) && b.isRabOkayN(pla,loc S)) {ADDMOVEPP(Board::getMove(loc S MN),hmval)}
	if(CW1(loc) && ISP(loc W) && b.isThawedC(loc W)                           ) {ADDMOVEPP(Board::getMove(loc W ME),hmval)}
	if(CE1(loc) && ISP(loc E) && b.isThawedC(loc E)                           ) {ADDMOVEPP(Board::getMove(loc E MW),hmval)}
	if(CN1(loc) && ISP(loc N) && b.isThawedC(loc N) && b.isRabOkayS(pla,loc N)) {ADDMOVEPP(Board::getMove(loc N MS),hmval)}
	return num;
}

//Can unfreeze and then pushpull a piece?
static bool canUFPPPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc)
{
	bool suc = false;
	if(CS1(ploc) && ISE(ploc S))
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawedC(ploc SS) && b.isRabOkayN(pla,ploc SS)) {TS(ploc SS,ploc S,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW)                             ) {TS(ploc SW,ploc S,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE)                             ) {TS(ploc SE,ploc S,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
	}
	if(CW1(ploc) && ISE(ploc W))
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.isRabOkayN(pla,ploc SW)) {TS(ploc SW,ploc W,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawedC(ploc WW)                              ) {TS(ploc WW,ploc W,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.isRabOkayS(pla,ploc NW)) {TS(ploc NW,ploc W,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
	}
	if(CE1(ploc) && ISE(ploc E))
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.isRabOkayN(pla,ploc SE)) {TS(ploc SE,ploc E,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawedC(ploc EE)                             ) {TS(ploc EE,ploc E,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.isRabOkayS(pla,ploc NE)) {TS(ploc NE,ploc E,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
	}
	if(CN1(ploc) && ISE(ploc N))
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW)                             ) {TS(ploc NW,ploc N,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE)                             ) {TS(ploc NE,ploc N,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawedC(ploc NN) && b.isRabOkayS(pla,ploc NN)) {TS(ploc NN,ploc N,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
	}
	return false;
}

//Can unfreeze and then pushpull a piece?
static int genUFPPPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	bool suc = false;
	int num = 0;
	if(CS1(ploc) && ISE(ploc S))
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawedC(ploc SS) && b.isRabOkayN(pla,ploc SS)) {TS(ploc SS,ploc S,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SS MN),hmval)}}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW)                             ) {TS(ploc SW,ploc S,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SW ME),hmval)}}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE)                             ) {TS(ploc SE,ploc S,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SE MW),hmval)}}
	}
	if(CW1(ploc) && ISE(ploc W))
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.isRabOkayN(pla,ploc SW)) {TS(ploc SW,ploc W,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SW MN),hmval)}}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawedC(ploc WW)                             ) {TS(ploc WW,ploc W,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc WW ME),hmval)}}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.isRabOkayS(pla,ploc NW)) {TS(ploc NW,ploc W,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NW MS),hmval)}}
	}
	if(CE1(ploc) && ISE(ploc E))
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.isRabOkayN(pla,ploc SE)) {TS(ploc SE,ploc E,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SE MN),hmval)}}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawedC(ploc EE)                             ) {TS(ploc EE,ploc E,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc EE MW),hmval)}}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.isRabOkayS(pla,ploc NE)) {TS(ploc NE,ploc E,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NE MS),hmval)}}
	}
	if(CN1(ploc) && ISE(ploc N))
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW)                             ) {TS(ploc NW,ploc N,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NW ME),hmval)}}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE)                             ) {TS(ploc NE,ploc N,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NE MW),hmval)}}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawedC(ploc NN) && b.isRabOkayS(pla,ploc NN)) {TS(ploc NN,ploc N,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NN MS),hmval)}}
	}
	return num;
}

static bool canUF2(Board& b, pla_t pla, loc_t ploc)
{
	pla_t opp = OPP(pla);
	//Safe to skip owners checks since all adjacent pieces are either 0 or controlled by enemy.
	int numStronger =
	(CS1(ploc) && GT(ploc S,ploc)) +
	(CW1(ploc) && GT(ploc W,ploc)) +
	(CE1(ploc) && GT(ploc E,ploc)) +
	(CN1(ploc) && GT(ploc N,ploc));

	if(numStronger == 1)
	{
		loc_t eloc;
		     if(CS1(ploc) && GT(ploc S,ploc)) {eloc = ploc S;}
		else if(CW1(ploc) && GT(ploc W,ploc)) {eloc = ploc W;}
		else if(CE1(ploc) && GT(ploc E,ploc)) {eloc = ploc E;}
		else                                  {eloc = ploc N;}

		if(canPullc(b,pla,eloc))
		{return true;}

		if(!b.isTrapSafe2(opp,eloc))
		{
			if     (ISO(eloc S)) {RIF(canPPCapAndUFc(b,pla,eloc S,ploc))}
			else if(ISO(eloc W)) {RIF(canPPCapAndUFc(b,pla,eloc W,ploc))}
			else if(ISO(eloc E)) {RIF(canPPCapAndUFc(b,pla,eloc E,ploc))}
			else if(ISO(eloc N)) {RIF(canPPCapAndUFc(b,pla,eloc N,ploc))}
		}
	}

	if(CS1(ploc) && canGetPlaTo2S(b,pla,ploc S,ploc)) {return true;}
	if(CW1(ploc) && canGetPlaTo2S(b,pla,ploc W,ploc)) {return true;}
	if(CE1(ploc) && canGetPlaTo2S(b,pla,ploc E,ploc)) {return true;}
	if(CN1(ploc) && canGetPlaTo2S(b,pla,ploc N,ploc)) {return true;}

	return false;
}

static int genUF2(Board& b, pla_t pla, loc_t ploc, move_t* mv, int* hm, int hmval)
{
	int num = 0;
	pla_t opp = OPP(pla);
	//Safe to skip owners checks since all adjacent pieces are either 0 or controlled by enemy.
	int numStronger =
	(CS1(ploc) && GT(ploc S,ploc)) +
	(CW1(ploc) && GT(ploc W,ploc)) +
	(CE1(ploc) && GT(ploc E,ploc)) +
	(CN1(ploc) && GT(ploc N,ploc));

	//Try to pull the freezing piece away
	if(numStronger == 1)
	{
		loc_t eloc;
		     if(CS1(ploc) && GT(ploc S,ploc)) {eloc = ploc S;}
		else if(CW1(ploc) && GT(ploc W,ploc)) {eloc = ploc W;}
		else if(CE1(ploc) && GT(ploc E,ploc)) {eloc = ploc E;}
		else                                  {eloc = ploc N;}

		num += genPull(b,pla,eloc,mv+num,hm+num,hmval);

		if(!b.isTrapSafe2(opp,eloc))
		{
			if     (ISO(eloc S)) {num += genPPCapAndUF(b,pla,eloc S,ploc,mv+num,hm+num,hmval);}
			else if(ISO(eloc W)) {num += genPPCapAndUF(b,pla,eloc W,ploc,mv+num,hm+num,hmval);}
			else if(ISO(eloc E)) {num += genPPCapAndUF(b,pla,eloc E,ploc,mv+num,hm+num,hmval);}
			else if(ISO(eloc N)) {num += genPPCapAndUF(b,pla,eloc N,ploc,mv+num,hm+num,hmval);}
		}
	}

	//Empty squares : move in a piece, which might be 2 steps away or frozen and one step away
	//Opponent squares, push.
	if(CS1(ploc)) {num += genGetPlaTo2S(b,pla,ploc S,ploc,mv+num,hm+num,hmval);}
	if(CW1(ploc)) {num += genGetPlaTo2S(b,pla,ploc W,ploc,mv+num,hm+num,hmval);}
	if(CE1(ploc)) {num += genGetPlaTo2S(b,pla,ploc E,ploc,mv+num,hm+num,hmval);}
	if(CN1(ploc)) {num += genGetPlaTo2S(b,pla,ploc N,ploc,mv+num,hm+num,hmval);}

	return num;
}

static bool canPPCapAndUFc(Board& b, pla_t pla, loc_t eloc, loc_t ploc)
{
	bool suc = false;
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc S,eloc,eloc W,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc S,eloc,eloc E,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc S,eloc,eloc N,suc = b.isThawedC(ploc)) RIF(suc)}

		if(CS2(eloc) && ISE(eloc SS)) {TPC(eloc,eloc S,eloc SS,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CW1(eloc) && ISE(eloc SW)) {TPC(eloc,eloc S,eloc SW,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CE1(eloc) && ISE(eloc SE)) {TPC(eloc,eloc S,eloc SE,suc = b.isThawedC(ploc)) RIF(suc)}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc W,eloc,eloc S,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc W,eloc,eloc E,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc W,eloc,eloc N,suc = b.isThawedC(ploc)) RIF(suc)}

		if(CS1(eloc) && ISE(eloc SW)) {TPC(eloc,eloc W,eloc SW,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CW2(eloc) && ISE(eloc WW)) {TPC(eloc,eloc W,eloc WW,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CN1(eloc) && ISE(eloc NW)) {TPC(eloc,eloc W,eloc NW,suc = b.isThawedC(ploc)) RIF(suc)}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc E,eloc,eloc S,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc E,eloc,eloc W,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc E,eloc,eloc N,suc = b.isThawedC(ploc)) RIF(suc)}

		if(CS1(eloc) && ISE(eloc SE)) {TPC(eloc,eloc E,eloc SE,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CE2(eloc) && ISE(eloc EE)) {TPC(eloc,eloc E,eloc EE,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CN1(eloc) && ISE(eloc NE)) {TPC(eloc,eloc E,eloc NE,suc = b.isThawedC(ploc)) RIF(suc)}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc N,eloc,eloc S,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc N,eloc,eloc W,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc N,eloc,eloc E,suc = b.isThawedC(ploc)) RIF(suc)}

		if(CW1(eloc) && ISE(eloc NW)) {TPC(eloc,eloc N,eloc NW,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CE1(eloc) && ISE(eloc NE)) {TPC(eloc,eloc N,eloc NE,suc = b.isThawedC(ploc)) RIF(suc)}
		if(CN2(eloc) && ISE(eloc NN)) {TPC(eloc,eloc N,eloc NN,suc = b.isThawedC(ploc)) RIF(suc)}
	}
	return false;
}


static int genPPCapAndUF(Board& b, pla_t pla, loc_t eloc, loc_t ploc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	bool suc = false;
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc S,eloc,eloc W,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MW,eloc S MN),hmval)}}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc S,eloc,eloc E,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc ME,eloc S MN),hmval)}}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc S,eloc,eloc N,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MN,eloc S MN),hmval)}}

		if(CS2(eloc) && ISE(eloc SS)) {TPC(eloc,eloc S,eloc SS,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc S MS,eloc MS),hmval)}}
		if(CW1(eloc) && ISE(eloc SW)) {TPC(eloc,eloc S,eloc SW,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc S MW,eloc MS),hmval)}}
		if(CE1(eloc) && ISE(eloc SE)) {TPC(eloc,eloc S,eloc SE,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc S ME,eloc MS),hmval)}}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc W,eloc,eloc S,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MS,eloc W ME),hmval)}}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc W,eloc,eloc E,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc ME,eloc W ME),hmval)}}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc W,eloc,eloc N,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MN,eloc W ME),hmval)}}

		if(CS1(eloc) && ISE(eloc SW)) {TPC(eloc,eloc W,eloc SW,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc W MS,eloc MW),hmval)}}
		if(CW2(eloc) && ISE(eloc WW)) {TPC(eloc,eloc W,eloc WW,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc W MW,eloc MW),hmval)}}
		if(CN1(eloc) && ISE(eloc NW)) {TPC(eloc,eloc W,eloc NW,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc W MN,eloc MW),hmval)}}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc E,eloc,eloc S,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MS,eloc E MW),hmval)}}
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc E,eloc,eloc W,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MW,eloc E MW),hmval)}}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc E,eloc,eloc N,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MN,eloc E MW),hmval)}}

		if(CS1(eloc) && ISE(eloc SE)) {TPC(eloc,eloc E,eloc SE,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc E MS,eloc ME),hmval)}}
		if(CE2(eloc) && ISE(eloc EE)) {TPC(eloc,eloc E,eloc EE,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc E ME,eloc ME),hmval)}}
		if(CN1(eloc) && ISE(eloc NE)) {TPC(eloc,eloc E,eloc NE,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc E MN,eloc ME),hmval)}}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc N,eloc,eloc S,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MS,eloc N MS),hmval)}}
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc N,eloc,eloc W,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc MW,eloc N MS),hmval)}}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc N,eloc,eloc E,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc ME,eloc N MS),hmval)}}

		if(CW1(eloc) && ISE(eloc NW)) {TPC(eloc,eloc N,eloc NW,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc N MW,eloc MN),hmval)}}
		if(CE1(eloc) && ISE(eloc NE)) {TPC(eloc,eloc N,eloc NE,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc N ME,eloc MN),hmval)}}
		if(CN2(eloc) && ISE(eloc NN)) {TPC(eloc,eloc N,eloc NN,suc = b.isThawedC(ploc)) if(suc) {ADDMOVEPP(Board::getMove(eloc N MN,eloc MN),hmval)}}
	}

	return num;
}

static bool canGetPlaTo2S(Board& b, pla_t pla, loc_t k, loc_t floc)
{
	pla_t opp = OPP(pla);
	if(ISE(k))
	{
		if     (CS1(k) && k S != floc && ISP(k S) && b.isRabOkayN(pla,k S))  {if(canUF(b,pla,k S)) {return true;}}
		else if(CS1(k) && k S != floc && ISE(k S) && b.isTrapSafe2(pla,k S)) {if(canStepStepc(b,pla,k S,k)) {return true;}}

		if     (CW1(k) && k W != floc && ISP(k W))                           {if(canUF(b,pla,k W)) {return true;}}
		else if(CW1(k) && k W != floc && ISE(k W) && b.isTrapSafe2(pla,k W)) {if(canStepStepc(b,pla,k W,k)) {return true;}}

		if     (CE1(k) && k E != floc && ISP(k E))                           {if(canUF(b,pla,k E)) {return true;}}
		else if(CE1(k) && k E != floc && ISE(k E) && b.isTrapSafe2(pla,k E)) {if(canStepStepc(b,pla,k E,k)) {return true;}}

		if     (CN1(k) && k N != floc && ISP(k N) && b.isRabOkayS(pla,k N))  {if(canUF(b,pla,k N)) {return true;}}
		else if(CN1(k) && k N != floc && ISE(k N) && b.isTrapSafe2(pla,k N)) {if(canStepStepc(b,pla,k N,k)) {return true;}}
	}
	else if(ISO(k)) {if(canPushc(b,pla,k)) {return true;}}

	return false;
}

static int genGetPlaTo2S(Board& b, pla_t pla, loc_t k, loc_t floc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	pla_t opp = OPP(pla);
	if(ISE(k))
	{
		if     (CS1(k) && k S != floc && ISP(k S) && b.isRabOkayN(pla,k S))  {num += genUF(b,pla,k S,mv+num,hm+num,hmval);}
		else if(CS1(k) && k S != floc && ISE(k S) && b.isTrapSafe2(pla,k S)) {num += genStepStep(b,pla,k S,k,mv+num,hm+num,hmval);}

		if     (CW1(k) && k W != floc && ISP(k W))                           {num += genUF(b,pla,k W,mv+num,hm+num,hmval);}
		else if(CW1(k) && k W != floc && ISE(k W) && b.isTrapSafe2(pla,k W)) {num += genStepStep(b,pla,k W,k,mv+num,hm+num,hmval);}

		if     (CE1(k) && k E != floc && ISP(k E))                           {num += genUF(b,pla,k E,mv+num,hm+num,hmval);}
		else if(CE1(k) && k E != floc && ISE(k E) && b.isTrapSafe2(pla,k E)) {num += genStepStep(b,pla,k E,k,mv+num,hm+num,hmval);}

		if     (CN1(k) && k N != floc && ISP(k N) && b.isRabOkayS(pla,k N))  {num += genUF(b,pla,k N,mv+num,hm+num,hmval);}
		else if(CN1(k) && k N != floc && ISE(k N) && b.isTrapSafe2(pla,k N)) {num += genStepStep(b,pla,k N,k,mv+num,hm+num,hmval);}
	}
	else if(ISO(k)) {num += genPush(b,pla,k,mv+num,hm+num,hmval);}

	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canUFUFPPPE(Board& b, pla_t pla, loc_t ploc, loc_t plocdest, loc_t ploc2, loc_t eloc)
{
	bool suc = false;

	//This includes sacrifical unfreezing (ending on a trap square, so that when the unfrozen piece moves,
	//the unfreezing piece dies). For move ordering heuristics, add a check!!!
	if(CS1(ploc) && ISE(ploc S) && ploc S != plocdest)
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawed(ploc SS) && b.isRabOkayN(pla,ploc SS)) {TS(ploc SS,ploc S,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW)                             ) {TS(ploc SW,ploc S,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE)                             ) {TS(ploc SE,ploc S,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
	}
	if(CW1(ploc) && ISE(ploc W) && ploc W != plocdest)
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW) && b.isRabOkayN(pla,ploc SW)) {TS(ploc SW,ploc W,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawed(ploc WW)                             ) {TS(ploc WW,ploc W,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW) && b.isRabOkayS(pla,ploc NW)) {TS(ploc NW,ploc W,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
	}
	if(CE1(ploc) && ISE(ploc E) && ploc E != plocdest)
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE) && b.isRabOkayN(pla,ploc SE)) {TS(ploc SE,ploc E,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawed(ploc EE)                             ) {TS(ploc EE,ploc E,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE) && b.isRabOkayS(pla,ploc NE)) {TS(ploc NE,ploc E,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
	}
	if(CN1(ploc) && ISE(ploc N) && ploc N != plocdest)
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW)                             ) {TS(ploc NW,ploc N,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE)                             ) {TS(ploc NE,ploc N,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawed(ploc NN) && b.isRabOkayS(pla,ploc NN)) {TS(ploc NN,ploc N,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {return true;}}
	}
	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genUFUFPPPE(Board& b, pla_t pla, loc_t ploc, loc_t plocdest, loc_t ploc2, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	bool suc = false;

	//This includes sacrifical unfreezing (ending on a trap square, so that when the unfrozen piece moves,
	//the unfreezing piece dies). For move ordering heuristics, add a check!!!
	if(CS1(ploc) && ISE(ploc S) && ploc S != plocdest)
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawed(ploc SS) && b.isRabOkayN(pla,ploc SS)) {TS(ploc SS,ploc S,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc SS MN),hmval)}}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW)                             ) {TS(ploc SW,ploc S,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc SW ME),hmval)}}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE)                             ) {TS(ploc SE,ploc S,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc SE MW),hmval)}}
	}
	if(CW1(ploc) && ISE(ploc W) && ploc W != plocdest)
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW) && b.isRabOkayN(pla,ploc SW)) {TS(ploc SW,ploc W,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc SW MN),hmval)}}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawed(ploc WW)                             ) {TS(ploc WW,ploc W,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc WW ME),hmval)}}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW) && b.isRabOkayS(pla,ploc NW)) {TS(ploc NW,ploc W,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc NW MS),hmval)}}
	}
	if(CE1(ploc) && ISE(ploc E) && ploc E != plocdest)
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE) && b.isRabOkayN(pla,ploc SE)) {TS(ploc SE,ploc E,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc SE MN),hmval)}}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawed(ploc EE)                             ) {TS(ploc EE,ploc E,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc EE MW),hmval)}}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE) && b.isRabOkayS(pla,ploc NE)) {TS(ploc NE,ploc E,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc NE MS),hmval)}}
	}
	if(CN1(ploc) && ISE(ploc N) && ploc N != plocdest)
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW)                             ) {TS(ploc NW,ploc N,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc NW ME),hmval)}}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE)                             ) {TS(ploc NE,ploc N,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc NE MW),hmval)}}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawed(ploc NN) && b.isRabOkayS(pla,ploc NN)) {TS(ploc NN,ploc N,TS(ploc,plocdest,suc = canPPPE(b,ploc2,eloc))) if(suc) {ADDMOVEPP(Board::getMove(ploc NN MS),hmval)}}
	}

	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canStepStepPPPE(Board& b, pla_t pla, loc_t dest, loc_t dest2, loc_t ploc, loc_t eloc)
{
	bool suc = false;

	if((dest N == dest2 && pla == 0) || (dest S == dest2 && pla == 1))
	{
		if(CS1(dest) && ISP(dest S) && b.isThawed(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.pieces[dest S] != RAB) {TS(dest S,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {return true;}}
		if(CW1(dest) && ISP(dest W) && b.isThawed(dest W) && b.wouldBeUF(pla,dest W,dest,dest W) && b.pieces[dest W] != RAB) {TS(dest W,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {return true;}}
		if(CE1(dest) && ISP(dest E) && b.isThawed(dest E) && b.wouldBeUF(pla,dest E,dest,dest E) && b.pieces[dest E] != RAB) {TS(dest E,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {return true;}}
		if(CN1(dest) && ISP(dest N) && b.isThawed(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.pieces[dest N] != RAB) {TS(dest N,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {return true;}}
	}
	else
	{
		if(CS1(dest) && ISP(dest S) && b.isThawed(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.isRabOkayN(pla,dest S)) {TS(dest S,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {return true;}}
		if(CW1(dest) && ISP(dest W) && b.isThawed(dest W) && b.wouldBeUF(pla,dest W,dest,dest W))                             {TS(dest W,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {return true;}}
		if(CE1(dest) && ISP(dest E) && b.isThawed(dest E) && b.wouldBeUF(pla,dest E,dest,dest E))                             {TS(dest E,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {return true;}}
		if(CN1(dest) && ISP(dest N) && b.isThawed(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.isRabOkayS(pla,dest N)) {TS(dest N,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {return true;}}
	}
	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genStepStepPPPE(Board& b, pla_t pla, loc_t dest, loc_t dest2, loc_t ploc, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	bool suc = false;

	if((dest N == dest2 && pla == 0) || (dest S == dest2 && pla == 1))
	{
		if(CS1(dest) && ISP(dest S) && b.isThawed(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.pieces[dest S] != RAB) {TS(dest S,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {ADDMOVEPP(Board::getMove(dest S MN),hmval)}}
		if(CW1(dest) && ISP(dest W) && b.isThawed(dest W) && b.wouldBeUF(pla,dest W,dest,dest W) && b.pieces[dest W] != RAB) {TS(dest W,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {ADDMOVEPP(Board::getMove(dest W ME),hmval)}}
		if(CE1(dest) && ISP(dest E) && b.isThawed(dest E) && b.wouldBeUF(pla,dest E,dest,dest E) && b.pieces[dest E] != RAB) {TS(dest E,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {ADDMOVEPP(Board::getMove(dest E MW),hmval)}}
		if(CN1(dest) && ISP(dest N) && b.isThawed(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.pieces[dest N] != RAB) {TS(dest N,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {ADDMOVEPP(Board::getMove(dest N MS),hmval)}}
	}
	else
	{
		if(CS1(dest) && ISP(dest S) && b.isThawed(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.isRabOkayN(pla,dest S)) {TS(dest S,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {ADDMOVEPP(Board::getMove(dest S MN),hmval)}}
		if(CW1(dest) && ISP(dest W) && b.isThawed(dest W) && b.wouldBeUF(pla,dest W,dest,dest W))                             {TS(dest W,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {ADDMOVEPP(Board::getMove(dest W ME),hmval)}}
		if(CE1(dest) && ISP(dest E) && b.isThawed(dest E) && b.wouldBeUF(pla,dest E,dest,dest E))                             {TS(dest E,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {ADDMOVEPP(Board::getMove(dest E MW),hmval)}}
		if(CN1(dest) && ISP(dest N) && b.isThawed(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.isRabOkayS(pla,dest N)) {TS(dest N,dest, TS(dest,dest2,suc = canPPPE(b,ploc,eloc))) if(suc) {ADDMOVEPP(Board::getMove(dest N MS),hmval)}}
	}

	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canPushPPPE(Board& b, pla_t pla, loc_t floc, loc_t ploc, loc_t eloc)
{
	bool suc = false;

	if(CS1(floc) && ISP(floc S) && GT(floc S,floc) && b.isThawed(floc S))
	{
		if(CW1(floc) && ISE(floc W)) {TP(floc S,floc,floc W,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CE1(floc) && ISE(floc E)) {TP(floc S,floc,floc E,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CN1(floc) && ISE(floc N)) {TP(floc S,floc,floc N,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
	}
	if(CW1(floc) && ISP(floc W) && GT(floc W,floc) && b.isThawed(floc W))
	{
		if(CS1(floc) && ISE(floc S)) {TP(floc W,floc,floc S,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CE1(floc) && ISE(floc E)) {TP(floc W,floc,floc E,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CN1(floc) && ISE(floc N)) {TP(floc W,floc,floc N,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
	}
	if(CE1(floc) && ISP(floc E) && GT(floc E,floc) && b.isThawed(floc E))
	{
		if(CS1(floc) && ISE(floc S)) {TP(floc E,floc,floc S,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CW1(floc) && ISE(floc W)) {TP(floc E,floc,floc W,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CN1(floc) && ISE(floc N)) {TP(floc E,floc,floc N,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
	}
	if(CN1(floc) && ISP(floc N) && GT(floc N,floc) && b.isThawed(floc N))
	{
		if(CS1(floc) && ISE(floc S)) {TP(floc N,floc,floc S,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CW1(floc) && ISE(floc W)) {TP(floc N,floc,floc W,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
		if(CE1(floc) && ISE(floc E)) {TP(floc N,floc,floc E,suc = canPPPE(b,ploc,eloc)) if(suc) {return true;}}
	}
	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genPushPPPE(Board& b, pla_t pla, loc_t floc, loc_t ploc, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	bool suc = false;

	if(CS1(floc) && ISP(floc S) && GT(floc S,floc) && b.isThawed(floc S))
	{
		if(CW1(floc) && ISE(floc W)) {TP(floc S,floc,floc W,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MW,floc S MN),hmval)}}
		if(CE1(floc) && ISE(floc E)) {TP(floc S,floc,floc E,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc ME,floc S MN),hmval)}}
		if(CN1(floc) && ISE(floc N)) {TP(floc S,floc,floc N,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MN,floc S MN),hmval)}}
	}
	if(CW1(floc) && ISP(floc W) && GT(floc W,floc) && b.isThawed(floc W))
	{
		if(CS1(floc) && ISE(floc S)) {TP(floc W,floc,floc S,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MS,floc W ME),hmval)}}
		if(CE1(floc) && ISE(floc E)) {TP(floc W,floc,floc E,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc ME,floc W ME),hmval)}}
		if(CN1(floc) && ISE(floc N)) {TP(floc W,floc,floc N,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MN,floc W ME),hmval)}}
	}
	if(CE1(floc) && ISP(floc E) && GT(floc E,floc) && b.isThawed(floc E))
	{
		if(CS1(floc) && ISE(floc S)) {TP(floc E,floc,floc S,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MS,floc E MW),hmval)}}
		if(CW1(floc) && ISE(floc W)) {TP(floc E,floc,floc W,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MW,floc E MW),hmval)}}
		if(CN1(floc) && ISE(floc N)) {TP(floc E,floc,floc N,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MN,floc E MW),hmval)}}
	}
	if(CN1(floc) && ISP(floc N) && GT(floc N,floc) && b.isThawed(floc N))
	{
		if(CS1(floc) && ISE(floc S)) {TP(floc N,floc,floc S,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MS,floc N MS),hmval)}}
		if(CW1(floc) && ISE(floc W)) {TP(floc N,floc,floc W,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc MW,floc N MS),hmval)}}
		if(CE1(floc) && ISE(floc E)) {TP(floc N,floc,floc E,suc = canPPPE(b,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(floc ME,floc N MS),hmval)}}
	}
	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canUF2PPPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc)
{
	pla_t opp = OPP(pla);
	//Safe to skip owners checks since all adjacent pieces are either 0 or controlled by enemy.
	int numStronger =
	(CS1(ploc) && GT(ploc S,ploc)) +
	(CW1(ploc) && GT(ploc W,ploc)) +
	(CE1(ploc) && GT(ploc E,ploc)) +
	(CN1(ploc) && GT(ploc N,ploc));

	//Try to pull the freezing piece away
	if(numStronger == 1)
	{
		loc_t floc;
		     if(CS1(ploc) && GT(ploc S,ploc)) {floc = ploc S;}
		else if(CW1(ploc) && GT(ploc W,ploc)) {floc = ploc W;}
		else if(CE1(ploc) && GT(ploc E,ploc)) {floc = ploc E;}
		else                                  {floc = ploc N;}

		if(canPullc(b,pla,floc))
		{return true;}

		if(!b.isTrapSafe2(opp,floc))
		{
			if     (ISO(floc S)) {RIF(canPPCapAndUFc(b,pla,floc S,ploc))}
			else if(ISO(floc W)) {RIF(canPPCapAndUFc(b,pla,floc W,ploc))}
			else if(ISO(floc E)) {RIF(canPPCapAndUFc(b,pla,floc E,ploc))}
			else if(ISO(floc N)) {RIF(canPPCapAndUFc(b,pla,floc N,ploc))}
		}
	}

	//Empty squares : move in a piece, which might be 2 steps away or frozen and one step away
	//Opponent squares, push.
	if(CS1(ploc) && canGetPlaTo2SPPPE(b,pla,ploc S,ploc,eloc)) {return true;}
	if(CW1(ploc) && canGetPlaTo2SPPPE(b,pla,ploc W,ploc,eloc)) {return true;}
	if(CE1(ploc) && canGetPlaTo2SPPPE(b,pla,ploc E,ploc,eloc)) {return true;}
	if(CN1(ploc) && canGetPlaTo2SPPPE(b,pla,ploc N,ploc,eloc)) {return true;}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genUF2PPPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval)
{
	int num = 0;
	pla_t opp = OPP(pla);

	//Safe to skip owners checks since all adjacent pieces are either 0 or controlled by enemy.
	int numStronger =
	(CS1(ploc) && GT(ploc S,ploc)) +
	(CW1(ploc) && GT(ploc W,ploc)) +
	(CE1(ploc) && GT(ploc E,ploc)) +
	(CN1(ploc) && GT(ploc N,ploc));

	//Try to pull the freezing piece away
	if(numStronger == 1)
	{
		loc_t floc;
		     if(CS1(ploc) && GT(ploc S,ploc)) {floc = ploc S;}
		else if(CW1(ploc) && GT(ploc W,ploc)) {floc = ploc W;}
		else if(CE1(ploc) && GT(ploc E,ploc)) {floc = ploc E;}
		else                                  {floc = ploc N;}

		num += genPull(b,pla,floc,mv+num,hm+num,hmval);

		if(!b.isTrapSafe2(opp,floc))
		{
			if     (ISO(floc S)) {num += genPPCapAndUF(b,pla,floc S,ploc,mv+num,hm+num,hmval);}
			else if(ISO(floc W)) {num += genPPCapAndUF(b,pla,floc W,ploc,mv+num,hm+num,hmval);}
			else if(ISO(floc E)) {num += genPPCapAndUF(b,pla,floc E,ploc,mv+num,hm+num,hmval);}
			else if(ISO(floc N)) {num += genPPCapAndUF(b,pla,floc N,ploc,mv+num,hm+num,hmval);}
		}
	}

	//Empty squares : move in a piece, which might be 2 steps away or frozen and one step away
	//Opponent squares, push.
	if(CS1(ploc)) {num += genGetPlaTo2SPPPE(b,pla,ploc S,ploc,eloc,mv+num,hm+num,hmval);}
	if(CW1(ploc)) {num += genGetPlaTo2SPPPE(b,pla,ploc W,ploc,eloc,mv+num,hm+num,hmval);}
	if(CE1(ploc)) {num += genGetPlaTo2SPPPE(b,pla,ploc E,ploc,eloc,mv+num,hm+num,hmval);}
	if(CN1(ploc)) {num += genGetPlaTo2SPPPE(b,pla,ploc N,ploc,eloc,mv+num,hm+num,hmval);}

	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canGetPlaTo2SPPPE(Board& b, pla_t pla, loc_t k, loc_t ploc, loc_t eloc)
{
	pla_t opp = OPP(pla);
	if(ISE(k))
	{
		if     (CS1(k) && k S != ploc && ISP(k S) && b.isRabOkayN(pla,k S))  {if(canUFUFPPPE(b,pla,k S,k,ploc,eloc)) {return true;}}
		else if(CS1(k) && k S != ploc && ISE(k S) && b.isTrapSafe2(pla,k S)) {if(canStepStepPPPE(b,pla,k S,k,ploc,eloc)) {return true;}}

		if     (CW1(k) && k W != ploc && ISP(k W))                           {if(canUFUFPPPE(b,pla,k W,k,ploc,eloc)) {return true;}}
		else if(CW1(k) && k W != ploc && ISE(k W) && b.isTrapSafe2(pla,k W)) {if(canStepStepPPPE(b,pla,k W,k,ploc,eloc)) {return true;}}

		if     (CE1(k) && k E != ploc && ISP(k E))                           {if(canUFUFPPPE(b,pla,k E,k,ploc,eloc)) {return true;}}
		else if(CE1(k) && k E != ploc && ISE(k E) && b.isTrapSafe2(pla,k E)) {if(canStepStepPPPE(b,pla,k E,k,ploc,eloc)) {return true;}}

		if     (CN1(k) && k N != ploc && ISP(k N) && b.isRabOkayS(pla,k N))  {if(canUFUFPPPE(b,pla,k N,k,ploc,eloc)) {return true;}}
		else if(CN1(k) && k N != ploc && ISE(k N) && b.isTrapSafe2(pla,k N)) {if(canStepStepPPPE(b,pla,k N,k,ploc,eloc)) {return true;}}
	}
	else if(ISO(k)) {if(canPushPPPE(b,pla,k,ploc,eloc)) {return true;}}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genGetPlaTo2SPPPE(Board& b, pla_t pla, loc_t k, loc_t ploc, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	pla_t opp = OPP(pla);
	if(ISE(k))
	{
		if     (CS1(k) && k S != ploc && ISP(k S) && b.isRabOkayN(pla,k S))  {num += genUFUFPPPE(b,pla,k S,k,ploc,eloc,mv+num,hm+num,hmval);}
		else if(CS1(k) && k S != ploc && ISE(k S) && b.isTrapSafe2(pla,k S)) {num += genStepStepPPPE(b,pla,k S,k,ploc,eloc,mv+num,hm+num,hmval);}

		if     (CW1(k) && k W != ploc && ISP(k W))                           {num += genUFUFPPPE(b,pla,k W,k,ploc,eloc,mv+num,hm+num,hmval);}
		else if(CW1(k) && k W != ploc && ISE(k W) && b.isTrapSafe2(pla,k W)) {num += genStepStepPPPE(b,pla,k W,k,ploc,eloc,mv+num,hm+num,hmval);}

		if     (CE1(k) && k E != ploc && ISP(k E))                           {num += genUFUFPPPE(b,pla,k E,k,ploc,eloc,mv+num,hm+num,hmval);}
		else if(CE1(k) && k E != ploc && ISE(k E) && b.isTrapSafe2(pla,k E)) {num += genStepStepPPPE(b,pla,k E,k,ploc,eloc,mv+num,hm+num,hmval);}

		if     (CN1(k) && k N != ploc && ISP(k N) && b.isRabOkayS(pla,k N))  {num += genUFUFPPPE(b,pla,k N,k,ploc,eloc,mv+num,hm+num,hmval);}
		else if(CN1(k) && k N != ploc && ISE(k N) && b.isTrapSafe2(pla,k N)) {num += genStepStepPPPE(b,pla,k N,k,ploc,eloc,mv+num,hm+num,hmval);}
	}
	else if(ISO(k)) {num += genPushPPPE(b,pla,k,ploc,eloc,mv+num,hm+num,hmval);}

	return num;
}

//Can push eloc at all?
static bool canPP(Board& b, pla_t pla, loc_t eloc)
{
	if(!b.isEdge(eloc))
	{
		if(ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
		{
			if(ISE(eloc W)  || ISE(eloc E)  || ISE(eloc N) ||
			   ISE(eloc SW) || ISE(eloc SE) || (CS2(eloc) && ISE(eloc SS)))
			{return true;}
		}
		if(ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
		{
			if(ISE(eloc S)  || ISE(eloc E)  || ISE(eloc N) ||
			   ISE(eloc SW) || ISE(eloc NW) || (CW2(eloc) && ISE(eloc WW)))
			{return true;}
		}
		if(ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
		{
			if(ISE(eloc S)  || ISE(eloc W)  || ISE(eloc N) ||
			   ISE(eloc SE) || ISE(eloc NE) || (CE2(eloc) && ISE(eloc EE)))
			{return true;}
		}
		if(ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
		{
			if(ISE(eloc S)  || ISE(eloc W) || ISE(eloc E) ||
			   ISE(eloc NW) || ISE(eloc NE) || (CN2(eloc) && ISE(eloc NN)))
			{return true;}
		}
	}
	else
	{
		if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
		{
			if((CW1(eloc) && ISE(eloc W))  || (CE1(eloc) && ISE(eloc E))  || (CN1(eloc) && ISE(eloc N)) ||
			   (CW1(eloc) && ISE(eloc SW)) || (CE1(eloc) && ISE(eloc SE)) || (CS2(eloc) && ISE(eloc SS)))
			{return true;}
		}
		if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
		{
			if((CS1(eloc) && ISE(eloc S))  || (CE1(eloc) && ISE(eloc E))  || (CN1(eloc) && ISE(eloc N)) ||
			   (CS1(eloc) && ISE(eloc SW)) || (CW2(eloc) && ISE(eloc WW)) || (CN1(eloc) && ISE(eloc NW)))
			{return true;}
		}
		if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
		{
			if((CS1(eloc) && ISE(eloc S))  || (CW1(eloc) && ISE(eloc W))  || (CN1(eloc) && ISE(eloc N)) ||
			   (CS1(eloc) && ISE(eloc SE)) || (CE2(eloc) && ISE(eloc EE)) || (CN1(eloc) && ISE(eloc NE)))
			{return true;}
		}
		if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
		{
			if((CS1(eloc) && ISE(eloc S))  || (CW1(eloc) && ISE(eloc W))  || (CE1(eloc) && ISE(eloc E)) ||
			   (CW1(eloc) && ISE(eloc NW)) || (CE1(eloc) && ISE(eloc NE)) || (CN2(eloc) && ISE(eloc NN)))
			{return true;}
		}
	}
	return false;
}

//Can push eloc at all?
static int genPP(Board& b, pla_t pla, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	if(!b.isEdge(eloc))
	{
		if(ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
		{
			if(ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW, eloc S MN),hmval)}
			if(ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME, eloc S MN),hmval)}
			if(ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN, eloc S MN),hmval)}

			if(CS2(eloc) && ISE(eloc SS)) {ADDMOVEPP(Board::getMove(eloc S MS, eloc MS),hmval)}
			if(             ISE(eloc SW)) {ADDMOVEPP(Board::getMove(eloc S MW, eloc MS),hmval)}
			if(             ISE(eloc SE)) {ADDMOVEPP(Board::getMove(eloc S ME, eloc MS),hmval)}
		}
		if(ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
		{
			if(ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS, eloc W ME),hmval)}
			if(ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME, eloc W ME),hmval)}
			if(ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN, eloc W ME),hmval)}

			if(             ISE(eloc SW)) {ADDMOVEPP(Board::getMove(eloc W MS, eloc MW),hmval)}
			if(CW2(eloc) && ISE(eloc WW)) {ADDMOVEPP(Board::getMove(eloc W MW, eloc MW),hmval)}
			if(             ISE(eloc NW)) {ADDMOVEPP(Board::getMove(eloc W MN, eloc MW),hmval)}
		}
		if(ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
		{
			if(ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS, eloc E MW),hmval)}
			if(ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW, eloc E MW),hmval)}
			if(ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN, eloc E MW),hmval)}

			if(             ISE(eloc SE)) {ADDMOVEPP(Board::getMove(eloc E MS, eloc ME),hmval)}
			if(CE2(eloc) && ISE(eloc EE)) {ADDMOVEPP(Board::getMove(eloc E ME, eloc ME),hmval)}
			if(             ISE(eloc NE)) {ADDMOVEPP(Board::getMove(eloc E MN, eloc ME),hmval)}
		}
		if(ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
		{
			if(ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS, eloc N MS),hmval)}
			if(ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW, eloc N MS),hmval)}
			if(ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME, eloc N MS),hmval)}

			if(             ISE(eloc NW)) {ADDMOVEPP(Board::getMove(eloc N MW, eloc MN),hmval)}
			if(             ISE(eloc NE)) {ADDMOVEPP(Board::getMove(eloc N ME, eloc MN),hmval)}
			if(CN2(eloc) && ISE(eloc NN)) {ADDMOVEPP(Board::getMove(eloc N MN, eloc MN),hmval)}
		}
	}
	else
	{
		if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
		{
			if(CW1(eloc) && ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW, eloc S MN),hmval)}
			if(CE1(eloc) && ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME, eloc S MN),hmval)}
			if(CN1(eloc) && ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN, eloc S MN),hmval)}

			if(CS2(eloc) && ISE(eloc SS)) {ADDMOVEPP(Board::getMove(eloc S MS, eloc MS),hmval)}
			if(CW1(eloc) && ISE(eloc SW)) {ADDMOVEPP(Board::getMove(eloc S MW, eloc MS),hmval)}
			if(CE1(eloc) && ISE(eloc SE)) {ADDMOVEPP(Board::getMove(eloc S ME, eloc MS),hmval)}
		}
		if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
		{
			if(CS1(eloc) && ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS, eloc W ME),hmval)}
			if(CE1(eloc) && ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME, eloc W ME),hmval)}
			if(CN1(eloc) && ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN, eloc W ME),hmval)}

			if(CS1(eloc) && ISE(eloc SW)) {ADDMOVEPP(Board::getMove(eloc W MS, eloc MW),hmval)}
			if(CW2(eloc) && ISE(eloc WW)) {ADDMOVEPP(Board::getMove(eloc W MW, eloc MW),hmval)}
			if(CN1(eloc) && ISE(eloc NW)) {ADDMOVEPP(Board::getMove(eloc W MN, eloc MW),hmval)}
		}
		if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
		{
			if(CS1(eloc) && ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS, eloc E MW),hmval)}
			if(CW1(eloc) && ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW, eloc E MW),hmval)}
			if(CN1(eloc) && ISE(eloc N)) {ADDMOVEPP(Board::getMove(eloc MN, eloc E MW),hmval)}

			if(CS1(eloc) && ISE(eloc SE)) {ADDMOVEPP(Board::getMove(eloc E MS, eloc ME),hmval)}
			if(CE2(eloc) && ISE(eloc EE)) {ADDMOVEPP(Board::getMove(eloc E ME, eloc ME),hmval)}
			if(CN1(eloc) && ISE(eloc NE)) {ADDMOVEPP(Board::getMove(eloc E MN, eloc ME),hmval)}
		}
		if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
		{
			if(CS1(eloc) && ISE(eloc S)) {ADDMOVEPP(Board::getMove(eloc MS, eloc N MS),hmval)}
			if(CW1(eloc) && ISE(eloc W)) {ADDMOVEPP(Board::getMove(eloc MW, eloc N MS),hmval)}
			if(CE1(eloc) && ISE(eloc E)) {ADDMOVEPP(Board::getMove(eloc ME, eloc N MS),hmval)}

			if(CW1(eloc) && ISE(eloc NW)) {ADDMOVEPP(Board::getMove(eloc N MW, eloc MN),hmval)}
			if(CE1(eloc) && ISE(eloc NE)) {ADDMOVEPP(Board::getMove(eloc N ME, eloc MN),hmval)}
			if(CN2(eloc) && ISE(eloc NN)) {ADDMOVEPP(Board::getMove(eloc N MN, eloc MN),hmval)}
		}
	}

	return num;
}

//Is there some stronger piece adjancent such that we can get
//eloc to dest in a pushpull?
//Dest can be occupied by a player or an enemy piece.
static bool canPPTo(Board& b, pla_t pla, loc_t eloc, loc_t dest)
{
	if(!b.isEdge2(dest))
	{
		if(ISE(dest))
		{
			return
			(ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S)) ||
			(ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W)) ||
			(ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E)) ||
			(ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N));
		}
		else if(ISP(dest) && GT(dest,eloc) && b.isThawedC(dest))
		{
			return
			(ISE(dest S)) ||
			(ISE(dest W)) ||
			(ISE(dest E)) ||
			(ISE(dest N));
		}
	}
	else
	{
		if(ISE(dest))
		{
			return
			(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S)) ||
			(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W)) ||
			(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E)) ||
			(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N));
		}
		else if(ISP(dest) && GT(dest,eloc) && b.isThawedC(dest))
		{
			return
			(CS1(dest) && ISE(dest S)) ||
			(CW1(dest) && ISE(dest W)) ||
			(CE1(dest) && ISE(dest E)) ||
			(CN1(dest) && ISE(dest N));
		}
	}

	return false;
}

//Is there some stronger piece adjacent such that we can get
//eloc to dest in a pushpull?
//Dest can be occupied by a player or an enemy piece.
static int genPPTo(Board& b, pla_t pla, loc_t eloc, loc_t dest,
move_t* mv, int* hm, int hmval)
{
	step_t estep = Board::STEPINDEX[eloc][dest];
	int num = 0;

	if(!b.isEdge2(dest))
	{
		//Pushing
		if(ISE(dest))
		{
			if(ISP(eloc S) && GT(eloc S, eloc) && b.isThawedC(eloc S)) {ADDMOVEPP(Board::getMove(estep,eloc S MN),hmval)}
			if(ISP(eloc W) && GT(eloc W, eloc) && b.isThawedC(eloc W)) {ADDMOVEPP(Board::getMove(estep,eloc W ME),hmval)}
			if(ISP(eloc E) && GT(eloc E, eloc) && b.isThawedC(eloc E)) {ADDMOVEPP(Board::getMove(estep,eloc E MW),hmval)}
			if(ISP(eloc N) && GT(eloc N, eloc) && b.isThawedC(eloc N)) {ADDMOVEPP(Board::getMove(estep,eloc N MS),hmval)}
		}
		//Pulling
		else if(ISP(dest) && GT(dest,eloc) && b.isThawedC(dest))
		{
			if(ISE(dest S)) {ADDMOVEPP(Board::getMove(dest MS,estep),hmval)}
			if(ISE(dest W)) {ADDMOVEPP(Board::getMove(dest MW,estep),hmval)}
			if(ISE(dest E)) {ADDMOVEPP(Board::getMove(dest ME,estep),hmval)}
			if(ISE(dest N)) {ADDMOVEPP(Board::getMove(dest MN,estep),hmval)}
		}
	}
	else
	{
		//Pushing
		if(ISE(dest))
		{
			if(CS1(eloc) && ISP(eloc S) && GT(eloc S, eloc) && b.isThawedC(eloc S)) {ADDMOVEPP(Board::getMove(estep,eloc S MN),hmval)}
			if(CW1(eloc) && ISP(eloc W) && GT(eloc W, eloc) && b.isThawedC(eloc W)) {ADDMOVEPP(Board::getMove(estep,eloc W ME),hmval)}
			if(CE1(eloc) && ISP(eloc E) && GT(eloc E, eloc) && b.isThawedC(eloc E)) {ADDMOVEPP(Board::getMove(estep,eloc E MW),hmval)}
			if(CN1(eloc) && ISP(eloc N) && GT(eloc N, eloc) && b.isThawedC(eloc N)) {ADDMOVEPP(Board::getMove(estep,eloc N MS),hmval)}
		}
		//Pulling
		else if(ISP(dest) && GT(dest,eloc) && b.isThawedC(dest))
		{
			if(CS1(dest) && ISE(dest S)) {ADDMOVEPP(Board::getMove(dest MS,estep),hmval)}
			if(CW1(dest) && ISE(dest W)) {ADDMOVEPP(Board::getMove(dest MW,estep),hmval)}
			if(CE1(dest) && ISE(dest E)) {ADDMOVEPP(Board::getMove(dest ME,estep),hmval)}
			if(CN1(dest) && ISE(dest N)) {ADDMOVEPP(Board::getMove(dest MN,estep),hmval)}
		}
	}
	return num;
}

//Can we move piece stronger than eloc adjacent to dest so that the piece would be unfrozen there, but not a move to a forbidden location?
//Assumes that dest is empty or opponent
//Assumes that dest is in the center.
static bool canMoveStrongAdjCent(Board& b, pla_t pla, loc_t dest, loc_t eloc, loc_t forbidden)
{
	if(ISE(dest S) && dest S != forbidden)
	{
		if(CS2(dest) && ISP(dest SS) && GT(dest SS,eloc) && b.isThawedC(dest SS) && b.wouldBeUF(pla,dest SS,dest S,dest SS)) {return true;}
		if(             ISP(dest SW) && GT(dest SW,eloc) && b.isThawedC(dest SW) && b.wouldBeUF(pla,dest SW,dest S,dest SW)) {return true;}
		if(             ISP(dest SE) && GT(dest SE,eloc) && b.isThawedC(dest SE) && b.wouldBeUF(pla,dest SE,dest S,dest SE)) {return true;}
	}
	if(ISE(dest W) && dest W != forbidden)
	{
		if(             ISP(dest SW) && GT(dest SW,eloc) && b.isThawedC(dest SW) && b.wouldBeUF(pla,dest SW,dest W,dest SW)) {return true;}
		if(CW2(dest) && ISP(dest WW) && GT(dest WW,eloc) && b.isThawedC(dest WW) && b.wouldBeUF(pla,dest WW,dest W,dest WW)) {return true;}
		if(             ISP(dest NW) && GT(dest NW,eloc) && b.isThawedC(dest NW) && b.wouldBeUF(pla,dest NW,dest W,dest NW)) {return true;}
	}
	if(ISE(dest E) && dest E != forbidden)
	{
		if(             ISP(dest SE) && GT(dest SE,eloc) && b.isThawedC(dest SE) && b.wouldBeUF(pla,dest SE,dest E,dest SE)) {return true;}
		if(CE2(dest) && ISP(dest EE) && GT(dest EE,eloc) && b.isThawedC(dest EE) && b.wouldBeUF(pla,dest EE,dest E,dest EE)) {return true;}
		if(             ISP(dest NE) && GT(dest NE,eloc) && b.isThawedC(dest NE) && b.wouldBeUF(pla,dest NE,dest E,dest NE)) {return true;}
	}
	if(ISE(dest N) && dest N != forbidden)
	{
		if(             ISP(dest NW) && GT(dest NW,eloc) && b.isThawedC(dest NW) && b.wouldBeUF(pla,dest NW,dest N,dest NW)) {return true;}
		if(             ISP(dest NE) && GT(dest NE,eloc) && b.isThawedC(dest NE) && b.wouldBeUF(pla,dest NE,dest N,dest NE)) {return true;}
		if(CN2(dest) && ISP(dest NN) && GT(dest NN,eloc) && b.isThawedC(dest NN) && b.wouldBeUF(pla,dest NN,dest N,dest NN)) {return true;}
	}
	return false;
}

//Can we move piece stronger than eloc adjacent to dest so that the piece would be unfrozen there, but not a move to a forbidden location?
//Assumes that dest is empty or opponent
//Assumes that dest is in the center.
static int genMoveStrongAdjCent(Board& b, pla_t pla, loc_t dest, loc_t eloc, loc_t forbidden,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	if(ISE(dest S) && dest S != forbidden)
	{
		if(CS2(dest) && ISP(dest SS) && GT(dest SS,eloc) && b.isThawedC(dest SS) && b.wouldBeUF(pla,dest SS,dest S,dest SS)) {ADDMOVE(Board::getMove(dest SS MN),hmval)}
		if(             ISP(dest SW) && GT(dest SW,eloc) && b.isThawedC(dest SW) && b.wouldBeUF(pla,dest SW,dest S,dest SW)) {ADDMOVE(Board::getMove(dest SW ME),hmval)}
		if(             ISP(dest SE) && GT(dest SE,eloc) && b.isThawedC(dest SE) && b.wouldBeUF(pla,dest SE,dest S,dest SE)) {ADDMOVE(Board::getMove(dest SE MW),hmval)}
	}
	if(ISE(dest W) && dest W != forbidden)
	{
		if(             ISP(dest SW) && GT(dest SW,eloc) && b.isThawedC(dest SW) && b.wouldBeUF(pla,dest SW,dest W,dest SW)) {ADDMOVE(Board::getMove(dest SW MN),hmval)}
		if(CW2(dest) && ISP(dest WW) && GT(dest WW,eloc) && b.isThawedC(dest WW) && b.wouldBeUF(pla,dest WW,dest W,dest WW)) {ADDMOVE(Board::getMove(dest WW ME),hmval)}
		if(             ISP(dest NW) && GT(dest NW,eloc) && b.isThawedC(dest NW) && b.wouldBeUF(pla,dest NW,dest W,dest NW)) {ADDMOVE(Board::getMove(dest NW MS),hmval)}
	}
	if(ISE(dest E) && dest E != forbidden)
	{
		if(             ISP(dest SE) && GT(dest SE,eloc) && b.isThawedC(dest SE) && b.wouldBeUF(pla,dest SE,dest E,dest SE)) {ADDMOVE(Board::getMove(dest SE MN),hmval)}
		if(CE2(dest) && ISP(dest EE) && GT(dest EE,eloc) && b.isThawedC(dest EE) && b.wouldBeUF(pla,dest EE,dest E,dest EE)) {ADDMOVE(Board::getMove(dest EE MW),hmval)}
		if(             ISP(dest NE) && GT(dest NE,eloc) && b.isThawedC(dest NE) && b.wouldBeUF(pla,dest NE,dest E,dest NE)) {ADDMOVE(Board::getMove(dest NE MS),hmval)}
	}
	if(ISE(dest N) && dest N != forbidden)
	{
		if(             ISP(dest NW) && GT(dest NW,eloc) && b.isThawedC(dest NW) && b.wouldBeUF(pla,dest NW,dest N,dest NW)) {ADDMOVE(Board::getMove(dest NW ME),hmval)}
		if(             ISP(dest NE) && GT(dest NE,eloc) && b.isThawedC(dest NE) && b.wouldBeUF(pla,dest NE,dest N,dest NE)) {ADDMOVE(Board::getMove(dest NE MW),hmval)}
		if(CN2(dest) && ISP(dest NN) && GT(dest NN,eloc) && b.isThawedC(dest NN) && b.wouldBeUF(pla,dest NN,dest N,dest NN)) {ADDMOVE(Board::getMove(dest NN MS),hmval)}
	}
	return num;
}

static bool canAdvanceUFStep(Board& b, pla_t pla, loc_t curploc, loc_t futploc)
{
	if(!b.isTrapSafe2(pla,curploc))
	{return false;}

	if(futploc == curploc S)
	{
		if(CW1(curploc) && ISP(curploc W) && ISE(futploc W) && b.isTrapSafe2(pla,futploc W) && b.wouldBeUF(pla,curploc,curploc,curploc W) && b.isRabOkayS(pla,curploc W)) {return true;}
		if(CE1(curploc) && ISP(curploc E) && ISE(futploc E) && b.isTrapSafe2(pla,futploc E) && b.wouldBeUF(pla,curploc,curploc,curploc E) && b.isRabOkayS(pla,curploc E)) {return true;}
	}
	else if(futploc == curploc W)
	{
		if(CS1(curploc) && ISP(curploc S) && ISE(futploc S) && b.isTrapSafe2(pla,futploc S) && b.wouldBeUF(pla,curploc,curploc,curploc S)) {return true;}
		if(CN1(curploc) && ISP(curploc N) && ISE(futploc N) && b.isTrapSafe2(pla,futploc N) && b.wouldBeUF(pla,curploc,curploc,curploc N)) {return true;}
	}
	else if(futploc == curploc E)
	{
		if(CS1(curploc) && ISP(curploc S) && ISE(futploc S) && b.isTrapSafe2(pla,futploc S) && b.wouldBeUF(pla,curploc,curploc,curploc S)) {return true;}
		if(CN1(curploc) && ISP(curploc N) && ISE(futploc N) && b.isTrapSafe2(pla,futploc N) && b.wouldBeUF(pla,curploc,curploc,curploc N)) {return true;}
	}
	else if(futploc == curploc N)
	{
		if(CW1(curploc) && ISP(curploc W) && ISE(futploc W) && b.isTrapSafe2(pla,futploc W) && b.wouldBeUF(pla,curploc,curploc,curploc W) && b.isRabOkayN(pla,curploc W)) {return true;}
		if(CE1(curploc) && ISP(curploc E) && ISE(futploc E) && b.isTrapSafe2(pla,futploc E) && b.wouldBeUF(pla,curploc,curploc,curploc E) && b.isRabOkayN(pla,curploc E)) {return true;}
	}
	return false;
}

static int genAdvanceUFStep(Board& b, pla_t pla, loc_t curploc, loc_t futploc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;

	if(!b.isTrapSafe2(pla,curploc))
	{return 0;}

	if(futploc == curploc S)
	{
		if(CW1(curploc) && ISP(curploc W) && ISE(futploc W) && b.isTrapSafe2(pla,futploc W) && b.wouldBeUF(pla,curploc,curploc,curploc W) && b.isRabOkayS(pla,curploc W)) {ADDMOVEPP(Board::getMove(curploc W MS),hmval)}
		if(CE1(curploc) && ISP(curploc E) && ISE(futploc E) && b.isTrapSafe2(pla,futploc E) && b.wouldBeUF(pla,curploc,curploc,curploc E) && b.isRabOkayS(pla,curploc E)) {ADDMOVEPP(Board::getMove(curploc E MS),hmval)}
	}
	else if(futploc == curploc W)
	{
		if(CS1(curploc) && ISP(curploc S) && ISE(futploc S) && b.isTrapSafe2(pla,futploc S) && b.wouldBeUF(pla,curploc,curploc,curploc S)) {ADDMOVEPP(Board::getMove(curploc S MW),hmval)}
		if(CN1(curploc) && ISP(curploc N) && ISE(futploc N) && b.isTrapSafe2(pla,futploc N) && b.wouldBeUF(pla,curploc,curploc,curploc N)) {ADDMOVEPP(Board::getMove(curploc N MW),hmval)}
	}
	else if(futploc == curploc E)
	{
		if(CS1(curploc) && ISP(curploc S) && ISE(futploc S) && b.isTrapSafe2(pla,futploc S) && b.wouldBeUF(pla,curploc,curploc,curploc S)) {ADDMOVEPP(Board::getMove(curploc S ME),hmval)}
		if(CN1(curploc) && ISP(curploc N) && ISE(futploc N) && b.isTrapSafe2(pla,futploc N) && b.wouldBeUF(pla,curploc,curploc,curploc N)) {ADDMOVEPP(Board::getMove(curploc N ME),hmval)}
	}
	else if(futploc == curploc N)
	{
		if(CW1(curploc) && ISP(curploc W) && ISE(futploc W) && b.isTrapSafe2(pla,futploc W) && b.wouldBeUF(pla,curploc,curploc,curploc W) && b.isRabOkayN(pla,curploc W)) {ADDMOVEPP(Board::getMove(curploc W MN),hmval)}
		if(CE1(curploc) && ISP(curploc E) && ISE(futploc E) && b.isTrapSafe2(pla,futploc E) && b.wouldBeUF(pla,curploc,curploc,curploc E) && b.isRabOkayN(pla,curploc E)) {ADDMOVEPP(Board::getMove(curploc E MN),hmval)}
	}
	return num;
}

static bool canUFStepWouldBeUF(Board& b, pla_t pla, loc_t ploc, loc_t wloc, loc_t kt)
{
	int meNum = ISP(kt S) + ISP(kt W) + ISP(kt E) + ISP(kt N);

	if(CS1(ploc) && ISE(ploc S) && ploc S != wloc && (ploc S != kt || meNum <= 2))
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawedC(ploc SS) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SS) && b.isRabOkayN(pla,ploc SS)) {return true;}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SW)                             ) {return true;}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SE)                             ) {return true;}
	}
	if(CW1(ploc) && ISE(ploc W) && ploc W != wloc && (ploc W != kt || meNum <= 2))
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SW) && b.isRabOkayN(pla,ploc SW)) {return true;}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawedC(ploc WW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc WW)                             ) {return true;}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NW) && b.isRabOkayS(pla,ploc NW)) {return true;}
	}
	if(CE1(ploc) && ISE(ploc E) && ploc E != wloc && (ploc E != kt || meNum <= 2))
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SE) && b.isRabOkayN(pla,ploc SE)) {return true;}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawedC(ploc EE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc EE)                             ) {return true;}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NE) && b.isRabOkayS(pla,ploc NE)) {return true;}
	}
	if(CN1(ploc) && ISE(ploc N) && ploc N != wloc && (ploc N != kt || meNum <= 2))
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NW)                             ) {return true;}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NE)                             ) {return true;}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawedC(ploc NN) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NN) && b.isRabOkayS(pla,ploc NN)) {return true;}
	}
	return false;
}

static int genUFStepWouldBeUF(Board& b, pla_t pla, loc_t ploc, loc_t wloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	int meNum = ISP(kt S) + ISP(kt W) + ISP(kt E) + ISP(kt N);

	//Cannot step on to wLoc, since that would block the piece unfrozen.
	//Can step onto kT only if there is only two defenders or less of the trap, creating a suidical capture. Ex:
	/*
	.C..
	.He.
	X*M.
	.c..
	*/

	if(CS1(ploc) && ISE(ploc S) && ploc S != wloc && (ploc S != kt || meNum <= 2))
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawedC(ploc SS) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SS) && b.isRabOkayN(pla,ploc SS)) {ADDMOVEPP(Board::getMove(ploc SS MN),hmval)}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SW)                             ) {ADDMOVEPP(Board::getMove(ploc SW ME),hmval)}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SE)                             ) {ADDMOVEPP(Board::getMove(ploc SE MW),hmval)}
	}
	if(CW1(ploc) && ISE(ploc W) && ploc W != wloc && (ploc W != kt || meNum <= 2))
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SW) && b.isRabOkayN(pla,ploc SW)) {ADDMOVEPP(Board::getMove(ploc SW MN),hmval)}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawedC(ploc WW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc WW)                             ) {ADDMOVEPP(Board::getMove(ploc WW ME),hmval)}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NW) && b.isRabOkayS(pla,ploc NW)) {ADDMOVEPP(Board::getMove(ploc NW MS),hmval)}
	}
	if(CE1(ploc) && ISE(ploc E) && ploc E != wloc && (ploc E != kt || meNum <= 2))
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc SE) && b.isRabOkayN(pla,ploc SE)) {ADDMOVEPP(Board::getMove(ploc SE MN),hmval)}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawedC(ploc EE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc EE)                             ) {ADDMOVEPP(Board::getMove(ploc EE MW),hmval)}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NE) && b.isRabOkayS(pla,ploc NE)) {ADDMOVEPP(Board::getMove(ploc NE MS),hmval)}
	}
	if(CN1(ploc) && ISE(ploc N) && ploc N != wloc && (ploc N != kt || meNum <= 2))
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NW)                             ) {ADDMOVEPP(Board::getMove(ploc NW ME),hmval)}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NE)                             ) {ADDMOVEPP(Board::getMove(ploc NE MW),hmval)}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawedC(ploc NN) && b.wouldBeUF(pla,ploc,wloc,ploc,ploc NN) && b.isRabOkayS(pla,ploc NN)) {ADDMOVEPP(Board::getMove(ploc NN MS),hmval)}
	}

	return num;
}

static bool canStepStepc(Board& b, pla_t pla, loc_t dest, loc_t dest2)
{
	if((dest N == dest2 && pla == 0) || (dest S == dest2 && pla == 1))
	{
		if(CS1(dest) && ISP(dest S) && b.isThawedC(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.pieces[dest S] != RAB) {return true;}
		if(CW1(dest) && ISP(dest W) && b.isThawedC(dest W) && b.wouldBeUF(pla,dest W,dest,dest W) && b.pieces[dest W] != RAB) {return true;}
		if(CE1(dest) && ISP(dest E) && b.isThawedC(dest E) && b.wouldBeUF(pla,dest E,dest,dest E) && b.pieces[dest E] != RAB) {return true;}
		if(CN1(dest) && ISP(dest N) && b.isThawedC(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.pieces[dest N] != RAB) {return true;}
	}
	else
	{
		if(CS1(dest) && ISP(dest S) && b.isThawedC(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.isRabOkayN(pla,dest S)) {return true;}
		if(CW1(dest) && ISP(dest W) && b.isThawedC(dest W) && b.wouldBeUF(pla,dest W,dest,dest W))                             {return true;}
		if(CE1(dest) && ISP(dest E) && b.isThawedC(dest E) && b.wouldBeUF(pla,dest E,dest,dest E))                             {return true;}
		if(CN1(dest) && ISP(dest N) && b.isThawedC(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.isRabOkayS(pla,dest N)) {return true;}
	}
	return false;
}

static int genStepStep(Board& b, pla_t pla, loc_t dest, loc_t dest2,
move_t* mv, int* hm, int hmval)
{
	int num = 0;

	if((dest N == dest2 && pla == 0) || (dest S == dest2 && pla == 1))
	{
		if(CS1(dest) && ISP(dest S) && b.isThawedC(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.pieces[dest S] != RAB) {ADDMOVEPP(Board::getMove(dest S MN),hmval)}
		if(CW1(dest) && ISP(dest W) && b.isThawedC(dest W) && b.wouldBeUF(pla,dest W,dest,dest W) && b.pieces[dest W] != RAB) {ADDMOVEPP(Board::getMove(dest W ME),hmval)}
		if(CE1(dest) && ISP(dest E) && b.isThawedC(dest E) && b.wouldBeUF(pla,dest E,dest,dest E) && b.pieces[dest E] != RAB) {ADDMOVEPP(Board::getMove(dest E MW),hmval)}
		if(CN1(dest) && ISP(dest N) && b.isThawedC(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.pieces[dest N] != RAB) {ADDMOVEPP(Board::getMove(dest N MS),hmval)}
	}
	else
	{
		if(CS1(dest) && ISP(dest S) && b.isThawedC(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.isRabOkayN(pla,dest S)) {ADDMOVEPP(Board::getMove(dest S MN),hmval)}
		if(CW1(dest) && ISP(dest W) && b.isThawedC(dest W) && b.wouldBeUF(pla,dest W,dest,dest W))                             {ADDMOVEPP(Board::getMove(dest W ME),hmval)}
		if(CE1(dest) && ISP(dest E) && b.isThawedC(dest E) && b.wouldBeUF(pla,dest E,dest,dest E))                             {ADDMOVEPP(Board::getMove(dest E MW),hmval)}
		if(CN1(dest) && ISP(dest N) && b.isThawedC(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.isRabOkayS(pla,dest N)) {ADDMOVEPP(Board::getMove(dest N MS),hmval)}
	}
	return num;
}


//2 STEP CAPTURES----------------------------------------------------------------------------------------------

static bool canPPIntoTrap2C(Board& b, pla_t pla, loc_t eloc, loc_t kt)
{
	return canPPTo(b,pla,eloc,kt);
}

static int genPPIntoTrap2C(Board& b, pla_t pla, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	return genPPTo(b,pla,eloc,kt,mv,hm,hmval);
}

static bool canRemoveDef2C(Board& b, pla_t pla, loc_t eloc)
{
	return canPP(b,pla,eloc);
}

static int genRemoveDef2C(Board& b, pla_t pla, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	return genPP(b,pla,eloc, mv, hm, hmval);
}

//3 STEP CAPTURES-----------------------------------------------------------------------------------------------

static bool canPPIntoTrapTE3C(Board& b, pla_t pla, loc_t eloc, loc_t kt)
{
	//Frozen but can push   //If eloc +/- X is kt, then it will not execute because no player on that trap.
	if(ISP(eloc S) && GT(eloc S,eloc) && b.isFrozenC(eloc S) && canUF(b,pla,eloc S)) {return true;}
	if(ISP(eloc W) && GT(eloc W,eloc) && b.isFrozenC(eloc W) && canUF(b,pla,eloc W)) {return true;}
	if(ISP(eloc E) && GT(eloc E,eloc) && b.isFrozenC(eloc E) && canUF(b,pla,eloc E)) {return true;}
	if(ISP(eloc N) && GT(eloc N,eloc) && b.isFrozenC(eloc N) && canUF(b,pla,eloc N)) {return true;}

	int meNum = ISP(kt S) + ISP(kt W) + ISP(kt E) + ISP(kt N);
	//Step a stronger piece next to it, unfrozen to prep a pp
	//Forbid stepping on the trap if not safe yet.
	if(meNum >= 2)
	{return canMoveStrongAdjCent(b,pla,eloc,eloc,ERRORSQUARE);}
	else
	{return canMoveStrongAdjCent(b,pla,eloc,eloc,kt);}

	return false;
}

static int genPPIntoTrapTE3C(Board& b, pla_t pla, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	int num = 0;

	//Frozen but can push   //If eloc +/- X is kt, then it will not execute because no player on that trap.
	if(ISP(eloc S) && GT(eloc S,eloc) && b.isFrozenC(eloc S)) {num += genUF(b,pla,eloc S,mv+num,hm+num,hmval);}
	if(ISP(eloc W) && GT(eloc W,eloc) && b.isFrozenC(eloc W)) {num += genUF(b,pla,eloc W,mv+num,hm+num,hmval);}
	if(ISP(eloc E) && GT(eloc E,eloc) && b.isFrozenC(eloc E)) {num += genUF(b,pla,eloc E,mv+num,hm+num,hmval);}
	if(ISP(eloc N) && GT(eloc N,eloc) && b.isFrozenC(eloc N)) {num += genUF(b,pla,eloc N,mv+num,hm+num,hmval);}

	int meNum = ISP(kt S) + ISP(kt W) + ISP(kt E) + ISP(kt N);

	//Step a stronger piece next to it, unfrozen to prep a pp
	//Forbid stepping on the trap if not safe yet.
	if(meNum >= 2)
	{num += genMoveStrongAdjCent(b,pla,eloc,eloc,ERRORSQUARE,mv+num,hm+num,hmval);}
	else
	{num += genMoveStrongAdjCent(b,pla,eloc,eloc,kt,mv+num,hm+num,hmval);}

	return num;
}

static bool canPPIntoTrapTP3C(Board& b, pla_t pla, loc_t eloc, loc_t kt)
{
	//Weak piece on trap
	if(b.pieces[kt] <= b.pieces[eloc])
	{
		//Step off and push on
		if(ISE(kt S) && b.isRabOkayS(pla,kt) && couldPushToTrap(b,pla,eloc,kt,kt SW,kt SE)) {return true;}
		if(ISE(kt W)                         && couldPushToTrap(b,pla,eloc,kt,kt SW,kt NW)) {return true;}
		if(ISE(kt E)                         && couldPushToTrap(b,pla,eloc,kt,kt SE,kt NE)) {return true;}
		if(ISE(kt N) && b.isRabOkayN(pla,kt) && couldPushToTrap(b,pla,eloc,kt,kt NW,kt NE)) {return true;}

		//SUICIDAL
		if(!b.isTrapSafe2(pla,kt))
		{
			if(ISP(kt S) && GT(kt S,eloc))
			{
				if     (ISE(kt SW) && b.isAdjacent(kt SW, eloc) && b.wouldBeUF(pla,kt S,kt SW,kt S)) {return true;}
				else if(ISE(kt SE) && b.isAdjacent(kt SE, eloc) && b.wouldBeUF(pla,kt S,kt SE,kt S)) {return true;}
			}
			else if(ISP(kt W) && GT(kt W,eloc))
			{
				if     (ISE(kt SW) && b.isAdjacent(kt SW, eloc) && b.wouldBeUF(pla,kt W,kt SW,kt W)) {return true;}
				else if(ISE(kt NW) && b.isAdjacent(kt NW, eloc) && b.wouldBeUF(pla,kt W,kt NW,kt W)) {return true;}
			}
			else if(ISP(kt E) && GT(kt E,eloc))
			{
				if     (ISE(kt SE) && b.isAdjacent(kt SE, eloc) && b.wouldBeUF(pla,kt E,kt SE,kt E)) {return true;}
				else if(ISE(kt NE) && b.isAdjacent(kt NE, eloc) && b.wouldBeUF(pla,kt E,kt NE,kt E)) {return true;}
			}
			else if(ISP(kt N) && GT(kt N,eloc))
			{
				if     (ISE(kt NW) && b.isAdjacent(kt NW, eloc) && b.wouldBeUF(pla,kt N,kt NW,kt N)) {return true;}
				else if(ISE(kt NE) && b.isAdjacent(kt NE, eloc) && b.wouldBeUF(pla,kt N,kt NE,kt N)) {return true;}
			}
		}
	}

	//Strong piece, but blocked. Since no two step captures were possible, all squares around must be blocked. Since there can be at most 1 enemy
	//piece around the trap, stepping away is always safe.
	else
	{
		if(ISP(kt S))
		{
			if(ISE(kt SS) && b.isRabOkayS(pla,kt S)) {return true;}
			if(ISE(kt SW))                           {return true;}
			if(ISE(kt SE))                           {return true;}
		}
		if(ISP(kt W))
		{
			if(ISE(kt SW) && b.isRabOkayS(pla,kt W)) {return true;}
			if(ISE(kt WW))                           {return true;}
			if(ISE(kt NW) && b.isRabOkayN(pla,kt W)) {return true;}
		}
		if(ISP(kt E))
		{
			if(ISE(kt SE) && b.isRabOkayS(pla,kt E)) {return true;}
			if(ISE(kt EE))                           {return true;}
			if(ISE(kt NE) && b.isRabOkayN(pla,kt E)) {return true;}
		}
		if(ISP(kt N))
		{
			if(ISE(kt NW))                           {return true;}
			if(ISE(kt NE))                           {return true;}
			if(ISE(kt NN) && b.isRabOkayN(pla,kt N)) {return true;}
		}
	}

	return false;
}

static int genPPIntoTrapTP3C(Board& b, pla_t pla, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	int num = 0;

	//Weak piece on trap
	if(b.pieces[kt] <= b.pieces[eloc])
	{
		//Step off and push on
		if(ISE(kt S) && b.isRabOkayS(pla,kt) && couldPushToTrap(b,pla,eloc,kt,kt SW,kt SE)) {ADDMOVE(Board::getMove(kt MS),hmval)}
		if(ISE(kt W)                         && couldPushToTrap(b,pla,eloc,kt,kt SW,kt NW)) {ADDMOVE(Board::getMove(kt MW),hmval)}
		if(ISE(kt E)                         && couldPushToTrap(b,pla,eloc,kt,kt SE,kt NE)) {ADDMOVE(Board::getMove(kt ME),hmval)}
		if(ISE(kt N) && b.isRabOkayN(pla,kt) && couldPushToTrap(b,pla,eloc,kt,kt NW,kt NE)) {ADDMOVE(Board::getMove(kt MN),hmval)}

		//SUICIDAL
		if(!b.isTrapSafe2(pla,kt))
		{
			if(ISP(kt S) && GT(kt S,eloc))
			{
				if     (ISE(kt SW) && b.isAdjacent(kt SW, eloc) && b.wouldBeUF(pla,kt S,kt SW,kt S)) {ADDMOVE(Board::getMove(kt S MW),hmval)}
				else if(ISE(kt SE) && b.isAdjacent(kt SE, eloc) && b.wouldBeUF(pla,kt S,kt SE,kt S)) {ADDMOVE(Board::getMove(kt S ME),hmval)}
			}
			else if(ISP(kt W) && GT(kt W,eloc))
			{
				if     (ISE(kt SW) && b.isAdjacent(kt SW, eloc) && b.wouldBeUF(pla,kt W,kt SW,kt W)) {ADDMOVE(Board::getMove(kt W MS),hmval)}
				else if(ISE(kt NW) && b.isAdjacent(kt NW, eloc) && b.wouldBeUF(pla,kt W,kt NW,kt W)) {ADDMOVE(Board::getMove(kt W MN),hmval)}
			}
			else if(ISP(kt E) && GT(kt E,eloc))
			{
				if     (ISE(kt SE) && b.isAdjacent(kt SE, eloc) && b.wouldBeUF(pla,kt E,kt SE,kt E)) {ADDMOVE(Board::getMove(kt E MS),hmval)}
				else if(ISE(kt NE) && b.isAdjacent(kt NE, eloc) && b.wouldBeUF(pla,kt E,kt NE,kt E)) {ADDMOVE(Board::getMove(kt E MN),hmval)}
			}
			else if(ISP(kt N) && GT(kt N,eloc))
			{
				if     (ISE(kt NW) && b.isAdjacent(kt NW, eloc) && b.wouldBeUF(pla,kt N,kt NW,kt N)) {ADDMOVE(Board::getMove(kt N MW),hmval)}
				else if(ISE(kt NE) && b.isAdjacent(kt NE, eloc) && b.wouldBeUF(pla,kt N,kt NE,kt N)) {ADDMOVE(Board::getMove(kt N ME),hmval)}
			}
		}
	}

	//Strong piece, but blocked. Since no two step captures were possible, all squares around must be blocked. Since there can be at most 1 enemy
	//piece around the trap, stepping away is always safe.
	else
	{
		if(ISP(kt S))
		{
			if(ISE(kt SS) && b.isRabOkayS(pla,kt S)) {ADDMOVE(Board::getMove(kt S MS),hmval)}
			if(ISE(kt SW))                           {ADDMOVE(Board::getMove(kt S MW),hmval)}
			if(ISE(kt SE))                           {ADDMOVE(Board::getMove(kt S ME),hmval)}
		}
		if(ISP(kt W))
		{
			if(ISE(kt SW) && b.isRabOkayS(pla,kt W)) {ADDMOVE(Board::getMove(kt W MS),hmval)}
			if(ISE(kt WW))                           {ADDMOVE(Board::getMove(kt W MW),hmval)}
			if(ISE(kt NW) && b.isRabOkayN(pla,kt W)) {ADDMOVE(Board::getMove(kt W MN),hmval)}
		}
		if(ISP(kt E))
		{
			if(ISE(kt SE) && b.isRabOkayS(pla,kt E)) {ADDMOVE(Board::getMove(kt E MS),hmval)}
			if(ISE(kt EE))                           {ADDMOVE(Board::getMove(kt E ME),hmval)}
			if(ISE(kt NE) && b.isRabOkayN(pla,kt E)) {ADDMOVE(Board::getMove(kt E MN),hmval)}
		}
		if(ISP(kt N))
		{
			if(ISE(kt NW))                           {ADDMOVE(Board::getMove(kt N MW),hmval)}
			if(ISE(kt NE))                           {ADDMOVE(Board::getMove(kt N ME),hmval)}
			if(ISE(kt NN) && b.isRabOkayN(pla,kt N)) {ADDMOVE(Board::getMove(kt N MN),hmval)}
		}
	}

	return num;
}

static bool canRemoveDef3C(Board& b, pla_t pla, loc_t eloc, loc_t kt)
{
	//Frozen but can pushpull. Remove the defender.   //If eloc +/- X is kt, then it will not execute because no player on that trap.
	if(ISP(eloc S) && GT(eloc S,eloc) && b.isFrozenC(eloc S) && canUFPPPE(b,pla,eloc S,eloc)) {return true;}
	if(ISP(eloc W) && GT(eloc W,eloc) && b.isFrozenC(eloc W) && canUFPPPE(b,pla,eloc W,eloc)) {return true;}
	if(ISP(eloc E) && GT(eloc E,eloc) && b.isFrozenC(eloc E) && canUFPPPE(b,pla,eloc E,eloc)) {return true;}
	if(ISP(eloc N) && GT(eloc N,eloc) && b.isFrozenC(eloc N) && canUFPPPE(b,pla,eloc N,eloc)) {return true;}

	//Unfrozen, could pushpull, but stuff is in the way.
	//Step out of the way and push the defender
	if(ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S) && canBlockedPP(b,pla,eloc S,eloc)) {return true;}
	if(ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W) && canBlockedPP(b,pla,eloc W,eloc)) {return true;}
	if(ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E) && canBlockedPP(b,pla,eloc E,eloc)) {return true;}
	if(ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N) && canBlockedPP(b,pla,eloc N,eloc)) {return true;}

	//A step away, not going through the trap
	if(canMoveStrongAdjCent(b,pla,eloc,eloc,kt))
	{return true;}

	return false;
}

static int genRemoveDef3C(Board& b, pla_t pla, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	int num = 0;

	//Frozen but can pushpull. Remove the defender.   //If eloc +/- X is kt, then it will not execute because no player on that trap.
	if(ISP(eloc S) && GT(eloc S,eloc) && b.isFrozenC(eloc S)) {num += genUFPPPE(b,pla,eloc S,eloc,mv+num,hm+num,hmval);}
	if(ISP(eloc W) && GT(eloc W,eloc) && b.isFrozenC(eloc W)) {num += genUFPPPE(b,pla,eloc W,eloc,mv+num,hm+num,hmval);}
	if(ISP(eloc E) && GT(eloc E,eloc) && b.isFrozenC(eloc E)) {num += genUFPPPE(b,pla,eloc E,eloc,mv+num,hm+num,hmval);}
	if(ISP(eloc N) && GT(eloc N,eloc) && b.isFrozenC(eloc N)) {num += genUFPPPE(b,pla,eloc N,eloc,mv+num,hm+num,hmval);}

	//Unfrozen, could pushpull, but stuff is in the way.
	//Step out of the way and push the defender
	if(ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S)) {num += genBlockedPP(b,pla,eloc S,eloc,mv+num,hm+num,hmval);}
	if(ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W)) {num += genBlockedPP(b,pla,eloc W,eloc,mv+num,hm+num,hmval);}
	if(ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E)) {num += genBlockedPP(b,pla,eloc E,eloc,mv+num,hm+num,hmval);}
	if(ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N)) {num += genBlockedPP(b,pla,eloc N,eloc,mv+num,hm+num,hmval);}

	//A step away, not going through the trap
	num += genMoveStrongAdjCent(b,pla,eloc,eloc,kt,mv+num,hm+num,hmval);

	return num;
}


//4-STEP CAPTURES----------------------------------------------------------


//0 DEFENDERS--------------------------------------------------------------

//Does NOT use the C version of thawed or frozen!
static bool can0PieceTrail(Board& b, pla_t pla, loc_t eloc, loc_t tr, loc_t kt)
{
	bool suc = false;

	if(ISE(tr))
	{
		if(CS1(eloc) && ISP(eloc S) && GT(eloc S, eloc) && b.isThawed(eloc S)) {TPC(eloc S,eloc,tr,suc = canPPTo(b,pla,tr,kt)) if(suc){return true;}}
		if(CW1(eloc) && ISP(eloc W) && GT(eloc W, eloc) && b.isThawed(eloc W)) {TPC(eloc W,eloc,tr,suc = canPPTo(b,pla,tr,kt)) if(suc){return true;}}
		if(CE1(eloc) && ISP(eloc E) && GT(eloc E, eloc) && b.isThawed(eloc E)) {TPC(eloc E,eloc,tr,suc = canPPTo(b,pla,tr,kt)) if(suc){return true;}}
		if(CN1(eloc) && ISP(eloc N) && GT(eloc N, eloc) && b.isThawed(eloc N)) {TPC(eloc N,eloc,tr,suc = canPPTo(b,pla,tr,kt)) if(suc){return true;}}
	}
	//Pulling
	else if(ISP(tr) && GT(tr,eloc) && b.isThawed(tr))
	{
		if(ISE(tr S)) {TPC(eloc,tr,tr S,suc = canPPTo(b,pla,tr,kt)) if(suc){return true;}}
		if(ISE(tr W)) {TPC(eloc,tr,tr W,suc = canPPTo(b,pla,tr,kt)) if(suc){return true;}}
		if(ISE(tr E)) {TPC(eloc,tr,tr E,suc = canPPTo(b,pla,tr,kt)) if(suc){return true;}}
		if(ISE(tr N)) {TPC(eloc,tr,tr N,suc = canPPTo(b,pla,tr,kt)) if(suc){return true;}}
	}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static int gen0PieceTrail(Board& b, pla_t pla, loc_t eloc, loc_t tr, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	bool suc = false;
	step_t estep = Board::STEPINDEX[eloc][tr];

	if(ISE(tr))
	{
		if(CS1(eloc) && ISP(eloc S) && GT(eloc S, eloc) && b.isThawed(eloc S)) {TPC(eloc S,eloc,tr,suc = canPPTo(b,pla,tr,kt)) if(suc){ADDMOVEPP(Board::getMove(estep,eloc S MN),hmval)}}
		if(CW1(eloc) && ISP(eloc W) && GT(eloc W, eloc) && b.isThawed(eloc W)) {TPC(eloc W,eloc,tr,suc = canPPTo(b,pla,tr,kt)) if(suc){ADDMOVEPP(Board::getMove(estep,eloc W ME),hmval)}}
		if(CE1(eloc) && ISP(eloc E) && GT(eloc E, eloc) && b.isThawed(eloc E)) {TPC(eloc E,eloc,tr,suc = canPPTo(b,pla,tr,kt)) if(suc){ADDMOVEPP(Board::getMove(estep,eloc E MW),hmval)}}
		if(CN1(eloc) && ISP(eloc N) && GT(eloc N, eloc) && b.isThawed(eloc N)) {TPC(eloc N,eloc,tr,suc = canPPTo(b,pla,tr,kt)) if(suc){ADDMOVEPP(Board::getMove(estep,eloc N MS),hmval)}}
	}
	//Pulling
	else if(ISP(tr) && GT(tr,eloc) && b.isThawed(tr))
	{
		if(ISE(tr S)) {TPC(eloc,tr,tr S,suc = canPPTo(b,pla,tr,kt)) if(suc){ADDMOVEPP(Board::getMove(tr MS,estep),hmval)}}
		if(ISE(tr W)) {TPC(eloc,tr,tr W,suc = canPPTo(b,pla,tr,kt)) if(suc){ADDMOVEPP(Board::getMove(tr MW,estep),hmval)}}
		if(ISE(tr E)) {TPC(eloc,tr,tr E,suc = canPPTo(b,pla,tr,kt)) if(suc){ADDMOVEPP(Board::getMove(tr ME,estep),hmval)}}
		if(ISE(tr N)) {TPC(eloc,tr,tr N,suc = canPPTo(b,pla,tr,kt)) if(suc){ADDMOVEPP(Board::getMove(tr MN,estep),hmval)}}
	}

	return num;
}

//2 DEFENDERS--------------------------------------------------------------

//Does NOT use the C version of thawed or frozen!
static bool can2PieceAdj(Board& b, pla_t pla, loc_t eloc1, loc_t eloc2, loc_t kt)
{
	bool suc = false;
	pla_t opp = OPP(pla);

	if(ISE(kt))
	{
		//Push eloc1 into trap, then remove eloc2
		if(ISP(eloc1 S) && GT(eloc1 S, eloc1) && b.isThawed(eloc1 S)) {TP(eloc1 S,eloc1,kt,suc = canPP(b,pla,eloc2)) if(suc){return true;}}
		if(ISP(eloc1 W) && GT(eloc1 W, eloc1) && b.isThawed(eloc1 W)) {TP(eloc1 W,eloc1,kt,suc = canPP(b,pla,eloc2)) if(suc){return true;}}
		if(ISP(eloc1 E) && GT(eloc1 E, eloc1) && b.isThawed(eloc1 E)) {TP(eloc1 E,eloc1,kt,suc = canPP(b,pla,eloc2)) if(suc){return true;}}
		if(ISP(eloc1 N) && GT(eloc1 N, eloc1) && b.isThawed(eloc1 N)) {TP(eloc1 N,eloc1,kt,suc = canPP(b,pla,eloc2)) if(suc){return true;}}

		//Remove eloc2, then pushpull eloc1 into trap
		if(ISP(eloc2 S) && GT(eloc2 S,eloc2) && b.isThawed(eloc2 S))
		{
			if(ISE(eloc2 W) && eloc2 W != kt) {TP(eloc2 S,eloc2,eloc2 W,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 E) && eloc2 E != kt) {TP(eloc2 S,eloc2,eloc2 E,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 N) && eloc2 N != kt) {TP(eloc2 S,eloc2,eloc2 N,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}

			if(CS2(eloc2) && ISE(eloc2 SS)) {TP(eloc2,eloc2 S,eloc2 SS,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(              ISE(eloc2 SW)) {TP(eloc2,eloc2 S,eloc2 SW,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(              ISE(eloc2 SE)) {TP(eloc2,eloc2 S,eloc2 SE,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
		}
		if(ISP(eloc2 W) && GT(eloc2 W,eloc2) && b.isThawed(eloc2 W))
		{
			if(ISE(eloc2 S) && eloc2 S != kt) {TP(eloc2 W,eloc2,eloc2 S,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 E) && eloc2 E != kt) {TP(eloc2 W,eloc2,eloc2 E,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 N) && eloc2 N != kt) {TP(eloc2 W,eloc2,eloc2 N,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}

			if(              ISE(eloc2 SW)) {TP(eloc2,eloc2 W,eloc2 SW,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(CW2(eloc2) && ISE(eloc2 WW)) {TP(eloc2,eloc2 W,eloc2 WW,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(              ISE(eloc2 NW)) {TP(eloc2,eloc2 W,eloc2 NW,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
		}
		if(ISP(eloc2 E) && GT(eloc2 E,eloc2) && b.isThawed(eloc2 E))
		{
			if(ISE(eloc2 S) && eloc2 S != kt) {TP(eloc2 E,eloc2,eloc2 S,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 W) && eloc2 W != kt) {TP(eloc2 E,eloc2,eloc2 W,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 N) && eloc2 N != kt) {TP(eloc2 E,eloc2,eloc2 N,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}

			if(              ISE(eloc2 SE)) {TP(eloc2,eloc2 E,eloc2 SE,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(CE2(eloc2) && ISE(eloc2 EE)) {TP(eloc2,eloc2 E,eloc2 EE,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(              ISE(eloc2 NE)) {TP(eloc2,eloc2 E,eloc2 NE,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
		}
		if(ISP(eloc2 N) && GT(eloc2 N,eloc2) && b.isThawed(eloc2 N))
		{
			if(ISE(eloc2 S) && eloc2 S != kt) {TP(eloc2 N,eloc2,eloc2 S,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 W) && eloc2 W != kt) {TP(eloc2 N,eloc2,eloc2 W,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 E) && eloc2 E != kt) {TP(eloc2 N,eloc2,eloc2 E,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}

			if(              ISE(eloc2 NW)) {TP(eloc2,eloc2 N,eloc2 NW,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(              ISE(eloc2 NE)) {TP(eloc2,eloc2 N,eloc2 NE,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(CN2(eloc2) && ISE(eloc2 NN)) {TP(eloc2,eloc2 N,eloc2 NN,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
		}
	}
	else if(ISP(kt))
	{
		//Pull eloc1 into trap, then remove eloc2
		if(GT(kt,eloc1))
		{
			if(ISE(kt S)) {TP(eloc1,kt,kt S,suc = canPP(b,pla,eloc2)) if(suc){return true;}}
			if(ISE(kt W)) {TP(eloc1,kt,kt W,suc = canPP(b,pla,eloc2)) if(suc){return true;}}
			if(ISE(kt E)) {TP(eloc1,kt,kt E,suc = canPP(b,pla,eloc2)) if(suc){return true;}}
			if(ISE(kt N)) {TP(eloc1,kt,kt N,suc = canPP(b,pla,eloc2)) if(suc){return true;}}

			//Remove eloc2 to make room, then pull into trap, TempRemove to make sure kt doesn't pull yet
			TR(kt,suc = canPullc(b,pla,eloc2))
			if(suc) {return true;}
		}
		//Push eloc 2, then push eloc 1 to trap
		if(GT(kt,eloc2))
		{
			if(ISE(eloc2 S)) {TP(kt,eloc2,eloc2 S,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 W)) {TP(kt,eloc2,eloc2 W,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 E)) {TP(kt,eloc2,eloc2 E,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
			if(ISE(eloc2 N)) {TP(kt,eloc2,eloc2 N,suc = canPPTo(b,pla,eloc1,kt)) if(suc){return true;}}
		}
	}
	else if(ISO(kt))
	{
		//Remove eloc2, then remove eloc1
		if(ISP(eloc2 S) && GT(eloc2 S,eloc2) && b.isThawed(eloc2 S))
		{
			if(ISE(eloc2 W)) {TP(eloc2 S,eloc2,eloc2 W,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(ISE(eloc2 E)) {TP(eloc2 S,eloc2,eloc2 E,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(ISE(eloc2 N)) {TP(eloc2 S,eloc2,eloc2 N,suc = canPP(b,pla,eloc1)) if(suc){return true;}}

			if(CS2(eloc2) && ISE(eloc2 SS)) {TP(eloc2,eloc2 S,eloc2 SS,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(              ISE(eloc2 SW)) {TP(eloc2,eloc2 S,eloc2 SW,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(              ISE(eloc2 SE)) {TP(eloc2,eloc2 S,eloc2 SE,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
		}
		if(ISP(eloc2 W) && GT(eloc2 W,eloc2) && b.isThawed(eloc2 W))
		{
			if(ISE(eloc2 S)) {TP(eloc2 W,eloc2,eloc2 S,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(ISE(eloc2 E)) {TP(eloc2 W,eloc2,eloc2 E,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(ISE(eloc2 N)) {TP(eloc2 W,eloc2,eloc2 N,suc = canPP(b,pla,eloc1)) if(suc){return true;}}

			if(              ISE(eloc2 SW)) {TP(eloc2,eloc2 W,eloc2 SW,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(CW2(eloc2) && ISE(eloc2 WW)) {TP(eloc2,eloc2 W,eloc2 WW,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(              ISE(eloc2 NW)) {TP(eloc2,eloc2 W,eloc2 NW,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
		}
		if(ISP(eloc2 E) && GT(eloc2 E,eloc2) && b.isThawed(eloc2 E))
		{
			if(ISE(eloc2 S)) {TP(eloc2 E,eloc2,eloc2 S,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(ISE(eloc2 W)) {TP(eloc2 E,eloc2,eloc2 W,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(ISE(eloc2 N)) {TP(eloc2 E,eloc2,eloc2 N,suc = canPP(b,pla,eloc1)) if(suc){return true;}}

			if(              ISE(eloc2 SE)) {TP(eloc2,eloc2 E,eloc2 SE,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(CE2(eloc2) && ISE(eloc2 EE)) {TP(eloc2,eloc2 E,eloc2 EE,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(              ISE(eloc2 NE)) {TP(eloc2,eloc2 E,eloc2 NE,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
		}
		if(ISP(eloc2 N) && GT(eloc2 N,eloc2) && b.isThawed(eloc2 N))
		{
			if(ISE(eloc2 S)) {TP(eloc2 N,eloc2,eloc2 S,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(ISE(eloc2 W)) {TP(eloc2 N,eloc2,eloc2 W,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(ISE(eloc2 E)) {TP(eloc2 N,eloc2,eloc2 E,suc = canPP(b,pla,eloc1)) if(suc){return true;}}

			if(              ISE(eloc2 NW)) {TP(eloc2,eloc2 N,eloc2 NW,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(              ISE(eloc2 NE)) {TP(eloc2,eloc2 N,eloc2 NE,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
			if(CN2(eloc2) && ISE(eloc2 NN)) {TP(eloc2,eloc2 N,eloc2 NN,suc = canPP(b,pla,eloc1)) if(suc){return true;}}
		}
	}

	return false;
}

#define PREFIXMOVES(m,i) for(int aa = num; aa < newnum; aa++) {mv[aa] = ((mv[aa] << ((i)*8)) | ((m) & ((1 << ((i)*8))-1)));} num = newnum;
#define SUFFIXMOVES(m,i) for(int aa = num; aa < newnum; aa++) {mv[aa] = (((m) << ((i)*8)) | (mv[aa] & ((1 << ((i)*8))-1)));} num = newnum;

//Does NOT use the C version of thawed or frozen!
static int gen2PieceAdj(Board& b, pla_t pla, loc_t eloc1, loc_t eloc2, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	pla_t opp = OPP(pla);
	step_t estep = Board::STEPINDEX[eloc1][kt];

	if(ISE(kt))
	{
		//Push eloc1 into trap, then remove eloc2
		if(ISP(eloc1 S) && GT(eloc1 S, eloc1) && b.isThawed(eloc1 S)) {TP(eloc1 S,eloc1,kt,int newnum = num + genPP(b,pla,eloc2,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(estep,eloc1 S MN),2)}
		if(ISP(eloc1 W) && GT(eloc1 W, eloc1) && b.isThawed(eloc1 W)) {TP(eloc1 W,eloc1,kt,int newnum = num + genPP(b,pla,eloc2,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(estep,eloc1 W ME),2)}
		if(ISP(eloc1 E) && GT(eloc1 E, eloc1) && b.isThawed(eloc1 E)) {TP(eloc1 E,eloc1,kt,int newnum = num + genPP(b,pla,eloc2,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(estep,eloc1 E MW),2)}
		if(ISP(eloc1 N) && GT(eloc1 N, eloc1) && b.isThawed(eloc1 N)) {TP(eloc1 N,eloc1,kt,int newnum = num + genPP(b,pla,eloc2,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(estep,eloc1 N MS),2)}

		//Remove eloc2, then pushpull eloc1 into trap
		if(ISP(eloc2 S) && GT(eloc2 S,eloc2) && b.isThawed(eloc2 S))
		{
			if(ISE(eloc2 W) && eloc2 W != kt) {TP(eloc2 S,eloc2,eloc2 W,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MW, eloc2 S MN),2)}
			if(ISE(eloc2 E) && eloc2 E != kt) {TP(eloc2 S,eloc2,eloc2 E,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 ME, eloc2 S MN),2)}
			if(ISE(eloc2 N) && eloc2 N != kt) {TP(eloc2 S,eloc2,eloc2 N,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MN, eloc2 S MN),2)}

			if(CS2(eloc2) && ISE(eloc2 SS)) {TP(eloc2,eloc2 S,eloc2 SS,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 S MS, eloc2 MS),2)}
			if(              ISE(eloc2 SW)) {TP(eloc2,eloc2 S,eloc2 SW,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 S MW, eloc2 MS),2)}
			if(              ISE(eloc2 SE)) {TP(eloc2,eloc2 S,eloc2 SE,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 S ME, eloc2 MS),2)}
		}
		if(ISP(eloc2 W) && GT(eloc2 W,eloc2) && b.isThawed(eloc2 W))
		{
			if(ISE(eloc2 S) && eloc2 S != kt) {TP(eloc2 W,eloc2,eloc2 S,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MS, eloc2 W ME),2)}
			if(ISE(eloc2 E) && eloc2 E != kt) {TP(eloc2 W,eloc2,eloc2 E,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 ME, eloc2 W ME),2)}
			if(ISE(eloc2 N) && eloc2 N != kt) {TP(eloc2 W,eloc2,eloc2 N,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MN, eloc2 W ME),2)}

			if(              ISE(eloc2 SW)) {TP(eloc2,eloc2 W,eloc2 SW,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 W MS, eloc2 MW),2)}
			if(CW2(eloc2) && ISE(eloc2 WW)) {TP(eloc2,eloc2 W,eloc2 WW,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 W MW, eloc2 MW),2)}
			if(              ISE(eloc2 NW)) {TP(eloc2,eloc2 W,eloc2 NW,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 W MN, eloc2 MW),2)}
		}
		if(ISP(eloc2 E) && GT(eloc2 E,eloc2) && b.isThawed(eloc2 E))
		{
			if(ISE(eloc2 S) && eloc2 S != kt) {TP(eloc2 E,eloc2,eloc2 S,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MS, eloc2 E MW),2)}
			if(ISE(eloc2 W) && eloc2 W != kt) {TP(eloc2 E,eloc2,eloc2 W,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MW, eloc2 E MW),2)}
			if(ISE(eloc2 N) && eloc2 N != kt) {TP(eloc2 E,eloc2,eloc2 N,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MN, eloc2 E MW),2)}

			if(              ISE(eloc2 SE)) {TP(eloc2,eloc2 E,eloc2 SE,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 E MS, eloc2 ME),2)}
			if(CE2(eloc2) && ISE(eloc2 EE)) {TP(eloc2,eloc2 E,eloc2 EE,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 E ME, eloc2 ME),2)}
			if(              ISE(eloc2 NE)) {TP(eloc2,eloc2 E,eloc2 NE,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 E MN, eloc2 ME),2)}
		}
		if(ISP(eloc2 N) && GT(eloc2 N,eloc2) && b.isThawed(eloc2 N))
		{
			if(ISE(eloc2 S) && eloc2 S != kt) {TP(eloc2 N,eloc2,eloc2 S,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MS, eloc2 N MS),2)}
			if(ISE(eloc2 W) && eloc2 W != kt) {TP(eloc2 N,eloc2,eloc2 W,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MW, eloc2 N MS),2)}
			if(ISE(eloc2 E) && eloc2 E != kt) {TP(eloc2 N,eloc2,eloc2 E,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 ME, eloc2 N MS),2)}

			if(              ISE(eloc2 NW)) {TP(eloc2,eloc2 N,eloc2 NW,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 N MW, eloc2 MN),2)}
			if(              ISE(eloc2 NE)) {TP(eloc2,eloc2 N,eloc2 NE,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 N ME, eloc2 MN),2)}
			if(CN2(eloc2) && ISE(eloc2 NN)) {TP(eloc2,eloc2 N,eloc2 NN,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 N MN, eloc2 MN),2)}
		}
	}
	else if(ISP(kt))
	{
		//Pull eloc1 into trap, then remove eloc2
		if(GT(kt,eloc1))
		{
			if(ISE(kt S)) {TP(eloc1,kt,kt S,int newnum = num + genPP(b,pla,eloc2,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(kt MS,estep),2)}
			if(ISE(kt W)) {TP(eloc1,kt,kt W,int newnum = num + genPP(b,pla,eloc2,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(kt MW,estep),2)}
			if(ISE(kt E)) {TP(eloc1,kt,kt E,int newnum = num + genPP(b,pla,eloc2,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(kt ME,estep),2)}
			if(ISE(kt N)) {TP(eloc1,kt,kt N,int newnum = num + genPP(b,pla,eloc2,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(kt MN,estep),2)}

			//Remove eloc2 to make room, then pull into trap, TempRemove to make sure kt doesn't pull yet
			TR(kt,int newnum = num + genPull(b,pla,eloc2,mv+num,hm+num,hmval)) SUFFIXMOVES(Board::getMove(Board::STEPINDEX[kt][eloc2],Board::STEPINDEX[eloc1][kt]),2)
		}
		//Push eloc 2, then push eloc 1 to trap
		if(GT(kt,eloc2))
		{
			step_t estep2 = Board::STEPINDEX[kt][eloc2];
			if(ISE(eloc2 S)) {TP(kt,eloc2,eloc2 S,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MS,estep2),2)}
			if(ISE(eloc2 W)) {TP(kt,eloc2,eloc2 W,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MW,estep2),2)}
			if(ISE(eloc2 E)) {TP(kt,eloc2,eloc2 E,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 ME,estep2),2)}
			if(ISE(eloc2 N)) {TP(kt,eloc2,eloc2 N,int newnum = num + genPPTo(b,pla,eloc1,kt,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MN,estep2),2)}
		}
	}
	else if(ISO(kt))
	{
		//Remove eloc2, then remove eloc1
		if(ISP(eloc2 S) && GT(eloc2 S,eloc2) && b.isThawed(eloc2 S))
		{
			if(ISE(eloc2 W)) {TP(eloc2 S,eloc2,eloc2 W,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MW, eloc2 S MN),2)}
			if(ISE(eloc2 E)) {TP(eloc2 S,eloc2,eloc2 E,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 ME, eloc2 S MN),2)}
			if(ISE(eloc2 N)) {TP(eloc2 S,eloc2,eloc2 N,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MN, eloc2 S MN),2)}

			if(CS2(eloc2) && ISE(eloc2 SS)) {TP(eloc2,eloc2 S,eloc2 SS,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 S MS, eloc2 MS),2)}
			if(              ISE(eloc2 SW)) {TP(eloc2,eloc2 S,eloc2 SW,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 S MW, eloc2 MS),2)}
			if(              ISE(eloc2 SE)) {TP(eloc2,eloc2 S,eloc2 SE,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 S ME, eloc2 MS),2)}
		}
		if(ISP(eloc2 W) && GT(eloc2 W,eloc2) && b.isThawed(eloc2 W))
		{
			if(ISE(eloc2 S)) {TP(eloc2 W,eloc2,eloc2 S,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MS, eloc2 W ME),2)}
			if(ISE(eloc2 E)) {TP(eloc2 W,eloc2,eloc2 E,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 ME, eloc2 W ME),2)}
			if(ISE(eloc2 N)) {TP(eloc2 W,eloc2,eloc2 N,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MN, eloc2 W ME),2)}

			if(              ISE(eloc2 SW)) {TP(eloc2,eloc2 W,eloc2 SW,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 W MS, eloc2 MW),2)}
			if(CW2(eloc2) && ISE(eloc2 WW)) {TP(eloc2,eloc2 W,eloc2 WW,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 W MW, eloc2 MW),2)}
			if(              ISE(eloc2 NW)) {TP(eloc2,eloc2 W,eloc2 NW,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 W MN, eloc2 MW),2)}
		}
		if(ISP(eloc2 E) && GT(eloc2 E,eloc2) && b.isThawed(eloc2 E))
		{
			if(ISE(eloc2 S)) {TP(eloc2 E,eloc2,eloc2 S,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MS, eloc2 E MW),2)}
			if(ISE(eloc2 W)) {TP(eloc2 E,eloc2,eloc2 W,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MW, eloc2 E MW),2)}
			if(ISE(eloc2 N)) {TP(eloc2 E,eloc2,eloc2 N,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MN, eloc2 E MW),2)}

			if(              ISE(eloc2 SE)) {TP(eloc2,eloc2 E,eloc2 SE,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 E MS, eloc2 ME),2)}
			if(CE2(eloc2) && ISE(eloc2 EE)) {TP(eloc2,eloc2 E,eloc2 EE,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 E ME, eloc2 ME),2)}
			if(              ISE(eloc2 NE)) {TP(eloc2,eloc2 E,eloc2 NE,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 E MN, eloc2 ME),2)}
		}
		if(ISP(eloc2 N) && GT(eloc2 N,eloc2) && b.isThawed(eloc2 N))
		{
			if(ISE(eloc2 S)) {TP(eloc2 N,eloc2,eloc2 S,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MS, eloc2 N MS),2)}
			if(ISE(eloc2 W)) {TP(eloc2 N,eloc2,eloc2 W,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 MW, eloc2 N MS),2)}
			if(ISE(eloc2 E)) {TP(eloc2 N,eloc2,eloc2 E,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 ME, eloc2 N MS),2)}

			if(              ISE(eloc2 NW)) {TP(eloc2,eloc2 N,eloc2 NW,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 N MW, eloc2 MN),2)}
			if(              ISE(eloc2 NE)) {TP(eloc2,eloc2 N,eloc2 NE,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 N ME, eloc2 MN),2)}
			if(CN2(eloc2) && ISE(eloc2 NN)) {TP(eloc2,eloc2 N,eloc2 NN,int newnum = num + genPP(b,pla,eloc1,mv+num,hm+num,hmval)) PREFIXMOVES(Board::getMove(eloc2 N MN, eloc2 MN),2)}
		}
	}

	return num;
}

//4-STEP CAPTURES, 1 DEFENDER--------------------------------------------------------------

//Can we move a piece stronger than eloc to sloc in two steps? (As in, two steps away), and have it unfrozen?
//Assumes that sloc is not a trap.
//Does NOT use the C version of thawed or frozen!
static bool canMoveStrong2S(Board& b, pla_t pla, loc_t sloc, loc_t eloc)
{
	bool suc = false;

	if(CS1(sloc) && ISE(sloc S) && b.isTrapSafe2(pla,sloc S))
	{
		if(CS2(sloc) && ISP(sloc SS) && GT(sloc SS,eloc) && b.isThawed(sloc SS) && b.wouldBeUF(pla,sloc SS,sloc S,sloc SS)) {TSC(sloc SS,sloc S,suc=b.wouldBeUF(pla,sloc S,sloc,sloc S)) if(suc){return true;}}
		if(CW1(sloc) && ISP(sloc SW) && GT(sloc SW,eloc) && b.isThawed(sloc SW) && b.wouldBeUF(pla,sloc SW,sloc S,sloc SW)) {TSC(sloc SW,sloc S,suc=b.wouldBeUF(pla,sloc S,sloc,sloc S)) if(suc){return true;}}
		if(CE1(sloc) && ISP(sloc SE) && GT(sloc SE,eloc) && b.isThawed(sloc SE) && b.wouldBeUF(pla,sloc SE,sloc S,sloc SE)) {TSC(sloc SE,sloc S,suc=b.wouldBeUF(pla,sloc S,sloc,sloc S)) if(suc){return true;}}
	}
	if(CW1(sloc) && ISE(sloc W) && b.isTrapSafe2(pla,sloc W))
	{
		if(CS1(sloc) && ISP(sloc SW) && GT(sloc SW,eloc) && b.isThawed(sloc SW) && b.wouldBeUF(pla,sloc SW,sloc W,sloc SW)) {TSC(sloc SW,sloc W,suc=b.wouldBeUF(pla,sloc W,sloc,sloc W)) if(suc){return true;}}
		if(CW2(sloc) && ISP(sloc WW) && GT(sloc WW,eloc) && b.isThawed(sloc WW) && b.wouldBeUF(pla,sloc WW,sloc W,sloc WW)) {TSC(sloc WW,sloc W,suc=b.wouldBeUF(pla,sloc W,sloc,sloc W)) if(suc){return true;}}
		if(CN1(sloc) && ISP(sloc NW) && GT(sloc NW,eloc) && b.isThawed(sloc NW) && b.wouldBeUF(pla,sloc NW,sloc W,sloc NW)) {TSC(sloc NW,sloc W,suc=b.wouldBeUF(pla,sloc W,sloc,sloc W)) if(suc){return true;}}
	}
	if(CE1(sloc) && ISE(sloc E) && b.isTrapSafe2(pla,sloc E))
	{
		if(CS1(sloc) && ISP(sloc SE) && GT(sloc SE,eloc) && b.isThawed(sloc SE) && b.wouldBeUF(pla,sloc SE,sloc E,sloc SE)) {TSC(sloc SE,sloc E,suc=b.wouldBeUF(pla,sloc E,sloc,sloc E)) if(suc){return true;}}
		if(CE2(sloc) && ISP(sloc EE) && GT(sloc EE,eloc) && b.isThawed(sloc EE) && b.wouldBeUF(pla,sloc EE,sloc E,sloc EE)) {TSC(sloc EE,sloc E,suc=b.wouldBeUF(pla,sloc E,sloc,sloc E)) if(suc){return true;}}
		if(CN1(sloc) && ISP(sloc NE) && GT(sloc NE,eloc) && b.isThawed(sloc NE) && b.wouldBeUF(pla,sloc NE,sloc E,sloc NE)) {TSC(sloc NE,sloc E,suc=b.wouldBeUF(pla,sloc E,sloc,sloc E)) if(suc){return true;}}
	}
	if(CN1(sloc) && ISE(sloc N) && b.isTrapSafe2(pla,sloc N))
	{
		if(CW1(sloc) && ISP(sloc NW) && GT(sloc NW,eloc) && b.isThawed(sloc NW) && b.wouldBeUF(pla,sloc NW,sloc N,sloc NW)) {TSC(sloc NW,sloc N,suc=b.wouldBeUF(pla,sloc N,sloc,sloc N)) if(suc){return true;}}
		if(CE1(sloc) && ISP(sloc NE) && GT(sloc NE,eloc) && b.isThawed(sloc NE) && b.wouldBeUF(pla,sloc NE,sloc N,sloc NE)) {TSC(sloc NE,sloc N,suc=b.wouldBeUF(pla,sloc N,sloc,sloc N)) if(suc){return true;}}
		if(CN2(sloc) && ISP(sloc NN) && GT(sloc NN,eloc) && b.isThawed(sloc NN) && b.wouldBeUF(pla,sloc NN,sloc N,sloc NN)) {TSC(sloc NN,sloc N,suc=b.wouldBeUF(pla,sloc N,sloc,sloc N)) if(suc){return true;}}
	}

	return false;
}

//Can we move a piece stronger than eloc to sloc in two steps? (As in, two steps away), and have it unfrozen?
//Assumes that sloc is not a trap.
//Does NOT use the C version of thawed or frozen!
static int genMoveStrong2S(Board& b, pla_t pla, loc_t sloc, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	bool suc = false;

	//Suicide cases cannot count as valid unfreezing, Ex: If cat steps N then W, sacrificing rabbit, then will frozen by e.
	/*
	   .*.
	   .r.
	   e..
	   .RC
	*/

	if(CS1(sloc) && ISE(sloc S) && b.isTrapSafe2(pla,sloc S))
	{
		if(CS2(sloc) && ISP(sloc SS) && GT(sloc SS,eloc) && b.isThawed(sloc SS) && b.wouldBeUF(pla,sloc SS,sloc S,sloc SS)) {TSC(sloc SS,sloc S,suc=b.wouldBeUF(pla,sloc S,sloc,sloc S)) if(suc){ADDMOVE(Board::getMove(sloc SS MN),hmval)}}
		if(CW1(sloc) && ISP(sloc SW) && GT(sloc SW,eloc) && b.isThawed(sloc SW) && b.wouldBeUF(pla,sloc SW,sloc S,sloc SW)) {TSC(sloc SW,sloc S,suc=b.wouldBeUF(pla,sloc S,sloc,sloc S)) if(suc){ADDMOVE(Board::getMove(sloc SW ME),hmval)}}
		if(CE1(sloc) && ISP(sloc SE) && GT(sloc SE,eloc) && b.isThawed(sloc SE) && b.wouldBeUF(pla,sloc SE,sloc S,sloc SE)) {TSC(sloc SE,sloc S,suc=b.wouldBeUF(pla,sloc S,sloc,sloc S)) if(suc){ADDMOVE(Board::getMove(sloc SE MW),hmval)}}
	}
	if(CW1(sloc) && ISE(sloc W) && b.isTrapSafe2(pla,sloc W))
	{
		if(CS1(sloc) && ISP(sloc SW) && GT(sloc SW,eloc) && b.isThawed(sloc SW) && b.wouldBeUF(pla,sloc SW,sloc W,sloc SW)) {TSC(sloc SW,sloc W,suc=b.wouldBeUF(pla,sloc W,sloc,sloc W)) if(suc){ADDMOVE(Board::getMove(sloc SW MN),hmval)}}
		if(CW2(sloc) && ISP(sloc WW) && GT(sloc WW,eloc) && b.isThawed(sloc WW) && b.wouldBeUF(pla,sloc WW,sloc W,sloc WW)) {TSC(sloc WW,sloc W,suc=b.wouldBeUF(pla,sloc W,sloc,sloc W)) if(suc){ADDMOVE(Board::getMove(sloc WW ME),hmval)}}
		if(CN1(sloc) && ISP(sloc NW) && GT(sloc NW,eloc) && b.isThawed(sloc NW) && b.wouldBeUF(pla,sloc NW,sloc W,sloc NW)) {TSC(sloc NW,sloc W,suc=b.wouldBeUF(pla,sloc W,sloc,sloc W)) if(suc){ADDMOVE(Board::getMove(sloc NW MS),hmval)}}
	}
	if(CE1(sloc) && ISE(sloc E) && b.isTrapSafe2(pla,sloc E))
	{
		if(CS1(sloc) && ISP(sloc SE) && GT(sloc SE,eloc) && b.isThawed(sloc SE) && b.wouldBeUF(pla,sloc SE,sloc E,sloc SE)) {TSC(sloc SE,sloc E,suc=b.wouldBeUF(pla,sloc E,sloc,sloc E)) if(suc){ADDMOVE(Board::getMove(sloc SE MN),hmval)}}
		if(CE2(sloc) && ISP(sloc EE) && GT(sloc EE,eloc) && b.isThawed(sloc EE) && b.wouldBeUF(pla,sloc EE,sloc E,sloc EE)) {TSC(sloc EE,sloc E,suc=b.wouldBeUF(pla,sloc E,sloc,sloc E)) if(suc){ADDMOVE(Board::getMove(sloc EE MW),hmval)}}
		if(CN1(sloc) && ISP(sloc NE) && GT(sloc NE,eloc) && b.isThawed(sloc NE) && b.wouldBeUF(pla,sloc NE,sloc E,sloc NE)) {TSC(sloc NE,sloc E,suc=b.wouldBeUF(pla,sloc E,sloc,sloc E)) if(suc){ADDMOVE(Board::getMove(sloc NE MS),hmval)}}
	}
	if(CN1(sloc) && ISE(sloc N) && b.isTrapSafe2(pla,sloc N))
	{
		if(CW1(sloc) && ISP(sloc NW) && GT(sloc NW,eloc) && b.isThawed(sloc NW) && b.wouldBeUF(pla,sloc NW,sloc N,sloc NW)) {TSC(sloc NW,sloc N,suc=b.wouldBeUF(pla,sloc N,sloc,sloc N)) if(suc){ADDMOVE(Board::getMove(sloc NW ME),hmval)}}
		if(CE1(sloc) && ISP(sloc NE) && GT(sloc NE,eloc) && b.isThawed(sloc NE) && b.wouldBeUF(pla,sloc NE,sloc N,sloc NE)) {TSC(sloc NE,sloc N,suc=b.wouldBeUF(pla,sloc N,sloc,sloc N)) if(suc){ADDMOVE(Board::getMove(sloc NE MW),hmval)}}
		if(CN2(sloc) && ISP(sloc NN) && GT(sloc NN,eloc) && b.isThawed(sloc NN) && b.wouldBeUF(pla,sloc NN,sloc N,sloc NN)) {TSC(sloc NN,sloc N,suc=b.wouldBeUF(pla,sloc N,sloc,sloc N)) if(suc){ADDMOVE(Board::getMove(sloc NN MS),hmval)}}
	}

	return num;
}

//Can we swap out the player piece on sloc for a new piece in two steps, with the new piece UF and stronger than eloc?
static bool canSwapPla(Board& b, pla_t pla, loc_t sloc, loc_t eloc)
{
	if(CS1(sloc) && ISP(sloc S) && GT(sloc S,eloc) && b.wouldBeUF(pla,sloc S,sloc S,sloc) && b.isTrapSafe2(pla,sloc S))
	{
		if(CW1(sloc) && ISE(sloc W) &&                           (b.isTrapSafe2(pla,sloc W) || b.wouldBeUF(pla,sloc S,sloc,sloc S))) {return true;}
		if(CE1(sloc) && ISE(sloc E) &&                           (b.isTrapSafe2(pla,sloc E) || b.wouldBeUF(pla,sloc S,sloc,sloc S))) {return true;}
		if(CN1(sloc) && ISE(sloc N) && b.isRabOkayN(pla,sloc) && (b.isTrapSafe2(pla,sloc N) || b.wouldBeUF(pla,sloc S,sloc,sloc S))) {return true;}
	}
	if(CW1(sloc) && ISP(sloc W) && GT(sloc W,eloc) && b.wouldBeUF(pla,sloc W,sloc W,sloc) && b.isTrapSafe2(pla,sloc W))
	{
		if(CS1(sloc) && ISE(sloc S) && b.isRabOkayS(pla,sloc) && (b.isTrapSafe2(pla,sloc S) || b.wouldBeUF(pla,sloc W,sloc,sloc W))) {return true;}
		if(CE1(sloc) && ISE(sloc E) &&                           (b.isTrapSafe2(pla,sloc E) || b.wouldBeUF(pla,sloc W,sloc,sloc W))) {return true;}
		if(CN1(sloc) && ISE(sloc N) && b.isRabOkayN(pla,sloc) && (b.isTrapSafe2(pla,sloc N) || b.wouldBeUF(pla,sloc W,sloc,sloc W))) {return true;}
	}
	if(CE1(sloc) && ISP(sloc E) && GT(sloc E,eloc) && b.wouldBeUF(pla,sloc E,sloc E,sloc) && b.isTrapSafe2(pla,sloc E))
	{
		if(CS1(sloc) && ISE(sloc S) && b.isRabOkayS(pla,sloc) && (b.isTrapSafe2(pla,sloc S) || b.wouldBeUF(pla,sloc E,sloc,sloc E))) {return true;}
		if(CW1(sloc) && ISE(sloc W) &&                           (b.isTrapSafe2(pla,sloc W) || b.wouldBeUF(pla,sloc E,sloc,sloc E))) {return true;}
		if(CN1(sloc) && ISE(sloc N) && b.isRabOkayN(pla,sloc) && (b.isTrapSafe2(pla,sloc N) || b.wouldBeUF(pla,sloc E,sloc,sloc E))) {return true;}
	}
	if(CN1(sloc) && ISP(sloc N) && GT(sloc N,eloc) && b.wouldBeUF(pla,sloc N,sloc N,sloc) && b.isTrapSafe2(pla,sloc N))
	{
		if(CS1(sloc) && ISE(sloc S) && b.isRabOkayS(pla,sloc) && (b.isTrapSafe2(pla,sloc S) || b.wouldBeUF(pla,sloc N,sloc,sloc N))) {return true;}
		if(CW1(sloc) && ISE(sloc W) &&                           (b.isTrapSafe2(pla,sloc W) || b.wouldBeUF(pla,sloc N,sloc,sloc N))) {return true;}
		if(CE1(sloc) && ISE(sloc E) &&                           (b.isTrapSafe2(pla,sloc E) || b.wouldBeUF(pla,sloc N,sloc,sloc N))) {return true;}
	}
	return false;
}

//Can we swap out the player piece on sloc for a new piece in two steps, with the new piece UF and stronger than eloc?
static int genSwapPla(Board& b, pla_t pla, loc_t sloc, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;

	if(CS1(sloc) && ISP(sloc S) && GT(sloc S,eloc) && b.wouldBeUF(pla,sloc S,sloc S,sloc) && b.isTrapSafe2(pla,sloc S))
	{
		if(CW1(sloc) && ISE(sloc W) &&                           (b.isTrapSafe2(pla,sloc W) || b.wouldBeUF(pla,sloc S,sloc,sloc S))) {ADDMOVE(Board::getMove(sloc MW),hmval)}
		if(CE1(sloc) && ISE(sloc E) &&                           (b.isTrapSafe2(pla,sloc E) || b.wouldBeUF(pla,sloc S,sloc,sloc S))) {ADDMOVE(Board::getMove(sloc ME),hmval)}
		if(CN1(sloc) && ISE(sloc N) && b.isRabOkayN(pla,sloc) && (b.isTrapSafe2(pla,sloc N) || b.wouldBeUF(pla,sloc S,sloc,sloc S))) {ADDMOVE(Board::getMove(sloc MN),hmval)}
	}
	if(CW1(sloc) && ISP(sloc W) && GT(sloc W,eloc) && b.wouldBeUF(pla,sloc W,sloc W,sloc) && b.isTrapSafe2(pla,sloc W))
	{
		if(CS1(sloc) && ISE(sloc S) && b.isRabOkayS(pla,sloc) && (b.isTrapSafe2(pla,sloc S) || b.wouldBeUF(pla,sloc W,sloc,sloc W))) {ADDMOVE(Board::getMove(sloc MS),hmval)}
		if(CE1(sloc) && ISE(sloc E) &&                           (b.isTrapSafe2(pla,sloc E) || b.wouldBeUF(pla,sloc W,sloc,sloc W))) {ADDMOVE(Board::getMove(sloc ME),hmval)}
		if(CN1(sloc) && ISE(sloc N) && b.isRabOkayN(pla,sloc) && (b.isTrapSafe2(pla,sloc N) || b.wouldBeUF(pla,sloc W,sloc,sloc W))) {ADDMOVE(Board::getMove(sloc MN),hmval)}
	}
	if(CE1(sloc) && ISP(sloc E) && GT(sloc E,eloc) && b.wouldBeUF(pla,sloc E,sloc E,sloc) && b.isTrapSafe2(pla,sloc E))
	{
		if(CS1(sloc) && ISE(sloc S) && b.isRabOkayS(pla,sloc) && (b.isTrapSafe2(pla,sloc S) || b.wouldBeUF(pla,sloc E,sloc,sloc E))) {ADDMOVE(Board::getMove(sloc MS),hmval)}
		if(CW1(sloc) && ISE(sloc W) &&                           (b.isTrapSafe2(pla,sloc W) || b.wouldBeUF(pla,sloc E,sloc,sloc E))) {ADDMOVE(Board::getMove(sloc MW),hmval)}
		if(CN1(sloc) && ISE(sloc N) && b.isRabOkayN(pla,sloc) && (b.isTrapSafe2(pla,sloc N) || b.wouldBeUF(pla,sloc E,sloc,sloc E))) {ADDMOVE(Board::getMove(sloc MN),hmval)}
	}
	if(CN1(sloc) && ISP(sloc N) && GT(sloc N,eloc) && b.wouldBeUF(pla,sloc N,sloc N,sloc) && b.isTrapSafe2(pla,sloc N))
	{
		if(CS1(sloc) && ISE(sloc S) && b.isRabOkayS(pla,sloc) && (b.isTrapSafe2(pla,sloc S) || b.wouldBeUF(pla,sloc N,sloc,sloc N))) {ADDMOVE(Board::getMove(sloc MS),hmval)}
		if(CW1(sloc) && ISE(sloc W) &&                           (b.isTrapSafe2(pla,sloc W) || b.wouldBeUF(pla,sloc N,sloc,sloc N))) {ADDMOVE(Board::getMove(sloc MW),hmval)}
		if(CE1(sloc) && ISE(sloc E) &&                           (b.isTrapSafe2(pla,sloc E) || b.wouldBeUF(pla,sloc N,sloc,sloc N))) {ADDMOVE(Board::getMove(sloc ME),hmval)}
	}
	return num;
}

//Can we swap out the opponent piece on sloc for a new pla piece in two steps, with the new piece UF and stronger than eloc?
//Assumes sloc cannot be a trap.
//Does NOT use the C version of thawed or frozen!
static bool canSwapOpp(Board& b, pla_t pla, loc_t sloc, loc_t eloc, loc_t kt)
{
	pla_t opp = OPP(pla);
	loc_t ignloc = ERRORSQUARE;
	if     (CS1(sloc) && ISO(sloc S) && !b.isTrapSafe2(opp,sloc S)) {ignloc = sloc S;}
	else if(CW1(sloc) && ISO(sloc W) && !b.isTrapSafe2(opp,sloc W)) {ignloc = sloc W;}
	else if(CE1(sloc) && ISO(sloc E) && !b.isTrapSafe2(opp,sloc E)) {ignloc = sloc E;}
	else if(CN1(sloc) && ISO(sloc N) && !b.isTrapSafe2(opp,sloc N)) {ignloc = sloc N;}

	if(CS1(sloc) && ISP(sloc S) && GT(sloc S,eloc) && GT(sloc S, sloc) && b.isThawed(sloc S) && b.wouldBeUF(pla,sloc S,sloc,sloc S,ignloc))
	{
		if(CW1(sloc) && ISE(sloc W) && !b.isAdjacent(kt,sloc W)) {return true;}
		if(CE1(sloc) && ISE(sloc E) && !b.isAdjacent(kt,sloc E)) {return true;}
		if(CN1(sloc) && ISE(sloc N) && !b.isAdjacent(kt,sloc N)) {return true;}
	}
	if(CW1(sloc) && ISP(sloc W) && GT(sloc W,eloc) && GT(sloc W, sloc) && b.isThawed(sloc W) && b.wouldBeUF(pla,sloc W,sloc,sloc W,ignloc))
	{
		if(CS1(sloc) && ISE(sloc S) && !b.isAdjacent(kt,sloc S)) {return true;}
		if(CE1(sloc) && ISE(sloc E) && !b.isAdjacent(kt,sloc E)) {return true;}
		if(CN1(sloc) && ISE(sloc N) && !b.isAdjacent(kt,sloc N)) {return true;}
	}
	if(CE1(sloc) && ISP(sloc E) && GT(sloc E,eloc) && GT(sloc E, sloc) && b.isThawed(sloc E) && b.wouldBeUF(pla,sloc E,sloc,sloc E,ignloc))
	{
		if(CS1(sloc) && ISE(sloc S) && !b.isAdjacent(kt,sloc S)) {return true;}
		if(CW1(sloc) && ISE(sloc W) && !b.isAdjacent(kt,sloc W)) {return true;}
		if(CN1(sloc) && ISE(sloc N) && !b.isAdjacent(kt,sloc N)) {return true;}
	}
	if(CN1(sloc) && ISP(sloc N) && GT(sloc N,eloc) && GT(sloc N, sloc) && b.isThawed(sloc N) && b.wouldBeUF(pla,sloc N,sloc,sloc N,ignloc))
	{
		if(CS1(sloc) && ISE(sloc S) && !b.isAdjacent(kt,sloc S)) {return true;}
		if(CW1(sloc) && ISE(sloc W) && !b.isAdjacent(kt,sloc W)) {return true;}
		if(CE1(sloc) && ISE(sloc E) && !b.isAdjacent(kt,sloc E)) {return true;}
	}
	return false;
}

//Can we swap out the opponent piece on sloc for a new pla piece in two steps, with the new piece UF and stronger than eloc?
//Assumes sloc cannot be a trap.
//Does not push any opp on to a square adjacent to the given trap
//Does NOT use the C version of thawed or frozen!
static int genSwapOpp(Board& b, pla_t pla, loc_t sloc, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	pla_t opp = OPP(pla);
	loc_t ignloc = ERRORSQUARE;
	if     (CS1(sloc) && ISO(sloc S) && !b.isTrapSafe2(opp,sloc S)) {ignloc = sloc S;}
	else if(CW1(sloc) && ISO(sloc W) && !b.isTrapSafe2(opp,sloc W)) {ignloc = sloc W;}
	else if(CE1(sloc) && ISO(sloc E) && !b.isTrapSafe2(opp,sloc E)) {ignloc = sloc E;}
	else if(CN1(sloc) && ISO(sloc N) && !b.isTrapSafe2(opp,sloc N)) {ignloc = sloc N;}

	if(CS1(sloc) && ISP(sloc S) && GT(sloc S,eloc) && GT(sloc S, sloc) && b.isThawed(sloc S) && b.wouldBeUF(pla,sloc S,sloc,sloc S,ignloc))
	{
		if(CW1(sloc) && ISE(sloc W) && !b.isAdjacent(kt,sloc W)) {ADDMOVE(Board::getMove(sloc MW,sloc S MN),hmval)}
		if(CE1(sloc) && ISE(sloc E) && !b.isAdjacent(kt,sloc E)) {ADDMOVE(Board::getMove(sloc ME,sloc S MN),hmval)}
		if(CN1(sloc) && ISE(sloc N) && !b.isAdjacent(kt,sloc N)) {ADDMOVE(Board::getMove(sloc MN,sloc S MN),hmval)}
	}
	if(CW1(sloc) && ISP(sloc W) && GT(sloc W,eloc) && GT(sloc W, sloc) && b.isThawed(sloc W) && b.wouldBeUF(pla,sloc W,sloc,sloc W,ignloc))
	{
		if(CS1(sloc) && ISE(sloc S) && !b.isAdjacent(kt,sloc S)) {ADDMOVE(Board::getMove(sloc MS,sloc W ME),hmval)}
		if(CE1(sloc) && ISE(sloc E) && !b.isAdjacent(kt,sloc E)) {ADDMOVE(Board::getMove(sloc ME,sloc W ME),hmval)}
		if(CN1(sloc) && ISE(sloc N) && !b.isAdjacent(kt,sloc N)) {ADDMOVE(Board::getMove(sloc MN,sloc W ME),hmval)}
	}
	if(CE1(sloc) && ISP(sloc E) && GT(sloc E,eloc) && GT(sloc E, sloc) && b.isThawed(sloc E) && b.wouldBeUF(pla,sloc E,sloc,sloc E,ignloc))
	{
		if(CS1(sloc) && ISE(sloc S) && !b.isAdjacent(kt,sloc S)) {ADDMOVE(Board::getMove(sloc MS,sloc E MW),hmval)}
		if(CW1(sloc) && ISE(sloc W) && !b.isAdjacent(kt,sloc W)) {ADDMOVE(Board::getMove(sloc MW,sloc E MW),hmval)}
		if(CN1(sloc) && ISE(sloc N) && !b.isAdjacent(kt,sloc N)) {ADDMOVE(Board::getMove(sloc MN,sloc E MW),hmval)}
	}
	if(CN1(sloc) && ISP(sloc N) && GT(sloc N,eloc) && GT(sloc N, sloc) && b.isThawed(sloc N) && b.wouldBeUF(pla,sloc N,sloc,sloc N,ignloc))
	{
		if(CS1(sloc) && ISE(sloc S) && !b.isAdjacent(kt,sloc S)) {ADDMOVE(Board::getMove(sloc MS,sloc N MS),hmval)}
		if(CW1(sloc) && ISE(sloc W) && !b.isAdjacent(kt,sloc W)) {ADDMOVE(Board::getMove(sloc MW,sloc N MS),hmval)}
		if(CE1(sloc) && ISE(sloc E) && !b.isAdjacent(kt,sloc E)) {ADDMOVE(Board::getMove(sloc ME,sloc N MS),hmval)}
	}
	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canPPIntoTrapTE4C(Board& b, pla_t pla, loc_t eloc, loc_t kt)
{
	bool suc = false;
	pla_t opp = OPP(pla);
	int meNum = ISP(kt S) + ISP(kt W) + ISP(kt E) + ISP(kt N);

	//Frozen but can push
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isFrozen(eloc S) && canUF2(b,pla,eloc S)) {return true;}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isFrozen(eloc W) && canUF2(b,pla,eloc W)) {return true;}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isFrozen(eloc E) && canUF2(b,pla,eloc E)) {return true;}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isFrozen(eloc N) && canUF2(b,pla,eloc N)) {return true;}

	//A step away, not going through the trap. Either must be frozen now, or be frozen after one step.***
	if(CS1(eloc) && ISE(eloc S) && eloc S != kt)
	{
		if     (CS2(eloc) && ISP(eloc SS) && GT(eloc SS,eloc) && b.isThawed(eloc SS)) {TS(eloc SS,eloc S,suc = canUF(b,pla,eloc S)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SS,eloc S)) {return true;}}
		else if(CS2(eloc) && ISP(eloc SS) && GT(eloc SS,eloc) && b.wouldBeUF(pla,eloc SS,eloc S,eloc SS) && canUFStepWouldBeUF(b,pla,eloc SS,eloc S,kt)) {return true;}

		if     (CW1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.isThawed(eloc SW)) {TS(eloc SW,eloc S,suc = canUF(b,pla,eloc S)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SW,eloc S)) {return true;} if(canAdvancePullCap(b,pla,eloc SW,eloc S)) {return true;}}
		else if(CW1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.wouldBeUF(pla,eloc SW,eloc S,eloc SW) && canUFStepWouldBeUF(b,pla,eloc SW,eloc S,kt)) {return true;}

		if     (CE1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.isThawed(eloc SE)) {TS(eloc SE,eloc S,suc = canUF(b,pla,eloc S)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SE,eloc S)) {return true;} if(canAdvancePullCap(b,pla,eloc SE,eloc S)) {return true;}}
		else if(CE1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.wouldBeUF(pla,eloc SE,eloc S,eloc SE) && canUFStepWouldBeUF(b,pla,eloc SE,eloc S,kt)) {return true;}
	}
	if(CW1(eloc) && ISE(eloc W) && eloc W != kt)
	{
		if     (CS1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.isThawed(eloc SW)) {TS(eloc SW,eloc W,suc = canUF(b,pla,eloc W)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SW,eloc W)) {return true;} if(canAdvancePullCap(b,pla,eloc SW,eloc W)) {return true;}}
		else if(CS1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.wouldBeUF(pla,eloc SW,eloc W,eloc SW) && canUFStepWouldBeUF(b,pla,eloc SW,eloc W,kt)) {return true;}

		if     (CW2(eloc) && ISP(eloc WW) && GT(eloc WW,eloc) && b.isThawed(eloc WW)) {TS(eloc WW,eloc W,suc = canUF(b,pla,eloc W)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc WW,eloc W)) {return true;}}
		else if(CW2(eloc) && ISP(eloc WW) && GT(eloc WW,eloc) && b.wouldBeUF(pla,eloc WW,eloc W,eloc WW) && canUFStepWouldBeUF(b,pla,eloc WW,eloc W,kt)) {return true;}

		if     (CN1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.isThawed(eloc NW)) {TS(eloc NW,eloc W,suc = canUF(b,pla,eloc W)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NW,eloc W)) {return true;} if(canAdvancePullCap(b,pla,eloc NW,eloc W)) {return true;}}
		else if(CN1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.wouldBeUF(pla,eloc NW,eloc W,eloc NW) && canUFStepWouldBeUF(b,pla,eloc NW,eloc W,kt)) {return true;}
	}
	if(CE1(eloc) && ISE(eloc E) && eloc E != kt)
	{
		if     (CS1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.isThawed(eloc SE)) {TS(eloc SE,eloc E,suc = canUF(b,pla,eloc E)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SE,eloc E)) {return true;} if(canAdvancePullCap(b,pla,eloc SE,eloc E)) {return true;}}
		else if(CS1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.wouldBeUF(pla,eloc SE,eloc E,eloc SE) && canUFStepWouldBeUF(b,pla,eloc SE,eloc E,kt)) {return true;}

		if     (CE2(eloc) && ISP(eloc EE) && GT(eloc EE,eloc) && b.isThawed(eloc EE)) {TS(eloc EE,eloc E,suc = canUF(b,pla,eloc E)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc EE,eloc E)) {return true;}}
		else if(CE2(eloc) && ISP(eloc EE) && GT(eloc EE,eloc) && b.wouldBeUF(pla,eloc EE,eloc E,eloc EE) && canUFStepWouldBeUF(b,pla,eloc EE,eloc E,kt)) {return true;}

		if     (CN1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.isThawed(eloc NE)) {TS(eloc NE,eloc E,suc = canUF(b,pla,eloc E)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NE,eloc E)) {return true;} if(canAdvancePullCap(b,pla,eloc NE,eloc E)) {return true;}}
		else if(CN1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.wouldBeUF(pla,eloc NE,eloc E,eloc NE) && canUFStepWouldBeUF(b,pla,eloc NE,eloc E,kt)) {return true;}
	}
	if(CN1(eloc) && ISE(eloc N) && eloc N != kt)
	{
		if     (CW1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.isThawed(eloc NW)) {TS(eloc NW,eloc N,suc = canUF(b,pla,eloc N)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NW,eloc N)) {return true;} if(canAdvancePullCap(b,pla,eloc NW,eloc N)) {return true;}}
		else if(CW1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.wouldBeUF(pla,eloc NW,eloc N,eloc NW) && canUFStepWouldBeUF(b,pla,eloc NW,eloc N,kt)) {return true;}

		if     (CE1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.isThawed(eloc NE)) {TS(eloc NE,eloc N,suc = canUF(b,pla,eloc N)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NE,eloc N)) {return true;} if(canAdvancePullCap(b,pla,eloc NE,eloc N)) {return true;}}
		else if(CE1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.wouldBeUF(pla,eloc NE,eloc N,eloc NE) && canUFStepWouldBeUF(b,pla,eloc NE,eloc N,kt)) {return true;}

		if     (CN2(eloc) && ISP(eloc NN) && GT(eloc NN,eloc) && b.isThawed(eloc NN)) {TS(eloc NN,eloc N,suc = canUF(b,pla,eloc N)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NN,eloc N)) {return true;}}
		else if(CN2(eloc) && ISP(eloc NN) && GT(eloc NN,eloc) && b.wouldBeUF(pla,eloc NN,eloc N,eloc NN) && canUFStepWouldBeUF(b,pla,eloc NN,eloc N,kt)) {return true;}
	}

	//Friendly piece in the way
	if(CS1(eloc) && ISP(eloc S) && canSwapPla(b,pla,eloc S,eloc)) {return true;}
	if(CW1(eloc) && ISP(eloc W) && canSwapPla(b,pla,eloc W,eloc)) {return true;}
	if(CE1(eloc) && ISP(eloc E) && canSwapPla(b,pla,eloc E,eloc)) {return true;}
	if(CN1(eloc) && ISP(eloc N) && canSwapPla(b,pla,eloc N,eloc)) {return true;}

	//Opponent's piece in the way
	if(CS1(eloc) && ISO(eloc S) && canSwapOpp(b,pla,eloc S,eloc,kt)) {return true;}
	if(CW1(eloc) && ISO(eloc W) && canSwapOpp(b,pla,eloc W,eloc,kt)) {return true;}
	if(CE1(eloc) && ISO(eloc E) && canSwapOpp(b,pla,eloc E,eloc,kt)) {return true;}
	if(CN1(eloc) && ISO(eloc N) && canSwapOpp(b,pla,eloc N,eloc,kt)) {return true;}

	//A step away, going through the trap.
	//Trap is very safe. Can just unfreeze our piece.
	if(meNum >= 3)
	{
		b.owners[kt] = opp; b.pieces[kt] = RAB;
		if(ISP(kt S) && kt S != eloc && GT(kt S,eloc) && b.isFrozenC(kt S) && canUF(b,pla,kt S)) {b.owners[kt] = NPLA; b.pieces[kt] = EMP; return true;}
		if(ISP(kt W) && kt W != eloc && GT(kt W,eloc) && b.isFrozenC(kt W) && canUF(b,pla,kt W)) {b.owners[kt] = NPLA; b.pieces[kt] = EMP; return true;}
		if(ISP(kt E) && kt E != eloc && GT(kt E,eloc) && b.isFrozenC(kt E) && canUF(b,pla,kt E)) {b.owners[kt] = NPLA; b.pieces[kt] = EMP; return true;}
		if(ISP(kt N) && kt N != eloc && GT(kt N,eloc) && b.isFrozenC(kt N) && canUF(b,pla,kt N)) {b.owners[kt] = NPLA; b.pieces[kt] = EMP; return true;}
		b.owners[kt] = NPLA; b.pieces[kt] = EMP;
	}
	//Trap is safe. Can just unfreeze our piece, but make sure not to step away the trap-guarding piece.
	else if(meNum >= 2)
	{
		//Do some trickery; temporarily remove the piece so it isn't considrered by genUF
		if(ISP(kt S) && GT(kt S,eloc) && b.isFrozen(kt S))
		{
			if     (ISP(kt W)) {TR(kt W, suc = canUF(b,pla,kt S)) if(suc) {return true;}}
			else if(ISP(kt E)) {TR(kt E, suc = canUF(b,pla,kt S)) if(suc) {return true;}}
			else if(ISP(kt N)) {TR(kt N, suc = canUF(b,pla,kt S)) if(suc) {return true;}}
		}
		if(ISP(kt W) && GT(kt W,eloc) && b.isFrozen(kt W))
		{
			if     (ISP(kt S)) {TR(kt S, suc = canUF(b,pla,kt W)) if(suc) {return true;}}
			else if(ISP(kt E)) {TR(kt E, suc = canUF(b,pla,kt W)) if(suc) {return true;}}
			else if(ISP(kt N)) {TR(kt N, suc = canUF(b,pla,kt W)) if(suc) {return true;}}
		}
		if(ISP(kt E) && GT(kt E,eloc) && b.isFrozen(kt E))
		{
			if     (ISP(kt S)) {TR(kt S, suc = canUF(b,pla,kt E)) if(suc) {return true;}}
			else if(ISP(kt W)) {TR(kt W, suc = canUF(b,pla,kt E)) if(suc) {return true;}}
			else if(ISP(kt N)) {TR(kt N, suc = canUF(b,pla,kt E)) if(suc) {return true;}}
		}
		if(ISP(kt N) && GT(kt N,eloc) && b.isFrozen(kt N))
		{
			if     (ISP(kt S)) {TR(kt S, suc = canUF(b,pla,kt N)) if(suc) {return true;}}
			else if(ISP(kt W)) {TR(kt W, suc = canUF(b,pla,kt N)) if(suc) {return true;}}
			else if(ISP(kt E)) {TR(kt E, suc = canUF(b,pla,kt N)) if(suc) {return true;}}
		}
	}
	//The trap is unsafe, so make it safe
	else
	{
		//Locate the player next to the trap and its side-adjacent defender, if any
		int ploc = ERRORSQUARE;
		int guardloc = ERRORSQUARE;
		if     (ISP(kt S) && GT(kt S,eloc) && b.isThawed(kt S)) {ploc = kt S; if(ISP(kt SW)) {guardloc = kt SW;} else if(ISP(kt SE)) {guardloc = kt SE;}}
		else if(ISP(kt W) && GT(kt W,eloc) && b.isThawed(kt W)) {ploc = kt W; if(ISP(kt SW)) {guardloc = kt SW;} else if(ISP(kt NW)) {guardloc = kt NW;}}
		else if(ISP(kt E) && GT(kt E,eloc) && b.isThawed(kt E)) {ploc = kt E; if(ISP(kt SE)) {guardloc = kt SE;} else if(ISP(kt NE)) {guardloc = kt NE;}}
		else if(ISP(kt N) && GT(kt N,eloc) && b.isThawed(kt N)) {ploc = kt N; if(ISP(kt NW)) {guardloc = kt NW;} else if(ISP(kt NE)) {guardloc = kt NE;}}

		//If there's a strong enough player
		if(ploc != ERRORSQUARE)
		{
			//Player is very unfrozen, just make the trap safe any way.
			if(!b.isDominated(ploc) || b.isGuarded2(pla, ploc))
			{if(canUF(b,pla,kt)) {return true;}}
			//Player dominated and only one defender
			else
			{
				//No side adjacent defender to worry about, make the trap safe
				if(guardloc == ERRORSQUARE)
				{if(canUF(b,pla,kt)) {return true;}}
				//Temporarily remove the piece so genKTSafe won't use it to guard the trap.
				else
				{TR(guardloc,suc = canUF(b,pla,kt)) if(suc){return true;}}
			}
		}
	}

	//2 Steps away
	if(ISE(eloc S) && eloc S != kt && canMoveStrong2S(b,pla,eloc S,eloc)) {return true;}
	if(ISE(eloc W) && eloc W != kt && canMoveStrong2S(b,pla,eloc W,eloc)) {return true;}
	if(ISE(eloc E) && eloc E != kt && canMoveStrong2S(b,pla,eloc E,eloc)) {return true;}
	if(ISE(eloc N) && eloc N != kt && canMoveStrong2S(b,pla,eloc N,eloc)) {return true;}

	//2 Steps away, going through the trap
	//The trap is empty, but must be safe for this to work.
	if(meNum >= 1 && canMoveStrongAdjCent(b,pla, kt, eloc, ERRORSQUARE))
	{return true;}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genPPIntoTrapTE4C(Board& b, pla_t pla, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	bool suc = false;
	int num = 0;
	pla_t opp = OPP(pla);
	int meNum = ISP(kt S) + ISP(kt W) + ISP(kt E) + ISP(kt N);

	//Frozen but can push
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isFrozen(eloc S)) {num += genUF2(b,pla,eloc S,mv+num,hm+num,hmval);}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isFrozen(eloc W)) {num += genUF2(b,pla,eloc W,mv+num,hm+num,hmval);}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isFrozen(eloc E)) {num += genUF2(b,pla,eloc E,mv+num,hm+num,hmval);}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isFrozen(eloc N)) {num += genUF2(b,pla,eloc N,mv+num,hm+num,hmval);}

	//A step away, not going through the trap. Either must be frozen now, or be frozen after one step.***
	if(CS1(eloc) && ISE(eloc S) && eloc S != kt)
	{
		if     (CS2(eloc) && ISP(eloc SS) && GT(eloc SS,eloc) && b.isThawed(eloc SS)) {TS(eloc SS,eloc S,suc = canUF(b,pla,eloc S)) if(suc) {ADDMOVE(Board::getMove(eloc SS MN),hmval)} num += genAdvanceUFStep(b,pla,eloc SS,eloc S,mv+num,hm+num,hmval);}
		else if(CS2(eloc) && ISP(eloc SS) && GT(eloc SS,eloc) && b.wouldBeUF(pla,eloc SS,eloc S,eloc SS)) {num += genUFStepWouldBeUF(b,pla,eloc SS,eloc S,kt,mv+num,hm+num,hmval);}

		if     (CW1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.isThawed(eloc SW)) {TS(eloc SW,eloc S,suc = canUF(b,pla,eloc S)) if(suc) {ADDMOVE(Board::getMove(eloc SW ME),hmval)} num += genAdvanceUFStep(b,pla,eloc SW,eloc S,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc SW,eloc S,mv+num,hm+num,hmval);}
		else if(CW1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.wouldBeUF(pla,eloc SW,eloc S,eloc SW)) {num += genUFStepWouldBeUF(b,pla,eloc SW,eloc S,kt,mv+num,hm+num,hmval);}

		if     (CE1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.isThawed(eloc SE)) {TS(eloc SE,eloc S,suc = canUF(b,pla,eloc S)) if(suc) {ADDMOVE(Board::getMove(eloc SE MW),hmval)} num += genAdvanceUFStep(b,pla,eloc SE,eloc S,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc SE,eloc S,mv+num,hm+num,hmval);}
		else if(CE1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.wouldBeUF(pla,eloc SE,eloc S,eloc SE)) {num += genUFStepWouldBeUF(b,pla,eloc SE,eloc S,kt,mv+num,hm+num,hmval);}
	}
	if(CW1(eloc) && ISE(eloc W) && eloc W != kt)
	{
		if     (CS1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.isThawed(eloc SW)) {TS(eloc SW,eloc W,suc = canUF(b,pla,eloc W)) if(suc) {ADDMOVE(Board::getMove(eloc SW MN),hmval)} num += genAdvanceUFStep(b,pla,eloc SW,eloc W,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc SW,eloc W,mv+num,hm+num,hmval);}
		else if(CS1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.wouldBeUF(pla,eloc SW,eloc W,eloc SW)) {num += genUFStepWouldBeUF(b,pla,eloc SW,eloc W,kt,mv+num,hm+num,hmval);}

		if     (CW2(eloc) && ISP(eloc WW) && GT(eloc WW,eloc) && b.isThawed(eloc WW)) {TS(eloc WW,eloc W,suc = canUF(b,pla,eloc W)) if(suc) {ADDMOVE(Board::getMove(eloc WW ME),hmval)} num += genAdvanceUFStep(b,pla,eloc WW,eloc W,mv+num,hm+num,hmval);}
		else if(CW2(eloc) && ISP(eloc WW) && GT(eloc WW,eloc) && b.wouldBeUF(pla,eloc WW,eloc W,eloc WW)) {num += genUFStepWouldBeUF(b,pla,eloc WW,eloc W,kt,mv+num,hm+num,hmval);}

		if     (CN1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.isThawed(eloc NW)) {TS(eloc NW,eloc W,suc = canUF(b,pla,eloc W)) if(suc) {ADDMOVE(Board::getMove(eloc NW MS),hmval)} num += genAdvanceUFStep(b,pla,eloc NW,eloc W,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc NW,eloc W,mv+num,hm+num,hmval);}
		else if(CN1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.wouldBeUF(pla,eloc NW,eloc W,eloc NW)) {num += genUFStepWouldBeUF(b,pla,eloc NW,eloc W,kt,mv+num,hm+num,hmval);}
	}
	if(CE1(eloc) && ISE(eloc E) && eloc E != kt)
	{
		if     (CS1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.isThawed(eloc SE)) {TS(eloc SE,eloc E,suc = canUF(b,pla,eloc E)) if(suc) {ADDMOVE(Board::getMove(eloc SE MN),hmval)} num += genAdvanceUFStep(b,pla,eloc SE,eloc E,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc SE,eloc E,mv+num,hm+num,hmval);}
		else if(CS1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.wouldBeUF(pla,eloc SE,eloc E,eloc SE)) {num += genUFStepWouldBeUF(b,pla,eloc SE,eloc E,kt,mv+num,hm+num,hmval);}

		if     (CE2(eloc) && ISP(eloc EE) && GT(eloc EE,eloc) && b.isThawed(eloc EE)) {TS(eloc EE,eloc E,suc = canUF(b,pla,eloc E)) if(suc) {ADDMOVE(Board::getMove(eloc EE MW),hmval)} num += genAdvanceUFStep(b,pla,eloc EE,eloc E,mv+num,hm+num,hmval);}
		else if(CE2(eloc) && ISP(eloc EE) && GT(eloc EE,eloc) && b.wouldBeUF(pla,eloc EE,eloc E,eloc EE)) {num += genUFStepWouldBeUF(b,pla,eloc EE,eloc E,kt,mv+num,hm+num,hmval);}

		if     (CN1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.isThawed(eloc NE)) {TS(eloc NE,eloc E,suc = canUF(b,pla,eloc E)) if(suc) {ADDMOVE(Board::getMove(eloc NE MS),hmval)} num += genAdvanceUFStep(b,pla,eloc NE,eloc E,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc NE,eloc E,mv+num,hm+num,hmval);}
		else if(CN1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.wouldBeUF(pla,eloc NE,eloc E,eloc NE)) {num += genUFStepWouldBeUF(b,pla,eloc NE,eloc E,kt,mv+num,hm+num,hmval);}
	}
	if(CN1(eloc) && ISE(eloc N) && eloc N != kt)
	{
		if     (CW1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.isThawed(eloc NW)) {TS(eloc NW,eloc N,suc = canUF(b,pla,eloc N)) if(suc) {ADDMOVE(Board::getMove(eloc NW ME),hmval)} num += genAdvanceUFStep(b,pla,eloc NW,eloc N,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc NW,eloc N,mv+num,hm+num,hmval);}
		else if(CW1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.wouldBeUF(pla,eloc NW,eloc N,eloc NW)) {num += genUFStepWouldBeUF(b,pla,eloc NW,eloc N,kt,mv+num,hm+num,hmval);}

		if     (CE1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.isThawed(eloc NE)) {TS(eloc NE,eloc N,suc = canUF(b,pla,eloc N)) if(suc) {ADDMOVE(Board::getMove(eloc NE MW),hmval)} num += genAdvanceUFStep(b,pla,eloc NE,eloc N,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc NE,eloc N,mv+num,hm+num,hmval);}
		else if(CE1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.wouldBeUF(pla,eloc NE,eloc N,eloc NE)) {num += genUFStepWouldBeUF(b,pla,eloc NE,eloc N,kt,mv+num,hm+num,hmval);}

		if     (CN2(eloc) && ISP(eloc NN) && GT(eloc NN,eloc) && b.isThawed(eloc NN)) {TS(eloc NN,eloc N,suc = canUF(b,pla,eloc N)) if(suc) {ADDMOVE(Board::getMove(eloc NN MS),hmval)} num += genAdvanceUFStep(b,pla,eloc NN,eloc N,mv+num,hm+num,hmval);}
		else if(CN2(eloc) && ISP(eloc NN) && GT(eloc NN,eloc) && b.wouldBeUF(pla,eloc NN,eloc N,eloc NN)) {num += genUFStepWouldBeUF(b,pla,eloc NN,eloc N,kt,mv+num,hm+num,hmval);}
	}

	//Friendly piece in the way
	if(CS1(eloc) && ISP(eloc S)) {num += genSwapPla(b,pla,eloc S,eloc,mv+num,hm+num,hmval);}
	if(CW1(eloc) && ISP(eloc W)) {num += genSwapPla(b,pla,eloc W,eloc,mv+num,hm+num,hmval);}
	if(CE1(eloc) && ISP(eloc E)) {num += genSwapPla(b,pla,eloc E,eloc,mv+num,hm+num,hmval);}
	if(CN1(eloc) && ISP(eloc N)) {num += genSwapPla(b,pla,eloc N,eloc,mv+num,hm+num,hmval);}

	//Opponent's piece in the way
	if(CS1(eloc) && ISO(eloc S)) {num += genSwapOpp(b,pla,eloc S,eloc,kt,mv+num,hm+num,hmval);}
	if(CW1(eloc) && ISO(eloc W)) {num += genSwapOpp(b,pla,eloc W,eloc,kt,mv+num,hm+num,hmval);}
	if(CE1(eloc) && ISO(eloc E)) {num += genSwapOpp(b,pla,eloc E,eloc,kt,mv+num,hm+num,hmval);}
	if(CN1(eloc) && ISO(eloc N)) {num += genSwapOpp(b,pla,eloc N,eloc,kt,mv+num,hm+num,hmval);}

	//A step away, going through the trap.
	//Trap is very safe. Can just unfreeze our piece.
	//Temporarily add a rabbit to prevent unfreezes stepping on the trap.
	if(meNum >= 3)
	{
		b.owners[kt] = opp; b.pieces[kt] = RAB;
		if(ISP(kt S) && kt S != eloc && GT(kt S,eloc) && b.isFrozenC(kt S)) {num += genUF(b,pla,kt S,mv+num,hm+num,hmval);}
		if(ISP(kt W) && kt W != eloc && GT(kt W,eloc) && b.isFrozenC(kt W)) {num += genUF(b,pla,kt W,mv+num,hm+num,hmval);}
		if(ISP(kt E) && kt E != eloc && GT(kt E,eloc) && b.isFrozenC(kt E)) {num += genUF(b,pla,kt E,mv+num,hm+num,hmval);}
		if(ISP(kt N) && kt N != eloc && GT(kt N,eloc) && b.isFrozenC(kt N)) {num += genUF(b,pla,kt N,mv+num,hm+num,hmval);}
		b.owners[kt] = NPLA; b.pieces[kt] = EMP;
	}
	//Trap is safe. Can just unfreeze our piece, but make sure not to step away the trap-guarding piece.
	else if(meNum >= 2)
	{
		//Do some trickery; temporarily remove the piece so it isn't considrered by genUF
		if(ISP(kt S) && GT(kt S,eloc) && b.isFrozen(kt S))
		{
			if     (ISP(kt W)) {TR(kt W, num += genUF(b,pla,kt S,mv+num,hm+num,hmval))}
			else if(ISP(kt E)) {TR(kt E, num += genUF(b,pla,kt S,mv+num,hm+num,hmval))}
			else if(ISP(kt N)) {TR(kt N, num += genUF(b,pla,kt S,mv+num,hm+num,hmval))}
		}
		if(ISP(kt W) && GT(kt W,eloc) && b.isFrozen(kt W))
		{
			if     (ISP(kt S)) {TR(kt S, num += genUF(b,pla,kt W,mv+num,hm+num,hmval))}
			else if(ISP(kt E)) {TR(kt E, num += genUF(b,pla,kt W,mv+num,hm+num,hmval))}
			else if(ISP(kt N)) {TR(kt N, num += genUF(b,pla,kt W,mv+num,hm+num,hmval))}
		}
		if(ISP(kt E) && GT(kt E,eloc) && b.isFrozen(kt E))
		{
			if     (ISP(kt S)) {TR(kt S, num += genUF(b,pla,kt E,mv+num,hm+num,hmval))}
			else if(ISP(kt W)) {TR(kt W, num += genUF(b,pla,kt E,mv+num,hm+num,hmval))}
			else if(ISP(kt N)) {TR(kt N, num += genUF(b,pla,kt E,mv+num,hm+num,hmval))}
		}
		if(ISP(kt N) && GT(kt N,eloc) && b.isFrozen(kt N))
		{
			if     (ISP(kt S)) {TR(kt S, num += genUF(b,pla,kt N,mv+num,hm+num,hmval))}
			else if(ISP(kt W)) {TR(kt W, num += genUF(b,pla,kt N,mv+num,hm+num,hmval))}
			else if(ISP(kt E)) {TR(kt E, num += genUF(b,pla,kt N,mv+num,hm+num,hmval))}
		}
	}
	//The trap is unsafe, so make it safe
	else
	{
		//Locate the player next to the trap and its side-adjacent defender, if any
		int ploc = ERRORSQUARE;
		int guardloc = ERRORSQUARE;
		if     (ISP(kt S) && GT(kt S,eloc) && b.isThawed(kt S)) {ploc = kt S; if(ISP(kt SW)) {guardloc = kt SW;} else if(ISP(kt SE)) {guardloc = kt SE;}}
		else if(ISP(kt W) && GT(kt W,eloc) && b.isThawed(kt W)) {ploc = kt W; if(ISP(kt SW)) {guardloc = kt SW;} else if(ISP(kt NW)) {guardloc = kt NW;}}
		else if(ISP(kt E) && GT(kt E,eloc) && b.isThawed(kt E)) {ploc = kt E; if(ISP(kt SE)) {guardloc = kt SE;} else if(ISP(kt NE)) {guardloc = kt NE;}}
		else if(ISP(kt N) && GT(kt N,eloc) && b.isThawed(kt N)) {ploc = kt N; if(ISP(kt NW)) {guardloc = kt NW;} else if(ISP(kt NE)) {guardloc = kt NE;}}

		//If there's a strong enough player
		if(ploc != ERRORSQUARE)
		{
			//Player is very unfrozen, just make the trap safe any way.
			if(!b.isDominated(ploc) || b.isGuarded2(pla,ploc))
			{num += genUF(b,pla,kt,mv+num,hm+num,hmval);}
			//Player dominated and only one defender
			else
			{
				//No side adjacent defender to worry about, make the trap safe
				if(guardloc == ERRORSQUARE)
				{num += genUF(b,pla,kt,mv+num,hm+num,hmval);}
				//Temporarily remove the piece so genKTSafe won't use it to guard the trap.
				else
				{TR(guardloc,num += genUF(b,pla,kt,mv+num,hm+num,hmval))}
			}
		}
	}

	//2 Steps away
	if(ISE(eloc S) && eloc S != kt) {num += genMoveStrong2S(b,pla,eloc S,eloc,mv+num,hm+num,hmval);}
	if(ISE(eloc W) && eloc W != kt) {num += genMoveStrong2S(b,pla,eloc W,eloc,mv+num,hm+num,hmval);}
	if(ISE(eloc E) && eloc E != kt) {num += genMoveStrong2S(b,pla,eloc E,eloc,mv+num,hm+num,hmval);}
	if(ISE(eloc N) && eloc N != kt) {num += genMoveStrong2S(b,pla,eloc N,eloc,mv+num,hm+num,hmval);}

	//2 Steps away, going through the trap
	//The trap is empty, but must be safe for this to work.
	if(meNum >= 1)
	{
		num += genMoveStrongAdjCent(b,pla,kt,eloc,ERRORSQUARE,mv+num,hm+num,hmval);
	}

	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canPPIntoTrapTP4C(Board& b, pla_t pla, loc_t eloc, loc_t kt)
{
	bool suc = false;
	pla_t opp = OPP(pla);

	//Weak piece on trap, must step off then push on.
	if(b.pieces[kt] <= b.pieces[eloc])
	{
		//Can step off immediately
		if(ISE(kt S) && kt S != eloc && b.isRabOkayS(pla,kt)) {TS(kt,kt S, suc = canPPIntoTrapTE3C(b,pla,eloc,kt)) if(suc) {return true;}}
		if(ISE(kt W) && kt W != eloc)                         {TS(kt,kt W, suc = canPPIntoTrapTE3C(b,pla,eloc,kt)) if(suc) {return true;}}
		if(ISE(kt E) && kt E != eloc)                         {TS(kt,kt E, suc = canPPIntoTrapTE3C(b,pla,eloc,kt)) if(suc) {return true;}}
		if(ISE(kt N) && kt N != eloc && b.isRabOkayN(pla,kt)) {TS(kt,kt N, suc = canPPIntoTrapTE3C(b,pla,eloc,kt)) if(suc) {return true;}}

		//Move each adjacent piece around and recurse
		if(ISP(kt S))
		{
			if(ISE(kt SS) && b.isRabOkayS(pla,kt S)) {TSC(kt S,kt SS,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
			if(ISE(kt SW)                          ) {TSC(kt S,kt SW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
			if(ISE(kt SE)                          ) {TSC(kt S,kt SE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
		}
		if(ISP(kt W))
		{
			if(ISE(kt SW) && b.isRabOkayS(pla,kt W)) {TSC(kt W,kt SW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
			if(ISE(kt WW)                          ) {TSC(kt W,kt WW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
			if(ISE(kt NW) && b.isRabOkayN(pla,kt W)) {TSC(kt W,kt NW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
		}
		if(ISP(kt E))
		{
			if(ISE(kt SE) && b.isRabOkayS(pla,kt E)) {TSC(kt E,kt SE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
			if(ISE(kt EE)                          ) {TSC(kt E,kt EE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
			if(ISE(kt NE) && b.isRabOkayN(pla,kt E)) {TSC(kt E,kt NE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
		}
		if(ISP(kt N))
		{
			if(ISE(kt NW)                          ) {TSC(kt N,kt NW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
			if(ISE(kt NE)                          ) {TSC(kt N,kt NE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
			if(ISE(kt NN) && b.isRabOkayN(pla,kt N)) {TSC(kt N,kt NN,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {return true;}}
		}

		//SUICIDAL
		if(!b.isTrapSafe2(pla,kt))
		{
			if(ISP(kt S) && GT(kt S,eloc))
			{
				if(ISO(kt SW) && eloc == kt W && GT(kt S,kt SW) && b.wouldBeUF(pla,kt S,kt SW,kt S))
				{
					if(ISE(kt SSW)) {return true;}
					if(ISE(kt SWW)) {return true;}
				}
				else if(ISO(kt SE) && eloc == kt E && GT(kt S,kt SE) && b.wouldBeUF(pla,kt S,kt SE,kt S))
				{
					if(ISE(kt SSE)) {return true;}
					if(ISE(kt SEE)) {return true;}
				}
			}
			else if(ISP(kt W) && GT(kt W,eloc))
			{
				if(ISO(kt SW) && eloc == kt S && GT(kt W,kt SW) && b.wouldBeUF(pla,kt W,kt SW,kt W))
				{
					if(ISE(kt SSW)) {return true;}
					if(ISE(kt SWW)) {return true;}
				}
				else if(ISO(kt NW) && eloc == kt N && GT(kt W,kt NW) && b.wouldBeUF(pla,kt W,kt NW,kt W))
				{
					if(ISE(kt NNW)) {return true;}
					if(ISE(kt NWW)) {return true;}
				}
			}
			else if(ISP(kt E) && GT(kt E,eloc))
			{
				if(ISO(kt SE) && eloc == kt S && GT(kt E,kt SE) && b.wouldBeUF(pla,kt E,kt SE,kt E))
				{
					if(ISE(kt SSE)) {return true;}
					if(ISE(kt SEE)) {return true;}
				}
				else if(ISO(kt NE) && eloc == kt N && GT(kt E,kt NE) && b.wouldBeUF(pla,kt E,kt NE,kt E))
				{
					if(ISE(kt NNE)) {return true;}
					if(ISE(kt NEE)) {return true;}
				}
			}
			else if(ISP(kt N) && GT(kt N,eloc))
			{
				if(ISO(kt NW) && eloc == kt W && GT(kt N,kt NW) && b.wouldBeUF(pla,kt N,kt NW,kt N))
				{
					if(ISE(kt NNW)) {return true;}
					if(ISE(kt NWW)) {return true;}
				}
				else if(ISO(kt NE) && eloc == kt E && GT(kt N,kt NE) && b.wouldBeUF(pla,kt N,kt NE,kt N))
				{
					if(ISE(kt NNE)) {return true;}
					if(ISE(kt NEE)) {return true;}
				}
			}
		}
	}

	//Strong piece, but blocked. Since no 3 step captures were possible, all squares around must be doubly blocked by friendly pieces.
	else if(b.isBlocked(kt))
	{
		if(ISP(kt S) && canEmpty2(b,pla,kt S,ERRORSQUARE,kt)) {return true;}
		if(ISP(kt W) && canEmpty2(b,pla,kt W,ERRORSQUARE,kt)) {return true;}
		if(ISP(kt E) && canEmpty2(b,pla,kt E,ERRORSQUARE,kt)) {return true;}
		if(ISP(kt N) && canEmpty2(b,pla,kt N,ERRORSQUARE,kt)) {return true;}
	}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genPPIntoTrapTP4C(Board& b, pla_t pla, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	bool suc = false;
	int num = 0;
	pla_t opp = OPP(pla);

	//Weak piece on trap, must step off then push on.
	if(b.pieces[kt] <= b.pieces[eloc])
	{
		//Can step off immediately
		if(ISE(kt S) && kt S != eloc && b.isRabOkayS(pla,kt)) {TS(kt,kt S, suc = canPPIntoTrapTE3C(b,pla,eloc,kt)) if(suc) {ADDMOVE(Board::getMove(kt MS),hmval)}}
		if(ISE(kt W) && kt W != eloc)                         {TS(kt,kt W, suc = canPPIntoTrapTE3C(b,pla,eloc,kt)) if(suc) {ADDMOVE(Board::getMove(kt MW),hmval)}}
		if(ISE(kt E) && kt E != eloc)                         {TS(kt,kt E, suc = canPPIntoTrapTE3C(b,pla,eloc,kt)) if(suc) {ADDMOVE(Board::getMove(kt ME),hmval)}}
		if(ISE(kt N) && kt N != eloc && b.isRabOkayN(pla,kt)) {TS(kt,kt N, suc = canPPIntoTrapTE3C(b,pla,eloc,kt)) if(suc) {ADDMOVE(Board::getMove(kt MN),hmval)}}

		//Move each adjacent piece around and recurse
		if(ISP(kt S))
		{
			if(ISE(kt SS) && b.isRabOkayS(pla,kt S)) {TSC(kt S,kt SS,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt S MS),hmval)}}
			if(ISE(kt SW)                          ) {TSC(kt S,kt SW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt S MW),hmval)}}
			if(ISE(kt SE)                          ) {TSC(kt S,kt SE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt S ME),hmval)}}
		}
		if(ISP(kt W))
		{
			if(ISE(kt SW) && b.isRabOkayS(pla,kt W)) {TSC(kt W,kt SW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt W MS),hmval)}}
			if(ISE(kt WW)                          ) {TSC(kt W,kt WW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt W MW),hmval)}}
			if(ISE(kt NW) && b.isRabOkayN(pla,kt W)) {TSC(kt W,kt NW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt W MN),hmval)}}
		}
		if(ISP(kt E))
		{
			if(ISE(kt SE) && b.isRabOkayS(pla,kt E)) {TSC(kt E,kt SE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt E MS),hmval)}}
			if(ISE(kt EE)                          ) {TSC(kt E,kt EE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt E ME),hmval)}}
			if(ISE(kt NE) && b.isRabOkayN(pla,kt E)) {TSC(kt E,kt NE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt E MN),hmval)}}
		}
		if(ISP(kt N))
		{
			if(ISE(kt NW)                          ) {TSC(kt N,kt NW,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt N MW),hmval)}}
			if(ISE(kt NE)                          ) {TSC(kt N,kt NE,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt N ME),hmval)}}
			if(ISE(kt NN) && b.isRabOkayN(pla,kt N)) {TSC(kt N,kt NN,suc = (ISE(kt) ? canPPIntoTrapTE3C(b,pla,eloc,kt) : canPPIntoTrapTP3C(b,pla,eloc,kt))) if(suc) {ADDMOVE(Board::getMove(kt N MN),hmval)}}
		}

		//SUICIDAL
		if(!b.isTrapSafe2(pla,kt))
		{
			if(ISP(kt S) && GT(kt S,eloc))
			{
				if     (ISO(kt SW) && eloc == kt W && GT(kt S,kt SW) && b.wouldBeUF(pla,kt S,kt SW,kt S))
				{
					if(ISE(kt SSW)) {ADDMOVE(Board::getMove(kt SW MS,kt S MW),hmval)}
					if(ISE(kt SWW)) {ADDMOVE(Board::getMove(kt SW MW,kt S MW),hmval)}
				}
				else if(ISO(kt SE) && eloc == kt E && GT(kt S,kt SE) && b.wouldBeUF(pla,kt S,kt SE,kt S))
				{
					if(ISE(kt SSE)) {ADDMOVE(Board::getMove(kt SE MS,kt S ME),hmval)}
					if(ISE(kt SEE)) {ADDMOVE(Board::getMove(kt SE ME,kt S ME),hmval)}
				}
			}
			else if(ISP(kt W) && GT(kt W,eloc))
			{
				if     (ISO(kt SW) && eloc == kt S && GT(kt W,kt SW) && b.wouldBeUF(pla,kt W,kt SW,kt W))
				{
					if(ISE(kt SSW)) {ADDMOVE(Board::getMove(kt SW MS,kt W MS),hmval)}
					if(ISE(kt SWW)) {ADDMOVE(Board::getMove(kt SW MW,kt W MS),hmval)}
				}
				else if(ISO(kt NW) && eloc == kt N && GT(kt W,kt NW) && b.wouldBeUF(pla,kt W,kt NW,kt W))
				{
					if(ISE(kt NNW)) {ADDMOVE(Board::getMove(kt NW MN,kt W MN),hmval)}
					if(ISE(kt NWW)) {ADDMOVE(Board::getMove(kt NW MW,kt W MN),hmval)}
				}
			}
			else if(ISP(kt E) && GT(kt E,eloc))
			{
				if     (ISO(kt SE) && eloc == kt S && GT(kt E,kt SE) && b.wouldBeUF(pla,kt E,kt SE,kt E))
				{
					if(ISE(kt SSE)) {ADDMOVE(Board::getMove(kt SE MS,kt E MS),hmval)}
					if(ISE(kt SEE)) {ADDMOVE(Board::getMove(kt SE ME,kt E MS),hmval)}
				}
				else if(ISO(kt NE) && eloc == kt N && GT(kt E,kt NE) && b.wouldBeUF(pla,kt E,kt NE,kt E))
				{
					if(ISE(kt NNE)) {ADDMOVE(Board::getMove(kt NE MN,kt E MN),hmval)}
					if(ISE(kt NEE)) {ADDMOVE(Board::getMove(kt NE ME,kt E MN),hmval)}
				}
			}
			else if(ISP(kt N) && GT(kt N,eloc))
			{
				if     (ISO(kt NW) && eloc == kt W && GT(kt N,kt NW) && b.wouldBeUF(pla,kt N,kt NW,kt N))
				{
					if(ISE(kt NNW)) {ADDMOVE(Board::getMove(kt NW MN,kt N MW),hmval)}
					if(ISE(kt NWW)) {ADDMOVE(Board::getMove(kt NW MW,kt N MW),hmval)}
				}
				else if(ISO(kt NE) && eloc == kt E && GT(kt N,kt NE) && b.wouldBeUF(pla,kt N,kt NE,kt N))
				{
					if(ISE(kt NNE)) {ADDMOVE(Board::getMove(kt NE MN,kt N ME),hmval)}
					if(ISE(kt NEE)) {ADDMOVE(Board::getMove(kt NE ME,kt N ME),hmval)}
				}
			}
		}
	}

	//Strong piece, but blocked. Since no 3 step captures were possible, all squares around must be doubly blocked by friendly pieces.
	else if(b.isBlocked(kt))
	{
		if(ISP(kt S)) {num += genEmpty2(b,pla,kt S,ERRORSQUARE,kt,mv+num,hm+num,hmval);}
		if(ISP(kt W)) {num += genEmpty2(b,pla,kt W,ERRORSQUARE,kt,mv+num,hm+num,hmval);}
		if(ISP(kt E)) {num += genEmpty2(b,pla,kt E,ERRORSQUARE,kt,mv+num,hm+num,hmval);}
		if(ISP(kt N)) {num += genEmpty2(b,pla,kt N,ERRORSQUARE,kt,mv+num,hm+num,hmval);}
	}
	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canRemoveDef4C(Board& b, pla_t pla, loc_t eloc, loc_t kt)
{
	bool suc = false;
	pla_t opp = OPP(pla);

	//Frozen but can pushpull. Remove the defender.
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S, eloc) && b.isFrozen(eloc S))
	{
		if(canUF2PPPE(b,pla,eloc S,eloc)) {return true;}
		if(!b.isOpen2(eloc S) && canUFBlockedPP(b,pla,eloc S,eloc)) {return true;}
	}
	else if(CS1(eloc) && ISP(eloc S) && GT(eloc S, eloc) && b.isThawed(eloc S) && canBlockedPP2(b,pla,eloc S,eloc,kt)) {return true;}

	if(CW1(eloc) && ISP(eloc W) && GT(eloc W, eloc) && b.isFrozen(eloc W))
	{
		if(canUF2PPPE(b,pla,eloc W,eloc)) {return true;}
		if(!b.isOpen2(eloc W) && canUFBlockedPP(b,pla,eloc W,eloc)) {return true;}
	}
	else if(CW1(eloc) && ISP(eloc W) && GT(eloc W, eloc) && b.isThawed(eloc W) && canBlockedPP2(b,pla,eloc W,eloc,kt)) {return true;}

	if(CE1(eloc) && ISP(eloc E) && GT(eloc E, eloc) && b.isFrozen(eloc E))
	{
		if(canUF2PPPE(b,pla,eloc E,eloc)) {return true;}
		if(!b.isOpen2(eloc E) && canUFBlockedPP(b,pla,eloc E,eloc)) {return true;}
	}
	else if(CE1(eloc) && ISP(eloc E) && GT(eloc E, eloc) && b.isThawed(eloc E) && canBlockedPP2(b,pla,eloc E,eloc,kt)) {return true;}

	if(CN1(eloc) && ISP(eloc N) && GT(eloc N, eloc) && b.isFrozen(eloc N))
	{
		if(canUF2PPPE(b,pla,eloc N,eloc)) {return true;}
		if(!b.isOpen2(eloc N) && canUFBlockedPP(b,pla,eloc N,eloc)) {return true;}
	}
	else if(CN1(eloc) && ISP(eloc N) && GT(eloc N, eloc) && b.isThawed(eloc N) && canBlockedPP2(b,pla,eloc N,eloc,kt)) {return true;}

	//A step away, not going through the trap. Either must be frozen now, or be frozen after one step.***
	if(CS1(eloc) && ISE(eloc S))
	{
		if(CS2(eloc) && ISP(eloc SS) && GT(eloc SS,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SS,eloc S,eloc SS);
			if(b.isThawed(eloc SS) && !wuf) {TS(eloc SS,eloc S,suc = canUFPPPE(b,pla,eloc S,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SS,eloc S)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc SS,eloc S,kt)) {return true;}}

		if(CW1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SW,eloc S,eloc SW);
			if(b.isThawed(eloc SW) && !wuf) {TS(eloc SW,eloc S,suc = canUFPPPE(b,pla,eloc S,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SW,eloc S)) {return true;} if(canAdvancePullCap(b,pla,eloc SW,eloc S)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc SW,eloc S,kt)) {return true;}}

		if(CE1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SE,eloc S,eloc SE);
			if(b.isThawed(eloc SE) && !wuf) {TS(eloc SE,eloc S,suc = canUFPPPE(b,pla,eloc S,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SE,eloc S)) {return true;} if(canAdvancePullCap(b,pla,eloc SE,eloc S)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc SE,eloc S,kt)) {return true;}}
	}
	if(CW1(eloc) && ISE(eloc W))
	{
		if(CS1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SW,eloc W,eloc SW);
			if(b.isThawed(eloc SW) && !wuf) {TS(eloc SW,eloc W,suc = canUFPPPE(b,pla,eloc W,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SW,eloc W)) {return true;} if(canAdvancePullCap(b,pla,eloc SW,eloc W)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc SW,eloc W,kt)) {return true;}}

		if(CW2(eloc) && ISP(eloc WW) && GT(eloc WW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc WW,eloc W,eloc WW);
			if(b.isThawed(eloc WW) && !wuf) {TS(eloc WW,eloc W,suc = canUFPPPE(b,pla,eloc W,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc WW,eloc W)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc WW,eloc W,kt)) {return true;}}

		if(CN1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NW,eloc W,eloc NW);
			if(b.isThawed(eloc NW) && !wuf) {TS(eloc NW,eloc W,suc = canUFPPPE(b,pla,eloc W,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NW,eloc W)) {return true;} if(canAdvancePullCap(b,pla,eloc NW,eloc W)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc NW,eloc W,kt)) {return true;}}
	}
	if(CE1(eloc) && ISE(eloc E))
	{
		if(CS1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SE,eloc E,eloc SE);
			if(b.isThawed(eloc SE) && !wuf) {TS(eloc SE,eloc E,suc = canUFPPPE(b,pla,eloc E,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc SE,eloc E)) {return true;} if(canAdvancePullCap(b,pla,eloc SE,eloc E)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc SE,eloc E,kt)) {return true;}}

		if(CE2(eloc) && ISP(eloc EE) && GT(eloc EE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc EE,eloc E,eloc EE);
			if(b.isThawed(eloc EE) && !wuf) {TS(eloc EE,eloc E,suc = canUFPPPE(b,pla,eloc E,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc EE,eloc E)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc EE,eloc E,kt)) {return true;}}

		if(CN1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NE,eloc E,eloc NE);
			if(b.isThawed(eloc NE && !wuf)) {TS(eloc NE,eloc E,suc = canUFPPPE(b,pla,eloc E,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NE,eloc E)) {return true;} if(canAdvancePullCap(b,pla,eloc NE,eloc E)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc NE,eloc E,kt)) {return true;}}
	}
	if(CN1(eloc) && ISE(eloc N))
	{
		if(CW1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NW,eloc N,eloc NW);
			if(b.isThawed(eloc NW) && !wuf) {TS(eloc NW,eloc N,suc = canUFPPPE(b,pla,eloc N,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NW,eloc N)) {return true;} if(canAdvancePullCap(b,pla,eloc NW,eloc N)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc NW,eloc N,kt)) {return true;}}

		if(CE1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NE,eloc N,eloc NE);
			if(b.isThawed(eloc NE) && !wuf) {TS(eloc NE,eloc N,suc = canUFPPPE(b,pla,eloc N,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NE,eloc N)) {return true;} if(canAdvancePullCap(b,pla,eloc NE,eloc N)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc NE,eloc N,kt)) {return true;}}

		if(CN2(eloc) && ISP(eloc NN) && GT(eloc NN,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NN,eloc N,eloc NN);
			if(b.isThawed(eloc NN) && !wuf) {TS(eloc NN,eloc N,suc = canUFPPPE(b,pla,eloc N,eloc)) if(suc) {return true;} if(canAdvanceUFStep(b,pla,eloc NN,eloc N)) {return true;}}
			else if(wuf && canUFStepWouldBeUF(b,pla,eloc NN,eloc N,kt)) {return true;}}
	}

	//Friendly piece in the way
	if(CS1(eloc) && ISP(eloc S) && canSwapPla(b,pla,eloc S,eloc)) {return true;}
	if(CW1(eloc) && ISP(eloc W) && canSwapPla(b,pla,eloc W,eloc)) {return true;}
	if(CE1(eloc) && ISP(eloc E) && canSwapPla(b,pla,eloc E,eloc)) {return true;}
	if(CN1(eloc) && ISP(eloc N) && canSwapPla(b,pla,eloc N,eloc)) {return true;}

	//Opponent's piece in the way
	if(CS1(eloc) && ISO(eloc S) && eloc S != kt && canSwapOpp(b,pla,eloc S,eloc,kt)) {return true;}
	if(CW1(eloc) && ISO(eloc W) && eloc W != kt && canSwapOpp(b,pla,eloc W,eloc,kt)) {return true;}
	if(CE1(eloc) && ISO(eloc E) && eloc E != kt && canSwapOpp(b,pla,eloc E,eloc,kt)) {return true;}
	if(CN1(eloc) && ISO(eloc N) && eloc N != kt && canSwapOpp(b,pla,eloc N,eloc,kt)) {return true;}

	//2 Steps away
	if(ISE(eloc S) && canMoveStrong2S(b,pla,eloc S,eloc)) {return true;}
	if(ISE(eloc W) && canMoveStrong2S(b,pla,eloc W,eloc)) {return true;}
	if(ISE(eloc E) && canMoveStrong2S(b,pla,eloc E,eloc)) {return true;}
	if(ISE(eloc N) && canMoveStrong2S(b,pla,eloc N,eloc)) {return true;}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genRemoveDef4C(Board& b, pla_t pla, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	bool suc = false;
	int num = 0;
	pla_t opp = OPP(pla);

	//Frozen but can pushpull. Remove the defender.
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S, eloc) && b.isFrozen(eloc S))
	{
		num += genUF2PPPE(b,pla,eloc S,eloc,mv+num,hm+num,hmval);
		if(!b.isOpen2(eloc S))
		{num += genUFBlockedPP(b,pla,eloc S,eloc,mv+num,hm+num,hmval);}
	}
	else if(CS1(eloc) && ISP(eloc S) && GT(eloc S, eloc) && b.isThawed(eloc S)) {num += genBlockedPP2(b,pla,eloc S,eloc,kt,mv+num,hm+num,hmval);}

	if(CW1(eloc) && ISP(eloc W) && GT(eloc W, eloc) && b.isFrozen(eloc W))
	{
		num += genUF2PPPE(b,pla,eloc W,eloc,mv+num,hm+num,hmval);
		if(!b.isOpen2(eloc W))
		{num += genUFBlockedPP(b,pla,eloc W,eloc,mv+num,hm+num,hmval);}
	}
	else if(CW1(eloc) && ISP(eloc W) && GT(eloc W, eloc) && b.isThawed(eloc W)) {num += genBlockedPP2(b,pla,eloc W,eloc,kt,mv+num,hm+num,hmval);}

	if(CE1(eloc) && ISP(eloc E) && GT(eloc E, eloc) && b.isFrozen(eloc E))
	{
		num += genUF2PPPE(b,pla,eloc E,eloc,mv+num,hm+num,hmval);
		if(!b.isOpen2(eloc E))
		{num += genUFBlockedPP(b,pla,eloc E,eloc,mv+num,hm+num,hmval);}
	}
	else if(CE1(eloc) && ISP(eloc E) && GT(eloc E, eloc) && b.isThawed(eloc E)) {num += genBlockedPP2(b,pla,eloc E,eloc,kt,mv+num,hm+num,hmval);}

	if(CN1(eloc) && ISP(eloc N) && GT(eloc N, eloc) && b.isFrozen(eloc N))
	{
		num += genUF2PPPE(b,pla,eloc N,eloc,mv+num,hm+num,hmval);
		if(!b.isOpen2(eloc N))
		{num += genUFBlockedPP(b,pla,eloc N,eloc,mv+num,hm+num,hmval);}
	}
	else if(CN1(eloc) && ISP(eloc N) && GT(eloc N, eloc) && b.isThawed(eloc N)) {num += genBlockedPP2(b,pla,eloc N,eloc,kt,mv+num,hm+num,hmval);}

	//A step away, not going through the trap. Either must be frozen now, or be frozen after one step.***
	if(CS1(eloc) && ISE(eloc S))
	{
		if(CS2(eloc) && ISP(eloc SS) && GT(eloc SS,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SS,eloc S,eloc SS);
			if(b.isThawed(eloc SS) && !wuf) {TS(eloc SS,eloc S,suc = canUFPPPE(b,pla,eloc S,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc SS MN),hmval)} num += genAdvanceUFStep(b,pla,eloc SS,eloc S,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc SS,eloc S,kt,mv+num,hm+num,hmval);}}

		if(CW1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SW,eloc S,eloc SW);
			if(b.isThawed(eloc SW) && !wuf) {TS(eloc SW,eloc S,suc = canUFPPPE(b,pla,eloc S,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc SW ME),hmval)} num += genAdvanceUFStep(b,pla,eloc SW,eloc S,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc SW,eloc S,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc SW,eloc S,kt,mv+num,hm+num,hmval);}}

		if(CE1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SE,eloc S,eloc SE);
			if(b.isThawed(eloc SE) && !wuf) {TS(eloc SE,eloc S,suc = canUFPPPE(b,pla,eloc S,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc SE MW),hmval)} num += genAdvanceUFStep(b,pla,eloc SE,eloc S,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc SE,eloc S,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc SE,eloc S,kt,mv+num,hm+num,hmval);}}
	}
	if(CW1(eloc) && ISE(eloc W))
	{
		if(CS1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SW,eloc W,eloc NW);
			if(b.isThawed(eloc SW) && !wuf) {TS(eloc SW,eloc W,suc = canUFPPPE(b,pla,eloc W,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc SW MN),hmval)} num += genAdvanceUFStep(b,pla,eloc SW,eloc W,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc SW,eloc W,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc SW,eloc W,kt,mv+num,hm+num,hmval);}}

		if(CW2(eloc) && ISP(eloc WW) && GT(eloc WW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc WW,eloc W,eloc WW);
			if(b.isThawed(eloc WW) && !wuf) {TS(eloc WW,eloc W,suc = canUFPPPE(b,pla,eloc W,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc WW ME),hmval)} num += genAdvanceUFStep(b,pla,eloc WW,eloc W,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc WW,eloc W,kt,mv+num,hm+num,hmval);}}

		if(CN1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NW,eloc W,eloc NW);
			if(b.isThawed(eloc NW) && !wuf) {TS(eloc NW,eloc W,suc = canUFPPPE(b,pla,eloc W,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc NW MS),hmval)} num += genAdvanceUFStep(b,pla,eloc NW,eloc W,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc NW,eloc W,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc NW,eloc W,kt,mv+num,hm+num,hmval);}}
	}
	if(CE1(eloc) && ISE(eloc E))
	{
		if(CS1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc SE,eloc E,eloc SE);
			if(b.isThawed(eloc SE) && !wuf) {TS(eloc SE,eloc E,suc = canUFPPPE(b,pla,eloc E,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc SE MN),hmval)} num += genAdvanceUFStep(b,pla,eloc SE,eloc E,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc SE,eloc E,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc SE,eloc E,kt,mv+num,hm+num,hmval);}}

		if(CE2(eloc) && ISP(eloc EE) && GT(eloc EE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc EE,eloc E,eloc EE);
			if( b.isThawed(eloc EE) && !wuf) {TS(eloc EE,eloc E,suc = canUFPPPE(b,pla,eloc E,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc EE MW),hmval)} num += genAdvanceUFStep(b,pla,eloc EE,eloc E,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc EE,eloc E,kt,mv+num,hm+num,hmval);}}

		if(CN1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NE,eloc E,eloc NE);
			if( b.isThawed(eloc NE) && !wuf) {TS(eloc NE,eloc E,suc = canUFPPPE(b,pla,eloc E,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc NE MS),hmval)} num += genAdvanceUFStep(b,pla,eloc NE,eloc E,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc NE,eloc E,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc NE,eloc E,kt,mv+num,hm+num,hmval);}}
	}
	if(CN1(eloc) && ISE(eloc N))
	{
		if(CW1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NW,eloc N,eloc NW);
			if(b.isThawed(eloc NW) && !wuf) {TS(eloc NW,eloc N,suc = canUFPPPE(b,pla,eloc N,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc NW ME),hmval)} num += genAdvanceUFStep(b,pla,eloc NW,eloc N,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc NW,eloc N,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc NW,eloc N,kt,mv+num,hm+num,hmval);}}

		if(CE1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NE,eloc N,eloc NE);
			if(b.isThawed(eloc NE) && !wuf) {TS(eloc NE,eloc N,suc = canUFPPPE(b,pla,eloc N,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc NE MW),hmval)} num += genAdvanceUFStep(b,pla,eloc NE,eloc N,mv+num,hm+num,hmval); num += genAdvancePullCap(b,pla,eloc NE,eloc N,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc NE,eloc N,kt,mv+num,hm+num,hmval);}}

		if(CN2(eloc) && ISP(eloc NN) && GT(eloc NN,eloc)) {bool wuf = b.wouldBeUF(pla,eloc NN,eloc N,eloc NN);
			if(b.isThawed(eloc NN) && !wuf) {TS(eloc NN,eloc N,suc = canUFPPPE(b,pla,eloc N,eloc)) if(suc) {ADDMOVE(Board::getMove(eloc NN MS),hmval)} num += genAdvanceUFStep(b,pla,eloc NN,eloc N,mv+num,hm+num,hmval);}
			else if(wuf) {num += genUFStepWouldBeUF(b,pla,eloc NN,eloc N,kt,mv+num,hm+num,hmval);}}
	}

	//Friendly piece in the way
	if(CS1(eloc) && ISP(eloc S)) {num += genSwapPla(b,pla,eloc S,eloc,mv+num,hm+num,hmval);}
	if(CW1(eloc) && ISP(eloc W)) {num += genSwapPla(b,pla,eloc W,eloc,mv+num,hm+num,hmval);}
	if(CE1(eloc) && ISP(eloc E)) {num += genSwapPla(b,pla,eloc E,eloc,mv+num,hm+num,hmval);}
	if(CN1(eloc) && ISP(eloc N)) {num += genSwapPla(b,pla,eloc N,eloc,mv+num,hm+num,hmval);}

	//Opponent's piece in the way
	if(CS1(eloc) && ISO(eloc S) && eloc S != kt) {num += genSwapOpp(b,pla,eloc S,eloc,kt,mv+num,hm+num,hmval);}
	if(CW1(eloc) && ISO(eloc W) && eloc W != kt) {num += genSwapOpp(b,pla,eloc W,eloc,kt,mv+num,hm+num,hmval);}
	if(CE1(eloc) && ISO(eloc E) && eloc E != kt) {num += genSwapOpp(b,pla,eloc E,eloc,kt,mv+num,hm+num,hmval);}
	if(CN1(eloc) && ISO(eloc N) && eloc N != kt) {num += genSwapOpp(b,pla,eloc N,eloc,kt,mv+num,hm+num,hmval);}

	//2 Steps away
	if(ISE(eloc S)) {num += genMoveStrong2S(b,pla,eloc S,eloc,mv+num,hm+num,hmval);}
	if(ISE(eloc W)) {num += genMoveStrong2S(b,pla,eloc W,eloc,mv+num,hm+num,hmval);}
	if(ISE(eloc E)) {num += genMoveStrong2S(b,pla,eloc E,eloc,mv+num,hm+num,hmval);}
	if(ISE(eloc N)) {num += genMoveStrong2S(b,pla,eloc N,eloc,mv+num,hm+num,hmval);}

	return num;
}

//Tries to empty the location in 2 steps.
//Assumes loc is not a trap location
//Does not push opponent pieces on floc
//Does not pull with player pieces adjacent to floc2
//Does not push opponent pieces adjacent to floc
//Ensures we do not freeze ploc when pulling an opponent, if loc is an opponent
//Does NOT use the C version of thawed or frozen!
static bool canEmpty2(Board& b, pla_t pla, loc_t loc, loc_t floc, loc_t ploc)
{
	pla_t opp = OPP(pla);

	if(ISP(loc))
	{
		if(!b.isDominated(loc) || b.isGuarded2(pla,loc))
		{
			if(CS1(loc) && ISP(loc S) && b.isRabOkayS(pla,loc) && canSteps1S(b,pla,loc S)) {return true;}
			if(CW1(loc) && ISP(loc W)                          && canSteps1S(b,pla,loc W)) {return true;}
			if(CE1(loc) && ISP(loc E)                          && canSteps1S(b,pla,loc E)) {return true;}
			if(CN1(loc) && ISP(loc N) && b.isRabOkayN(pla,loc) && canSteps1S(b,pla,loc N)) {return true;}
		}

		if(b.isFrozen(loc))
		{
			if(CS1(loc) && ISE(loc S) && ((CW1(loc) && ISE(loc W)                         ) || (CE1(loc) && ISE(loc E)) || (CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)))) {if(canUFAt(b,pla,loc S)) {return true;}}
			if(CW1(loc) && ISE(loc W) && ((CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) || (CE1(loc) && ISE(loc E)) || (CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)))) {if(canUFAt(b,pla,loc W)) {return true;}}
			if(CE1(loc) && ISE(loc E) && ((CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) || (CW1(loc) && ISE(loc W)) || (CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)))) {if(canUFAt(b,pla,loc E)) {return true;}}
			if(CN1(loc) && ISE(loc N) && ((CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) || (CW1(loc) && ISE(loc W)) || (CE1(loc) && ISE(loc E)                         ))) {if(canUFAt(b,pla,loc N)) {return true;}}
		}
		else
		{
			if(CS1(loc) && ISO(loc S) && loc S != floc && GT(loc,loc S) && canPushE(b,loc S,floc)) {return true;}
			if(CW1(loc) && ISO(loc W) && loc W != floc && GT(loc,loc W) && canPushE(b,loc W,floc)) {return true;}
			if(CE1(loc) && ISO(loc E) && loc E != floc && GT(loc,loc E) && canPushE(b,loc E,floc)) {return true;}
			if(CN1(loc) && ISO(loc N) && loc N != floc && GT(loc,loc N) && canPushE(b,loc N,floc)) {return true;}
		}
	}
	else if(ISO(loc))
	{
		if(CS1(loc) && ISP(loc S) && !b.isAdjacent(loc S,floc) && GT(loc S,loc) && b.isThawed(loc S) && (!b.isAdjacent(loc S,ploc) || (!b.isDominated(ploc) && !GT(loc,ploc)) || b.isGuarded2(pla,ploc)) && b.isOpen(loc S)){return true;}
		if(CW1(loc) && ISP(loc W) && !b.isAdjacent(loc W,floc) && GT(loc W,loc) && b.isThawed(loc W) && (!b.isAdjacent(loc W,ploc) || (!b.isDominated(ploc) && !GT(loc,ploc)) || b.isGuarded2(pla,ploc)) && b.isOpen(loc W)){return true;}
		if(CE1(loc) && ISP(loc E) && !b.isAdjacent(loc E,floc) && GT(loc E,loc) && b.isThawed(loc E) && (!b.isAdjacent(loc E,ploc) || (!b.isDominated(ploc) && !GT(loc,ploc)) || b.isGuarded2(pla,ploc)) && b.isOpen(loc E)){return true;}
		if(CN1(loc) && ISP(loc N) && !b.isAdjacent(loc N,floc) && GT(loc N,loc) && b.isThawed(loc N) && (!b.isAdjacent(loc N,ploc) || (!b.isDominated(ploc) && !GT(loc,ploc)) || b.isGuarded2(pla,ploc)) && b.isOpen(loc N)){return true;}
	}

	return false;
}

//Tries to empty the location in 2 steps.
//Assumes loc is not a trap location
//Does not push opponent pieces on floc
//Does not pull with player pieces adjacent to floc2
//Does not push opponent pieces adjacent to floc
//Ensures we do not freeze ploc when pulling an opponent, if loc is an opponent
//Does NOT use the C version of thawed or frozen!
static int genEmpty2(Board& b, pla_t pla, loc_t loc, loc_t floc, loc_t ploc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	pla_t opp = OPP(pla);

	if(ISP(loc))
	{
		if(!b.isDominated(loc) || b.isGuarded2(pla,loc))
		{
			if(CS1(loc) && ISP(loc S) && b.isRabOkayS(pla,loc)) {num += genSteps1S(b,pla,loc S,mv+num,hm+num,hmval);}
			if(CW1(loc) && ISP(loc W))                          {num += genSteps1S(b,pla,loc W,mv+num,hm+num,hmval);}
			if(CE1(loc) && ISP(loc E))                          {num += genSteps1S(b,pla,loc E,mv+num,hm+num,hmval);}
			if(CN1(loc) && ISP(loc N) && b.isRabOkayN(pla,loc)) {num += genSteps1S(b,pla,loc N,mv+num,hm+num,hmval);}
		}

		if(b.isFrozen(loc))
		{
			if(CS1(loc) && ISE(loc S) && ((CW1(loc) && ISE(loc W)                         ) || (CE1(loc) && ISE(loc E)) || (CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)))) {num += genUFAt(b,pla,loc S,mv+num,hm+num,hmval);}
			if(CW1(loc) && ISE(loc W) && ((CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) || (CE1(loc) && ISE(loc E)) || (CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)))) {num += genUFAt(b,pla,loc W,mv+num,hm+num,hmval);}
			if(CE1(loc) && ISE(loc E) && ((CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) || (CW1(loc) && ISE(loc W)) || (CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)))) {num += genUFAt(b,pla,loc E,mv+num,hm+num,hmval);}
			if(CN1(loc) && ISE(loc N) && ((CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) || (CW1(loc) && ISE(loc W)) || (CE1(loc) && ISE(loc E)                         ))) {num += genUFAt(b,pla,loc N,mv+num,hm+num,hmval);}
		}
		else
		{
			if(CS1(loc) && ISO(loc S) && loc S != floc && GT(loc,loc S)) {num += genPushPE(b,loc,loc S,floc,mv+num,hm+num,hmval);}
			if(CW1(loc) && ISO(loc W) && loc W != floc && GT(loc,loc W)) {num += genPushPE(b,loc,loc W,floc,mv+num,hm+num,hmval);}
			if(CE1(loc) && ISO(loc E) && loc E != floc && GT(loc,loc E)) {num += genPushPE(b,loc,loc E,floc,mv+num,hm+num,hmval);}
			if(CN1(loc) && ISO(loc N) && loc N != floc && GT(loc,loc N)) {num += genPushPE(b,loc,loc N,floc,mv+num,hm+num,hmval);}
		}
	}
	else if(ISO(loc))
	{
		if(CS1(loc) && ISP(loc S) && !b.isAdjacent(loc S,floc) && GT(loc S,loc) && b.isThawed(loc S) && (!b.isAdjacent(loc S,ploc) || (!b.isDominated(ploc) && !GT(loc,ploc)) || b.isGuarded2(pla,ploc))) {num += genPullPE(b,loc S,loc,mv+num,hm+num,hmval);}
		if(CW1(loc) && ISP(loc W) && !b.isAdjacent(loc W,floc) && GT(loc W,loc) && b.isThawed(loc W) && (!b.isAdjacent(loc W,ploc) || (!b.isDominated(ploc) && !GT(loc,ploc)) || b.isGuarded2(pla,ploc))) {num += genPullPE(b,loc W,loc,mv+num,hm+num,hmval);}
		if(CE1(loc) && ISP(loc E) && !b.isAdjacent(loc E,floc) && GT(loc E,loc) && b.isThawed(loc E) && (!b.isAdjacent(loc E,ploc) || (!b.isDominated(ploc) && !GT(loc,ploc)) || b.isGuarded2(pla,ploc))) {num += genPullPE(b,loc E,loc,mv+num,hm+num,hmval);}
		if(CN1(loc) && ISP(loc N) && !b.isAdjacent(loc N,floc) && GT(loc N,loc) && b.isThawed(loc N) && (!b.isAdjacent(loc N,ploc) || (!b.isDominated(ploc) && !GT(loc,ploc)) || b.isGuarded2(pla,ploc))) {num += genPullPE(b,loc N,loc,mv+num,hm+num,hmval);}
	}

	return num;
}

//Does NOT use the C version of thawed or frozen!
static bool canUFBlockedPP(Board& b, pla_t pla, loc_t ploc, loc_t eloc)
{
	bool suc = false;
	pla_t opp = OPP(pla);

	if(CS1(ploc) && ISE(ploc S))
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawed(ploc SS) && b.isRabOkayN(pla,ploc SS)) {TS(ploc SS,ploc S,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW))                              {TS(ploc SW,ploc S,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE))                              {TS(ploc SE,ploc S,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
	}
	if(CW1(ploc) && ISE(ploc W))
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW) && b.isRabOkayN(pla,ploc SW)) {TS(ploc SW,ploc W,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawed(ploc WW))                              {TS(ploc WW,ploc W,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW) && b.isRabOkayS(pla,ploc NW)) {TS(ploc NW,ploc W,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
	}
	if(CE1(ploc) && ISE(ploc E))
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE) && b.isRabOkayN(pla,ploc SE)) {TS(ploc SE,ploc E,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawed(ploc EE))                              {TS(ploc EE,ploc E,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE) && b.isRabOkayS(pla,ploc NE)) {TS(ploc NE,ploc E,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
	}
	if(CN1(ploc) && ISE(ploc N))
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW))                              {TS(ploc NW,ploc N,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE))                              {TS(ploc NE,ploc N,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawed(ploc NN) && b.isRabOkayS(pla,ploc NN)) {TS(ploc NN,ploc N,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {return true;}}
	}

	//Pull at the same time as unfreezing to unblock
	if(CS1(ploc) && CW1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW))
	{
		if(ISO(ploc S) && GT(ploc SW,ploc S) && ISE(ploc W)) {return true;}
		if(ISO(ploc W) && GT(ploc SW,ploc W) && ISE(ploc S)) {return true;}
	}
	if(CS1(ploc) && CE1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE))
	{
		if(ISO(ploc S) && GT(ploc SE,ploc S) && ISE(ploc E)) {return true;}
		if(ISO(ploc E) && GT(ploc SE,ploc E) && ISE(ploc S)) {return true;}
	}
	if(CN1(ploc) && CW1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW))
	{
		if(ISO(ploc N) && GT(ploc NW,ploc N) && ISE(ploc W)) {return true;}
		if(ISO(ploc W) && GT(ploc NW,ploc W) && ISE(ploc N)) {return true;}
	}
	if(CN1(ploc) && CE1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE))
	{
		if(ISO(ploc N) && GT(ploc NE,ploc N) && ISE(ploc E)) {return true;}
		if(ISO(ploc E) && GT(ploc NE,ploc E) && ISE(ploc N)) {return true;}
	}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static int genUFBlockedPP(Board& b, pla_t pla, loc_t ploc, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;
	bool suc = false;
	pla_t opp = OPP(pla);

	if(CS1(ploc) && ISE(ploc S))
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawed(ploc SS) && b.isRabOkayN(pla,ploc SS)) {TS(ploc SS,ploc S,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SS MN),hmval)}}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW))                              {TS(ploc SW,ploc S,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SW ME),hmval)}}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE))                              {TS(ploc SE,ploc S,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SE MW),hmval)}}
	}
	if(CW1(ploc) && ISE(ploc W))
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW) && b.isRabOkayN(pla,ploc SW)) {TS(ploc SW,ploc W,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SW MN),hmval)}}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawed(ploc WW))                              {TS(ploc WW,ploc W,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc WW ME),hmval)}}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW) && b.isRabOkayS(pla,ploc NW)) {TS(ploc NW,ploc W,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NW MS),hmval)}}
	}
	if(CE1(ploc) && ISE(ploc E))
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE) && b.isRabOkayN(pla,ploc SE)) {TS(ploc SE,ploc E,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc SE MN),hmval)}}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawed(ploc EE))                              {TS(ploc EE,ploc E,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc EE MW),hmval)}}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE) && b.isRabOkayS(pla,ploc NE)) {TS(ploc NE,ploc E,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NE MS),hmval)}}
	}
	if(CN1(ploc) && ISE(ploc N))
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW))                              {TS(ploc NW,ploc N,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NW ME),hmval)}}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE))                              {TS(ploc NE,ploc N,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NE MW),hmval)}}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawed(ploc NN) && b.isRabOkayS(pla,ploc NN)) {TS(ploc NN,ploc N,suc = canBlockedPP(b,pla,ploc,eloc)) if(suc) {ADDMOVEPP(Board::getMove(ploc NN MS),hmval)}}
	}

	//Pull at the same time as unfreezing to unblock
	if(CS1(ploc) && CW1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW))
	{
		if(ISO(ploc S) && GT(ploc SW,ploc S) && ISE(ploc W)) {ADDMOVEPP(Board::getMove(ploc SW MN,ploc S MW),hmval)}
		if(ISO(ploc W) && GT(ploc SW,ploc W) && ISE(ploc S)) {ADDMOVEPP(Board::getMove(ploc SW ME,ploc W MS),hmval)}
	}
	if(CS1(ploc) && CE1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE))
	{
		if(ISO(ploc S) && GT(ploc SE,ploc S) && ISE(ploc E)) {ADDMOVEPP(Board::getMove(ploc SE MN,ploc S ME),hmval)}
		if(ISO(ploc E) && GT(ploc SE,ploc E) && ISE(ploc S)) {ADDMOVEPP(Board::getMove(ploc SE MW,ploc E MS),hmval)}
	}
	if(CN1(ploc) && CW1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW))
	{
		if(ISO(ploc N) && GT(ploc NW,ploc N) && ISE(ploc W)) {ADDMOVEPP(Board::getMove(ploc NW MS,ploc N MW),hmval)}
		if(ISO(ploc W) && GT(ploc NW,ploc W) && ISE(ploc N)) {ADDMOVEPP(Board::getMove(ploc NW ME,ploc W MN),hmval)}
	}
	if(CN1(ploc) && CE1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE))
	{
		if(ISO(ploc N) && GT(ploc NE,ploc N) && ISE(ploc E)) {ADDMOVEPP(Board::getMove(ploc NE MS,ploc N ME),hmval)}
		if(ISO(ploc E) && GT(ploc NE,ploc E) && ISE(ploc N)) {ADDMOVEPP(Board::getMove(ploc NE MW,ploc E MN),hmval)}
	}

	return num;
}

//Generates all possible 3-step pushpulls of a defender of the trap by a player piece, when blocked by other pieces.
//Assumes the player piece and enemy pieces are there, the player piece is large enough, and is currently unfrozen.
static bool canBlockedPP(Board& b, pla_t pla, loc_t ploc, loc_t eloc)
{
	//Push
	if(CS1(eloc) && eloc S != ploc && ISP(eloc S) && b.isThawedC(eloc S) && canSteps1S(b,pla,eloc S)) {return true;}
	if(CW1(eloc) && eloc W != ploc && ISP(eloc W) && b.isThawedC(eloc W) && canSteps1S(b,pla,eloc W)) {return true;}
	if(CE1(eloc) && eloc E != ploc && ISP(eloc E) && b.isThawedC(eloc E) && canSteps1S(b,pla,eloc E)) {return true;}
	if(CN1(eloc) && eloc N != ploc && ISP(eloc N) && b.isThawedC(eloc N) && canSteps1S(b,pla,eloc N)) {return true;}

	//Pull
	if(!b.isDominated(ploc) || b.isGuarded2(pla,ploc))
	{
		if(CS1(ploc) && ISP(ploc S) && canSteps1S(b,pla,ploc S)) {return true;}
		if(CW1(ploc) && ISP(ploc W) && canSteps1S(b,pla,ploc W)) {return true;}
		if(CE1(ploc) && ISP(ploc E) && canSteps1S(b,pla,ploc E)) {return true;}
		if(CN1(ploc) && ISP(ploc N) && canSteps1S(b,pla,ploc N)) {return true;}
	}

	return false;
}

//Generates all possible 3-step pushpulls of a defender of the trap by a player piece, when blocked by other pieces.
//Assumes the player piece and enemy pieces are there, the player piece is large enough, and is currently unfrozen.
static int genBlockedPP(Board& b, pla_t pla, loc_t ploc, loc_t eloc,
move_t* mv, int* hm, int hmval)
{
	int num = 0;

	//Push
	if(CS1(eloc) && eloc S != ploc && ISP(eloc S) && b.isThawedC(eloc S)) {num += genSteps1S(b,pla,eloc S,mv+num,hm+num,hmval);}
	if(CW1(eloc) && eloc W != ploc && ISP(eloc W) && b.isThawedC(eloc W)) {num += genSteps1S(b,pla,eloc W,mv+num,hm+num,hmval);}
	if(CE1(eloc) && eloc E != ploc && ISP(eloc E) && b.isThawedC(eloc E)) {num += genSteps1S(b,pla,eloc E,mv+num,hm+num,hmval);}
	if(CN1(eloc) && eloc N != ploc && ISP(eloc N) && b.isThawedC(eloc N)) {num += genSteps1S(b,pla,eloc N,mv+num,hm+num,hmval);}

	//Pull
	if(!b.isDominated(ploc) || b.isGuarded2(pla,ploc))
	{
		if(CS1(ploc) && ISP(ploc S)) {num += genSteps1S(b,pla,ploc S,mv+num,hm+num,hmval);}
		if(CW1(ploc) && ISP(ploc W)) {num += genSteps1S(b,pla,ploc W,mv+num,hm+num,hmval);}
		if(CE1(ploc) && ISP(ploc E)) {num += genSteps1S(b,pla,ploc E,mv+num,hm+num,hmval);}
		if(CN1(ploc) && ISP(ploc N)) {num += genSteps1S(b,pla,ploc N,mv+num,hm+num,hmval);}
	}

	return num;
}

//Generates all possible 4-step pushpulls of a defender of the trap by a player piece, when blocked by other pieces.
//Assumes the player piece and enemy pieces are there, the player piece is large enough, and is currently unfrozen.
//Don't need to worry about the rearrangements causing ploc to unfreeze, should be caught in genUnfreezes2
//Does NOT use the C version of thawed or frozen!
static bool canBlockedPP2(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t kt)
{
	pla_t opp = OPP(pla);

	//Clear space around eloc for pushing, but not the target on the trap!
	if(CS1(eloc) && eloc S != ploc && eloc S != kt && canEmpty2(b,pla,eloc S,kt,ploc)) {return true;}
	if(CW1(eloc) && eloc W != ploc && eloc W != kt && canEmpty2(b,pla,eloc W,kt,ploc)) {return true;}
	if(CE1(eloc) && eloc E != ploc && eloc E != kt && canEmpty2(b,pla,eloc E,kt,ploc)) {return true;}
	if(CN1(eloc) && eloc N != ploc && eloc N != kt && canEmpty2(b,pla,eloc N,kt,ploc)) {return true;}

	//Clear space around ploc for pulling
	if(CS1(ploc) && ploc S != eloc && b.wouldBeUF(pla,ploc,ploc,ploc S) && canEmpty2(b,pla,ploc S,kt,ploc)) {return true;}
	if(CW1(ploc) && ploc W != eloc && b.wouldBeUF(pla,ploc,ploc,ploc W) && canEmpty2(b,pla,ploc W,kt,ploc)) {return true;}
	if(CE1(ploc) && ploc E != eloc && b.wouldBeUF(pla,ploc,ploc,ploc E) && canEmpty2(b,pla,ploc E,kt,ploc)) {return true;}
	if(CN1(ploc) && ploc N != eloc && b.wouldBeUF(pla,ploc,ploc,ploc N) && canEmpty2(b,pla,ploc N,kt,ploc)) {return true;}

	//Clear space around ploc by capture
	if(CS1(ploc) && ploc S != eloc && ISO(ploc S) && !b.isTrapSafe2(opp,ploc S) && canPPCapAndUFc(b,pla,findAdjOpp(b,pla,ploc S),ploc)) {return true;}
	if(CW1(ploc) && ploc W != eloc && ISO(ploc W) && !b.isTrapSafe2(opp,ploc W) && canPPCapAndUFc(b,pla,findAdjOpp(b,pla,ploc W),ploc)) {return true;}
	if(CE1(ploc) && ploc E != eloc && ISO(ploc E) && !b.isTrapSafe2(opp,ploc E) && canPPCapAndUFc(b,pla,findAdjOpp(b,pla,ploc E),ploc)) {return true;}
	if(CN1(ploc) && ploc N != eloc && ISO(ploc N) && !b.isTrapSafe2(opp,ploc N) && canPPCapAndUFc(b,pla,findAdjOpp(b,pla,ploc N),ploc)) {return true;}

	return false;
}

//Generates all possible 4-step pushpulls of a defender of the trap by a player piece, when blocked by other pieces.
//Assumes the player piece and enemy pieces are there, the player piece is large enough, and is currently unfrozen.
//Don't need to worry about the rearrangements causing ploc to unfreeze, should be caught in genUnfreezes2
//Does NOT use the C version of thawed or frozen!
static int genBlockedPP2(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t kt,
move_t* mv, int* hm, int hmval)
{
	pla_t opp = OPP(pla);
	int num = 0;

	//Clear space around eloc for pushing, but not the target on the trap!
	if(CS1(eloc) && eloc S != ploc && eloc S != kt) {num += genEmpty2(b,pla,eloc S,kt,ploc,mv+num,hm+num,hmval);}
	if(CW1(eloc) && eloc W != ploc && eloc W != kt) {num += genEmpty2(b,pla,eloc W,kt,ploc,mv+num,hm+num,hmval);}
	if(CE1(eloc) && eloc E != ploc && eloc E != kt) {num += genEmpty2(b,pla,eloc E,kt,ploc,mv+num,hm+num,hmval);}
	if(CN1(eloc) && eloc N != ploc && eloc N != kt) {num += genEmpty2(b,pla,eloc N,kt,ploc,mv+num,hm+num,hmval);}

	//Clear space around ploc for pulling
	if(CS1(ploc) && ploc S != eloc && b.wouldBeUF(pla,ploc,ploc,ploc S)) {num += genEmpty2(b,pla,ploc S,kt,ploc,mv+num,hm+num,hmval);}
	if(CW1(ploc) && ploc W != eloc && b.wouldBeUF(pla,ploc,ploc,ploc W)) {num += genEmpty2(b,pla,ploc W,kt,ploc,mv+num,hm+num,hmval);}
	if(CE1(ploc) && ploc E != eloc && b.wouldBeUF(pla,ploc,ploc,ploc E)) {num += genEmpty2(b,pla,ploc E,kt,ploc,mv+num,hm+num,hmval);}
	if(CN1(ploc) && ploc N != eloc && b.wouldBeUF(pla,ploc,ploc,ploc N)) {num += genEmpty2(b,pla,ploc N,kt,ploc,mv+num,hm+num,hmval);}

	//Clear space around ploc by capture
	if(CS1(ploc) && ploc S != eloc && ISO(ploc S) && !b.isTrapSafe2(opp,ploc S)) {num += genPPCapAndUF(b,pla,findAdjOpp(b,pla,ploc S),ploc,mv+num,hm+num,hmval);}
	if(CW1(ploc) && ploc W != eloc && ISO(ploc W) && !b.isTrapSafe2(opp,ploc W)) {num += genPPCapAndUF(b,pla,findAdjOpp(b,pla,ploc W),ploc,mv+num,hm+num,hmval);}
	if(CE1(ploc) && ploc E != eloc && ISO(ploc E) && !b.isTrapSafe2(opp,ploc E)) {num += genPPCapAndUF(b,pla,findAdjOpp(b,pla,ploc E),ploc,mv+num,hm+num,hmval);}
	if(CN1(ploc) && ploc N != eloc && ISO(ploc N) && !b.isTrapSafe2(opp,ploc N)) {num += genPPCapAndUF(b,pla,findAdjOpp(b,pla,ploc N),ploc,mv+num,hm+num,hmval);}

	return num;
}

//Assumes there is an adjacent opponent
static loc_t findAdjOpp(Board& b, pla_t pla, loc_t k)
{
	pla_t opp = OPP(pla);

	if     (ISO(k S)) {return k S;}
	else if(ISO(k W)) {return k W;}
	else if(ISO(k E)) {return k E;}
	else              {return k N;}

	return ERRORSQUARE;
}

static bool canAdvancePullCap(Board& b, pla_t pla, loc_t ploc, loc_t tr)
{
	if(Board::ADJACENTTRAP[tr] == ERRORSQUARE)
		return false;

	pla_t opp = OPP(pla);

	int strongerCount = 0;
	loc_t eloc = ERRORSQUARE;
	if(ISO(tr S) && GT(tr S,ploc)) {eloc = tr S; strongerCount++;}
	if(ISO(tr W) && GT(tr W,ploc)) {eloc = tr W; strongerCount++;}
	if(ISO(tr E) && GT(tr E,ploc)) {eloc = tr E; strongerCount++;}
	if(ISO(tr N) && GT(tr N,ploc)) {eloc = tr N; strongerCount++;}

	if(strongerCount == 1 && !b.isTrapSafe2(opp,eloc))
	{
		loc_t edef = findAdjOpp(b,pla,eloc);
		if(b.isAdjacent(ploc,edef) && GT(ploc,edef)) {return true;}
	}

	return false;
}

static int genAdvancePullCap(Board& b, pla_t pla, loc_t ploc, loc_t tr,
move_t* mv, int* hm, int hmval)
{
	if(Board::ADJACENTTRAP[tr] == ERRORSQUARE)
		return 0;

	pla_t opp = OPP(pla);
	int num = 0;

	int strongerCount = 0;
	loc_t eloc = ERRORSQUARE;
	if(ISO(tr S) && GT(tr S,ploc)) {eloc = tr S; strongerCount++;}
	if(ISO(tr W) && GT(tr W,ploc)) {eloc = tr W; strongerCount++;}
	if(ISO(tr E) && GT(tr E,ploc)) {eloc = tr E; strongerCount++;}
	if(ISO(tr N) && GT(tr N,ploc)) {eloc = tr N; strongerCount++;}

	if(strongerCount == 1 && !b.isTrapSafe2(opp,eloc))
	{
		loc_t edef = findAdjOpp(b,pla,eloc);
		if(b.isAdjacent(ploc,edef) && GT(ploc,edef)) {ADDMOVEPP(Board::getMove(Board::STEPINDEX[ploc][tr],Board::STEPINDEX[edef][ploc]),hmval)}
	}

	return num;
}


int BoardTrees::genUFPPPEShared(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval)
{
	return genUFPPPE(b, pla, ploc, eloc, mv, hm, hmval);
}
int BoardTrees::genBlockedPPShared(Board& b, pla_t pla, loc_t ploc, loc_t eloc, move_t* mv, int* hm, int hmval)
{
	return genBlockedPP(b, pla, ploc, eloc, mv, hm, hmval);
}
int BoardTrees::genMoveStrongAdjCentShared(Board& b, pla_t pla, loc_t dest, loc_t eloc, loc_t forbidden, move_t* mv, int* hm, int hmval)
{
	return genMoveStrongAdjCent(b, pla, dest, eloc, forbidden, mv, hm, hmval);
}





