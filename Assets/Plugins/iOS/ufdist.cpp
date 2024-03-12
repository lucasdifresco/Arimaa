/*
 * ufdist.cpp
 * Author: davidwu
 */
#include "pch.h"
#include <iostream>
#include <cmath>
#include "global.h"
#include "bitmap.h"
#include "board.h"
#include "boardtreeconst.h"
#include "ufdist.h"


static const int IMMOFROZENCOST_N = 2; //If no good squares
static const int IMMOCOST_N = 1; //Always

//FOR USE IN UF ONLY!
//Check if a piece is immobilized just by blockading. Includes blocking in by self pieces, but only 2 or more layers deep
static bool isImmo(const Board& b, pla_t pla, loc_t ploc)
{
  pla_t opp = OPP(pla);
  return
  !(
    (CS1(ploc) && b.isRabOkayS(pla,ploc) && (b.owners[ploc-8] == NPLA || (b.owners[ploc-8] == pla && b.isOpenToStep(ploc-8) && b.wouldBeUF(pla,ploc,ploc,ploc-8)) || (b.owners[ploc-8] == opp && b.isOpen(ploc-8) && b.pieces[ploc-8] < b.pieces[ploc])) && b.isTrapSafe2(pla,ploc-8)) ||
    (CW1(ploc)                           && (b.owners[ploc-1] == NPLA || (b.owners[ploc-1] == pla && b.isOpenToStep(ploc-1) && b.wouldBeUF(pla,ploc,ploc,ploc-1)) || (b.owners[ploc-1] == opp && b.isOpen(ploc-1) && b.pieces[ploc-1] < b.pieces[ploc])) && b.isTrapSafe2(pla,ploc-1)) ||
    (CE1(ploc)                           && (b.owners[ploc+1] == NPLA || (b.owners[ploc+1] == pla && b.isOpenToStep(ploc+1) && b.wouldBeUF(pla,ploc,ploc,ploc+1)) || (b.owners[ploc+1] == opp && b.isOpen(ploc+1) && b.pieces[ploc+1] < b.pieces[ploc])) && b.isTrapSafe2(pla,ploc+1)) ||
    (CN1(ploc) && b.isRabOkayN(pla,ploc) && (b.owners[ploc+8] == NPLA || (b.owners[ploc+8] == pla && b.isOpenToStep(ploc+8) && b.wouldBeUF(pla,ploc,ploc,ploc+8)) || (b.owners[ploc+8] == opp && b.isOpen(ploc+8) && b.pieces[ploc+8] < b.pieces[ploc])) && b.isTrapSafe2(pla,ploc+8))
  );
}

//FOR USE IN UF ONLY!
//Tests if only one or zero moveable locations where, if unfrozen (or remaining unfrozen) with this location open,
//the piece could move and marks it.
//True + errorsquare means will always take a lot longer to UF
//True + Loc being selected means "this location MUST remain open or else it will take a lot longer to UF".
//False means can make anywhere open and it's fine.

//FOR USE IN UF ONLY!
//Obtains the status of a thawed but immo piece
static int UF_NEED_DEFENDER = 1; //Need someone to come along and UF. loc = UFing here is no help
static int UF_NEED_SPACE = 2; //Need to open out space to step. loc = Opening space here is no help
static int UF_NEED_SPACE_AND_DEFENDER = 3; //Need both open space and a new defender

