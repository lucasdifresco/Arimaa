
/*
 * boardmovegen.cpp
 * Author: davidwu
 *
 * Standard move generation functions
 */
#include "pch.h"

#include <iostream>
#include <cmath>
#include "bitmap.h"
#include "board.h"
#include "boardtreeconst.h"
#include "boardmovegen.h"

using namespace std;

//MANHATTAN-CHAIN COMBO GENERATION--------------------------------------------

int BoardMoveGen::genLocalComboMoves(const Board& b, pla_t pla, int numSteps, move_t* mv)
{
  return genLocalComboMoves(b,pla,numSteps,mv,ERRORMOVE,0,Bitmap::BMPZEROS);
}

int BoardMoveGen::genLocalComboMoves(const Board& b, pla_t pla, int numSteps, move_t* mv, move_t prefix, int prefixLen, Bitmap relevantArea)
{
  if(numSteps <= 0)
    return 0;

  Bitmap rel = relevantArea;
  if(rel == Bitmap::BMPZEROS)
    rel = Bitmap::BMPONES;

  //Generate all steps and pushpulls
  int numMoves = 0;
  move_t moves[256];
  if(numSteps > 1)
    numMoves += genPushPullsInvolving(b,pla,moves+numMoves,rel);
  numMoves += genStepsInvolving(b,pla,moves+numMoves,rel);

  //Attach prefix and put them into passed in list
  int numMV = numMoves;
  for(int i = 0; i<numMoves; i++)
  {
    mv[i] =  Board::concatMoves(prefix,moves[i],prefixLen);
  }
  //No recursion needed?
  if(numSteps <= 1)
    return numMV;

  //Otherwise recurse down all possible combinations
  for(int i = 0; i<numMoves; i++)
  {
    int ns = Board::numStepsInMove(moves[i]);
    if(ns < numSteps)
    {
      Board copy = b;
      copy.makeMove(moves[i]);
      Bitmap affected = (copy.pieceMaps[0][0] ^ b.pieceMaps[0][0]) | (copy.pieceMaps[1][0] ^ b.pieceMaps[1][0]);
      affected |= Bitmap::adj(affected);
      affected |= Bitmap::adj(affected & Bitmap::BMPTRAPS);
      numMV += genLocalComboMoves(copy,pla,numSteps-ns,mv+numMV,mv[i],prefixLen+ns,affected);
    }
  }
  return numMV;
}

//SIMPLE-CHAIN COMBO GENERATION-----------------------------------------------

int BoardMoveGen::genSimpleChainMoves(Board& b, pla_t pla, int numSteps, move_t* mv)
{
  return genSimpleChainMoves(b,pla,numSteps,mv,Bitmap::BMPONES);
}

int BoardMoveGen::genSimpleChainMoves(Board& b, pla_t pla, int numSteps, move_t* mv, Bitmap relPlaPieces)
{
  int num = 0;
  Bitmap plaMap = b.pieceMaps[pla][0] & (~b.frozenMap) & relPlaPieces;
  while(plaMap.hasBits())
    num += genSimpleChainMoves(b,pla,plaMap.nextBit(),numSteps,mv+num,ERRORMOVE,0,ERRORSQUARE);
  return num;
}

