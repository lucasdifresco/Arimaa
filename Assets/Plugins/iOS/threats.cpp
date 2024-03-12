
/*
 * threats.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <cmath>
#include "global.h"
#include "bitmap.h"
#include "board.h"
#include "boardtrees.h"
#include "boardtreeconst.h"
#include "threats.h"

static int blockVal(const Board& b, pla_t pla, loc_t ploc, loc_t loc)
{
	if(ISE(loc))
	{
		if(Board::ISTRAP[loc])
		{
			int trapIndex = Board::TRAPINDEX[loc];
			if((Board::ISADJACENT[ploc][loc] && b.trapGuardCounts[pla][trapIndex] <= 1) || b.trapGuardCounts[pla][trapIndex] == 0)
					return 1 + !b.canMakeTrapSafeFor(pla, loc, ploc);
		}
		return !b.wouldBeUF(pla,ploc,loc,ploc);
	}
	else if(ISP(loc))
		return 1;
	else
	{
		if(GT(ploc,loc))
		{
			if(((Board::ISADJACENT[ploc][loc] && !b.isTrapSafe2(pla,loc)) || (!Board::ISADJACENT[ploc][loc] && !b.isTrapSafe1(pla,loc))) &&
					!b.canMakeTrapSafeFor(pla, loc, ploc))
				return 2;
			return 1;
		}
		return 2;
	}
}

const int Threats::ZEROS[64] = {
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
};

static int blockValsPath(const Board& b, pla_t pla, loc_t ploc, loc_t dest,
		int dxTotal, int dyTotal, int dxDir, int dyDir, int maxBlock, const int* bonusVals = Threats::ZEROS)
{
	if(Board::MANHATTANDIST[ploc][dest] == 1)
		return 0;

	int totalBlock = 0;
	for(int dist = 1; true; dist++)
	{
		int minBlock = maxBlock;
		int yi;
		for(int xi = min(dxTotal,dist); xi >= 0 && (yi = dist-xi) <= dyTotal; xi--)
		{
			loc_t loc = ploc+xi*dxDir+yi*dyDir*8;
			int val = blockVal(b,pla,ploc,loc) + bonusVals[loc];
			if(val < minBlock)
			{
				minBlock = val;
				if(minBlock <= 0)
				  break;
			}
		}

		totalBlock += minBlock;
		if(totalBlock >= maxBlock)
		  return maxBlock;

		int dist2 = Board::MANHATTANDIST[ploc][dest]-dist;
		if(dist2 <= dist)
		  break;
		minBlock = maxBlock;
		for(int xi = min(dxTotal,dist2); xi >= 0 && (yi = dist2-xi) <= dyTotal; xi--)
		{
			loc_t loc = ploc+xi*dxDir+yi*dyDir*8;
			int val = blockVal(b,pla,ploc,loc) + bonusVals[loc];
			if(val < minBlock)
			{
				minBlock = val;
				if(minBlock <= 0)
				  break;
			}
		}
		totalBlock += minBlock;
		if(totalBlock >= maxBlock)
		  return maxBlock;

		if(dist+1 >= dist2)
		  break;
	}

	return totalBlock;
}

//ploc to get adjacent to dest ending UF, assumes ploc not rabbit
//From attacker persepecitve
int Threats::traverseAdjUfDist(const Board& b, pla_t pla, loc_t ploc, loc_t dest, const int ufDist[64], int maxDist)
{
	int uDist = ufDist[ploc];
	int manHatM1 = Board::MANHATTANDIST[ploc][dest]-1;
	if(manHatM1 == 0)
		return uDist;

	int fastDistEst = uDist + manHatM1;
	if(fastDistEst >= maxDist)
		return fastDistEst;

	int best = maxDist;
	int dy = ((int)dest>>3)-((int)ploc>>3);
	int dx = ((int)dest&7)-((int)ploc&7);

	int bDist;
	//Duplication so that depending on direction, you test the spaces closer to you first
	if(dy == 0)
	{
		if(CW1(dest) && dx >= 0 && b.isRabOkay(pla,ploc,ploc,dest W) && (bDist = Board::MANHATTANDIST[ploc][dest W]+blockVal(b,pla,ploc,dest W)) < best) {bDist += blockValsPath(b,pla,ploc,dest W, dx==0?1: dx-1,   dy<0?-dy:dy, dx==0?-1:1,  dy<0?-1:1, maxDist-bDist>2?2:maxDist-bDist);  if(bDist < best) {best = bDist; if(best <= manHatM1) {return uDist+best;}}}
		if(CE1(dest) && dx <= 0 && b.isRabOkay(pla,ploc,ploc,dest E) && (bDist = Board::MANHATTANDIST[ploc][dest E]+blockVal(b,pla,ploc,dest E)) < best) {bDist += blockValsPath(b,pla,ploc,dest E, dx==0?1:-dx-1,   dy<0?-dy:dy, dx==0?1:-1,  dy<0?-1:1, maxDist-bDist>2?2:maxDist-bDist);  if(bDist < best) {best = bDist; if(best <= manHatM1) {return uDist+best;}}}
	}
	if(CS1(dest) && dy >= 0 && b.isRabOkay(pla,ploc,ploc,dest S) && (bDist = Board::MANHATTANDIST[ploc][dest S]+blockVal(b,pla,ploc,dest S)) < best) {bDist += blockValsPath(b,pla,ploc,dest S,   dx<0?-dx:dx, dy==0?1: dy-1,  dx<0?-1:1, dy==0?-1:1, maxDist-bDist>2?2:maxDist-bDist);  if(bDist < best) {best = bDist; if(best <= manHatM1) {return uDist+best;}}}
	if(CN1(dest) && dy <= 0 && b.isRabOkay(pla,ploc,ploc,dest N) && (bDist = Board::MANHATTANDIST[ploc][dest N]+blockVal(b,pla,ploc,dest N)) < best) {bDist += blockValsPath(b,pla,ploc,dest N,   dx<0?-dx:dx, dy==0?1:-dy-1,  dx<0?-1:1, dy==0?1:-1, maxDist-bDist>2?2:maxDist-bDist);  if(bDist < best) {best = bDist; if(best <= manHatM1) {return uDist+best;}}}
	if(dy != 0)
	{
		if(CW1(dest) && dx >= 0 && b.isRabOkay(pla,ploc,ploc,dest W) && (bDist = Board::MANHATTANDIST[ploc][dest W]+blockVal(b,pla,ploc,dest W)) < best) {bDist += blockValsPath(b,pla,ploc,dest W, dx==0?1: dx-1,   dy<0?-dy:dy, dx==0?-1:1,  dy<0?-1:1, maxDist-bDist>2?2:maxDist-bDist);  if(bDist < best) {best = bDist; if(best <= manHatM1) {return uDist+best;}}}
		if(CE1(dest) && dx <= 0 && b.isRabOkay(pla,ploc,ploc,dest E) && (bDist = Board::MANHATTANDIST[ploc][dest E]+blockVal(b,pla,ploc,dest E)) < best) {bDist += blockValsPath(b,pla,ploc,dest E, dx==0?1:-dx-1,   dy<0?-dy:dy, dx==0?1:-1,  dy<0?-1:1, maxDist-bDist>2?2:maxDist-bDist);  if(bDist < best) {best = bDist;}}
	}
	return uDist+best;
}

int Threats::traverseDist(const Board& b, loc_t ploc, loc_t dest, bool endUF,
		const int ufDist[64], int maxDist, const int* bonusVals)
{
	if(ploc == dest)
		return endUF ? ufDist[ploc] : 0;

	int dist = ufDist[ploc];
	dist += Board::MANHATTANDIST[ploc][dest];
	if(dist >= maxDist)
		return dist;
	if(!b.isRabOkay(b.owners[ploc],ploc,ploc,dest))
		return maxDist;

	int dy = ((int)dest>>3)-((int)ploc>>3);
	int dx = ((int)dest&7)-((int)ploc&7);
	int dxDir = 1;
	int dyDir = 1;
	if(dx < 0) {dxDir = -1; dx = -dx;}
	if(dy < 0) {dyDir = -1; dy = -dy;}

	int partialDist = dist+blockVal(b,b.owners[ploc],ploc,dest) + bonusVals[dest];
	if(partialDist >= maxDist)
		return partialDist;
	int pathBlock = blockValsPath(b,b.owners[ploc],ploc,dest,dx,dy,dxDir,dyDir,maxDist-partialDist,bonusVals);
	return partialDist + pathBlock;
}


int Threats::moveAdjDistUF(const Board& b, pla_t pla, loc_t loc, int maxDist,
		const int ufDist[64], const Bitmap pStrongerMaps[2][NUMTYPES], piece_t strongerThan)
{
  //TODO is it worth checking if maxDist <= 0?
	if(strongerThan == ELE)
		return maxDist;

	int bestDist = maxDist;
	//Radius 1
	if(CS1(loc) && ISP(loc S) && b.pieces[loc S] > strongerThan && b.isRabOkayN(pla,loc S)) {int dist = ufDist[loc S]; if(dist < bestDist) {bestDist = dist; if(bestDist <= 0) return bestDist;}}
	if(CW1(loc) && ISP(loc W) && b.pieces[loc W] > strongerThan)                            {int dist = ufDist[loc W]; if(dist < bestDist) {bestDist = dist; if(bestDist <= 0) return bestDist;}}
	if(CE1(loc) && ISP(loc E) && b.pieces[loc E] > strongerThan)                            {int dist = ufDist[loc E]; if(dist < bestDist) {bestDist = dist; if(bestDist <= 0) return bestDist;}}
	if(CN1(loc) && ISP(loc N) && b.pieces[loc N] > strongerThan && b.isRabOkayS(pla,loc N)) {int dist = ufDist[loc N]; if(dist < bestDist) {bestDist = dist; if(bestDist <= 0) return bestDist;}}
	if(bestDist <= 1)
		return bestDist;

	//Radius 2 or more
	for(int rad = 2; rad<=maxDist; rad++)
	{
		Bitmap map;
		if(pStrongerMaps == NULL)
			map = Board::RADIUS[rad][loc] & b.pieceMaps[pla][0];
		else
			map = Board::RADIUS[rad][loc] & pStrongerMaps[OPP(pla)][strongerThan];

		while(map.hasBits())
		{
		  loc_t ploc = map.nextBit();
		  //TODO If a square has a high UFDist for an immofrozen piece, then count that in the block value.
			int dist = Threats::traverseAdjUfDist(b,pla,ploc,loc,ufDist,bestDist);

			if(dist < bestDist)
			{
				bestDist = dist;
				if(bestDist <= rad-1)
					return bestDist;
			}
		}
		if(bestDist <= rad)
			return bestDist;
	}
	return bestDist;
}

int Threats::attackDist(const Board& b, loc_t ploc, int maxDist,
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64])
{
	return moveAdjDistUF(b,OPP(b.owners[ploc]),ploc,maxDist,ufDist,pStrongerMaps,b.pieces[ploc]);
}

//Dist for pla to occupy loc^M
//Not quite accurate - disregards freezing at the end
int Threats::occupyDist(const Board& b, pla_t pla, loc_t loc, int maxDist,
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64])
{
	if(maxDist == 0 || b.owners[loc] == pla)
		return 0;
	if(b.owners[loc] == NPLA)
		return 1+moveAdjDistUF(b,pla,loc,maxDist-1,ufDist,pStrongerMaps,EMP);
	return 2+attackDist(b,loc,maxDist-2,pStrongerMaps,ufDist);
}


//CAPTURE FINDING--------------------------------------------------------------

//static inline int min(int a, int b) {return a < b ? a : b;}

static int capInterfereCost(const Board& b, pla_t pla, loc_t eloc, loc_t loc, loc_t kt, const int ufDist[64])
{
	if(b.owners[loc] == NPLA)
		return 0;
	else if(b.owners[loc] == pla)
  {
    int cost = 0;
    if(b.pieces[loc] <= b.pieces[eloc])
      cost += 1;
    if(ufDist[loc] != 0)
      cost += 1;
    return cost;
  }
  else
  {
		if(Board::ISADJACENT[loc][kt])
			return 0;
		return 2;
  }
}

//Extra cost of sources of interference when pla is attempting to capture eloc in kt
static int capInterferePathCost(const Board& b, pla_t pla, loc_t kt, loc_t eloc, const int ufDist[64], int maxDist)
{
  int dx = (int)(eloc&7) - (int)(kt&7);
  int dy = (int)(eloc>>3) - (int)(kt>>3);
  int mdist = Board::MANHATTANDIST[kt][eloc];
  if(mdist < 2)
    return 0;

  int cost = 0;
  cost += capInterfereCost(b,pla,eloc,kt,kt,ufDist);
  if(cost >= maxDist)
  	return cost;

  if(dx == 0 || dy == 0)
  {
    if(dy < 0)       cost += capInterfereCost(b,pla,eloc,kt-8,kt,ufDist);
    else if (dx < 0) cost += capInterfereCost(b,pla,eloc,kt-1,kt,ufDist);
    else if (dx > 0) cost += capInterfereCost(b,pla,eloc,kt+1,kt,ufDist);
    else             cost += capInterfereCost(b,pla,eloc,kt+8,kt,ufDist);
  }
  else
  {
  	loc_t kty = (dy > 0) ? kt+8 : kt-8;
  	loc_t ktx = (dx > 0) ? kt+1 : kt-1;
  	int v = capInterfereCost(b,pla,eloc,kty,kt,ufDist);
  	if(v > 0)
  	{int v2 = capInterfereCost(b,pla,eloc,ktx,kt,ufDist); cost += v > v2 ? v2 : v;}
  }

  if(cost >= maxDist)
  	return cost;

  if(mdist > 2)
  {
    if(dx == 0 || dy == 0)
    {
      if(dy < 0)       cost += capInterfereCost(b,pla,eloc,eloc+8,kt,ufDist);
      else if (dx < 0) cost += capInterfereCost(b,pla,eloc,eloc+1,kt,ufDist);
      else if (dx > 0) cost += capInterfereCost(b,pla,eloc,eloc-1,kt,ufDist);
      else             cost += capInterfereCost(b,pla,eloc,eloc-8,kt,ufDist);
    }
    else
    {
    	loc_t elocy = (dy > 0) ? eloc-8 : eloc+8;
    	loc_t elocx = (dx > 0) ? eloc-1 : eloc+1;
    	int v = capInterfereCost(b,pla,eloc,elocy,kt,ufDist);
			if(v > 0)
			{int v2 = capInterfereCost(b,pla,eloc,elocx,kt,ufDist); cost += v > v2 ? v2 : v;}
    }
  }

  return cost;
}

int Threats::findCapThreats(Board& b, pla_t pla, loc_t kt, CapThreat* threats, const int ufDist[64],
		const Bitmap pStronger[2][NUMTYPES], int maxCapDist, int maxThreatLen, int& rdSteps)
{
	if(maxCapDist < 2)
		return 0;

  int numThreats = 0;
  pla_t opp = OPP(pla);

  //Count up basic statistics
  int defCount = 0; //Defender count
  int removeDefSteps = 0; //+2 for each defender, +3 for each undominated defender
  piece_t strongestPiece = 0; //Strongest defender
  loc_t strongestLoc = ERRORSQUARE; //Location of strongest defender
  loc_t defenders[4]; //Locations of defenders
  for(int i = 0; i<4; i++)
  {
    loc_t loc = kt + Board::ADJOFFSETS[i];
    if(b.owners[loc] == opp)
    {
      defenders[defCount] = loc;
      defCount++;
      removeDefSteps += 2;
      if(!b.isDominated(loc))
        removeDefSteps++;
      if(b.pieces[loc] > strongestPiece)
      {
        strongestPiece = b.pieces[loc];
        strongestLoc = loc;
      }
    }
  }

  //Store prelim value in case we return
  rdSteps = removeDefSteps;

  //No hope of capture!
  if(rdSteps > maxCapDist || strongestPiece == ELE || defCount == 4)
    return 0;

  //Instant capture!
  if(defCount == 0 && b.owners[kt] == opp)
    threats[numThreats++] = CapThreat(0,kt,kt,ERRORSQUARE);

  //Harder to cap when own piece on the trap
  if(defCount > 0 && b.owners[kt] == pla && !b.isDominating(kt))
    removeDefSteps++;
  //Harder to cap when opp piece on the trap.
  else if(defCount > 0 && removeDefSteps > 4 && b.owners[kt] == opp)
    removeDefSteps++;

  //Can't possibly capture within max moves
  if(removeDefSteps > maxCapDist)
    return numThreats;

  //Total steps counting pushing the strongest
  int removeDefStepsTotal = 16;
  if(defCount == 0)
    removeDefStepsTotal = 0;
  else if(defCount > 0)
  {
    //Expand outward over threateners
    int firstCapRad = 16;
    int stepAdjustmentRadG1 = b.isDominated(strongestLoc) ? 0 : 1;
    for(int rad = 1; rad-2+removeDefSteps <= maxCapDist && rad <= firstCapRad+2; rad++)
    {
      //Find potential capturers
    	Bitmap threateners;
    	int stepAdjustment = 0;
    	if(rad == 1)
    		threateners = pStronger[opp][strongestPiece] & Board::DISK[rad][strongestLoc];
    	else
    	{
        //Adjust steps - one less if the strongest is dominated
    		stepAdjustment = stepAdjustmentRadG1;
    		threateners = pStronger[opp][strongestPiece] & Board::RADIUS[rad][strongestLoc];
    	}

      //Iterate over the threateners!
      while(threateners.hasBits())
      {
        loc_t ploc = threateners.nextBit();

        //Init attack dist to be computed - 2 cases
        int attackSteps = 0;
        bool computeDistToStrongest = true;  //Do use dist to strongest, or just uf dist for threatener?

        //Case 0 - Always use UF dist for rad 1
        if(rad == 1)
          computeDistToStrongest = false;

        //Case 1 - Check if adjacent to some other defender
        else if(Board::MANHATTANDIST[kt][ploc] == 2)
        {
          for(int k = 0; k<defCount; k++)
            if(Board::ISADJACENT[ploc][defenders[k]])
            {computeDistToStrongest = false; break;}
        }

        //Compute attack dist!
        if(computeDistToStrongest)
          attackSteps = traverseAdjUfDist(b,pla,ploc,strongestLoc,ufDist,maxCapDist+1+stepAdjustment-removeDefSteps) - stepAdjustment;
        else
          attackSteps = ufDist[ploc];

        //Additional adjustment - don't double count the cost of a weak friendly piece on a trap in both the prelim checks and by attack dist
        if(b.owners[kt] == pla && rad == 2 && Board::ISADJACENT[ploc][kt] && !b.isDominating(kt))
          attackSteps--;

        //Found a cap!
        if(removeDefSteps + attackSteps <= maxCapDist)
        {
          //If shortest for trap, mark it
          if(removeDefSteps + attackSteps < removeDefStepsTotal)
            removeDefStepsTotal = removeDefSteps + attackSteps;

          //Mark closest rad so we know when to stop
          if(firstCapRad == 16)
            firstCapRad = rad;

          //Extra if opp on trap already
          int extra = 0;
          //Opp on trap
          if(b.owners[kt] == opp)
          {
          	DEBUGASSERT(maxThreatLen > numThreats);
            extra = 3;
            threats[numThreats++] = CapThreat(removeDefSteps+attackSteps,kt,kt,ploc);
          }
          //Defenders
          int d = removeDefSteps+attackSteps+extra;
          if(d <= maxCapDist)
          {
            for(int j = 0; j<defCount; j++)
            {
            	DEBUGASSERT(maxThreatLen > numThreats);
              threats[numThreats++] = CapThreat(d,defenders[j],kt,ploc);
            }
          }
        }
      }
    }
  }

  //Store remove def steps
  rdSteps = removeDefStepsTotal;

  //Locate captures for further pieces
  //Iterate out by radius
  for(int rad = 2; rad*2+removeDefStepsTotal <= maxCapDist; rad++)
  {
    //Compute base distance
    int basePushSteps = rad*2+removeDefStepsTotal;

    //Iterate over potential capturable pieces
    Bitmap otherPieces = b.pieceMaps[opp][0] & Board::RADIUS[rad][kt];
    while(otherPieces.hasBits())
    {
      loc_t eloc = otherPieces.nextBit();
      int totalPushSteps = basePushSteps + capInterferePathCost(b,pla,kt,eloc,ufDist,maxCapDist-basePushSteps+1);

      //Max rad to go to
      int maxThreatRad = maxCapDist + 1 - totalPushSteps > 3 ? 3 : maxCapDist + 1 - totalPushSteps;
      for(int threatRad = 1; threatRad <= maxThreatRad; threatRad++)
      {
        //Capturers
        Bitmap radThreateners = pStronger[opp][b.pieces[eloc]] & Board::RADIUS[threatRad][eloc];
        while(radThreateners.hasBits())
        {
          //Here we go!
          loc_t ploc = radThreateners.nextBit();
          int traverseDist = traverseAdjUfDist(b,pla,ploc,eloc,ufDist,maxThreatRad-1);
          int d = totalPushSteps+traverseDist;
          if(d <= maxCapDist)
          {
          	DEBUGASSERT(maxThreatLen > numThreats);
            threats[numThreats++] = CapThreat(d,eloc,kt,ploc);
          }
        }
      }
    }
  }

  //Ensure we don't raise any false alarms - find fastest cap
  if(removeDefSteps <= 4)
  {
    int fastestDist = 64;
    for(int i = 0; i<numThreats; i++)
    {
      if(threats[i].dist < fastestDist && threats[i].dist != 0)
      {
        fastestDist = threats[i].dist;
        if(fastestDist <= 3 || (fastestDist <= 4 && removeDefSteps >= 4))
          break;
      }
    }

    //If fastest cap is exactly 4, on the edge of possibility...
    if(fastestDist == 4)
    {
      //Handle instacaps by temp removal
      bool instaCap = false;
      piece_t oldPiece = 0;
      if(defCount == 0 && b.owners[kt] == opp)
      {
        instaCap = true;
        oldPiece = b.pieces[kt];
        b.owners[kt] = NPLA;
        b.pieces[kt] = EMP;
      }
      //Confirm that cap is actually possible!
      if(!BoardTrees::canCaps(b,pla,4,0,kt))
      {
        for(int i = 0; i<numThreats; i++)
          if(threats[i].dist <= 4 && threats[i].dist != 0)
            threats[i].dist = 5;
      }
      //Undo temp removal
      if(instaCap)
      {
        b.owners[kt] = opp;
        b.pieces[kt] = oldPiece;
      }
    }
  }

  return numThreats;
}


/*
void Threats::getGoalDistEst(const Board& b, pla_t pla, int goalDist[64], int max)
{
  loc_t queueBuf[64];
  loc_t queueBuf2[64];

  for(int i = 0; i<64; i++)
    goalDist[i] = max;

  int numInQueue = 0;
  loc_t* oldQueue = queueBuf;
  loc_t* newQueue = queueBuf2;
  Bitmap added;

  int goalOffset = (pla == GOLD) ? 56 : 0;
  for(int x = 0; x<8; x++)
  {
    loc_t loc = goalOffset+x;

    int extra = 0;
    if(b.owners[loc] == NPLA) extra = !b.isTrapSafe2(pla,loc) ? 1 : !b.wouldRabbitBeUFAt(pla,loc) ? 1 : 0;
    else if(b.owners[loc] == pla) extra = b.isFrozen(loc);
    else extra = b.isDominated(loc) ? 2 : 3;
    goalDist[loc] = extra;
    added.setOn(loc);
    oldQueue[numInQueue++] = loc;
  }

  for(int dist = 0; dist < max-1; dist++)
  {
    int newNumInQueue = 0;
    for(int i = 0; i<numInQueue; i++)
    {
      loc_t loc = oldQueue[i];
      if(goalDist[loc] != dist)
      {
        newQueue[newNumInQueue++] = loc;
        continue;
      }
      for(int dir = 0; dir<4; dir++)
      {
        if(!Board::ADJOKAY[dir][loc])
          continue;
        loc_t adj = loc + Board::ADJOFFSETS[dir];
        if(added.isOne(adj))
          continue;
        added.setOn(adj);

        int extra = 0;
        if(b.owners[adj] == NPLA) extra = !b.isTrapSafe2(pla,adj) ? 1 : !b.wouldRabbitBeUFAt(pla,adj) ? 1 : 0;
        else if(b.owners[adj] == pla) extra = 1;
        else extra = b.isDominated(adj) ? 2 : 3;

        goalDist[adj] = dist+extra+1 > max ? max : dist+extra+1;
        added.setOn(adj);
        newQueue[newNumInQueue++] = adj;
      }
    }
    loc_t* temp = newQueue;
    newQueue = oldQueue;
    oldQueue = temp;
    numInQueue = newNumInQueue;
  }
}
*/


