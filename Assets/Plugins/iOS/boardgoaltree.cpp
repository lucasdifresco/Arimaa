
/*
 * boardgoaltree.cpp
 * Author: davidwu
 *
 * Goal detection functions
 */
#include "pch.h"

#include <iostream>
#include "bitmap.h"
#include "board.h"
#include "boardtrees.h"
#include "boardtreeconst.h"

using namespace std;

//Debug option - perform a board check consistency upon entry into goal tree
//(such as to detect whether we are in the middle of a tempstep by the caller code)
//#define CHECK_GOALTREE_CONSISTENCY

static bool canGoal1S(Board& b, pla_t pla, loc_t rloc);
static bool canGoal2S(Board& b, pla_t pla, loc_t rloc);
static bool canGoal3S(Board& b, pla_t pla, loc_t rloc);
static bool canGoal4S(Board& b, pla_t pla, loc_t rloc);

static bool canUF(Board& b, pla_t pla, loc_t ploc, loc_t floc);
static bool canUFAtAndGoal(Board& b, pla_t pla, loc_t dest, loc_t ploc, bool (*gfunc)(Board&,pla_t,loc_t));
static bool canUFCAtAndGoal(Board& b, pla_t pla, loc_t dest, loc_t ploc, bool (*gfunc)(Board&,pla_t,loc_t));
static bool canTrapDefAtAndRabStepWEGoal2S(Board& b, pla_t pla, loc_t adjtrap, loc_t kt, loc_t rloc);
static bool canSafeifyAndGoal(Board& b, pla_t pla, loc_t kt, loc_t rloc, loc_t floc, loc_t floc2, loc_t radjloc, loc_t fadjloc);
static bool canUF2(Board& b, pla_t pla, loc_t ploc, loc_t floc, loc_t floc2);
static bool canPushg(Board& b, pla_t pla, loc_t eloc);
static bool canPullg(Board& b, pla_t pla, loc_t eloc);
static bool canPPCapAndUFg(Board& b, pla_t pla, loc_t eloc, loc_t ploc);
static bool canSwapE2S(Board& b, pla_t pla, loc_t loc, loc_t ploc, loc_t floc);
static bool canStepStepg(Board& b, pla_t pla, loc_t dest, loc_t dest2);
static bool canPushAndGoal2SC(Board& b, pla_t pla, loc_t eloc, loc_t rloc);
static bool canSwapEandGoal2S(Board& b, pla_t pla, loc_t loc, loc_t ploc, loc_t floc);
static bool canUF2AndGoal2S(Board& b, pla_t pla, loc_t ploc, loc_t floc);
static bool canUFStepGoal2S(Board& b, pla_t pla, loc_t ploc, loc_t loc, loc_t rloc, loc_t floc);
static bool canUF2ForceAndGoal2S(Board& b, pla_t pla, loc_t ploc, loc_t floc);
static bool canPushEndUF(Board& b, pla_t pla, loc_t eloc, loc_t floc, loc_t floc2);
static bool canMoveTo1SEndUFGoal2S(Board& b, pla_t pla, loc_t dest, loc_t dest2, loc_t rloc);
static bool canUFandMoveTo1S(Board& b, pla_t pla, loc_t ploc, loc_t dest);
static bool canAdvanceUFStepGTree(Board& b, pla_t pla, loc_t curploc, loc_t futploc);
static bool canSelfUFByPullCap(Board& b, pla_t pla, loc_t ploc, loc_t dest, loc_t dest2);
static bool canMoveTo2SEndUF(Board& b, pla_t pla, loc_t dest, loc_t dest2, loc_t floc);
static bool isOpenToStepGTree(Board& b, loc_t k);
static bool canPull3S(Board& b, pla_t pla, loc_t eloc);
static bool canUFPushPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t floc);
static bool canUnblockPushPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t floc);
static bool canPullStepInPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t floc);
static bool canSwapOpp3S(Board& b, pla_t pla, loc_t eloc, loc_t floc);
static bool canSwapEmptyUF3S(Board& b, pla_t pla, loc_t loc, loc_t ploc, loc_t floc);
static bool canSafeAndSwapE2S(Board& b, pla_t pla, loc_t kt, loc_t loc, loc_t ploc, loc_t floc);
static bool canUF3(Board& b, pla_t pla, loc_t ploc, loc_t floc);
static bool canPPPEAndGoal(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t rloc, bool (*gfunc)(Board&,pla_t,loc_t));
static bool canPPCapAndGoal(Board& b, pla_t pla, loc_t eloc, loc_t kt, loc_t rloc, bool (*gfunc)(Board&,pla_t,loc_t));
static bool canRemoveDef3CGTree(Board& b, pla_t pla, loc_t eloc, loc_t kt);

static const int DY[2] = {-8,8};

static const int DYDIR[2] = {0,192};

//inline static int min(int x, int y) { return x < y ? x : y; }

int BoardTrees::goalDist(Board& b, pla_t pla, int steps)
{
	int dist = 5;

	Bitmap rMap = b.pieceMaps[pla][RAB];
	rMap &= Board::PGOALMASKS[steps][pla];

	move_t bestGoalTreeMove = ERRORMOVE;
	while(rMap.hasBits())
	{
		loc_t k = rMap.nextBit();
		int nextDist = goalDist(b,pla,min(steps,dist-1),k);
		if(nextDist < dist)
		{
			dist = nextDist;
			bestGoalTreeMove = b.goalTreeMove;
			if(dist <= 0)
			  return 0;
		}
	}
	b.goalTreeMove = bestGoalTreeMove;
	return dist;
}

int BoardTrees::goalDist(Board& b, pla_t pla, int steps, loc_t rloc)
{
#ifdef CHECK_GOALTREE_CONSISTENCY
	assert(b.testConsistency(cout));
#endif

	b.goalTreeMove = ERRORMOVE;
	if(Board::GOALYDIST[pla][rloc] == 0)
	  return 0;

	int dy = DY[pla];
	int extracost = (b.isFrozen(rloc) ? 1 : 0) + (b.owners[rloc+dy] != NPLA ? 1 : 0);

	if((pla == 0 && (int)rloc >= 8+8*(steps-extracost)) || (pla == 1 && (int)rloc < 56-8*(steps-extracost)))
	  return 5;

	if(steps >= 1 && canGoal1S(b,pla,rloc))
	  return 1;

	if(steps >= 2 && canGoal2S(b,pla,rloc))
	  return 2;

	if(steps >= 3 && canGoal3S(b,pla,rloc))
	  return 3;

	if(steps >= 4 && canGoal4S(b,pla,rloc))
	  return 4;

	b.goalTreeMove = ERRORMOVE;
	return 5;
}

static bool canGoal1S(Board& b, pla_t pla, loc_t rloc)
{
	SETGM((rloc MG));
	return Board::GOALYDIST[pla][rloc] == 1 && ISE(rloc+DY[pla]) && b.isThawedC(rloc);
}

static bool canGoal2S(Board& b, pla_t pla, loc_t rloc)
{
	int gdist = Board::GOALYDIST[pla][rloc];
	int dy = DY[pla];

	//One step away, rabbit frozen or blocked, or both
	if(gdist == 1)
	{
		//Not blocked, but frozen
		if(ISE(rloc G))
		  return canUF(b,pla,rloc,rloc G);

		//Blocked but not frozen
		else if(b.isThawedC(rloc))
		{
			//Friendly piece move out of way?
			if(ISP(rloc G) && b.wouldBeUF(pla,rloc,rloc,rloc G))
			{
				if(CW1(rloc) && ISE(rloc GW)) {SETGM((rloc G MW, rloc MG)); return true;}
				if(CE1(rloc) && ISE(rloc GE)) {SETGM((rloc G ME, rloc MG)); return true;}
			}
			//Walk around?
			if(CW1(rloc) && ISE(rloc W) && ISE(rloc GW) && b.wouldBeUF(pla,rloc,rloc W,rloc)) {SETGM((rloc MW, rloc W MG)); return true;}
			if(CE1(rloc) && ISE(rloc E) && ISE(rloc GE) && b.wouldBeUF(pla,rloc,rloc E,rloc)) {SETGM((rloc ME, rloc E MG)); return true;}
		}
	}

	//Two steps away
	else if(gdist == 2)
	{
		SETGM((rloc MG, rloc G MG));
		return ISE(rloc G) && ISE(rloc GG) && b.isThawedC(rloc) && b.wouldBeUF(pla,rloc,rloc G,rloc);
	}

	return false;
}

