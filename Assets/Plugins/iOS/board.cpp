
/*
 * board.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <vector>
#include "global.h"
#include "rand.h"
#include "bitmap.h"
#include "board.h"
#include "boardmovegen.h"
#include "arimaaio.h"

using namespace std;

//BOARD IMPLEMENTATION-------------------------------------------------------------

Board::Board()
{
	player = GOLD;
	step = 0;

	for(int i = 0; i<64; i++)
	{
    owners[i] = NPLA;
		pieces[i] = EMP;
	}

  for(int i = 0; i<NUMTYPES; i++)
  {
    pieceMaps[0][i] = Bitmap();
    pieceMaps[1][i] = Bitmap();
    pieceCounts[0][i] = 0;
    pieceCounts[1][i] = 0;
  }

  for(int i = 0; i<4; i++)
  {
    trapGuardCounts[0][i] = 0;
    trapGuardCounts[1][i] = 0;
  }

  frozenMap = Bitmap();

  posStartHash = 0;
  posCurrentHash = 0;
  sitCurrentHash = posStartHash ^ HASHPLA[player] ^ HASHSTEP[step];
  turnNumber = 0;
}

void Board::setTurnNumber(int num)
{
	if(num < 0)
		Global::fatalError(string("Board::setTurnNumber: Invalid num ") + Global::intToString(num));

	turnNumber = num;
}

void Board::setPlaStep(pla_t p, char s)
{
	if(p != GOLD && p != SILV)
		Global::fatalError(string("Board::setPlaStep: Invalid player ") + Global::intToString((int)p));
	if(s < 0 || s > 3)
		Global::fatalError(string("Board::setPlaStep: Invalid step ") + Global::intToString((int)s));

	sitCurrentHash ^= HASHPLA[player];
	sitCurrentHash ^= HASHPLA[p];
	sitCurrentHash ^= HASHSTEP[step];
	sitCurrentHash ^= HASHSTEP[s];

	player = p;
	step = s;
}

void Board::setPiece(loc_t k, pla_t owner, piece_t piece)
{
	if(k >= 64)
		Global::fatalError(string("Board::setPiece: Invalid location ") + Global::intToString((int)k));
	if(owner < 0 || owner > 2 || piece < 0 || piece >= NUMTYPES || (piece == EMP && owner != NPLA) || (piece != EMP && owner == NPLA))
		Global::fatalError(string("Board::setPiece: Invalid owner,piece (") + Global::intToString((int)owner) + string(",") + Global::intToString((int)piece) + string(")"));

	if(owners[k] != NPLA)
	{
		pieceMaps[owners[k]][pieces[k]].setOff(k);
		pieceMaps[owners[k]][0].setOff(k);
		pieceCounts[owners[k]][pieces[k]]--;
		pieceCounts[owners[k]][0]--;
		frozenMap.setOff(k);
		posCurrentHash ^= HASHPIECE[owners[k]][pieces[k]][k];
		sitCurrentHash ^= HASHPIECE[owners[k]][pieces[k]][k];
    if(ADJACENTTRAP[k] != ERRORSQUARE)
      trapGuardCounts[owners[k]][TRAPINDEX[ADJACENTTRAP[k]]]--;
	}

	owners[k] = owner;
	pieces[k] = piece;

	if(owners[k] != NPLA)
	{
		pieceMaps[owners[k]][pieces[k]].setOn(k);
		pieceMaps[owners[k]][0].setOn(k);
		pieceCounts[owners[k]][pieces[k]]++;
		pieceCounts[owners[k]][0]++;
		if(isDominated(k) && !isGuarded(owners[k],k))
		{frozenMap.setOn(k);}
		posCurrentHash ^= HASHPIECE[owners[k]][pieces[k]][k];
		sitCurrentHash ^= HASHPIECE[owners[k]][pieces[k]][k];
    if(ADJACENTTRAP[k] != ERRORSQUARE)
      trapGuardCounts[owners[k]][TRAPINDEX[ADJACENTTRAP[k]]]++;
	}

	//Update surrounding freezement
	if(CS1(k)) {frozenMap.setOff(k-8);}
	if(CS1(k) && owners[k-8] != NPLA && isDominated(k-8) && !isGuarded(owners[k-8],k-8)) {frozenMap.setOn(k-8);}
	if(CW1(k)) {frozenMap.setOff(k-1);}
	if(CW1(k) && owners[k-1] != NPLA && isDominated(k-1) && !isGuarded(owners[k-1],k-1)) {frozenMap.setOn(k-1);}
	if(CE1(k)) {frozenMap.setOff(k+1);}
	if(CE1(k) && owners[k+1] != NPLA && isDominated(k+1) && !isGuarded(owners[k+1],k+1)) {frozenMap.setOn(k+1);}
	if(CN1(k)) {frozenMap.setOff(k+8);}
	if(CN1(k) && owners[k+8] != NPLA && isDominated(k+8) && !isGuarded(owners[k+8],k+8)) {frozenMap.setOn(k+8);}
}

void Board::refreshStartHash()
{
	posStartHash = posCurrentHash;
}

bool Board::isStepLegal(step_t s) const
{
	if(s == ERRORSTEP)
	  return false;

	if(s == PASSSTEP || s == QPASSSTEP)
	  return true;

	loc_t k0 = K0INDEX[s];
	loc_t k1 = K1INDEX[s];

	if(k1 == ERRORSQUARE ||
	   owners[k0] == NPLA ||
	   owners[k1] != NPLA)
	  return false;

	if(owners[k0] == player)
	{
		if(isFrozen(k0) || (pieces[k0] == RAB && !RABBITVALID[player][s]))
		  return false;
	}

	return true;
}

void Board::makeStep(step_t s)
{
	makeStepRaw(s);
	if(s != PASSSTEP && s != QPASSSTEP)
		recalculateFreezeMap();
}


void Board::makeStepRaw(step_t s)
{
	//QPass step should do nothing
	if(s == QPASSSTEP)
		return;

  //If this is the first step of a turn, refresh the start hash
  if(step == 0)
    posStartHash = posCurrentHash;

  //If the player to move is changing, update everything accordingly to flip the turn
  if(s == PASSSTEP || step == 3)
  {
    sitCurrentHash ^= HASHSTEP[step] ^ HASHSTEP[0] ^ HASHPLA[0] ^ HASHPLA[1];
    player = OPP(player);
    step = 0;
    turnNumber++;
  }
  //Otherwise, just increment the step.
  else
  {
    sitCurrentHash ^= HASHSTEP[step] ^ HASHSTEP[step+1];
    step++;
  }

  //If this was a pass, we are done
  if(s == PASSSTEP)
    return;

	//Extract locations--------------------
	loc_t k0 = K0INDEX[s];
  loc_t k1 = K1INDEX[s];

  //Update Arrays------------------------
  pla_t pla = owners[k0];
  piece_t piece = pieces[k0];
  owners[k1] = pla;
  pieces[k1] = piece;
  owners[k0] = NPLA;
  pieces[k0] = EMP;

  hash_t hashDiff = HASHPIECE[pla][piece][k0] ^ HASHPIECE[pla][piece][k1];

  //Piece Bitmaps-------------------------
  pieceMaps[pla][piece].setOff(k0);
  pieceMaps[pla][piece].setOn(k1);
  pieceMaps[pla][0].setOff(k0);
  pieceMaps[pla][0].setOn(k1);

  //Trap Guarding-------------------------
  if(ADJACENTTRAP[k0] != ERRORSQUARE)
    trapGuardCounts[pla][TRAPINDEX[ADJACENTTRAP[k0]]]--;
  if(ADJACENTTRAP[k1] != ERRORSQUARE)
    trapGuardCounts[pla][TRAPINDEX[ADJACENTTRAP[k1]]]++;

  //Captures-----------------------------
  loc_t caploc = ADJACENTTRAP[k0];
  if(caploc != ERRORSQUARE)
    checkCaps(caploc, hashDiff);

  //Update hash-----------------------
  posCurrentHash ^= hashDiff;
  sitCurrentHash ^= hashDiff;

}

void Board::recalculateFreezeMap()
{
  //Recalculate Whole Board Freezing---------------
	pla_t pla = GOLD;
  pla_t opp = SILV;
  frozenMap = Bitmap();

  Bitmap plaDomMap = Bitmap();
  Bitmap oppPower  = pieceMaps[opp][ELE]; plaDomMap |= Bitmap::adj(oppPower) & pieceMaps[pla][CAM];
         oppPower |= pieceMaps[opp][CAM]; plaDomMap |= Bitmap::adj(oppPower) & pieceMaps[pla][HOR];
         oppPower |= pieceMaps[opp][HOR]; plaDomMap |= Bitmap::adj(oppPower) & pieceMaps[pla][DOG];
         oppPower |= pieceMaps[opp][DOG]; plaDomMap |= Bitmap::adj(oppPower) & pieceMaps[pla][CAT];
         oppPower |= pieceMaps[opp][CAT]; plaDomMap |= Bitmap::adj(oppPower) & pieceMaps[pla][RAB];

  frozenMap |= plaDomMap & ~Bitmap::adj(pieceMaps[pla][0]);

  Bitmap oppDomMap = Bitmap();
  Bitmap plaPower  = pieceMaps[pla][ELE]; oppDomMap |= Bitmap::adj(plaPower) & pieceMaps[opp][CAM];
         plaPower |= pieceMaps[pla][CAM]; oppDomMap |= Bitmap::adj(plaPower) & pieceMaps[opp][HOR];
         plaPower |= pieceMaps[pla][HOR]; oppDomMap |= Bitmap::adj(plaPower) & pieceMaps[opp][DOG];
         plaPower |= pieceMaps[pla][DOG]; oppDomMap |= Bitmap::adj(plaPower) & pieceMaps[opp][CAT];
         plaPower |= pieceMaps[pla][CAT]; oppDomMap |= Bitmap::adj(plaPower) & pieceMaps[opp][RAB];

  frozenMap |= oppDomMap & ~Bitmap::adj(pieceMaps[opp][0]);
}

bool Board::makeStepLegal(step_t s)
{
  if(!isStepLegal(s))
    return false;

  makeStep(s);
  return true;
}

bool Board::makeMovesLegal(vector<move_t> moves, int start, int end)
{
	for(int i = start; i < end && i < (int)moves.size(); i++)
	{
		bool suc = makeMoveLegal(moves[i]);
		if(!suc)
			return false;
	}
	return true;
}

void Board::makeMove(move_t m)
{
	bool recalcFreeze = false;
	for(int i = 0; i<4; i++)
	{
	  step_t s = Board::getStep(m,i);
		if(s == ERRORSTEP || s == QPASSSTEP){break;}
		makeStepRaw(s);
		if(s == PASSSTEP){break;}
		recalcFreeze = true;
	}

	if(recalcFreeze)
		recalculateFreezeMap();
}

bool Board::makeMoveLegal(move_t m)
{
	if(m == ERRORMOVE)
		return false;

	loc_t pushK = ERRORSQUARE;
	loc_t pullK = ERRORSQUARE;
	piece_t pushPower = 0;
	piece_t pullPower = 0;
	pla_t pla = player;
	pla_t opp = OPP(pla);

	for(int i = 0; i<4; i++)
	{
    step_t s = Board::getStep(m,i);
		if(s == ERRORSTEP || s == QPASSSTEP)
		{
			if(pushK != ERRORSQUARE)
			  return false;
			return true;
		}
		else if(s == PASSSTEP)
		{
			if(pushK != ERRORSQUARE)
			  return false;
		}
		else
		{
			if(!isStepLegal(s))
				return false;
			loc_t k0 = K0INDEX[s];
			loc_t k1 = K1INDEX[s];
			if(owners[k0] == opp)
			{
				if(pushK != ERRORSQUARE)
					return false;

				if(pullK == k1 && pullPower > pieces[k0])
				{
					pullK = ERRORSQUARE;
					pullPower = 0;
				}
				else
				{
					pushK = k0;
					pushPower = pieces[k0];
				}
			}
			else
			{
				if(pushK != ERRORSQUARE)
				{
					if(k1 != pushK || pieces[k0] <= pushPower)
						return false;
					pushK = ERRORSQUARE;
					pushPower = 0;
				}
				else
				{
					pullK = k0;
					pullPower = pieces[k0];
				}
			}
		}

		makeStep(s);

		if(s == PASSSTEP)
		  return true;
	}

  if(pushK != ERRORSQUARE)
     return false;

	return true;
}

bool Board::makeMoveLegalUpTo(move_t m, int numSteps)
{
	if(m == ERRORMOVE)
		return false;

	loc_t pushK = ERRORSQUARE;
	loc_t pullK = ERRORSQUARE;
	piece_t pushPower = 0;
	piece_t pullPower = 0;
	pla_t pla = player;
	pla_t opp = OPP(pla);

	for(int i = 0; i<4; i++)
	{
    step_t s = Board::getStep(m,i);
		if(s == ERRORSTEP || s == QPASSSTEP)
		{
			if(pushK != ERRORSQUARE)
			  return false;
			return true;
		}
		else if(s == PASSSTEP)
		{
			if(pushK != ERRORSQUARE)
			  return false;
			if(i == numSteps)
				return true;
		}
		else
		{
			if(!isStepLegal(s))
				return false;
			loc_t k0 = K0INDEX[s];
			loc_t k1 = K1INDEX[s];
			if(owners[k0] == opp)
			{
				if(pushK != ERRORSQUARE)
					return false;

				if(pullK == k1 && pullPower > pieces[k0])
				{
					pullK = ERRORSQUARE;
					pullPower = 0;
				}
				else
				{
					if(i == numSteps)
						return true;

					pushK = k0;
					pushPower = pieces[k0];
				}
			}
			else
			{
				if(pushK != ERRORSQUARE)
				{
					if(k1 != pushK || pieces[k0] <= pushPower)
						return false;
					pushK = ERRORSQUARE;
					pushPower = 0;
				}
				else
				{
					if(i == numSteps)
						return true;

					pullK = k0;
					pullPower = pieces[k0];
				}
			}
		}

		makeStep(s);

		if(s == PASSSTEP)
		  return true;
	}

  if(pushK != ERRORSQUARE)
     return false;

	return true;
}

bool Board::checkCaps(loc_t k, hash_t &hashDiff)
{
	pla_t owner = owners[k];
  if(ISTRAP[k] && owner != NPLA && trapGuardCounts[owner][TRAPINDEX[k]] == 0)
	{
		hashDiff ^= HASHPIECE[owner][pieces[k]][k];
		pieceMaps[owner][pieces[k]].setOff(k);
		pieceMaps[owner][0].setOff(k);
		pieceCounts[owner][pieces[k]]--;
		pieceCounts[owner][0]--;
		owners[k] = NPLA;
		pieces[k] = EMP;
		return true;
	}
	return false;
}

pla_t Board::getWinner() const
{
	pla_t pla = player;
	pla_t opp = OPP(pla);

	if(step == 0)
	{
		if(isGoal(opp)) return opp;
		if(isGoal(pla)) return pla;
		if(isRabbitless(pla)) return opp;
		if(isRabbitless(opp)) return pla;
		if(noMoves(pla)) return opp;
	}
	else
	{
		if(isGoal(pla)) return pla;
		if(isRabbitless(opp)) return pla;
		//Win if opp has no moves, pos different, opp no goal, we have rabbits
		if(noMoves(opp) && posCurrentHash != posStartHash && !isGoal(opp) && !isRabbitless(pla)) return pla;
	}
	return NPLA;
}

bool Board::noMoves(pla_t pla) const
{
	return BoardMoveGen::noMoves(*this,pla);
}

int Board::numStronger(pla_t pla, piece_t piece) const
{
	pla_t opp = OPP(pla);
	int count = 0;
	for(piece_t i = NUMTYPES-1; i > piece; i--)
	{
		count += pieceCounts[opp][i];
	}
	return count;
}

int Board::numEqual(pla_t pla, piece_t piece) const
{
	return pieceCounts[OPP(pla)][piece];
}

int Board::numWeaker(pla_t pla, piece_t piece) const
{
  pla_t opp = OPP(pla);
	int count = 0;
	for(piece_t i = 1; i < piece; i++)
	{
		count += pieceCounts[opp][i];
	}
	return count;
}

loc_t Board::findElephant(pla_t pla) const
{
  Bitmap map = pieceMaps[pla][ELE];
  if(map.isEmpty())
    return ERRORSQUARE;
  return map.nextBit();
}

void Board::initializeStronger(int pStronger[2][NUMTYPES]) const
{
  pStronger[SILV][ELE] = 0;
  pStronger[GOLD][ELE] = 0;
  pStronger[SILV][CAM] = pieceCounts[GOLD][ELE];
  pStronger[GOLD][CAM] = pieceCounts[SILV][ELE];
  pStronger[SILV][HOR] = pieceCounts[GOLD][CAM] + pStronger[SILV][CAM];
  pStronger[GOLD][HOR] = pieceCounts[SILV][CAM] + pStronger[GOLD][CAM];
  pStronger[SILV][DOG] = pieceCounts[GOLD][HOR] + pStronger[SILV][HOR];
  pStronger[GOLD][DOG] = pieceCounts[SILV][HOR] + pStronger[GOLD][HOR];
  pStronger[SILV][CAT] = pieceCounts[GOLD][DOG] + pStronger[SILV][DOG];
  pStronger[GOLD][CAT] = pieceCounts[SILV][DOG] + pStronger[GOLD][DOG];
  pStronger[SILV][RAB] = pieceCounts[GOLD][CAT] + pStronger[SILV][CAT];
  pStronger[GOLD][RAB] = pieceCounts[SILV][CAT] + pStronger[GOLD][CAT];
  pStronger[SILV][EMP] = pieceCounts[GOLD][RAB] + pStronger[SILV][RAB];
  pStronger[GOLD][EMP] = pieceCounts[SILV][RAB] + pStronger[GOLD][RAB];
}

void Board::initializeStrongerMaps(Bitmap pStrongerMaps[2][NUMTYPES]) const
{
	  pStrongerMaps[SILV][ELE] = Bitmap();
	  pStrongerMaps[GOLD][ELE] = Bitmap();
	  pStrongerMaps[SILV][CAM] = pieceMaps[GOLD][ELE];
	  pStrongerMaps[GOLD][CAM] = pieceMaps[SILV][ELE];
	  pStrongerMaps[SILV][HOR] = pieceMaps[GOLD][CAM] | pStrongerMaps[SILV][CAM];
	  pStrongerMaps[GOLD][HOR] = pieceMaps[SILV][CAM] | pStrongerMaps[GOLD][CAM];
	  pStrongerMaps[SILV][DOG] = pieceMaps[GOLD][HOR] | pStrongerMaps[SILV][HOR];
	  pStrongerMaps[GOLD][DOG] = pieceMaps[SILV][HOR] | pStrongerMaps[GOLD][HOR];
	  pStrongerMaps[SILV][CAT] = pieceMaps[GOLD][DOG] | pStrongerMaps[SILV][DOG];
	  pStrongerMaps[GOLD][CAT] = pieceMaps[SILV][DOG] | pStrongerMaps[GOLD][DOG];
	  pStrongerMaps[SILV][RAB] = pieceMaps[GOLD][CAT] | pStrongerMaps[SILV][CAT];
	  pStrongerMaps[GOLD][RAB] = pieceMaps[SILV][CAT] | pStrongerMaps[GOLD][CAT];
	  pStrongerMaps[SILV][EMP] = pieceMaps[GOLD][RAB] | pStrongerMaps[SILV][RAB];
	  pStrongerMaps[GOLD][EMP] = pieceMaps[SILV][RAB] | pStrongerMaps[GOLD][RAB];
}

loc_t Board::nearestDominator(pla_t pla, piece_t piece, loc_t loc, int maxRad) const
{
	Bitmap map;
	for(piece_t p = ELE; p > piece; p--)
		map |= pieceMaps[OPP(pla)][p];

	if(maxRad > 15)
		maxRad = 15;
	map &= Board::DISK[maxRad][loc];

	if(map.isEmpty())
		return ERRORSQUARE;

	for(int i = 1; i <= maxRad; i++)
	{
		Bitmap map2 = map & Board::RADIUS[i][loc];
		if(map2.hasBits())
			return map2.nextBit();
	}
	return ERRORSQUARE;
}

//---------------------------------------------------------------------------------

int Board::getChanges(move_t move, loc_t* src, loc_t* dest) const
{
  int8_t newTrapGuardCounts[2][4];
  return getChanges(move,src,dest,newTrapGuardCounts);
}

int Board::getChanges(move_t move, loc_t* src, loc_t* dest, int8_t newTrapGuardCounts[2][4]) const
{
  //Copy over trap guarding counts so we can detect captures
	for(int i = 0; i<4; i++)
	{
		newTrapGuardCounts[0][i] = trapGuardCounts[0][i];
		newTrapGuardCounts[1][i] = trapGuardCounts[1][i];
	}

	//Iterate through the move and examine each step
  int num = 0;
  for(int i = 0; i<4; i++)
  {
    step_t s = getStep(move,i);
    if(s == ERRORSTEP || s == PASSSTEP || s == QPASSSTEP)
      break;

    loc_t k0 = Board::K0INDEX[s];
    loc_t k1 = Board::K1INDEX[s];

    //Check if it was a piece already moved
    int j;
    for(j = 0; j<num; j++)
    {
      //And if so, update it to its new location
      if(dest[j] == k0)
      {dest[j] = k1; break;}
    }
    //Otherwise
    if(j == num)
    {
      //Add a new entry
      src[j] = k0;
      dest[j] = k1;
      num++;
    }

    //Update trap guards and check for captures
    pla_t pla = owners[src[j]];
    if(ADJACENTTRAP[k0] != ERRORSQUARE)
    {
      loc_t kt = ADJACENTTRAP[k0];
      newTrapGuardCounts[pla][TRAPINDEX[kt]]--;

      //0 defenders! Check for captures
      if(newTrapGuardCounts[pla][TRAPINDEX[kt]] == 0)
      {
        //Iterate over pieces to see if they are on the trap
        int m;
        bool pieceOnTrapMovedOff = false; //Did a piece begin this turn on the trap and move off of it?
        for(m = 0; m<num; m++)
        {
          //Is this piece currently on the trap and it is of the player whose piece stepped? If so, it dies!
          if(dest[m] == kt && owners[src[m]] == pla)
          {dest[m] = ERRORSQUARE; break;}
          //If this piece began it's turn on the trap and moved off, mark so:
          if(src[m] == kt)
          {pieceOnTrapMovedOff = true;}
        }
        //No piece captured yet? Check if there is a piece beginning on the trap itself:
        if(m == num && owners[kt] == pla && !pieceOnTrapMovedOff)
        {
          src[num] = kt;
          dest[num] = ERRORSQUARE;
          num++;
        }
      }
    }
    //Update trap guards at destination
    if(ADJACENTTRAP[k1] != ERRORSQUARE)
      newTrapGuardCounts[pla][TRAPINDEX[ADJACENTTRAP[k1]]]++;
  }

  //Remove all null changes - where src and dest are the same
  int newNum = 0;
  for(int i = 0; i<num; i++)
  {
    if(src[i] != dest[i])
    {
      src[newNum] = src[i];
      dest[newNum] = dest[i];
      newNum++;
    }
  }
  return newNum;
}

//Get the direction for the best approach to the dest. If equal, returns an arbitrary direction
int Board::getApproachDir(loc_t src, loc_t dest)
{
  int dx = (dest&7)-(src&7);
  int dy = (dest>>3)-(src>>3);
  int u = dx+dy;
  int v = dx-dy;
  if(u < 0)
  	return v < 0 ? 1 : 0;
  else if(u > 0)
  	return v > 0 ? 2 : 3;
  else
  	return v > 0 ? 0 : 3;
}

//Get the direction for best retreat from dest. If equal, sets nothing. If only one retreat dir, sets nothing for dir2
void Board::getRetreatDirs(loc_t src, loc_t dest, int& dir1, int& dir2)
{
  int dx = (dest&7)-(src&7);
  int dy = (dest>>3)-(src>>3);
  if(dx == 0)
  {
  	if(dy == 0)
  		return;
  	dir1 = (dy > 0) ? 0 : 3;
  }
  else if(dy == 0)
  {
  	dir1 = (dx > 0) ? 1 : 2;
  }
  else
  {
  	dir1 = (dy > 0) ? 0 : 3;
  	dir2 = (dx > 0) ? 1 : 2;
  }
}



//OUTPUT---------------------------------------------------------------------------

ostream& operator<<(ostream& out, const Board& b)
{
	out << ArimaaIO::writeBoard(b);
	return out;
}


//TESTING------------------------------------------------------------------------

bool Board::testConsistency(ostream& out) const
{
  hash_t hashVal = 0;

	for(int i = 0; i<64; i++)
	{
		//Piece array consistency
		if(owners[i] == NPLA && pieces[i] != EMP)
		{out << "Inconsistent board\n" << *this << endl; return false;}
		if(owners[i] != NPLA && pieces[i] == EMP)
		{out << "Inconsistent board\n" << *this << endl; return false;}

		//Bitmap consistency
		for(int j = 0; j<2; j++)
		{
			for(int k = 1; k<NUMTYPES; k++)
			{
				if((owners[i] == j && pieces[i] == k) != pieceMaps[j][k].isOne(i))
				{out << "Inconsistent board\n" << "Piecemap disagrees with array" << endl; return false;}
				if((owners[i] == j) != pieceMaps[j][0].isOne(i))
				{out << "Inconsistent board\n" << "Allmap disagrees with array" << endl; return false;}
				if(pieceMaps[j][k].countBits() != pieceCounts[j][k])
				{out << "Inconsistent board\n" << "Bitmap count disagrees with piececount" << endl; return false;}
			}
		}

		//Freezing
		if(owners[i] != NPLA)
		{
			if(isFrozen(i) != isFrozenC(i) || isFrozenC(i) == isThawedC(i) || isThawedC(i) != isThawed(i))
			{out << "Inconsistent board\n" << "Frozen/thawed error" << i << endl; out << *this << endl; return false;}
			if(isFrozen(i) != (!isGuarded(owners[i],i) && isDominated(i)))
			{out << "Inconsistent board\n" << "Frozen disagrees with dominated/guarded " << i << endl; out << *this << endl; return false;}
		}

		//Traps
		if(i == 18 || i == 21 || i == 42 || i == 45)
		{
			if(!ISTRAP[i] || !isTrap(i))
			{out << "Inconsistent board\n" << "ISTRAP array error" << endl; return false;}
			if(owners[i] != NPLA && !isGuarded(owners[i],i))
			{out << "Inconsistent board\n" << "Trap error" << endl; out << *this << endl; return false;}
			if(isGuarded2(GOLD,i) != isTrapSafe2(GOLD,i))
			{out << "Inconsistent board\n" << "gold isGuarded2 != isTrapSafe2 at " << i << endl; out << *this << endl; return false;}
			if(isGuarded2(SILV,i) != isTrapSafe2(SILV,i))
			{out << "Inconsistent board\n" << "silver isGuarded2 != isTrapSafe2 at " << i << endl; out << *this << endl; return false;}

			int gCount = 0;
			int sCount = 0;
			for(int dir = 0; dir<4; dir++)
			{
				if(owners[i + Board::ADJOFFSETS[dir]] == GOLD)
					gCount++;
				else if(owners[i + Board::ADJOFFSETS[dir]] == SILV)
					sCount++;
			}
			if(gCount != trapGuardCounts[GOLD][Board::TRAPINDEX[i]])
			{out << "Inconsistent board\n" << "gold trapGuardCounts != actual count at " << i << endl; out << *this << endl; return false;}
			if(sCount != trapGuardCounts[SILV][Board::TRAPINDEX[i]])
			{out << "Inconsistent board\n" << "silver trapGuardCounts != actual count at " << i << endl; out << *this << endl; return false;}
		}

		if(owners[i] != NPLA)
		{hashVal ^= HASHPIECE[owners[i]][pieces[i]][i];}
	}

	//Hash consistency
	if(posCurrentHash != hashVal)
	{out << "Inconsistent board\n" << "Hash error posCur != calcualated" << endl; out << *this << endl;; return false;}
	if((posCurrentHash ^ HASHPLA[player] ^ HASHSTEP[step]) != sitCurrentHash)
	{out << "Inconsistent board\n" << "Hash error posCur + sithashs != sitCur" << endl; out << *this << endl;; return false;}

	//Piece counts
	int pieceCountMaxes[NUMTYPES] = {16,8,2,2,2,1,1};
	for(int i = 0; i < NUMTYPES; i++)
	{
	  if(pieceCounts[0][i] > pieceCountMaxes[i] || pieceCounts[0][i] < 0)
	  {out << "Inconsistent board\n" << "Piece count > max or < 0" << endl; out << *this << endl; return false;}
    if(pieceCounts[1][i] > pieceCountMaxes[i] || pieceCounts[1][i] < 0)
    {out << "Inconsistent board\n" << "Piece count > max or < 0" << endl; out << *this << endl; return false;}
	}

	return true;
}

//STATIC DATA MEMBERS-----------------------------------------------------------------------

hash_t Board::HASHPIECE[2][NUMTYPES][64];
hash_t Board::HASHPLA[2];
hash_t Board::HASHSTEP[4];

uint8_t Board::STEPINDEX[64][64];
uint8_t Board::K0INDEX[256];
uint8_t Board::K1INDEX[256];
bool Board::RABBITVALID[2][256];
Bitmap Board::GOALMASKS[2];
bool Board::ISADJACENT[64][64];
int Board::MANHATTANDIST[64][64];
uint8_t Board::SPIRAL[64][64];
Bitmap Board::RADIUS[16][64];
Bitmap Board::DISK[16][64];
Bitmap Board::GPRUNEMASKS[2][64][5];

//INLINE TABLES----------------------------------------------------
static const uint8_t ESQ = ERRORSQUARE;

const uint8_t Board::TRAPLOCS[4] = {18,21,42,45};

const uint8_t Board::TRAPDEFLOCS[4][4] = {
{10,17,19,26},
{13,20,22,29},
{34,41,43,50},
{37,44,46,53},
};

const uint8_t Board::TRAPBACKLOCS[4] = {10,13,50,53};
const uint8_t Board::TRAPOUTSIDELOCS[4] = {17,22,41,46};
const uint8_t Board::TRAPINSIDELOCS[4] = {19,20,43,44};
const uint8_t Board::TRAPTOPLOCS[4] = {26,29,34,37};

const int Board::PLATRAPINDICES[2][2] = {{2,3},{0,1}};
const uint8_t Board::PLATRAPLOCS[2][2] = {{42,45},{18,21}};

const bool Board::ISPLATRAP[4][2] =
{{false,true},{false,true},{true,false},{true,false}};

const int Board::TRAPINDEX[64] =
{
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
ESQ,ESQ, 0 ,ESQ,ESQ, 1 ,ESQ,ESQ,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
ESQ,ESQ, 2 ,ESQ,ESQ, 3 ,ESQ,ESQ,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
};

const bool Board::ISTRAP[64] =
{
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,true ,false,false,true ,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,true ,false,false,true ,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
};

const bool Board::ISEDGE[64] =
{
true ,true ,true ,true ,true ,true ,true ,true ,
true ,false,false,false,false,false,false,true ,
true ,false,false,false,false,false,false,true ,
true ,false,false,false,false,false,false,true ,
true ,false,false,false,false,false,false,true ,
true ,false,false,false,false,false,false,true ,
true ,false,false,false,false,false,false,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
};

const bool Board::ISEDGE2[64] =
{
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,false,false,false,false,true ,true ,
true ,true ,false,false,false,false,true ,true ,
true ,true ,false,false,false,false,true ,true ,
true ,true ,false,false,false,false,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
};

const int Board::EDGEDIST[64] =
{
0,0,0,0,0,0,0,0,
0,1,1,1,1,1,1,0,
0,1,2,2,2,2,1,0,
0,1,2,3,3,2,1,0,
0,1,2,3,3,2,1,0,
0,1,2,2,2,2,1,0,
0,1,1,1,1,1,1,0,
0,0,0,0,0,0,0,0,
};

const uint8_t Board::ADJACENTTRAP[64] =
{
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
ESQ,ESQ, 18,ESQ,ESQ, 21,ESQ,ESQ,
ESQ, 18,ESQ, 18, 21,ESQ, 21,ESQ,
ESQ,ESQ, 18,ESQ,ESQ, 21,ESQ,ESQ,
ESQ,ESQ, 42,ESQ,ESQ, 45,ESQ,ESQ,
ESQ, 42,ESQ, 42, 45,ESQ, 45,ESQ,
ESQ,ESQ, 42,ESQ,ESQ, 45,ESQ,ESQ,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
};

const uint8_t Board::RAD2TRAP[64] =
{
ESQ,ESQ, 18,ESQ,ESQ, 21,ESQ,ESQ,
ESQ, 18,ESQ, 18, 21,ESQ, 21,ESQ,
 18,ESQ,ESQ, 21, 18,ESQ,ESQ, 21,
ESQ, 18, 42, 18, 21, 45, 21,ESQ,
ESQ, 42, 18, 42, 45, 21, 45,ESQ,
 42,ESQ,ESQ, 45, 42,ESQ,ESQ, 45,
ESQ, 42,ESQ, 42, 45,ESQ, 45,ESQ,
ESQ,ESQ, 42,ESQ,ESQ, 45,ESQ,ESQ,
};

const int Board::STARTY[2] = {7,0};
const int Board::GOALY[2] = {0,7};
const int Board::GOALYINC[2] = {-1,+1};
const int Board::GOALLOCINC[2] = {-8,+8};

const bool Board::ISGOAL[2][64] =
{{
true ,true ,true ,true ,true ,true ,true ,true ,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
},{
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
false,false,false,false,false,false,false,false,
true ,true ,true ,true ,true ,true ,true ,true ,
}};

const Bitmap Board::PGOALMASKS[8][2] =
{
    {Bitmap(0x00000000000000FFull), Bitmap(0xFF00000000000000ull)},
    {Bitmap(0x000000000000FFFFull), Bitmap(0xFFFF000000000000ull)},
    {Bitmap(0x0000000000FFFFFFull), Bitmap(0xFFFFFF0000000000ull)},
    {Bitmap(0x00000000FFFFFFFFull), Bitmap(0xFFFFFFFF00000000ull)},
    {Bitmap(0x000000FFFFFFFFFFull), Bitmap(0xFFFFFFFFFF000000ull)},
    {Bitmap(0x0000FFFFFFFFFFFFull), Bitmap(0xFFFFFFFFFFFF0000ull)},
    {Bitmap(0x00FFFFFFFFFFFFFFull), Bitmap(0xFFFFFFFFFFFFFF00ull)},
    {Bitmap(0xFFFFFFFFFFFFFFFFull), Bitmap(0xFFFFFFFFFFFFFFFFull)},
};

const int Board::ADJOFFSETS[4] =
{-8,-1,1,8};

const int Board::STEPOFFSETS[4] =
{0,64,128,192};

const bool Board::ADJOKAY[4][64] =
{
{
false,false,false,false,false,false,false,false,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
},
{
false,true ,true ,true ,true ,true ,true ,true ,
false,true ,true ,true ,true ,true ,true ,true ,
false,true ,true ,true ,true ,true ,true ,true ,
false,true ,true ,true ,true ,true ,true ,true ,
false,true ,true ,true ,true ,true ,true ,true ,
false,true ,true ,true ,true ,true ,true ,true ,
false,true ,true ,true ,true ,true ,true ,true ,
false,true ,true ,true ,true ,true ,true ,true ,
},
{
true ,true ,true ,true ,true ,true ,true ,false,
true ,true ,true ,true ,true ,true ,true ,false,
true ,true ,true ,true ,true ,true ,true ,false,
true ,true ,true ,true ,true ,true ,true ,false,
true ,true ,true ,true ,true ,true ,true ,false,
true ,true ,true ,true ,true ,true ,true ,false,
true ,true ,true ,true ,true ,true ,true ,false,
true ,true ,true ,true ,true ,true ,true ,false,
},
{
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
true ,true ,true ,true ,true ,true ,true ,true ,
false,false,false,false,false,false,false,false,
},
};

const int Board::GOALYDIST[2][64] =
{{
0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,
2,2,2,2,2,2,2,2,
3,3,3,3,3,3,3,3,
4,4,4,4,4,4,4,4,
5,5,5,5,5,5,5,5,
6,6,6,6,6,6,6,6,
7,7,7,7,7,7,7,7,
},{
7,7,7,7,7,7,7,7,
6,6,6,6,6,6,6,6,
5,5,5,5,5,5,5,5,
4,4,4,4,4,4,4,4,
3,3,3,3,3,3,3,3,
2,2,2,2,2,2,2,2,
1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,
}};

const int Board::ADVANCEMENT[2][64] =
{{
4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,
3,3,3,3,3,3,3,3,
2,2,2,2,2,2,2,2,
1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
},{
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,
2,2,2,2,2,2,2,2,
3,3,3,3,3,3,3,3,
4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,
}};

const int Board::ADVANCE_OFFSET[2] = {-8,8};

const uint8_t Board::CLOSEST_TRAP[64] =
{
18,18,18,18,21,21,21,21,
18,18,18,18,21,21,21,21,
18,18,18,18,21,21,21,21,
18,18,18,18,21,21,21,21,
42,42,42,42,45,45,45,45,
42,42,42,42,45,45,45,45,
42,42,42,42,45,45,45,45,
42,42,42,42,45,45,45,45,
};

const int Board::CLOSEST_TRAPINDEX[64] =
{
0,0,0,0,1,1,1,1,
0,0,0,0,1,1,1,1,
0,0,0,0,1,1,1,1,
0,0,0,0,1,1,1,1,
2,2,2,2,3,3,3,3,
2,2,2,2,3,3,3,3,
2,2,2,2,3,3,3,3,
2,2,2,2,3,3,3,3,
};

const int Board::CLOSEST_TDIST[64] =
{
4,3,2,3,3,2,3,4,
3,2,1,2,2,1,2,3,
2,1,0,1,1,0,1,2,
3,2,1,2,2,1,2,3,
3,2,1,2,2,1,2,3,
2,1,0,1,1,0,1,2,
3,2,1,2,2,1,2,3,
4,3,2,3,3,2,3,4,
};

const int Board::CENTRALITY[64] =
{
6,5,4,3,3,4,5,6,
5,4,3,2,2,3,4,5,
4,3,2,1,1,2,3,4,
3,2,1,0,0,1,2,3,
3,2,1,0,0,1,2,3,
4,3,2,1,1,2,3,4,
5,4,3,2,2,3,4,5,
6,5,4,3,3,4,5,6,
};


const uint8_t Board::SYMLOC[2][64] =
{
{
28,29,30,31,31,30,29,28,
24,25,26,27,27,26,25,24,
20,21,22,23,23,22,21,20,
16,17,18,19,19,18,17,16,
12,13,14,15,15,14,13,12,
 8, 9,10,11,11,10, 9, 8,
 4, 5, 6, 7, 7, 6, 5, 4,
 0, 1, 2, 3, 3, 2, 1, 0,
}
,
{
 0, 1, 2, 3, 3, 2, 1, 0,
 4, 5, 6, 7, 7, 6, 5, 4,
 8, 9,10,11,11,10, 9, 8,
12,13,14,15,15,14,13,12,
16,17,18,19,19,18,17,16,
20,21,22,23,23,22,21,20,
24,25,26,27,27,26,25,24,
28,29,30,31,31,30,29,28,
}
};

const uint8_t Board::PSYMLOC[2][64] =
{
{
56,57,58,59,60,61,62,63,
48,49,50,51,52,53,54,55,
40,41,42,43,44,45,46,47,
32,33,34,35,36,37,38,39,
24,25,26,27,28,29,30,31,
16,17,18,19,20,21,22,23,
 8, 9,10,11,12,13,14,15,
 0, 1, 2, 3, 4, 5, 6, 7,
}
,
{
 0, 1, 2, 3, 4, 5, 6, 7,
 8, 9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,
24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,
56,57,58,59,60,61,62,63,
}
};

const int Board::SYMTRAPINDEX[2][4] =
{{2,3,0,1},{0,1,2,3}};

const int Board::SYMDIR[2][4] =
{{1,2,2,0},{0,2,2,1}};

const int Board::PSYMDIR[2][4] =
{{3,1,2,0},{0,1,2,3}};

//INITIALIZATION OF PRECOMPUTED TABLES-----------------------------------------------

static bool isInBounds(int x, int y)
{
  return x >= 0 && x < 8 && y >= 0 && y < 8;
}

static Bitmap makeGPruneMask(pla_t pla, int loc, int gDist)
{
  int dy = pla*2-1;
  int yGoal = pla*7;
  int yDist = (pla == 0) ? loc/8 : 7-loc/8;
  int x = loc%8;
  Bitmap b;
  if(yDist == 0)
  {
    b.setOn(loc);
    return b;
  }

  for(int i = 1; i<=yDist; i++)
    b.setOn((yGoal-i*dy)*8+x);

  int reps = (gDist-yDist)*2 + 1;
  for(int i = 0; i<reps; i++)
    b |= Bitmap::adj(b);

  return b;
}

void Board::initData()
{
	Rand rand(Global::getHash("Board::initData"));

  //Initialize zobrist hash values for pieces at locs
  hash_t r = 0LL;
  for(int i = 0; i<2; i++)
  {
    for(int k = 0; k<64; k ++)
      HASHPIECE[i][0][k] = 0LL;

    for(int j = 1; j<NUMTYPES; j++)
    { for(int k = 0; k<64; k++)
      {
        r = 0LL;
        while(r == 0LL)
          r = rand.nextUInt64();
        HASHPIECE[i][j][k] = r;
      }
    }
  }

  //Initialize zobrist hash value for player
  for(int i = 0; i<2; i++)
  {
    r = 0LL;
    while(r == 0LL)
      r = rand.nextUInt64();
    HASHPLA[i] = r;
  }

  //Initialize zobrist hash value for setp
  for(int i = 0; i<4; i++)
  {
    r = 0LL;
    while(r == 0LL)
      r = rand.nextUInt64();
    HASHSTEP[i] = r;
  }

  //Initialize step index computation from src and dest locs
  for(int y0 = 0; y0 < 8; y0++)
  {
    for(int x0 = 0; x0 < 8; x0++)
    {
      for(int y1 = 0; y1 < 8; y1++)
      {
        for(int x1 = 0; x1 < 8; x1++)
        {
          int i0 = x0 + y0*8;
          int i1 = x1 + y1*8;

          if(x0 != x1 && y0 != y1)
          {STEPINDEX[i0][i1] = ERRORSTEP;}
          else
          {
            if(y1 == y0-1)      {STEPINDEX[i0][i1] = (step_t)i0;}
            else if(x1 == x0-1) {STEPINDEX[i0][i1] = (step_t)(i0+64);}
            else if(x1 == x0+1) {STEPINDEX[i0][i1] = (step_t)(i0+128);}
            else if(y1 == y0+1) {STEPINDEX[i0][i1] = (step_t)(i0+192);}
            else                {STEPINDEX[i0][i1] = ERRORSTEP;}
          }
        }
      }
    }
  }

  //Initialize src and dest computation from step indices
  for(int i = 0; i<256; i++)
  {
    int k0 = i & 0x3F;
    int dir = i/64;
    K0INDEX[i] = (loc_t)k0;
    K1INDEX[i] = 0;
    if(dir == 0) {K1INDEX[i] = (CS1(k0)) ? (loc_t)(k0-8) : ERRORSQUARE;}
    if(dir == 1) {K1INDEX[i] = (CW1(k0)) ? (loc_t)(k0-1) : ERRORSQUARE;}
    if(dir == 2) {K1INDEX[i] = (CE1(k0)) ? (loc_t)(k0+1) : ERRORSQUARE;}
    if(dir == 3) {K1INDEX[i] = (CN1(k0)) ? (loc_t)(k0+8) : ERRORSQUARE;}

    if(K1INDEX[i] == ERRORSQUARE)
      K0INDEX[i] = ERRORSQUARE;
  }

  //Initialize rabbit valid steps
  for(int i = 0; i<192; i++)   {RABBITVALID[0][i] = true;}
  for(int i = 192; i<256; i++) {RABBITVALID[0][i] = false;}
  for(int i = 0; i<64; i++)    {RABBITVALID[1][i] = false;}
  for(int i = 64; i<256; i++)  {RABBITVALID[1][i] = true;}

  //Initialize goal masks
  GOALMASKS[0] = Bitmap::BMPY0;
  GOALMASKS[1] = Bitmap::BMPY7;

  //Initialize adjacency matrix for locations
  for(int k0 = 0; k0 < 64; k0++)
  {
    for(int k1 = k0; k1 < 64; k1++)
    {
      if((CS1(k0) && k0-8 == k1) ||
         (CW1(k0) && k0-1 == k1) ||
         (CE1(k0) && k0+1 == k1) ||
         (CN1(k0) && k0+8 == k1))
      {
        ISADJACENT[k0][k1] = true;
        ISADJACENT[k1][k0] = true;
      }
      else
      {
        ISADJACENT[k0][k1] = false;
        ISADJACENT[k1][k0] = false;
      }
    }
  }

  //Initialize manhattan distance for locations
  for(int k0 = 0; k0 < 64; k0++)
  {
    for(int k1 = 0; k1 < 64; k1++)
    {
      int x0 = k0 % 8;
      int y0 = k0 / 8;
      int x1 = k1 % 8;
      int y1 = k1 / 8;
      MANHATTANDIST[k0][k1] = abs(x0-x1) + abs(y0-y1);
    }
  }

  //Initialize spiral listing of locations
  for(int k0 = 0; k0 < 64; k0++)
  {
    int x0 = k0 % 8;
    int y0 = k0 / 8;

    int index = 0;
    int diagLen = 1;
    while(index < 64)
    {
      for(int dx = 0; dx < diagLen; dx++)
      {
        int dy = diagLen-1-dx;
        if(isInBounds(x0+dx,y0+dy))                       {SPIRAL[k0][index++] = (loc_t)(x0+dx + (y0+dy)*8);}
        if(isInBounds(x0-dx,y0+dy) && dx != 0)            {SPIRAL[k0][index++] = (loc_t)(x0-dx + (y0+dy)*8);}
        if(isInBounds(x0+dx,y0-dy) && dy != 0)            {SPIRAL[k0][index++] = (loc_t)(x0+dx + (y0-dy)*8);}
        if(isInBounds(x0-dx,y0-dy) && dx != 0 && dy != 0) {SPIRAL[k0][index++] = (loc_t)(x0-dx + (y0-dy)*8);}
      }
      diagLen++;
    }
  }

  //Initialize radius and disk bitmaps
  for(int k0 = 0; k0 < 64; k0++)
  {
    Bitmap b;
    b.setOn(k0);
    RADIUS[0][k0] = b;
    DISK[0][k0] = b;

    for(int i = 1; i<16; i++)
    {
      DISK[i][k0] = DISK[i-1][k0] | Bitmap::adj(DISK[i-1][k0]);
      RADIUS[i][k0] = DISK[i][k0] & (~DISK[i-1][k0]);
    }
  }

  //Initialize goal pruning masks
  for(char pla = 0; pla <= 1; pla++)
    for(int loc = 0; loc < 64; loc++)
      for(int gDist = 0; gDist < 5; gDist++)
        GPRUNEMASKS[pla][loc][gDist] = makeGPruneMask(pla,loc,gDist);

}

