
/*
 * boardcapdef.cpp
 * Author: davidwu
 *
 * Capture defense move generation functions
 */
#include "pch.h"

#include <iostream>
#include "global.h"
#include "bitmap.h"
#include "board.h"
#include "boardmovegen.h"
#include "boardtrees.h"
#include "boardtreeconst.h"
#include "threats.h"

using namespace std;

int BoardTrees::genRunawayDefs(Board& b, pla_t pla, loc_t ploc, int numSteps, move_t* mv)
{
	int num = 0;
	pla_t opp = OPP(pla);
	if(b.isThawed(ploc))
	{
		//Step each possible direction that isn't suicide, push through if blocked
		if(CS1(ploc) && b.isRabOkayS(pla,ploc))
		{
			if(ISE(ploc S) && b.isTrapSafe2(pla,ploc S)) {mv[num++] = Board::getMove(ploc MS);}
			else if(ISP(ploc S) && numSteps >= 2 && b.isTrapSafe2(pla,ploc) && b.wouldBeUF(pla,ploc,ploc,ploc S))
			{
				loc_t loc = ploc S;
				if(CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) {mv[num++] = Board::getMove(loc MS, ploc MS);}
				if(CW1(loc) && ISE(loc W))                          {mv[num++] = Board::getMove(loc MW, ploc MS);}
				if(CE1(loc) && ISE(loc E))                          {mv[num++] = Board::getMove(loc ME, ploc MS);}
				if(CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)) {mv[num++] = Board::getMove(loc MN, ploc MS);}
			}
			else if(ISO(ploc S) && numSteps >= 2 && GT(ploc,ploc S))
			{
				loc_t loc = ploc S;
				if(CS1(loc) && ISE(loc S)) {mv[num++] = Board::getMove(loc MS, ploc MS);}
				if(CW1(loc) && ISE(loc W)) {mv[num++] = Board::getMove(loc MW, ploc MS);}
				if(CE1(loc) && ISE(loc E)) {mv[num++] = Board::getMove(loc ME, ploc MS);}
				if(CN1(loc) && ISE(loc N)) {mv[num++] = Board::getMove(loc MN, ploc MS);}
			}
		}
		if(CW1(ploc))
		{
			if(ISE(ploc W) && b.isTrapSafe2(pla,ploc W)) {mv[num++] = Board::getMove(ploc MW);}
			else if(ISP(ploc W) && numSteps >= 2 && b.isTrapSafe2(pla,ploc) && b.wouldBeUF(pla,ploc,ploc,ploc W))
			{
				loc_t loc = ploc W;
				if(CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) {mv[num++] = Board::getMove(loc MS, ploc MW);}
				if(CW1(loc) && ISE(loc W))                          {mv[num++] = Board::getMove(loc MW, ploc MW);}
				if(CE1(loc) && ISE(loc E))                          {mv[num++] = Board::getMove(loc ME, ploc MW);}
				if(CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)) {mv[num++] = Board::getMove(loc MN, ploc MW);}
			}
			else if(ISO(ploc W) && numSteps >= 2 && GT(ploc,ploc W))
			{
				loc_t loc = ploc W;
				if(CS1(loc) && ISE(loc S)) {mv[num++] = Board::getMove(loc MS, ploc MW);}
				if(CW1(loc) && ISE(loc W)) {mv[num++] = Board::getMove(loc MW, ploc MW);}
				if(CE1(loc) && ISE(loc E)) {mv[num++] = Board::getMove(loc ME, ploc MW);}
				if(CN1(loc) && ISE(loc N)) {mv[num++] = Board::getMove(loc MN, ploc MW);}
			}
		}
		if(CE1(ploc))
		{
			if(ISE(ploc E) && b.isTrapSafe2(pla,ploc E)) {mv[num++] = Board::getMove(ploc ME);}
			else if(ISP(ploc E) && numSteps >= 2 && b.isTrapSafe2(pla,ploc) && b.wouldBeUF(pla,ploc,ploc,ploc E))
			{
				loc_t loc = ploc E;
				if(CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) {mv[num++] = Board::getMove(loc MS, ploc ME);}
				if(CW1(loc) && ISE(loc W))                          {mv[num++] = Board::getMove(loc MW, ploc ME);}
				if(CE1(loc) && ISE(loc E))                          {mv[num++] = Board::getMove(loc ME, ploc ME);}
				if(CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)) {mv[num++] = Board::getMove(loc MN, ploc ME);}
			}
			else if(ISO(ploc E) && numSteps >= 2 && GT(ploc,ploc E))
			{
				loc_t loc = ploc E;
				if(CS1(loc) && ISE(loc S)) {mv[num++] = Board::getMove(loc MS, ploc ME);}
				if(CW1(loc) && ISE(loc W)) {mv[num++] = Board::getMove(loc MW, ploc ME);}
				if(CE1(loc) && ISE(loc E)) {mv[num++] = Board::getMove(loc ME, ploc ME);}
				if(CN1(loc) && ISE(loc N)) {mv[num++] = Board::getMove(loc MN, ploc ME);}
			}
		}
		if(CN1(ploc) && b.isRabOkayN(pla,ploc))
		{
			if(ISE(ploc N) && b.isTrapSafe2(pla,ploc N)) {mv[num++] = Board::getMove(ploc MN);}
			else if(ISP(ploc N) && numSteps >= 2 && b.isTrapSafe2(pla,ploc) && b.wouldBeUF(pla,ploc,ploc,ploc N))
			{
				loc_t loc = ploc N;
				if(CS1(loc) && ISE(loc S) && b.isRabOkayS(pla,loc)) {mv[num++] = Board::getMove(loc MS, ploc MN);}
				if(CW1(loc) && ISE(loc W))                          {mv[num++] = Board::getMove(loc MW, ploc MN);}
				if(CE1(loc) && ISE(loc E))                          {mv[num++] = Board::getMove(loc ME, ploc MN);}
				if(CN1(loc) && ISE(loc N) && b.isRabOkayN(pla,loc)) {mv[num++] = Board::getMove(loc MN, ploc MN);}
			}
			else if(ISO(ploc N) && numSteps >= 2 && GT(ploc,ploc N))
			{
				loc_t loc = ploc N;
				if(CS1(loc) && ISE(loc S)) {mv[num++] = Board::getMove(loc MS, ploc MN);}
				if(CW1(loc) && ISE(loc W)) {mv[num++] = Board::getMove(loc MW, ploc MN);}
				if(CE1(loc) && ISE(loc E)) {mv[num++] = Board::getMove(loc ME, ploc MN);}
				if(CN1(loc) && ISE(loc N)) {mv[num++] = Board::getMove(loc MN, ploc MN);}
			}
		}

	}
	//Frozen
	else if(numSteps > 1)
	{
		Bitmap map = b.pieceMaps[pla][0] & Board::DISK[numSteps][ploc];
		while(map.hasBits())
		{
			loc_t hloc = map.nextBit();
			if(Board::MANHATTANDIST[ploc][hloc] == numSteps && b.isFrozen(hloc))
				continue;

			Bitmap simpleWalkLocs;
			num += genDefsUsing(b,pla,hloc,ploc,numSteps-1,mv+num,ERRORMOVE,0,true,simpleWalkLocs);
		}
	}

	return num;
}