int BoardMoveGen::genSimpleChainMoves(Board& b, pla_t pla, loc_t ploc, int numSteps, move_t* mv, move_t prefix, int prefixLen, loc_t prohibitedLoc)
{
  if(numSteps <= 0 || b.owners[ploc] != pla || b.isFrozenC(ploc))
    return 0;

  pla_t opp = OPP(pla);
  int num = 0;
  if(numSteps >= 1)
  {
    if(CS1(ploc) && ISE(ploc S) && ploc S != prohibitedLoc && b.isRabOkayS(pla,ploc)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MS),prefixLen);}
    if(CW1(ploc) && ISE(ploc W) && ploc W != prohibitedLoc)                           {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MW),prefixLen);}
    if(CE1(ploc) && ISE(ploc E) && ploc E != prohibitedLoc)                           {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc ME),prefixLen);}
    if(CN1(ploc) && ISE(ploc N) && ploc N != prohibitedLoc && b.isRabOkayN(pla,ploc)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MN),prefixLen);}
  }
  if(numSteps >= 2 && b.pieces[ploc] > RAB)
  {
    if(CS1(ploc) && ISO(ploc S) && GT(ploc, ploc S))
    {
      if(CW1(ploc) && ISE(ploc W))  {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MW, ploc S MN),prefixLen);}
      if(CE1(ploc) && ISE(ploc E))  {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc ME, ploc S MN),prefixLen);}
      if(CN1(ploc) && ISE(ploc N))  {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MN, ploc S MN),prefixLen);}
      if(CS2(ploc) && ISE(ploc SS)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc S MS, ploc MS),prefixLen);}
      if(CW1(ploc) && ISE(ploc SW)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc S MW, ploc MS),prefixLen);}
      if(CE1(ploc) && ISE(ploc SE)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc S ME, ploc MS),prefixLen);}
    }
    if(CW1(ploc) && ISO(ploc W) && GT(ploc, ploc W))
    {
      if(CS1(ploc) && ISE(ploc S)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MS, ploc W ME),prefixLen);}
      if(CE1(ploc) && ISE(ploc E)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc ME, ploc W ME),prefixLen);}
      if(CN1(ploc) && ISE(ploc N)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MN, ploc W ME),prefixLen);}
      if(CS1(ploc) && ISE(ploc SW)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc W MS, ploc MW),prefixLen);}
      if(CW2(ploc) && ISE(ploc WW)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc W MW, ploc MW),prefixLen);}
      if(CN1(ploc) && ISE(ploc NW)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc W MN, ploc MW),prefixLen);}
    }
    if(CE1(ploc) && ISO(ploc E) && GT(ploc, ploc E))
    {
      if(CS1(ploc) && ISE(ploc S)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MS, ploc E MW),prefixLen);}
      if(CW1(ploc) && ISE(ploc W)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MW, ploc E MW),prefixLen);}
      if(CN1(ploc) && ISE(ploc N)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MN, ploc E MW),prefixLen);}
      if(CS1(ploc) && ISE(ploc SE)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc E MS, ploc ME),prefixLen);}
      if(CE2(ploc) && ISE(ploc EE)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc E ME, ploc ME),prefixLen);}
      if(CN1(ploc) && ISE(ploc NE)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc E MN, ploc ME),prefixLen);}
    }
    if(CN1(ploc) && ISO(ploc N) && GT(ploc, ploc N))
    {
      if(CS1(ploc) && ISE(ploc S)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MS, ploc N MS),prefixLen);}
      if(CW1(ploc) && ISE(ploc W)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc MW, ploc N MS),prefixLen);}
      if(CE1(ploc) && ISE(ploc E)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc ME, ploc N MS),prefixLen);}
      if(CW1(ploc) && ISE(ploc NW)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc N MW, ploc MN),prefixLen);}
      if(CE1(ploc) && ISE(ploc NE)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc N ME, ploc MN),prefixLen);}
      if(CN2(ploc) && ISE(ploc NN)) {mv[num++] = Board::concatMoves(prefix,Board::Board::getMove(ploc N MN, ploc MN),prefixLen);}
    }
  }

  if(numSteps >= 2)
  {
    loc_t newProhibLoc = (Board::ADJACENTTRAP[ploc] != ERRORSQUARE && b.trapGuardCounts[pla][Board::TRAPINDEX[Board::ADJACENTTRAP[ploc]]] <= 1) ? ERRORSQUARE : ploc;
    if(CS1(ploc) && ISE(ploc S) && ploc S != prohibitedLoc && b.isRabOkayS(pla,ploc)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(Board::STEPINDEX[ploc][ploc S]),prefixLen); TSC(ploc,ploc S, num += genSimpleChainMoves(b,pla,ploc S,numSteps-1,mv+num,newPrefix,prefixLen+1,newProhibLoc))}
    if(CW1(ploc) && ISE(ploc W) && ploc W != prohibitedLoc)                           {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(Board::STEPINDEX[ploc][ploc W]),prefixLen); TSC(ploc,ploc W, num += genSimpleChainMoves(b,pla,ploc W,numSteps-1,mv+num,newPrefix,prefixLen+1,newProhibLoc))}
    if(CE1(ploc) && ISE(ploc E) && ploc E != prohibitedLoc)                           {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(Board::STEPINDEX[ploc][ploc E]),prefixLen); TSC(ploc,ploc E, num += genSimpleChainMoves(b,pla,ploc E,numSteps-1,mv+num,newPrefix,prefixLen+1,newProhibLoc))}
    if(CN1(ploc) && ISE(ploc N) && ploc N != prohibitedLoc && b.isRabOkayN(pla,ploc)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(Board::STEPINDEX[ploc][ploc N]),prefixLen); TSC(ploc,ploc N, num += genSimpleChainMoves(b,pla,ploc N,numSteps-1,mv+num,newPrefix,prefixLen+1,newProhibLoc))}
  }

  if(numSteps >= 3 && b.pieces[ploc] > RAB)
  {
    if(CS1(ploc) && ISO(ploc S) && GT(ploc, ploc S))
    {
      if(CW1(ploc) && ISE(ploc W))  {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MW, ploc S MN),prefixLen); TPC(ploc S,ploc,ploc W, num += genSimpleChainMoves(b,pla,ploc W,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CE1(ploc) && ISE(ploc E))  {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc ME, ploc S MN),prefixLen); TPC(ploc S,ploc,ploc E, num += genSimpleChainMoves(b,pla,ploc E,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CN1(ploc) && ISE(ploc N))  {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MN, ploc S MN),prefixLen); TPC(ploc S,ploc,ploc N, num += genSimpleChainMoves(b,pla,ploc N,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CS2(ploc) && ISE(ploc SS)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc S MS, ploc MS),prefixLen); TPC(ploc,ploc S,ploc SS, num += genSimpleChainMoves(b,pla,ploc S,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CW1(ploc) && ISE(ploc SW)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc S MW, ploc MS),prefixLen); TPC(ploc,ploc S,ploc SW, num += genSimpleChainMoves(b,pla,ploc S,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CE1(ploc) && ISE(ploc SE)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc S ME, ploc MS),prefixLen); TPC(ploc,ploc S,ploc SE, num += genSimpleChainMoves(b,pla,ploc S,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
    }
    if(CW1(ploc) && ISO(ploc W) && GT(ploc, ploc W))
    {
      if(CS1(ploc) && ISE(ploc S)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MS, ploc W ME),prefixLen); TPC(ploc W,ploc,ploc S, num += genSimpleChainMoves(b,pla,ploc S,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CE1(ploc) && ISE(ploc E)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc ME, ploc W ME),prefixLen); TPC(ploc W,ploc,ploc E, num += genSimpleChainMoves(b,pla,ploc E,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CN1(ploc) && ISE(ploc N)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MN, ploc W ME),prefixLen); TPC(ploc W,ploc,ploc N, num += genSimpleChainMoves(b,pla,ploc N,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CS1(ploc) && ISE(ploc SW)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc W MS, ploc MW),prefixLen); TPC(ploc,ploc W,ploc SW, num += genSimpleChainMoves(b,pla,ploc W,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CW2(ploc) && ISE(ploc WW)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc W MW, ploc MW),prefixLen); TPC(ploc,ploc W,ploc WW, num += genSimpleChainMoves(b,pla,ploc W,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CN1(ploc) && ISE(ploc NW)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc W MN, ploc MW),prefixLen); TPC(ploc,ploc W,ploc NW, num += genSimpleChainMoves(b,pla,ploc W,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
    }
    if(CE1(ploc) && ISO(ploc E) && GT(ploc, ploc E))
    {
      if(CS1(ploc) && ISE(ploc S)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MS, ploc E MW),prefixLen); TPC(ploc E,ploc,ploc S, num += genSimpleChainMoves(b,pla,ploc S,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CW1(ploc) && ISE(ploc W)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MW, ploc E MW),prefixLen); TPC(ploc E,ploc,ploc W, num += genSimpleChainMoves(b,pla,ploc W,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CN1(ploc) && ISE(ploc N)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MN, ploc E MW),prefixLen); TPC(ploc E,ploc,ploc N, num += genSimpleChainMoves(b,pla,ploc N,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CS1(ploc) && ISE(ploc SE)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc E MS, ploc ME),prefixLen); TPC(ploc,ploc E,ploc SE, num += genSimpleChainMoves(b,pla,ploc E,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CE2(ploc) && ISE(ploc EE)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc E ME, ploc ME),prefixLen); TPC(ploc,ploc E,ploc EE, num += genSimpleChainMoves(b,pla,ploc E,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CN1(ploc) && ISE(ploc NE)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc E MN, ploc ME),prefixLen); TPC(ploc,ploc E,ploc NE, num += genSimpleChainMoves(b,pla,ploc E,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
    }
    if(CN1(ploc) && ISO(ploc N) && GT(ploc, ploc N))
    {
      if(CS1(ploc) && ISE(ploc S)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MS, ploc N MS),prefixLen); TPC(ploc N,ploc,ploc S, num += genSimpleChainMoves(b,pla,ploc S,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CW1(ploc) && ISE(ploc W)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc MW, ploc N MS),prefixLen); TPC(ploc N,ploc,ploc W, num += genSimpleChainMoves(b,pla,ploc W,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CE1(ploc) && ISE(ploc E)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc ME, ploc N MS),prefixLen); TPC(ploc N,ploc,ploc E, num += genSimpleChainMoves(b,pla,ploc E,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CW1(ploc) && ISE(ploc NW)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc N MW, ploc MN),prefixLen); TPC(ploc,ploc N,ploc NW, num += genSimpleChainMoves(b,pla,ploc N,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CE1(ploc) && ISE(ploc NE)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc N ME, ploc MN),prefixLen); TPC(ploc,ploc N,ploc NE, num += genSimpleChainMoves(b,pla,ploc N,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
      if(CN2(ploc) && ISE(ploc NN)) {move_t newPrefix = Board::concatMoves(prefix,Board::Board::getMove(ploc N MN, ploc MN),prefixLen); TPC(ploc,ploc N,ploc NN, num += genSimpleChainMoves(b,pla,ploc N,numSteps-2,mv+num,newPrefix,prefixLen+2,ERRORSQUARE))}
    }
  }
  return num;
}

//MAIN GENERATION-------------------------------------------------------------

int BoardMoveGen::genPushPulls(const Board& b, pla_t pla, move_t* mv)
{
  pla_t opp = OPP(pla);

	Bitmap pushMap = b.pieceMaps[pla][0];
	pushMap &= ~b.frozenMap;           //Must be unfrozen
	pushMap &= ~b.pieceMaps[pla][RAB]; //Rabbits can't push
	pushMap = Bitmap::adj(pushMap);
	pushMap &= b.pieceMaps[opp][0];    //Must have opp to push
	pushMap &= ~b.pieceMaps[opp][ELE]; //Elephants can't be pushed
	pushMap &= Bitmap::adj(Bitmap::BMPONES & (~b.pieceMaps[pla][0]) & (~b.pieceMaps[opp][0])); //Must have empty space

	Bitmap pullMap = b.pieceMaps[opp][0];
	pullMap &= ~b.pieceMaps[opp][ELE]; //Elephants can't be pulled
	pullMap = Bitmap::adj(pullMap);
	pullMap &= b.pieceMaps[pla][0];    //Must have pla to pull
	pullMap &= ~b.frozenMap;           //Must be unfrozen
	pullMap &= ~b.pieceMaps[pla][RAB]; //Rabbits can't pull
	pullMap &= Bitmap::adj(Bitmap::BMPONES & (~b.pieceMaps[pla][0]) & (~b.pieceMaps[opp][0])); //Must have empty space

	int num = 0;
	while(pushMap.hasBits())
	{
		loc_t k = pushMap.nextBit();
		if(CS1(k) && ISP(k S) && GT(k S,k) && b.isThawed(k S))
		{
			if(CW1(k) && ISE(k W)){mv[num++] = Board::getMove(k MW,k S MN);}
			if(CE1(k) && ISE(k E)){mv[num++] = Board::getMove(k ME,k S MN);}
			if(CN1(k) && ISE(k N)){mv[num++] = Board::getMove(k MN,k S MN);}
		}
		if(CW1(k) && ISP(k W) && GT(k W,k) && b.isThawed(k W))
		{
			if(CS1(k) && ISE(k S)){mv[num++] = Board::getMove(k MS,k W ME);}
			if(CE1(k) && ISE(k E)){mv[num++] = Board::getMove(k ME,k W ME);}
			if(CN1(k) && ISE(k N)){mv[num++] = Board::getMove(k MN,k W ME);}
		}
		if(CE1(k) && ISP(k E) && GT(k E,k) && b.isThawed(k E))
		{
			if(CS1(k) && ISE(k S)){mv[num++] = Board::getMove(k MS,k E MW);}
			if(CW1(k) && ISE(k W)){mv[num++] = Board::getMove(k MW,k E MW);}
			if(CN1(k) && ISE(k N)){mv[num++] = Board::getMove(k MN,k E MW);}
		}
		if(CN1(k) && ISP(k N) && GT(k N,k) && b.isThawed(k N))
		{
			if(CS1(k) && ISE(k S)){mv[num++] = Board::getMove(k MS,k N MS);}
			if(CW1(k) && ISE(k W)){mv[num++] = Board::getMove(k MW,k N MS);}
			if(CE1(k) && ISE(k E)){mv[num++] = Board::getMove(k ME,k N MS);}
		}
	}
	while(pullMap.hasBits())
	{
	  loc_t k = pullMap.nextBit();
		if(CS1(k) && ISO(k S) && GT(k,k S))
		{
			if(CW1(k) && ISE(k W)){mv[num++] = Board::getMove(k MW,k S MN);}
			if(CE1(k) && ISE(k E)){mv[num++] = Board::getMove(k ME,k S MN);}
			if(CN1(k) && ISE(k N)){mv[num++] = Board::getMove(k MN,k S MN);}
		}
		if(CW1(k) && ISO(k W) && GT(k,k W))
		{
			if(CS1(k) && ISE(k S)){mv[num++] = Board::getMove(k MS,k W ME);}
			if(CE1(k) && ISE(k E)){mv[num++] = Board::getMove(k ME,k W ME);}
			if(CN1(k) && ISE(k N)){mv[num++] = Board::getMove(k MN,k W ME);}
		}
		if(CE1(k) && ISO(k E) && GT(k,k E))
		{
			if(CS1(k) && ISE(k S)){mv[num++] = Board::getMove(k MS,k E MW);}
			if(CW1(k) && ISE(k W)){mv[num++] = Board::getMove(k MW,k E MW);}
			if(CN1(k) && ISE(k N)){mv[num++] = Board::getMove(k MN,k E MW);}
		}
		if(CN1(k) && ISO(k N) && GT(k,k N))
		{
			if(CS1(k) && ISE(k S)){mv[num++] = Board::getMove(k MS,k N MS);}
			if(CW1(k) && ISE(k W)){mv[num++] = Board::getMove(k MW,k N MS);}
			if(CE1(k) && ISE(k E)){mv[num++] = Board::getMove(k ME,k N MS);}
		}
	}
	return num;
}

bool BoardMoveGen::canPushPulls(const Board& b, pla_t pla)
{
  pla_t opp = OPP(pla);

	Bitmap pushMap = b.pieceMaps[pla][0];
	pushMap &= ~b.frozenMap;           //Must be unfrozen
	pushMap &= ~b.pieceMaps[pla][RAB]; //Rabbits can't push
	pushMap = Bitmap::adj(pushMap);
	pushMap &= b.pieceMaps[opp][0];    //Must have opp to push
	pushMap &= ~b.pieceMaps[opp][ELE]; //Elephants can't be pushed
	pushMap &= Bitmap::adj(Bitmap::BMPONES & (~b.pieceMaps[pla][0]) & (~b.pieceMaps[opp][0])); //Must have empty space

	Bitmap pullMap = b.pieceMaps[opp][0];
	pullMap &= ~b.pieceMaps[opp][ELE]; //Elephants can't be pulled
	pullMap = Bitmap::adj(pullMap);
	pullMap &= b.pieceMaps[pla][0];    //Must have pla to pull
	pullMap &= ~b.frozenMap;           //Must be unfrozen
	pullMap &= ~b.pieceMaps[pla][RAB]; //Rabbits can't pull
	pullMap &= Bitmap::adj(Bitmap::BMPONES & (~b.pieceMaps[pla][0]) & (~b.pieceMaps[opp][0])); //Must have empty space

	while(pushMap.hasBits())
	{
		loc_t k = pushMap.nextBit();
		if(CS1(k) && ISP(k S) && GT(k S,k) && b.isThawed(k S))
		{
			if(CW1(k) && ISE(k W)){return true;}
			if(CE1(k) && ISE(k E)){return true;}
			if(CN1(k) && ISE(k N)){return true;}
		}
		if(CW1(k) && ISP(k W) && GT(k W,k) && b.isThawed(k W))
		{
			if(CS1(k) && ISE(k S)){return true;}
			if(CE1(k) && ISE(k E)){return true;}
			if(CN1(k) && ISE(k N)){return true;}
		}
		if(CE1(k) && ISP(k E) && GT(k E,k) && b.isThawed(k E))
		{
			if(CS1(k) && ISE(k S)){return true;}
			if(CW1(k) && ISE(k W)){return true;}
			if(CN1(k) && ISE(k N)){return true;}
		}
		if(CN1(k) && ISP(k N) && GT(k N,k) && b.isThawed(k N))
		{
			if(CS1(k) && ISE(k S)){return true;}
			if(CW1(k) && ISE(k W)){return true;}
			if(CE1(k) && ISE(k E)){return true;}
		}
	}
	while(pullMap.hasBits())
	{
	  loc_t k = pullMap.nextBit();
		if(CS1(k) && ISO(k S) && GT(k,k S))
		{
			if(CW1(k) && ISE(k W)){return true;}
			if(CE1(k) && ISE(k E)){return true;}
			if(CN1(k) && ISE(k N)){return true;}
		}
		if(CW1(k) && ISO(k W) && GT(k,k W))
		{
			if(CS1(k) && ISE(k S)){return true;}
			if(CE1(k) && ISE(k E)){return true;}
			if(CN1(k) && ISE(k N)){return true;}
		}
		if(CE1(k) && ISO(k E) && GT(k,k E))
		{
			if(CS1(k) && ISE(k S)){return true;}
			if(CW1(k) && ISE(k W)){return true;}
			if(CN1(k) && ISE(k N)){return true;}
		}
		if(CN1(k) && ISO(k N) && GT(k,k N))
		{
			if(CS1(k) && ISE(k S)){return true;}
			if(CW1(k) && ISE(k W)){return true;}
			if(CE1(k) && ISE(k E)){return true;}
		}
	}
	return false;
}

int BoardMoveGen::genSteps(const Board& b, pla_t pla, move_t* mv)
{
  return genStepsInto(b,pla,mv,Bitmap::BMPONES);
}

bool BoardMoveGen::canSteps(const Board& b, pla_t pla)
{
  pla_t opp = OPP(pla);
	Bitmap pieceMap = b.pieceMaps[pla][0];
	Bitmap allMap = b.pieceMaps[opp][0] | pieceMap;
	Bitmap plaRabMap = b.pieceMaps[pla][RAB];
	pieceMap &= ~b.frozenMap;

	if(pla == GOLD)
	{
		if((Bitmap::shiftN(pieceMap) & ~allMap).hasBits()) {return true;}
    if((Bitmap::shiftW(pieceMap) & ~allMap).hasBits()) {return true;}
    if((Bitmap::shiftE(pieceMap) & ~allMap).hasBits()) {return true;}
    if((Bitmap::shiftS(pieceMap & ~plaRabMap) & ~allMap).hasBits()) {return true;}
	}
	else //pla == SILV
	{
    if((Bitmap::shiftS(pieceMap) & ~allMap).hasBits()) {return true;}
    if((Bitmap::shiftW(pieceMap) & ~allMap).hasBits()) {return true;}
    if((Bitmap::shiftE(pieceMap) & ~allMap).hasBits()) {return true;}
    if((Bitmap::shiftN(pieceMap & ~plaRabMap) & ~allMap).hasBits()) {return true;}
	}
	return false;
}

bool BoardMoveGen::noMoves(const Board& b, pla_t pla)
{
  return !canSteps(b,pla) && !canPushPulls(b,pla);
}

int BoardMoveGen::genPushPullsInvolving(const Board& b, pla_t pla, move_t* mv, Bitmap relArea)
{
  pla_t opp = OPP(pla);

  Bitmap pushMap = b.pieceMaps[pla][0];
  pushMap &= ~b.frozenMap;           //Must be unfrozen
  pushMap &= ~b.pieceMaps[pla][RAB]; //Rabbits can't push
  pushMap = Bitmap::adj(pushMap);
  pushMap &= b.pieceMaps[opp][0];    //Must have opp to push
  pushMap &= ~b.pieceMaps[opp][ELE]; //Elephants can't be pushed
  pushMap &= Bitmap::adj(Bitmap::BMPONES & (~b.pieceMaps[pla][0]) & (~b.pieceMaps[opp][0])); //Must have empty space
  pushMap &= relArea | Bitmap::adj(relArea); //Must be adjacent to relevant area

  Bitmap pullMap = b.pieceMaps[opp][0];
  pullMap &= ~b.pieceMaps[opp][ELE]; //Elephants can't be pulled
  pullMap = Bitmap::adj(pullMap);
  pullMap &= b.pieceMaps[pla][0];    //Must have pla to pull
  pullMap &= ~b.frozenMap;           //Must be unfrozen
  pullMap &= ~b.pieceMaps[pla][RAB]; //Rabbits can't pull
  pullMap &= Bitmap::adj(Bitmap::BMPONES & (~b.pieceMaps[pla][0]) & (~b.pieceMaps[opp][0])); //Must have empty space
  pullMap &= relArea | Bitmap::adj(relArea); //Must be adjacent to relevant area

  int num = 0;
  while(pushMap.hasBits())
  {
    loc_t k = pushMap.nextBit();
    if(CS1(k) && ISP(k S) && GT(k S,k) && b.isThawed(k S))
    {
      if(CW1(k) && ISE(k W) && (relArea.isOne(k) || relArea.isOne(k S) || relArea.isOne(k W))){mv[num++] = Board::getMove(k MW,k S MN);}
      if(CE1(k) && ISE(k E) && (relArea.isOne(k) || relArea.isOne(k S) || relArea.isOne(k E))){mv[num++] = Board::getMove(k ME,k S MN);}
      if(CN1(k) && ISE(k N) && (relArea.isOne(k) || relArea.isOne(k S) || relArea.isOne(k N))){mv[num++] = Board::getMove(k MN,k S MN);}
    }
    if(CW1(k) && ISP(k W) && GT(k W,k) && b.isThawed(k W))
    {
      if(CS1(k) && ISE(k S) && (relArea.isOne(k) || relArea.isOne(k W) || relArea.isOne(k S))){mv[num++] = Board::getMove(k MS,k W ME);}
      if(CE1(k) && ISE(k E) && (relArea.isOne(k) || relArea.isOne(k W) || relArea.isOne(k E))){mv[num++] = Board::getMove(k ME,k W ME);}
      if(CN1(k) && ISE(k N) && (relArea.isOne(k) || relArea.isOne(k W) || relArea.isOne(k N))){mv[num++] = Board::getMove(k MN,k W ME);}
    }
    if(CE1(k) && ISP(k E) && GT(k E,k) && b.isThawed(k E))
    {
      if(CS1(k) && ISE(k S) && (relArea.isOne(k) || relArea.isOne(k E) || relArea.isOne(k S))){mv[num++] = Board::getMove(k MS,k E MW);}
      if(CW1(k) && ISE(k W) && (relArea.isOne(k) || relArea.isOne(k E) || relArea.isOne(k W))){mv[num++] = Board::getMove(k MW,k E MW);}
      if(CN1(k) && ISE(k N) && (relArea.isOne(k) || relArea.isOne(k E) || relArea.isOne(k N))){mv[num++] = Board::getMove(k MN,k E MW);}
    }
    if(CN1(k) && ISP(k N) && GT(k N,k) && b.isThawed(k N))
    {
      if(CS1(k) && ISE(k S) && (relArea.isOne(k) || relArea.isOne(k N) || relArea.isOne(k S))){mv[num++] = Board::getMove(k MS,k N MS);}
      if(CW1(k) && ISE(k W) && (relArea.isOne(k) || relArea.isOne(k N) || relArea.isOne(k W))){mv[num++] = Board::getMove(k MW,k N MS);}
      if(CE1(k) && ISE(k E) && (relArea.isOne(k) || relArea.isOne(k N) || relArea.isOne(k E))){mv[num++] = Board::getMove(k ME,k N MS);}
    }
  }
  while(pullMap.hasBits())
  {
    loc_t k = pullMap.nextBit();
    if(CS1(k) && ISO(k S) && GT(k,k S))
    {
      if(CW1(k) && ISE(k W) && (relArea.isOne(k) || relArea.isOne(k S) || relArea.isOne(k W))){mv[num++] = Board::getMove(k MW,k S MN);}
      if(CE1(k) && ISE(k E) && (relArea.isOne(k) || relArea.isOne(k S) || relArea.isOne(k E))){mv[num++] = Board::getMove(k ME,k S MN);}
      if(CN1(k) && ISE(k N) && (relArea.isOne(k) || relArea.isOne(k S) || relArea.isOne(k N))){mv[num++] = Board::getMove(k MN,k S MN);}
    }
    if(CW1(k) && ISO(k W) && GT(k,k W))
    {
      if(CS1(k) && ISE(k S) && (relArea.isOne(k) || relArea.isOne(k W) || relArea.isOne(k S))){mv[num++] = Board::getMove(k MS,k W ME);}
      if(CE1(k) && ISE(k E) && (relArea.isOne(k) || relArea.isOne(k W) || relArea.isOne(k E))){mv[num++] = Board::getMove(k ME,k W ME);}
      if(CN1(k) && ISE(k N) && (relArea.isOne(k) || relArea.isOne(k W) || relArea.isOne(k N))){mv[num++] = Board::getMove(k MN,k W ME);}
    }
    if(CE1(k) && ISO(k E) && GT(k,k E))
    {
      if(CS1(k) && ISE(k S) && (relArea.isOne(k) || relArea.isOne(k E) || relArea.isOne(k S))){mv[num++] = Board::getMove(k MS,k E MW);}
      if(CW1(k) && ISE(k W) && (relArea.isOne(k) || relArea.isOne(k E) || relArea.isOne(k W))){mv[num++] = Board::getMove(k MW,k E MW);}
      if(CN1(k) && ISE(k N) && (relArea.isOne(k) || relArea.isOne(k E) || relArea.isOne(k N))){mv[num++] = Board::getMove(k MN,k E MW);}
    }
    if(CN1(k) && ISO(k N) && GT(k,k N))
    {
      if(CS1(k) && ISE(k S) && (relArea.isOne(k) || relArea.isOne(k N) || relArea.isOne(k S))){mv[num++] = Board::getMove(k MS,k N MS);}
      if(CW1(k) && ISE(k W) && (relArea.isOne(k) || relArea.isOne(k N) || relArea.isOne(k W))){mv[num++] = Board::getMove(k MW,k N MS);}
      if(CE1(k) && ISE(k E) && (relArea.isOne(k) || relArea.isOne(k N) || relArea.isOne(k E))){mv[num++] = Board::getMove(k ME,k N MS);}
    }
  }
  return num;
}

int BoardMoveGen::genStepsIntoOutTSWF(const Board& b, pla_t pla, move_t* mv, Bitmap relArea)
{
  Bitmap pieceMap = b.pieceMaps[pla][0];
  pieceMap &= ~b.frozenMap;

  Bitmap inMap = pieceMap & relArea;
  Bitmap outMap = pieceMap & Bitmap::adj(relArea) & (~inMap);
  int num = 0;
  while(inMap.hasBits())
  {
  	loc_t loc = inMap.nextBit();
  	if(CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc) && (relArea.isOne(loc S) || !b.isTrapSafe1(pla,loc) || (b.isDominated(loc) && !b.isGuarded2(pla,loc)))) mv[num++] = Board::getMove(loc MS);
  	if(CW1(loc) && ISE(loc W)                          && (relArea.isOne(loc W) || !b.isTrapSafe1(pla,loc) || (b.isDominated(loc) && !b.isGuarded2(pla,loc)))) mv[num++] = Board::getMove(loc MW);
  	if(CE1(loc) && ISE(loc E)                          && (relArea.isOne(loc E) || !b.isTrapSafe1(pla,loc) || (b.isDominated(loc) && !b.isGuarded2(pla,loc)))) mv[num++] = Board::getMove(loc ME);
  	if(CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc) && (relArea.isOne(loc N) || !b.isTrapSafe1(pla,loc) || (b.isDominated(loc) && !b.isGuarded2(pla,loc)))) mv[num++] = Board::getMove(loc MN);
  }
  while(outMap.hasBits())
  {
  	loc_t loc = outMap.nextBit();
  	if(CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc) && relArea.isOne(loc S)) mv[num++] = Board::getMove(loc MS);
  	if(CW1(loc) && ISE(loc W)                          && relArea.isOne(loc W)) mv[num++] = Board::getMove(loc MW);
  	if(CE1(loc) && ISE(loc E)                          && relArea.isOne(loc E)) mv[num++] = Board::getMove(loc ME);
  	if(CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc) && relArea.isOne(loc N)) mv[num++] = Board::getMove(loc MN);
  }
  return num;
}