//Finds cap threats. Allows an existing piece on the trap with no defenders - will count as a 0 step cap.
//rdSteps - number of steps to just remove defenders will be stored here.
/*
int ThreatsOld::findCapThreats(Board& b, pla_t pla, loc_t kt, CapThreat* threats, int* cache, int maxCapDist, int maxThreatLen, int& rdSteps)
{
  int numThreats = 0;
  pla_t opp = OPP(pla);

  //Count up basic statistics
  int defCount = 0; //Defender count
  int removeDefSteps = 0; //+2 for each defender, +3 for each undominated defender
  pce_t strongestPiece = 0; //Strongest defender
  loc_t strongestLoc = ERRORSQUARE; //Location of strongest defender
  loc_t defenders[4]; //Locations of defenders
  for(int i = 0; i<4; i++)
  {
    loc_t loc = kt + Board::ADJOFFSETS[i];
    if(b.owners[loc] == opp)
    {
      defenders[defCount] = loc;
      defCount++;
      removeDefSteps += 2;
      if(!b.isDominated(loc))
        removeDefSteps++;
      if(b.pieces[loc] > strongestPiece)
      {
        strongestPiece = b.pieces[loc];
        strongestLoc = loc;
      }
    }
  }

  //Store prelim value in case we return
  rdSteps = removeDefSteps;

  //No hope of capture ever!
  if(removeDefSteps > maxCapDist || strongestPiece == ELE)
    return 0;

  //Instant capture!
  if(defCount == 0 && b.owners[kt] == opp)
    threats[numThreats++] = CapThreat(0,kt,kt,ERRORSQUARE);

  //Harder to cap when own piece on the trap
  if(b.owners[kt] == pla && defCount != 0 && !b.isDominating(kt))
    removeDefSteps++;

  //Cannot threaten a 4 step cap of 2 defs?
  if(defCount == 2 && removeDefSteps == 4)
  {
    if(!BoardTrees::canCaps(b,pla,4,0,kt))
      removeDefSteps++;
  }

  //Harder to cap when opp piece on the trap.
  if(defCount > 0 && removeDefSteps > 4 && b.owners[kt] == opp)
    removeDefSteps++;

  //Can't possibly capture within 8 moves
  if(removeDefSteps > maxCapDist)
    return numThreats;

  //Total steps counting pushing the strongest
  int removeDefStepsTotal = 16;
  if(defCount == 0)
    removeDefStepsTotal = 0;
  else if(defCount > 0)
  {
    //Find pla strong enough to contest
    Bitmap plaStronger;
    for(int power = strongestPiece+1; power <= ELE; power++)
      plaStronger |= b.pieceMaps[pla][power];

    //Expand outward over threateners
    int firstCapRad = 16;
    for(int rad = 1; rad-2+removeDefSteps <= maxCapDist && rad <= firstCapRad+2; rad++)
    {
      //Adjust steps - one less if the strongest is dominated
      int stepAdjustment = (rad != 1 && b.isDominated(strongestLoc)) ? 0 : 1;

      //Find potential capturers
      Bitmap threateners = plaStronger & (rad == 1 ? Board::DISK[rad][strongestLoc] : Board::RADIUS[rad][strongestLoc]);

      //Iterate over the threateners!
      while(threateners.hasBits())
      {
        loc_t ploc = threateners.nextBit();

        //Init attack dist to be computed - 2 cases
        int attackSteps = 0;
        bool computeDistToStrongest = true;  //Do use dist to strongest, or just uf dist for threatener?

        //Case 0 - Always use UF dist for rad 1
        if(rad == 1)
          computeDistToStrongest = false;

        //Case 1 - Check if adjacent to some other defender
        else if(Board::MANHATTANDIST[ploc][kt] == 2)
        {
          for(int k = 0; k<defCount; k++)
          {
            if(Board::ISADJACENT[ploc][defenders[k]])
            {
              computeDistToStrongest = false;
              break;
            }
          }
        }

        //Compute attack dist!
        if(computeDistToStrongest)
          attackSteps = BoardEst::traverseAdjDist(b,ploc,strongestLoc,true,cache,maxCapDist+1+stepAdjustment-removeDefSteps) - stepAdjustment;
        else
          attackSteps = BoardEst::ufDist(b,ploc,cache);

        //Additional adjustment - don't double count the cost of a weak friendly piece on a trap in both the prelim checks and by attack dist
        if(b.owners[kt] == pla && defCount != 0 && rad == 2 && Board::ISADJACENT[ploc][kt] && !b.isDominating(kt))
          attackSteps--;

        //Found a cap!
        if(removeDefSteps + attackSteps <= maxCapDist)
        {
          //If shortest for trap, mark it
          if(removeDefSteps + attackSteps < removeDefStepsTotal)
            removeDefStepsTotal = removeDefSteps + attackSteps;

          //Mark closest rad so we know when to stop
          if(firstCapRad == 16)
            firstCapRad = rad;

          //Extra if opp on trap already
          int extra = 0;
          //Opp on trap
          if(b.owners[kt] == opp)
          {
            if(maxThreatLen <= numThreats)
            {
              if(DEBUG_ON) {cout << "MaxCaps" << endl; cout << b;}
              return numThreats;
            }
            extra = 3;
            threats[numThreats++] = CapThreat(removeDefSteps+attackSteps,kt,kt,ploc);
          }
          //Defenders
          int d = removeDefSteps+attackSteps+extra;
          if(d <= maxCapDist)
          {
            for(int j = 0; j<defCount; j++)
            {
              if(maxThreatLen <= numThreats)
              {
                if(DEBUG_ON) {cout << "MaxCaps" << endl; cout << b;}
                return numThreats;
              }
              threats[numThreats++] = CapThreat(d,defenders[j],kt,ploc);
            }
          }
        }
      }
    }
  }

  //Store remove def steps
  rdSteps = removeDefStepsTotal;

  //Locate captures for further pieces
  //Get pieces that could perform captures
  Bitmap threateners = b.pieceMaps[pla][0] & ~(b.pieceMaps[pla][RAB]);
  //Iterate out by radius
  for(int rad = 2; rad*2+removeDefStepsTotal <= maxCapDist; rad++)
  {
    //Compute base distance
    int basePushSteps = rad*2+removeDefStepsTotal;

    //Iterate over potential capturable pieces
    Bitmap otherPieces = b.pieceMaps[opp][0] & Board::RADIUS[rad][kt];
    while(otherPieces.hasBits())
    {
      loc_t eloc = otherPieces.nextBit();
      int totalPushSteps = basePushSteps + capInterferePathCost(b,pla,kt,eloc);

      //Max rad to go to
      int maxThreatRad = maxCapDist+1 - totalPushSteps > 3 ? 3 : (maxCapDist+1 - totalPushSteps);
      for(int threatRad = 1; threatRad <= maxThreatRad; threatRad++)
      {
        //Capturers
        Bitmap radThreateners = threateners & Board::RADIUS[threatRad][eloc];
        while(radThreateners.hasBits())
        {
          loc_t ploc = radThreateners.nextBit();
          //Exclude pieces not enough to threaten
          if(b.pieces[ploc] <= b.pieces[eloc]) continue;
          //Here we go!
          int traverseDist = BoardEst::traverseAdjDist(b,ploc,eloc,true,cache,maxThreatRad-1);
          int d = totalPushSteps+traverseDist;
          if(d <= maxCapDist)
          {
            if(maxThreatLen <= numThreats)
            {
              if(DEBUG_ON) {cout << "MaxCaps" << endl; cout << b;}
              return numThreats;
            }
            threats[numThreats++] = CapThreat(d,eloc,kt,ploc);
          }
        }
      }
    }
  }

  //Ensure we don't raise any false alarms - find fastest cap
  if(removeDefSteps <= 4)
  {
    int fastestDist = 64;
    for(int i = 0; i<numThreats; i++)
    {
      if(threats[i].dist < fastestDist && threats[i].dist != 0)
      {
        fastestDist = threats[i].dist;
        if(fastestDist <= 3)
          break;
      }
    }

    //If fastest cap is exactly 4, on the edge of possibility...
    if(fastestDist == 4)
    {
      //Handle instacaps by temp removal
      bool instaCap = false;
      pce_t oldPiece = 0;
      if(defCount == 0 && b.owners[kt] == opp)
      {
        instaCap = true;
        oldPiece = b.pieces[kt];
        b.owners[kt] = NPLA;
        b.pieces[kt] = EMP;
      }
      //Confirm that cap is actually possible!
      if(!BoardTrees::canCaps(b,pla,4,0,kt))
      {
        for(int i = 0; i<numThreats; i++)
          if(threats[i].dist <= 4 && threats[i].dist != 0)
            threats[i].dist = 5;
      }
      //Undo temp removal
      if(instaCap)
      {
        b.owners[kt] = opp;
        b.pieces[kt] = oldPiece;
      }
    }
  }

  return numThreats;
}
*/

