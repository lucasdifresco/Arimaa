/*
 * strats.cpp
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
#include "strats.h"



//FRAMES --------------------------------------------------------------

//Attempts to detect whether pla holds a frame around kt, and if so, stores the relevant info in the given FrameThreat reference.
//Detects partial frames - that is, frames that are open but such that a piece stepping off could immediately be pushed back on.
static Bitmap getFrameHolderMap(const Board& b, pla_t pla, loc_t kt)
{
  pla_t opp = OPP(pla);
  piece_t piece = b.pieces[kt];
  Bitmap plaGeq;
  for(int i = piece; i<=ELE; i++)
    plaGeq |= b.pieceMaps[pla][i];

  Bitmap map = Bitmap::adj(Bitmap::makeLoc(kt));
  map &= ~b.pieceMaps[opp][0];
  map |= Bitmap::adj(map & b.pieceMaps[pla][0] & ~plaGeq);
  map.setOff(kt);
  return map;
}

int Strats::findFrame(const Board& b, pla_t pla, loc_t kt, FrameThreat* threats)
{
  //Ensure that there is an opp on the trap!
  pla_t opp = OPP(pla);
  if(b.owners[kt] != opp)
  {return 0;}

  //Check each direction. At most one partial allowed, at most one opponent allowed.
  FrameThreat threat;
  threat.pinnedLoc = ERRORSQUARE;
  int partialcount = 0;
  int oppCount = 0;
  if     (b.owners[kt-8] == NPLA) {if(b.isRabOkayS(opp,kt)) {return 0;}}
  else if(b.owners[kt-8] == opp)  {oppCount++; threat.pinnedLoc = kt-8; if(oppCount > 1) return 0;}
  else if(b.pieces[kt-8] < b.pieces[kt] && b.isOpen(kt-8)) {partialcount++; if(partialcount > 1 || b.wouldBeUF(opp,kt,kt-8,kt)) return 0;}

  if     (b.owners[kt-1] == NPLA) {return 0;}
  else if(b.owners[kt-1] == opp)  {oppCount++; threat.pinnedLoc = kt-1; if(oppCount > 1) return 0;}
  else if(b.pieces[kt-1] < b.pieces[kt] && b.isOpen(kt-1)) {partialcount++; if(partialcount > 1 || b.wouldBeUF(opp,kt,kt-1,kt)) return 0;}

  if     (b.owners[kt+1] == NPLA) {return 0;}
  else if(b.owners[kt+1] == opp)  {oppCount++; threat.pinnedLoc = kt+1; if(oppCount > 1) return 0;}
  else if(b.pieces[kt+1] < b.pieces[kt] && b.isOpen(kt+1)) {partialcount++; if(partialcount > 1 || b.wouldBeUF(opp,kt,kt+1,kt)) return 0;}

  if     (b.owners[kt+8] == NPLA) {if(b.isRabOkayN(opp,kt)) {return 0;}}
  else if(b.owners[kt+8] == opp)  {oppCount++; threat.pinnedLoc = kt+8; if(oppCount > 1) return 0;}
  else if(b.pieces[kt+8] < b.pieces[kt] && b.isOpen(kt+8)) {partialcount++; if(partialcount > 1 || b.wouldBeUF(opp,kt,kt+8,kt)) return 0;}

  if(partialcount >= 1)
    threat.isPartial = true;
  else
    threat.isPartial = false;

  threat.kt = kt;
  threat.holderMap = getFrameHolderMap(b,pla,kt);
  threats[0] = threat;
  return 1;
}



//HOSTAGES --------------------------------------------------------------
//Possible hostage locations around trap 18.
//111100000
//110100000
//100000000
//010000000
//000000000
//000000000
//000000000
//000000000

static const int hostageLocs[4][Strats::maxHostagesPerKT] = {
{25,16, 9, 2,11, 8, 1, 0, 3}, //18
{30,23,14, 5,12,15, 6, 7, 4}, //21
{33,40,49,58,51,48,57,56,59}, //42
{38,47,54,61,52,55,62,63,60}, //45
};


static int capInterfereCost(const Board& b, pla_t pla, loc_t eloc, loc_t loc)
{
  if(b.owners[loc] == pla)
  {
    int cost = 0;
    if(b.pieces[loc] <= b.pieces[eloc])
      cost += 1;
    if(b.isFrozen(loc) != 0)
      cost += 1;
    return cost;
  }
  return 0;
}

static int capInterferePathCost(const Board& b, pla_t pla, loc_t kt, loc_t eloc)
{
  int dx = (int)(eloc&7) - (int)(kt&7);
  int dy = (int)(eloc>>3) - (int)(kt>>3);
  int mdist = Board::MANHATTANDIST[kt][eloc];
  if(mdist < 2)
    return 0;

  int cost = 0;
  cost += capInterfereCost(b,pla,eloc,kt);

  if(dx == 0 || dy == 0)
  {
    if(dy < 0)       cost += capInterfereCost(b,pla,eloc,kt-8);
    else if (dx < 0) cost += capInterfereCost(b,pla,eloc,kt-1);
    else if (dx > 0) cost += capInterfereCost(b,pla,eloc,kt+1);
    else             cost += capInterfereCost(b,pla,eloc,kt+8);
  }
  else
  {
    if(dy < 0 && dx < 0)      cost += min(capInterfereCost(b,pla,eloc,kt-8),capInterfereCost(b,pla,eloc,kt-1));
    else if(dy < 0 && dx > 0) cost += min(capInterfereCost(b,pla,eloc,kt-8),capInterfereCost(b,pla,eloc,kt+1));
    else if(dy > 0 && dx < 0) cost += min(capInterfereCost(b,pla,eloc,kt+8),capInterfereCost(b,pla,eloc,kt-1));
    else                      cost += min(capInterfereCost(b,pla,eloc,kt+8),capInterfereCost(b,pla,eloc,kt+1));
  }

  if(mdist > 2)
  {
    if(dx == 0 || dy == 0)
    {
      if(dy < 0)       cost += capInterfereCost(b,pla,eloc,eloc+8);
      else if (dx < 0) cost += capInterfereCost(b,pla,eloc,eloc+1);
      else if (dx > 0) cost += capInterfereCost(b,pla,eloc,eloc-1);
      else             cost += capInterfereCost(b,pla,eloc,eloc-8);
    }
    else
    {
      if(dy < 0 && dx < 0)      cost += min(capInterfereCost(b,pla,eloc,eloc+8),capInterfereCost(b,pla,eloc,eloc+1));
      else if(dy < 0 && dx > 0) cost += min(capInterfereCost(b,pla,eloc,eloc+8),capInterfereCost(b,pla,eloc,eloc-1));
      else if(dy > 0 && dx < 0) cost += min(capInterfereCost(b,pla,eloc,eloc-8),capInterfereCost(b,pla,eloc,eloc+1));
      else                      cost += min(capInterfereCost(b,pla,eloc,eloc-8),capInterfereCost(b,pla,eloc,eloc-1));
    }
  }

  return cost;
}


static int getHostageThreatDist(const Board& b, pla_t pla, loc_t hostageLoc, loc_t holderLoc, loc_t holderLoc2, loc_t kt, const int ufDist[64])
{
  int threatDist = Board::MANHATTANDIST[hostageLoc][kt]*2 + capInterferePathCost(b,pla,kt,hostageLoc);
  if(b.owners[kt] == pla && b.pieces[kt] <= b.pieces[hostageLoc])
    threatDist += 1;
  int ufD = ufDist[holderLoc];
  if(holderLoc2 != ERRORSQUARE && ufD > 0)
  {
    int ufD2 = ufDist[holderLoc2];
    if(ufD2 < ufD)
      ufD = ufD2;
  }
  return threatDist+ufD;
}

int Strats::findHostages(const Board& b, pla_t pla, loc_t kt, HostageThreat* threats, const int ufDist[64])
{
  int numThreats = 0;
  pla_t opp = OPP(pla);
  int ktindex = Board::TRAPINDEX[kt];
  for(int i = 0; i<Strats::maxHostagesPerKT; i++)
  {
    loc_t loc = hostageLocs[ktindex][i];
    if(b.owners[loc] == opp)
    {
      //Hostage if there is a holder that dominates and the hostage is frozen or immo
      //For each surrounding square, locate possible holders.
      loc_t holderLoc = ERRORSQUARE;
      loc_t holderLoc2 = ERRORSQUARE;
      for(int j = 0; j<4; j++)
      {
        if(!Board::ADJOKAY[j][loc])
          continue;
        loc_t hloc = loc + Board::ADJOFFSETS[j];
        if(b.owners[hloc] == pla && b.pieces[hloc] > b.pieces[loc])
        {
          holderLoc2 = holderLoc;
          holderLoc = hloc;
        }
      }

      if(holderLoc == ERRORSQUARE)
        continue;

      //Ensure frozen or immo
      if(ufDist[loc] <= 0)
        continue;

      threats[numThreats].kt = kt;
      threats[numThreats].hostageLoc = loc;
      threats[numThreats].holderLoc = holderLoc;
      threats[numThreats].holderLoc2 = holderLoc2;
      threats[numThreats].threatSteps = getHostageThreatDist(b,pla,loc,holderLoc,holderLoc2,kt,ufDist);
      numThreats++;
    }
  }

  return numThreats;
}

static const int AUTOBLOCKADE[64] =
{
1,1,1,1,1,1,1,1,
2,3,3,3,3,3,3,2,
3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,
2,3,3,3,3,3,3,2,
1,1,1,1,1,1,1,1,
};

//loc contains opp. Pla is blocking
static Bitmap stepBlockMap(const Board& b, pla_t pla, loc_t oloc, loc_t loc)
{
  pla_t opp = OPP(pla);
  Bitmap map = Board::DISK[1][loc];
  if(CS1(loc) && !b.isRabOkayS(opp,loc))
    map.setOff(loc-8);
  else if(CN1(loc) && !b.isRabOkayN(opp,loc))
    map.setOff(loc+8);
  map.setOff(oloc);
  return map;
}

//loc contains opp. Pla is blocking
static Bitmap stepPushBlockMap(const Board& b, pla_t pla, loc_t oloc, loc_t loc)
{
  if(b.pieces[loc] == RAB) {return stepBlockMap(b,pla,oloc,loc);}
  pla_t opp = OPP(pla);
  Bitmap bmap;
  if(CS1(loc) && loc != oloc)
  {
    if     (b.owners[loc-8] == opp) {bmap |= stepBlockMap(b,pla,loc,loc-8);}
    else if(b.owners[loc-8] == pla && b.pieces[loc-8] >= b.pieces[loc]) {bmap.setOn(loc-8);}
    else if(b.owners[loc-8] == pla) {bmap |= Board::DISK[1][loc-8] & ~Bitmap::makeLoc(loc);}
  }
  if(CW1(loc) && loc != oloc)
  {
    if     (b.owners[loc-1] == opp) {bmap |= stepBlockMap(b,pla,loc,loc-1);}
    else if(b.owners[loc-1] == pla && b.pieces[loc-1] >= b.pieces[loc]) {bmap.setOn(loc-1);}
    else if(b.owners[loc-1] == pla) {bmap |= Board::DISK[1][loc-1] & ~Bitmap::makeLoc(loc);}
  }
  if(CE1(loc) && loc != oloc)
  {
    if     (b.owners[loc+1] == opp) {bmap |= stepBlockMap(b,pla,loc,loc+1);}
    else if(b.owners[loc+1] == pla && b.pieces[loc+1] >= b.pieces[loc]) {bmap.setOn(loc+1);}
    else if(b.owners[loc+1] == pla) {bmap |= Board::DISK[1][loc+1] & ~Bitmap::makeLoc(loc);}
  }
  if(CN1(loc) && loc != oloc)
  {
    if     (b.owners[loc+8] == opp) {bmap |= stepBlockMap(b,pla,loc,loc+8);}
    else if(b.owners[loc+8] == pla && b.pieces[loc+8] >= b.pieces[loc]) {bmap.setOn(loc+8);}
    else if(b.owners[loc+8] == pla) {bmap |= Board::DISK[1][loc+8] & ~Bitmap::makeLoc(loc);}
  }
  return bmap;
}

//Helper checking if pla is blockading oloc. Can oloc escape through loc? Mark the relevant blockers. Assume oloc is not frozen.
static int getBlockadeStr(Board& b, pla_t pla, loc_t oloc, loc_t loc, Bitmap& defMap, int max)
{
  pla_t opp = OPP(pla);
  if(b.owners[loc] == pla)
  {
    if(b.pieces[loc] >= b.pieces[oloc]) {defMap.setOn(loc); return BlockadeThreat::FULLBLOCKADE;}
    if(!b.isTrapSafe2(opp,loc)) {defMap.setOn(loc); return BlockadeThreat::FULLBLOCKADE;}
    if(!b.isOpen(loc)) {defMap |= Board::DISK[1][loc] & ~Bitmap::makeLoc(oloc); return BlockadeThreat::FULLBLOCKADE;}
    if(max <= BlockadeThreat::FULLBLOCKADE) return BlockadeThreat::OPENBLOCKADE;

    if(AUTOBLOCKADE[loc] <= BlockadeThreat::LOOSEBLOCKADE) {defMap.setOn(loc); return BlockadeThreat::LOOSEBLOCKADE;}
    if(!b.isOpen2(loc))
    {
      loc_t openloc = b.findOpen(loc);
      b.tempPP(oloc,loc,openloc);
      Bitmap tempMap;
      bool good =
        (!(CS1(loc) && loc-8 != oloc && getBlockadeStr(b,pla,loc,loc-8,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
         !(CW1(loc) && loc-1 != oloc && getBlockadeStr(b,pla,loc,loc-1,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
         !(CE1(loc) && loc+1 != oloc && getBlockadeStr(b,pla,loc,loc+1,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
         !(CN1(loc) && loc+8 != oloc && getBlockadeStr(b,pla,loc,loc+8,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE));
      b.tempPP(openloc,loc,oloc);
      if(good) {tempMap.setOff(openloc); tempMap.setOn(loc); defMap |= tempMap; return BlockadeThreat::LOOSEBLOCKADE;}
    }
    if(max <= BlockadeThreat::LOOSEBLOCKADE) return BlockadeThreat::OPENBLOCKADE;

    if(AUTOBLOCKADE[loc] <= BlockadeThreat::LEAKYBLOCKADE) {defMap.setOn(loc); return BlockadeThreat::LEAKYBLOCKADE;}
    if(b.isTrap(loc)) {defMap.setOn(loc); return BlockadeThreat::LEAKYBLOCKADE;}
  }
  else if(b.owners[loc] == opp)
  {
    if(!b.isOpenToStep(loc) && !b.isOpenToMove(loc)) {defMap |= stepPushBlockMap(b,pla,oloc,loc); return BlockadeThreat::FULLBLOCKADE;}
    if(max <= BlockadeThreat::FULLBLOCKADE) return BlockadeThreat::OPENBLOCKADE;

    if(AUTOBLOCKADE[loc] <= BlockadeThreat::LOOSEBLOCKADE) {defMap.setOn(loc); return BlockadeThreat::LOOSEBLOCKADE;}
    if(b.isOpenToStep(loc))
    {
      loc_t openloc = b.findOpenToStep(loc);
      b.tempPP(oloc,loc,openloc);
      Bitmap tempMap;
      bool good =
        (!(CS1(loc) && loc-8 != oloc && getBlockadeStr(b,pla,loc,loc-8,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
         !(CW1(loc) && loc-1 != oloc && getBlockadeStr(b,pla,loc,loc-1,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
         !(CE1(loc) && loc+1 != oloc && getBlockadeStr(b,pla,loc,loc+1,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
         !(CN1(loc) && loc+8 != oloc && getBlockadeStr(b,pla,loc,loc+8,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE));
      b.tempPP(openloc,loc,oloc);
      if(good) {tempMap.setOff(openloc); defMap |= tempMap; return BlockadeThreat::LOOSEBLOCKADE;}
    }
    if(max <= BlockadeThreat::LOOSEBLOCKADE) return BlockadeThreat::OPENBLOCKADE;

    if(AUTOBLOCKADE[loc] <= BlockadeThreat::LEAKYBLOCKADE) {defMap.setOn(loc); return BlockadeThreat::LEAKYBLOCKADE;}
    if(b.isTrap(loc) && !b.isOpenToStep(loc)) {defMap |= stepBlockMap(b,pla,oloc,loc); return BlockadeThreat::LEAKYBLOCKADE;}
  }
  else //if empty
  {
    if(!b.isTrapSafe2(opp,loc)) {defMap.setOn(loc); return BlockadeThreat::FULLBLOCKADE;}
    if(max <= BlockadeThreat::FULLBLOCKADE) return BlockadeThreat::OPENBLOCKADE;

    if(AUTOBLOCKADE[loc] <= BlockadeThreat::LOOSEBLOCKADE) return BlockadeThreat::LOOSEBLOCKADE;
    b.tempStep(oloc,loc);
    Bitmap tempMap;
    bool good =
      (!(CS1(loc) && loc-8 != oloc && getBlockadeStr(b,pla,loc,loc-8,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
       !(CW1(loc) && loc-1 != oloc && getBlockadeStr(b,pla,loc,loc-1,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
       !(CE1(loc) && loc+1 != oloc && getBlockadeStr(b,pla,loc,loc+1,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE) &&
       !(CN1(loc) && loc+8 != oloc && getBlockadeStr(b,pla,loc,loc+8,tempMap,BlockadeThreat::FULLBLOCKADE) > BlockadeThreat::FULLBLOCKADE));
    b.tempStep(loc,oloc);

    if(good) {tempMap.setOff(loc); defMap |= tempMap; return BlockadeThreat::LOOSEBLOCKADE;}
    if(max <= BlockadeThreat::LOOSEBLOCKADE) return BlockadeThreat::OPENBLOCKADE;

    if(AUTOBLOCKADE[loc] <= BlockadeThreat::LEAKYBLOCKADE) return BlockadeThreat::LEAKYBLOCKADE;
    if(b.isTrap(loc) && !b.isTrapSafe3(opp,loc)) {defMap.setOn(loc); return BlockadeThreat::LEAKYBLOCKADE;}
  }
  return BlockadeThreat::OPENBLOCKADE;
}

//Is pla blockading oloc?
static bool isBlockade(Board& b, pla_t pla, loc_t oloc, BlockadeThreat& threat)
{
  if(!b.isFrozen(oloc))
  {
    Bitmap defMap;
    int strsum = 0;
    if(CS1(oloc)) {strsum += getBlockadeStr(b,pla,oloc,oloc-8,defMap,BlockadeThreat::OPENBLOCKADE); if(strsum >= 3) return false;}
    if(CW1(oloc)) {strsum += getBlockadeStr(b,pla,oloc,oloc-1,defMap,BlockadeThreat::OPENBLOCKADE); if(strsum >= 3) return false;}
    if(CE1(oloc)) {strsum += getBlockadeStr(b,pla,oloc,oloc+1,defMap,BlockadeThreat::OPENBLOCKADE); if(strsum >= 3) return false;}
    if(CN1(oloc)) {strsum += getBlockadeStr(b,pla,oloc,oloc+8,defMap,BlockadeThreat::OPENBLOCKADE); if(strsum >= 3) return false;}
    threat.holderMap = defMap;
    threat.tightness = strsum;
    threat.pinnedLoc = oloc;
    return true;
  }
  else
  {
    Bitmap map[4] = {Bitmap(),Bitmap(),Bitmap(),Bitmap()};
    int strsum = 0;
    int strmaxIndex = 0;
    int strmax = -1;
    if(CS1(oloc)) {int str = getBlockadeStr(b,pla,oloc,oloc-8,map[0],BlockadeThreat::OPENBLOCKADE); strsum += str; if(str > strmax) {strmax = str; strmaxIndex = 0;}}
    if(CW1(oloc)) {int str = getBlockadeStr(b,pla,oloc,oloc-1,map[1],BlockadeThreat::OPENBLOCKADE); strsum += str; if(str > strmax) {strmax = str; strmaxIndex = 1;}}
    if(CE1(oloc)) {int str = getBlockadeStr(b,pla,oloc,oloc+1,map[2],BlockadeThreat::OPENBLOCKADE); strsum += str; if(str > strmax) {strmax = str; strmaxIndex = 2;}}
    if(CN1(oloc)) {int str = getBlockadeStr(b,pla,oloc,oloc+8,map[3],BlockadeThreat::OPENBLOCKADE); strsum += str; if(str > strmax) {strmax = str; strmaxIndex = 3;}}

    strsum -= strmax;
    if(strsum >= 3) {return false;}

    //This is buggy if all are full-blockaded. It could eliminate one necessary to the blockade, like the freezing piece! TODO
    Bitmap defMap;
    for(int i = 0; i<4; i++)
       if(i != strmaxIndex)
         defMap |= map[i];

    threat.holderMap = defMap;
    threat.tightness = strsum;
    threat.pinnedLoc = oloc;
    return true;
  }
}

//Pla blockades opp
int Strats::findBlockades(Board& b, pla_t pla, BlockadeThreat* threats)
{
  int numThreats = 0;
  pla_t opp = OPP(pla);
  Bitmap eMap = b.pieceMaps[opp][ELE];
  if(eMap.hasBits())
  {
    loc_t loc = eMap.nextBit();
    if(isBlockade(b,pla,loc,threats[numThreats]))
      numThreats++;
  }
  return numThreats;
}











//NEW BLOCKADE--------------------------------------------------------------------------

static const int TOTAL_IMMO = Strats::TOTAL_IMMO;
static const int ALMOST_IMMO = Strats::ALMOST_IMMO;
static const int CENTRAL_BLOCK = Strats::CENTRAL_BLOCK;
static const int CENTRAL_BLOCK_WEAK = Strats::CENTRAL_BLOCK_WEAK;

//Most important location for blockading
static const int CENTRAL_ADJ_1[64] =
{
 8, 9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,
24,25,26,27,28,29,30,31,
25,26,34,35,36,37,29,30,
33,34,26,27,28,29,37,38,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,
};
//Second important location for blockading
static const int CENTRAL_ADJ_2[64] =
{
 1, 2, 3, 4, 3, 4, 5, 6,
 9,10,11,12,11,12,13,14,
17,18,19,20,19,20,21,22,
32,33,27,28,27,28,38,39,
24,25,35,36,35,36,30,31,
41,42,43,44,43,44,45,46,
49,50,51,52,51,52,53,54,
57,58,59,60,59,60,61,62,
};
//Third important location for blockading
static const int EDGE_ADJ_1[64] =
{
 0, 0, 1, 2, 5, 6, 7, 7,
 8, 8, 9,10,13,14,15,15,
16,16,17,18,21,22,23,23,
24,24,25,26,29,30,31,31,
32,32,33,34,37,38,39,39,
40,40,41,42,45,46,47,47,
48,48,49,50,53,54,55,55,
56,56,57,58,61,62,63,63,
};

//Least important location for blockading
static const int EDGE_ADJ_2[64] =
{
 0, 1, 2, 3, 4, 5 ,6, 7,
 0, 1, 2, 3, 4, 5, 6, 7,
 8, 9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,
40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,
56,57,58,59,60,61,62,63,
56,57,58,59,60,61,62,63,
};

static const int WEAK_CENTRAL_BLOCK_OK[64] =
{
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  1,1,0,0,0,0,1,1,
  1,1,0,0,0,0,1,1,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
};

/*
static loc_t smallestDominator(const Board& b, pla_t pla, loc_t ploc)
{
  pla_t opp = OPP(pla);
  piece_t piece = ELE;
  loc_t loc = ERRORSQUARE;
  if(CS1(ploc) && b.owners[ploc S] == opp && b.pieces[ploc S] > b.pieces[ploc]) {if(b.pieces[ploc S] <= piece) {loc = ploc S; piece = b.pieces[ploc S];}}
  if(CW1(ploc) && b.owners[ploc W] == opp && b.pieces[ploc W] > b.pieces[ploc]) {if(b.pieces[ploc W] <= piece) {loc = ploc W; piece = b.pieces[ploc W];}}
  if(CE1(ploc) && b.owners[ploc E] == opp && b.pieces[ploc E] > b.pieces[ploc]) {if(b.pieces[ploc E] <= piece) {loc = ploc E; piece = b.pieces[ploc E];}}
  if(CN1(ploc) && b.owners[ploc N] == opp && b.pieces[ploc N] > b.pieces[ploc]) {if(b.pieces[ploc N] <= piece) {loc = ploc N; piece = b.pieces[ploc N];}}
  return loc;
}*/