static int getThawedImmoUFStatus(const Board& b, pla_t pla, loc_t ploc, loc_t& badloc)
{
  badloc = ERRORSQUARE;

  //If nothing else happens, the only possible problem is rabbits not being able to use a cleared space behind them
  if(b.pieces[ploc] == RAB)
    badloc = (pla == GOLD && CS1(ploc)) ? ploc-8 : (pla == SILV && CN1(ploc)) ? ploc+8 : ERRORSQUARE;

  //Find defenders and count dominators
  pla_t opp = OPP(pla);
  loc_t defloc = ERRORSQUARE;
  int numDom = 0;
  if(CS1(ploc)) {
    if(b.owners[ploc-8] == pla){if(defloc != ERRORSQUARE) return UF_NEED_SPACE; else defloc = ploc-8;}
    else if(b.owners[ploc-8] == opp && b.pieces[ploc-8] > b.pieces[ploc]){numDom++;}
  }
  if(CW1(ploc)) {
    if(b.owners[ploc-1] == pla){if(defloc != ERRORSQUARE) return UF_NEED_SPACE; else defloc = ploc-1;}
    else if(b.owners[ploc-1] == opp && b.pieces[ploc-1] > b.pieces[ploc]){numDom++;}
  }
  if(CE1(ploc)) {
    if(b.owners[ploc+1] == pla){if(defloc != ERRORSQUARE) return UF_NEED_SPACE; else defloc = ploc+1;}
    else if(b.owners[ploc+1] == opp && b.pieces[ploc+1] > b.pieces[ploc]){numDom++;}
  }
  if(CN1(ploc)) {
    if(b.owners[ploc+8] == pla){if(defloc != ERRORSQUARE) return UF_NEED_SPACE; else defloc = ploc+8;}
    else if(b.owners[ploc+8] == opp && b.pieces[ploc+8] > b.pieces[ploc]){numDom++;}
  }

  //Not dominated
  if(numDom == 0)
    return UF_NEED_SPACE;

  //Dominated, and only one defender, so we can't move that defender away
  badloc = defloc;
  return UF_NEED_SPACE;
}

//Obtains the status of a frozen piece
static int getFrozenUFStatus(const Board& b, pla_t pla, loc_t ploc, loc_t& badloc)
{
  badloc = ERRORSQUARE;

  pla_t opp = OPP(pla);
  loc_t escapeloc = ERRORSQUARE;
  if(CS1(ploc) && b.isRabOkayS(pla,ploc) && (b.owners[ploc-8] == NPLA || (b.owners[ploc-8] == opp && b.pieces[ploc-8] < b.pieces[ploc] && b.isOpen(ploc-8)) || (b.owners[ploc-8] == opp && b.isDominatedByUF(ploc-8))) && b.isTrapSafe2(pla,ploc-8)) {if(escapeloc != ERRORSQUARE) return UF_NEED_DEFENDER; else escapeloc = ploc-8;}
  if(CW1(ploc)                           && (b.owners[ploc-1] == NPLA || (b.owners[ploc-1] == opp && b.pieces[ploc-1] < b.pieces[ploc] && b.isOpen(ploc-1)) || (b.owners[ploc-1] == opp && b.isDominatedByUF(ploc-1))) && b.isTrapSafe2(pla,ploc-1)) {if(escapeloc != ERRORSQUARE) return UF_NEED_DEFENDER; else escapeloc = ploc-1;}
  if(CE1(ploc)                           && (b.owners[ploc+1] == NPLA || (b.owners[ploc+1] == opp && b.pieces[ploc+1] < b.pieces[ploc] && b.isOpen(ploc+1)) || (b.owners[ploc+1] == opp && b.isDominatedByUF(ploc+1))) && b.isTrapSafe2(pla,ploc+1)) {if(escapeloc != ERRORSQUARE) return UF_NEED_DEFENDER; else escapeloc = ploc+1;}
  if(CN1(ploc) && b.isRabOkayN(pla,ploc) && (b.owners[ploc+8] == NPLA || (b.owners[ploc+8] == opp && b.pieces[ploc+8] < b.pieces[ploc] && b.isOpen(ploc+8)) || (b.owners[ploc+8] == opp && b.isDominatedByUF(ploc+8))) && b.isTrapSafe2(pla,ploc+8)) {if(escapeloc != ERRORSQUARE) return UF_NEED_DEFENDER; else escapeloc = ploc+8;}

  //Nowhere we can even escape to - we are hosed.
  if(escapeloc == ERRORSQUARE)
    return UF_NEED_SPACE_AND_DEFENDER;

  //Only one space to escape to, so we can't UF from here.
  badloc = escapeloc;
  return UF_NEED_DEFENDER;
}

