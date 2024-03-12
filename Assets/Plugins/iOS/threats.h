
/*
 * threats.h
 * Author: davidwu
 */

#ifndef THREATS_H
#define THREATS_H

#include <iostream>
#include "bitmap.h"
#include "board.h"

class CapThreat;

namespace Threats
{
	//Fill array with goal distance estimate
	//TODO Not exactly, since a pla piece counts as zero if unfrozen, because pla pieces tend to help
	//more than empty spots.
	//void getGoalDistEst(const Board& b, pla_t pla, int goalDist[64], int max);

	//Distance to walk adjacent to dest remaining unfrozen, assumes ploc is not rabbit
	int traverseAdjUfDist(const Board& b, pla_t pla, loc_t ploc, loc_t dest, const int ufDist[64], int maxDist);

	extern const int ZEROS[64];
	int traverseDist(const Board& b, loc_t ploc, loc_t dest, bool endUF,
			const int ufDist[64], int maxDist, const int* bonusVals = ZEROS);

	//Dist to move a piece stronger than strongerThan adjacent and unfrozen
	int moveAdjDistUF(const Board& b, pla_t pla, loc_t loc, int maxDist,
			const int ufDist[64], const Bitmap pStrongerMaps[2][NUMTYPES] = NULL, piece_t strongerThan = EMP);

	//Dist for opp to threaten ploc (adjacent UF stronger piece)
	int attackDist(const Board& b, loc_t ploc, int maxDist,
			const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64]);

	//Dist for pla to occupy loc
	//Not quite accurate - disregards freezing at the end
	int occupyDist(const Board& b, pla_t pla, loc_t loc, int maxDist,
			const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64]);

	//Threats BY pla
	const int maxCapsPerPla = 128;
	int findCapThreats(Board& b, pla_t pla, loc_t kt, CapThreat* threats, const int ufDist[64],
			const Bitmap pStronger[2][NUMTYPES], int maxCapDist, int maxThreatLen, int& rdSteps);
}

class CapThreat
{
  public:
  unsigned char dist;     //Number of steps required for capture
  loc_t oloc;             //The capturable piece
  loc_t kt;               //The trap used for capture
  loc_t ploc;             //The capturing piece or ERRORSQUARE if this is a sacrifical cap.
  unsigned char baseDist; //The original number of steps required for capture, pre-defense modification

  inline CapThreat()
  :dist(0),oloc(0),kt(0),ploc(0),baseDist(0)
  {}

  inline CapThreat(unsigned char dist, loc_t oloc, loc_t kt, loc_t ploc)
  :dist(dist),oloc(oloc),kt(kt),ploc(ploc),baseDist(dist)
  {}

  inline friend ostream& operator<<(ostream& out, const CapThreat& t)
  {
    out << "Cap " << (int)t.oloc << " by " << (int)t.ploc << " at " << (int)t.kt << " in " << (int)t.dist << " base " << (int)t.baseDist;
    return out;
  }

};

#endif