static bool canGoal3S(Board& b, pla_t pla, loc_t rloc)
{
	int dy = DY[pla];
	pla_t opp = OPP(pla);
	int gdist = Board::GOALYDIST[pla][rloc];
	bool suc = false;

	int extracost = (b.isFrozenC(rloc) ? 1 : 0) + (!ISE(rloc G) ? 1 : 0);
	if(gdist + extracost > 3)
	  return false;

	//One step away
	if(gdist == 1)
	{
		//Not blocked. Must be frozen.
		if(ISE(rloc G))
		{
			//Unfreeze in two steps?
			if(canUF2(b,pla,rloc,rloc G,ERRORSQUARE)) {return true;}

			//Unfreeze in one step, blocking the direct loc, and goal in 2 steps?
			if(canUFAtAndGoal(b,pla,rloc G,rloc,&canGoal2S)) {return true;}

			//Kill the freezer - but this is handled in canUF2

			//If we are unblocked, but cannot do any unfreezing, that's it.
			return false;
		}

		//Unfrozen, try stepping left and right and recursing
		if(b.isThawedC(rloc))
		{
			//Processing trap captures is necessary
			if(CW1(rloc) && ISE(rloc W)) {TSC(rloc,rloc W,suc = canGoal2S(b,pla,rloc W)); RIFGM(suc,(rloc MW))}
			if(CE1(rloc) && ISE(rloc E)) {TSC(rloc,rloc E,suc = canGoal2S(b,pla,rloc E)); RIFGM(suc,(rloc ME))}

			//Advance stepping the unfreezer
			if(ISP(rloc F) && b.wouldBeUF(pla,rloc,rloc,rloc F))
			{
				if(CW1(rloc) && ISE(rloc FW) && ISE(rloc W) && ISE(rloc GW) && b.isTrapSafe2(pla,rloc FW)) {SETGM((rloc F MW,rloc MW,rloc W MG)) return true;}
				if(CE1(rloc) && ISE(rloc FE) && ISE(rloc E) && ISE(rloc GE) && b.isTrapSafe2(pla,rloc FE)) {SETGM((rloc F ME,rloc ME,rloc E MG)) return true;}
			}
		}
		//Frozen and blocked.
		else
		{
			//Unfreeze and goal 2S?
			if(             ISE(rloc F) && canUFAtAndGoal(b,pla,rloc F,rloc,&canGoal2S)) {return true;}
			if(CW1(rloc) && ISE(rloc W) && canUFAtAndGoal(b,pla,rloc W,rloc,&canGoal2S)) {return true;}
			if(CE1(rloc) && ISE(rloc E) && canUFAtAndGoal(b,pla,rloc E,rloc,&canGoal2S)) {return true;}
		}

		//Singly blocked by a friendly piece? Have that piece get out of the way.
		if(ISP(rloc G))
		{
			if(CW1(rloc) && ISE(rloc GW)) {TS(rloc G,rloc GW, suc = canGoal2S(b,pla,rloc)) RIFGM(suc,(rloc G MW))}
			if(CE1(rloc) && ISE(rloc GE)) {TS(rloc G,rloc GE, suc = canGoal2S(b,pla,rloc)) RIFGM(suc,(rloc G ME))}
		}

		//Doubly blocked: single player, or both players, or pushable enemy and player. Careful on rabbit freezing!
		//Or a single step and a frozen rabbit.
		//We do not do rabbit checks since a rabbit violation would mean rabbit on the goal line anyways
		if(CW1(rloc))
		{
			if(
			(ISP(rloc G) && ISO(rloc GW) && GT(rloc G,rloc GW)) ||
			(ISO(rloc G) && ISP(rloc GW) && GT(rloc GW,rloc G) && b.isThawedC(rloc GW)) ||
			(ISP(rloc G) && ISP(rloc GW)))
			{
				if(CW2(rloc) && ISE(rloc GWW)) {TS(rloc GW, rloc GWW, TS(rloc G,rloc GW, suc = b.isThawedC(rloc))) RIFGM(suc,(rloc GW MW, rloc G MW, rloc MG))}
				if(ISE(rloc W))                {TS(rloc GW, rloc W,   TS(rloc G,rloc GW, suc = b.isThawedC(rloc))) RIFGM(suc,(rloc GW MF, rloc G MW, rloc MG))}
			}
		}
		if(CE1(rloc))
		{
			if(
			(ISP(rloc G) && ISO(rloc GE) && GT(rloc G,rloc GE)) ||
			(ISO(rloc G) && ISP(rloc GE) && GT(rloc GE,rloc G) && b.isThawedC(rloc GE)) ||
			(ISP(rloc G) && ISP(rloc GE)))
			{
				if(CE2(rloc) && ISE(rloc GEE)) {TS(rloc GE, rloc GEE, TS(rloc G,rloc GE, suc = b.isThawedC(rloc))) RIFGM(suc,(rloc GE ME, rloc G ME, rloc MG))}
				if(ISE(rloc E))                {TS(rloc GE, rloc E,   TS(rloc G,rloc GE, suc = b.isThawedC(rloc))) RIFGM(suc,(rloc GE MF, rloc G ME, rloc MG))}
			}
		}

		//Piece on the side must step back, and possibly die
		if(CW1(rloc) && ISP(rloc W) && !b.isRabbit(rloc W) && ISE(rloc FW) && ISE(rloc GW) && b.wouldBeUF(pla,rloc,rloc,rloc W) && (b.isTrapSafe2(pla,rloc FW) || b.wouldBeUF(pla,rloc,rloc W,rloc))) {SETGM((rloc W MF,rloc MW,rloc W MG)) return true;}
		if(CE1(rloc) && ISP(rloc E) && !b.isRabbit(rloc E) && ISE(rloc FE) && ISE(rloc GE) && b.wouldBeUF(pla,rloc,rloc,rloc E) && (b.isTrapSafe2(pla,rloc FE) || b.wouldBeUF(pla,rloc,rloc E,rloc))) {SETGM((rloc E MF,rloc ME,rloc E MG)) return true;}

		//Piece on the side must step further to the side
		if(CW2(rloc) && ISP(rloc W) && ISE(rloc WW) && ISE(rloc GW) && b.wouldBeUF(pla,rloc,rloc,rloc W)) {SETGM((rloc W MW,rloc MW, rloc W MG)) return true;}
		if(CE2(rloc) && ISP(rloc E) && ISE(rloc EE) && ISE(rloc GE) && b.wouldBeUF(pla,rloc,rloc,rloc E)) {SETGM((rloc E ME,rloc ME, rloc E MG)) return true;}

	}

	//Two steps away, rabbit frozen, frozen on the way, blocked, or blocked on the way
	else if(gdist == 2)
	{
		//If frozen, unfreeze and recurse
		if(b.isFrozenC(rloc))
		{
			//Must be empty to have any chance of goal
			if(ISE(rloc G) && ISE(rloc GG))
			{
				//Unfreeze and goal 2S without blocking the path.
				if(             ISE(rloc F) && canUFAtAndGoal(b,pla,rloc F,rloc,&canGoal2S)) {return true;}
				if(CW1(rloc) && ISE(rloc W) && canUFAtAndGoal(b,pla,rloc W,rloc,&canGoal2S)) {return true;}
				if(CE1(rloc) && ISE(rloc E) && canUFAtAndGoal(b,pla,rloc E,rloc,&canGoal2S)) {return true;}
			}
		}
		else
		{
			//Step each way
			if(CW1(rloc) && ISE(rloc W) && ISE(rloc GW) && ISE(rloc GGW) && b.isTrapSafe2(pla,rloc W) && b.wouldBeUF(pla,rloc,rloc W,rloc) && b.wouldBeUF(pla,rloc,rloc GW)) {SETGM((rloc MW,rloc W MG,rloc GW MG)) return true;}
			if(CE1(rloc) && ISE(rloc E) && ISE(rloc GE) && ISE(rloc GGE) && b.isTrapSafe2(pla,rloc E) && b.wouldBeUF(pla,rloc,rloc E,rloc) && b.wouldBeUF(pla,rloc,rloc GE)) {SETGM((rloc ME,rloc E MG,rloc GE MG)) return true;}

			//Stepping forward and recursing
			if(ISE(rloc G)) {TSC(rloc,rloc G, suc = canGoal2S(b,pla,rloc G)) RIFGM(suc,(rloc MG))}

			//Advance stepping the unfreezer
			if(ISE(rloc G) && ISE(rloc GG) && b.isTrapSafe2(pla,rloc))
			{
				if(CW1(rloc) && ISP(rloc W) && ISE(rloc GW) && b.wouldBeUF(pla,rloc,rloc,rloc W)) {SETGM((rloc W MG,rloc MG,rloc G MG)) return true;}
				if(CE1(rloc) && ISP(rloc E) && ISE(rloc GE) && b.wouldBeUF(pla,rloc,rloc,rloc E)) {SETGM((rloc E MG,rloc MG,rloc G MG)) return true;}
			}

			//Blocked by a friendly piece? Have that piece get out of the way.
			if(ISP(rloc G) && ISE(rloc GG) && b.isTrapSafe2(pla,rloc) && b.wouldBeUF(pla,rloc,rloc,rloc G))
			{
				if(CW1(rloc) && ISE(rloc GW)) {SETGM((rloc G MW,rloc MG,rloc G MG)) return true;}
				if(CE1(rloc) && ISE(rloc GE)) {SETGM((rloc G ME,rloc MG,rloc G MG)) return true;}
			}
		}
	}

	//Three steps away
	else if(gdist == 3)
	{
		SETGM((rloc MG,rloc G MG,rloc GG MG));
		return (ISE(rloc G) && ISE(rloc GG) && ISE(rloc GGG) && b.isTrapSafe2(pla,rloc G)
		&& b.isThawedC(rloc) && b.wouldBeUF(pla,rloc,rloc G,rloc) && b.wouldBeUF(pla,rloc,rloc GG));
	}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static bool canGoal4S(Board& b, pla_t pla, loc_t rloc)
{
	int gdist = Board::GOALYDIST[pla][rloc];
	int dy = DY[pla];
	pla_t opp = OPP(pla);
	bool suc = false;

	//One step away
	if(gdist == 1)
	{
		//Not blocked. Must be frozen.
		if(ISE(rloc G))
		{
			//Unfreeze in three steps?
			if(canUF3(b,pla,rloc,rloc G)) {return true;}

			//Unfreeze in one step, blocking the direct loc, and goal in 3 steps?
			if(canUFAtAndGoal(b,pla,rloc G,rloc,&canGoal3S)) {return true;}

			//Unfreeze in two step, blocking the direct loc, and goal in 2 steps?
			if(canUF2ForceAndGoal2S(b,pla,rloc,rloc G)) {return true;}

			//This counts as an unfreeze, but we will treat it as a special case. Kill the freezer! Skip cw1/ce1 checks because trap
			if(ISO(rloc F) && !b.isTrapSafe2(opp,rloc F))
			{
				if(ISO(rloc FF) && canPPCapAndGoal(b,pla,rloc FF,rloc F,rloc,&canGoal2S)) {return true;}
				else if(ISO(rloc FF) && b.wouldBeUF(pla,rloc,rloc,rloc F))
				{
					//Not possible for the rabbit to be re-frozen if the opp defending piece is on the opposite side
					suc = canRemoveDef3CGTree(b,pla,rloc FF,rloc F);
					RIF(suc)
				}

				if(ISO(rloc FW) && canPPCapAndGoal(b,pla,rloc FW,rloc F,rloc,&canGoal2S)) {return true;}
				else if(ISO(rloc FW) && b.wouldBeUF(pla,rloc,rloc,rloc F))
				{
					//Clever hack? Add opponent's rabbits to both sides to prevent anything moving there and refreezing the rabbit
					//Only do so if the trap defender is not a rabbit, because it's okay to push a rabbit next to our rabbit
					//it won't refreeze and we might need the space.
					bool left = false;
					bool right = false;
					bool isOppDefRab = b.pieces[rloc FW] == RAB;
					if(ISE(rloc W) && !isOppDefRab) {b.owners[rloc W] = opp; b.pieces[rloc W] = RAB; left = true;}
					if(ISE(rloc E) && !isOppDefRab) {b.owners[rloc E] = opp; b.pieces[rloc E] = RAB; right = true;}
					suc = canRemoveDef3CGTree(b,pla,rloc FW,rloc F);
					if(left)  {b.owners[rloc W] = NPLA; b.pieces[rloc W] = EMP;}
					if(right) {b.owners[rloc E] = NPLA; b.pieces[rloc E] = EMP;}

					RIF(suc)
				}

				if(ISO(rloc FE) && canPPCapAndGoal(b,pla,rloc FE,rloc F,rloc,&canGoal2S)) {return true;}
				else if(ISO(rloc FE) && b.wouldBeUF(pla,rloc,rloc,rloc F))
				{
					//Clever hack? Add opponent's rabbits to both sides to prevent anything moving there and refreezing the rabbit
					//Only do so if the trap defender is not a rabbit, because it's okay to push a rabbit next to our rabbit
					//it won't refreeze and we might need the space.
					bool left = false;
					bool right = false;
					bool isOppDefRab = b.pieces[rloc FE] == RAB;
					if(ISE(rloc W) && !isOppDefRab) {b.owners[rloc W] = opp; b.pieces[rloc W] = RAB; left = true;}
					if(ISE(rloc E) && !isOppDefRab) {b.owners[rloc E] = opp; b.pieces[rloc E] = RAB; right = true;}
					suc = canRemoveDef3CGTree(b,pla,rloc FE,rloc F);
					if(left)  {b.owners[rloc W] = NPLA; b.pieces[rloc W] = EMP;}
					if(right) {b.owners[rloc E] = NPLA; b.pieces[rloc E] = EMP;}

					RIF(suc)
				}
			}

			//If we are unblocked, but cannot do any unfreezing, that's it.
			return false;
		}

		//If unfrozen, try stepping left and right and recursing
		if(b.isThawed(rloc))
		{
			//Resolving trap captures
			if(CW1(rloc) && ISE(rloc W)) {TSC(rloc,rloc W, suc = canGoal3S(b,pla,rloc W)) RIFGM(suc,(rloc MW))}
			if(CE1(rloc) && ISE(rloc E)) {TSC(rloc,rloc E, suc = canGoal3S(b,pla,rloc E)) RIFGM(suc,(rloc ME))}

			//If we would sacrifice an unfreezer behind or leave it unfrozen, try
			//advance stepping the unfreezer or defending it for later unfreezement
			if(ISP(rloc F))
			{
				if(!b.isTrapSafe2(pla,rloc F) || !b.wouldBeUF(pla,rloc F,rloc F,rloc))
				{
					//If we would sacrifice it, try defending first, for later unfreezement
					if(!b.isTrapSafe2(pla,rloc F))
					{
						if(ISE(rloc W) || ISE(rloc E))
						{
							if(ISE(rloc FF) && canTrapDefAtAndRabStepWEGoal2S(b,pla,rloc FF,rloc F,rloc)) {return true;}
							if(ISE(rloc FW) && canTrapDefAtAndRabStepWEGoal2S(b,pla,rloc FW,rloc F,rloc)) {return true;}
							if(ISE(rloc FE) && canTrapDefAtAndRabStepWEGoal2S(b,pla,rloc FE,rloc F,rloc)) {return true;}
						}
					}

					//Cannot do the simple analog as in canGoal3S, because of advance-step, unfreeze, step, goal.
					if(CW1(rloc) && ISE(rloc FW) && ISE(rloc W))
					{
						if(b.isTrapSafe2(pla,rloc FW)) {TS(rloc F, rloc FW, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc F MW))}
						//Not trapsafe - can we defend the trap? If so, we must be goaling at GW and the rabbit must be unfrozen without F
						else if(ISE(rloc GW) && b.wouldBeUF(pla,rloc,rloc,rloc F))
						{
							//Only 3 possible locations - FFFW, FFWW, FWWW. If it was in FF, then the unfreezer would have himself
							//been unfrozen even after the rabbit moves, so wouldn't need to advance step. If it was in WW, then
							//the rabbit would be unfrozen after it steps W so wouldn't need the unfreezer behind it.
							if(ISE(rloc FFW))
							{
								if(ISP(rloc FFFW) && b.isThawed(rloc FFFW)) {SETGM((rloc FFFW MG,rloc F MW,rloc MW,rloc W MG)) return true;}
								if(ISP(rloc FFWW) && b.isThawed(rloc FFWW)) {SETGM((rloc FFWW ME,rloc F MW,rloc MW,rloc W MG)) return true;}
							}
							if(ISE(rloc FWW))
							{
								if(ISP(rloc FFWW) && b.isThawed(rloc FFWW)) {SETGM((rloc FFWW MG,rloc F MW,rloc MW,rloc W MG)) return true;}
								if(ISP(rloc FWWW) && b.isThawed(rloc FWWW)) {SETGM((rloc FWWW ME,rloc F MW,rloc MW,rloc W MG)) return true;}
							}
						}
					}
					if(CE1(rloc) && ISE(rloc FE) && ISE(rloc E))
					{
						if(b.isTrapSafe2(pla,rloc FE)) {TS(rloc F, rloc FE, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc F ME))}
						//Not trapsafe - can we defend the trap? If so, we must be goaling at GE and the rabbit must be unfrozen without F
						else if(ISE(rloc GE) && b.wouldBeUF(pla,rloc,rloc,rloc F))
						{
							//Only 3 possible locations - FFFE, FFEE, FEEE. If it was in FF, then the unfreezer would have himself
							//been unfrozen even after the rabbit moves, so wouldn't need to advance step. If it was in EE, then
							//the rabbit would be unfrozen after it steps E so wouldn't need the unfreezer behind it.
							if(ISE(rloc FFE))
							{
								if(ISP(rloc FFFE) && b.isThawed(rloc FFFE)) {SETGM((rloc FFFE MG,rloc F ME,rloc ME,rloc E MG)) return true;}
								if(ISP(rloc FFEE) && b.isThawed(rloc FFEE)) {SETGM((rloc FFEE MW,rloc F ME,rloc ME,rloc E MG)) return true;}
							}
							if(ISE(rloc FEE))
							{
								if(ISP(rloc FFEE) && b.isThawed(rloc FFEE)) {SETGM((rloc FFEE MG,rloc F ME,rloc ME,rloc E MG)) return true;}
								if(ISP(rloc FEEE) && b.isThawed(rloc FEEE)) {SETGM((rloc FEEE MW,rloc F ME,rloc ME,rloc E MG)) return true;}
							}
						}
					}

					//Advance pushpulling
					if(CW1(rloc) && ISO(rloc FW) && ISE(rloc W) && GT(rloc F,rloc FW) && canPPPEAndGoal(b,pla,rloc F,rloc FW,rloc,&canGoal2S)) {return true;}
					if(CE1(rloc) && ISO(rloc FE) && ISE(rloc E) && GT(rloc F,rloc FE) && canPPPEAndGoal(b,pla,rloc F,rloc FE,rloc,&canGoal2S)) {return true;}

					//Interesting type of advance unfreeze
					//RRCR..R.
					//D.rr....
					//..*dE*H.
					//.....hr.
					if(CW1(rloc) && CE1(rloc))
					{
						if(ISE(rloc GE) && ISE(rloc E) && ISP(rloc W) && ISE(rloc FW)) {SETGM((rloc F MW,rloc ME,rloc W ME,rloc E MG)) return true;}
						if(ISE(rloc GW) && ISE(rloc W) && ISP(rloc E) && ISE(rloc FE)) {SETGM((rloc F ME,rloc MW,rloc E MW,rloc W MG)) return true;}
					}
				} //End player at rloc F would die or be left frozen if rloc moves

			} //End player at rloc F

			//Another trap advance step
			//Skip cw1 ce1 checks due to !trapsafe.
			//...HcE..
			//.h*m.*..
			//r.cd.R.r
			//rrr..rrr
			if(ISE(rloc F) && !b.isTrapSafe3(pla,rloc F) &&
					((ISE(rloc W) && ISE(rloc FW) && ISE(rloc GW)) ||
					 (ISE(rloc E) && ISE(rloc FE) && ISE(rloc GE))))
			{
				if(canUFAtAndGoal(b,pla,rloc F,rloc,&canGoal3S)) {return true;}
			}

			//Various types of single blocks--------------

			//Singly blocked by a friendly piece? Have that piece get out of the way.
			if(ISP(rloc G))
			{
				if(CW1(rloc) && ISE(rloc GW)) {TS(rloc G,rloc GW, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc G MW))}
				if(CE1(rloc) && ISE(rloc GE)) {TS(rloc G,rloc GE, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc G ME))}
			}

			//Step and pull
			if(CW1(rloc) && ISO(rloc G) && ISE(rloc GW) && ISP(rloc W) && GT(rloc W,rloc G) && b.wouldBeUF(pla,rloc W,rloc GW,rloc W)) {SETGM((rloc W MG,rloc GW MF,rloc G MW,rloc MG)); return true;}
			if(CE1(rloc) && ISO(rloc G) && ISE(rloc GE) && ISP(rloc E) && GT(rloc E,rloc G) && b.wouldBeUF(pla,rloc E,rloc GE,rloc E)) {SETGM((rloc E MG,rloc GE MF,rloc G ME,rloc MG)); return true;}

			//Pieces on the side must move
			if(CW1(rloc) && ISP(rloc W) && !b.isRabbit(rloc W) && ISE(rloc FW) && (!ISO(rloc GW) || (CW2(rloc) && ISE(rloc GWW) && ISE(rloc WW))))
			{
				TSC(rloc W, rloc FW, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc W MF))
				//Protect the trap first so we don't lose a piece for future UF
				if(!b.isTrapSafe2(pla,rloc FW))
				{
					if(             ISE(rloc FFW) && canUFAtAndGoal(b,pla,rloc FFW,rloc,&canGoal3S)) {return true;}
					if(CW2(rloc) && ISE(rloc FWW) && canUFAtAndGoal(b,pla,rloc FWW,rloc,&canGoal3S)) {return true;}
					if(             ISE(rloc F  ) && canUFAtAndGoal(b,pla,rloc F  ,rloc,&canGoal3S)) {return true;}
				}
			}
			if(CE1(rloc) && ISP(rloc E) && !b.isRabbit(rloc E) && ISE(rloc FE) && (!ISO(rloc GE) || (CE2(rloc) && ISE(rloc GEE) && ISE(rloc EE))))
			{
				TSC(rloc E, rloc FE, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc E MF))
				//Protect the trap first so we don't lose a piece for future UF
				if(!b.isTrapSafe2(pla,rloc FE))
				{
					if(             ISE(rloc FFE) && canUFAtAndGoal(b,pla,rloc FFE,rloc,&canGoal3S)) {return true;}
					if(CE2(rloc) && ISE(rloc FEE) && canUFAtAndGoal(b,pla,rloc FEE,rloc,&canGoal3S)) {return true;}
					if(             ISE(rloc F  ) && canUFAtAndGoal(b,pla,rloc F  ,rloc,&canGoal3S)) {return true;}
				}
			}

			//Side pieces must get out of the way
			if(CW2(rloc) && ISP(rloc W))
			{
				if(ISE(rloc WW) && (ISE(rloc GW) || (ISP(rloc GW) && ISE(rloc GWW)))) {TSC(rloc W, rloc WW, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc W MW))}
				if(ISE(rloc GW) && ISE(rloc GWW) && b.wouldBeUF(pla,rloc,rloc,rloc W)) {TSC(rloc W, rloc GW, TS(rloc, rloc W, suc = canGoal2S(b,pla,rloc W))) RIFGM(suc,(rloc W MG))}
			}
			if(CE2(rloc) && ISP(rloc E))
			{
				if(ISE(rloc EE) && (ISE(rloc GE) || (ISP(rloc GE) && ISE(rloc GEE)))) {TSC(rloc E, rloc EE, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc E ME))}
				if(ISE(rloc GE) && ISE(rloc GEE) && b.wouldBeUF(pla,rloc,rloc,rloc E)) {TSC(rloc E, rloc GE, TS(rloc, rloc E, suc = canGoal2S(b,pla,rloc E))) RIFGM(suc,(rloc E MG))}
			}

			//Piece must step forward and block, to get out of the eventual way.
			if(CW1(rloc) && ISP(rloc GW) && b.isThawed(rloc GW) && ISE(rloc W))
			{
				if(CW2(rloc) && ISE(rloc WW)) {SETGM((rloc GW MF,rloc W MW,rloc MW,rloc W MG)); return true;}
				if(ISE(rloc FW) && (b.isTrapSafe1(pla,rloc FW) || b.wouldBeUF(pla,rloc,rloc W,rloc,rloc GW))) {SETGM((rloc GW MF,rloc W MF,rloc MW,rloc W MG)); return true;}
			}
			if(CE1(rloc) && ISP(rloc GE) && b.isThawed(rloc GE) && ISE(rloc E))
			{
				if(CE2(rloc) && ISE(rloc EE)) {SETGM((rloc GE MF,rloc E ME,rloc ME,rloc E MG)); return true;}
				if(ISE(rloc FE) && (b.isTrapSafe1(pla,rloc FE) || b.wouldBeUF(pla,rloc,rloc E,rloc,rloc GE))) {SETGM((rloc GE MF,rloc E MF,rloc ME,rloc E MG)); return true;}
			}

			//Special case - pull into trap to advance unfreeze when simple stepping is a sacrifice
			//....d...
			//.c*hcM..
			//.....Rr.
			//rr...r..
			//--------
			//Compare: capture is necessary - no solution here
			//....dc..
			//.c*hcM..
			//.....Rrr
			//rr...rrr
			//Compare: but a solution here without capture!
			//....d...
			//.c*hcMm.
			//.....RRe
			//rr...rrr
			//Compare: solution with no trap
			//..dd.r..
			//crhcMer.
			//....RRm.
			//r...rrr.
			if(ISP(rloc F) && b.isOpen(rloc F)
					&& (b.isGuarded2(pla,rloc) || (!b.isTrapSafe2(opp,rloc F) && b.wouldBeUF(pla,rloc,rloc,rloc F))))
			{
				if(CW1(rloc) && ISE(rloc W) && ISE(rloc GW) && ISO(rloc FW) && GT(rloc F,rloc FW) && b.wouldBeUF(pla,rloc,rloc W,rloc,rloc FW)) {SETGM((rloc F MF,rloc FW ME,rloc MW,rloc W MG)); return true;}
				if(CE1(rloc) && ISE(rloc E) && ISE(rloc GE) && ISO(rloc FE) && GT(rloc F,rloc FE) && b.wouldBeUF(pla,rloc,rloc E,rloc,rloc FE)) {SETGM((rloc F MF,rloc FE MW,rloc ME,rloc E MG)); return true;}
			}

			//Double step to advance unfreeze
			//4|.R.Mc...|
			//3|.r*.hMEm|
			//2|.C.rRDhR|
			//1|R..R...R|
			// +--------+
			if(ISE(rloc F) && ISO(rloc G))
			{
				if(CW1(rloc) && ISE(rloc GW) && ISE(rloc W) && ISE(rloc FW) && b.isTrapSafe1(pla,rloc FW))
				{
					if(             ISP(rloc FF) && b.isThawed(rloc FF)) {SETGM((rloc FF MG,rloc F MW,rloc MW,rloc W MG)); return true;}
					if(CE1(rloc) && ISP(rloc FE) && b.isThawed(rloc FE)) {SETGM((rloc FE MW,rloc F MW,rloc MW,rloc W MG)); return true;}
				}
				if(CE1(rloc) && ISE(rloc GE) && ISE(rloc E) && ISE(rloc FE) && b.isTrapSafe1(pla,rloc FE))
				{
					if(             ISP(rloc FF) && b.isThawed(rloc FF)) {SETGM((rloc FF MG,rloc F ME,rloc ME,rloc E MG)); return true;}
					if(CW1(rloc) && ISP(rloc FW) && b.isThawed(rloc FW)) {SETGM((rloc FW ME,rloc F ME,rloc ME,rloc E MG)); return true;}
				}
			}

		}//End unfrozen

		//Frozen and blocked
		else
		{
			//Unfreeze and goal 3S?
			if(             ISE(rloc F) && canUFCAtAndGoal(b,pla,rloc F,rloc,&canGoal3S)) {return true;}
			if(CW1(rloc) && ISE(rloc W) && canUFCAtAndGoal(b,pla,rloc W,rloc,&canGoal3S)) {return true;}
			if(CE1(rloc) && ISE(rloc E) && canUFCAtAndGoal(b,pla,rloc E,rloc,&canGoal3S)) {return true;}

			//Unfreeze in 2 and goal 2S?
			if(canUF2AndGoal2S(b,pla,rloc,ERRORSQUARE)) {return true;}

			//This counts as an unfreeze, but we will treat it as a special case. Kill the freezer!
			if(ISO(rloc F) && !b.isTrapSafe2(opp,rloc F) && b.wouldBeUF(pla,rloc,rloc,rloc F))
			{
				//Can skip cw1/ce1 because next to a trap
				if(ISO(rloc FW) && canPPCapAndGoal(b,pla,rloc FW,rloc F,rloc,&canGoal2S)) {return true;}
				if(ISO(rloc FE) && canPPCapAndGoal(b,pla,rloc FE,rloc F,rloc,&canGoal2S)) {return true;}
				if(ISO(rloc FF) && canPPCapAndGoal(b,pla,rloc FF,rloc F,rloc,&canGoal2S)) {return true;}
			}

			//Advance step unfreezing
			// -----
			// r.d.r
			// c.Rm.
			// d....
			// eCC..
			//Advance capture unfreezing (d on trap)
			// -----
			// rr.rr
			// cR.r.
			// e.d..
			// .Dc..
			//Advance capture unfreezing
			// +--------+
			//8|r...c...|
			//7|rrm.R.rr|
			//6|.HrDcdD.|
			//5|dCD.RC.E|

			if(CW1(rloc) && ISE(rloc W) && ISE(rloc GW))
			{
				if(ISE(rloc F) && ISP(rloc FF))
				{
					if(ISE(rloc FW) && ISP(rloc FFW) && b.isTrapSafe2(pla,rloc FW) &&
							b.wouldBeUF(pla,rloc FF,rloc FF,rloc FFW))
					{SETGM((rloc FFW MG,rloc FF MG,rloc MW,rloc W MG)); return true;}
					if(Board::ISTRAP[rloc FW] && ISO(rloc FW) && ISO(rloc FFW) && !ISO(rloc FWW) &&
							GT(rloc FF,rloc FFW) && b.isThawed(rloc FF) && b.wouldBeUF(pla,rloc,rloc W,rloc,rloc FW))
					{SETGM((rloc FF MG,rloc FFW ME,rloc MW,rloc W MG)); return true;}
				}

				if(CE1(rloc) && ISE(rloc E) && ISP(rloc FE) && Board::ISTRAP[rloc FW] && ISO(rloc FW) && ISO(rloc F) && GT(rloc FE,rloc F) &&
						b.trapGuardCounts[opp][Board::TRAPINDEX[rloc FW]] == 1 && b.isThawed(rloc FE) && b.wouldBeUF(pla,rloc,rloc W,rloc,rloc FW))
				{SETGM((rloc FE MG,rloc F ME,rloc MW,rloc W MG)); return true;}
			}
			if(CE1(rloc) && ISE(rloc E) && ISE(rloc GE))
			{
				if(ISE(rloc F) && ISP(rloc FF))
				{
					if(ISE(rloc FE) && ISP(rloc FFE) && b.isTrapSafe2(pla,rloc FE) &&
							b.wouldBeUF(pla,rloc FF,rloc FF,rloc FFE))
					{SETGM((rloc FFE MG,rloc FF MG,rloc ME,rloc E MG)); return true;}
					if(Board::ISTRAP[rloc FE] && ISO(rloc FE) && ISO(rloc FFE) && !ISO(rloc FEE) &&
							GT(rloc FF,rloc FFE) && b.isThawed(rloc FF) && b.wouldBeUF(pla,rloc,rloc E,rloc,rloc FE))
					{SETGM((rloc FF MG,rloc FFE MW,rloc ME,rloc E MG)); return true;}
				}

				if(CW1(rloc) && ISE(rloc W) && ISP(rloc FW) && Board::ISTRAP[rloc FE] && ISO(rloc FE) && ISO(rloc F) && GT(rloc FW,rloc F) &&
						b.trapGuardCounts[opp][Board::TRAPINDEX[rloc FE]] == 1 && b.isThawed(rloc FW) && b.wouldBeUF(pla,rloc,rloc E,rloc,rloc FE))
				{SETGM((rloc FW MG,rloc F MW,rloc ME,rloc E MG)); return true;}
			}
		} //End frozen and blocked

		//Bust through - double and triple blocks and various related patterns!-----------------------

		//Push and step back
		if(CW1(rloc) && CE1(rloc) && ISO(rloc G) && b.wouldBeUF(pla,rloc,rloc,rloc G))
		{
			if(ISP(rloc GW) && ISE(rloc GE) && GT(rloc GW,rloc G) && b.isThawed(rloc GW)) {SETGM((rloc G ME,rloc GW ME,rloc G MW,rloc MG)); return true;}
			if(ISP(rloc GE) && ISE(rloc GW) && GT(rloc GE,rloc G) && b.isThawed(rloc GE)) {SETGM((rloc G MW,rloc GE MW,rloc G ME,rloc MG)); return true;}
		}

		//Doubly blocked, both players, or pushable enemy and player. Careful on rabbit freezing! Or a single step and a frozen rabbit.
		if(CW1(rloc))
		{
			if(
			(ISP(rloc G) && ISO(rloc GW) && GT(rloc G,rloc GW)) ||
			(ISO(rloc G) && ISP(rloc GW) && GT(rloc GW,rloc G) && b.isThawed(rloc GW)) ||
			(ISP(rloc G) && ISP(rloc GW)))
			{
				if(CW2(rloc) && ISE(rloc GWW)) {TS(rloc GW, rloc GWW, TS(rloc G,rloc GW, suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GW MW,rloc G MW))}
				if(ISE(rloc W))                {TS(rloc GW, rloc W,   TS(rloc G,rloc GW, suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GW MF,rloc G MW))}
			}

			//Doubly blocked advance step
			// +--------+
			//8|rrrrmHdr|
			//7|h.cr.R.r|
			//6|.d*..*R.|
			//5|CE..e...|
			if(ISP(rloc G) && ISO(rloc GW) && GT(rloc G,rloc GW) && ISE(rloc W) && ISP(rloc FW) && ISE(rloc F) && b.isThawed(rloc FW)) {SETGM((rloc FW ME,rloc GW MF,rloc G MW,rloc MG)) return true;}
		}
		if(CE1(rloc))
		{
			if(
			(ISP(rloc G) && ISO(rloc GE) && GT(rloc G,rloc GE)) ||
			(ISO(rloc G) && ISP(rloc GE) && GT(rloc GE,rloc G) && b.isThawed(rloc GE)) ||
			(ISP(rloc G) && ISP(rloc GE)))
			{
				if(CE2(rloc) && ISE(rloc GEE)) {TS(rloc GE, rloc GEE, TS(rloc G,rloc GE, suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GE ME,rloc G ME))}
				if(ISE(rloc E))                {TS(rloc GE, rloc E,   TS(rloc G,rloc GE, suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GE MF,rloc G ME))}
			}

			//Doubly blocked advance step
			// +--------+
			//8|rrrrmHdr|
			//7|h.cr.R.r|
			//6|.d*..*R.|
			//5|CE..e...|
			if(ISP(rloc G) && ISO(rloc GE) && GT(rloc G,rloc GE) && ISE(rloc E) && ISP(rloc FE) && ISE(rloc F) && b.isThawed(rloc FE)) {SETGM((rloc FE MW,rloc GE MF,rloc G ME,rloc MG)) return true;}
		}

		//Triple block
		if(CW1(rloc))
		{
			if(CW2(rloc) && b.wouldBeUF(pla,rloc,rloc,rloc G) && (
			(ISP(rloc G) && ISP(rloc GW) && ISP(rloc GWW)) ||
			(ISP(rloc G) && ISP(rloc GW) && ISO(rloc GWW) && GT(rloc GW,rloc GWW)) ||
			(ISO(rloc G) && ISP(rloc GW) && ISP(rloc GWW) && GT(rloc GW,rloc G) && b.wouldBeUF(pla,rloc GW,rloc GW,rloc GWW)) ||
			(ISP(rloc G) && ISO(rloc GW) && ISP(rloc GWW) && b.isThawed(rloc GWW) && (GT(rloc GWW,rloc GW) || GT(rloc G,rloc GW)))
			))
			{
				if(CW3(rloc) && ISE(rloc GWWW))                               {SETGM((rloc GWW MW,rloc GW MW,rloc G MW,rloc MG)); return true;}
				if(ISE(rloc WW) && (!ISP(rloc GWW) || !b.isRabbit(rloc GWW))) {SETGM((rloc GWW MF,rloc GW MW,rloc G MW,rloc MG)); return true;}
			}

			if((!ISP(rloc GW) || !b.isRabbit(rloc GW)) && (
			(ISP(rloc G) && ISP(rloc GW) && ISP(rloc W)) ||
			(ISP(rloc G) && ISP(rloc GW) && ISO(rloc W) && GT(rloc GW,rloc W)) ||
			(ISO(rloc G) && ISP(rloc GW) && ISP(rloc W) && GT(rloc GW,rloc G) && b.wouldBeUF(pla,rloc GW,rloc GW,rloc W)) ||
			(ISP(rloc G) && ISO(rloc GW) && ISP(rloc W) && (GT(rloc W,rloc GW) || GT(rloc G,rloc GW)))
			))
			{
				if(CW2(rloc) && ISE(rloc WW))                             {TS(rloc W, rloc WW, TS(rloc GW, rloc W, TS(rloc G,rloc GW, suc = b.isThawedC(rloc);))) RIFGM(suc,(rloc W MW,rloc GW MF,rloc G MW,rloc MG));}
				if(ISE(rloc FW) && (!ISP(rloc W) || !b.isRabbit(rloc W))) {TS(rloc W, rloc FW, TS(rloc GW, rloc W, TS(rloc G,rloc GW, suc = b.isThawedC(rloc);))) RIFGM(suc,(rloc W MF,rloc GW MF,rloc G MW,rloc MG));}
			}
		}
		if(CE1(rloc))
		{
			if(CE2(rloc) && b.wouldBeUF(pla,rloc,rloc,rloc G) && (
			(ISP(rloc G) && ISP(rloc GE) && ISP(rloc GEE)) ||
			(ISP(rloc G) && ISP(rloc GE) && ISO(rloc GEE) && GT(rloc GE,rloc GEE)) ||
			(ISO(rloc G) && ISP(rloc GE) && ISP(rloc GEE) && GT(rloc GE,rloc G) && b.wouldBeUF(pla,rloc GE,rloc GE,rloc GEE)) ||
			(ISP(rloc G) && ISO(rloc GE) && ISP(rloc GEE) && b.isThawed(rloc GEE) && (GT(rloc GEE,rloc GE) || GT(rloc G,rloc GE)))
			))
			{
				if(CE3(rloc) && ISE(rloc GEEE))                               {SETGM((rloc GEE ME,rloc GE ME,rloc G ME,rloc MG)); return true;}
				if(ISE(rloc EE) && (!ISP(rloc GEE) || !b.isRabbit(rloc GEE))) {SETGM((rloc GEE MF,rloc GE ME,rloc G ME,rloc MG)); return true;}
			}

			if((!ISP(rloc GE) || !b.isRabbit(rloc GE)) && (
			(ISP(rloc G) && ISP(rloc GE) && ISP(rloc E)) ||
			(ISP(rloc G) && ISP(rloc GE) && ISO(rloc E) && GT(rloc GE,rloc E)) ||
			(ISO(rloc G) && ISP(rloc GE) && ISP(rloc E) && GT(rloc GE,rloc G) && b.wouldBeUF(pla,rloc GE,rloc GE,rloc E)) ||
			(ISP(rloc G) && ISO(rloc GE) && ISP(rloc E) && (GT(rloc E,rloc GE) || GT(rloc G,rloc GE)))
			))
			{
				if(CE2(rloc) && ISE(rloc EE))                             {TS(rloc E, rloc EE, TS(rloc GE, rloc E, TS(rloc G,rloc GE, suc = b.isThawedC(rloc);))) RIFGM(suc,(rloc E ME,rloc GE MF,rloc G ME,rloc MG));}
				if(ISE(rloc FE) && (!ISP(rloc E) || !b.isRabbit(rloc E))) {TS(rloc E, rloc FE, TS(rloc GE, rloc E, TS(rloc G,rloc GE, suc = b.isThawedC(rloc);))) RIFGM(suc,(rloc E MF,rloc GE MF,rloc G ME,rloc MG));}
			}
		}

		//Step and pull
		if(CW2(rloc) && ISO(rloc G) && ISE(rloc GW) && ISP(rloc GWW) && GT(rloc GWW,rloc G) && b.isThawed(rloc GWW) && b.wouldBeUF(pla,rloc GWW,rloc GW,rloc GWW) && (ISE(rloc W) || b.wouldBeUF(pla,rloc,rloc,rloc G))) {SETGM((rloc GWW ME)); return true;}
		if(CE2(rloc) && ISO(rloc G) && ISE(rloc GE) && ISP(rloc GEE) && GT(rloc GEE,rloc G) && b.isThawed(rloc GEE) && b.wouldBeUF(pla,rloc GEE,rloc GE,rloc GEE) && (ISE(rloc E) || b.wouldBeUF(pla,rloc,rloc,rloc G))) {SETGM((rloc GEE MW)); return true;}

		//Doubly side blocked
		if(CW1(rloc))
		{
			if(CW2(rloc) && ISE(rloc GW) && b.wouldBeUF(pla,rloc,rloc,rloc W) && (
			(ISP(rloc W) && ISO(rloc WW) && GT(rloc W,rloc WW)) ||
			(ISO(rloc W) && ISP(rloc WW) && GT(rloc WW,rloc W) && b.isThawed(rloc WW)) ||
			(ISP(rloc W) && ISP(rloc WW))))
			{
				if(CW3(rloc) && ISE(rloc WWW))                              {TS(rloc WW,rloc WWW, TSC(rloc W, rloc WW, TS(rloc, rloc W, suc = b.isThawedC(rloc W)))) RIFGM(suc,(rloc WW MW,rloc W MW,rloc MW,rloc W MG))}
				if(ISE(rloc GWW))                                           {TS(rloc WW,rloc GWW, TSC(rloc W, rloc WW, TS(rloc, rloc W, suc = b.isThawedC(rloc W)))) RIFGM(suc,(rloc WW MG,rloc W MW,rloc MW,rloc W MG))}
				if(ISE(rloc FWW) && !(ISP(rloc WW) && b.isRabbit(rloc WW))) {TS(rloc WW,rloc FWW, TSC(rloc W, rloc WW, TS(rloc, rloc W, suc = b.isThawedC(rloc W)))) RIFGM(suc,(rloc WW MF,rloc W MW,rloc MW,rloc W MG))}
			}
			if(ISE(rloc GW) && (
			(ISP(rloc W) && ISO(rloc FW) && GT(rloc W,rloc FW)) ||
			(ISO(rloc W) && ISP(rloc FW) && GT(rloc FW,rloc W) && b.isThawed(rloc FW)) ||
			(ISP(rloc W) && ISP(rloc FW) && !b.isRabbit(rloc W))))
			{
				bool hangingOppBehind = Board::ISTRAP[rloc F] && ISO(rloc F) && ISO(rloc FW) && b.trapGuardCounts[opp][Board::TRAPINDEX[rloc F]] == 1 && !b.wouldBeDom(pla,rloc,rloc,rloc F);
				if(CW2(rloc) && ISE(rloc FWW)                              && (b.wouldBeUF(pla,rloc,rloc,rloc W) || hangingOppBehind)) {TS(rloc FW,rloc FWW, TSC(rloc W, rloc FW, TS(rloc, rloc W, suc = b.isThawedC(rloc W)))) RIFGM(suc,(rloc FW MW,rloc W MF,rloc MW,rloc W MG))}
				if(ISE(rloc FFW) && !(ISP(rloc FW) && b.isRabbit(rloc FW)) && (b.wouldBeUF(pla,rloc,rloc,rloc W) || hangingOppBehind)) {TS(rloc FW,rloc FFW, TSC(rloc W, rloc FW, TS(rloc, rloc W, suc = b.isThawedC(rloc W)))) RIFGM(suc,(rloc FW MF,rloc W MF,rloc MW,rloc W MG))}
				if(ISE(rloc F))                                             {TPC(rloc W, rloc FW,rloc F, suc = canGoal2S(b,pla,rloc)) RIFGM(suc,(rloc FW ME,rloc W MF))}
			}
		}

		//Doubly blocked, other direction
		if(CE1(rloc))
		{
			if(CE2(rloc) && ISE(rloc GE) && b.wouldBeUF(pla,rloc,rloc,rloc E) && (
			(ISP(rloc E) && ISO(rloc EE) && GT(rloc E,rloc EE)) ||
			(ISO(rloc E) && ISP(rloc EE) && GT(rloc EE,rloc E) && b.isThawed(rloc EE)) ||
			(ISP(rloc E) && ISP(rloc EE))))
			{
				if(CE3(rloc) && ISE(rloc EEE))                             {TS(rloc EE,rloc EEE, TSC(rloc E, rloc EE, TS(rloc,rloc E, suc = b.isThawedC(rloc E)))) RIFGM(suc,(rloc EE ME,rloc E ME,rloc ME,rloc E MG))}
				if(ISE(rloc GEE))                                          {TS(rloc EE,rloc GEE, TSC(rloc E, rloc EE, TS(rloc,rloc E, suc = b.isThawedC(rloc E)))) RIFGM(suc,(rloc EE MG,rloc E ME,rloc ME,rloc E MG))}
				if(ISE(rloc FEE)&& !(ISP(rloc EE) && b.isRabbit(rloc EE))) {TS(rloc EE,rloc FEE, TSC(rloc E, rloc EE, TS(rloc,rloc E, suc = b.isThawedC(rloc E)))) RIFGM(suc,(rloc EE MF,rloc E ME,rloc ME,rloc E MG))}
			}
			if(ISE(rloc GE) && (
			(ISP(rloc E) && ISO(rloc FE) && GT(rloc E,rloc FE)) ||
			(ISO(rloc E) && ISP(rloc FE) && GT(rloc FE,rloc E) && b.isThawed(rloc FE)) ||
			(ISP(rloc E) && ISP(rloc FE) && !b.isRabbit(rloc E))))
			{
				bool hangingOppBehind = Board::ISTRAP[rloc F] && ISO(rloc F) && ISO(rloc FE) && b.trapGuardCounts[opp][Board::TRAPINDEX[rloc F]] == 1 && !b.wouldBeDom(pla,rloc,rloc,rloc F);
				if(CE2(rloc) && ISE(rloc FEE)                              && (b.wouldBeUF(pla,rloc,rloc,rloc E) || hangingOppBehind)) {TS(rloc FE,rloc FEE, TSC(rloc E, rloc FE, TS(rloc, rloc E, suc = b.isThawedC(rloc E)))) RIFGM(suc,(rloc FE ME,rloc E MF,rloc ME,rloc E MG))}
				if(ISE(rloc FFE) && !(ISP(rloc FE) && b.isRabbit(rloc FE)) && (b.wouldBeUF(pla,rloc,rloc,rloc E) || hangingOppBehind)) {TS(rloc FE,rloc FFE, TSC(rloc E, rloc FE, TS(rloc, rloc E, suc = b.isThawedC(rloc E)))) RIFGM(suc,(rloc FE MF,rloc E MF,rloc ME,rloc E MG))}
				if(ISE(rloc F))                                             {TPC(rloc E, rloc FE,rloc F, suc = canGoal2S(b,pla,rloc)) RIFGM(suc,(rloc FE MW,rloc E MF))}
			}
		}

		return false;
	}

	//Two steps away
	else if(gdist == 2)
	{
		//Frozen, so unfreeze
		if(b.isFrozen(rloc))
		{
			//Unfreeze and goal 3S?
			if(             ISE(rloc F) && canUFAtAndGoal(b,pla,rloc F,rloc,&canGoal3S)) {return true;}
			if(CW1(rloc) && ISE(rloc W) && canUFAtAndGoal(b,pla,rloc W,rloc,&canGoal3S)) {return true;}
			if(CE1(rloc) && ISE(rloc E) && canUFAtAndGoal(b,pla,rloc E,rloc,&canGoal3S)) {return true;}
			if(             ISE(rloc G) && canUFAtAndGoal(b,pla,rloc G,rloc,&canGoal3S)) {return true;}

			//Unfreeze in two without blocking the goal?
			//Note: it's possible that it starts out blocked!
			// +--------+
			//8|rr.C.rcr|
			//7|.eM.H.D.|
			//6|.H*Rd*e.|
			//5|....c...|
			//4|..C..dH.|
			if((ISE(rloc GG) || ISP(rloc GG)) && canUF2AndGoal2S(b,pla,rloc,rloc G)) {return true;}

			//This counts as an unfreeze, but we will treat it as a special case. Kill the freezer!
			if(CW1(rloc) && ISO(rloc W) && !b.isTrapSafe2(opp,rloc W) && b.wouldBeUF(pla,rloc,rloc,rloc W))
			{
				if(             ISO(rloc FW) && canPPCapAndGoal(b,pla,rloc FW,rloc W,rloc,&canGoal2S)) {return true;}
				if(CW2(rloc) && ISO(rloc WW) && canPPCapAndGoal(b,pla,rloc WW,rloc W,rloc,&canGoal2S)) {return true;}
				if(             ISO(rloc GW) && canPPCapAndGoal(b,pla,rloc GW,rloc W,rloc,&canGoal2S)) {return true;}
			}
			//This counts as an unfreeze, but we will treat it as a special case. Kill the freezer!
			if(CE1(rloc) && ISO(rloc E) && !b.isTrapSafe2(opp,rloc E) && b.wouldBeUF(pla,rloc,rloc,rloc E))
			{
				if(             ISO(rloc FE) && canPPCapAndGoal(b,pla,rloc FE,rloc E,rloc,&canGoal2S)) {return true;}
				if(CE2(rloc) && ISO(rloc EE) && canPPCapAndGoal(b,pla,rloc EE,rloc E,rloc,&canGoal2S)) {return true;}
				if(             ISO(rloc GE) && canPPCapAndGoal(b,pla,rloc GE,rloc E,rloc,&canGoal2S)) {return true;}
			}
			//Special case advance unfreeze
			if(ISE(rloc G) && ISE(rloc GG))
			{
				if(CW2(rloc) && ISE(rloc W) && ISP(rloc WW) && ISE(rloc GW) && ISP(rloc GWW) && b.wouldBeUF(pla,rloc WW,rloc WW,rloc GWW) && b.isTrapSafe2(pla,rloc WW))
				{SETGM((rloc GWW ME,rloc WW ME,rloc MG,rloc G MG)) return true;}
				if(CE2(rloc) && ISE(rloc E) && ISP(rloc EE) && ISE(rloc GE) && ISP(rloc GEE) && b.wouldBeUF(pla,rloc EE,rloc EE,rloc GEE) && b.isTrapSafe2(pla,rloc EE))
				{SETGM((rloc GEE MW,rloc EE MW,rloc MG,rloc G MG)) return true;}
			}
		}
		//Unfrozen
		else
		{
			//Step each way and recurse
			if(CW1(rloc) && ISE(rloc W) && b.isTrapSafe2(pla,rloc W))
			{TSC(rloc, rloc W, suc = canGoal3S(b,pla,rloc W)) RIFGM(suc,(rloc MW))}

			//Special case, unsafe trap where the rabbit wants to step.
			else if(CW1(rloc) && ISE(rloc W) && !b.isTrapSafe2(pla,rloc W) && canSafeifyAndGoal(b,pla,rloc W,rloc,rloc GW,rloc GGW,rloc F,rloc GWW))
			{return true;}

			//Step each way and recurse
			if(CE1(rloc) && ISE(rloc E) && b.isTrapSafe2(pla,rloc E))
			{TSC(rloc, rloc E, suc = canGoal3S(b,pla,rloc E)) RIFGM(suc,(rloc ME))}

			//Special case, unsafe trap where the rabbit wants to step.
			else if(CE1(rloc) && ISE(rloc E) && !b.isTrapSafe2(pla,rloc E) && canSafeifyAndGoal(b,pla,rloc E,rloc,rloc GE,rloc GGE,rloc F,rloc GEE))
			{return true;}

			//Empty in front
			if(ISE(rloc G))
			{
				//Step forward
				TSC(rloc, rloc G, suc = canGoal3S(b,pla,rloc G)) RIFGM(suc,(rloc MG))

				//Advance step forward
				if(!b.wouldBeUF(pla,rloc,rloc G,rloc))
				{
					//Advance step forward (could be advance, uf, step, step)
					if(b.isTrapSafe2(pla,rloc))
					{
						if(CW1(rloc) && ISP(rloc W) && ISE(rloc GW) && (!b.wouldBeUF(pla,rloc W,rloc W,rloc) || !b.isTrapSafe2(pla,rloc W))) {TS(rloc W, rloc GW, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc W MG))}
						if(CE1(rloc) && ISP(rloc E) && ISE(rloc GE) && (!b.wouldBeUF(pla,rloc E,rloc E,rloc) || !b.isTrapSafe2(pla,rloc E))) {TS(rloc E, rloc GE, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc E MG))}
					}
					//Rabbit on unsafe trap - make safe so that a future advance step will work
					//But, as illustrated, we don't even need trap problems, if the stepper is the future advance stepper.
					//RR.R....
					//D..H....
					//d.rdM...
					//.HcE....
					//But what they do seem to have in common, is GG empty
					if(ISE(rloc GG))
					{
						if(             ISE(rloc F) && canUFAtAndGoal(b,pla,rloc F,rloc,&canGoal3S)) {return true;}
						if(CW1(rloc) && ISE(rloc W) && canUFAtAndGoal(b,pla,rloc W,rloc,&canGoal3S)) {return true;}
						if(CE1(rloc) && ISE(rloc E) && canUFAtAndGoal(b,pla,rloc E,rloc,&canGoal3S)) {return true;}

						//Advance double step involving trap
						if(CW1(rloc) && ISE(rloc W) && ISE(rloc GW) && !b.isTrapSafe3(pla,rloc W))
						{
							if(CW2(rloc) && ISP(rloc WW) && b.isThawed(rloc WW)) {SETGM((rloc WW ME, rloc W MG,rloc MG,rloc G MG)); return true;}
							if(             ISP(rloc FW) && b.isThawed(rloc FW)) {SETGM((rloc FW MG, rloc W MG,rloc MG,rloc G MG)); return true;}
						}
						if(CE1(rloc) && ISE(rloc E) && ISE(rloc GE) && !b.isTrapSafe3(pla,rloc E))
						{
							if(CE2(rloc) && ISP(rloc EE) && b.isThawed(rloc EE)) {SETGM((rloc EE MW, rloc E MG,rloc MG,rloc G MG)); return true;}
							if(             ISP(rloc FE) && b.isThawed(rloc FE)) {SETGM((rloc FE MG, rloc E MG,rloc MG,rloc G MG)); return true;}
						}

						//Advance step from behind
						// .......R
						// ....R.H.
						// ..*..rr.
						// R..REc..
						//Can skip b.wouldBeUF(pla,rloc,rloc,rloc F) && b.isTrapSafe2(pla,rloc) because both subconditions have another adjacent player piece
						if(ISP(rloc F))
						{
							if(CW1(rloc) && ISE(rloc FW) && ISP(rloc W)) {SETGM((rloc F MW,rloc MG,rloc W ME, rloc G MG)) return true;}
							if(CE1(rloc) && ISE(rloc FE) && ISP(rloc E)) {SETGM((rloc F ME,rloc MG,rloc E MW, rloc G MG)) return true;}
						}

						//Advance pushpull to unfreeze
						if(b.isTrapSafe2(pla,rloc))
						{
							//Try both a push and a pull for each side
							if(CW1(rloc) && ISP(rloc W) && ISO(rloc GW) && GT(rloc W,rloc GW))
							{
								if(b.wouldBeUF(pla,rloc,rloc,rloc W)) {loc_t openLoc = ISE(rloc GGW) ? rloc GGW : (CW2(rloc) && ISE(rloc GWW)) ? rloc GWW : ERRORSQUARE; if(openLoc != ERRORSQUARE) {SETGM((Board::STEPINDEX[rloc GW][openLoc],rloc W MG)); return true;}}
								if((b.isGuarded2(pla,rloc) || (!b.isTrapSafe2(opp,rloc W) && b.wouldBeUF(pla,rloc,rloc,rloc W))) && !b.wouldBeDom(pla,rloc,rloc G,rloc GW)) {loc_t openLoc = b.findOpen(rloc W); if(openLoc != ERRORSQUARE) {SETGM((Board::STEPINDEX[rloc W][openLoc],rloc GW MF)); return true;}}
							}
							if(CE1(rloc) && ISP(rloc E) && ISO(rloc GE) && GT(rloc E,rloc GE))
							{
								if(b.wouldBeUF(pla,rloc,rloc,rloc E)) {loc_t openLoc = ISE(rloc GGE) ? rloc GGE : (CE2(rloc) && ISE(rloc GEE)) ? rloc GEE : ERRORSQUARE; if(openLoc != ERRORSQUARE) {SETGM((Board::STEPINDEX[rloc GE][openLoc],rloc E MG)); return true;}}
								if((b.isGuarded2(pla,rloc) || (!b.isTrapSafe2(opp,rloc E) && b.wouldBeUF(pla,rloc,rloc,rloc E))) && !b.wouldBeDom(pla,rloc,rloc G,rloc GE)) {loc_t openLoc = b.findOpen(rloc E); if(openLoc != ERRORSQUARE) {SETGM((Board::STEPINDEX[rloc E][openLoc],rloc GE MF)); return true;}}
							}
						}

					} //End empty rloc GG

				} //End would not be uf at rloc G

				//Special advance step where GG is blocked.
				// +--------+
				//8|rrHeC..r|
				//7|Re...hrD|
				//6|.R*RR*er|
				//5|...m.ECR|
				// +--------+
				//8|rd.Ce..r|
				//7|.e..hhr.|
				//6|..DR.*er|
				//5|........|
				if(ISP(rloc GG) && b.isTrapSafe2(pla,rloc))
				{
					if(CW1(rloc) && ISP(rloc W) && ISE(rloc GW) && b.wouldBeUF(pla,rloc,rloc,rloc W)) {if(ISE(rloc GGW)) {SETGM((rloc W MG,rloc MG,rloc GG MW,rloc G MG)) return true;} if(CE1(rloc) && ISE(rloc GGE)) {SETGM((rloc W MG,rloc MG,rloc GG ME,rloc G MG)) return true;}}
					if(CE1(rloc) && ISP(rloc E) && ISE(rloc GE) && b.wouldBeUF(pla,rloc,rloc,rloc E)) {if(ISE(rloc GGE)) {SETGM((rloc E MG,rloc MG,rloc GG ME,rloc G MG)) return true;} if(CW1(rloc) && ISE(rloc GGW)) {SETGM((rloc E MG,rloc MG,rloc GG MW,rloc G MG)) return true;}}
				}

				//Piece must step forward and block, to get out of the eventual way.
				if(ISP(rloc GG) && b.pieces[rloc GG] != RAB && b.isThawed(rloc GG))
				{
					if(CW1(rloc) && ISE(rloc GW)) {SETGM((rloc GG MF,rloc G MW, rloc MG, rloc G MG)); return true;}
					if(CE1(rloc) && ISE(rloc GE)) {SETGM((rloc GG MF,rloc G ME, rloc MG, rloc G MG)); return true;}
				}

				//Special case - pieces on the adjacent traps, make them safe so the rabbit will be unfrozen later.
				//The only reason to do this is so that piece can help later. This can only be the case if we are
				//stepping forward twice or stepping forward and around, in which case we know completely what
				//the goal move sequence must be.
				//+----------
				//|.......
				//|d.d....
				//|.RC....
				//|...C...
				//
				//+----------
				//|.r.....
				//|r..c...
				//|.RCc...
				//|...C...
				if(CW1(rloc) && ISP(rloc W) && !b.isTrapSafe2(pla,rloc W))
				{
					loc_t kt = rloc W;
					//Make it safe!
					if(ISE(kt F))
					{
						if(ISP(kt FF) && b.isThawed(kt FF)) {if(ISE(rloc GG)) {SETGM((kt FF MG,rloc MG,rloc W ME,rloc G MG)) return true;} else if(ISE(rloc GW) && ISE(rloc GGW) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt FF MG,rloc MG,rloc G MW,rloc GW MG)) return true;}}
						if(ISP(kt FW) && b.isThawed(kt FW)) {if(ISE(rloc GG)) {SETGM((kt FW ME,rloc MG,rloc W ME,rloc G MG)) return true;} else if(ISE(rloc GW) && ISE(rloc GGW) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt FW ME,rloc MG,rloc G MW,rloc GW MG)) return true;}}
						if(ISP(kt FE) && b.isThawed(kt FE)) {                                                                                   if(ISE(rloc GW) && ISE(rloc GGW) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt FE MW,rloc MG,rloc G MW,rloc GW MG)) return true;}} //Don't need the GG case, it's caught by the 'Advance step from behind' case.
					}
					//Make it safe!
					if(ISE(kt W))
					{
						if(ISP(kt FW) && b.isThawed(kt FW)                      ) {if(ISE(rloc GG)) {SETGM((kt FW MG,rloc MG,rloc W ME,rloc G MG)) return true;} else if(ISE(rloc GW) && ISE(rloc GGW) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt FW MG,rloc MG,rloc G MW,rloc GW MG)) return true;}}
						if(ISP(kt WW) && b.isThawed(kt WW)                      ) {if(ISE(rloc GG)) {SETGM((kt WW ME,rloc MG,rloc W ME,rloc G MG)) return true;} else if(ISE(rloc GW) && ISE(rloc GGW) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt WW ME,rloc MG,rloc G MW,rloc GW MG)) return true;}}
						if(ISP(kt GW) && b.isThawed(kt GW) && !b.isRabbit(kt GW)) {if(ISE(rloc GG)) {SETGM((kt GW MF,rloc MG,rloc W ME,rloc G MG)) return true;} } //Don't need the go around case because the piece we'd move to save the trap already guards the GW square
					}
				}
				//Aand the other way
				if(CE1(rloc) && ISP(rloc E) && !b.isTrapSafe2(pla,rloc E))
				{
					loc_t kt = rloc E;
					//Make it safe!
					if(ISE(kt F))
					{
						if(ISP(kt FF) && b.isThawed(kt FF)) {if(ISE(rloc GG)) {SETGM((kt FF MG,rloc MG,rloc E MW,rloc G MG)) return true;} else if(ISE(rloc GE) && ISE(rloc GGE) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt FF MG,rloc MG,rloc G ME,rloc GE MG)) return true;}}
						if(ISP(kt FE) && b.isThawed(kt FE)) {if(ISE(rloc GG)) {SETGM((kt FE MW,rloc MG,rloc E MW,rloc G MG)) return true;} else if(ISE(rloc GE) && ISE(rloc GGE) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt FE MW,rloc MG,rloc G ME,rloc GE MG)) return true;}}
						if(ISP(kt FW) && b.isThawed(kt FW)) {                                                                                   if(ISE(rloc GE) && ISE(rloc GGE) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt FW ME,rloc MG,rloc G ME,rloc GE MG)) return true;}} //Don't need the GG case, it's caught by the 'Advance step from behind' case.
					}
					//Make it safe!
					if(ISE(kt E))
					{
						if(ISP(kt FE) && b.isThawed(kt FE)                      ) {if(ISE(rloc GG)) {SETGM((kt FE MG,rloc MG,rloc E MW,rloc G MG)) return true;} else if(ISE(rloc GE) && ISE(rloc GGE) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt FE MG,rloc MG,rloc G ME,rloc GE MG)) return true;}}
						if(ISP(kt EE) && b.isThawed(kt EE)                      ) {if(ISE(rloc GG)) {SETGM((kt EE MW,rloc MG,rloc E MW,rloc G MG)) return true;} else if(ISE(rloc GE) && ISE(rloc GGE) && b.wouldBeUF(pla,rloc,rloc G,rloc)) {SETGM((kt EE MW,rloc MG,rloc G ME,rloc GE MG)) return true;}}
						if(ISP(kt GE) && b.isThawed(kt GE) && !b.isRabbit(kt GE)) {if(ISE(rloc GG)) {SETGM((kt GE MF,rloc MG,rloc E MW,rloc G MG)) return true;} } //Don't need the go around case because the piece we'd move to save the trap already guards the GW square
					}
				}

			} //End empty in front

			//Advance stepping the unfreezer behind
			if(ISP(rloc F) && b.wouldBeUF(pla,rloc,rloc,rloc F) && b.isTrapSafe2(pla,rloc))
			{
				if(CW1(rloc) && ISE(rloc FW) && ISE(rloc W) && ISE(rloc GW) && ISE(rloc GGW) && b.wouldBeUF(pla,rloc,rloc GW)) {SETGM((rloc F MW,rloc MW,rloc W MG,rloc GW MG)) return true;}
				if(CE1(rloc) && ISE(rloc FE) && ISE(rloc E) && ISE(rloc GE) && ISE(rloc GGE) && b.wouldBeUF(pla,rloc,rloc GE)) {SETGM((rloc F ME,rloc ME,rloc E MG,rloc GE MG)) return true;}
			}

			//Blocked by a friendly piece? Have that piece get out of the way.
			if(ISP(rloc G) && b.isTrapSafe2(pla,rloc))
			{
				if(CW1(rloc) && ISE(rloc GW)) {TS(rloc G,rloc GW, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc G MW))}
				if(CE1(rloc) && ISE(rloc GE)) {TS(rloc G,rloc GE, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc G ME))}
				if(             ISE(rloc GG)) {TS(rloc G,rloc GG, suc = canGoal3S(b,pla,rloc)) RIFGM(suc,(rloc G MG))}
			}

			//On trap and not safe. Make the trap safe then!
			else if(ISP(rloc G))
			{
				//Well, if we're spending a step to safe the trap, and we're 2 steps away, and there's a piece blocking, there's
				//only one way to goal - step the blocking piece to the side, walk forward twice. To do that, we need GG empty
				//and we need GW or GE empty, unless it has the piece we're moving to safe the trap
				if(ISE(rloc GG))
				{
					loc_t openLoc = ISE(rloc GW) ? rloc GW : ISE(rloc GE) ? rloc GE : ERRORSQUARE;
					if(openLoc != ERRORSQUARE && ISE(rloc F))
					{
						if(ISP(rloc FF) && b.isThawed(rloc FF)) {SETGM((rloc FF MG,Board::STEPINDEX[rloc G][openLoc],rloc MG,rloc G MG)) return true;}
						if(ISP(rloc FW) && b.isThawed(rloc FW)) {SETGM((rloc FW ME,Board::STEPINDEX[rloc G][openLoc],rloc MG,rloc G MG)) return true;}
						if(ISP(rloc FE) && b.isThawed(rloc FE)) {SETGM((rloc FE MW,Board::STEPINDEX[rloc G][openLoc],rloc MG,rloc G MG)) return true;}
					}
					if(ISE(rloc W))
					{
						if(openLoc != ERRORSQUARE && ISP(rloc FW) && b.isThawed(rloc FW)                        ) {SETGM((rloc FW MG,Board::STEPINDEX[rloc G][openLoc],rloc MG,rloc G MG)) return true;}
						if(openLoc != ERRORSQUARE && ISP(rloc WW) && b.isThawed(rloc WW)                        ) {SETGM((rloc WW ME,Board::STEPINDEX[rloc G][openLoc],rloc MG,rloc G MG)) return true;}
						if(                          ISP(rloc GW) && b.isThawed(rloc GW) && !b.isRabbit(rloc GW)) {SETGM((rloc GW MF,rloc G MW,rloc MG,rloc G MG)) return true;}
					}
					if(ISE(rloc E))
					{
						if(openLoc != ERRORSQUARE && ISP(rloc FE) && b.isThawed(rloc FE)                        ) {SETGM((rloc FE MG,Board::STEPINDEX[rloc G][openLoc],rloc MG,rloc G MG)) return true;}
						if(openLoc != ERRORSQUARE && ISP(rloc EE) && b.isThawed(rloc EE)                        ) {SETGM((rloc EE MW,Board::STEPINDEX[rloc G][openLoc],rloc MG,rloc G MG)) return true;}
						if(                          ISP(rloc GE) && b.isThawed(rloc GE) && !b.isRabbit(rloc GE)) {SETGM((rloc GE MF,rloc G ME,rloc MG,rloc G MG)) return true;}
					}
				}
			}

			//Pieces on the side must move
			if(CW1(rloc) && ISP(rloc W) && ISE(rloc GW) && ISE(rloc GGW) && b.isTrapSafe2(pla,rloc) && b.wouldBeUF(pla,rloc,rloc,rloc W) && b.wouldBeUF(pla,rloc,rloc GW,rloc W))
			{
				if(!b.isRabbit(rloc W) && ISE(rloc FW))                                                            {SETGM((rloc W MF,rloc MW,rloc W MG,rloc GW MG)) return true;}
				if(CW2(rloc) && ISE(rloc WW) && (b.isTrapSafe2(pla,rloc WW) || b.wouldBeUF(pla,rloc,rloc W,rloc))) {SETGM((rloc W MW,rloc MW,rloc W MG,rloc GW MG)) return true;}
			}
			if(CE1(rloc) && ISP(rloc E) && ISE(rloc GE) && ISE(rloc GGE) && b.isTrapSafe2(pla,rloc) && b.wouldBeUF(pla,rloc,rloc,rloc E) && b.wouldBeUF(pla,rloc,rloc GE,rloc E))
			{
				if(!b.isRabbit(rloc E) && ISE(rloc FE))                                                            {SETGM((rloc E MF,rloc ME,rloc E MG,rloc GE MG)) return true;}
				if(CE2(rloc) && ISE(rloc EE) && (b.isTrapSafe2(pla,rloc EE) || b.wouldBeUF(pla,rloc,rloc E,rloc))) {SETGM((rloc E ME,rloc ME,rloc E MG,rloc GE MG)) return true;}
			}

		} //End unfrozen

		//Get block out of way : both players, or pushable enemy and player. Careful on rabbit freezing! Or a single step and a frozen rabbit.
		//Must resolve captures - see following, with d on trap.
		//dRr
		//rEr
		//r.r
		if(CW1(rloc) && ISE(rloc GG))
		{
			if(
			(ISP(rloc G) && ISO(rloc GW) && GT(rloc G,rloc GW)) ||
			(ISO(rloc G) && ISP(rloc GW) && GT(rloc GW,rloc G) && b.isThawed(rloc GW)) ||
			(ISP(rloc G) && ISP(rloc GW)))
			{
				if(ISE(rloc GGW))                                         {TSC(rloc GW, rloc GGW, TS(rloc G,rloc GW, suc = b.isTrapSafe1(pla,rloc) && canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GW MG,rloc G MW))}
				if(CW2(rloc) && ISE(rloc GWW))                            {TSC(rloc GW, rloc GWW, TS(rloc G,rloc GW, suc = b.isTrapSafe1(pla,rloc) && canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GW MW,rloc G MW))}
				if(ISE(rloc W) && !(b.isRabbit(rloc GW) && ISP(rloc GW))) {TSC(rloc GW, rloc W,   TS(rloc G,rloc GW, suc = b.isTrapSafe1(pla,rloc) && canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GW MF,rloc G MW))}
			}
		}
		if(CE1(rloc) && ISE(rloc GG))
		{
			if(
			(ISP(rloc G) && ISO(rloc GE) && GT(rloc G,rloc GE)) ||
			(ISO(rloc G) && ISP(rloc GE) && GT(rloc GE,rloc G) && b.isThawed(rloc GE)) ||
			(ISP(rloc G) && ISP(rloc GE)))
			{
				if(ISE(rloc GGE))                                         {TSC(rloc GE, rloc GGE, TS(rloc G,rloc GE, suc = b.isTrapSafe1(pla,rloc) && canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GE MG,rloc G ME))}
				if(CE2(rloc) && ISE(rloc GEE))                            {TSC(rloc GE, rloc GEE, TS(rloc G,rloc GE, suc = b.isTrapSafe1(pla,rloc) && canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GE ME,rloc G ME))}
				if(ISE(rloc E) && !(b.isRabbit(rloc GE) && ISP(rloc GE))) {TSC(rloc GE, rloc E,   TS(rloc G,rloc GE, suc = b.isTrapSafe1(pla,rloc) && canGoal2S(b,pla,rloc))) RIFGM(suc,(rloc GE MF,rloc G ME))}
			}
		}

		return false;
	}

	//Three steps away, rabbit frozen, frozen on the way, blocked, or blocked on the way
	else if(gdist == 3)
	{
		//If frozen, unfreeze and recurse
		if(b.isFrozen(rloc))
		{
			//Unfreeze and goal 3S without blocking the goal?
			if(             ISE(rloc F) && canUFAtAndGoal(b,pla,rloc F,rloc,&canGoal3S)) {return true;}
			if(CW1(rloc) && ISE(rloc W) && canUFAtAndGoal(b,pla,rloc W,rloc,&canGoal3S)) {return true;}
			if(CE1(rloc) && ISE(rloc E) && canUFAtAndGoal(b,pla,rloc E,rloc,&canGoal3S)) {return true;}
		}
		//Unfrozen
		else
		{
			//Step each way and all the way forward
			loc_t ignSquare = (ISP(rloc G) && !b.isTrapSafe2(pla,rloc G)) ? rloc G : ERRORSQUARE;
			if(CW1(rloc) && ISE(rloc W) && ISE(rloc GW) && ISE(rloc GGW) && ISE(rloc GGGW) && b.isTrapSafe1(pla,rloc GW) &&
					b.wouldBeUF(pla,rloc,rloc W,rloc) && b.wouldBeUF(pla,rloc,rloc GW,ignSquare) && b.wouldBeUF(pla,rloc,rloc GGW))
			{SETGM((rloc MW,rloc W MG, rloc GW MG, rloc GGW MG)) return true;}
			if(CE1(rloc) && ISE(rloc E) && ISE(rloc GE) && ISE(rloc GGE) && ISE(rloc GGGE) && b.isTrapSafe1(pla,rloc GE) &&
					b.wouldBeUF(pla,rloc,rloc E,rloc) && b.wouldBeUF(pla,rloc,rloc GE,ignSquare) && b.wouldBeUF(pla,rloc,rloc GGE))
			{SETGM((rloc ME,rloc E MG, rloc GE MG, rloc GGE MG)) return true;}

			//Step forward
			if(ISE(rloc G) && b.isTrapSafe2(pla,rloc G)) {TS(rloc, rloc G, suc = canGoal3S(b,pla,rloc G)) RIFGM(suc,(rloc MG))}

			//The trap in front is not safe. Make it safe!
			else if(ISE(rloc G) && ISE(rloc GG) && ISE(rloc GGG) && !b.isTrapSafe2(pla,rloc G))
			{
				if(CW1(rloc) && ISE(rloc GW))
				{
					if(b.isThawed(rloc) &&              ISP(rloc GGW) && !b.isRabbit(rloc GGW) && b.isThawed(rloc GGW) && b.wouldBeUF(pla,rloc,rloc GG,rloc GGW))                             {SETGM((rloc GGW MF,rloc MG,rloc G MG,rloc GG MG)) return true;}
					if(b.isThawed(rloc) && CW2(rloc) && ISP(rloc GWW)                          && b.isThawed(rloc GWW) && b.wouldBeUF(pla,rloc,rloc GG))                                      {SETGM((rloc GWW ME,rloc MG,rloc G MG,rloc GG MG)) return true;}
					if(                                 ISP(rloc W)                            && b.isThawed(rloc W)   && b.wouldBeUF(pla,rloc,rloc GG) && b.wouldBeUF(pla,rloc,rloc,rloc W)) {SETGM((rloc W MG,rloc MG,rloc G MG,rloc GG MG)) return true;}
				}
				if(CE1(rloc) && ISE(rloc GE))
				{
					if(b.isThawed(rloc) &&              ISP(rloc GGE) && !b.isRabbit(rloc GGE) && b.isThawed(rloc GGE) && b.wouldBeUF(pla,rloc,rloc GG,rloc GGE))                             {SETGM((rloc GGE MF,rloc MG,rloc G MG,rloc GG MG)) return true;}
					if(b.isThawed(rloc) && CE2(rloc) && ISP(rloc GEE)                          && b.isThawed(rloc GEE) && b.wouldBeUF(pla,rloc,rloc GG))                                      {SETGM((rloc GEE MW,rloc MG,rloc G MG,rloc GG MG)) return true;}
					if(                                 ISP(rloc E)                            && b.isThawed(rloc E)   && b.wouldBeUF(pla,rloc,rloc GG) && b.wouldBeUF(pla,rloc,rloc,rloc E)) {SETGM((rloc E MG,rloc MG,rloc G MG,rloc GG MG)) return true;}
				}
			}

			//Advance stepping the unfreezer
			if(ISE(rloc G) && ISE(rloc GG) && ISE(rloc GGG) && b.wouldBeUF(pla,rloc,rloc GG))
			{
				if(CW1(rloc) && ISP(rloc W) && ISE(rloc GW) && b.isTrapSafe2(pla,rloc GW) && b.wouldBeUF(pla,rloc,rloc,rloc W)) {SETGM((rloc W MG,rloc MG,rloc G MG,rloc GG MG)); return true;}
				if(CE1(rloc) && ISP(rloc E) && ISE(rloc GE) && b.isTrapSafe2(pla,rloc GE) && b.wouldBeUF(pla,rloc,rloc,rloc E)) {SETGM((rloc E MG,rloc MG,rloc G MG,rloc GG MG)); return true;}
			}

			//Blocked by a friendly piece? Have that piece get out of the way.
			if(ISP(rloc G) && ISE(rloc GG) && ISE(rloc GGG) && b.wouldBeUF(pla,rloc,rloc,rloc G) && b.wouldBeUF(pla,rloc,rloc GG,rloc G))
			{
				if(CW1(rloc) && ISE(rloc GW) && (b.isTrapSafe2(pla,rloc GW) || b.wouldBeUF(pla,rloc,rloc G,rloc))) {SETGM((rloc G MW,rloc MG,rloc G MG,rloc GG MG)); return true;}
				if(CE1(rloc) && ISE(rloc GE) && (b.isTrapSafe2(pla,rloc GE) || b.wouldBeUF(pla,rloc,rloc G,rloc))) {SETGM((rloc G ME,rloc MG,rloc G MG,rloc GG MG)); return true;}
			}
		}
		return false;
	}

	//Four steps away
	else if(gdist == 4)
	{
	  SETGM((rloc MG,rloc G MG,rloc GG MG,rloc GGG MG));
		return
		ISE(rloc G) && ISE(rloc GG) && ISE(rloc GGG) && ISE(rloc GGGG)
		&& b.isTrapSafe1(pla,rloc GG)
		&& b.isThawed(rloc)
		&& b.wouldBeUF(pla,rloc,rloc G,rloc)
		&& b.wouldBeUF(pla,rloc,rloc GG)
		&& b.wouldBeUF(pla,rloc,rloc GGG);
	}

	return false;
}