int BoardTrees::shortestGoodTrapDef(Board& b, pla_t pla, loc_t kt, move_t* mv, int num, const Bitmap& currentWholeCapMap)
{
	pla_t opp = OPP(pla);
	int shortestLen = 5;
	for(int i = 0; i<num; i++)
	{
		move_t move = mv[i];

		//Make sure the move is shorter than the best
		int ns = Board::numStepsInMove(move);
		if(ns >= shortestLen)
			continue;

		//Try the move
		Board copy = b;
		copy.makeMove(move);

		//Make we didn't commit suicide
		if(copy.pieceCounts[pla][0] < b.pieceCounts[pla][0])
			continue;

		//Make sure nothing here is captureable now and no goal is possible at all
		if(BoardTrees::canCaps(copy,opp,4,2,kt) || BoardTrees::goalDist(copy,opp,4) < 5)
			continue;

		//Make sure nothing *new* is captureable elsewhere
		bool newCapture = false;
		for(int j = 0; j<4; j++)
		{
			loc_t otherkt = Board::TRAPLOCS[j];
			if(otherkt == kt)
				continue;
			Bitmap capMap;
			int capDist;
			if(BoardTrees::canCaps(copy,opp,4,kt,capMap,capDist))
			{
				if((capMap & ~currentWholeCapMap).hasBits())
				{newCapture = true; break;}
			}
		}
		if(!newCapture)
			shortestLen = ns;
	}
	return shortestLen;
}