static const int LOC_NOT_BLOCKADED = 0; //Not blocked at all.
static const int LOC_BLOCKADED_LOOSE = 1; //Can push through but then stuck.
static const int LOC_BLOCKADED_HARD = 2; //Can't step through
//Pla blockading opp
//Recursive on each surrounding piece when it encounters an opp piece.
//oloc - previous opp loc.
//prevloc - previous loc in the recursion (could be different from oloc if we cross an empty square)
//AllowLoose - if it's okay for loc to be empty or push-in-able, so long it's still blockadey
//WasLoose - whether it was loose or not. MUST BE INITED TO FALSE.
//Recursedmap - All pieces or trap squares recursed on, whether part of the blockade or not. These include unsafe traps and any interior blockers
//HolderHeldMap - All pieces involved, but excludes the case where a pla piece is on a trap if the trap is unsafe for the opp. Might have empty squares, but can filter them out
//FreezeHeldMap - All pieces that need to be physically frozen in order to hold the blockade
static bool isBlockadeRec(Board& b, const int ufDist[64], pla_t pla, loc_t oloc, loc_t prevloc, loc_t loc,
    Bitmap& allowRecheck, Bitmap& recursedMap, Bitmap& holderHeldMap, Bitmap& freezeHeldMap, bool allowLoose, bool& wasLoose)
{
  //Don't recurse on squares already handled
  //There's one glitch however, where a piece can be found to be strong ehough to block one piece, but there's a
  //SECOND piece in the blockade it's not strong neough to block.
  //So we use "allowRecheck" to track this and allow rechecks in this case.
  if((recursedMap & ~allowRecheck).isOne(loc))
    return true;

  recursedMap.setOn(loc);
  pla_t opp = OPP(pla);

  //Unsafe trap
  if(b.owners[loc] != opp && !b.isTrapSafe2(opp,loc))
    return true;

  //Opp? Check surrounding squares
  if(b.owners[loc] == opp)
  {
    holderHeldMap.setOn(loc);
    loc_t nextloc;
    nextloc = CENTRAL_ADJ_1[loc]; if(nextloc != prevloc                   && b.isRabOkay(opp,loc,nextloc) && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) return false;
    nextloc = CENTRAL_ADJ_2[loc]; if(nextloc != prevloc                   && b.isRabOkay(opp,loc,nextloc) && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) return false;
    nextloc = EDGE_ADJ_1[loc];    if(nextloc != prevloc && nextloc != loc && b.isRabOkay(opp,loc,nextloc) && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) return false;
    nextloc = EDGE_ADJ_2[loc];    if(nextloc != prevloc && nextloc != loc && b.isRabOkay(opp,loc,nextloc) && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) return false;
    return true;
  }
  //Empty? Allow only if allowing loose, and make sure surrounding squares are tight.
  else if(b.owners[loc] == NPLA)
  {
    if(!allowLoose)
      return false;

    //Temporarily fill the square with an elephant, and see if the blockade still works.
    b.owners[loc] = opp;
    b.pieces[loc] = ELE;
    wasLoose = true;
    loc_t nextloc;
    nextloc = CENTRAL_ADJ_1[loc]; if(nextloc != prevloc                   && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) {b.owners[loc] = NPLA; b.pieces[loc] = EMP; return false;}
    nextloc = CENTRAL_ADJ_2[loc]; if(nextloc != prevloc                   && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) {b.owners[loc] = NPLA; b.pieces[loc] = EMP; return false;}
    nextloc = EDGE_ADJ_1[loc];    if(nextloc != prevloc && nextloc != loc && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) {b.owners[loc] = NPLA; b.pieces[loc] = EMP; return false;}
    nextloc = EDGE_ADJ_2[loc];    if(nextloc != prevloc && nextloc != loc && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) {b.owners[loc] = NPLA; b.pieces[loc] = EMP; return false;}
    b.owners[loc] = NPLA; b.pieces[loc] = EMP;
    return true;
  }
  //Pla
  else
  {
    holderHeldMap.setOn(loc);
    //Unpushable?
    if(b.pieces[loc] >= b.pieces[oloc])
    {return true;}

    //We cannot infinite loop because of the rechecking allowed by this map. Because allowRecheck is set ONLY on pla pieces initially,
    //and in any case where we allowed a recheck, the check either must terminate on the above "unpushable" line without recursing, or it
    //must remove another additional bit from this map.
    allowRecheck.setOff(loc);

    int numOpen = b.countOpen(loc);
    if(numOpen > 0)
    {
      if(!allowLoose || numOpen >= 2)
        return false;

      loc_t openSq = b.findOpen(loc);
      //Pretend we've pushed and insert a new elephant to determine blockedness.
      b.owners[openSq] = b.owners[loc];
      b.pieces[openSq] = b.pieces[loc];
      b.owners[loc] = opp;
      b.pieces[loc] = ELE;
      wasLoose = true;

      bool isBlock = true;
      holderHeldMap.setOn(loc);
      loc_t nextloc;
      if(isBlock) {nextloc = CENTRAL_ADJ_1[loc]; if(nextloc != prevloc                   && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) isBlock = false;}
      if(isBlock) {nextloc = CENTRAL_ADJ_2[loc]; if(nextloc != prevloc                   && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) isBlock = false;}
      if(isBlock) {nextloc = EDGE_ADJ_1[loc];    if(nextloc != prevloc && nextloc != loc && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) isBlock = false;}
      if(isBlock) {nextloc = EDGE_ADJ_2[loc];    if(nextloc != prevloc && nextloc != loc && !isBlockadeRec(b,ufDist,pla,loc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) isBlock = false;}

      b.owners[loc] = b.owners[openSq]; b.pieces[loc] = b.pieces[openSq];
      b.owners[openSq] = NPLA; b.pieces[openSq] = EMP;
      return isBlock;
    }

    //If there's a pla piece on the trap, then if the opponent has not enough defenders, it's still blockaed.
    //We don't have to check pla defenders, because opptrapunsafe3 && blocked ==> pla trapsafe2.
    if(!b.isTrapSafe3(opp,loc))
    {
      holderHeldMap |= (Board::RADIUS[1][loc] & b.pieceMaps[pla][0]);
      return true;
    }

    //Otherwise, check for a phalanx
    loc_t nextloc;
    loc_t locs[4] = {static_cast<loc_t>(CENTRAL_ADJ_1[loc]), static_cast<loc_t>(CENTRAL_ADJ_2[loc]), static_cast<loc_t>(EDGE_ADJ_1[loc]), static_cast<loc_t>(EDGE_ADJ_2[loc]) };
    for(int i = 0; i<4; i++)
    {
      nextloc = locs[i];
      if(nextloc == prevloc || nextloc == loc)
        continue;
      //if(b.owners[nextloc] == NPLA) return false; //Don't need to check because we checked b.isOpen above
      if(b.owners[nextloc] == opp) {
        if(!wasLoose && b.isFrozenC(nextloc)) {holderHeldMap.setOn(nextloc); freezeHeldMap.setOn(nextloc);}
        else if(ufDist[nextloc] == 0 || !isBlockadeRec(b,ufDist,pla,oloc,loc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,false,wasLoose)) {return false;}
        else holderHeldMap.setOn(nextloc);
      }
      else //pla
        holderHeldMap.setOn(nextloc);
    }
    return true;
  }
}

//TODO Use restrict keyword?

//Pla is holder
static bool isEleBlockade(Board& b, const int ufDist[64], pla_t pla, loc_t oppEleLoc,
    Bitmap& recursedMap, Bitmap& holderHeldMap, Bitmap& freezeHeldMap, int& immoType)
{
  Bitmap allowRecheck = b.pieceMaps[pla][0];
  recursedMap.setOn(oppEleLoc);
  holderHeldMap.setOn(oppEleLoc);
  bool isEverLoose = false;
  bool wasLoose;

  immoType = TOTAL_IMMO;

  loc_t nextloc = CENTRAL_ADJ_1[oppEleLoc];
  wasLoose = false;
  if(!isBlockadeRec(b,ufDist,pla,oppEleLoc,oppEleLoc,nextloc,allowRecheck,recursedMap,holderHeldMap,freezeHeldMap,true,wasLoose)) return false;
  isEverLoose |= wasLoose;

  //Create temporaries so that we dont add fake holders to isBlockadeRecs that return false
  Bitmap allowRecheckTemp = allowRecheck;
  Bitmap recursedMapTemp = recursedMap;
  Bitmap holderHeldMapTemp = holderHeldMap;
  Bitmap freezeHeldMapTemp = freezeHeldMap;

  nextloc = CENTRAL_ADJ_2[oppEleLoc];
  wasLoose = false;
  if(!isBlockadeRec(b,ufDist,pla,oppEleLoc,oppEleLoc,nextloc,allowRecheckTemp,recursedMapTemp,holderHeldMapTemp,freezeHeldMapTemp,true,wasLoose))
  {
    allowRecheckTemp = allowRecheck; recursedMapTemp = recursedMap; holderHeldMapTemp = holderHeldMap; freezeHeldMapTemp = freezeHeldMap; //Restore
    immoType = CENTRAL_BLOCK;
  }
  else
  {allowRecheck = allowRecheckTemp; recursedMap = recursedMapTemp; holderHeldMap = holderHeldMapTemp; freezeHeldMap = freezeHeldMapTemp; isEverLoose |= wasLoose;} //Save

  nextloc = EDGE_ADJ_1[oppEleLoc];
  wasLoose = false;
  if(nextloc != oppEleLoc)
  {
    if(!isBlockadeRec(b,ufDist,pla,oppEleLoc,oppEleLoc,nextloc,allowRecheckTemp,recursedMapTemp,holderHeldMapTemp,freezeHeldMapTemp,true,wasLoose))
    {
      //Both left and right were unblocked, so we can't have any sort of immo.
      if(immoType == CENTRAL_BLOCK)
      {
        //Actually, let's still catch a weak block-ish situation
        //At least one of these surrounding locs is blocked, and also only in certain locations
        if(WEAK_CENTRAL_BLOCK_OK[oppEleLoc] != 0 &&
            ((b.owners[CENTRAL_ADJ_2[oppEleLoc]] != NPLA && b.isBlocked(CENTRAL_ADJ_2[oppEleLoc])) ||
             (b.owners[nextloc] != NPLA && b.isBlocked(nextloc))))
        {
          immoType = CENTRAL_BLOCK_WEAK;
          return true;
        }
        //allowRecheckTemp = allowRecheck; recursedMapTemp = recursedMap; holderHeldMapTemp = holderHeldMap; freezeHeldMapTemp = freezeHeldMap; //Restore
        return false;
      }
      immoType = CENTRAL_BLOCK;
      return true;
    }
    else
    {allowRecheck = allowRecheckTemp; recursedMap = recursedMapTemp; holderHeldMap = holderHeldMapTemp; freezeHeldMap = freezeHeldMapTemp;  isEverLoose |= wasLoose;} //Save
  }

  if(immoType == CENTRAL_BLOCK)
    return true;


  nextloc = EDGE_ADJ_2[oppEleLoc];
  wasLoose = false;
  if(nextloc != oppEleLoc)
  {
    if(!isBlockadeRec(b,ufDist,pla,oppEleLoc,oppEleLoc,nextloc,allowRecheckTemp,recursedMapTemp,holderHeldMapTemp,freezeHeldMapTemp,true,wasLoose))
    {
      //allowRecheckTemp = allowRecheck; recursedMapTemp = recursedMap; holderHeldMapTemp = holderHeldMap; freezeHeldMapTemp = freezeHeldMap; //Restore
      immoType = ALMOST_IMMO;
      return true;
    }
    else
    {/*allowRecheck = allowRecheckTemp;*/ recursedMap = recursedMapTemp; holderHeldMap = holderHeldMapTemp; freezeHeldMap = freezeHeldMapTemp; isEverLoose |= wasLoose;} //Save
  }

  if(immoType == TOTAL_IMMO && isEverLoose)
    immoType = ALMOST_IMMO;
  return true;
}

loc_t Strats::findEBlockade(Board& b, pla_t pla, const int ufDist[64], int& immoType, Bitmap& recursedMap, Bitmap& holderHeldMap, Bitmap& freezeHeldMap)
{
  pla_t opp = OPP(pla);
  Bitmap eMap = b.pieceMaps[opp][ELE];
  if(eMap.hasBits())
  {
    loc_t loc = eMap.nextBit();
    if(isEleBlockade(b, ufDist, pla, loc, recursedMap, holderHeldMap, freezeHeldMap, immoType))
    {
      Bitmap anyMap = b.pieceMaps[SILV][0] | b.pieceMaps[GOLD][0];
      holderHeldMap &= anyMap; //Remove empty squares
      return loc;
    }
  }
  return ERRORSQUARE;
}



//Value for building a phalanx temporarily against
//elephant
/*
static const int E_LR_PHALANX_VAL[64] =
{
2700,2500,2000, 300, 300,2000,2500,2700,
2200,2000,1800, 300, 300,1800,2000,2200,
1900,1600, 300, 300, 300, 300,1600,1900,
1700, 900, 700, 300, 300, 700, 900,1700,
1700, 900, 700, 300, 300, 700, 900,1700,
1900,1600, 300, 300, 300, 300,1600,1900,
2200,2000,1800, 300, 300,1800,2000,2200,
2700,2500,2000, 300, 300,2000,2500,2700,
};
static const int E_UD_PHALANX_VAL[64] =
{
2000,2000,2000,1800,1800,2000,2000,2000,
1500,1500,1000,1500,1500,1000,1500,1500,
1900,1600, 300,1200,1200, 300,1600,1900,
 300, 300, 300, 300, 300, 300, 300, 300,
 300, 300, 300, 300, 300, 300, 300, 300,
1900,1600, 300,1000,1000, 300,1600,1900,
1500,1500,1000,1500,1500,1000,1500,1500,
2000,2000,2000,1800,1800,2000,2000,2000,
};
*/


/*
//TODO testing
Bitmap recursedMap;
Bitmap holderHeldMap;
Bitmap freezeHeldMap;
int immoType;
if(isEleBlockade(b, ufDist, pla, loc, recursedMap, holderHeldMap, freezeHeldMap, immoType))
{
  cout << "IMMO " << immoType  << endl;
  cout << "Recursed/Interior" << endl;
  cout << recursedMap << endl;
  cout << "HolderHeld" << endl;
  cout << holderHeldMap << endl;
  cout << "Freezeheld" << endl;
  cout << freezeHeldMap << endl;
}
*/



