//GOAL FINDING--------------------------------------------------------------
/*
//[yDist][yDist-dy]
static int dxGoalTable[8][8] = {
{0,0,0,0,0,0,0,0},
{3,3,0,0,0,0,0,0},
{2,2,2,0,0,0,0,0},
{2,2,1,1,0,0,0,0},
{2,1,1,1,1,0,0,0},
{1,1,1,1,0,0,0,0},
{1,1,1,0,0,0,0,0},
{1,1,0,0,0,0,0,0},
};

static const int goalThreatMaxDist = 8;

//Badloc - the location the rabbit would come through or ERRORSQUARE
//Badloc2 - the location the rabbit must leave through or ERRORSQUARE

static int goalBlockVal(Board& b, pla_t pla, loc_t rloc, loc_t loc, loc_t badloc, loc_t badloc2, int* cache)
{
  //Empty
  if(b.owners[loc] == NPLA)
  {
    //Goal? No block.
    if(Board::ISGOAL[pla][loc])
      return 0;
    //Trapsafe and rabbit unfrozen? No block.
    if(!Board::ISTRAP[loc] && b.wouldBeUF(pla,rloc,loc,rloc) || b.wouldBeG(pla,loc,rloc))
      return 0;
    //Else, frozen and/or trapunsafe, so block.
    return 1;
  }
  //Player occupied
  else if(b.owners[loc] == pla)
  {
    int dist = 3;
    if(CS1(loc)) dist = min(dist,b.pieces[loc-8] == NPLA ? 1 : BoardEst::blockValCanEndF(b,loc,loc-8,cache) + ((loc == badloc || loc == badloc2) ? 1 : 0));
    if(CW1(loc)) dist = min(dist,b.pieces[loc-1] == NPLA ? 1 : BoardEst::blockValCanEndF(b,loc,loc-1,cache) + ((loc == badloc || loc == badloc2) ? 1 : 0));
    if(CE1(loc)) dist = min(dist,b.pieces[loc+1] == NPLA ? 1 : BoardEst::blockValCanEndF(b,loc,loc+1,cache) + ((loc == badloc || loc == badloc2) ? 1 : 0));
    if(CN1(loc)) dist = min(dist,b.pieces[loc+8] == NPLA ? 1 : BoardEst::blockValCanEndF(b,loc,loc+8,cache) + ((loc == badloc || loc == badloc2) ? 1 : 0));

    //Trapsafe and step away freeze costs
    if(Board::ISADJACENT[loc][rloc])
      if(!b.isTrapSafe2(pla,rloc) || !b.wouldBeUF(pla,rloc,rloc,loc))
        dist += 1;

    //Add immo cost - thawed piece but UFdist
    if(b.isThawedC(loc))
      dist += BoardEst::ufDist(b,loc,cache);

    return dist+1;
  }
  //Opponent occupied
  else
  {
    pla_t opp = OPP(pla);
    //Take attack distance, plus 2 to remove opponent
    int dist = min(4,BoardEst::attackDistUncached(b,loc,3,cache)+2);
    //Add an additional dist if the opponent is flanked, making it harder to remove.
    if(!(CW1(loc) && b.owners[loc-1] != opp) && !(CE1(loc) && b.owners[loc+1] != opp))
      dist += 1;
    return dist;
  }
}
*/
/*
static int findGoalThreats(Board& b, pla_t pla, loc_t rloc, GoalThreat* threats, int* cache)
{
  int numThreats = 0;
  int gy = (pla == 1) ? 8 : -8;
  int rx = rloc % 8;
  int ry = rloc / 8;
  int yDist = Board::GOALYDIST[pla][rloc];

  //Already at Goal!
  if(yDist == 0)
  {
    numThreats++;
    *(threats++) = GoalThreat(0,rloc,rloc);
    return numThreats;
  }

  int ufCost = BoardEst::ufDist(b,rloc,cache);
  if(ufCost+yDist > goalThreatMaxDist)
    return 0;

  //A*
  unsigned char queue[64];
  unsigned char pathCost[64];
  unsigned char blockCost[64];
  unsigned char status[64];
  int queueLen = 0;
  for(int i = 0; i<64; i++)
  {
    pathCost[i] = 30;
    blockCost[i] = 64;
    status[i] = 0;
  }

  bool hExtraBase[8];
  int hExtraMinDist[8];
  int hExtra[8];
  for(int x = 0; x<8; x++)
  {
    unsigned char gloc = x+(ry+yDist)*gy;
    if(b.owners[gloc] == OPP(pla))
    {
     hExtraMinDist[x] = 1;
     hExtraBase[x] = true;
    }
    else if(b.owners[gloc-gy] == OPP(pla))
    {
     hExtraMinDist[x] = 2;
     hExtraBase[x] = true;
    }
    else
    {
     hExtraMinDist[x] = 0;
     hExtraBase[x] = false;
    }
  }
  hExtra[0] = hExtraBase[0] ? (hExtraBase[1] ? 2 : 1) : 0;
  hExtra[7] = hExtraBase[7] ? (hExtraBase[6] ? 2 : 1) : 0;
  for(int x = 1; x<7; x++)
   hExtra[x] = hExtraBase[x] ? (hExtraBase[x-1] && hExtraBase[x+1] ? 2 : 1) : 0;

  blockCost[rloc] = ufCost;
  pathCost[rloc] = ufCost;
  status[rloc] = 1;
  queue[queueLen++] = rloc;

  int next = 0;
  while(next < queueLen)
  {
    //Find next best
    int besti = next;
    int x = queue[besti]%8;
    int bestval = Board::GOALYDIST[pla][queue[besti]] + pathCost[queue[besti]] + (yDist >= hExtraMinDist[x] ? hExtra[x] : 0);
    for(int i = next+1; i<queueLen; i++)
    {
      int x2 = queue[i]%8;
      int yDist = Board::GOALYDIST[pla][queue[i]];
      int val = yDist + pathCost[queue[i]] + (yDist >= hExtraMinDist[x2] ? hExtra[x2] : 0);
      if(val < bestval)
      {
        besti = i;
        bestval = val;
      }
    }

    //Swap it to the front of the queue
    int temp = queue[next];
    queue[next] = queue[besti];
    queue[besti] = temp;

    //Provably too long to get to goal. Done.
    if(bestval > goalThreatMaxDist)
      break;

    //Explore next best
    loc_t loc = queue[next];
    int locDist = Board::GOALYDIST[pla][loc];
    next++;
    status[loc] = 2;

    //Goal?
    if(locDist == 0)
    {
      int bestCost = pathCost[loc];
      if(bestCost < 5)
      {
        if(BoardTrees::goalDist(b,pla,4,rloc) >= 5)
          bestCost = 5;
      }
      threats[numThreats++] = GoalThreat(bestCost,rloc,loc);
      break;
    }

    //Add neighbors to queue
    {
      loc_t neighbor = loc+gy;
      //Already in queue - try to improve cost
      if(status[neighbor] == 1)
        pathCost[neighbor] = min(pathCost[neighbor], pathCost[loc]+1+blockCost[neighbor]);
      //Not in queue
      else if(status[neighbor] == 0)
      {
        blockCost[neighbor] = goalBlockVal(b,pla,rloc,neighbor,(locDist == 1) ? loc : ERRORSQUARE,ERRORSQUARE,cache);
        pathCost[neighbor] = pathCost[loc]+1+blockCost[neighbor];
        status[neighbor] = 1;
        int yExtra = locDist-1 >= hExtraMinDist[loc%8] ? hExtra[loc%8] : 0;
        if(pathCost[neighbor]+locDist-1+yExtra <= goalThreatMaxDist)
          queue[queueLen++] = neighbor;
      }
    }
    int dxMax = dxGoalTable[yDist][yDist-locDist];
    int wExtra;
    if(CW1(loc) && loc%8 > rx-dxMax && pathCost[loc]+1+locDist+(wExtra=(locDist >= hExtraMinDist[loc%8-1] ? hExtra[loc%8-1] : 0)) <= goalThreatMaxDist)
    {
      loc_t neighbor = loc-1;
      //Already in queue - try to improve cost
      if(status[neighbor] == 1)
        pathCost[neighbor] = min(pathCost[neighbor], pathCost[loc]+1+blockCost[neighbor]);
      //Not in queue
      else if(status[neighbor] == 0)
      {
        blockCost[neighbor] = goalBlockVal(b,pla,rloc,neighbor,Board::GOALYDIST[pla][loc] == 0 ? loc : ERRORSQUARE,ERRORSQUARE,cache);
        pathCost[neighbor] = pathCost[loc]+1+blockCost[neighbor];
        status[neighbor] = 1;
        if(pathCost[neighbor]+locDist+wExtra <= goalThreatMaxDist)
          queue[queueLen++] = neighbor;
      }
    }
    int eExtra;
    if(CE1(loc) && loc%8 < rx+dxMax && pathCost[loc]+1+locDist+(eExtra=(locDist >= hExtraMinDist[loc%8+1] ? hExtra[loc%8+1] : 0)) <= goalThreatMaxDist)
    {
      loc_t neighbor = loc+1;
      //Already in queue - try to improve cost
      if(status[neighbor] == 1)
        pathCost[neighbor] = min(pathCost[neighbor], pathCost[loc]+1+blockCost[neighbor]);
      //Not in queue
      else if(status[neighbor] == 0)
      {
        blockCost[neighbor] = goalBlockVal(b,pla,rloc,neighbor,Board::GOALYDIST[pla][loc] == 0 ? loc : ERRORSQUARE,ERRORSQUARE,cache);
        pathCost[neighbor] = pathCost[loc]+1+blockCost[neighbor];
        status[neighbor] = 1;
        if(pathCost[neighbor]+locDist+eExtra <= goalThreatMaxDist)
          queue[queueLen++] = neighbor;
      }
    }
  }

  return numThreats;
}
*/
//CAPTURE DEFENSES--------------------------------------------------------------