int BoardTrees::genTrapDefs(Board& b, pla_t pla, loc_t kt, int numSteps, const Bitmap pStrongerMaps[2][NUMTYPES], move_t* mv)
{
	int num = 0;
	pla_t opp = OPP(pla);
	int numGuards = b.trapGuardCounts[pla][Board::TRAPINDEX[kt]];
	if(numGuards == 0)
	{
		//Find how strong opp is near this trap
		int oppStrRad2 = ELE;
		while(oppStrRad2 > RAB && (b.pieceMaps[opp][oppStrRad2] & Board::DISK[3][kt]).isEmpty())
			oppStrRad2--;

		//Find defenders
		Bitmap strongMap = pStrongerMaps[opp][oppStrRad2-1] & Board::DISK[numSteps+1][kt];
		Bitmap weakMap = b.pieceMaps[pla][0] & Board::DISK[numSteps][kt];
		weakMap &= ~strongMap;

		//Defense by strong pieces
		while(strongMap.hasBits())
		{
			loc_t ploc = strongMap.nextBit();
			if(Board::MANHATTANDIST[kt][ploc] == numSteps+1 && b.isFrozen(ploc))
				continue;

			Bitmap simpleWalkLocs;
			num += genDefsUsing(b,pla,ploc,kt,numSteps,mv+num,ERRORMOVE,0,true,simpleWalkLocs);
		}
		//Defense by weak pieces
		while(weakMap.hasBits())
		{
			loc_t ploc = weakMap.nextBit();
			if(Board::MANHATTANDIST[kt][ploc] == numSteps && b.isFrozen(ploc))
				continue;
			Bitmap simpleWalkLocs;
			move_t tempmv[32];
			int tempNum = genDefsUsing(b,pla,ploc,kt,numSteps-1,tempmv,ERRORMOVE,0,true,simpleWalkLocs);

			//Try each defense and see if it works by itself
			for(int i = 0; i<tempNum; i++)
			{
				Board copy = b;
				copy.makeMove(tempmv[i]);
				if(!BoardTrees::canCaps(copy,opp,4,2,kt))
					mv[num++] = tempmv[i];
				//It failed. So let's see if we can add more defense to this trap
				else
				{
					int nsInMove =  Board::numStepsInMove(tempmv[i]);
					int numStepsLeft = numSteps - nsInMove;
					if(numStepsLeft <= 0)
						continue;
					int tempTempNum = genTrapDefs(copy,pla,kt,numStepsLeft,pStrongerMaps,mv+num);
					for(int j = 0; j<tempTempNum; j++)
					{
						mv[num+j] = Board::concatMoves(tempmv[i],mv[num+j],nsInMove);
					}
					num += tempTempNum;
				}
			}
		}

	}
	//NOTE THAT THE BELOW CANNOT USE pStrongerMaps without invalidating the recurstion a few lines above.
	else if(numGuards >= 1)
	{
		Bitmap map = b.pieceMaps[pla][0] & Board::DISK[numSteps+1][kt] & ~Board::RADIUS[1][kt];
		while(map.hasBits())
		{
			loc_t ploc = map.nextBit();
			if(Board::MANHATTANDIST[kt][ploc] == numSteps+1 && b.isFrozen(ploc))
				continue;
			Bitmap simpleWalkLocs;
			num += genDefsUsing(b,pla,ploc,kt,numSteps,mv+num,ERRORMOVE,0,true,simpleWalkLocs);
		}
	}
	return num;
}