//Compute best uf distance for pieces within radius 2. Min = min dist that we care about - return if equal or smaller
//Max = max dist we care about, return if it must be equal or larger. minNewUfDist - assume ufDists are >= this.
static int ufRad2(const Board& b, pla_t pla, loc_t ploc, const int ufDist[64], int min, int max, int minNewUfDist, loc_t badLoc, int ufStatus)
{
  int best = UFDist::MAX_UF_DIST;

  if(ufStatus == UF_NEED_SPACE)
  {
    if(CS1(ploc) && ploc-8 != badLoc && b.owners[ploc-8] == pla && best > ufDist[ploc-8]+1) {best = ufDist[ploc-8]+1; if(best <= min) return best;}
    if(CW1(ploc) && ploc-1 != badLoc && b.owners[ploc-1] == pla && best > ufDist[ploc-1]+1) {best = ufDist[ploc-1]+1; if(best <= min) return best;}
    if(CE1(ploc) && ploc+1 != badLoc && b.owners[ploc+1] == pla && best > ufDist[ploc+1]+1) {best = ufDist[ploc+1]+1; if(best <= min) return best;}
    if(CN1(ploc) && ploc+8 != badLoc && b.owners[ploc+8] == pla && best > ufDist[ploc+8]+1) {best = ufDist[ploc+8]+1; if(best <= min) return best;}
  }

  if((b.pieceMaps[pla][0] & Board::DISK[2][ploc] & (~Board::DISK[0][ploc])).isEmpty())
    return best;

  pla_t opp = OPP(pla);
  for(int i = 0; i<4; i++)
  {
    if(!Board::ADJOKAY[i][ploc])
      continue;
    loc_t loc = ploc + Board::ADJOFFSETS[i];
    int extra = loc == badLoc ? IMMOFROZENCOST_N : 0;
    if(b.owners[loc] == NPLA)
    {
      if(ufStatus == UF_NEED_SPACE)
        extra++;
      if(minNewUfDist + extra + 1 >= max)
        continue;
      if(CS1(loc) && loc S != ploc && ISP(loc S) && b.isRabOkayN(pla,loc S)) {if(best > ufDist[loc S]+1+extra) {best = ufDist[loc S]+1+extra; if(best <= min) return best;}}
      if(CW1(loc) && loc W != ploc && ISP(loc W)                           ) {if(best > ufDist[loc W]+1+extra) {best = ufDist[loc W]+1+extra; if(best <= min) return best;}}
      if(CE1(loc) && loc E != ploc && ISP(loc E)                           ) {if(best > ufDist[loc E]+1+extra) {best = ufDist[loc E]+1+extra; if(best <= min) return best;}}
      if(CN1(loc) && loc N != ploc && ISP(loc N) && b.isRabOkayS(pla,loc N)) {if(best > ufDist[loc N]+1+extra) {best = ufDist[loc N]+1+extra; if(best <= min) return best;}}
    }
    else if(b.owners[loc] == opp)
    {
      if(b.wouldBeUF(pla,ploc,loc))
        extra = 0;
      else if(!b.isOpen(loc))
        continue;

      if(minNewUfDist + extra + 2 >= max)
        continue;
      if(CS1(loc) && ISP(loc S) && GT(loc S, loc)) {if(best > ufDist[loc S]+2) {best = ufDist[loc S]+2+extra; if(best <= min) return best;}}
      if(CW1(loc) && ISP(loc W) && GT(loc W, loc)) {if(best > ufDist[loc W]+2) {best = ufDist[loc W]+2+extra; if(best <= min) return best;}}
      if(CE1(loc) && ISP(loc E) && GT(loc E, loc)) {if(best > ufDist[loc E]+2) {best = ufDist[loc E]+2+extra; if(best <= min) return best;}}
      if(CN1(loc) && ISP(loc N) && GT(loc N, loc)) {if(best > ufDist[loc N]+2) {best = ufDist[loc N]+2+extra; if(best <= min) return best;}}
    }
  }
  return best;
}