int BoardMoveGen::genStepsInvolving(const Board& b, pla_t pla, move_t* mv, Bitmap relArea)
{
  pla_t opp = OPP(pla);
  Bitmap pieceMap = b.pieceMaps[pla][0];
  Bitmap allMap = b.pieceMaps[opp][0] | pieceMap;
  Bitmap plaRabMap = b.pieceMaps[pla][RAB];
  pieceMap &= ~b.frozenMap;

  int num = 0;
  if(pla == GOLD)
  {
    Bitmap mp;
    mp = Bitmap::shiftN(pieceMap) & ~allMap & (relArea | Bitmap::shiftN(relArea));              while(mp.hasBits()) {loc_t loc = mp.nextBit()-8; mv[num++] = Board::getMove(loc MN);}
    mp = Bitmap::shiftW(pieceMap) & ~allMap & (relArea | Bitmap::shiftW(relArea));              while(mp.hasBits()) {loc_t loc = mp.nextBit()+1; mv[num++] = Board::getMove(loc MW);}
    mp = Bitmap::shiftE(pieceMap) & ~allMap & (relArea | Bitmap::shiftE(relArea));              while(mp.hasBits()) {loc_t loc = mp.nextBit()-1; mv[num++] = Board::getMove(loc ME);}
    mp = Bitmap::shiftS(pieceMap & ~plaRabMap) & ~allMap & (relArea | Bitmap::shiftS(relArea)); while(mp.hasBits()) {loc_t loc = mp.nextBit()+8; mv[num++] = Board::getMove(loc MS);}
  }
  else //pla == SILV
  {
    Bitmap mp;
    mp = Bitmap::shiftS(pieceMap) & ~allMap & (relArea | Bitmap::shiftS(relArea));              while(mp.hasBits()) {loc_t loc = mp.nextBit()+8; mv[num++] = Board::getMove(loc MS);}
    mp = Bitmap::shiftW(pieceMap) & ~allMap & (relArea | Bitmap::shiftW(relArea));              while(mp.hasBits()) {loc_t loc = mp.nextBit()+1; mv[num++] = Board::getMove(loc MW);}
    mp = Bitmap::shiftE(pieceMap) & ~allMap & (relArea | Bitmap::shiftE(relArea));              while(mp.hasBits()) {loc_t loc = mp.nextBit()-1; mv[num++] = Board::getMove(loc ME);}
    mp = Bitmap::shiftN(pieceMap & ~plaRabMap) & ~allMap & (relArea | Bitmap::shiftN(relArea)); while(mp.hasBits()) {loc_t loc = mp.nextBit()-8; mv[num++] = Board::getMove(loc MN);}
  }
  return num;
}