//Generate moves that defend a target location
int BoardTrees::genDefsUsing(Board& b, pla_t pla, loc_t ploc, loc_t targ, int numSteps, move_t* mv,
		move_t moveSoFar, int stepsSoFar, bool simpleWalk, Bitmap& simpleWalkLocs, bool simpleWalkIsGood)
{
  int num = 0;
  pla_t opp = OPP(pla);

  int plocTargMDist = Board::MANHATTANDIST[ploc][targ];
  if(plocTargMDist == 1)
  	return 0;

  if(simpleWalk)
  	simpleWalkLocs.setOn(ploc);

  //If right next to the target and UF, directly gen the moves to defend
  if((plocTargMDist == 0 || plocTargMDist == 2) && b.isThawedC(ploc))
  {
		//Try each direction
    for(int dir = 0; dir < 4; dir++)
    {
      if(!Board::ADJOKAY[dir][ploc])
        continue;

      loc_t dest = ploc + Board::ADJOFFSETS[dir];
      if(Board::MANHATTANDIST[targ][dest] != 1 || !b.isRabOkay(pla,ploc,dest))
        continue;

      step_t step = ploc + Board::STEPOFFSETS[dir];

      //Step directly?
      if(ISE(dest))
			{
      	if(simpleWalk && (simpleWalkLocs.isOne(dest) || !simpleWalkIsGood))
      		continue;
      	mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(step),stepsSoFar);
      	if(simpleWalk)
      		simpleWalkLocs.setOn(dest);
      }
      //Player in the way - only defend if all safe and we are stronger than the piece we're replacing and we're not saccing the piece.
      else if(ISP(dest) && numSteps >= 2 && GT(ploc,dest) && b.isTrapSafe2(pla,ploc) && b.wouldBeUF(pla,ploc,ploc,dest))
      {
        if(CS1(dest) && ISE(dest S) && b.isTrapSafe2(pla,dest S) && b.isRabOkayS(pla,dest)) mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(dest MS,step),stepsSoFar);
        if(CW1(dest) && ISE(dest W) && b.isTrapSafe2(pla,dest W))                           mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(dest MW,step),stepsSoFar);
        if(CE1(dest) && ISE(dest E) && b.isTrapSafe2(pla,dest E))                           mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(dest ME,step),stepsSoFar);
        if(CN1(dest) && ISE(dest N) && b.isTrapSafe2(pla,dest N) && b.isRabOkayN(pla,dest)) mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(dest MN,step),stepsSoFar);
      }
      //Opp in the way
      else if(ISO(dest) && numSteps >= 2 && GT(ploc,dest))
      {
        if(CS1(dest) && ISE(dest S)) mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(dest MS,step),stepsSoFar);
        if(CW1(dest) && ISE(dest W)) mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(dest MW,step),stepsSoFar);
        if(CE1(dest) && ISE(dest E)) mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(dest ME,step),stepsSoFar);
        if(CN1(dest) && ISE(dest N)) mv[num++] = Board::concatMoves(moveSoFar,Board::getMove(dest MN,step),stepsSoFar);
      }
    }
  }

  //If we are frozen, try 1-step unfreezes
  if(b.isFrozenC(ploc))
  {
  	if(numSteps < plocTargMDist)
  		return num;

  	//For each direction
    for(int dir = 0; dir < 4; dir++)
    {
      if(!Board::ADJOKAY[dir][ploc])
        continue;

      //If it's empty, we can step someone in
      loc_t hloc = ploc + Board::ADJOFFSETS[dir];
      if(!ISE(hloc))
      	continue;

      //Look for pieces to step in
      for(int dir2 = 0; dir2 < 4; dir2++)
      {
        if(!Board::ADJOKAY[dir2][hloc])
          continue;
        loc_t hloc2 = hloc + Board::ADJOFFSETS[dir2];
        if(hloc2 == ploc) continue;
        if(ISP(hloc2) && b.isThawedC(hloc2) && b.isRabOkay(pla,hloc2,hloc))
        {
          TSC(hloc2,hloc,num += genDefsUsing(b,pla,ploc,targ,numSteps-1,mv+num,
          		Board::concatMoves(moveSoFar,Board::getMove(Board::STEPINDEX[hloc2][hloc]),stepsSoFar),stepsSoFar+1,
          		false,simpleWalkLocs,simpleWalkIsGood))
        }
      }
    }

    return num;
  }

  if(numSteps < plocTargMDist-1 || plocTargMDist == 0)
  	return num;

  //Try to step in each profitable direction.
  int bestDir = 0;
  int worstDir1 = -1;
  int worstDir2 = -1;
  if(targ != ploc)
  {bestDir = Board::getApproachDir(ploc,targ); Board::getRetreatDirs(ploc,targ,worstDir1,worstDir2);}

  int dirs[4] = {0,1,2,3};
  dirs[bestDir] = 0;
  dirs[0] = bestDir;
  for(int i = 0; i < 4; i++)
  {
  	int dir = dirs[i];
    if(!Board::ADJOKAY[dir][ploc] || dir == worstDir1 || dir == worstDir2)
      continue;
    loc_t next = ploc + Board::ADJOFFSETS[dir];
    if(Board::MANHATTANDIST[targ][next] == 1 || !b.isRabOkay(pla,ploc,next))
    	continue;
    step_t step = ploc + Board::STEPOFFSETS[dir];

    //Next loc is empty...
    if(ISE(next))
    {
    	if(simpleWalk && simpleWalkLocs.isOne(next))
    		continue;

    	//Here we go!
    	if(b.isTrapSafe2(pla,next))
    	{
    		TSC(ploc,next,num += genDefsUsing(b,pla,next,targ,numSteps-1,mv+num,
    											Board::concatMoves(moveSoFar,Board::getMove(step),stepsSoFar),stepsSoFar+1,
    											simpleWalk,simpleWalkLocs,simpleWalkIsGood))
    	}
    	//Trapunsafe? Defend trap!
    	else
    	{
    		if(numSteps < plocTargMDist)
    			continue;

				//For each direction look for defenders
				for(int dir1 = 0; dir1 < 4; dir1++)
				{
					if(!Board::ADJOKAY[dir1][next])
						continue;

					//If it's empty, we can step someone in
					loc_t hloc = next + Board::ADJOFFSETS[dir1];
					if(!ISE(hloc))
						continue;

					//Look for pieces to step in
					for(int dir2 = 0; dir2 < 4; dir2++)
					{
						if(!Board::ADJOKAY[dir2][hloc])
							continue;
						loc_t hloc2 = hloc + Board::ADJOFFSETS[dir2];
						if(hloc2 == next) continue;
						if(ISP(hloc2) && b.isThawedC(hloc2) && b.isRabOkay(pla,hloc2,hloc))
						{
							TSC(hloc2,hloc,num += genDefsUsing(b,pla,ploc,targ,numSteps-1,mv+num,
									Board::concatMoves(moveSoFar,Board::getMove(Board::STEPINDEX[hloc2][hloc]),stepsSoFar),stepsSoFar+1,
									false,simpleWalkLocs,simpleWalkIsGood))
						}
					}
				}
    	}
    }
		//Player in the way
		else if(ISP(next) && numSteps >= plocTargMDist+1 && b.isTrapSafe2(pla,ploc) && b.wouldBeUF(pla,ploc,ploc,next))
		{
			if(CS1(next) && ISE(next S) && b.isRabOkayS(pla,next)) {TPC(ploc,next,next S, num += genDefsUsing(b,pla,next,targ,numSteps-2,mv+num, Board::concatMoves(moveSoFar,Board::getMove(next MS,step),stepsSoFar),stepsSoFar+2,false,simpleWalkLocs,simpleWalkIsGood));}
			if(CW1(next) && ISE(next W))                           {TPC(ploc,next,next W, num += genDefsUsing(b,pla,next,targ,numSteps-2,mv+num, Board::concatMoves(moveSoFar,Board::getMove(next MW,step),stepsSoFar),stepsSoFar+2,false,simpleWalkLocs,simpleWalkIsGood));}
			if(CE1(next) && ISE(next E))                           {TPC(ploc,next,next E, num += genDefsUsing(b,pla,next,targ,numSteps-2,mv+num, Board::concatMoves(moveSoFar,Board::getMove(next ME,step),stepsSoFar),stepsSoFar+2,false,simpleWalkLocs,simpleWalkIsGood));}
			if(CN1(next) && ISE(next N) && b.isRabOkayN(pla,next)) {TPC(ploc,next,next N, num += genDefsUsing(b,pla,next,targ,numSteps-2,mv+num, Board::concatMoves(moveSoFar,Board::getMove(next MN,step),stepsSoFar),stepsSoFar+2,false,simpleWalkLocs,simpleWalkIsGood));}
		}
		//Opp in the way
		else if(ISO(next) && numSteps >= plocTargMDist+1 && GT(ploc,next) && b.isTrapSafe2(pla,next))
		{
			if(CS1(next) && ISE(next S)) {TPC(ploc,next,next S, num += genDefsUsing(b,pla,next,targ,numSteps-2,mv+num, Board::concatMoves(moveSoFar,Board::getMove(next MS,step),stepsSoFar),stepsSoFar+2,false,simpleWalkLocs,simpleWalkIsGood));}
			if(CW1(next) && ISE(next W)) {TPC(ploc,next,next W, num += genDefsUsing(b,pla,next,targ,numSteps-2,mv+num, Board::concatMoves(moveSoFar,Board::getMove(next MW,step),stepsSoFar),stepsSoFar+2,false,simpleWalkLocs,simpleWalkIsGood));}
			if(CE1(next) && ISE(next E)) {TPC(ploc,next,next E, num += genDefsUsing(b,pla,next,targ,numSteps-2,mv+num, Board::concatMoves(moveSoFar,Board::getMove(next ME,step),stepsSoFar),stepsSoFar+2,false,simpleWalkLocs,simpleWalkIsGood));}
			if(CN1(next) && ISE(next N)) {TPC(ploc,next,next N, num += genDefsUsing(b,pla,next,targ,numSteps-2,mv+num, Board::concatMoves(moveSoFar,Board::getMove(next MN,step),stepsSoFar),stepsSoFar+2,false,simpleWalkLocs,simpleWalkIsGood));}
		}
  }
  return num;
}