static int ufRad3(const Board& b, pla_t pla, loc_t ploc, const int ufDist[64], int min, int max, int minNewUfDist, loc_t badLoc, int ufStatus)
{
  if((b.pieceMaps[pla][0] & Board::DISK[3][ploc] & (~Board::DISK[0][ploc])).isEmpty())
    return UFDist::MAX_UF_DIST;

  int best = UFDist::MAX_UF_DIST;
  pla_t opp = OPP(pla);
  for(int i = 0; i<4; i++)
  {
    if(!Board::ADJOKAY[i][ploc])
      continue;
    loc_t loc = ploc + Board::ADJOFFSETS[i];
    int ifextra = loc == badLoc ? IMMOFROZENCOST_N : 0;

    if(b.owners[loc] == NPLA)
    {
      if(ufStatus == UF_NEED_SPACE)
        ifextra++;

      for(int j = 0; j<4; j++)
      {
        if(!Board::ADJOKAY[j][loc])
          continue;
        loc_t loc2 = loc + Board::ADJOFFSETS[j];
        if(loc2 == ploc)
          continue;

        if(b.owners[loc2] == NPLA)
        {
          int extra = 2;
          if(!b.isTrapSafe2(pla,loc2))
          {if(b.trapGuardCounts[pla][Board::TRAPINDEX[loc2]] == 0) {extra += 2;} else {extra += 1;}}
          if(minNewUfDist + extra + ifextra >= max)
            continue;

          bool allowRabbit = b.isRabOkay(pla,loc2,loc);
          if(CS1(loc2) && loc2 S != loc && ISP(loc2 S) && b.isRabOkayN(pla,loc2 S))               {int extra2 = b.wouldBeUF(pla,loc2 S,loc2,loc2 S) ? 0 : 1; if(best > ufDist[loc2 S]+extra+extra2+ifextra) {best = ufDist[loc2 S]+extra+extra2+ifextra; if(best <= min) return best;}}
          if(CW1(loc2) && loc2 W != loc && ISP(loc2 W) && (allowRabbit || b.pieces[loc2] != RAB)) {int extra2 = b.wouldBeUF(pla,loc2 W,loc2,loc2 W) ? 0 : 1; if(best > ufDist[loc2 W]+extra+extra2+ifextra) {best = ufDist[loc2 W]+extra+extra2+ifextra; if(best <= min) return best;}}
          if(CE1(loc2) && loc2 E != loc && ISP(loc2 E) && (allowRabbit || b.pieces[loc2] != RAB)) {int extra2 = b.wouldBeUF(pla,loc2 E,loc2,loc2 E) ? 0 : 1; if(best > ufDist[loc2 E]+extra+extra2+ifextra) {best = ufDist[loc2 E]+extra+extra2+ifextra; if(best <= min) return best;}}
          if(CN1(loc2) && loc2 N != loc && ISP(loc2 N) && b.isRabOkayS(pla,loc2 N))               {int extra2 = b.wouldBeUF(pla,loc2 N,loc2,loc2 N) ? 0 : 1; if(best > ufDist[loc2 N]+extra+extra2+ifextra) {best = ufDist[loc2 N]+extra+extra2+ifextra; if(best <= min) return best;}}
        }
        else if(b.owners[loc2] == opp && b.isTrapSafe2(pla,loc2))
        {
          if(minNewUfDist + 3 + ifextra >= max)
            continue;
          if(CS1(loc2) && loc2 S != loc && ISP(loc2 S) && GT(loc2 S, loc2)) {if(best > ufDist[loc2 S]+3+ifextra) {best = ufDist[loc2 S]+3+ifextra; if(best <= min) return best;}}
          if(CW1(loc2) && loc2 W != loc && ISP(loc2 W) && GT(loc2 W, loc2)) {if(best > ufDist[loc2 W]+3+ifextra) {best = ufDist[loc2 W]+3+ifextra; if(best <= min) return best;}}
          if(CE1(loc2) && loc2 E != loc && ISP(loc2 E) && GT(loc2 E, loc2)) {if(best > ufDist[loc2 E]+3+ifextra) {best = ufDist[loc2 E]+3+ifextra; if(best <= min) return best;}}
          if(CN1(loc2) && loc2 N != loc && ISP(loc2 N) && GT(loc2 N, loc2)) {if(best > ufDist[loc2 N]+3+ifextra) {best = ufDist[loc2 N]+3+ifextra; if(best <= min) return best;}}
        }
      }
    }
    else if(b.owners[loc] == opp)
    {
      if(b.wouldBeUF(pla,ploc,loc))
        ifextra = 0;
      else if(!b.isOpen2(loc))
        continue;

      if(minNewUfDist + 3 + ifextra >= max)
        continue;
      for(int j = 0; j<4; j++)
      {
        if(!Board::ADJOKAY[j][loc])
          continue;
        loc_t loc2 = loc + Board::ADJOFFSETS[j];
        if(loc2 == ploc)
          continue;
        if(b.owners[loc2] == NPLA && b.isTrapSafe2(pla,loc2))
        {
          if(CS1(loc2) && loc2 S != loc && ISP(loc2 S) && GT(loc2 S, loc)) {int extra2 = b.wouldBeUF(pla,loc2 S,loc2,loc2 S) ? 0 : 1; if(best > ufDist[loc2 S]+3+extra2+ifextra) {best = ufDist[loc2 S]+3+extra2+ifextra; if(best <= min) return best;}}
          if(CW1(loc2) && loc2 W != loc && ISP(loc2 W) && GT(loc2 W, loc)) {int extra2 = b.wouldBeUF(pla,loc2 W,loc2,loc2 W) ? 0 : 1; if(best > ufDist[loc2 W]+3+extra2+ifextra) {best = ufDist[loc2 W]+3+extra2+ifextra; if(best <= min) return best;}}
          if(CE1(loc2) && loc2 E != loc && ISP(loc2 E) && GT(loc2 E, loc)) {int extra2 = b.wouldBeUF(pla,loc2 E,loc2,loc2 E) ? 0 : 1; if(best > ufDist[loc2 E]+3+extra2+ifextra) {best = ufDist[loc2 E]+3+extra2+ifextra; if(best <= min) return best;}}
          if(CN1(loc2) && loc2 N != loc && ISP(loc2 N) && GT(loc2 N, loc)) {int extra2 = b.wouldBeUF(pla,loc2 N,loc2,loc2 N) ? 0 : 1; if(best > ufDist[loc2 N]+3+extra2+ifextra) {best = ufDist[loc2 N]+3+extra2+ifextra; if(best <= min) return best;}}
        }
      }
    }
  }
  return best;
}