static bool canUF(Board& b, pla_t pla, loc_t ploc, loc_t floc)
{
	if(CS1(ploc) && ISE(ploc S) && ploc S != floc)
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawedC(ploc SS) && b.isRabOkayN(pla,ploc SS)) {SETGM((ploc SS MN)); return true;}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW)                             ) {SETGM((ploc SW ME)); return true;}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE)                             ) {SETGM((ploc SE MW)); return true;}
	}
	if(CW1(ploc) && ISE(ploc W) && ploc W != floc)
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.isRabOkayN(pla,ploc SW)) {SETGM((ploc SW MN)); return true;}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawedC(ploc WW)                             ) {SETGM((ploc WW ME)); return true;}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.isRabOkayS(pla,ploc NW)) {SETGM((ploc NW MS)); return true;}
	}
	if(CE1(ploc) && ISE(ploc E) && ploc E != floc)
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.isRabOkayN(pla,ploc SE)) {SETGM((ploc SE MN)); return true;}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawedC(ploc EE)                             ) {SETGM((ploc EE MW)); return true;}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.isRabOkayS(pla,ploc NE)) {SETGM((ploc NE MS)); return true;}
	}
	if(CN1(ploc) && ISE(ploc N) && ploc N != floc)
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW)                             ) {SETGM((ploc NW ME)); return true;}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE)                             ) {SETGM((ploc NE MW)); return true;}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawedC(ploc NN) && b.isRabOkayS(pla,ploc NN)) {SETGM((ploc NN MS)); return true;}
	}
	return false;
}