int BoardMoveGen::genStepsInto(const Board& b, pla_t pla, move_t* mv, Bitmap relArea)
{
  pla_t opp = OPP(pla);
  Bitmap pieceMap = b.pieceMaps[pla][0];
  Bitmap empAndRel = ~(b.pieceMaps[opp][0] | pieceMap) & relArea;
  Bitmap plaRabMap = b.pieceMaps[pla][RAB];
  pieceMap &= ~b.frozenMap;

  int num = 0;
  if(pla == GOLD)
  {
    Bitmap mp;
    mp = Bitmap::shiftN(pieceMap) & empAndRel;              while(mp.hasBits()) {loc_t loc = mp.nextBit() S; mv[num++] = Board::getMove(loc MN);}
    mp = Bitmap::shiftW(pieceMap) & empAndRel;              while(mp.hasBits()) {loc_t loc = mp.nextBit() E; mv[num++] = Board::getMove(loc MW);}
    mp = Bitmap::shiftE(pieceMap) & empAndRel;              while(mp.hasBits()) {loc_t loc = mp.nextBit() W; mv[num++] = Board::getMove(loc ME);}
    mp = Bitmap::shiftS(pieceMap & ~plaRabMap) & empAndRel; while(mp.hasBits()) {loc_t loc = mp.nextBit() N; mv[num++] = Board::getMove(loc MS);}
  }
  else //pla == SILV
  {
    Bitmap mp;
    mp = Bitmap::shiftS(pieceMap) & empAndRel;              while(mp.hasBits()) {loc_t loc = mp.nextBit() N; mv[num++] = Board::getMove(loc MS);}
    mp = Bitmap::shiftW(pieceMap) & empAndRel;              while(mp.hasBits()) {loc_t loc = mp.nextBit() E; mv[num++] = Board::getMove(loc MW);}
    mp = Bitmap::shiftE(pieceMap) & empAndRel;              while(mp.hasBits()) {loc_t loc = mp.nextBit() W; mv[num++] = Board::getMove(loc ME);}
    mp = Bitmap::shiftN(pieceMap & ~plaRabMap) & empAndRel; while(mp.hasBits()) {loc_t loc = mp.nextBit() S; mv[num++] = Board::getMove(loc MN);}
  }
  return num;
}