static int ufRad4(const Board& b, pla_t pla, loc_t ploc, const int ufDist[64], int min, int max, int minNewUfDist, loc_t badLoc, int ufStatus)
{
  if((b.pieceMaps[pla][0] & Board::DISK[4][ploc] & (~Board::DISK[0][ploc])).isEmpty())
    return UFDist::MAX_UF_DIST;

  pla_t opp = OPP(pla);
  int best = UFDist::MAX_UF_DIST;
  for(int i = 0; i<4; i++)
  {
    if(!Board::ADJOKAY[i][ploc])
      continue;
    loc_t loc = ploc + Board::ADJOFFSETS[i];
    int ifextra = loc == badLoc ? IMMOFROZENCOST_N : 0;
    if(b.owners[loc] == pla) continue;
    piece_t strReq = EMP;
    int extra = 3;

    if(b.owners[loc] == opp)
    {strReq = b.pieces[loc]+1; extra++;}
    else if(ufStatus == UF_NEED_SPACE && b.owners[loc] == NPLA)
      extra++;

    if(minNewUfDist + extra + ifextra >= max)
      continue;

    for(int j = 0; j<4; j++)
    {
      if(!Board::ADJOKAY[j][loc])
        continue;
      loc_t loc2 = loc + Board::ADJOFFSETS[j];
      if(loc2 == ploc || b.owners[loc2] != NPLA || !b.isTrapSafe1(pla,loc2))
        continue;
      bool allowRabbit = b.isRabOkay(pla,loc2,loc);

      for(int k = 0; k<4; k++)
      {
        if(!Board::ADJOKAY[k][loc2])
          continue;
        loc_t loc3 = loc2 + Board::ADJOFFSETS[k];
        if(loc3 == loc || b.owners[loc3] != NPLA || !b.isTrapSafe2(pla,loc3))
          continue;
        bool allowRabbit2 = allowRabbit && b.isRabOkay(pla,loc3,loc2);

        if(CS1(loc3) && loc3 S != loc2 && ISP(loc3 S) && b.pieces[loc3 S] >= strReq && b.isRabOkayN(pla,loc3 S)                  && b.wouldBeUF(pla,loc3 S,loc3,loc3 S) && b.wouldBeUF(pla,loc3 S,loc2)) {if(best > ufDist[loc3 S]+extra+ifextra) {best = ufDist[loc3 S]+extra+ifextra; if(best <= min) return best;}}
        if(CW1(loc3) && loc3 W != loc2 && ISP(loc3 W) && b.pieces[loc3 W] >= strReq && (allowRabbit2 || b.pieces[loc3 W] != RAB) && b.wouldBeUF(pla,loc3 W,loc3,loc3 W) && b.wouldBeUF(pla,loc3 W,loc2)) {if(best > ufDist[loc3 W]+extra+ifextra) {best = ufDist[loc3 W]+extra+ifextra; if(best <= min) return best;}}
        if(CE1(loc3) && loc3 E != loc2 && ISP(loc3 E) && b.pieces[loc3 E] >= strReq && (allowRabbit2 || b.pieces[loc3 E] != RAB) && b.wouldBeUF(pla,loc3 E,loc3,loc3 E) && b.wouldBeUF(pla,loc3 E,loc2)) {if(best > ufDist[loc3 E]+extra+ifextra) {best = ufDist[loc3 E]+extra+ifextra; if(best <= min) return best;}}
        if(CN1(loc3) && loc3 N != loc2 && ISP(loc3 N) && b.pieces[loc3 N] >= strReq && b.isRabOkayS(pla,loc3 N)                  && b.wouldBeUF(pla,loc3 N,loc3,loc3 N) && b.wouldBeUF(pla,loc3 N,loc2)) {if(best > ufDist[loc3 N]+extra+ifextra) {best = ufDist[loc3 N]+extra+ifextra; if(best <= min) return best;}}
      }
    }
  }
  return best;
}