//           RALC
//      TRAP RLOC
// FALC FLOC
//      F2LC
//Does NOT use the C version of thawed or frozen!
static bool canSafeifyAndGoal(Board& b, pla_t pla, loc_t kt, loc_t rloc, loc_t floc, loc_t floc2, loc_t radjloc, loc_t fadjloc)
{
	if(!(ISE(floc) && ISE(floc2)))
	{return false;}

	if(!b.wouldBeUF(pla,rloc,floc))
	{return false;}

	bool rabuf = b.wouldBeUF(pla,rloc,rloc,radjloc);
	bool fabuf = b.wouldBeUF(pla,rloc,floc,fadjloc);

	step_t rstep = Board::STEPINDEX[rloc][kt];
	if(kt S != floc && ISE(kt S))
	{
		if(ISP(kt SS) && (rabuf || kt SS != radjloc) && (fabuf || kt SS != fadjloc) && b.isThawed(kt SS) && b.isRabOkayN(pla,kt SS)) {SETGM((kt SS MN,rstep)); return true;}
		if(ISP(kt SW) && (rabuf || kt SW != radjloc) && (fabuf || kt SW != fadjloc) && b.isThawed(kt SW))                            {SETGM((kt SW ME,rstep)); return true;}
		if(ISP(kt SE) && (rabuf || kt SE != radjloc) && (fabuf || kt SE != fadjloc) && b.isThawed(kt SE))                            {SETGM((kt SE MW,rstep)); return true;}
	}
	if(kt W != floc && ISE(kt W))
	{
		if(ISP(kt SW) && (rabuf || kt SW != radjloc) && (fabuf || kt SW != fadjloc) && b.isThawed(kt SW) && b.isRabOkayN(pla,kt SW)) {SETGM((kt SW MN,rstep)); return true;}
		if(ISP(kt WW) && (rabuf || kt WW != radjloc) && (fabuf || kt WW != fadjloc) && b.isThawed(kt WW))                            {SETGM((kt WW ME,rstep)); return true;}
		if(ISP(kt NW) && (rabuf || kt NW != radjloc) && (fabuf || kt NW != fadjloc) && b.isThawed(kt NW) && b.isRabOkayS(pla,kt NW)) {SETGM((kt NW MS,rstep)); return true;}
	}
	if(kt E != floc && ISE(kt E))
	{
		if(ISP(kt SE) && (rabuf || kt SE != radjloc) && (fabuf || kt SE != fadjloc) && b.isThawed(kt SE) && b.isRabOkayN(pla,kt SE)) {SETGM((kt SE MN,rstep)); return true;}
		if(ISP(kt EE) && (rabuf || kt EE != radjloc) && (fabuf || kt EE != fadjloc) && b.isThawed(kt EE))                            {SETGM((kt EE MW,rstep)); return true;}
		if(ISP(kt NE) && (rabuf || kt NE != radjloc) && (fabuf || kt NE != fadjloc) && b.isThawed(kt NE) && b.isRabOkayS(pla,kt NE)) {SETGM((kt NE MS,rstep)); return true;}
	}
	if(kt N != floc && ISE(kt N))
	{
		if(ISP(kt NW) && (rabuf || kt NW != radjloc) && (fabuf || kt NW != fadjloc) && b.isThawed(kt NW))                            {SETGM((kt NW ME,rstep)); return true;}
		if(ISP(kt NE) && (rabuf || kt NE != radjloc) && (fabuf || kt NE != fadjloc) && b.isThawed(kt NE))                            {SETGM((kt NE MW,rstep)); return true;}
		if(ISP(kt NN) && (rabuf || kt NN != radjloc) && (fabuf || kt NN != fadjloc) && b.isThawed(kt NN) && b.isRabOkayS(pla,kt NN)) {SETGM((kt NN MS,rstep)); return true;}
	}

	return false;
}