/*
int BoardTrees::genGoalThreats(Board& b, pla_t pla, int numSteps, move_t* mv)
{
	int goalDistEst[64];
	Threats::getGoalDistEst(b,pla,goalDistEst,numSteps+4);

	Bitmap relevant;
	for(int i = 0; i<64; i++)
	{
		if(goalDistEst[i] <= 7)
			relevant.setOn(i);
	}
	relevant |= Bitmap::adj(relevant);

	Bitmap rabRadMap;
	Bitmap rabMap = b.pieceMaps[pla][RAB];
	while(rabMap.hasBits())
	{
		loc_t rabLoc = rabMap.nextBit();
		if(Board::GOALYDIST[rabLoc] <= 0)
			continue;
		if(goalDistEst[rabLoc] + (int)(b.isFrozen(rabLoc)) > numSteps+4)
			continue;
		loc_t inFront = Board::ADVANCE_OFFSET[pla] + rabLoc;
		rabRadMap |= Board::DISK[numSteps+1][inFront];
	}
	relevant &= rabRadMap;

	return genGoalThreatsRec(b,pla,relevant,numSteps,mv,ERRORMOVE,0);
}

int BoardTrees::genGoalThreatsRec(Board& b, pla_t pla, Bitmap relevant, int numStepsLeft, move_t* mv,
		move_t moveSoFar, int numStepsSoFar)
{
	int num = 0;
	if(numStepsLeft == 0)
	{
		if(BoardTrees::goalDist(b,pla,4) <= 4)
			mv[num++] = moveSoFar;
		return num;
	}

	if(numStepsSoFar > 0 && BoardTrees::goalDist(b,pla,4) <= 4)
		mv[num++] = moveSoFar;

	int numMoves = 0;
	move_t moves[256];
	if(numStepsLeft >= 1)
		numMoves += BoardMoveGen::genStepsInvolving(b,pla,moves+numMoves,relevant);
	if(numStepsLeft >= 2)
		numMoves += BoardMoveGen::genPushPullsInvolving(b,pla,moves+numMoves,relevant);

	for(int i = 0; i<numMoves; i++)
	{
		move_t move = moves[i];
		int ns = Board::numStepsInMove(move);

		step_t lastStep = Board::getStep(move,ns-1);

		Bitmap newRelevant = Board::RADIUS[1][Board::K0INDEX[lastStep]] | Board::RADIUS[1][Board::K1INDEX[lastStep]];
		if(numStepsLeft-ns > 0)
			newRelevant |= Bitmap::adj(newRelevant & Bitmap::BMPTRAPS);

		Board copy = b;
		copy.makeMove(move);
		num += genGoalThreatsRec(copy,pla,newRelevant,numStepsLeft-ns,mv+num,Board::concatMoves(moveSoFar,move,numStepsSoFar),numStepsSoFar+ns);

	}

	return num;
}
*/