//HIGHER LEVEL MOVE GEN ----------------------------------------------------------------------------
/*
static int genWalkToRec(const Board& b, pla_t pla, int numSteps, loc_t ploc, int dx, int dy, int dxTotal, int dyTotal, int destCost, move_t* mv)
{
	if(dxTotal + dyTotal == 0)
	{
		mv[0] = ERRORMOVE;
		return 1;
	}

	bool isFrozen = b.isFrozenC(ploc);
	if(dxTotal + dyTotal + isFrozen + destCost > numSteps)
		return 0;

	if(dxTotal > 0)
	{
		loc_t dest = ploc+dx;
		if(isFrozen(pla))
		{

		}
		if(b.owners[dest] == NPLA && !isFrozen && b.isTrapSafe2(ploc,dest))
		{
			TempRecord rec = b.tempStepC(ploc,dest);
			int num = genWalkToRec(b, pla, numSteps-1, dest, dx, dy, dxTotal-1, dyTotal, destCost, mv);
			b.undoTemp(rec);
			if(num > 0)	{mv[0] = Board::preConcatMove(Board::STEPINDEX[ploc][dest],mv[0]); return 1;}
		}
		else if(b.owners[dest])
	}
}

int BoardMoveGen::genWalkTo(const Board& b, pla_t pla, int numSteps, loc_t ploc, loc_t dest, move_t* mv)
{
	int baseDist = Board::MANHATTANDIST[ploc][dest] + b.isFrozen(ploc);
	if(baseDist > numSteps)
		return 0;

}

*/