//Assumes dest is empty!!
//Moves a piece into dest, presumably unfreezing ploc, then tests if ploc can goal.
static bool canUFAtAndGoal(Board& b, pla_t pla, loc_t dest, loc_t ploc, bool (*gfunc)(Board&,pla_t,loc_t))
{
	bool suc = false;

	if(CS1(dest) && ISP(dest S) && dest S != ploc && b.isThawedC(dest S) && b.isRabOkayN(pla,dest S)) {TS(dest S,dest, suc = (*gfunc)(b,pla,ploc)) RIFGM(suc,(dest S MN))}
	if(CW1(dest) && ISP(dest W) && dest W != ploc && b.isThawedC(dest W)                            ) {TS(dest W,dest, suc = (*gfunc)(b,pla,ploc)) RIFGM(suc,(dest W ME))}
	if(CE1(dest) && ISP(dest E) && dest E != ploc && b.isThawedC(dest E)                            ) {TS(dest E,dest, suc = (*gfunc)(b,pla,ploc)) RIFGM(suc,(dest E MW))}
	if(CN1(dest) && ISP(dest N) && dest N != ploc && b.isThawedC(dest N) && b.isRabOkayS(pla,dest N)) {TS(dest N,dest, suc = (*gfunc)(b,pla,ploc)) RIFGM(suc,(dest N MS))}

	return false;
}

//Assumes dest is empty!!
//Moves a piece into dest, presumably unfreezing ploc, then tests if ploc can goal.
//Resolves captures
static bool canUFCAtAndGoal(Board& b, pla_t pla, loc_t dest, loc_t ploc, bool (*gfunc)(Board&,pla_t,loc_t))
{
	bool suc = false;

	if(CS1(dest) && ISP(dest S) && dest S != ploc && b.isThawedC(dest S) && b.isRabOkayN(pla,dest S)) {TSC(dest S,dest, suc = (*gfunc)(b,pla,ploc)) RIFGM(suc,(dest S MN))}
	if(CW1(dest) && ISP(dest W) && dest W != ploc && b.isThawedC(dest W)                            ) {TSC(dest W,dest, suc = (*gfunc)(b,pla,ploc)) RIFGM(suc,(dest W ME))}
	if(CE1(dest) && ISP(dest E) && dest E != ploc && b.isThawedC(dest E)                            ) {TSC(dest E,dest, suc = (*gfunc)(b,pla,ploc)) RIFGM(suc,(dest E MW))}
	if(CN1(dest) && ISP(dest N) && dest N != ploc && b.isThawedC(dest N) && b.isRabOkayS(pla,dest N)) {TSC(dest N,dest, suc = (*gfunc)(b,pla,ploc)) RIFGM(suc,(dest N MS))}

	return false;
}

//Assumes dest is empty!!
//Moves a piece into adjtrap to defend it, then tests if rloc can goal by stepping left or right.
//Does not check caps, does not check if rloc is in fact the moved piece into adjtrap - it should not be!
//Assumes a pla piece is on the trap, so that rloc is unfrozen!
static bool canTrapDefAtAndRabStepWEGoal2S(Board& b, pla_t pla, loc_t adjtrap, loc_t kt, loc_t rloc)
{
	bool suc = false;

	if(adjtrap S != kt && ISP(adjtrap S) && b.isThawed(adjtrap S) && b.isRabOkayN(pla,adjtrap S)) {TS(adjtrap S,adjtrap, if(ISE(rloc W)){TS(rloc,rloc W, suc = canGoal2S(b,pla,rloc W))} if(!suc && ISE(rloc E)) {TS(rloc,rloc E, suc = canGoal2S(b,pla,rloc E))}) RIFGM(suc,(adjtrap S MN))}
	if(adjtrap W != kt && ISP(adjtrap W) && b.isThawed(adjtrap W)                               ) {TS(adjtrap W,adjtrap, if(ISE(rloc W)){TS(rloc,rloc W, suc = canGoal2S(b,pla,rloc W))} if(!suc && ISE(rloc E)) {TS(rloc,rloc E, suc = canGoal2S(b,pla,rloc E))}) RIFGM(suc,(adjtrap W ME))}
	if(adjtrap E != kt && ISP(adjtrap E) && b.isThawed(adjtrap E)                               ) {TS(adjtrap E,adjtrap, if(ISE(rloc W)){TS(rloc,rloc W, suc = canGoal2S(b,pla,rloc W))} if(!suc && ISE(rloc E)) {TS(rloc,rloc E, suc = canGoal2S(b,pla,rloc E))}) RIFGM(suc,(adjtrap E MW))}
	if(adjtrap N != kt && ISP(adjtrap N) && b.isThawed(adjtrap N) && b.isRabOkayS(pla,adjtrap N)) {TS(adjtrap N,adjtrap, if(ISE(rloc W)){TS(rloc,rloc W, suc = canGoal2S(b,pla,rloc W))} if(!suc && ISE(rloc E)) {TS(rloc,rloc E, suc = canGoal2S(b,pla,rloc E))}) RIFGM(suc,(adjtrap N MS))}

	return false;
}

//Does trap cap unfreezes, unlike UF3
//Don't move any piece at all into floc
//Don't move any opponent piece into floc2 through a capture push
//Special, unlike the goal variants and such. Uniquely processes trap capture unfreezes
static bool canUF2(Board& b, pla_t pla, loc_t ploc, loc_t floc, loc_t floc2)
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
		loc_t eloc;
		     if(CS1(ploc) && GT(ploc S,ploc)) {eloc = ploc S;}
		else if(CW1(ploc) && GT(ploc W,ploc)) {eloc = ploc W;}
		else if(CE1(ploc) && GT(ploc E,ploc)) {eloc = ploc E;}
		else                                  {eloc = ploc N;}

		//Clever hack? add an opponent's rabbit in floc to prevent pulls from working there.
		bool suc = false;
		bool added = false;
		bool added2 = false;
		if(floc != ERRORSQUARE && ISE(floc)) {b.owners[floc] = opp; b.pieces[floc] = RAB; added = true;}
		if(floc2 != ERRORSQUARE && ISE(floc2)) {b.owners[floc2] = opp; b.pieces[floc2] = RAB; added2 = true;}

		//Pull the piece away
		suc = canPullg(b,pla,eloc);

		//Kill the piece!
		if(!suc && !b.isTrapSafe2(opp,eloc))
		{
			if     (ISO(eloc S)) {suc = canPPCapAndUFg(b,pla,eloc S,ploc);}
			else if(ISO(eloc W)) {suc = canPPCapAndUFg(b,pla,eloc W,ploc);}
			else if(ISO(eloc E)) {suc = canPPCapAndUFg(b,pla,eloc E,ploc);}
			else if(ISO(eloc N)) {suc = canPPCapAndUFg(b,pla,eloc N,ploc);}
		}

		if(added) {b.owners[floc] = NPLA; b.pieces[floc] = EMP;}
		if(added2) {b.owners[floc2] = NPLA; b.pieces[floc2] = EMP;}

		if(suc) {return true;}
	}

	//Fan out one step away. Empty squares, try moving in pieces. Enemy squares, try pushing.
	//For an empty square, either the adjacent pieces are also empty, or they are enemies, or they are frozen friendlies, or rabbits.
	//For the case of frozen friendlies, we do NOT need to worry about the case where the frozen friendly is unfrozen in one step in a way
	//That causes it to block its movement to unfreeze the pushpulling piece. Because that would also be adjacent to the pushpulling piece,
	//which would unfreeze it immediately and allow a 3-step capture.

	if     (CS1(ploc) && ploc S != floc && ISE(ploc S)) {if(canSwapE2S(b,pla,ploc S,ploc,floc)) {return true;}}
	else if(CS1(ploc) && ploc S != floc && ISO(ploc S)) {if(canPushg(b,pla,ploc S)) {return true;}}

	if     (CW1(ploc) && ploc W != floc && ISE(ploc W)) {if(canSwapE2S(b,pla,ploc W,ploc,floc)) {return true;}}
	else if(CW1(ploc) && ploc W != floc && ISO(ploc W)) {if(canPushg(b,pla,ploc W)) {return true;}}

	if     (CE1(ploc) && ploc E != floc && ISE(ploc E)) {if(canSwapE2S(b,pla,ploc E,ploc,floc)) {return true;}}
	else if(CE1(ploc) && ploc E != floc && ISO(ploc E)) {if(canPushg(b,pla,ploc E)) {return true;}}

	if     (CN1(ploc) && ploc N != floc && ISE(ploc N)) {if(canSwapE2S(b,pla,ploc N,ploc,floc)) {return true;}}
	else if(CN1(ploc) && ploc N != floc && ISO(ploc N)) {if(canPushg(b,pla,ploc N)) {return true;}}

	return false;
}

static bool canPushg(Board& b, pla_t pla, loc_t eloc)
{
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CW1(eloc) && ISE(eloc W)) {SETGM((eloc MW,eloc S MN)) return true;}
		if(CE1(eloc) && ISE(eloc E)) {SETGM((eloc ME,eloc S MN))return true;}
		if(CN1(eloc) && ISE(eloc N)) {SETGM((eloc MN,eloc S MN))return true;}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc S)) {SETGM((eloc MS,eloc W ME))return true;}
		if(CE1(eloc) && ISE(eloc E)) {SETGM((eloc ME,eloc W ME))return true;}
		if(CN1(eloc) && ISE(eloc N)) {SETGM((eloc MN,eloc W ME))return true;}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc S)) {SETGM((eloc MS,eloc E MW))return true;}
		if(CW1(eloc) && ISE(eloc W)) {SETGM((eloc MW,eloc E MW))return true;}
		if(CN1(eloc) && ISE(eloc N)) {SETGM((eloc MN,eloc E MW))return true;}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CS1(eloc) && ISE(eloc S)) {SETGM((eloc MS,eloc N MS))return true;}
		if(CW1(eloc) && ISE(eloc W)) {SETGM((eloc MW,eloc N MS))return true;}
		if(CE1(eloc) && ISE(eloc E)) {SETGM((eloc ME,eloc N MS))return true;}
	}
	return false;
}


static bool canPullg(Board& b, pla_t pla, loc_t eloc)
{
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CS2(eloc) && ISE(eloc SS)) {SETGM((eloc S MS,eloc MS)) return true;}
		if(CW1(eloc) && ISE(eloc SW)) {SETGM((eloc S MW,eloc MS)) return true;}
		if(CE1(eloc) && ISE(eloc SE)) {SETGM((eloc S ME,eloc MS)) return true;}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc SW)) {SETGM((eloc W MS,eloc MW)) return true;}
		if(CW2(eloc) && ISE(eloc WW)) {SETGM((eloc W MW,eloc MW)) return true;}
		if(CN1(eloc) && ISE(eloc NW)) {SETGM((eloc W MN,eloc MW)) return true;}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc SE)) {SETGM((eloc E MS,eloc ME)) return true;}
		if(CE2(eloc) && ISE(eloc EE)) {SETGM((eloc E ME,eloc ME)) return true;}
		if(CN1(eloc) && ISE(eloc NE)) {SETGM((eloc E MN,eloc ME)) return true;}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CW1(eloc) && ISE(eloc NW)) {SETGM((eloc N MW,eloc MN)) return true;}
		if(CE1(eloc) && ISE(eloc NE)) {SETGM((eloc N ME,eloc MN)) return true;}
		if(CN2(eloc) && ISE(eloc NN)) {SETGM((eloc N MN,eloc MN)) return true;}
	}
	return false;
}

//Also a genPPCapAndUF version there
static bool canPPCapAndUFg(Board& b, pla_t pla, loc_t eloc, loc_t ploc)
{
	bool suc = false;
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc S,eloc,eloc W,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MW,eloc S MN))}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc S,eloc,eloc E,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc ME,eloc S MN))}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc S,eloc,eloc N,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MN,eloc S MN))}

		if(CS2(eloc) && ISE(eloc SS)) {TPC(eloc,eloc S,eloc SS,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc S MS,eloc MS))}
		if(CW1(eloc) && ISE(eloc SW)) {TPC(eloc,eloc S,eloc SW,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc S MW,eloc MS))}
		if(CE1(eloc) && ISE(eloc SE)) {TPC(eloc,eloc S,eloc SE,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc S ME,eloc MS))}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc W,eloc,eloc S,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MS,eloc W ME))}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc W,eloc,eloc E,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc ME,eloc W ME))}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc W,eloc,eloc N,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MN,eloc W ME))}

		if(CS1(eloc) && ISE(eloc SW)) {TPC(eloc,eloc W,eloc SW,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc W MS,eloc MW))}
		if(CW2(eloc) && ISE(eloc WW)) {TPC(eloc,eloc W,eloc WW,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc W MW,eloc MW))}
		if(CN1(eloc) && ISE(eloc NW)) {TPC(eloc,eloc W,eloc NW,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc W MN,eloc MW))}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc E,eloc,eloc S,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MS,eloc E MW))}
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc E,eloc,eloc W,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MW,eloc E MW))}
		if(CN1(eloc) && ISE(eloc N)) {TPC(eloc E,eloc,eloc N,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MN,eloc E MW))}

		if(CS1(eloc) && ISE(eloc SE)) {TPC(eloc,eloc E,eloc SE,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc E MS,eloc ME))}
		if(CE2(eloc) && ISE(eloc EE)) {TPC(eloc,eloc E,eloc EE,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc E ME,eloc ME))}
		if(CN1(eloc) && ISE(eloc NE)) {TPC(eloc,eloc E,eloc NE,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc E MN,eloc ME))}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CS1(eloc) && ISE(eloc S)) {TPC(eloc N,eloc,eloc S,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MS,eloc N MS))}
		if(CW1(eloc) && ISE(eloc W)) {TPC(eloc N,eloc,eloc W,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc MW,eloc N MS))}
		if(CE1(eloc) && ISE(eloc E)) {TPC(eloc N,eloc,eloc E,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc ME,eloc N MS))}

		if(CW1(eloc) && ISE(eloc NW)) {TPC(eloc,eloc N,eloc NW,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc N MW,eloc MN))}
		if(CE1(eloc) && ISE(eloc NE)) {TPC(eloc,eloc N,eloc NE,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc N ME,eloc MN))}
		if(CN2(eloc) && ISE(eloc NN)) {TPC(eloc,eloc N,eloc NN,suc = b.isThawedC(ploc)) RIFGM(suc,(eloc N MN,eloc MN))}
	}
	return false;
}

//Can we swap out an empty space at loc with a player piece to unfreeze ploc? Without blocking floc.
static bool canSwapE2S(Board& b, pla_t pla, loc_t loc, loc_t ploc, loc_t floc)
{
	if     (CS1(loc) && loc S != ploc && ISP(loc S) && b.isRabOkayN(pla,loc S) && canUF(b,pla,loc S,floc)) {return true;}
	else if(CS1(loc) && loc S != ploc && ISE(loc S) && b.isTrapSafe2(pla, loc S) && canStepStepg(b,pla,loc S,loc)) {return true;}

	if     (CW1(loc) && loc W != ploc && ISP(loc W)                            && canUF(b,pla,loc W,floc)) {return true;}
	else if(CW1(loc) && loc W != ploc && ISE(loc W) && b.isTrapSafe2(pla, loc W) && canStepStepg(b,pla,loc W,loc)) {return true;}

	if     (CE1(loc) && loc E != ploc && ISP(loc E)                            && canUF(b,pla,loc E,floc)) {return true;}
	else if(CE1(loc) && loc E != ploc && ISE(loc E) && b.isTrapSafe2(pla, loc E) && canStepStepg(b,pla,loc E,loc)) {return true;}

	if     (CN1(loc) && loc N != ploc && ISP(loc N) && b.isRabOkayS(pla,loc N) && canUF(b,pla,loc N,floc)) {return true;}
	else if(CN1(loc) && loc N != ploc && ISE(loc N) && b.isTrapSafe2(pla, loc N) && canStepStepg(b,pla,loc N,loc)) {return true;}

	return false;
}

static bool canStepStepg(Board& b, pla_t pla, loc_t dest, loc_t dest2)
{
	if((dest N == dest2 && pla == 0) || (dest S == dest2 && pla == 1))
	{
		if(CS1(dest) && ISP(dest S) && b.isThawedC(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.pieces[dest S] != RAB) {SETGM((dest S MN)) return true;}
		if(CW1(dest) && ISP(dest W) && b.isThawedC(dest W) && b.wouldBeUF(pla,dest W,dest,dest W) && b.pieces[dest W] != RAB) {SETGM((dest W ME)) return true;}
		if(CE1(dest) && ISP(dest E) && b.isThawedC(dest E) && b.wouldBeUF(pla,dest E,dest,dest E) && b.pieces[dest E] != RAB) {SETGM((dest E MW)) return true;}
		if(CN1(dest) && ISP(dest N) && b.isThawedC(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.pieces[dest N] != RAB) {SETGM((dest N MS)) return true;}
	}
	else
	{
		if(CS1(dest) && ISP(dest S) && b.isThawedC(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && b.isRabOkayN(pla,dest S)) {SETGM((dest S MN)) return true;}
		if(CW1(dest) && ISP(dest W) && b.isThawedC(dest W) && b.wouldBeUF(pla,dest W,dest,dest W))                             {SETGM((dest W ME)) return true;}
		if(CE1(dest) && ISP(dest E) && b.isThawedC(dest E) && b.wouldBeUF(pla,dest E,dest,dest E))                             {SETGM((dest E MW)) return true;}
		if(CN1(dest) && ISP(dest N) && b.isThawedC(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && b.isRabOkayS(pla,dest N)) {SETGM((dest N MS)) return true;}
	}
	return false;
}

//Does NOT use the C version of thawed or frozen!
static bool canPushAndGoal2SC(Board& b, pla_t pla, loc_t eloc, loc_t rloc)
{
	bool suc = false;

	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawed(eloc S))
	{
		if(CW1(eloc) && ISE(eloc W)) {TSC(eloc,eloc W,TS(eloc S,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MW,eloc S MN))}
		if(CE1(eloc) && ISE(eloc E)) {TSC(eloc,eloc E,TS(eloc S,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc ME,eloc S MN))}
		if(CN1(eloc) && ISE(eloc N)) {TSC(eloc,eloc N,TS(eloc S,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MN,eloc S MN))}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawed(eloc W))
	{
		if(CS1(eloc) && ISE(eloc S)) {TSC(eloc,eloc S,TS(eloc W,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MS,eloc W ME))}
		if(CE1(eloc) && ISE(eloc E)) {TSC(eloc,eloc E,TS(eloc W,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc ME,eloc W ME))}
		if(CN1(eloc) && ISE(eloc N)) {TSC(eloc,eloc N,TS(eloc W,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MN,eloc W ME))}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawed(eloc E))
	{
		if(CS1(eloc) && ISE(eloc S)) {TSC(eloc,eloc S,TS(eloc E,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MS,eloc E MW))}
		if(CW1(eloc) && ISE(eloc W)) {TSC(eloc,eloc W,TS(eloc E,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MW,eloc E MW))}
		if(CN1(eloc) && ISE(eloc N)) {TSC(eloc,eloc N,TS(eloc E,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MN,eloc E MW))}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawed(eloc N))
	{
		if(CS1(eloc) && ISE(eloc S)) {TSC(eloc,eloc S,TS(eloc N,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MS,eloc N MS))}
		if(CW1(eloc) && ISE(eloc W)) {TSC(eloc,eloc W,TS(eloc N,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc MW,eloc N MS))}
		if(CE1(eloc) && ISE(eloc E)) {TSC(eloc,eloc E,TS(eloc N,eloc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(eloc ME,eloc N MS))}
	}
	return false;
}

//Can we swap out an empty space at loc witha player piece to unfreeze ploc, then goal? Without blocking floc.
//Nasty position - requires a special check
// Rd..r.Rr
// hR*rh*rr
// H.Rre...
// ..c.D...
//Does NOT use the C version of thawed or frozen!
static bool canSwapEandGoal2S(Board& b, pla_t pla, loc_t loc, loc_t ploc, loc_t floc)
{
	if     (CS1(loc) && loc S != ploc && ISP(loc S) && b.isRabOkayN(pla,loc S) && canUFStepGoal2S(b,pla,loc S,loc,ploc,floc))     {return true;}
	else if(CS1(loc) && loc S != ploc && ISE(loc S) && b.isTrapSafe2(pla, loc S) && canMoveTo1SEndUFGoal2S(b,pla,loc S,loc,ploc)) {return true;}

	if     (CW1(loc) && loc W != ploc && ISP(loc W) && canUFStepGoal2S(b,pla,loc W,loc,ploc,floc))                                {return true;}
	else if(CW1(loc) && loc W != ploc && ISE(loc W) && b.isTrapSafe2(pla, loc W) && canMoveTo1SEndUFGoal2S(b,pla,loc W,loc,ploc)) {return true;}

	if     (CE1(loc) && loc E != ploc && ISP(loc E) && canUFStepGoal2S(b,pla,loc E,loc,ploc,floc))                                {return true;}
	else if(CE1(loc) && loc E != ploc && ISE(loc E) && b.isTrapSafe2(pla, loc E) && canMoveTo1SEndUFGoal2S(b,pla,loc E,loc,ploc)) {return true;}

	if     (CN1(loc) && loc N != ploc && ISP(loc N) && b.isRabOkayS(pla,loc N) && canUFStepGoal2S(b,pla,loc N,loc,ploc,floc))     {return true;}
	else if(CN1(loc) && loc N != ploc && ISE(loc N) && b.isTrapSafe2(pla, loc N) && canMoveTo1SEndUFGoal2S(b,pla,loc N,loc,ploc)) {return true;}

	return false;
}

//Unfreeze ploc, step to loc, and then rloc goal? without blocking floc
//Does NOT use the C version of thawed or frozen!
static bool canUFStepGoal2S(Board& b, pla_t pla, loc_t ploc, loc_t loc, loc_t rloc, loc_t floc)
{
	bool suc = false;
	step_t pstep = Board::STEPINDEX[ploc][loc];
	if(CS1(ploc) && ISE(ploc S) && ploc S != floc && ploc S != loc)
	{
		if(CS2(ploc) && ISP(ploc SS) && b.isThawed(ploc SS) && b.isRabOkayN(pla,ploc SS)) {TS(ploc SS,ploc S,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc SS MN,pstep))}
		if(CW1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW)                             ) {TS(ploc SW,ploc S,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc SW ME,pstep))}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE)                             ) {TS(ploc SE,ploc S,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc SE MW,pstep))}
	}
	if(CW1(ploc) && ISE(ploc W) && ploc W != floc && ploc W != loc)
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW) && b.isRabOkayN(pla,ploc SW)) {TS(ploc SW,ploc W,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc SW MN,pstep))}
		if(CW2(ploc) && ISP(ploc WW) && b.isThawed(ploc WW)                             ) {TS(ploc WW,ploc W,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc WW ME,pstep))}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW) && b.isRabOkayS(pla,ploc NW)) {TS(ploc NW,ploc W,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc NW MS,pstep))}
	}
	if(CE1(ploc) && ISE(ploc E) && ploc E != floc && ploc E != loc)
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE) && b.isRabOkayN(pla,ploc SE)) {TS(ploc SE,ploc E,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc SE MN,pstep))}
		if(CE2(ploc) && ISP(ploc EE) && b.isThawed(ploc EE)                             ) {TS(ploc EE,ploc E,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc EE MW,pstep))}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE) && b.isRabOkayS(pla,ploc NE)) {TS(ploc NE,ploc E,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc NE MS,pstep))}
	}
	if(CN1(ploc) && ISE(ploc N) && ploc N != floc && ploc N != loc)
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW)                             ) {TS(ploc NW,ploc N,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc NW ME,pstep))}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE)                             ) {TS(ploc NE,ploc N,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc NE MW,pstep))}
		if(CN2(ploc) && ISP(ploc NN) && b.isThawed(ploc NN) && b.isRabOkayS(pla,ploc NN)) {TS(ploc NN,ploc N,TS(ploc,loc,suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(ploc NN MS,pstep))}
	}
	return false;
}