/*
int ThreatsOld::findCapElephantDefense(Board& b, pla_t pla, loc_t kt, CapMDef* defenses, int* cache)
{
  Bitmap elemap = b.pieceMaps[pla][ELE];
  if(!elemap.hasBits())
    return 0;

  loc_t ploc = elemap.nextBit();
  if(Board::MANHATTANDIST[ploc][kt] > 5)
    return 0;

  int dist = BoardEst::traverseAdjDist(b,ploc,kt,false,cache,5);
  if(dist <= 4)
  {
    *(defenses++) = CapMDef(dist, kt, ploc);
    return 1;
  }

  return 0;
}

//Finds ways of defending a trap with multiple defenders, ignoring the elephant!
int ThreatsOld::findCapMultipieceDefense(Board& b, pla_t pla, loc_t kt, CapMDef* defenses, int* cache)
{
  //Count existing defenders other than elephant
  int numDefenses = 0;
  int defCount = 0;
  for(int i = 0; i<4; i++)
  {
    loc_t loc = kt + Board::ADJOFFSETS[i];
    if(b.owners[loc] == pla && b.pieces[loc] != ELE)
      defCount++;
  }

  //If defense is not needed, we are done. 3 or more defenders, or 2 with both undominated
  if(defCount > 2)
    return 0;
  if(defCount == 2)
  {
    for(int i = 0; i<4; i++)
    {
      loc_t loc = kt + Board::ADJOFFSETS[i];
      if(b.owners[loc] == pla && b.pieces[loc] != ELE)
        if(b.isDominated(loc))
          break;
      if(i == 3)
        return 0;
    }
  }

  if(defCount == 0)
  {
    //Look around for possible defenders
    int cost = 0;
    int numDefended = 0;
    Bitmap possibleDefenders = b.pieceMaps[pla][0] & ~b.pieceMaps[pla][ELE];
    for(int i = 0; i<4; i++)
    {
      loc_t loc = kt + Board::ADJOFFSETS[i];

      //To defend this location, must have a piece at least one stronger than any enemy here
      pce_t minPow = (b.owners[loc] != OPP(pla)) ? 0 : b.pieces[loc]+1;
      if(minPow > ELE)
        continue;

      loc_t lastDef = ERRORSQUARE;
      int count = 0; //Count of possible defenders for *this* square
      int bestCost = 16;

      //Search around at radius 1
      Bitmap pDef = Board::RADIUS[1][loc] & possibleDefenders;
      //For each piece...
      while(pDef.hasBits())
      {
        //If it is strong enough to displace any enemy standing here
        loc_t ploc = pDef.nextBit();
        if(b.pieces[ploc] < minPow)
          continue;

        //Can def within 2 steps?
        int ufDist = BoardEst::ufDist(b,ploc,cache);
        if(ufDist < 2 && b.isRabOkay(pla,ploc,loc))
        {
          //Found one!
          int guardDist = (minPow > 0) ? ufDist+2 : ufDist+1;
          lastDef = ploc;
          bestCost = min(bestCost, guardDist);
          count++;
        }
      }

      //Search around at radius 2 if we haven't found anything
      if(count < 1)
      {
        //For each radius 2 possible defender...
        Bitmap pDef2 = Board::RADIUS[2][loc] & possibleDefenders;
        while(pDef2.hasBits())
        {
          loc_t ploc = pDef2.nextBit();
          //If it can get here quick enough...
          if(BoardEst::traverseDist(b,ploc,loc,false,cache,3) <= 2)
          {
            //Found one!
            count++;
            lastDef = ploc;
            bestCost = min(bestCost, 2);
            break;
          }
        }
      }

      //Exclude defender if it's the only one, so not usable by other squares as a defender
      if(count == 1)
        possibleDefenders.setOff(lastDef);

      //Found a defender? Woohoo!
      if(count > 0)
      {
        numDefended++;
        cost += bestCost;
      }

      //If we have sucessfully defended at least 2 squares, we are done!
      if(numDefended >= 2 && cost < 4)
      {
        defenses[numDefenses++] = CapMDef(cost,kt,lastDef);
        return numDefenses;
      }
    }
  }
  else if(defCount == 1 || defCount == 2)
  {
    //It costs 2 to defend at an existing location.
    //However, do NOT count the elephant! We are happy to replace the elephant at an existing location
    if(b.owners[kt-8] == pla && b.pieces[kt-8] != ELE) {BoardEst::simulateBlock(pla,kt-8,2,cache);}
    if(b.owners[kt-1] == pla && b.pieces[kt-1] != ELE) {BoardEst::simulateBlock(pla,kt-1,2,cache);}
    if(b.owners[kt+1] == pla && b.pieces[kt+1] != ELE) {BoardEst::simulateBlock(pla,kt+1,2,cache);}
    if(b.owners[kt+8] == pla && b.pieces[kt+8] != ELE) {BoardEst::simulateBlock(pla,kt+8,2,cache);}

    //Special case - multipiece defense will not work when the new defending piece would be dominated, and the single existing
    //defender is already dominated, by a different piece.
    bool special1DefFail = false;
    loc_t specialDomLoc = ERRORSQUARE;
    if(defCount == 1)
    {
      loc_t defLoc = b.findGuard(pla,kt);
      int numDom = 0;
      for(int i = 0; i<4; i++)
      {
        loc_t loc = defLoc + Board::ADJOFFSETS[i];
        if(b.owners[loc] == OPP(pla) && b.pieces[loc] > b.pieces[defLoc])
        {numDom++; specialDomLoc = loc;}
      }
      if(numDom >= 1) special1DefFail = true;
      if(numDom >= 2) specialDomLoc = ERRORSQUARE;
    }

    //Iterate around for defenders
    int best = 16;
    Bitmap friends = b.pieceMaps[pla][0] & ~b.pieceMaps[pla][ELE];
    for(int rad = 0; rad<Board::NUMRADII[kt] && rad <= best+1 && rad <= 3; rad++)
    {
      if(rad == 1) {continue;}
      Bitmap radfriends = friends & Board::RADIUS[rad][kt];
      while(radfriends.hasBits() && rad < best+1)
      {
        loc_t ploc = radfriends.nextBit();

        //Special case - block each square where it would be dominated by a different piece
        if(special1DefFail)
        {
          for(int i = 0; i<4; i++)
          {
            loc_t loc = kt + Board::ADJOFFSETS[i];
            if(b.wouldBeDom(pla,ploc,loc,specialDomLoc))
              BoardEst::simulateBlock(pla,loc,4,cache);
          }
        }

        int dist = BoardEst::traverseAdjDist(b, ploc, kt, false, cache, best);

        //Undo special case
        if(special1DefFail)
        {
          for(int i = 0; i<4; i++)
          {
            loc_t loc = kt + Board::ADJOFFSETS[i];
            if(b.wouldBeDom(pla,ploc,loc,specialDomLoc))
              BoardEst::simulateBlock(pla,loc,-4,cache);
          }
        }


        if(dist < best)
          best = dist;
        if(dist <= 3 && dist <= best+1)
        {
          defenses[numDefenses++] = CapMDef(dist,kt,ploc);
          //Too many generated?
          if(numDefenses >= maxCapMDefPerKT)
          {
            //Quit!
            rad = 4;
            break;
          }
        }
      }
    }

    //Undo before returning
    if(b.owners[kt-8] == pla && b.pieces[kt-8] != ELE) {BoardEst::simulateBlock(pla,kt-8,-2,cache);}
    if(b.owners[kt-1] == pla && b.pieces[kt-1] != ELE) {BoardEst::simulateBlock(pla,kt-1,-2,cache);}
    if(b.owners[kt+1] == pla && b.pieces[kt+1] != ELE) {BoardEst::simulateBlock(pla,kt+1,-2,cache);}
    if(b.owners[kt+8] == pla && b.pieces[kt+8] != ELE) {BoardEst::simulateBlock(pla,kt+8,-2,cache);}
  }

  return numDefenses;
}

static int getSimpleThreatDist(Board& b, pla_t pla, loc_t ploc, loc_t loc, int* oppRDSteps)
{
  int minThreatDist = 8;
  for(int i = 0; i<4; i++)
  {
    loc_t kt = Board::TRAPLOCS[i];
    int dist = Board::MANHATTANDIST[loc][kt] * 2 + oppRDSteps[i] + (Board::ISADJACENT[ploc][kt] ? -2 : 0);
    if(dist < minThreatDist)
    {
      if(ploc != loc && !b.wouldBeDom(pla,ploc,loc))
        dist += 1;
      if(dist < minThreatDist)
        minThreatDist = dist;
    }
  }
  return minThreatDist;
}

CapRDef ThreatsOld::findCapRunawayDefense(Board& b, pla_t pla, loc_t ploc, int* oppRDSteps, int* cache)
{
  //Get current threat dist
  int curThreatDist = getSimpleThreatDist(b,pla,ploc,ploc,oppRDSteps);
  if(curThreatDist <= 0 || curThreatDist > 4)
    return CapRDef(16,ploc);

  //What locations are good to retreat to?
  pla_t opp = OPP(pla);
  int numRetreatLocs = 0;
  loc_t retreatLocs[4]; //Adjacent only. If a 2 step retreat, this will be the adjacent loc leading to it.
  char retreatSteps[4];
  for(int i = 0; i<4; i++)
  {
    if(!Board::ADJOKAY[i][ploc])
      continue;
    loc_t loc = ploc + Board::ADJOFFSETS[i];
    if(b.owners[loc] == pla || b.owners[loc] == opp && (b.pieces[loc] <= b.pieces[ploc] || b.isBlocked(loc)))
      continue;
    char steps = 1;
    if(b.owners[loc] == opp)
      steps = 2;

    int dist = getSimpleThreatDist(b,pla,ploc,loc,oppRDSteps);
    if(dist > 4 && dist > curThreatDist)
    {
      retreatLocs[numRetreatLocs] = loc;
      retreatSteps[numRetreatLocs] = steps;
      numRetreatLocs++;
    }
    else if(steps == 1 && b.wouldBeUF(pla,ploc,loc,ploc) && b.isTrapSafe2(pla,loc))
    {
      for(int j = 0; j<4; j++)
      {
        if(!Board::ADJOKAY[j][loc])
          continue;
        loc_t loc2 = loc + Board::ADJOFFSETS[j];
        if(loc2 == ploc || b.owners[loc2] != NPLA)
          continue;
        int dist2 = getSimpleThreatDist(b,pla,ploc,loc2,oppRDSteps);
        if(dist2 > 4 && dist2 > curThreatDist)
        {
          retreatLocs[numRetreatLocs] = loc;
          retreatSteps[numRetreatLocs] = steps+1;
          numRetreatLocs++;
          break;
        }
      }
    }
  }

  if(numRetreatLocs <= 0)
    return CapRDef(16, ploc);

  //Unfrozen? Find best retreat
  if(b.isThawed(ploc))
  {
    int rsteps = 16;
    for(int i = 0; i<numRetreatLocs; i++)
      rsteps = min(rsteps, retreatSteps[i]);
    return CapRDef(rsteps, ploc);
  }
  //Can we unfreeze in 1 step and then enact a retreat?
  else
  {
    for(int i = 0; i<4; i++)
    {
      if(!Board::ADJOKAY[i][ploc])
        continue;
      loc_t loc = ploc + Board::ADJOFFSETS[i];
      if(b.canStepAndOccupy(pla,loc) && b.isTrapSafe3(pla,loc))
      {
        int rsteps = 16;
        for(int j = 0; j<numRetreatLocs; j++)
        {
          if(!Board::ADJOKAY[j][ploc])
            continue;
          loc_t loc2 = ploc + Board::ADJOFFSETS[j];
          if(loc == loc2)
            continue;
          rsteps = min(rsteps, retreatSteps[j]);
        }
        if(rsteps <= 2)
          return CapRDef(rsteps+2, ploc);
      }
    }
  }

  return CapRDef(16,ploc);
}
*/