static void solveUFDist(const Board& b, pla_t pla, int ufDist[64])
{
  Bitmap pieces = b.pieceMaps[pla][0];
  Bitmap frozenPieces = pieces & b.frozenMap;
  Bitmap thawedPieces = pieces & (~b.frozenMap);

  //Track the pieces that are frozen and their dists from unfreezing
  loc_t frozenLocs[16];
  loc_t badLoc[16];
  int ufStatus[16];
  int dists[16];
  int frozenCount = 0;

  //Mark all thawed pieces, except the immo ones
  while(thawedPieces.hasBits())
  {
    loc_t ploc = thawedPieces.nextBit();
    if(isImmo(b,pla,ploc))
    {
      frozenLocs[frozenCount] = ploc;
      ufStatus[frozenCount] = getThawedImmoUFStatus(b,pla,ploc,badLoc[frozenCount]);
      dists[frozenCount] = UFDist::MAX_UF_DIST;
      ufDist[ploc] = UFDist::MAX_UF_DIST;
      frozenCount++;
    }
    else
      ufDist[ploc] = 0;
  }

  while(frozenPieces.hasBits())
  {
    loc_t ploc = frozenPieces.nextBit();
    frozenLocs[frozenCount] = ploc;
    ufStatus[frozenCount] = getFrozenUFStatus(b,pla,ploc,badLoc[frozenCount]);
    dists[frozenCount] = UFDist::MAX_UF_DIST;
    ufDist[ploc] = UFDist::MAX_UF_DIST;
    frozenCount++;
  }

  //Iterate in rounds of unfreezing calcs
  //Round 1
  int newCount = 0;
  for(int i = 0; i<frozenCount; i++)
  {
    loc_t ploc = frozenLocs[i];
    //TODO should the penalty be 2 : 0? Because if you have only 1 space (badLoc), it costs 2 to use the badloc
    //but if you have 0 spaces, it costs only 1, from every direction?
    int extraCost = (ufStatus[i] == UF_NEED_SPACE_AND_DEFENDER) ? 1 : 0;

    int dist = ufRad2(b,pla,ploc,ufDist,1,UFDist::MAX_UF_DIST,0,badLoc[i],ufStatus[i]) + extraCost;
    ufDist[ploc] = dist;
    if(dist > 2)
    {
      frozenLocs[newCount] = ploc;
      ufStatus[newCount] = ufStatus[i];
      badLoc[newCount] = badLoc[i];
      dists[newCount] = dist;
      newCount++;
    }
  }
  frozenCount = newCount;
  //Round 2
  newCount = 0;
  for(int i = 0; i<frozenCount; i++)
  {
    loc_t ploc = frozenLocs[i];
    int dist = dists[i];
    int extraCost = (ufStatus[i] == UF_NEED_SPACE_AND_DEFENDER) ? 1 : 0;

    int newDist = ufRad2(b,pla,ploc,ufDist,2,dist,1,badLoc[i],ufStatus[i]) + extraCost;
    if(newDist < dist)
    {dist = newDist; ufDist[ploc] = newDist; if(newDist <= 3) continue;}

    newDist = ufRad3(b,pla,ploc,ufDist,2,dist,0,badLoc[i],ufStatus[i]) + extraCost;
    if(newDist < dist)
    {dist = newDist; ufDist[ploc] = newDist; if(newDist <= 3) continue;}

    frozenLocs[newCount] = ploc;
    ufStatus[newCount] = ufStatus[i];
    badLoc[newCount] = badLoc[i];
    dists[newCount] = dist;
    newCount++;
  }
  frozenCount = newCount;
  //Round 3
  newCount = 0;
  for(int i = 0; i<frozenCount; i++)
  {
    loc_t ploc = frozenLocs[i];
    int dist = dists[i];
    int extraCost = (ufStatus[i] == UF_NEED_SPACE_AND_DEFENDER) ? 1 : 0;

    int newDist = ufRad2(b,pla,ploc,ufDist,3,dist,2,badLoc[i],ufStatus[i]) + extraCost;
    if(newDist < dist)
    {dist = newDist; ufDist[ploc] = newDist; if(newDist <= 4) continue;}

    newDist = ufRad3(b,pla,ploc,ufDist,3,dist,1,badLoc[i],ufStatus[i]) + extraCost;
    if(newDist < dist)
    {dist = newDist; ufDist[ploc] = newDist; if(newDist <= 4) continue;}

    newDist = ufRad4(b,pla,ploc,ufDist,3,dist,0,badLoc[i],ufStatus[i]) + extraCost;
    if(newDist < dist)
    {dist = newDist; ufDist[ploc] = newDist; if(newDist <= 4) continue;}

    frozenLocs[newCount] = ploc;
    ufStatus[newCount] = ufStatus[i];
    badLoc[newCount] = badLoc[i];
    dists[newCount] = dist;
    newCount++;
  }
}

void UFDist::get(const Board& b, int ufDist[64])
{
  solveUFDist(b,GOLD,ufDist);
  solveUFDist(b,SILV,ufDist);
}