//Contains special modifications for processing trap captures.
//Does NOT use the C version of thawed or frozen!
static bool canUF2AndGoal2S(Board& b, pla_t pla, loc_t ploc, loc_t floc)
{
	bool suc = false;
	pla_t opp = OPP(pla);
	//This needs to check traps...

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

		//Pieces around can pull?
		if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawed(eloc S))
		{
			if(CS2(eloc)                    && ISE(eloc SS)) {TPC(eloc,eloc S,eloc SS, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc S MS,eloc MS))}
			if(CW1(eloc) && eloc SW != floc && ISE(eloc SW)) {TPC(eloc,eloc S,eloc SW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc S MW,eloc MS))}
			if(CE1(eloc) && eloc SE != floc && ISE(eloc SE)) {TPC(eloc,eloc S,eloc SE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc S ME,eloc MS))}
		}
		if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawed(eloc W))
		{
			if(CS1(eloc) && eloc SW != floc && ISE(eloc SW)) {TPC(eloc,eloc W,eloc SW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc W MS,eloc MW))}
			if(CW2(eloc)                    && ISE(eloc WW)) {TPC(eloc,eloc W,eloc WW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc W MW,eloc MW))}
			if(CN1(eloc) && eloc NW != floc && ISE(eloc NW)) {TPC(eloc,eloc W,eloc NW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc W MN,eloc MW))}
		}
		if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawed(eloc E))
		{
			if(CS1(eloc) && eloc SE != floc && ISE(eloc SE)) {TPC(eloc,eloc E,eloc SE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc E MS,eloc ME))}
			if(CE2(eloc)                    && ISE(eloc EE)) {TPC(eloc,eloc E,eloc EE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc E ME,eloc ME))}
			if(CN1(eloc) && eloc NE != floc && ISE(eloc NE)) {TPC(eloc,eloc E,eloc NE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc E MN,eloc ME))}
		}
		if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawed(eloc N))
		{
			if(CW1(eloc) && eloc NW != floc && ISE(eloc NW)) {TPC(eloc,eloc N,eloc NW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc N MW,eloc MN))}
			if(CE1(eloc) && eloc NE != floc && ISE(eloc NE)) {TPC(eloc,eloc N,eloc NE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc N ME,eloc MN))}
			if(CN2(eloc)                    && ISE(eloc NN)) {TPC(eloc,eloc N,eloc NN, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc N MN,eloc MN))}
		}
	}

	//Fan out one step away. Empty squares, try moving in pieces. Enemy squares, try pushing.
	//For an empty square, either the adjacent pieces are also empty, or they are enemies, or they are frozen friendlies, or rabbits.
	//For the case of frozen friendlies, we do NOT need to worry about the case where the frozen friendly is unfrozen in one step in a way
	//That causes it to block its movement to unfreeze the pushpulling piece. Because that would also be adjacent to the pushpulling piece,
	//which would unfreeze it immediately and allow a 3-step capture.

	if     (CS1(ploc) && ploc S != floc && ISE(ploc S)) {if(canSwapEandGoal2S(b,pla, ploc S, ploc, floc)) {return true;}}
	else if(CS1(ploc) && ploc S != floc && ISO(ploc S)) {if(canPushAndGoal2SC(b,pla, ploc S, ploc)) {return true;}}

	if     (CW1(ploc) && ploc W != floc && ISE(ploc W)) {if(canSwapEandGoal2S(b,pla, ploc W, ploc, floc)) {return true;}}
	else if(CW1(ploc) && ploc W != floc && ISO(ploc W)) {if(canPushAndGoal2SC(b,pla, ploc W, ploc)) {return true;}}

	if     (CE1(ploc) && ploc E != floc && ISE(ploc E)) {if(canSwapEandGoal2S(b,pla, ploc E, ploc, floc)) {return true;}}
	else if(CE1(ploc) && ploc E != floc && ISO(ploc E)) {if(canPushAndGoal2SC(b,pla, ploc E, ploc)) {return true;}}

	if     (CN1(ploc) && ploc N != floc && ISE(ploc N)) {if(canSwapEandGoal2S(b,pla, ploc N, ploc, floc)) {return true;}}
	else if(CN1(ploc) && ploc N != floc && ISO(ploc N)) {if(canPushAndGoal2SC(b,pla, ploc N, ploc)) {return true;}}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static bool canUF2ForceAndGoal2S(Board& b, pla_t pla, loc_t ploc, loc_t floc)
{
	//This needs to check traps...

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

		//Pieces around can pull?
    bool suc = false;
		if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawed(eloc S))
		{
			if(CW1(eloc) && eloc SW == floc && ISE(eloc SW)) {TP(eloc,eloc S,eloc SW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc S MW,eloc MS))}
			if(CE1(eloc) && eloc SE == floc && ISE(eloc SE)) {TP(eloc,eloc S,eloc SE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc S ME,eloc MS))}
		}
		if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawed(eloc W))
		{
			if(CS1(eloc) && eloc SW == floc && ISE(eloc SW)) {TP(eloc,eloc W,eloc SW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc W MS,eloc MW))}
			if(CN1(eloc) && eloc NW == floc && ISE(eloc NW)) {TP(eloc,eloc W,eloc NW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc W MN,eloc MW))}
		}
		if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawed(eloc E))
		{
			if(CS1(eloc) && eloc SE == floc && ISE(eloc SE)) {TP(eloc,eloc E,eloc SE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc E MS,eloc ME))}
			if(CN1(eloc) && eloc NE == floc && ISE(eloc NE)) {TP(eloc,eloc E,eloc NE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc E MN,eloc ME))}
		}
		if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawed(eloc N))
		{
			if(CW1(eloc) && eloc NW == floc && ISE(eloc NW)) {TP(eloc,eloc N,eloc NW, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc N MW,eloc MN))}
			if(CE1(eloc) && eloc NE == floc && ISE(eloc NE)) {TP(eloc,eloc N,eloc NE, suc = canGoal2S(b,pla,ploc)) RIFGM(suc,(eloc N ME,eloc MN))}
		}
	}

	//Fan out one step away. Empty squares, try moving in pieces. Enemy squares, try pushing.
	//For an empty square, either the adjacent pieces are also empty, or they are enemies, or they are frozen friendlies, or rabbits.
	//For the case of frozen friendlies, we do NOT need to worry about the case where the frozen friendly is unfrozen in one step in a way
	//That causes it to block its movement to unfreeze the pushpulling piece. Because that would also be adjacent to the pushpulling piece,
	//which would unfreeze it immediately and allow a 3-step capture.

	if(ISE(floc))
	{
		if(canSwapEandGoal2S(b,pla, floc, ploc, floc)) {return true;}
	}

	return false;
}

//Can push eloc with some player, ending unfrozen, and without pushing to either floc or floc2
//However, it is okay to push to floc if the piece is captured
//Does NOT use the C version of thawed or frozen!
static bool canPushEndUF(Board& b, pla_t pla, loc_t eloc, loc_t floc, loc_t floc2)
{
	pla_t opp = OPP(pla);
	loc_t caploc = Board::ADJACENTTRAP[eloc];
	loc_t ign2;
	if(caploc != ERRORSQUARE && ISO(caploc) && !b.isTrapSafe2(opp,caploc))
	  ign2 = caploc;
	else
	  ign2 = ERRORSQUARE;

	loc_t pusherLoc = ERRORSQUARE;
  if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawed(eloc S) && b.wouldBeUF(pla,eloc S,eloc,eloc S,ign2)) {pusherLoc = eloc S;}
  if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawed(eloc W) && b.wouldBeUF(pla,eloc W,eloc,eloc W,ign2)) {pusherLoc = eloc W;}
  if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawed(eloc E) && b.wouldBeUF(pla,eloc E,eloc,eloc E,ign2)) {pusherLoc = eloc E;}
  if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawed(eloc N) && b.wouldBeUF(pla,eloc N,eloc,eloc N,ign2)) {pusherLoc = eloc N;}

  loc_t pushToLoc = ERRORSQUARE;
  if(CS1(eloc) && (eloc S != floc || !b.isTrapSafe2(opp,eloc S)) && eloc S != floc2 && ISE(eloc S)) {pushToLoc = eloc S;}
  if(CW1(eloc) && (eloc W != floc || !b.isTrapSafe2(opp,eloc W)) && eloc W != floc2 && ISE(eloc W)) {pushToLoc = eloc W;}
  if(CE1(eloc) && (eloc E != floc || !b.isTrapSafe2(opp,eloc E)) && eloc E != floc2 && ISE(eloc E)) {pushToLoc = eloc E;}
  if(CN1(eloc) && (eloc N != floc || !b.isTrapSafe2(opp,eloc N)) && eloc N != floc2 && ISE(eloc N)) {pushToLoc = eloc N;}

  if(pusherLoc != ERRORSQUARE && pushToLoc != ERRORSQUARE)
  {SETGM((Board::STEPINDEX[eloc][pushToLoc],Board::STEPINDEX[pusherLoc][eloc])); return true;}

  return false;
}

//Does NOT use the C version of thawed or frozen!
static bool canMoveTo1SEndUFGoal2S(Board& b, pla_t pla, loc_t dest, loc_t dest2, loc_t rloc)
{
	pla_t forbiddenPla = NPLA;
	if(dest N == dest2)
	{forbiddenPla = 0;}
	else if(dest S == dest2)
	{forbiddenPla = 1;}

	//Double rabbit check!!!!!!!!!!!!!!!
  bool suc = false;
	step_t dstep = Board::STEPINDEX[dest][dest2];
	if(CS1(dest) && ISP(dest S) && b.isThawed(dest S) && b.wouldBeUF(pla,dest S,dest,dest S) && (b.pieces[dest S] != RAB || pla != forbiddenPla) && (b.pieces[dest S] != RAB || pla == 1)) {TSC(dest S,dest, TS(dest,dest2, suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(dest S MN,dstep))}
	if(CW1(dest) && ISP(dest W) && b.isThawed(dest W) && b.wouldBeUF(pla,dest W,dest,dest W) && (b.pieces[dest W] != RAB || pla != forbiddenPla))                                          {TSC(dest W,dest, TS(dest,dest2, suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(dest W ME,dstep))}
	if(CE1(dest) && ISP(dest E) && b.isThawed(dest E) && b.wouldBeUF(pla,dest E,dest,dest E) && (b.pieces[dest E] != RAB || pla != forbiddenPla))                                          {TSC(dest E,dest, TS(dest,dest2, suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(dest E MW,dstep))}
	if(CN1(dest) && ISP(dest N) && b.isThawed(dest N) && b.wouldBeUF(pla,dest N,dest,dest N) && (b.pieces[dest N] != RAB || pla != forbiddenPla) && (b.pieces[dest N] != RAB || pla == 0)) {TSC(dest N,dest, TS(dest,dest2, suc = canGoal2S(b,pla,rloc))) RIFGM(suc,(dest N MS,dstep))}

	return false;
}

//Can unfreeze and move pla from ploc to dest and then on to a location adjacent to dest?
//DOES NOT CHECK RABBIT OKAY!
static bool canUFandMoveTo1S(Board& b, pla_t pla, loc_t ploc, loc_t dest)
{
	//This includes sacrifical unfreezing (ending on a trap square, so that when the unfrozen piece moves,
	//the unfreezing piece dies). For move ordering heuristics, add a check!!!
  bool suc = false;
	if(CS1(ploc) && ISE(ploc S) && ploc S != dest)
	{
    if(CS2(ploc) && ISP(ploc SS) && b.isThawedC(ploc SS) && b.isRabOkayN(pla,ploc SS)) {TS(ploc SS,ploc S,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc SS MN))}
    if(CW1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW))                              {TS(ploc SW,ploc S,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc SW ME))}
    if(CE1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE))                              {TS(ploc SE,ploc S,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc SE MW))}
	}
  if(CW1(ploc) && ISE(ploc W) && ploc W != dest)
  {
    if(CS1(ploc) && ISP(ploc SW) && b.isThawedC(ploc SW) && b.isRabOkayN(pla,ploc SW)) {TS(ploc SW,ploc W,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc SW MN))}
    if(CW2(ploc) && ISP(ploc WW) && b.isThawedC(ploc WW))                              {TS(ploc WW,ploc W,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc WW ME))}
    if(CN1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW) && b.isRabOkayS(pla,ploc NW)) {TS(ploc NW,ploc W,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc NW MS))}
  }
  if(CE1(ploc) && ISE(ploc E) && ploc E != dest)
  {
    if(CS1(ploc) && ISP(ploc SE) && b.isThawedC(ploc SE) && b.isRabOkayN(pla,ploc SE)) {TS(ploc SE,ploc E,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc SE MN))}
    if(CE2(ploc) && ISP(ploc EE) && b.isThawedC(ploc EE))                              {TS(ploc EE,ploc E,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc EE MW))}
    if(CN1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE) && b.isRabOkayS(pla,ploc NE)) {TS(ploc NE,ploc E,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc NE MS))}
  }
  if(CN1(ploc) && ISE(ploc N) && ploc N != dest)
  {
    if(CW1(ploc) && ISP(ploc NW) && b.isThawedC(ploc NW))                              {TS(ploc NW,ploc N,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc NW ME))}
    if(CE1(ploc) && ISP(ploc NE) && b.isThawedC(ploc NE))                              {TS(ploc NE,ploc N,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc NE MW))}
    if(CN2(ploc) && ISP(ploc NN) && b.isThawedC(ploc NN) && b.isRabOkayS(pla,ploc NN)) {TS(ploc NN,ploc N,suc = b.wouldBeUF(pla, ploc, dest, ploc)) RIFGM(suc,(ploc NN MS))}
  }
	return false;
}

static bool canAdvanceUFStepGTree(Board& b, pla_t pla, loc_t curploc, loc_t futploc)
{
  if(!b.isTrapSafe2(pla,curploc))
  {return false;}

  if(futploc == curploc S)
  {
    if(CW1(curploc) && ISP(curploc W) && ISE(futploc W) && b.isTrapSafe2(pla,futploc W) && b.wouldBeUF(pla,curploc,curploc,curploc W) && b.isRabOkayS(pla,curploc W)) {SETGM((curploc W MS)); return true;}
    if(CE1(curploc) && ISP(curploc E) && ISE(futploc E) && b.isTrapSafe2(pla,futploc E) && b.wouldBeUF(pla,curploc,curploc,curploc E) && b.isRabOkayS(pla,curploc E)) {SETGM((curploc E MS)); return true;}
  }
  else if(futploc == curploc W)
  {
    if(CS1(curploc) && ISP(curploc S) && ISE(futploc S) && b.isTrapSafe2(pla,futploc S) && b.wouldBeUF(pla,curploc,curploc,curploc S)) {SETGM((curploc S MW)); return true;}
    if(CN1(curploc) && ISP(curploc N) && ISE(futploc N) && b.isTrapSafe2(pla,futploc N) && b.wouldBeUF(pla,curploc,curploc,curploc N)) {SETGM((curploc N MW)); return true;}
  }
  else if(futploc == curploc E)
  {
    if(CS1(curploc) && ISP(curploc S) && ISE(futploc S) && b.isTrapSafe2(pla,futploc S) && b.wouldBeUF(pla,curploc,curploc,curploc S)) {SETGM((curploc S ME)); return true;}
    if(CN1(curploc) && ISP(curploc N) && ISE(futploc N) && b.isTrapSafe2(pla,futploc N) && b.wouldBeUF(pla,curploc,curploc,curploc N)) {SETGM((curploc N ME)); return true;}
  }
  else if(futploc == curploc N)
  {
    if(CW1(curploc) && ISP(curploc W) && ISE(futploc W) && b.isTrapSafe2(pla,futploc W) && b.wouldBeUF(pla,curploc,curploc,curploc W) && b.isRabOkayN(pla,curploc W)) {SETGM((curploc W MN)); return true;}
    if(CE1(curploc) && ISP(curploc E) && ISE(futploc E) && b.isTrapSafe2(pla,futploc E) && b.wouldBeUF(pla,curploc,curploc,curploc E) && b.isRabOkayN(pla,curploc E)) {SETGM((curploc E MN)); return true;}
  }
  return false;
}

//Can we step to dest and end unfrozen by pulling a defender and thereby capturing
//the piece that would freeze us? Then move to dest2.
//Assumes currently unfrozen and ploc adjacent to dest and dest empty and dest adjacent to dest2 and dest2 empty
//Does NOT use the C version of thawed or frozen!
// +--------+
//8|........|
//7|...R..H.|
//6|..*c.mc.|
//5|........|
static bool canSelfUFByPullCap(Board& b, pla_t pla, loc_t ploc, loc_t dest, loc_t dest2)
{
	pla_t opp = OPP(pla);
	loc_t kt = Board::ADJACENTTRAP[dest];
	if(kt == ERRORSQUARE)
		return false;

	//Hanging opponent on a trap that would freeze us at dest
	if(b.owners[kt] == opp && b.pieces[kt] > b.pieces[ploc] && b.trapGuardCounts[opp][Board::TRAPINDEX[kt]] == 1
			&& !b.isGuarded2(pla,dest))
	{
		loc_t defender = b.findGuard(opp,kt);
		//We are able to pull as we move and would be uf if the trap opp was captured
		if(b.pieces[defender] < b.pieces[ploc] && Board::ISADJACENT[ploc][defender] && !b.wouldBeDom(pla,ploc,dest,kt))
		{
			//It works!
			SETGM((Board::STEPINDEX[ploc][dest],Board::STEPINDEX[defender][ploc],Board::STEPINDEX[dest][dest2]));
			return true;
		}
	}
	return false;
}

//Does NOT use the C version of thawed or frozen!
static bool canMoveTo2SEndUF(Board& b, pla_t pla, loc_t dest, loc_t dest2, loc_t floc)
{
	pla_t forbiddenPla = NPLA;
	if     (dest N == dest2) {forbiddenPla = 0;}
	else if(dest S == dest2) {forbiddenPla = 1;}

	bool suc = false;

	if(CS1(dest) && ISP(dest S) && (b.pieces[dest S] != RAB || pla == 1) && (b.pieces[dest S] != RAB || pla != forbiddenPla))
	{
		if(b.isThawed(dest S)) {TSC(dest S,dest, suc = canUF(b,pla,dest,floc)) RIFGM(suc,(dest S MN)) suc = canAdvanceUFStepGTree(b,pla, dest S, dest); RIF(suc) suc = canSelfUFByPullCap(b,pla,dest S,dest,dest2); RIF(suc)}
		else if(canUFandMoveTo1S(b,pla,dest S,dest)) {return true;}
	}

	else if(CS1(dest) && ISE(dest S) && b.isTrapSafe2(pla,dest S))
	{
		if(CS2(dest) && ISP(dest SS) && b.isThawed(dest SS) && b.wouldBeUF(pla,dest SS,dest S,dest SS) && b.wouldBeUF(pla,dest SS,dest,b.getSacLoc(pla,dest SS)) && (b.pieces[dest SS] != RAB || pla != forbiddenPla) && (b.pieces[dest SS] != RAB || pla == 1)){SETGM((dest SS MN)); return true;}
		if(CW1(dest) && ISP(dest SW) && b.isThawed(dest SW) && b.wouldBeUF(pla,dest SW,dest S,dest SW) && b.wouldBeUF(pla,dest SW,dest,b.getSacLoc(pla,dest SW)) && (b.pieces[dest SW] != RAB || pla != forbiddenPla) && (b.pieces[dest SW] != RAB || pla == 1)){SETGM((dest SW ME)); return true;}
		if(CE1(dest) && ISP(dest SE) && b.isThawed(dest SE) && b.wouldBeUF(pla,dest SE,dest S,dest SE) && b.wouldBeUF(pla,dest SE,dest,b.getSacLoc(pla,dest SE)) && (b.pieces[dest SE] != RAB || pla != forbiddenPla) && (b.pieces[dest SE] != RAB || pla == 1)){SETGM((dest SE MW)); return true;}
	}

	if(CW1(dest) && ISP(dest W) && (b.pieces[dest W] != RAB || pla != forbiddenPla))
	{
		if(b.isThawed(dest W)) {TSC(dest W,dest, suc = canUF(b,pla,dest,floc)) RIFGM(suc,(dest W ME)) suc = canAdvanceUFStepGTree(b,pla, dest W, dest); RIF(suc) suc = canSelfUFByPullCap(b,pla,dest W,dest,dest2); RIF(suc)}
		else if(canUFandMoveTo1S(b,pla,dest W,dest)) {return true;}
	}

	else if(CW1(dest) && ISE(dest W) && b.isTrapSafe2(pla,dest W))
	{
		if(CS1(dest) && ISP(dest SW) && b.isThawed(dest SW) && b.wouldBeUF(pla,dest SW,dest W,dest SW) && b.wouldBeUF(pla,dest SW,dest,b.getSacLoc(pla,dest SW)) && (b.pieces[dest SW] != RAB || pla != forbiddenPla) && (b.pieces[dest SW] != RAB || pla == 1)){SETGM((dest SW MN)); return true;}
		if(CW2(dest) && ISP(dest WW) && b.isThawed(dest WW) && b.wouldBeUF(pla,dest WW,dest W,dest WW) && b.wouldBeUF(pla,dest WW,dest,b.getSacLoc(pla,dest WW)) && (b.pieces[dest WW] != RAB || pla != forbiddenPla))                                          {SETGM((dest WW ME)); return true;}
		if(CN1(dest) && ISP(dest NW) && b.isThawed(dest NW) && b.wouldBeUF(pla,dest NW,dest W,dest NW) && b.wouldBeUF(pla,dest NW,dest,b.getSacLoc(pla,dest NW)) && (b.pieces[dest NW] != RAB || pla != forbiddenPla) && (b.pieces[dest NW] != RAB || pla == 0)){SETGM((dest NW MS)); return true;}
	}

	if(CE1(dest) && ISP(dest E) && (b.pieces[dest E] != RAB || pla != forbiddenPla))
	{
		if(b.isThawed(dest E)) {TSC(dest E,dest, suc = canUF(b,pla,dest,floc)) RIFGM(suc,(dest E MW)) suc = canAdvanceUFStepGTree(b,pla, dest E, dest); RIF(suc) suc = canSelfUFByPullCap(b,pla,dest E,dest,dest2); RIF(suc)}
		else if(canUFandMoveTo1S(b,pla,dest E,dest)) {return true;}
	}

	else if(CE1(dest) && ISE(dest E) && b.isTrapSafe2(pla,dest E))
	{
		if(CS1(dest) && ISP(dest SE) && b.isThawed(dest SE) && b.wouldBeUF(pla,dest SE,dest E,dest SE) && b.wouldBeUF(pla,dest SE,dest,b.getSacLoc(pla,dest SE)) && (b.pieces[dest SE] != RAB || pla != forbiddenPla) && (b.pieces[dest SE] != RAB || pla == 1)){SETGM((dest SE MN)); return true;}
		if(CE2(dest) && ISP(dest EE) && b.isThawed(dest EE) && b.wouldBeUF(pla,dest EE,dest E,dest EE) && b.wouldBeUF(pla,dest EE,dest,b.getSacLoc(pla,dest EE)) && (b.pieces[dest EE] != RAB || pla != forbiddenPla))                                          {SETGM((dest EE MW)); return true;}
		if(CN1(dest) && ISP(dest NE) && b.isThawed(dest NE) && b.wouldBeUF(pla,dest NE,dest E,dest NE) && b.wouldBeUF(pla,dest NE,dest,b.getSacLoc(pla,dest NE)) && (b.pieces[dest NE] != RAB || pla != forbiddenPla) && (b.pieces[dest NE] != RAB || pla == 0)){SETGM((dest NE MS)); return true;}
	}

	if(CN1(dest) && ISP(dest N) && (b.pieces[dest N] != RAB || pla == 0) && (b.pieces[dest N] != RAB || pla != forbiddenPla))
	{
		if(b.isThawed(dest N)) {TSC(dest N,dest, suc = canUF(b,pla,dest,floc)) RIFGM(suc,(dest N MS)) suc = canAdvanceUFStepGTree(b,pla, dest N, dest); RIF(suc) suc = canSelfUFByPullCap(b,pla,dest N,dest,dest2); RIF(suc)}
		else if(canUFandMoveTo1S(b,pla,dest N,dest)) {return true;}
	}

	else if(CN1(dest) && ISE(dest N) && b.isTrapSafe2(pla,dest N))
	{
		if(CW1(dest) && ISP(dest NW) && b.isThawed(dest NW) && b.wouldBeUF(pla,dest NW,dest N,dest NW) && b.wouldBeUF(pla,dest NW,dest,b.getSacLoc(pla,dest NW)) && (b.pieces[dest NW] != RAB || pla != forbiddenPla) && (b.pieces[dest NW] != RAB || pla == 0)){SETGM((dest NW ME));return true;}
		if(CE1(dest) && ISP(dest NE) && b.isThawed(dest NE) && b.wouldBeUF(pla,dest NE,dest N,dest NE) && b.wouldBeUF(pla,dest NE,dest,b.getSacLoc(pla,dest NE)) && (b.pieces[dest NE] != RAB || pla != forbiddenPla) && (b.pieces[dest NE] != RAB || pla == 0)){SETGM((dest NE MW));return true;}
		if(CN2(dest) && ISP(dest NN) && b.isThawed(dest NN) && b.wouldBeUF(pla,dest NN,dest N,dest NN) && b.wouldBeUF(pla,dest NN,dest,b.getSacLoc(pla,dest NN)) && (b.pieces[dest NN] != RAB || pla != forbiddenPla) && (b.pieces[dest NN] != RAB || pla == 0)){SETGM((dest NN MS));return true;}
	}

	return false;
}

static bool isOpenToStepGTree(Board& b, loc_t k)
{
  if(CS1(k) && ISE(k S) && b.isRabOkayS(b.owners[k],k)) {SETGM((k MS)); return true;}
  if(CW1(k) && ISE(k W))                                {SETGM((k MW)); return true;}
  if(CE1(k) && ISE(k E))                                {SETGM((k ME)); return true;}
  if(CN1(k) && ISE(k N) && b.isRabOkayN(b.owners[k],k)) {SETGM((k MN)); return true;}
  return false;
}

//Does NOT use the C version of thawed or frozen!
static bool canPull3S(Board& b, pla_t pla, loc_t eloc)
{
	//Pieces around can pull?
	if(CS1(eloc) && ISE(eloc S))
	{
		if(CS2(eloc) && ISP(eloc SS) && GT(eloc SS,eloc) && b.isThawed(eloc SS) && b.wouldBeUF(pla,eloc SS,eloc S,eloc SS) && b.isTrapSafe2(pla,eloc S)) {SETGM((eloc SS MN)); return true;}
		if(CW1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.isThawed(eloc SW) && b.wouldBeUF(pla,eloc SW,eloc S,eloc SW) && b.isTrapSafe2(pla,eloc S)) {SETGM((eloc SW ME)); return true;}
		if(CE1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.isThawed(eloc SE) && b.wouldBeUF(pla,eloc SE,eloc S,eloc SE) && b.isTrapSafe2(pla,eloc S)) {SETGM((eloc SE MW)); return true;}
	}
	if(CW1(eloc) && ISE(eloc W))
	{
		if(CS1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.isThawed(eloc SW) && b.wouldBeUF(pla,eloc SW,eloc W,eloc SW) && b.isTrapSafe2(pla,eloc W)) {SETGM((eloc SW MN)); return true;}
		if(CW2(eloc) && ISP(eloc WW) && GT(eloc WW,eloc) && b.isThawed(eloc WW) && b.wouldBeUF(pla,eloc WW,eloc W,eloc WW) && b.isTrapSafe2(pla,eloc W)) {SETGM((eloc WW ME)); return true;}
		if(CN1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.isThawed(eloc NW) && b.wouldBeUF(pla,eloc NW,eloc W,eloc NW) && b.isTrapSafe2(pla,eloc W)) {SETGM((eloc NW MS)); return true;}
	}
	if(CE1(eloc) && ISE(eloc E))
	{
		if(CS1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.isThawed(eloc SE) && b.wouldBeUF(pla,eloc SE,eloc E,eloc SE) && b.isTrapSafe2(pla,eloc E)) {SETGM((eloc SE MN)); return true;}
		if(CE2(eloc) && ISP(eloc EE) && GT(eloc EE,eloc) && b.isThawed(eloc EE) && b.wouldBeUF(pla,eloc EE,eloc E,eloc EE) && b.isTrapSafe2(pla,eloc E)) {SETGM((eloc EE MW)); return true;}
		if(CN1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.isThawed(eloc NE) && b.wouldBeUF(pla,eloc NE,eloc E,eloc NE) && b.isTrapSafe2(pla,eloc E)) {SETGM((eloc NE MS)); return true;}
	}
	if(CN1(eloc) && ISE(eloc N))
	{
		if(CW1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.isThawed(eloc NW) && b.wouldBeUF(pla,eloc NW,eloc N,eloc NW) && b.isTrapSafe2(pla,eloc N)) {SETGM((eloc NW ME)); return true;}
		if(CE1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.isThawed(eloc NE) && b.wouldBeUF(pla,eloc NE,eloc N,eloc NE) && b.isTrapSafe2(pla,eloc N)) {SETGM((eloc NE MW)); return true;}
		if(CN2(eloc) && ISP(eloc NN) && GT(eloc NN,eloc) && b.isThawed(eloc NN) && b.wouldBeUF(pla,eloc NN,eloc N,eloc NN) && b.isTrapSafe2(pla,eloc N)) {SETGM((eloc NN MS)); return true;}
	}

	//Piece adjacent. Try to unfreeze, or to make space to pull
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isFrozen(eloc S) && b.isOpen2(eloc S) && canUF(b,pla,eloc S,ERRORSQUARE))
	{return true;}
	else if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawed(eloc S) && b.isTrapSafe2(pla,eloc S) && (!b.isDominated(eloc S) || b.isGuarded2(pla,eloc S)))
	{
		if(CS2(eloc) && ISP(eloc SS) && isOpenToStepGTree(b,eloc SS)) {return true;}
		if(CW1(eloc) && ISP(eloc SW) && isOpenToStepGTree(b,eloc SW)) {return true;}
		if(CE1(eloc) && ISP(eloc SE) && isOpenToStepGTree(b,eloc SE)) {return true;}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isFrozen(eloc W) && b.isOpen2(eloc W) && canUF(b,pla,eloc W,ERRORSQUARE))
	{return true;}
	else if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawed(eloc W) && b.isTrapSafe2(pla,eloc W) && (!b.isDominated(eloc W) || b.isGuarded2(pla,eloc W)))
	{
		if(CS1(eloc) && ISP(eloc SW) && isOpenToStepGTree(b,eloc SW)) {return true;}
		if(CW2(eloc) && ISP(eloc WW) && isOpenToStepGTree(b,eloc WW)) {return true;}
		if(CN1(eloc) && ISP(eloc NW) && isOpenToStepGTree(b,eloc NW)) {return true;}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isFrozen(eloc E) && b.isOpen2(eloc E) && canUF(b,pla,eloc E,ERRORSQUARE))
	{return true;}
	else if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawed(eloc E) && b.isTrapSafe2(pla,eloc E) && (!b.isDominated(eloc E) || b.isGuarded2(pla,eloc E)))
	{
		if(CS1(eloc) && ISP(eloc SE) && isOpenToStepGTree(b,eloc SE)) {return true;}
		if(CE2(eloc) && ISP(eloc EE) && isOpenToStepGTree(b,eloc EE)) {return true;}
		if(CN1(eloc) && ISP(eloc NE) && isOpenToStepGTree(b,eloc NE)) {return true;}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isFrozen(eloc N) && b.isOpen2(eloc N) && canUF(b,pla,eloc N,ERRORSQUARE))
	{return true;}
	else if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawed(eloc N) && b.isTrapSafe2(pla,eloc N) && (!b.isDominated(eloc N) || b.isGuarded2(pla,eloc N)))
	{
		if(CW1(eloc) && ISP(eloc NW) && isOpenToStepGTree(b,eloc NW)) {return true;}
		if(CE1(eloc) && ISP(eloc NE) && isOpenToStepGTree(b,eloc NE)) {return true;}
		if(CN2(eloc) && ISP(eloc NN) && isOpenToStepGTree(b,eloc NN)) {return true;}
	}

	return false;
}

//Can we unfreeze ploc and have it push eloc in 3 steps, without covering a forbidden spot by the unfreezer?
//Assumes already that ploc is big enough to push eloc and is adjacent
//Does NOT use the C version of thawed or frozen!
static bool canUFPushPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t floc)
{
	if(b.isOpen(eloc))
	{return canUF(b,pla,ploc,floc);}

	//Consider cases where we move the unfreezer and the spot vacated allows the push!
	if(CS1(ploc) && ISE(ploc S) && ploc S != floc)
	{
		if(CW1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW)                              && b.isAdjacent(ploc SW,eloc)) {SETGM((ploc SW ME)); return true;}
		if(CE1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE)                              && b.isAdjacent(ploc SE,eloc)) {SETGM((ploc SE MW)); return true;}
	}
	if(CW1(ploc) && ISE(ploc W) && ploc W != floc)
	{
		if(CS1(ploc) && ISP(ploc SW) && b.isThawed(ploc SW) && b.isRabOkayN(pla,ploc SW) && b.isAdjacent(ploc SW,eloc)) {SETGM((ploc SW MN)); return true;}
		if(CN1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW) && b.isRabOkayS(pla,ploc NW) && b.isAdjacent(ploc NW,eloc)) {SETGM((ploc NW MS)); return true;}
	}
	if(CE1(ploc) && ISE(ploc E) && ploc E != floc)
	{
		if(CS1(ploc) && ISP(ploc SE) && b.isThawed(ploc SE) && b.isRabOkayN(pla,ploc SE) && b.isAdjacent(ploc SE,eloc)) {SETGM((ploc SE MN)); return true;}
		if(CN1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE) && b.isRabOkayS(pla,ploc NE) && b.isAdjacent(ploc NE,eloc)) {SETGM((ploc NE MS)); return true;}
	}
	if(CN1(ploc) && ISE(ploc N) && ploc N != floc)
	{
		if(CW1(ploc) && ISP(ploc NW) && b.isThawed(ploc NW)                              && b.isAdjacent(ploc NW,eloc)) {SETGM((ploc NW ME)); return true;}
		if(CE1(ploc) && ISP(ploc NE) && b.isThawed(ploc NE)                              && b.isAdjacent(ploc NE,eloc)) {SETGM((ploc NE MW)); return true;}
	}
	return false;
}

//Can we move a piece away from eloc to clear room for a push, but not step a piece into floc?
//Assumes that ploc is unfrozen already.
//Includes sacrifical removals!
//Does NOT use the C version of thawed or frozen!
static bool canUnblockPushPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t floc)
{
	if(CS1(eloc) && ISP(eloc S) && eloc S != ploc && b.isThawed(eloc S))
	{
		if(CS2(eloc) && eloc SS != floc && ISE(eloc SS) && b.isRabOkayS(pla,eloc S))  {SETGM((eloc S MS)); return true;}
		if(CW1(eloc) && eloc SW != floc && ISE(eloc SW))                              {SETGM((eloc S MW)); return true;}
		if(CE1(eloc) && eloc SE != floc && ISE(eloc SE))                              {SETGM((eloc S ME)); return true;}

		if(!b.isTrapSafe2(pla,eloc S))
		{
			if(CS2(eloc) && ISP(eloc SS) && b.wouldBeUF(pla,ploc,ploc,eloc SS) && isOpenToStepGTree(b,eloc SS)) {return true;}
			if(CW1(eloc) && ISP(eloc SW) && b.wouldBeUF(pla,ploc,ploc,eloc SW) && isOpenToStepGTree(b,eloc SW)) {return true;}
			if(CE1(eloc) && ISP(eloc SE) && b.wouldBeUF(pla,ploc,ploc,eloc SE) && isOpenToStepGTree(b,eloc SE)) {return true;}
		}
	}
	if(CW1(eloc) && ISP(eloc W) && eloc W != ploc && b.isThawed(eloc W))
	{
		if(CS1(eloc) && eloc SW != floc && ISE(eloc SW) && b.isRabOkayS(pla,eloc W))  {SETGM((eloc W MS)); return true;}
		if(CW2(eloc) && eloc WW != floc && ISE(eloc WW))                              {SETGM((eloc W MW)); return true;}
		if(CN1(eloc) && eloc NW != floc && ISE(eloc NW) && b.isRabOkayN(pla,eloc W))  {SETGM((eloc W MN)); return true;}

		if(!b.isTrapSafe2(pla,eloc W))
		{
			if(CS1(eloc) && ISP(eloc SW) && b.wouldBeUF(pla,ploc,ploc,eloc SW) && isOpenToStepGTree(b,eloc SW)) {return true;}
			if(CW2(eloc) && ISP(eloc WW) && b.wouldBeUF(pla,ploc,ploc,eloc WW) && isOpenToStepGTree(b,eloc WW)) {return true;}
			if(CN1(eloc) && ISP(eloc NW) && b.wouldBeUF(pla,ploc,ploc,eloc NW) && isOpenToStepGTree(b,eloc NW)) {return true;}
		}
	}
	if(CE1(eloc) && ISP(eloc E) && eloc E != ploc && b.isThawed(eloc E))
	{
		if(CS1(eloc) && eloc SE != floc && ISE(eloc SE) && b.isRabOkayS(pla,eloc E))  {SETGM((eloc E MS)); return true;}
		if(CE2(eloc) && eloc EE != floc && ISE(eloc EE))                              {SETGM((eloc E ME)); return true;}
		if(CN1(eloc) && eloc NE != floc && ISE(eloc NE) && b.isRabOkayN(pla,eloc E))  {SETGM((eloc E MN)); return true;}

		if(!b.isTrapSafe2(pla,eloc E))
		{
			if(CS1(eloc) && ISP(eloc SE) && b.wouldBeUF(pla,ploc,ploc,eloc SE) && isOpenToStepGTree(b,eloc SE)) {return true;}
			if(CE2(eloc) && ISP(eloc EE) && b.wouldBeUF(pla,ploc,ploc,eloc EE) && isOpenToStepGTree(b,eloc EE)) {return true;}
			if(CN1(eloc) && ISP(eloc NE) && b.wouldBeUF(pla,ploc,ploc,eloc NE) && isOpenToStepGTree(b,eloc NE)) {return true;}
		}
	}
	if(CN1(eloc) && ISP(eloc N) && eloc N != ploc && b.isThawed(eloc N))
	{
		if(CW1(eloc) && eloc NW != floc && ISE(eloc NW))                              {SETGM((eloc N MW)); return true;}
		if(CE1(eloc) && eloc NE != floc && ISE(eloc NE))                              {SETGM((eloc N ME)); return true;}
		if(CN2(eloc) && eloc NN != floc && ISE(eloc NN) && b.isRabOkayN(pla,eloc N))  {SETGM((eloc N MN)); return true;}

		if(!b.isTrapSafe2(pla,eloc N))
		{
			if(CW1(eloc) && ISP(eloc NW) && b.wouldBeUF(pla,ploc,ploc,eloc NW) && isOpenToStepGTree(b,eloc NW)) {return true;}
			if(CE1(eloc) && ISP(eloc NE) && b.wouldBeUF(pla,ploc,ploc,eloc NE) && isOpenToStepGTree(b,eloc NE)) {return true;}
			if(CN2(eloc) && ISP(eloc NN) && b.wouldBeUF(pla,ploc,ploc,eloc NN) && isOpenToStepGTree(b,eloc NN)) {return true;}
		}
	}
	return false;
}

//Can we pull eloc with ploc, and then step a friendly piece inside? And without having ploc step on floc?
//Assumes that ploc is unfrozen already.
//!!NOTE: DOES NOT PERFORM RABBIT CHECKS!!. Since only used near goal line in UF3, no rabbit ever needs to step backwards.
static bool canPullStepInPE(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t floc)
{
  step_t estep = Board::STEPINDEX[eloc][ploc];
	if(CS1(ploc) && ISE(ploc S) && ploc S != floc)
	{
		if(CS1(eloc) && eloc S != ploc && ISP(eloc S) && (b.isAdjacent(ploc S, eloc S) || b.wouldBeUF(pla,eloc S,eloc S,eloc))) {SETGM((ploc MS,estep)); return true;}
		if(CW1(eloc) && eloc W != ploc && ISP(eloc W) && (                                b.wouldBeUF(pla,eloc W,eloc W,eloc))) {SETGM((ploc MS,estep)); return true;}
		if(CE1(eloc) && eloc E != ploc && ISP(eloc E) && (                                b.wouldBeUF(pla,eloc E,eloc E,eloc))) {SETGM((ploc MS,estep)); return true;}
		if(CN1(eloc) && eloc N != ploc && ISP(eloc N) && (                                b.wouldBeUF(pla,eloc N,eloc N,eloc))) {SETGM((ploc MS,estep)); return true;}
	}
	if(CW1(ploc) && ISE(ploc W) && ploc W != floc)
	{
		if(CS1(eloc) && eloc S != ploc && ISP(eloc S) && (                                b.wouldBeUF(pla,eloc S,eloc S,eloc))) {SETGM((ploc MW,estep)); return true;}
		if(CW1(eloc) && eloc W != ploc && ISP(eloc W) && (b.isAdjacent(ploc W, eloc W) || b.wouldBeUF(pla,eloc W,eloc W,eloc))) {SETGM((ploc MW,estep)); return true;}
		if(CE1(eloc) && eloc E != ploc && ISP(eloc E) && (                                b.wouldBeUF(pla,eloc E,eloc E,eloc))) {SETGM((ploc MW,estep)); return true;}
		if(CN1(eloc) && eloc N != ploc && ISP(eloc N) && (                                b.wouldBeUF(pla,eloc N,eloc N,eloc))) {SETGM((ploc MW,estep)); return true;}
	}
	if(CE1(ploc) && ISE(ploc E) && ploc E != floc)
	{
		if(CS1(eloc) && eloc S != ploc && ISP(eloc S) && (                                b.wouldBeUF(pla,eloc S,eloc S,eloc))) {SETGM((ploc ME,estep)); return true;}
		if(CW1(eloc) && eloc W != ploc && ISP(eloc W) && (                                b.wouldBeUF(pla,eloc W,eloc W,eloc))) {SETGM((ploc ME,estep)); return true;}
		if(CE1(eloc) && eloc E != ploc && ISP(eloc E) && (b.isAdjacent(ploc E, eloc E) || b.wouldBeUF(pla,eloc E,eloc E,eloc))) {SETGM((ploc ME,estep)); return true;}
		if(CN1(eloc) && eloc N != ploc && ISP(eloc N) && (                                b.wouldBeUF(pla,eloc N,eloc N,eloc))) {SETGM((ploc ME,estep)); return true;}
	}
	if(CN1(ploc) && ISE(ploc N) && ploc N != floc)
	{
		if(CS1(eloc) && eloc S != ploc && ISP(eloc S) && (                                b.wouldBeUF(pla,eloc S,eloc S,eloc))) {SETGM((ploc MN,estep)); return true;}
		if(CW1(eloc) && eloc W != ploc && ISP(eloc W) && (                                b.wouldBeUF(pla,eloc W,eloc W,eloc))) {SETGM((ploc MN,estep)); return true;}
		if(CE1(eloc) && eloc E != ploc && ISP(eloc E) && (                                b.wouldBeUF(pla,eloc E,eloc E,eloc))) {SETGM((ploc MN,estep)); return true;}
		if(CN1(eloc) && eloc N != ploc && ISP(eloc N) && (b.isAdjacent(ploc N, eloc N) || b.wouldBeUF(pla,eloc N,eloc N,eloc))) {SETGM((ploc MN,estep)); return true;}
	}
	return false;
}

//Can we swap out the opponent at eloc with a player piece in 3 steps, while not covering floc?
//Floc is assumed to be 2 steps from eloc.
//Does NOT use the C version of thawed or frozen!
static bool canSwapOpp3S(Board& b, pla_t pla, loc_t eloc, loc_t floc)
{
	//Search around for an adjacent player.
	//If frozen, unfreeze it.
	//Otherwise, it must be blocked. So clear some room and push, or pull and step a piece in.
	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc))
	{
		if(b.isFrozen(eloc S)) {if(canUFPushPE(b,pla,eloc S,eloc,floc)) {return true;}}
		else
		{
			if(canUnblockPushPE(b,pla,eloc S,eloc,floc)) {return true;}
			if(canPullStepInPE(b,pla,eloc S,eloc,floc)) {return true;}
		}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc))
	{
		if(b.isFrozen(eloc W)) {if(canUFPushPE(b,pla,eloc W,eloc,floc)) {return true;}}
		else
		{
			if(canUnblockPushPE(b,pla,eloc W,eloc,floc)) {return true;}
			if(canPullStepInPE(b,pla,eloc W,eloc,floc)) {return true;}
		}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc))
	{
		if(b.isFrozen(eloc E)) {if(canUFPushPE(b,pla,eloc E,eloc,floc)) {return true;}}
		else
		{
			if(canUnblockPushPE(b,pla,eloc E,eloc,floc)) {return true;}
			if(canPullStepInPE(b,pla,eloc E,eloc,floc)) {return true;}
		}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc))
	{
		if(b.isFrozen(eloc N)) {if(canUFPushPE(b,pla,eloc N,eloc,floc)) {return true;}}
		else
		{
			if(canUnblockPushPE(b,pla,eloc N,eloc,floc)) {return true;}
			if(canPullStepInPE(b,pla,eloc N,eloc,floc)) {return true;}
		}
	}

	bool suc = false;

	//Move a piece closer and push!
	if(CS1(eloc) && ISE(eloc S) && b.isTrapSafe2(pla,eloc S))
	{
		if(CS2(eloc) && ISP(eloc SS) && GT(eloc SS,eloc) && b.isThawed(eloc SS) && b.wouldBeUF(pla,eloc SS,eloc S,eloc SS)) {TSC(eloc SS,eloc S, suc = b.isOpen(eloc)) RIFGM(suc,(eloc SS MN))}
		if(CW1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.isThawed(eloc SW) && b.wouldBeUF(pla,eloc SW,eloc S,eloc SW)) {TSC(eloc SW,eloc S, suc = b.isOpen(eloc)) RIFGM(suc,(eloc SW ME))}
		if(CE1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.isThawed(eloc SE) && b.wouldBeUF(pla,eloc SE,eloc S,eloc SE)) {TSC(eloc SE,eloc S, suc = b.isOpen(eloc)) RIFGM(suc,(eloc SE MW))}
	}
	if(CW1(eloc) && ISE(eloc W) && b.isTrapSafe2(pla,eloc W))
	{
		if(CS1(eloc) && ISP(eloc SW) && GT(eloc SW,eloc) && b.isThawed(eloc SW) && b.wouldBeUF(pla,eloc SW,eloc W,eloc SW)) {TSC(eloc SW,eloc W, suc = b.isOpen(eloc)) RIFGM(suc,(eloc SW MN))}
		if(CW2(eloc) && ISP(eloc WW) && GT(eloc WW,eloc) && b.isThawed(eloc WW) && b.wouldBeUF(pla,eloc WW,eloc W,eloc WW)) {TSC(eloc WW,eloc W, suc = b.isOpen(eloc)) RIFGM(suc,(eloc WW ME))}
		if(CN1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.isThawed(eloc NW) && b.wouldBeUF(pla,eloc NW,eloc W,eloc NW)) {TSC(eloc NW,eloc W, suc = b.isOpen(eloc)) RIFGM(suc,(eloc NW MS))}
	}
	if(CE1(eloc) && ISE(eloc E) && b.isTrapSafe2(pla,eloc E))
	{
		if(CS1(eloc) && ISP(eloc SE) && GT(eloc SE,eloc) && b.isThawed(eloc SE) && b.wouldBeUF(pla,eloc SE,eloc E,eloc SE)) {TSC(eloc SE,eloc E, suc = b.isOpen(eloc)) RIFGM(suc,(eloc SE MN))}
		if(CE2(eloc) && ISP(eloc EE) && GT(eloc EE,eloc) && b.isThawed(eloc EE) && b.wouldBeUF(pla,eloc EE,eloc E,eloc EE)) {TSC(eloc EE,eloc E, suc = b.isOpen(eloc)) RIFGM(suc,(eloc EE MW))}
		if(CN1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.isThawed(eloc NE) && b.wouldBeUF(pla,eloc NE,eloc E,eloc NE)) {TSC(eloc NE,eloc E, suc = b.isOpen(eloc)) RIFGM(suc,(eloc NE MS))}
	}
	if(CN1(eloc) && ISE(eloc N) && b.isTrapSafe2(pla,eloc N))
	{
		if(CW1(eloc) && ISP(eloc NW) && GT(eloc NW,eloc) && b.isThawed(eloc NW) && b.wouldBeUF(pla,eloc NW,eloc N,eloc NW)) {TSC(eloc NW,eloc N, suc = b.isOpen(eloc)) RIFGM(suc,(eloc NW ME))}
		if(CE1(eloc) && ISP(eloc NE) && GT(eloc NE,eloc) && b.isThawed(eloc NE) && b.wouldBeUF(pla,eloc NE,eloc N,eloc NE)) {TSC(eloc NE,eloc N, suc = b.isOpen(eloc)) RIFGM(suc,(eloc NE MW))}
		if(CN2(eloc) && ISP(eloc NN) && GT(eloc NN,eloc) && b.isThawed(eloc NN) && b.wouldBeUF(pla,eloc NN,eloc N,eloc NN)) {TSC(eloc NN,eloc N, suc = b.isOpen(eloc)) RIFGM(suc,(eloc NN MS))}
	}

	return false;
}

//Can we unfreeze by swapping out the empty loc for a player?
//Does not attempt to swap out ploc, since ploc is our target for unfreezing.
//Does not fill floc, an adjacent location to ploc.
//Does NOT use the C version of thawed or frozen!
static bool canSwapEmptyUF3S(Board& b, pla_t pla, loc_t loc, loc_t ploc, loc_t floc)
{
	pla_t opp = OPP(pla);

	if(CS1(loc) && loc S != ploc)
	{
		if     (ISP(loc S) && b.isRabOkayN(pla,loc S) && canUF2(b,pla,loc S,floc,loc))            {return true;}
		else if(ISE(loc S) && b.isTrapSafe2(pla,loc S) && canMoveTo2SEndUF(b,pla,loc S,loc,floc)) {return true;}
		else if(ISO(loc S) && b.isTrapSafe2(pla,loc S) && canPushEndUF(b,pla,loc S,loc,floc))     {return true;}
		else if(ISE(loc S) && !b.isTrapSafe2(pla,loc S) && b.isGuarded(pla,loc S) && canSafeAndSwapE2S(b,pla,loc S,loc,ploc,floc)) {return true;}
	}
	if(CW1(loc) && loc W != ploc)
	{
		if     (ISP(loc W) && canUF2(b,pla,loc W,floc,loc))                                     {return true;}
		else if(ISE(loc W) && b.isTrapSafe2(pla,loc W) && canMoveTo2SEndUF(b,pla,loc W,loc,floc)) {return true;}
		else if(ISO(loc W) && b.isTrapSafe2(pla,loc W) && canPushEndUF(b,pla,loc W,loc,floc))     {return true;}
		else if(ISE(loc W) && !b.isTrapSafe2(pla,loc W) && b.isGuarded(pla,loc W) && canSafeAndSwapE2S(b,pla,loc W,loc,ploc,floc)) {return true;}
	}
	if(CE1(loc) && loc E != ploc)
	{
		if     (ISP(loc E) && canUF2(b,pla,loc E,floc,loc))                                     {return true;}
		else if(ISE(loc E) && b.isTrapSafe2(pla,loc E) && canMoveTo2SEndUF(b,pla,loc E,loc,floc)) {return true;}
		else if(ISO(loc E) && b.isTrapSafe2(pla,loc E) && canPushEndUF(b,pla,loc E,loc,floc))     {return true;}
		else if(ISE(loc E) && !b.isTrapSafe2(pla,loc E) && b.isGuarded(pla,loc E) && canSafeAndSwapE2S(b,pla,loc E,loc,ploc,floc)) {return true;}
	}
	if(CN1(loc) && loc N != ploc)
	{
		if     (ISP(loc N) && b.isRabOkayS(pla,loc N) && canUF2(b,pla,loc N,floc,loc))            {return true;}
		else if(ISE(loc N) && b.isTrapSafe2(pla,loc N) && canMoveTo2SEndUF(b,pla,loc N,loc,floc)) {return true;}
		else if(ISO(loc N) && b.isTrapSafe2(pla,loc N) && canPushEndUF(b,pla,loc N,loc,floc))     {return true;}
		else if(ISE(loc N) && !b.isTrapSafe2(pla,loc N) && b.isGuarded(pla,loc N) && canSafeAndSwapE2S(b,pla,loc N,loc,ploc,floc)) {return true;}
	}

	return false;
}

//Does NOT use the C version of thawed or frozen!
static bool canSafeAndSwapE2S(Board& b, pla_t pla, loc_t kt, loc_t loc, loc_t ploc, loc_t floc)
{
	//Same code as canUF!
	bool suc = false;
	if(CS1(kt) && ISE(kt S))
	{
		if(CS2(kt) && ISP(kt SS) && b.isThawed(kt SS) && b.isRabOkayN(pla,kt SS)) {TS(kt SS,kt S,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt SS MN))}
		if(CW1(kt) && ISP(kt SW) && b.isThawed(kt SW)                           ) {TS(kt SW,kt S,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt SW ME))}
		if(CE1(kt) && ISP(kt SE) && b.isThawed(kt SE)                           ) {TS(kt SE,kt S,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt SE MW))}
	}
	if(CW1(kt) && ISE(kt W))
	{
		if(CS1(kt) && ISP(kt SW) && b.isThawed(kt SW) && b.isRabOkayN(pla,kt SW)) {TS(kt SW,kt W,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt SW MN))}
		if(CW2(kt) && ISP(kt WW) && b.isThawed(kt WW)                           ) {TS(kt WW,kt W,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt WW ME))}
		if(CN1(kt) && ISP(kt NW) && b.isThawed(kt NW) && b.isRabOkayS(pla,kt NW)) {TS(kt NW,kt W,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt NW MS))}
	}
	if(CE1(kt) && ISE(kt E))
	{
		if(CS1(kt) && ISP(kt SE) && b.isThawed(kt SE) && b.isRabOkayN(pla,kt SE)) {TS(kt SE,kt E,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt SE MN))}
		if(CE2(kt) && ISP(kt EE) && b.isThawed(kt EE)                           ) {TS(kt EE,kt E,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt EE MW))}
		if(CN1(kt) && ISP(kt NE) && b.isThawed(kt NE) && b.isRabOkayS(pla,kt NE)) {TS(kt NE,kt E,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt NE MS))}
	}
	if(CN1(kt) && ISE(kt N))
	{
		if(CW1(kt) && ISP(kt NW) && b.isThawed(kt NW)                           ) {TS(kt NW,kt N,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt NW ME))}
		if(CE1(kt) && ISP(kt NE) && b.isThawed(kt NE)                           ) {TS(kt NE,kt N,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt NE MW))}
		if(CN2(kt) && ISP(kt NN) && b.isThawed(kt NN) && b.isRabOkayS(pla,kt NN)) {TS(kt NN,kt N,suc = canSwapE2S(b,pla,loc,ploc,floc)) RIFGM(suc,(kt NN MS))}
	}
	return false;
}

//Does NOT use the C version of thawed or frozen!
static bool canUF3(Board& b, pla_t pla, loc_t ploc, loc_t floc)
{
	pla_t opp = OPP(pla);
	//This needs to check traps...

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

		if(canPull3S(b,pla,eloc)) {return true;}
	}

	//Swap out the surrounding empty spaces or opponent pieces for player pieces.
	if(CS1(ploc) && ploc S != floc)
	{
		if     (ISE(ploc S) && canSwapEmptyUF3S(b,pla, ploc S, ploc, floc)) {return true;}
		else if(ISO(ploc S) && canSwapOpp3S(b,pla, ploc S, floc))           {return true;}
	}
	if(CW1(ploc) && ploc W != floc)
	{
		if     (ISE(ploc W) && canSwapEmptyUF3S(b,pla, ploc W, ploc, floc)) {return true;}
		else if(ISO(ploc W) && canSwapOpp3S(b,pla, ploc W, floc))           {return true;}
	}
	if(CE1(ploc) && ploc E != floc)
	{
		if     (ISE(ploc E) && canSwapEmptyUF3S(b,pla, ploc E, ploc, floc)) {return true;}
		else if(ISO(ploc E) && canSwapOpp3S(b,pla, ploc E, floc))           {return true;}
	}
	if(CN1(ploc) && ploc N != floc)
	{
		if     (ISE(ploc N) && canSwapEmptyUF3S(b,pla, ploc N, ploc, floc)) {return true;}
		else if(ISO(ploc N) && canSwapOpp3S(b,pla, ploc N, floc))           {return true;}
	}

	return false;
}

static bool canPPPEAndGoal(Board& b, pla_t pla, loc_t ploc, loc_t eloc, loc_t rloc, bool (*gfunc)(Board&,pla_t,loc_t))
{
	bool suc = false;

	//Push
  step_t stp = Board::STEPINDEX[ploc][eloc];
	if(CS1(eloc) && ISE(eloc S)) {TPC(ploc,eloc,eloc S, suc = (*gfunc)(b,pla,rloc)) RIFGM(suc,(eloc MS,stp))}
	if(CW1(eloc) && ISE(eloc W)) {TPC(ploc,eloc,eloc W, suc = (*gfunc)(b,pla,rloc)) RIFGM(suc,(eloc MW,stp))}
	if(CE1(eloc) && ISE(eloc E)) {TPC(ploc,eloc,eloc E, suc = (*gfunc)(b,pla,rloc)) RIFGM(suc,(eloc ME,stp))}
	if(CN1(eloc) && ISE(eloc N)) {TPC(ploc,eloc,eloc N, suc = (*gfunc)(b,pla,rloc)) RIFGM(suc,(eloc MN,stp))}

	//Pull
  step_t stp2 = Board::STEPINDEX[eloc][ploc];
	if(CS1(ploc) && ISE(ploc S)) {TPC(eloc,ploc,ploc S, suc = (*gfunc)(b,pla,rloc)) RIFGM(suc,(ploc MS,stp2))}
	if(CW1(ploc) && ISE(ploc W)) {TPC(eloc,ploc,ploc W, suc = (*gfunc)(b,pla,rloc)) RIFGM(suc,(ploc MW,stp2))}
	if(CE1(ploc) && ISE(ploc E)) {TPC(eloc,ploc,ploc E, suc = (*gfunc)(b,pla,rloc)) RIFGM(suc,(ploc ME,stp2))}
	if(CN1(ploc) && ISE(ploc N)) {TPC(eloc,ploc,ploc N, suc = (*gfunc)(b,pla,rloc)) RIFGM(suc,(ploc MN,stp2))}

	return false;
}

static bool canPPCapAndGoal(Board& b, pla_t pla, loc_t eloc, loc_t kt, loc_t rloc, bool (*gfunc)(Board&,pla_t,loc_t))
{
	bool suc = false;
	pla_t owner = b.owners[kt];
	piece_t piece = b.pieces[kt];
	b.owners[kt] = NPLA;
	b.pieces[kt] = EMP;

	if(CS1(eloc) && ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S))
	{
		if(CW1(eloc) && ISE(eloc W) && eloc W != kt) {TP(eloc S,eloc,eloc W, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MW,eloc S MN)); return true;}}
		if(CE1(eloc) && ISE(eloc E) && eloc E != kt) {TP(eloc S,eloc,eloc E, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc ME,eloc S MN)); return true;}}
		if(CN1(eloc) && ISE(eloc N) && eloc N != kt) {TP(eloc S,eloc,eloc N, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MN,eloc S MN)); return true;}}

		if(CS2(eloc) && ISE(eloc SS) && eloc SS != kt) {TP(eloc,eloc S,eloc SS, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc S MS,eloc MS)); return true;}}
		if(CW1(eloc) && ISE(eloc SW) && eloc SW != kt) {TP(eloc,eloc S,eloc SW, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc S MW,eloc MS)); return true;}}
		if(CE1(eloc) && ISE(eloc SE) && eloc SE != kt) {TP(eloc,eloc S,eloc SE, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc S ME,eloc MS)); return true;}}
	}
	if(CW1(eloc) && ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W))
	{
		if(CS1(eloc) && ISE(eloc S) && eloc S != kt) {TP(eloc W,eloc,eloc S, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MS,eloc W ME)); return true;}}
		if(CE1(eloc) && ISE(eloc E) && eloc E != kt) {TP(eloc W,eloc,eloc E, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc ME,eloc W ME)); return true;}}
		if(CN1(eloc) && ISE(eloc N) && eloc N != kt) {TP(eloc W,eloc,eloc N, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MN,eloc W ME)); return true;}}

		if(CS1(eloc) && ISE(eloc SW) && eloc SW != kt) {TP(eloc,eloc W,eloc SW, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc W MS,eloc MW)); return true;}}
		if(CW2(eloc) && ISE(eloc WW) && eloc WW != kt) {TP(eloc,eloc W,eloc WW, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc W MW,eloc MW)); return true;}}
		if(CN1(eloc) && ISE(eloc NW) && eloc NW != kt) {TP(eloc,eloc W,eloc NW, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc W MN,eloc MW)); return true;}}
	}
	if(CE1(eloc) && ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E))
	{
		if(CS1(eloc) && ISE(eloc S) && eloc S != kt) {TP(eloc E,eloc,eloc S, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MS,eloc E MW)); return true;}}
		if(CW1(eloc) && ISE(eloc W) && eloc W != kt) {TP(eloc E,eloc,eloc W, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MW,eloc E MW)); return true;}}
		if(CN1(eloc) && ISE(eloc N) && eloc N != kt) {TP(eloc E,eloc,eloc N, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MN,eloc E MW)); return true;}}

		if(CS1(eloc) && ISE(eloc SE) && eloc SE != kt) {TP(eloc,eloc E,eloc SE, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc E MS,eloc ME)); return true;}}
		if(CE2(eloc) && ISE(eloc EE) && eloc EE != kt) {TP(eloc,eloc E,eloc EE, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc E ME,eloc ME)); return true;}}
		if(CN1(eloc) && ISE(eloc NE) && eloc NE != kt) {TP(eloc,eloc E,eloc NE, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc E MN,eloc ME)); return true;}}
	}
	if(CN1(eloc) && ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N))
	{
		if(CS1(eloc) && ISE(eloc S) && eloc S != kt) {TP(eloc N,eloc,eloc S, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MS,eloc N MS)); return true;}}
		if(CW1(eloc) && ISE(eloc W) && eloc W != kt) {TP(eloc N,eloc,eloc W, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc MW,eloc N MS)); return true;}}
		if(CE1(eloc) && ISE(eloc E) && eloc E != kt) {TP(eloc N,eloc,eloc E, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc ME,eloc N MS)); return true;}}

		if(CW1(eloc) && ISE(eloc NW) && eloc NW != kt) {TP(eloc,eloc N,eloc NW, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc N MW,eloc MN)); return true;}}
		if(CE1(eloc) && ISE(eloc NE) && eloc NE != kt) {TP(eloc,eloc N,eloc NE, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc N ME,eloc MN)); return true;}}
		if(CN2(eloc) && ISE(eloc NN) && eloc NN != kt) {TP(eloc,eloc N,eloc NN, suc = (*gfunc)(b,pla,rloc)) if(suc){b.owners[kt] = owner; b.pieces[kt] = piece; SETGM((eloc N MN,eloc MN)); return true;}}
	}

	b.owners[kt] = owner;
	b.pieces[kt] = piece;

	return false;
}

//TODO optimize?
static bool canRemoveDef3CGTree(Board& b, pla_t pla, loc_t eloc, loc_t kt)
{
  int num = 0;
  move_t mv[24];
  int hm[24];

  //Frozen but can pushpull. Remove the defender.   //If eloc +/- X is kt, then it will not execute because no player on that trap.
  if(ISP(eloc S) && GT(eloc S,eloc) && b.isFrozenC(eloc S)) {num = BoardTrees::genUFPPPEShared(b,pla,eloc S,eloc,mv,hm,0); if(num > 0) {b.goalTreeMove = mv[0]; return true;}}
  if(ISP(eloc W) && GT(eloc W,eloc) && b.isFrozenC(eloc W)) {num = BoardTrees::genUFPPPEShared(b,pla,eloc W,eloc,mv,hm,0); if(num > 0) {b.goalTreeMove = mv[0]; return true;}}
  if(ISP(eloc E) && GT(eloc E,eloc) && b.isFrozenC(eloc E)) {num = BoardTrees::genUFPPPEShared(b,pla,eloc E,eloc,mv,hm,0); if(num > 0) {b.goalTreeMove = mv[0]; return true;}}
  if(ISP(eloc N) && GT(eloc N,eloc) && b.isFrozenC(eloc N)) {num = BoardTrees::genUFPPPEShared(b,pla,eloc N,eloc,mv,hm,0); if(num > 0) {b.goalTreeMove = mv[0]; return true;}}

  //Unfrozen, could pushpull, but stuff is in the way.
  //Step out of the way and push the defender
  if(ISP(eloc S) && GT(eloc S,eloc) && b.isThawedC(eloc S)) {num = BoardTrees::genBlockedPPShared(b,pla,eloc S,eloc,mv,hm,0); if(num > 0) {b.goalTreeMove = mv[0]; return true;}}
  if(ISP(eloc W) && GT(eloc W,eloc) && b.isThawedC(eloc W)) {num = BoardTrees::genBlockedPPShared(b,pla,eloc W,eloc,mv,hm,0); if(num > 0) {b.goalTreeMove = mv[0]; return true;}}
  if(ISP(eloc E) && GT(eloc E,eloc) && b.isThawedC(eloc E)) {num = BoardTrees::genBlockedPPShared(b,pla,eloc E,eloc,mv,hm,0); if(num > 0) {b.goalTreeMove = mv[0]; return true;}}
  if(ISP(eloc N) && GT(eloc N,eloc) && b.isThawedC(eloc N)) {num = BoardTrees::genBlockedPPShared(b,pla,eloc N,eloc,mv,hm,0); if(num > 0) {b.goalTreeMove = mv[0]; return true;}}

  //A step away, not going through the trap
  num = BoardTrees::genMoveStrongAdjCentShared(b,pla,eloc,eloc,kt,mv,hm,0);
  if(num > 0) {b.goalTreeMove = mv[0]; return true;}

  return false;
}
