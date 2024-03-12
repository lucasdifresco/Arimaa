
/*
 * evalstrats.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <cmath>
#include <cstdio>
#include "global.h"
#include "board.h"
#include "eval.h"
#include "threats.h"
#include "strats.h"
#include "searchparams.h"

//SHARED------------------------------------------------------------------------------------

static void copyPieceCounts(const Board& b, pla_t pla, double* plaPieceCounts, double* oppPieceCounts)
{
  pla_t opp = OPP(pla);
  for(int piece = ELE; piece > EMP; piece--)
  {
    plaPieceCounts[piece] = b.pieceCounts[pla][piece];
    oppPieceCounts[piece] = b.pieceCounts[opp][piece];
  }
}

/**
 * Get a smooth logistic weighting for scale. As prop increases from zero to infinity, the return value of this function
 * will increase from zero and appoach scale. At prop = total, the return value will be around 70 percentish of scale.
 */
static int computeLogistic(double prop, double total, int scale)
{
  return (int)(scale * (-1.0 + 2.0 / (1.0 + exp(-2.0 * prop/total))));
}

//SFP------------------------------------------------------------------------------

//Constants for computing SFP
static const double SFP_TIER_VAL[10] =
{500,140,50,20,10,5,2,1,1,1};

static const double SFP_TIER_PROP[10][10] = {
{  0,-100,-200,-290,-370,-440,-500,-550,-590,-620},
{100,   0, -50,-120,-190,-250,-300,-340,-370,-390},
{200,  50,   0, -20, -70,-120,-160,-200,-230,-220},
{290, 120,  20,   0, -10, -40, -80,-110,-140,-160},
{370, 190,  70,  10,   0,  -5, -25, -60, -90,-120},
{440, 250, 120,  40,   5,   0,  -2, -20, -50, -70},
{500, 300, 160,  80,  25,   2,   0,  -1, -15, -40},
{550, 340, 200, 110,  60,  15,   1,   0,  -1, -10},
{590, 370, 230, 140, 100,  50,  15,   1,   0,   0},
{620, 390, 220, 160, 120,  70,  40,  10,   0,   0},
};

//inline static double max(double d0, double d1) { return d0 > d1 ? d0 : d1; }

static int getSFPTierScore(double tier, double pcount, double ocount)
{
  int t0 = (int)tier;
  int t1 = t0+1;
  double tprop = tier - t0;
  int pc0 = (int)pcount;
  int pc1 = pc0+1;
  double pcprop = pcount - pc0;
  int oc0 = (int)ocount;
  int oc1 = oc0+1;
  double ocprop = ocount - oc0;

  double stprop =
  (SFP_TIER_PROP[pc0][oc0]*(1-ocprop) + SFP_TIER_PROP[pc0][oc1]*(ocprop))*(1-pcprop) +
  (SFP_TIER_PROP[pc1][oc0]*(1-ocprop) + SFP_TIER_PROP[pc1][oc1]*(ocprop))*(pcprop);
  double stval = SFP_TIER_VAL[t0]*(1-tprop) + SFP_TIER_VAL[t1]*(tprop);

  return (int)round(stprop*stval/100.0);
}

//Factor to adjust by depending on relation to penalty piece (myPiece-penaltyPiece+6)
static const double ADVGOODDIFFTABLE[13] = {3.0,3.0,3.0,2.9,2.8,2.5,1.0,0.5,0.35,0.18,0.10,0.10,0.10};

/**
 * Get the material according to SFP.
 * @param b - the board, for debuggingpurposes
 * @param pieceCountsPla - the counts (possibly fractional) of pieces pla has
 * @param pieceCountsOpp - the counts (possibly fractional) of pieces opp has
 * @return the material score
 */
static int computeSFPScore(const double* pieceCountsPla, const double* pieceCountsOpp,
		double* advancementGood = NULL, piece_t breakPenaltyPiece = EMP);

static int computeSFPScore(const double* pieceCountsPla, const double* pieceCountsOpp,
		double* advancementGood, piece_t breakPenaltyPiece)
{
  double plaNumStronger[NUMTYPES];
  double oppNumStronger[NUMTYPES];
  plaNumStronger[ELE] = 0;
  oppNumStronger[ELE] = 0;
  if(advancementGood != NULL)
  	advancementGood[ELE] = 0;
  for(int piece = CAM; piece >= RAB; piece--)
  {
    plaNumStronger[piece] = plaNumStronger[piece+1] + pieceCountsOpp[piece+1];
    oppNumStronger[piece] = oppNumStronger[piece+1] + pieceCountsPla[piece+1];

    DEBUGASSERT(!(pieceCountsPla[piece+1] < 0 || pieceCountsOpp[piece+1] < 0 || pieceCountsPla[piece+1] > 2.001 || pieceCountsOpp[piece+1] > 2.001))

    if(advancementGood != NULL)
    {
    	advancementGood[piece] = 3.0/(oppNumStronger[piece]+1.5)/(oppNumStronger[piece]+1.2);
    	advancementGood[piece] *= ADVGOODDIFFTABLE[piece-breakPenaltyPiece+6];
    	advancementGood[piece] -= 0.1;
    	if(advancementGood[piece] < 0) advancementGood[piece] = 0;
    	//cout << "advancementGood " << Board::PIECECHARS[GOLD][piece] << " " << advancementGood[piece] << endl;
    }
  }

  int score = 0;
  double tier = 0.0;
  int piece = ELE;
  while(piece >= CAT)
  {
    double plac = 0.0;
    double oppc = 0.0;
    double tierinc = 0.0;
    while(true)
    {
      plac += pieceCountsPla[piece];
      oppc += pieceCountsOpp[piece];
      tierinc += max(pieceCountsPla[piece],pieceCountsOpp[piece]);
      piece--;
      if(piece <= RAB)
        break;
      if(plac == 0.0 && oppc == 0.0)
        continue;
      if(plac == 0.0 && pieceCountsPla[piece] == 0.0)
        continue;
      if(oppc == 0.0 && pieceCountsOpp[piece] == 0.0)
        continue;
      break;
    }
    int s = getSFPTierScore(tier,plac,oppc);
    score += s;
    //printf("tier %1.2f pla %1.2f opp %1.2f s %d\n", tier, plac, oppc, s);
    tier += tierinc;
  }

  return score;
}

//Percent bonus for staying away from the sfp situation
static const int SFPAdvGood_Percent_Rad[16] =
{30,40,58,76,84,91,96,100,100,100,100,100,100,100,100,100};
static const int SFPAdvGood_Percent_GYDist[8] =
{80,100,100,95,90,85,70,50};
static const int SFPAdvGood_Percent_Centrality[7] =
{100,100,100,86,70,50,30};

static const double SFPAdvMinFactor = 0.70;
static const double SFPAdvMaxFactor = 1.20;

static double computeSFPAdvGoodFactor(const Board& b, pla_t pla, loc_t sfpCenter, const double* advancementGood)
{
	double totalAdvancementGood = 0.0;
	double totalPercent = 0.0;
	for(piece_t piece = CAT; piece <= ELE; piece++)
	{
		if(advancementGood[piece] < 0.001)
			continue;
		int count = b.pieceCounts[pla][piece];
		if(count <= 0)
			continue;
		totalAdvancementGood += count*advancementGood[piece];

		Bitmap map = b.pieceMaps[pla][piece];
		while(map.hasBits())
		{
			loc_t ploc = map.nextBit();
			double percent =
					SFPAdvGood_Percent_Rad[Board::MANHATTANDIST[sfpCenter][ploc]] *
					SFPAdvGood_Percent_GYDist[Board::GOALYDIST[pla][ploc]] *
					SFPAdvGood_Percent_Centrality[Board::CENTRALITY[ploc]];
			totalPercent += percent/1000000.0 * advancementGood[piece];
		}
	}
	if(totalAdvancementGood < 0.001)
		return 1.0;

	totalPercent /= totalAdvancementGood;
	if(totalAdvancementGood > 3)
		totalAdvancementGood = 3;

	double factor = (totalPercent-0.5)*totalAdvancementGood/2.7 + 1.0;
	if(factor < SFPAdvMinFactor)
		factor = SFPAdvMinFactor;
	else if(factor > SFPAdvMaxFactor)
		factor = SFPAdvMaxFactor;
	return factor;
}

//ROTATION----------------------------------------------------------------------------
static const double ROTDIV = 100;
static const int rotationFactorLen = 18;
static const int rotationFactor[rotationFactorLen] =
{0,7,10,16,20,33,36,40,43,50,54,60,67,74,82,90,100,100};

//FRAMES-------------------------------------------------------------------------------------

static const int FRAMEBREAKSTEPSDIV = 100;
static const int FRAMEBREAKSTEPSFACTORLEN = 12;
static const int FRAMEBREAKSTEPSFACTOR[FRAMEBREAKSTEPSFACTORLEN] =
{9,21,34,45,55,67,75,83,90,95,98,100};

static const int FRAME_SFP_LOGISTIC_DIV = 240;
static const double FRAMEPINMOBLOSS = 70;
static const double FRAMEMOBLOSS[2][64] = {
{
 70, 70, 70, 69, 69, 70, 70, 70,
 69, 68, 66, 64, 64, 66, 68, 69,
 66, 65, 60, 58, 58, 60, 65, 66,
 64, 55, 54, 49, 49, 54, 55, 64,
 64, 56, 55, 46, 46, 55, 56, 64,
 66, 63, 58, 49, 49, 58, 63, 66,
 68, 67, 67, 58, 58, 67, 67, 68,
 70, 69, 68, 67, 67, 68, 69, 70,
},
{
 65, 65, 65, 64, 64, 65, 65, 65,
 64, 63, 61, 59, 59, 61, 63, 64,
 60, 59, 54, 52, 52, 54, 59, 60,
 58, 49, 48, 42, 42, 48, 49, 58,
 57, 49, 52, 39, 39, 52, 49, 57,
 68, 85, 42, 44, 44, 42, 85, 68,
 61, 70, 70, 51, 51, 70, 70, 61,
 68, 68, 66, 61, 61, 66, 68, 68,
}};

static const int EFRAMEHOLDPENALTY[64] =
{
1400,1400,1400,1400,1400,1400,1400,1400,
1400,1400,1000, 800, 800,1000,1400,1400,
1000, 900, 200, 150, 150, 200, 900,1000,
 700, 300, 150, 100, 100, 150, 300, 700,
 700, 400, 100, 100, 100, 100, 400, 700,
1100,1100, 200, 200, 200, 200,1100,1100,
1600,1600,1200,1000,1000,1200,1600,1600,
1600,1600,1600,1600,1600,1600,1600,1600,
};

static const int ROTATION_EFRAMEHOLDPENALTY_MOD[rotationFactorLen] =
{15, 19,24,30,36, 43,52,62,72, 80,87,92,96, 98,99,99,100, 100};

static double getFrameMobLoss(pla_t pla, loc_t loc, int mobilityLevel)
{
  if(pla == 1)
    return FRAMEMOBLOSS[mobilityLevel][loc]/100.0;
  else
    return FRAMEMOBLOSS[mobilityLevel][(7-loc/8)*8+loc%8]/100.0;
}

static int getFrameBestSFP(const Board& b, pla_t pla, FrameThreat threat,
    const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], double* advancementGood,
    int& eFrameHolderPenalty, bool print)
{
  pla_t opp = OPP(pla);
  loc_t kt = threat.kt;
  loc_t pinnedLoc = threat.pinnedLoc;

	//Copy over counts
	double plaPieceCounts[NUMTYPES];
	double oppPieceCounts[NUMTYPES];
	copyPieceCounts(b,pla,plaPieceCounts,oppPieceCounts);

	//Opp - Framed piece
	{
		double prop = FRAMEPINMOBLOSS/100.0;
		oppPieceCounts[b.pieces[kt]] -= prop;
		oppPieceCounts[0] -= prop;
		//Opp - Pinned piece
		prop = getFrameMobLoss(opp,pinnedLoc,0);
		oppPieceCounts[b.pieces[pinnedLoc]] -= prop;
		oppPieceCounts[0] -= prop;
	}

	//Pla - Holders
	Bitmap holderMap = threat.holderMap;
	eFrameHolderPenalty = 0;
	while(holderMap.hasBits())
	{
		loc_t loc = holderMap.nextBit();
		if(b.owners[loc] != pla)
			continue;
		double prop = getFrameMobLoss(pla,loc,1);
		plaPieceCounts[b.pieces[loc]] -= prop;
		plaPieceCounts[0] -= prop;

		if(b.pieces[loc] == ELE)
		  eFrameHolderPenalty = EFRAMEHOLDPENALTY[pla == 1 ? loc : 63 - loc];
	}

	//Compute rotations
	Bitmap rotatableMap = threat.holderMap & ~b.pieceMaps[opp][0] & ~b.pieceMaps[pla][RAB] & ~b.pieceMaps[pla][CAT];
	int numRotatableMax = rotatableMap.countBits()*2;
    
    loc_t* holderRotLocs = new loc_t[numRotatableMax];
    int* holderRotDists = new int[numRotatableMax];
    loc_t* holderRotBlockers = new loc_t[numRotatableMax];
    bool* holderIsSwarm = new bool[numRotatableMax];
    int* minStrNeededArr = new int[numRotatableMax];

    //loc_t holderRotLocs[numRotatableMax];
	//int holderRotDists[numRotatableMax];
	//loc_t holderRotBlockers[numRotatableMax];
	//bool holderIsSwarm[numRotatableMax];
    //int minStrNeededArr[numRotatableMax];
    int numRotatable = Eval::fillBlockadeRotationArrays(b,pla,holderRotLocs,holderRotDists,holderRotBlockers,holderIsSwarm,minStrNeededArr,
			Bitmap::makeLoc(kt),threat.holderMap & b.pieceMaps[pla][0],rotatableMap,pStrongerMaps,ufDist);

	//Try rotations - find the best SFP score for holding the frame
	int bestSfpVal = computeSFPScore(plaPieceCounts,oppPieceCounts,advancementGood,b.pieces[threat.kt]);
	for(int i = 0; i<numRotatable; i++)
	{
		loc_t loc = holderRotLocs[i];
		int rotDist = holderRotDists[i];
		loc_t blockerLoc = holderRotBlockers[i];
		if(rotDist >= rotationFactorLen)
			continue;
		ARIMAADEBUG(if(print)	cout << "Rotate " << (int)loc << " in " << rotDist  << " with " << (int)blockerLoc << endl;)

		double mobilityPropCost = getFrameMobLoss(pla,loc,1);
		double rotationProp = rotationFactor[rotDist]/ROTDIV;
		plaPieceCounts[b.pieces[loc]] += mobilityPropCost;
		plaPieceCounts[b.pieces[loc]] -= mobilityPropCost * rotationProp;
		if(blockerLoc != ERRORSQUARE)
			plaPieceCounts[b.pieces[blockerLoc]] -= mobilityPropCost * (1.0 - rotationProp);
		int sfpVal = computeSFPScore(plaPieceCounts,oppPieceCounts);
		if(blockerLoc != ERRORSQUARE)
			plaPieceCounts[b.pieces[blockerLoc]] += mobilityPropCost * (1.0 - rotationProp);
		plaPieceCounts[b.pieces[loc]] += mobilityPropCost * rotationProp;
		plaPieceCounts[b.pieces[loc]] -= mobilityPropCost;

		if(b.pieces[loc] == ELE)
		  eFrameHolderPenalty = eFrameHolderPenalty * ROTATION_EFRAMEHOLDPENALTY_MOD[rotDist]/100;

		if(sfpVal > bestSfpVal)
			bestSfpVal = sfpVal;
	}

    delete[] holderRotLocs;
    delete[] holderRotDists;
    delete[] holderRotBlockers;
    delete[] holderIsSwarm;
    delete[] minStrNeededArr;

	return bestSfpVal;
}

Strat Eval::evalFrame(const Board& b, pla_t pla, FrameThreat threat, const eval_t pValues[2][NUMTYPES],
		const Bitmap pStrongerMaps[2][NUMTYPES], int ufDist[64], bool print)
{
  pla_t opp = OPP(pla);
  loc_t kt = threat.kt;
  loc_t pinnedLoc = threat.pinnedLoc;
  int framedValue = evalCapOccur(b,pValues[opp],kt,0);

  double advancementGood[NUMTYPES];
  int eFrameHolderPenalty = 0;
  int bestSfpVal = getFrameBestSFP(b,pla,threat,pStrongerMaps,ufDist,advancementGood,eFrameHolderPenalty,print);

  //Evaluate cost
  int frameVal = computeLogistic(bestSfpVal,FRAME_SFP_LOGISTIC_DIV,framedValue);

  double sfpAdvGoodFactor = computeSFPAdvGoodFactor(b,pla,threat.kt,advancementGood);
  frameVal = (int)(frameVal * sfpAdvGoodFactor) - eFrameHolderPenalty;

  //Simulate freezing
  ufDist[kt] += 6; ufDist[pinnedLoc] += 6;
  int breakSteps = getFrameBreakDistance(b, pla, kt, threat.holderMap, pStrongerMaps, ufDist);
  ufDist[kt] -= 6; ufDist[pinnedLoc] -= 6;

  if(breakSteps < FRAMEBREAKSTEPSFACTORLEN)
    frameVal = frameVal * FRAMEBREAKSTEPSFACTOR[breakSteps] / FRAMEBREAKSTEPSDIV;

  //Adjust for partial frames
  if(threat.isPartial)
    frameVal /= 2;

  if(frameVal < 0)
    frameVal = 0;

  Strat strat(pla,frameVal,kt,kt,false,threat.holderMap & b.pieceMaps[pla][0]);

  ARIMAADEBUG(if(print)
  {
    cout << "Frame " << strat << endl;
    cout << "FramebestSFP " << bestSfpVal << " breaksteps " << breakSteps << endl;
    cout << "sfpAdvGoodFactor" << sfpAdvGoodFactor << endl;
  })

  return strat;
}

//TODO Use restrict keyword?
//holderRotBlockers can sometimes be errorsquare
int Eval::fillBlockadeRotationArrays(const Board& b, pla_t pla, loc_t* holderRotLocs, int* holderRotDists,
    loc_t* holderRotBlockers, bool* holderIsSwarm, int* minStrNeededArr, Bitmap heldMap, Bitmap holderMap, Bitmap rotatableMap,
    const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64])
{
  //Simulate blocks
  int singleBonusSteps[64];
  int multiBonusSteps[64];
  Eval::fillBlockadeRotationBonusSteps(heldMap,holderMap,singleBonusSteps,multiBonusSteps);
  int n = 0;
  Bitmap map = rotatableMap;
  while(map.hasBits())
  {
    loc_t loc = map.nextBit();
    bool tryMultiSwarm = false;
    int minStrNeeded = RAB;
    BlockadeRotation rot = getBlockadeSingleRotation(b, pla, loc, heldMap, holderMap,
        pStrongerMaps, ufDist, singleBonusSteps, multiBonusSteps, tryMultiSwarm, minStrNeeded);

    holderRotLocs[n] = loc;
    holderRotDists[n] = rot.dist;
    holderRotBlockers[n] = rot.blockerLoc;
    holderIsSwarm[n] = rot.isSwarm;
    minStrNeededArr[n] = minStrNeeded;
    n++;

    if(tryMultiSwarm)
    {
      BlockadeRotation rot2 = getBlockadeMultiRotation(b,pla,loc,holderMap,pStrongerMaps);
      holderRotLocs[n] = loc;
      holderRotDists[n] = rot2.dist;
      holderRotBlockers[n] = rot2.blockerLoc;
      holderIsSwarm[n] = rot2.isSwarm;
      minStrNeededArr[n] = RAB;
      n++;
    }

  }
  int numRotatable = n;

  //Find multistep rotations - can i be replaced by j after j is replaced by some other piece?
  for(int i = 0; i<numRotatable; i++)
  {
    loc_t loc = holderRotLocs[i];
    int minStrNeeded = minStrNeededArr[i];
    if(minStrNeeded <= RAB)
      continue;
    for(int j = 0; j<numRotatable; j++)
    {
      loc_t loc2 = holderRotLocs[j];
      if(j == i || b.pieces[loc2] < minStrNeeded || b.pieces[loc2] >= b.pieces[loc] || holderRotDists[j] > holderRotDists[i]-2)
        continue;
      int dist = Threats::traverseDist(b,loc2,loc,false,ufDist,holderRotDists[i]-holderRotDists[j]-1,singleBonusSteps);
      int totalDist = dist + 1 + holderRotDists[j];
      if(totalDist < holderRotDists[i])
      {
        holderRotDists[i] = totalDist;
        holderRotBlockers[i] = holderRotBlockers[j];
        holderIsSwarm[i] = holderIsSwarm[j];
      }
    }
  }
  return numRotatable;
}

void Eval::fillBlockadeRotationBonusSteps(Bitmap heldMap, Bitmap holderMap, int singleBonusSteps[64], int multiBonusSteps[64])
{
  for(int i = 0; i<64; i++)
  {singleBonusSteps[i] = 0; multiBonusSteps[i] = 0;}
  while(heldMap.hasBits())
  {loc_t loc = heldMap.nextBit(); singleBonusSteps[loc] = 8; multiBonusSteps[loc] = 8;}
  while(holderMap.hasBits())
  {multiBonusSteps[holderMap.nextBit()] = -1;}
}

BlockadeRotation Eval::getBlockadeSingleRotation(const Board& b, pla_t pla, loc_t ploc, Bitmap heldMap, Bitmap holderMap,
    const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], int singleBonusSteps[64], int multiBonusSteps[64],
    bool& tryMultiSwarm, piece_t& minStrNeeded)
{
  pla_t opp = OPP(pla);
  int bestBlockerDist = 10;
  loc_t bestBlockerLoc = ERRORSQUARE;
  bool bestIsSwarm = false;
  tryMultiSwarm = false;
  minStrNeeded = RAB;

  //Special case - on a trap
  if(Board::ISTRAP[ploc] && b.isOpenToMove(ploc) && (!b.isTrapSafe2(opp,ploc) || b.isBlocked(ploc)))
    return BlockadeRotation(bestBlockerLoc,false,2);

  tryMultiSwarm = true;

  //Try rotation of a single piece. If a weaker piece could be put here, attempt to find such a weaker piece and put it here.
  //Meanwhile finding how strong a piece we need if we are doing it nonswarming
  piece_t minStr = RAB;
  Bitmap map = heldMap & ~b.frozenMap;
  bool isBlocked = b.isBlocked(ploc);
  bool canSwarm = isBlocked;
  while(map.hasBits())
  {
    loc_t loc = map.nextBit();
    if(!Board::ISADJACENT[loc][ploc]) continue;
    if(b.pieces[loc] > minStr) minStr = b.pieces[loc];
  }
  map = heldMap & b.frozenMap;
  while(map.hasBits())
  {
    loc_t loc = map.nextBit();
    if(!Board::ISADJACENT[loc][ploc]) continue;
    if(b.wouldBeDom(opp,loc,loc,ploc)) continue;
    if(b.pieces[loc] == RAB)
    {
      if(!b.isRabOkay(opp,loc,ploc)) continue;
      if(b.isOpenToStep(loc,ploc)) {canSwarm = false; tryMultiSwarm = false; if(CAT > minStr) {minStr = CAT;}}
    }
    else
    {
      canSwarm = false; tryMultiSwarm = false;
      if(b.pieces[loc]+1 > minStr) {minStr = b.pieces[loc]+1;}
    }
  }
  minStrNeeded = minStr;
  //Locate all weaker pieces strong enough to handle it singly
  Bitmap possibleBlockers = minStr <= RAB ? b.pieceMaps[pla][0] : pStrongerMaps[opp][minStr-1];
  possibleBlockers &= ~holderMap & ~pStrongerMaps[opp][b.pieces[ploc]-1];

  //Find closest, iterating out by radius
  const int maxRad = 6;
  //An extra 1 if blocked for radius 1, an extra 1 if blocked for all higher radii
  int extraSteps = isBlocked;
  for(int rad = 1; rad < maxRad && rad < bestBlockerDist - extraSteps; rad++)
  {
    Bitmap radBlockers = possibleBlockers & Board::RADIUS[rad][ploc];
    while(radBlockers.hasBits())
    {
      loc_t loc = radBlockers.nextBit();
      bool bonused[4] = {false,false,false,false};
      for(int i = 0; i < 4; i++)
      {
        if(!Board::ADJOKAY[i][ploc]) continue;
        loc_t adjloc =  ploc + Board::ADJOFFSETS[i];
        if(!b.wouldBeUF(pla,loc,adjloc,ploc)) {bonused[i] = true; singleBonusSteps[adjloc] += 1;}
      }

      int tdist = Threats::traverseDist(b,loc,ploc,false,ufDist,bestBlockerDist-extraSteps,singleBonusSteps);
      for(int i = 0; i < 4; i++)
        if(bonused[i])
        {loc_t adjloc =  ploc + Board::ADJOFFSETS[i]; singleBonusSteps[adjloc] -= 1;}

      int dist = tdist + extraSteps;
      if(dist < bestBlockerDist || (dist == bestBlockerDist && bestBlockerLoc != ERRORSQUARE && b.pieces[loc] < b.pieces[bestBlockerLoc]))
      {
        bestBlockerLoc = loc;
        bestBlockerDist = dist;

        if(rad >= bestBlockerDist - extraSteps)
          break;
      }
    }
    //1 if blocked for radius 1, 2 if blocked for all higher radii
    if(rad == 1 && isBlocked)
      extraSteps++;
  }

  //Handle the case where we can swarm block, but do it with only one piece rotated in
  //because the rest of the swarm is already there.

  if(canSwarm && minStr > RAB)
  {
    tryMultiSwarm = false;
    Bitmap swarmPossibleBlockers = b.pieceMaps[pla][0] & ~holderMap & ~possibleBlockers;
    const int maxRad = 6;
    int extraSteps = 4; //We're caught in the middle, so we have to do a lot of shuffling.
    for(int rad = 2; rad < maxRad && rad < bestBlockerDist - extraSteps; rad++)
    {
      Bitmap radBlockers = swarmPossibleBlockers & Board::RADIUS[rad][ploc];
      while(radBlockers.hasBits())
      {
        loc_t loc = radBlockers.nextBit();
        int tdist = Threats::traverseDist(b,loc,ploc,false,ufDist,bestBlockerDist-extraSteps,multiBonusSteps);

        int dist = tdist + extraSteps;
        if(dist < bestBlockerDist)
        {
          bestBlockerLoc = loc;
          bestBlockerDist = dist;
          bestIsSwarm = true;

          if(rad < bestBlockerDist - extraSteps)
            break;
        }
      }
    }
  }

  if(minStr <= RAB)
    tryMultiSwarm = false;

  return BlockadeRotation(bestBlockerLoc,bestIsSwarm,bestBlockerDist < 10 ? bestBlockerDist : 16);
}

BlockadeRotation Eval::getBlockadeMultiRotation(const Board& b, pla_t pla, loc_t ploc, Bitmap holderMap,
    const Bitmap pStrongerMaps[2][NUMTYPES])
{
  pla_t opp = OPP(pla);
  //Count how many new blockers we need
  int numBlockersNeeded = 1;
  for(int i = 0; i<4; i++)
  {
    if(!Board::ADJOKAY[i][ploc]) continue;
    loc_t loc = ploc + Board::ADJOFFSETS[i];
    if(b.owners[loc] == NPLA)
      numBlockersNeeded += 1;
  }
  if(numBlockersNeeded == 1)
    return BlockadeRotation(ERRORSQUARE,false,16);

  //Iterate out, looking for blockers
  int bestBlockerDist = 11;
  loc_t bestBlockerLoc = ERRORSQUARE;
  int numBlockersFound = 0;
  int totalDist = 0;
  //For general inefficiency and confusion in shuffling around
  int extraDist = 5;
  Bitmap possibleBlockers = b.pieceMaps[pla][0] & ~holderMap & ~pStrongerMaps[opp][b.pieces[ploc]-1];
  const int maxRad = 4;
  for(int rad = 2; rad <= maxRad && totalDist + (numBlockersNeeded - numBlockersFound)*(rad-1) + extraDist < bestBlockerDist; rad++)
  {
    Bitmap radBlockers = possibleBlockers & Board::RADIUS[rad][ploc];
    int numNewBlockers = radBlockers.countBits(); //TODO iterative count bits better?
    if(numNewBlockers > numBlockersNeeded-numBlockersFound)
      numNewBlockers = numBlockersNeeded-numBlockersFound;
    numBlockersFound += numNewBlockers;
    totalDist += numNewBlockers*(rad-1);
    if(numBlockersFound >= numBlockersNeeded)
    {
      bestBlockerLoc = radBlockers.nextBit();
      break;
    }
  }
  if(numBlockersFound >= numBlockersNeeded)
    bestBlockerDist = totalDist + extraDist;
  return BlockadeRotation(bestBlockerLoc,true,bestBlockerDist < 11 ? bestBlockerDist : 16);
}

int Eval::getFrameBreakDistance(const Board& b, pla_t pla, loc_t bloc, Bitmap defenseMap,
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64])
{
  pla_t opp = OPP(pla);

  //For each square, compute ability to break, and take the min
  int minBreakDist = 16;
  while(defenseMap.hasBits())
  {
    loc_t loc = defenseMap.nextBit();
    int bs = 16;
    if(b.owners[loc] == opp)
    {
      if(b.isOpenToStep(loc)) bs = ufDist[loc]+3;
      else if(b.isOpenToMove(loc)) bs = ufDist[loc]+4;
    }
    else if(b.owners[loc] == pla)
    {
      bs = Threats::attackDist(b,loc,5,pStrongerMaps,ufDist)+3;
      if(Board::ISADJACENT[bloc][loc] && !b.isRabOkay(opp,bloc,loc))
        bs++;
    }
    else
    {
    	bs = Threats::occupyDist(b,opp,loc,5,pStrongerMaps,ufDist)+2;
    }

    if(bs < minBreakDist)
      minBreakDist = bs;
  }

  if(minBreakDist < 0)
    minBreakDist = 0;

  return minBreakDist;
}


//HOSTAGES-------------------------------------------------------------------------------------

static const int HOSTAGE_UF_BS_LEN = 7;
static const int HOSTAGE_UF_BS_FACTOR[HOSTAGE_UF_BS_LEN] =
{30,45,60,80,91,96,100};
static const int HOSTAGE_HOLDER_BS_LEN = 12;
static const int HOSTAGE_HOLDER_BS_FACTOR[HOSTAGE_HOLDER_BS_LEN] =
{34,45,55,67,75,83,90,95,98,100,100,100};
//static const int HOSTAGE_OPP_TSTAB_LEN = TCMAX+1;
//static const int HOSTAGE_OPP_TSTAB_RATIO[HOSTAGE_OPP_TSTAB_LEN] =
//{100,96,92,87,81,72,62,50,40,31,25,20,17,14,12,10,9,8,7,7}; //TODO test these to see which is better
static const int HOSTAGE_OPP_TSTAB_LEN = 40;
static const int HOSTAGE_OPP_TSTAB_RATIO[HOSTAGE_OPP_TSTAB_LEN] =
{100,99,98,97,96,95,94,93,92,91, 89,88,86,85,83,81,79,77,74,72,
		69,66,63,60,56,52,48,44,40,36, 33,30,27,24,22,20,19,17,16,15};
//100-65  65-30  30-(-5) (-5)-(-40)

static const int HOSTAGE_SWARMY_LEN = 13;
static const int HOSTAGE_SWARMY_FACTOR[HOSTAGE_SWARMY_LEN] =
{100,97,93,88,83,79,75,72,69,67,65,64,63};

static const int HOSTAGE_SFP_DIST_LEN = 6;
static const int HOSTAGE_SFP_DIST_FACTOR[HOSTAGE_SFP_DIST_LEN] =
{60,75,85,92,100,100};
static const int HOSTAGE_SECONDARY_SFP_DIST_LEN = 6;
static const int HOSTAGE_SECONDARY_SFP_DIST_FACTOR[HOSTAGE_SECONDARY_SFP_DIST_LEN] =
{85,90,96,98,100,100};

static const int HOSTAGE_SFP_LOGISTIC_DIV = 280;
static const double HOSTAGEHELDMOBLOSS = 70;
static const double HOSTAGEMOBLOSS[64] = {
 65, 65, 65, 64, 64, 65, 65, 65,
 64, 63, 61, 59, 59, 61, 63, 64,
 62, 58, 54, 52, 52, 54, 58, 62,
 61, 49, 48, 42, 42, 48, 49, 61,
 58, 49, 48, 39, 39, 48, 49, 58,
 58, 55, 42, 41, 41, 42, 55, 58,
 61, 60, 60, 51, 51, 60, 60, 61,
 64, 63, 62, 61, 61, 62, 63, 64,
};

//[ktindex][loc]
static const int SWARMYFACTOR[4][64] = {
{
3,3,4,3,1,0,0,0,
3,4,5,2,1,0,0,0,
4,5,5,4,2,0,0,0,
3,4,4,3,1,0,0,0,
2,3,3,2,0,0,0,0,
1,2,2,1,0,0,0,0,
0,1,1,0,0,0,0,0,
0,0,0,0,0,0,0,0,
},
{
0,0,0,1,3,4,3,3,
0,0,0,1,2,5,4,3,
0,0,0,2,4,5,5,4,
0,0,0,1,3,4,4,3,
0,0,0,0,2,3,3,2,
0,0,0,0,1,2,2,1,
0,0,0,0,0,1,1,0,
0,0,0,0,0,0,0,0,
},
{
0,0,0,0,0,0,0,0,
0,1,1,0,0,0,0,0,
1,2,2,1,0,0,0,0,
2,3,3,2,0,0,0,0,
3,4,4,3,1,0,0,0,
4,5,5,4,2,0,0,0,
3,4,5,2,1,0,0,0,
3,3,4,3,1,0,0,0,
},
{
0,0,0,0,0,0,0,0,
0,0,0,0,0,1,1,0,
0,0,0,0,1,2,2,1,
0,0,0,0,2,3,3,2,
0,0,0,1,3,4,4,3,
0,0,0,2,4,5,5,4,
0,0,0,1,2,5,4,3,
0,0,0,1,3,4,3,3,
},
};

//Swarm is good for opponent if holder is ELE and HOSTAGE_SWARM_GOOD[holderLoc] == pla
static const int HOSTAGE_SWARM_GOOD[64] = {
1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
1,1,2,2,2,2,1,1,
1,2,2,2,2,2,2,1,
0,2,2,2,2,2,2,0,
0,0,2,2,2,2,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
};

double Eval::getHostageMobLoss(pla_t pla, loc_t loc)
{
  if(pla == GOLD)
    return HOSTAGEMOBLOSS[loc]/100.0;
  else
    return HOSTAGEMOBLOSS[(7-loc/8)*8+loc%8]/100.0;
}

bool Eval::isHostageIndefensible(const Board& b, pla_t pla, HostageThreat threat, int runawayBS, int holderAttackDist,
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64])
{
	return false; //TODO should work, but is a bit fragile right now, so disabled

	pla_t opp = OPP(pla);
	loc_t kt = threat.kt;
  int trapIndex = Board::TRAPINDEX[kt];
  //Must be unable to runaway or break the hostage, and must have no opp defenders and at least 2 pla defenders
  if(!(runawayBS > 4 && holderAttackDist > 2 && (b.isGuarded(pla,threat.holderLoc) || holderAttackDist > 4)
  		&& b.trapGuardCounts[opp][trapIndex] == 0 && b.trapGuardCounts[pla][trapIndex] >= 2 && threat.threatSteps <= 4))
  	return false;
	//Find the strongest piece within rad 2 of the trap
	piece_t strongest = ELE;
	while(strongest > RAB && (b.pieceMaps[pla][strongest] & Board::DISK[2][kt]).isEmpty())
		strongest--;
	if(strongest <= RAB)
		return false;

	//Check if any opponent pieces at least that strong could possibly reach in time
	Bitmap oppMap = pStrongerMaps[pla][strongest-1];
	if((Board::DISK[1][kt] & oppMap).hasBits())
		return false;
	int moveAdjDist = Threats::moveAdjDistUF(b,opp,kt,5,ufDist,pStrongerMaps,strongest-1);
	if(moveAdjDist < 5)
		return false;

	//Check that the total time to move multi pieces in to defend everything is too large
	//and that none of pla's current defenders can be pushed.
	int totalDist = 0;
	int maxDistToTest = 5;
	for(int i = 0; i<4; i++)
	{
		loc_t loc = Board::TRAPDEFLOCS[trapIndex][i];
		if(b.owners[loc] == pla)
		{
			if(Threats::attackDist(b,loc,3,pStrongerMaps,ufDist) + 2 < 5)
				return false;
		}
		else if(b.owners[loc] == NPLA)
		{
			int dist = Threats::occupyDist(b,opp,loc,maxDistToTest,pStrongerMaps,ufDist);
			totalDist += maxDistToTest;
			if(dist < 2)
				return false;
			maxDistToTest -= dist;
			if(maxDistToTest < 2) maxDistToTest = 2;
		}
	}
	if(totalDist < 5)
		return false;

	return true;
}


double Eval::evalHostageProp(const Board& b, pla_t pla, HostageThreat threat,
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], const int tc[2][4], bool& indefensible,
		 double* advancementGood, bool print)
{
  pla_t opp = OPP(pla);
  loc_t kt = threat.kt;
  loc_t hostageLoc = threat.hostageLoc;
  loc_t holderLoc = threat.holderLoc;

  //Copy over counts
  double plaPieceCounts[NUMTYPES];
  double oppPieceCounts[NUMTYPES];
  copyPieceCounts(b,pla,plaPieceCounts,oppPieceCounts);

  //Hostaged piece
  double prop = HOSTAGEHELDMOBLOSS/100.0;
  oppPieceCounts[b.pieces[hostageLoc]] -= prop;
  oppPieceCounts[0] -= prop;
  //Holding piece
  prop = getHostageMobLoss(pla,holderLoc);
  plaPieceCounts[b.pieces[holderLoc]] -= prop;
  plaPieceCounts[0] -= prop;

  //Mobility for all defenders
  double oppKTMobilityProp = getHostageMobLoss(opp,kt);

  //Mark off strongest piece (usually ELE) and compute its SFP, then undo it
  int baseSFPVal = 0;
  piece_t strongestOppPiece = ELE;
  for(int piece = ELE; piece >= RAB; piece--)
  {
    if(b.pieceCounts[opp][piece] > 0)
    {
      if(oppPieceCounts[piece] >= oppKTMobilityProp)
      {
        oppPieceCounts[piece] -= oppKTMobilityProp;
        oppPieceCounts[0] -= oppKTMobilityProp;
        baseSFPVal = computeSFPScore(plaPieceCounts,oppPieceCounts,advancementGood,b.pieces[hostageLoc]);
        oppPieceCounts[piece] += oppKTMobilityProp;
        oppPieceCounts[0] += oppKTMobilityProp;
        strongestOppPiece = piece;
        break;
      }
    }
  }

  //Mark out all weaker defenders
  int numOtherDefendersFound = 0;
  for(int i = 0; i<4; i++)
  {
    loc_t loc = kt + Board::ADJOFFSETS[i];
    if(b.owners[loc] == opp && b.pieces[loc] < strongestOppPiece)
    {
      numOtherDefendersFound++;
      oppPieceCounts[b.pieces[loc]] -= oppKTMobilityProp;
      oppPieceCounts[0] -= oppKTMobilityProp;
    }
  }

  //If too few defenders, mark out the next highest ones possible till we have enough
  int numDefendersNeeded = 2-numOtherDefendersFound;
  for(int piece = strongestOppPiece-1; piece >= RAB && numDefendersNeeded > 0; piece--)
  {
    while(oppPieceCounts[piece] >= oppKTMobilityProp && numDefendersNeeded > 0)
    {
      oppPieceCounts[piece] -= oppKTMobilityProp;
      oppPieceCounts[0] -= oppKTMobilityProp;
      numDefendersNeeded--;
    }
  }

  int defendedSFPVal = computeSFPScore(plaPieceCounts,oppPieceCounts);

  //Compute values
  double baseValue = computeLogistic(baseSFPVal,HOSTAGE_SFP_LOGISTIC_DIV,10000)/10000.0;
  double defendedValue = computeLogistic(defendedSFPVal,HOSTAGE_SFP_LOGISTIC_DIV,10000)/10000.0;
  if(baseValue <= 0)
    baseValue = 0;
  if(defendedValue <= 0)
    defendedValue = 0;
  if(defendedValue > baseValue)
    defendedValue = baseValue;

  //Adjust ratio by trap stability
  int oppTrapStab = (100-getHostageNoETrapControl(b,pla,kt,ufDist,tc))*HOSTAGE_OPP_TSTAB_LEN/140;
  double ratio;
  if(oppTrapStab >= HOSTAGE_OPP_TSTAB_LEN)
  	ratio = HOSTAGE_OPP_TSTAB_RATIO[HOSTAGE_OPP_TSTAB_LEN-1]/100.0;
  else if(oppTrapStab < 0)
  	ratio = 1;
	else
		ratio = HOSTAGE_OPP_TSTAB_RATIO[oppTrapStab]/100.0;

  //Adjust ratio by piece swarming for defender
  int swarmyness = 0;
  if(b.pieces[holderLoc] == ELE && HOSTAGE_SWARM_GOOD[holderLoc] == pla)
  {
    int ktIndex = Board::TRAPINDEX[kt];
    Bitmap map = b.pieceMaps[opp][0];
    while(map.hasBits())
    {
      loc_t loc = map.nextBit();
      if(loc != hostageLoc && b.pieces[loc] != ELE)
        swarmyness += (b.pieces[loc]+3) * SWARMYFACTOR[ktIndex][loc];
    }
    swarmyness /= 15;
    ratio *= HOSTAGE_SWARMY_FACTOR[swarmyness >= HOSTAGE_SWARMY_LEN ? HOSTAGE_SWARMY_LEN-1 : swarmyness]/100.0;
  }

  double value = baseValue*ratio + defendedValue*(1-ratio);

  //Adjust by breakSteps
  int holderAttackDist = Threats::attackDist(b,holderLoc,5,pStrongerMaps,ufDist);
  int runawayBS = getHostageRunawayBreakSteps(b, pla, hostageLoc, holderLoc, ufDist);
  int holderBS = getHostageHolderBreakSteps(b, pla, holderLoc, holderAttackDist);

  int factor1 = HOSTAGE_UF_BS_FACTOR[runawayBS >= HOSTAGE_UF_BS_LEN ? HOSTAGE_UF_BS_LEN-1 : runawayBS];
  int factor2 = HOSTAGE_HOLDER_BS_FACTOR[holderBS >= HOSTAGE_HOLDER_BS_LEN ? HOSTAGE_HOLDER_BS_LEN-1 : holderBS];
  int bsFactor;
  if(factor1 < factor2)
    bsFactor = factor1 * (factor2+100)/2;
  else
    bsFactor = factor2 * (factor1+100)/2;

  value *= bsFactor / 10000.0;

  //Penalize close strongest free piece
  piece_t sfpStr = EMP;
  piece_t sfpStr2 = EMP;
  for(piece_t p = ELE; p >= DOG; p--)
  {
    if(plaPieceCounts[p] > oppPieceCounts[p])
    {sfpStr = p; break;}
  }
  for(piece_t p = sfpStr-1; p >= DOG; p--)
  {
    if(plaPieceCounts[p] > oppPieceCounts[p])
    {sfpStr2 = p; break;}
  }

  if(sfpStr != EMP)
  {
    Bitmap map = b.pieceMaps[pla][sfpStr];
    int totalDist = 0;;
    int count = 0;
    while(map.hasBits())
    {int d = Board::MANHATTANDIST[holderLoc][map.nextBit()]; totalDist += d > 6 ? 6 : d; count++;}
    if(count > 0)
    {
      int avgDist = totalDist/count;
      value *= HOSTAGE_SFP_DIST_FACTOR[avgDist >= HOSTAGE_SFP_DIST_LEN ? HOSTAGE_SFP_DIST_LEN-1 : avgDist] / 100.0;
    }
  }

  if(sfpStr2 != EMP)
  {
    Bitmap map = b.pieceMaps[pla][sfpStr2];
    int totalDist = 0;;
    int count = 0;
    while(map.hasBits())
    {int d = Board::MANHATTANDIST[holderLoc][map.nextBit()]; totalDist += d > 6 ? 6 : d; count++;}
    if(count > 0)
    {
      int avgDist = totalDist/count;
      value *= HOSTAGE_SECONDARY_SFP_DIST_FACTOR[avgDist >= HOSTAGE_SECONDARY_SFP_DIST_LEN ? HOSTAGE_SECONDARY_SFP_DIST_LEN-1 : avgDist] / 100.0;
    }
  }
  indefensible = isHostageIndefensible(b,pla,threat,runawayBS,holderAttackDist,pStrongerMaps,ufDist);

  ARIMAADEBUG(if(print)
  {
		cout << "Hostage " << endl;
		cout << threat
		<< " sfpbase " << baseSFPVal
		<< " sfpdef " << defendedSFPVal
		<< " baseval " << baseValue
		<< " oppTrapStab " << oppTrapStab
		<< " swarmyness " << swarmyness
		<< " ratioedval " << (baseValue*ratio + defendedValue*(1.0-ratio)) << endl
		<< " runawayBS " << runawayBS
		<< " holderBS " << holderBS
		<< " indef " << indefensible
		<< " propvalue " << value << endl;
  })

  return value;
}

Strat Eval::evalHostage(const Board& b, pla_t pla, HostageThreat threat, const eval_t pValues[2][NUMTYPES],
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], const int tc[2][4], bool print)
{
	pla_t opp = OPP(pla);
  loc_t kt = threat.kt;
  loc_t hostageLoc = threat.hostageLoc;
  loc_t holderLoc = threat.holderLoc;

  bool indefensible = false;
  double advancementGood[NUMTYPES];
	int hostageValue = evalCapOccur(b,pValues[opp],hostageLoc,threat.threatSteps);
	double prop = evalHostageProp(b,pla,threat,pStrongerMaps,ufDist,tc, indefensible, advancementGood, print);

	double sfpAdvGoodFactor = computeSFPAdvGoodFactor(b,pla,threat.kt,advancementGood);
	prop *= sfpAdvGoodFactor;

  int value = (int)(hostageValue * prop);

  //Check if it looks outright indefensible
  if(value < hostageValue && indefensible)
		value = (hostageValue*2 + value)/3;

  Strat strat(pla,value,hostageLoc,kt,true,Bitmap::makeLoc(holderLoc));

  ARIMAADEBUG(if(print)
  {
  	cout << "sfpAdvGoodFactor " << sfpAdvGoodFactor << endl;
		cout << "Hostage final " << value << endl;
  })

  return strat;
}

int Eval::getHostageRunawayBreakSteps(const Board& b, pla_t pla, loc_t hostageLoc, loc_t holderLoc, const int ufDist[64])
{
  //Simulate blocks
  int bonusVals[64];
  for(int i = 0; i<64; i++)
  	bonusVals[i] = 0;

  //We only want to consider UFs that don't really target the holder, so block the holder's square
  bonusVals[holderLoc] += 2;

  //Add extra if the hostage has to push his way out. That is, if only one open square adjacent to it, block it by 1. If there are none,
  //block all of them by 1.
  if(b.isOpen(hostageLoc))
  {
    if(!b.isOpen2(hostageLoc))
    	bonusVals[b.findOpen(hostageLoc)] += 1;
  }
  else
  {
    for(int i = 0; i<4; i++)
      if(Board::ADJOKAY[i][hostageLoc])
      	bonusVals[hostageLoc+Board::ADJOFFSETS[i]] += 1;
  }

  //Compute dist
  int dist = Threats::moveAdjDistUF(b,OPP(pla),hostageLoc,5,ufDist);

  return dist+1;
}

static const unsigned char ESQ = ERRORSQUARE;
static const int HOSTAGE_PULL_GUARD_OFFSETS[64] = {
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
  8,  8,  1,  8,  8, -1,  8,  8,
  8,  8,ESQ,ESQ,ESQ,ESQ,  8,  8,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
 -8, -8,ESQ,ESQ,ESQ,ESQ, -8, -8,
 -8, -8,  1, -8, -8, -1, -8, -8,
ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,ESQ,
};
static const int HOSTAGE_PULL_GUARD_BONUS[64] = {
 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2,
 2, 1, 0,-1,-1, 0, 1, 2,
-1,-1,-1, 0, 0,-1,-1,-1,
-1,-1,-1, 0, 0,-1,-1,-1,
 2, 1, 0,-1,-1, 0, 1, 2,
 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2,
};

int Eval::getHostageHolderBreakSteps(const Board& b, pla_t pla, loc_t holderLoc, int holderAttackDist)
{
  if(b.pieces[holderLoc] == ELE)
    return 16;

  //Base attack distance, plus 2 is the break distance.
  int dist = holderAttackDist+2;

  //Check for pull guarding and add the appropriate amount of protection
  int pullGuardOffset = HOSTAGE_PULL_GUARD_OFFSETS[holderLoc];
  if(pullGuardOffset == ERRORSQUARE || b.owners[holderLoc+pullGuardOffset] == pla || b.wouldBeG(pla,holderLoc+pullGuardOffset,holderLoc))
    dist += HOSTAGE_PULL_GUARD_BONUS[holderLoc];

  if(dist < 0)
    dist = 0;

  return dist;
}


int Eval::getHostageNoETrapControl(const Board& b, pla_t pla, loc_t kt, const int ufDist[64], const int tc[2][4])
{
  pla_t opp = OPP(pla);
  int trapIndex = Board::TRAPINDEX[kt];
  //adding the contribution of opponent ele removes its contribution since the opponent is negative
  return tc[pla][trapIndex] + Eval::elephantTrapControlContribution(b,opp,ufDist,trapIndex);
}


//BLOCKADES---------------------------------------------------------------------------------

static const int BLOCKADEBREAKSTEPSFACTORLEN = 12;
static const int BLOCKADEBREAKSTEPSFACTOR[BLOCKADEBREAKSTEPSFACTORLEN] =
{9,21,34,45,55,67,75,83,90,95,98,100};

static const int BLOCKADE_SFP_LOGISTIC_DIV = 240;
static const int BLOCKADEPINMOBLOSS = 70;
static const double BLOCKADEMOBLOSS[64] = {
 65, 65, 65, 64, 64, 65, 65, 65,
 64, 63, 61, 59, 59, 61, 63, 64,
 60, 59, 54, 52, 52, 54, 59, 60,
 58, 49, 48, 42, 42, 48, 49, 58,
 57, 49, 48, 39, 39, 48, 49, 57,
 58, 55, 42, 41, 41, 42, 55, 58,
 61, 60, 60, 51, 51, 60, 60, 61,
 64, 63, 62, 61, 61, 62, 63, 64,
};

static const int BLOCKADE_VALUE_PCT[64] = {
    100,100,100,100,100,100,100,100,
    100, 70,100, 70, 70,100, 70,100,
     50, 30, 50, 30, 30, 50, 30, 50,
     30, 15, 15, 15, 15, 15, 15, 30,
     30, 20, 15, 15, 15, 15, 20, 30,
     32, 24, 15, 20, 20, 15, 24, 32,
     35, 30, 24, 26, 26, 24, 30, 35,
     35, 35, 30, 30, 30, 30, 35, 35,
};

static const int BLOCKADE_SUBTRACT[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50,  0, 50, 50,  0, 50, 50,
     70,100,150,100,100,150,100, 70,
    200,150,100,100,100,100,150,200,
    200,200,200,150,150,200,200,200,
    200,250,300,250,250,300,250,200,
    220,300,300,300,300,300,300,220,
    150,200,250,250,250,250,200,150,
};


double Eval::getBlockadeMobLoss(pla_t pla, loc_t loc)
{
  if(pla == 1)
    return BLOCKADEMOBLOSS[loc]/100.0;
  else
    return BLOCKADEMOBLOSS[(7-loc/8)*8+loc%8]/100.0;
}

int Eval::getBlockadeBestSFP(const Board& b, pla_t pla, BlockadeThreat threat,
    const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], int* baseSFP,
    double* advancementGood, bool print)
{
    pla_t opp = OPP(pla);
    loc_t pinnedLoc = threat.pinnedLoc;

    //Copy over counts
    double plaPieceCounts[NUMTYPES];
    double oppPieceCounts[NUMTYPES];
    copyPieceCounts(b, pla, plaPieceCounts, oppPieceCounts);

    if (baseSFP != NULL)
        *baseSFP = computeSFPScore(plaPieceCounts, oppPieceCounts);

    //Opp - Blockaded piece
    {
        double prop = BLOCKADEPINMOBLOSS / 100.0;
        oppPieceCounts[b.pieces[pinnedLoc]] -= prop;
        oppPieceCounts[0] -= prop;
    }

    //Pla - Holders
    Bitmap holderMap = threat.holderMap;
    while (holderMap.hasBits())
    {
        loc_t loc = holderMap.nextBit();
        if (b.owners[loc] != pla)
            continue;
        double prop = getBlockadeMobLoss(pla, loc);
        plaPieceCounts[b.pieces[loc]] -= prop;
        plaPieceCounts[0] -= prop;
    }

    //Compute rotations
    Bitmap rotatableMap = threat.holderMap & ~b.pieceMaps[opp][0] & ~b.pieceMaps[pla][RAB] & ~b.pieceMaps[pla][CAT];
    int numRotatableMax = rotatableMap.countBits() * 2;


    loc_t* holderRotLocs = new loc_t[numRotatableMax];
    int* holderRotDists = new int[numRotatableMax];
    loc_t* holderRotBlockers = new loc_t[numRotatableMax];
    bool* holderIsSwarm = new bool[numRotatableMax];
    int* minStrNeededArr = new int[numRotatableMax];

    //loc_t holderRotLocs[numRotatableMax];
    //int holderRotDists[numRotatableMax];
    //loc_t holderRotBlockers[numRotatableMax];
    //bool holderIsSwarm[numRotatableMax];
    //int minStrNeededArr[numRotatableMax];

    int numRotatable = fillBlockadeRotationArrays(b, pla, holderRotLocs, holderRotDists, holderRotBlockers, holderIsSwarm, minStrNeededArr,
        Bitmap::makeLoc(pinnedLoc), threat.holderMap & b.pieceMaps[pla][0], rotatableMap, pStrongerMaps, ufDist);

    //Try rotations - find the best SFP score for holding the frame
    int bestSfpVal = computeSFPScore(plaPieceCounts, oppPieceCounts, advancementGood, b.pieces[threat.pinnedLoc]);
    for (int i = 0; i < numRotatable; i++)
    {
        loc_t loc = holderRotLocs[i];
        int rotDist = holderRotDists[i];
        loc_t blockerLoc = holderRotBlockers[i];
        if (rotDist >= rotationFactorLen)
            continue;
        ARIMAADEBUG(if (print) cout << "Rotate " << (int)loc << " in " << rotDist << " with " << (int)blockerLoc << endl;)

            double mobilityPropCost = getBlockadeMobLoss(pla, loc);
        double rotationProp = rotationFactor[rotDist] / ROTDIV;
        plaPieceCounts[b.pieces[loc]] += mobilityPropCost;
        plaPieceCounts[b.pieces[loc]] -= mobilityPropCost * rotationProp;
        if (blockerLoc != ERRORSQUARE)
            plaPieceCounts[b.pieces[blockerLoc]] -= mobilityPropCost * (1.0 - rotationProp);
        int sfpVal = computeSFPScore(plaPieceCounts, oppPieceCounts);
        if (blockerLoc != ERRORSQUARE)
            plaPieceCounts[b.pieces[blockerLoc]] += mobilityPropCost * (1.0 - rotationProp);
        plaPieceCounts[b.pieces[loc]] += mobilityPropCost * rotationProp;
        plaPieceCounts[b.pieces[loc]] -= mobilityPropCost;

        if (sfpVal > bestSfpVal)
            bestSfpVal = sfpVal;
    }

    delete[] holderRotLocs;
    delete[] holderRotDists;
    delete[] holderRotBlockers;
    delete[] holderIsSwarm;
    delete[] minStrNeededArr;
    return bestSfpVal;
}


Strat Eval::evalBlockade(const Board& b, pla_t pla, BlockadeThreat threat,
		const Bitmap pStrongerMaps[2][NUMTYPES], int ufDist[64], bool print)
{
  loc_t pinnedLoc = threat.pinnedLoc;

  //Compute best blockade SFP
  int baseSfpVal;
  double advancementGood[NUMTYPES];
  int bestSfpVal = getBlockadeBestSFP(b,pla,threat,pStrongerMaps,ufDist,&baseSfpVal,advancementGood,print);

  //Evaluate cost
  int blockadeVal = computeLogistic(bestSfpVal-baseSfpVal,BLOCKADE_SFP_LOGISTIC_DIV,3000);
  if(blockadeVal <= 0)
    blockadeVal = 0;

	double sfpAdvGoodFactor = computeSFPAdvGoodFactor(b,pla,threat.pinnedLoc,advancementGood);
	blockadeVal = (int)(blockadeVal * sfpAdvGoodFactor);

  //Adjust by breakSteps
  ufDist[pinnedLoc] += 8;
  int breakSteps = getBlockadeBreakDistance(b, pla, pinnedLoc, threat.holderMap, pStrongerMaps, ufDist);
  if(breakSteps < BLOCKADEBREAKSTEPSFACTORLEN)
    blockadeVal = blockadeVal * BLOCKADEBREAKSTEPSFACTOR[breakSteps] / 100;
  ufDist[pinnedLoc] -= 8;

  //Penalize for non-tight
  if(threat.tightness != BlockadeThreat::FULLBLOCKADE)
    blockadeVal = blockadeVal * 3 / 4;

  int symloc = pla == GOLD ? pinnedLoc : 63 - pinnedLoc;
  blockadeVal = blockadeVal * BLOCKADE_VALUE_PCT[symloc] / 100  - BLOCKADE_SUBTRACT[symloc];
  if(blockadeVal < 0)
    blockadeVal = 0;

  Strat strat(pla,blockadeVal,pinnedLoc,pinnedLoc,false,threat.holderMap & b.pieceMaps[pla][0]);

  ARIMAADEBUG(if(print)
  {
		cout << "Blockade " << strat << endl;
		cout << "baseSfpVal " << baseSfpVal << endl;
		cout << "bestSfpVal " << bestSfpVal << endl;
		cout << "breakSteps " << breakSteps << endl;
		cout << "blockadeVal " << blockadeVal << endl;
  	cout << "sfpAdvGoodFactor " << sfpAdvGoodFactor << endl;
  })

  return strat;
}



int Eval::getBlockadeBreakDistance(const Board& b, pla_t pla, loc_t bloc, Bitmap defenseMap,
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64])
{
  pla_t opp = OPP(pla);

  loc_t ktLoc = Board::ADJACENTTRAP[bloc];

  //For each square, compute ability to break, and take the min
  int minBreakDist = 16;
  while(defenseMap.hasBits())
  {
    loc_t loc = defenseMap.nextBit();
    int bs = 16;
    if(b.owners[loc] == opp)
    {
      if(b.isOpenToStep(loc))
        bs = ufDist[loc]+3;
      else if(b.isOpenToMove(loc))
        bs = ufDist[loc]+4;

      if(ktLoc != ERRORSQUARE && Board::ISADJACENT[ktLoc][loc])
      {
        int numAdjBloc = 0;
        for(int i = 0; i<4; i++)
        {
          loc_t loc2 = loc + Board::ADJOFFSETS[i];
          if(Board::ISADJACENT[loc2][bloc])
            numAdjBloc++;
        }
        if(numAdjBloc == 1)
          bs = 7;
      }
    }
    else if(b.owners[loc] == pla)
    {
      //Don't bother attacking pieces that are completely interior
      if(b.isBlocked(loc) && !b.isDominated(loc))
        continue;
      bs = Threats::attackDist(b,loc,5,pStrongerMaps,ufDist)+3;
      if(Board::ISTRAP[loc] && (!b.isTrapSafe2(opp,loc) || b.isBlocked(loc)))
        bs += 2;
      if(ktLoc != ERRORSQUARE && Board::ISADJACENT[ktLoc][loc])
      {
        int numAdjBloc = 0;
        for(int i = 0; i<4; i++)
        {
          loc_t loc2 = loc + Board::ADJOFFSETS[i];
          if(Board::ISADJACENT[loc2][bloc])
            numAdjBloc++;
        }
        if(numAdjBloc == 1)
          bs = 16;
      }
    }
    else
    {
      //Probably a square in the middle of a blockade that's actually quite okay to be open.
      if(!Board::ISTRAP[loc])
        continue;

      if(b.isTrapSafe2(opp,loc) && !b.isBlocked(loc))
        bs = 2;
      else
        bs = Threats::moveAdjDistUF(b,opp,loc,5,ufDist,pStrongerMaps,RAB)+3; //TODO questionable - do we need the piece to be unfrozen?
    }

    if(bs < minBreakDist)
      minBreakDist = bs;
  }

  if(minBreakDist < 0)
    minBreakDist = 0;

  return minBreakDist;
}



//TODO - add sfp logic
//Encournage E and M to move away and forward


//Value for holding an elephant
//ImmoTypes: Immo, Almost immo, CentralBlocked
//BreakDist small: -
//Opp advancement of non-rabbit pieces: -
//Opp advancement of rabbit pieces: +
//[immotype][loc]
//Top side is holder's side.
static const int E_IMMO_VAL[64] =
{
  4800,4200,3700,3800,3800,3700,4200,4800,
  4000,3250,3600,2900,2900,3600,3250,4000,
  3000,2200,1500,1000,1000,1500,2200,3000,
  2100,1300,1300, 800, 800,1300,1300,2100,
  2100,1300,1000, 800, 800,1000,1300,2100,
  2200,1500,1000, 700, 700,1000,1500,2200,
  2700,1900,1700,1200,1200,1700,1900,2700,
  3200,2500,2000,1800,1800,2000,2500,3200,
};

//Out of 100
//[isholderturn*4 + steps][breaksteps]
static const int IMMO_BREAKSTEPS_FACTOR[8][9] =
{
    {50,55,62,59,76,87,94,102,107},
    {52,57,63,69,79,88,95,102,107},
    {56,63,66,73,81,89,96,102,107},
    {58,63,69,76,83,90,96,102,107},
    {62,67,74,82,86,91,96,102,107},
    {61,65,72,80,85,91,96,102,107},
    {59,64,70,78,84,90,96,102,107},
    {58,63,69,74,82,90,96,102,107},
};

//[goalydist], from perspective of opp, counts rabbits recursed on
static const int OPP_RAB_IN_BLOCKADE_BONUS[8] =
{0,100,200,130,110,110,150,50};

//[mdist+ydist+ADV_RABBIT_XDIST_BONUS[xdist] from ele]
static const int RELEVANT_ELE_ADVANCE_RADIUS = 7;
//[x1-x2+7]
static const int ADV_RABBIT_XDIST_BONUS[15] =
{5,4,3,2,1,0,0,0,0,0,1,2,3,4,5};
static const int ADV_RABBIT_YDIST_BONUS[15] =
{7,6,5,4,3,2,1,0,1,2,3,4,5,6,7};
static const int BLOCKADE_ADVANCE_RABBIT_BONUS[32] =
{144,144,144,144,140, 130,120,100,80, 64,48,34,22, 11,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0};
static const int BLOCKADE_ADVANCE_MHH_PENALTY[32] =
{200,200,200,200,194, 182,168,140,118, 96,74,52,32, 16,0,0,0, 0,0,0,0,  0,0,0,0, 0,0,0,0, 0,0,0};

//TODO expand to account for inner?
//TODO expand to account for location?
//Basic cost per pla holder in blockade, by numstronger, 7 = rabbit
static const int PLA_HOLDER_COST[8] = {160,90,65,50,45,40,35,30};

//Factor for having few leftover pieces [numpieces*2]
static const int NUM_LEFTOVER_PIECES_FACTOR[34] =
{0,0, 0,0,10,16, 22,36,50,60, 70,76,82,89, 95,97,99,100, 100,100,100,100, 100,100,100,100, 100,100,100,100, 100,100,100,100};


//[E][M][m]
static const int EMm_BLOCKADE_SFP_FACTOR[2][4][4] =
{
 //E not free
 {{30,15,5,5},{100,30,15,15},{120,50,30,30},{120,50,30,30}},
 //E free
 {{110,98,85,85},{120,101,87,87},{130,110,90,90},{130,110,90,90}},
};

//[Rotationsteps * 2, with some bonus values added for weak breakdist, whose turn it is etc]
static const int ROTATION_INTERP[33] =
{99, 99,99,98,97, 95,92,89,86, 84,81,78,75, 71,67,63,60, 56,52,48,45, 41,37,33,30, 26,22,18,14, 10,6,2,0};
static const int NUM_LEFTOVER_PIECES_ROTATE_MULTI_ADD[34] =
{33,33, 33,26,19,15, 11,9,7,5, 4,3,2,1, 1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

//Make advancing be "farther" from the blockade to EDISTANCE and MDISTANCE bonuses
static const int DISTANCE_ADJ[2][64] =
{{
 3,3,3,3,3,3,3,3,
 3,3,3,3,3,3,3,3,
 1,2,3,4,4,3,2,1,
 1,1,2,3,3,2,1,1,
 0,1,2,2,2,2,1,0,
 0,1,2,3,3,2,1,0,
 0,0,1,1,1,1,0,0,
 0,0,0,0,0,0,0,0,
},
{
 0,0,0,0,0,0,0,0,
 0,0,1,1,1,1,0,0,
 0,1,2,3,3,2,1,0,
 0,1,2,2,2,2,1,0,
 1,1,2,3,3,2,1,1,
 1,2,3,4,4,3,2,1,
 3,3,3,3,3,3,3,3,
 3,3,3,3,3,3,3,3,
}};


static const int EDISTANCE_FACTOR[21] =
{0, 84,84,84,86, 90,95,100,103, 105,106,106,106, 106,106,106,106, 106,106,106,106};
static const int EDISTANCE_BONUS[21] =
{0, 0,0,0,60,  105,145,185,200, 205,205,205,205, 205,205,205,205, 205,205,205,205,};
static const int MDISTANCE_BONUS[21] =
{0, 0,0,25,45, 65,85,95,100, 100,100,100,100, 100,100,100,100, 100,100,100,100};


int Eval::evalEleBlockade(Board& b, pla_t mainPla, pla_t pla, const int pStronger[2][NUMTYPES], const Bitmap pStrongerMaps[2][NUMTYPES],
    const int tc[2][4], int ufDist[64], bool print)
{
  int immoType;
  Bitmap recursedMap;
  Bitmap holderHeldMap;
  loc_t oppEleLoc;
  {
    Bitmap freezeHolderMap;
    Bitmap freezeHeldMap;
    oppEleLoc = Strats::findEBlockade(b,pla,ufDist,immoType,recursedMap,holderHeldMap,freezeHeldMap);
    if(oppEleLoc == ERRORSQUARE)
      return 0;
    while(freezeHeldMap.hasBits())
    {
      loc_t loc = freezeHeldMap.nextBit();
      loc_t bestHolder = ERRORSQUARE;
      if(CS1(loc) && b.owners[loc-8] == pla && b.pieces[loc-8] > b.pieces[loc])
      {if(bestHolder == ERRORSQUARE || (holderHeldMap.isOne(loc-8) && !holderHeldMap.isOne(bestHolder)) || b.pieces[bestHolder] > b.pieces[loc-8] ) bestHolder = loc-8;}
      if(CW1(loc) && b.owners[loc-1] == pla && b.pieces[loc-1] > b.pieces[loc])
      {if(bestHolder == ERRORSQUARE || (holderHeldMap.isOne(loc-1) && !holderHeldMap.isOne(bestHolder)) || b.pieces[bestHolder] > b.pieces[loc-1] ) bestHolder = loc-1;}
      if(CE1(loc) && b.owners[loc+1] == pla && b.pieces[loc+1] > b.pieces[loc])
      {if(bestHolder == ERRORSQUARE || (holderHeldMap.isOne(loc+1) && !holderHeldMap.isOne(bestHolder)) || b.pieces[bestHolder] > b.pieces[loc+1] ) bestHolder = loc+1;}
      if(CN1(loc) && b.owners[loc+8] == pla && b.pieces[loc+8] > b.pieces[loc])
      {if(bestHolder == ERRORSQUARE || (holderHeldMap.isOne(loc+8) && !holderHeldMap.isOne(bestHolder)) || b.pieces[bestHolder] > b.pieces[loc+8] ) bestHolder = loc+8;}
      holderHeldMap.setOn(bestHolder);
      freezeHolderMap.setOn(bestHolder);
    }
  }
  pla_t opp = OPP(pla);

  //Pieces that are threatening to break the blockade but are frozen
  /*
  Bitmap specialFreezeHolderMap;
  {
    Bitmap specialFreezeHeldMap = b.frozenMap & pStrongerMaps[pla][RAB] & Bitmap::adj(holderHeldMap) & ~holderHeldMap;
    if(specialFreezeHeldMap.hasBits())
    {
      while(specialFreezeHeldMap.hasBits())
      {
        //Make sure it's actually dominating something in the blockade
        loc_t loc = fhmap.nextBit();
        if((holderHeldMap & b.pieceMaps[pla][0] & ~pStrongerMaps[opp][b.pieces[loc]-1] & Board::RADIUS[1][loc]).isEmpty())
        {specialFreezeHeldMap.setOff(loc); continue;}

        loc_t bestHolder = ERRORSQUARE;
        if(CS1(loc) && b.owners[loc-8] == pla && b.pieces[loc-8] > b.pieces[loc])
        {if(bestHolder == ERRORSQUARE || (holderHeldMap.isOne(loc-8) && !holderHeldMap.isOne(bestHolder)) || b.pieces[bestHolder] > b.pieces[loc-8] ) bestHolder = loc-8;}
        if(CW1(loc) && b.owners[loc-1] == pla && b.pieces[loc-1] > b.pieces[loc])
        {if(bestHolder == ERRORSQUARE || (holderHeldMap.isOne(loc-1) && !holderHeldMap.isOne(bestHolder)) || b.pieces[bestHolder] > b.pieces[loc-1] ) bestHolder = loc-1;}
        if(CE1(loc) && b.owners[loc+1] == pla && b.pieces[loc+1] > b.pieces[loc])
        {if(bestHolder == ERRORSQUARE || (holderHeldMap.isOne(loc+1) && !holderHeldMap.isOne(bestHolder)) || b.pieces[bestHolder] > b.pieces[loc+1] ) bestHolder = loc+1;}
        if(CN1(loc) && b.owners[loc+8] == pla && b.pieces[loc+8] > b.pieces[loc])
        {if(bestHolder == ERRORSQUARE || (holderHeldMap.isOne(loc+8) && !holderHeldMap.isOne(bestHolder)) || b.pieces[bestHolder] > b.pieces[loc+8] ) bestHolder = loc+8;}
        specialFreezeHolderMap.setOn(bestHolder);
      }
      holderHeldMap |= specialFreezeHolderMap | specialFreezeHeldMap;
    }
  }*/

  int baseValue = E_IMMO_VAL[pla == GOLD ? oppEleLoc : 63 - oppEleLoc];
  if(pla == OPP(mainPla) && (immoType == Strats::TOTAL_IMMO || immoType == Strats::ALMOST_IMMO))
    baseValue = baseValue * 11 / 9;

  //Adjust by breakSteps
  Bitmap heldImmoMap = holderHeldMap & b.pieceMaps[opp][0] & ~b.frozenMap;
  while(heldImmoMap.hasBits())
    ufDist[heldImmoMap.nextBit()] += 8;
  int breakSteps = getBlockadeBreakDistance(b, pla, oppEleLoc, holderHeldMap | recursedMap, pStrongerMaps, ufDist);
  heldImmoMap = holderHeldMap & b.pieceMaps[opp][0] & ~b.frozenMap;
  while(heldImmoMap.hasBits())
    ufDist[heldImmoMap.nextBit()] -= 8;
  int breakStepsFactor = IMMO_BREAKSTEPS_FACTOR[(b.player == pla)*4 + b.step][breakSteps > 8 ? 8 : breakSteps];


  int plaHolderCost = 0;
  Bitmap plaHolderMap = holderHeldMap & b.pieceMaps[pla][0] & ~Bitmap::BMPTRAPS;;
  int numUsedPiecesX2 = plaHolderMap.countBits() * 2;
  //Add half the pieces around the trap, if there is one.
  Bitmap aroundTrapMap = Board::ADJACENTTRAP[oppEleLoc] == ERRORSQUARE || b.owners[Board::ADJACENTTRAP[oppEleLoc]] == opp ? Bitmap::BMPZEROS
      : (Board::DISK[1][Board::ADJACENTTRAP[oppEleLoc]] & b.pieceMaps[pla][0] & ~plaHolderMap);
  numUsedPiecesX2 += aroundTrapMap.countBitsIterative();
  int numLeftoverPiecesX2 = b.pieceCounts[pla][0]*2 - numUsedPiecesX2;
  int leftoverFactor = NUM_LEFTOVER_PIECES_FACTOR[numLeftoverPiecesX2];
  while(plaHolderMap.hasBits())
  {
    loc_t loc = plaHolderMap.nextBit();
    int idx = b.pieces[loc] == RAB ? 7 : pStronger[pla][b.pieces[loc]];
    plaHolderCost += PLA_HOLDER_COST[idx];
  }
  while(aroundTrapMap.hasBits())
  {
    loc_t loc = aroundTrapMap.nextBit();
    int idx = b.pieces[loc] == RAB ? 7 : pStronger[pla][b.pieces[loc]];
    plaHolderCost += PLA_HOLDER_COST[idx]/2;
  }

  int oppRabInBlockadeBonus = 0;
  Bitmap oppRabInBlockadeMap = b.pieceMaps[opp][RAB] & recursedMap;
  while(oppRabInBlockadeMap.hasBits())
    oppRabInBlockadeBonus += OPP_RAB_IN_BLOCKADE_BONUS[7-Board::GOALYDIST[pla][oppRabInBlockadeMap.nextBit()]];

  int pieceAdvancementPenalty = 0;
  int rabbitAdvancementBonus = 0;
  loc_t center;
  if(Board::GOALYDIST[pla][oppEleLoc] < 4)
    center = oppEleLoc - Board::GOALLOCINC[pla];
  else
    center = oppEleLoc;
  Bitmap advancementMap =
      (Board::DISK[RELEVANT_ELE_ADVANCE_RADIUS][center] & ((pStrongerMaps[pla][CAT] & ~holderHeldMap) | b.pieceMaps[opp][RAB]));
  while(advancementMap.hasBits())
  {
    loc_t loc = advancementMap.nextBit();
    int dist = Board::MANHATTANDIST[loc][center] + ADV_RABBIT_XDIST_BONUS[loc%8 - center%8 + 7] + ADV_RABBIT_YDIST_BONUS[loc/8 - center/8 + 7];
    if(b.pieces[loc] == RAB)
    {
      //Squares behind traps
      if(Bitmap(0x0042000000004200ULL).isOne(loc))
        dist += 2;
      if(print)
        cout << dist << endl;
      rabbitAdvancementBonus += BLOCKADE_ADVANCE_RABBIT_BONUS[dist];
      if(Board::ISTRAP[loc] && (pla == GOLD) == (loc < 32) && holderHeldMap.isOne(loc))
        rabbitAdvancementBonus += 40;
    }
    else if(pStronger[opp][b.pieces[loc]] == 1)
      pieceAdvancementPenalty += BLOCKADE_ADVANCE_MHH_PENALTY[dist];
    else if(pStronger[opp][b.pieces[loc]] == 2)
      pieceAdvancementPenalty += BLOCKADE_ADVANCE_MHH_PENALTY[dist] * 2 / 3;
    else if(pStronger[opp][b.pieces[loc]] <= 4)
      pieceAdvancementPenalty += BLOCKADE_ADVANCE_MHH_PENALTY[dist] * 1 / 3;
  }
  //Advancing rabbits is worse when the opponent really needs to make those replacements.
  rabbitAdvancementBonus = rabbitAdvancementBonus * 3 * (80+100-leftoverFactor) / 160;

  //Don't overvalue rabbit advancement bonus when the ele is not so advanced.
  const int RAB_ADV_BONUS_FACTOR_OUT_OF_16[8] =
  {16,16,14,10,7,6,5,2};
  rabbitAdvancementBonus = rabbitAdvancementBonus * RAB_ADV_BONUS_FACTOR_OUT_OF_16[Board::GOALYDIST[opp][oppEleLoc]] / 16;

  //Compute rotations
  Bitmap rotatableMap = holderHeldMap;
  if(immoType >= Strats::CENTRAL_BLOCK) rotatableMap &= (b.pieceMaps[pla][ELE] | b.pieceMaps[pla][CAM]);
  else rotatableMap &= b.pieceMaps[pla][0] & ~b.pieceMaps[pla][RAB] & ~b.pieceMaps[pla][CAT];
  int numRotatableMax = rotatableMap.countBits()*2;
  int numRotatable;

  loc_t* holderRotLocs = new loc_t[numRotatableMax];
  int* holderRotDists = new int[numRotatableMax];
  loc_t* holderRotBlockers = new loc_t[numRotatableMax];
  bool* holderIsSwarm = new bool[numRotatableMax];
  int* minStrNeededArr = new int[numRotatableMax];

  //loc_t holderRotLocs[numRotatableMax];
  //int holderRotDists[numRotatableMax];
  //loc_t holderRotBlockers[numRotatableMax];
  //bool holderIsSwarm[numRotatableMax];
  //int minStrNeededArr[numRotatableMax];

  int sfpBaseFactor;
  int sfpRotatedFactor;
  int bestRotateLoc = 0;
  int bestRotateyness = 0;
  int mPiece = ELE;
  int hPiece = ELE;
  {
    int numEs = b.pieceCounts[pla][ELE] - (holderHeldMap & b.pieceMaps[pla][ELE]).hasBits();
    int numPlaMs = 0;
    int numOppMs = 0;
    int piece;
    for(piece = CAM; piece > RAB; piece--)
    {
      if(b.pieceCounts[pla][piece] > 0 || b.pieceCounts[opp][piece] > 0)
      {
        numPlaMs = b.pieceCounts[pla][piece];
        numOppMs = b.pieceCounts[opp][piece];
        piece--;
        if(b.pieceCounts[opp][piece+1] == 0)
        {
          while(b.pieceCounts[opp][piece] == 0 && piece > RAB)
          {numPlaMs += b.pieceCounts[pla][piece]; piece--;}
        }
        else if(b.pieceCounts[pla][piece+1] == 0)
        {
          while(b.pieceCounts[pla][piece] == 0 && piece > RAB)
          {numOppMs += b.pieceCounts[opp][piece]; piece--;}
        }
        break;
      }
    }
    mPiece = piece+1;

    //int numPlaHs = 0;
    //int numOppHs = 0;
    for(; piece > RAB; piece--)
    {
      if(b.pieceCounts[pla][piece] > 0 || b.pieceCounts[opp][piece] > 0)
      {
        //numPlaHs = b.pieceCounts[pla][piece];
        //numOppHs = b.pieceCounts[opp][piece];
        piece--;
        if(b.pieceCounts[opp][piece+1] == 0)
        {
          while(b.pieceCounts[opp][piece] == 0 && piece > RAB)
          {
            //numPlaHs += b.pieceCounts[pla][piece];
            piece--;
          }
        }
        else if(b.pieceCounts[pla][piece+1] == 0)
        {
          while(b.pieceCounts[pla][piece] == 0 && piece > RAB)
          {
            //numOppHs += b.pieceCounts[opp][piece];
            piece--;
          }
        }
        break;
      }
    }
    hPiece = piece+1;

    numPlaMs -= (holderHeldMap & pStrongerMaps[opp][mPiece-1] & ~b.pieceMaps[pla][ELE]).countBitsIterative();
    numOppMs -= (holderHeldMap & pStrongerMaps[pla][mPiece-1] & ~b.pieceMaps[opp][ELE]).countBitsIterative();
    //numPlaHs -= (holderHeldMap & pStrongerMaps[opp][hPiece-1] & ~pStrongerMaps[opp][mPiece-1]).countBitsIterative();
    //numOppHs -= (holderHeldMap & pStrongerMaps[pla][hPiece-1] & ~pStrongerMaps[pla][mPiece-1]).countBitsIterative();

    if(numPlaMs < 0) numPlaMs = 0;
    if(numOppMs < 0) numOppMs = 0;
    if(numPlaMs > 3) numPlaMs = 3;
    if(numOppMs > 3) numOppMs = 3;

    sfpBaseFactor = EMm_BLOCKADE_SFP_FACTOR[numEs][numPlaMs][numOppMs];
    sfpRotatedFactor = sfpBaseFactor;

    numRotatable = fillBlockadeRotationArrays(b,pla,holderRotLocs,holderRotDists,holderRotBlockers,holderIsSwarm,minStrNeededArr,
        holderHeldMap & b.pieceMaps[opp][0], holderHeldMap & b.pieceMaps[pla][0],rotatableMap & pStrongerMaps[opp][hPiece-1], pStrongerMaps, ufDist);

    for(int i = 0; i<numRotatable; i++)
    {
      loc_t rotLoc = holderRotLocs[i];
      int otherSFPFactor = sfpBaseFactor;
      piece_t piece = b.pieces[rotLoc];
      piece_t blockerPiece = holderRotBlockers[i] == ERRORSQUARE ? EMP : b.pieces[holderRotBlockers[i]];
      if(holderIsSwarm[i])
      {
        piece_t strongestAdj = b.strongestAdjacentPla(pla,rotLoc);
        if(strongestAdj > blockerPiece)
          blockerPiece = strongestAdj;
      }
      if(piece == ELE)
      {
        if(blockerPiece >= mPiece)
          otherSFPFactor = EMm_BLOCKADE_SFP_FACTOR[numEs+1][max(numPlaMs-1,0)][numOppMs];
        else
          otherSFPFactor = EMm_BLOCKADE_SFP_FACTOR[numEs+1][numPlaMs][numOppMs];
      }
      else if(piece >= mPiece)
      {
        if(blockerPiece < mPiece)
          otherSFPFactor = EMm_BLOCKADE_SFP_FACTOR[numEs][min(numPlaMs+1,2)][numOppMs];
      }
      else continue;

      int rotateDist = holderRotDists[i] * 2;
      //if((pStrongerMaps[pla][blockerPiece] & ~holderHeldMap & Board::DISK[1][rotLoc]).hasBits())
        rotateDist += 2;
      //if((pStrongerMaps[pla][blockerPiece] & ~holderHeldMap & Board::DISK[3][rotLoc]).hasBits())
      //  rotateDist += 1;
      if(holderIsSwarm[i] && numLeftoverPiecesX2 <= 0)
        continue;
      else if(holderIsSwarm[i])
        rotateDist += NUM_LEFTOVER_PIECES_ROTATE_MULTI_ADD[numLeftoverPiecesX2];

      if(b.player == pla)
        rotateDist -= (3-b.step);
      if(rotateDist < 0)
        rotateDist = 0;
      if(rotateDist > 32)
        rotateDist = 32;

      int lambda = ROTATION_INTERP[rotateDist];
      int fctr = (otherSFPFactor * lambda + sfpBaseFactor * (100-lambda))/100;

      if(holderIsSwarm[i])
        fctr = fctr * NUM_LEFTOVER_PIECES_FACTOR[numLeftoverPiecesX2-2] / (NUM_LEFTOVER_PIECES_FACTOR[numLeftoverPiecesX2] + 1);
      if(fctr > sfpRotatedFactor)
      {
        sfpRotatedFactor = fctr;
        bestRotateLoc = rotLoc;
        bestRotateyness = rotateDist;
      }
    }
  }

  int eDistFactor = 100;
  int distBonus = 0;
  Bitmap map = b.pieceMaps[pla][ELE];
  while(map.hasBits())
  {
    loc_t loc = map.nextBit();
    int dist = DISTANCE_ADJ[pla][loc] + Board::MANHATTANDIST[loc][oppEleLoc];
    distBonus += EDISTANCE_BONUS[dist];
    eDistFactor = EDISTANCE_FACTOR[dist];
  }
  for(int piece = CAM; piece >= mPiece && piece >= RAB; piece--)
  {
    Bitmap map = b.pieceMaps[pla][piece];
    while(map.hasBits())
    {
      loc_t loc = map.nextBit();
      distBonus += MDISTANCE_BONUS[DISTANCE_ADJ[pla][loc] + Board::MANHATTANDIST[loc][oppEleLoc]];
    }
  }

  int tc0 = tc[pla][Board::PLATRAPINDICES[opp][0]];
  int tc1 = tc[pla][Board::PLATRAPINDICES[opp][1]];
  int tc2 = tc[pla][Board::PLATRAPINDICES[pla][0]];
  int tc3 = tc[pla][Board::PLATRAPINDICES[pla][1]];
  int tcBonus = 60 + (tc0+tc1+tc2/2+tc3/2) +
      ((tc0 <= 0 ? 0 : tc0 * tc0) +
       (tc1 <= 0 ? 0 : tc1 * tc1) -
       (tc2 >= 0 ? 0 : tc2 * tc2) -
       (tc3 >= 0 ? 0 : tc3 * tc3))/30;

  int value = (baseValue + oppRabInBlockadeBonus - pieceAdvancementPenalty + rabbitAdvancementBonus - plaHolderCost + distBonus + tcBonus)
  * breakStepsFactor * leftoverFactor / 10000 * eDistFactor * max(sfpRotatedFactor,0) / 10000;

  if(immoType == Strats::ALMOST_IMMO)
    value = (value - 200) * 6/7;
  else if(immoType == Strats::CENTRAL_BLOCK)
    value = (value - 500) / 9 + distBonus/3 + tcBonus/2;
  else if(immoType == Strats::CENTRAL_BLOCK_WEAK)
    value = (value - 500) / 15 + distBonus/4 + tcBonus/2;

  if(value < 0)
    value = 0;
  if(value > 4200) //Reduce super-high-valued blockades
    value = 4200 + (value - 4200)/2;

  ARIMAADEBUG(
  if(print)
  {
    cout << "--Blockade Type " << immoType << "--" << endl;
    cout << "Base value: " << baseValue << endl;
    cout << "SFP: " << sfpBaseFactor << endl;
    cout << "SFP Rotated: " << sfpRotatedFactor << " best loc " << bestRotateLoc << " " << bestRotateyness << endl;
    cout << "BreakSteps: " << breakSteps << " " << breakStepsFactor << endl;
    cout << "RabInBlockadeBonus: " << oppRabInBlockadeBonus << endl;
    cout << "RabAdvBonus: " << rabbitAdvancementBonus << endl;
    cout << "PieceAdvPenalty: " << pieceAdvancementPenalty << endl;
    cout << "PlaHolderCost: " << plaHolderCost << endl;
    cout << "NumLeftoverFactor: " << leftoverFactor << endl;
    cout << "Distbonus: " << distBonus << endl;
    cout << "EDistFactor: " << eDistFactor << endl;
    cout << "TCbonus: " << tcBonus << endl;
    cout << "Final: " << value << endl;

    for(int i = 0; i<numRotatable; i++)
    {
      loc_t loc = holderRotLocs[i];
      int rotDist = holderRotDists[i];
      loc_t blockerLoc = holderRotBlockers[i];
      if(rotDist >= rotationFactorLen)
        continue;
      ARIMAADEBUG(if(print) cout << "Rotate " << (int)loc << " in " << rotDist  << " with " << (int)blockerLoc << endl;)
    }
  }
  )

  delete[] holderRotLocs;
  delete[] holderRotDists;
  delete[] holderRotBlockers;
  delete[] holderIsSwarm;
  delete[] minStrNeededArr;

  return value;
}


//Assumes nonrabbit
/*
void Eval::getMobility(const Board& b, pla_t pla, loc_t ploc, const Bitmap pStrongerMaps[2][NUMTYPES],
		const int ufDist[64], Bitmap mobMap[5])
{
	mobMap[0] = Bitmap();
	mobMap[0].setOn(ploc);
	int i;
	for(i = 1; i <= ufDist[ploc] && i < 5; i++)
	{
		mobMap[i] = mobMap[i-1];
	}
	int base = i;
	pla_t opp = OPP(pla);
	Bitmap plaMap = b.pieceMaps[pla][0];
	Bitmap blocked;
	for(int x = 0; x<64; x++)
	{
		int openCount =
				(CS1(x) && b.owners[x-8] == NPLA && (b.owners[x] != pla || b.isRabOkayS(pla,x))) +
				(CW1(x) && b.owners[x-1] == NPLA) +
				(CE1(x) && b.owners[x+1] == NPLA) +
				(CN1(x) && b.owners[x+8] == NPLA && (b.owners[x] != pla || b.isRabOkayN(pla,x)));
		if((openCount == 1 && !b.ISADJACENT[x][ploc]) || openCount == 0)
			blocked.setOn(x);
	}
	Bitmap oppMap = b.pieceMaps[opp][0];
	Bitmap opp1sMap = oppMap & ~pStrongerMaps[pla][b.pieces[ploc]-1];
	Bitmap emptyMap = Bitmap::BMPONES & ~plaMap & ~oppMap;

	Bitmap trap1s;
	Bitmap trap2s;
	for(int j = 0; j<4; j++)
	{
		loc_t trap = Board::TRAPLOCS[j];
		int trapDefCount =
				(b.owners[trap-8] == pla) +
				(b.owners[trap-1] == pla) +
				(b.owners[trap+1] == pla) +
				(b.owners[trap+8] == pla);
		trapDefCount -= Board::ISADJACENT[trap][ploc];
		if(trapDefCount == 0)
		{
			bool canDef1 =
				(b.owners[trap-8] == NPLA && b.canStepAndOccupy(pla,trap-8)) ||
				(b.owners[trap-1] == NPLA && b.canStepAndOccupy(pla,trap-1)) ||
				(b.owners[trap+1] == NPLA && b.canStepAndOccupy(pla,trap+1)) ||
				(b.owners[trap+8] == NPLA && b.canStepAndOccupy(pla,trap+8));
			if(!canDef1)
				trap2s.setOn(trap);
			trap1s.setOn(trap);
		}
	}
	Bitmap block1s = ((opp1sMap | plaMap) & ~blocked) | (emptyMap & trap1s & ~trap2s);

	for(; i < 5; i++)
	{
		mobMap[i] = mobMap[i-1];
		mobMap[i] |= Bitmap::adj(mobMap[i-1]) & emptyMap & ~trap1s;
		if(i > base)
		{
			mobMap[i] |= Bitmap::adj(mobMap[i-2]) & block1s;
		}
	}
}

eval_t Eval::getEMobilityScore(const Board& b, pla_t pla, const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], bool print)
{
	Bitmap map = b.pieceMaps[pla][ELE];
	if(!map.hasBits())
		return 0;
	loc_t eleLoc = map.nextBit();

	Bitmap mobMap[5];
	getMobility(b,pla,eleLoc,pStrongerMaps,ufDist,mobMap);

  ARIMAADEBUG(if(print)
	{
		cout << "Ele " << (int)pla << " Mobility "
				<< mobMap[1].countBits() << " "
				<< mobMap[2].countBits() << " "
				<< mobMap[3].countBits() << " "
				<< mobMap[4].countBits() << endl;
	})

	//int eleMob3 = mobMap[3].countBits();
	//int eleMob4 = mobMap[4].countBits();

	return 0;
	//return (eleMob3 + eleMob4) * 2;
}
*/

//FINAL-----------------------------------------------------------------------

void Eval::getStrats(Board& b, pla_t mainPla, const eval_t pValues[2][NUMTYPES], const int pStronger[2][NUMTYPES],
		const Bitmap pStrongerMaps[2][NUMTYPES], int ufDist[64], const int tc[2][4],
		eval_t pieceThreats[64], int numPStrats[2], Strat pStrats[2][numStratsMax], eval_t stratScore[2], bool print)
{
  FrameThreat pFrames[2][frameThreatMax];
  HostageThreat pHostages[2][hostageThreatMax];
  int numPFrames[2] = {0,0};
  int numPHostages[2] = {0,0};

  for(pla_t p = 0; p<2; p++)
	{
  	for(int i = 0; i<4; i++) numPFrames[p] += Strats::findFrame(b,p,Board::TRAPLOCS[i],pFrames[p]+numPFrames[p]);
  	for(int i = 0; i<2; i++) numPHostages[p] += Strats::findHostages(b,p,Board::PLATRAPLOCS[p][i],pHostages[p]+numPHostages[p],ufDist);
	}

  numPStrats[0] = 0;
  numPStrats[1] = 0;

  for(pla_t p = 0; p<2; p++)
  {
    for(int i = 0; i<numPFrames[p]; i++) pStrats[p][numPStrats[p]++] = Eval::evalFrame(b,p,pFrames[p][i],pValues,pStrongerMaps,ufDist,false);
    for(int i = 0; i<numPHostages[p]; i++) pStrats[p][numPStrats[p]++] = Eval::evalHostage(b,p,pHostages[p][i],pValues,pStrongerMaps,ufDist,tc,false);
  }

  //Compute value if all strats are used
  int eleBlockadeEval[2];
  eleBlockadeEval[SILV] = evalEleBlockade(b, mainPla, SILV, pStronger, pStrongerMaps, tc, ufDist, print);
  eleBlockadeEval[GOLD] = evalEleBlockade(b, mainPla, GOLD, pStronger, pStrongerMaps, tc, ufDist, print);
  int eleBlockOverlap[2] = {0,0};

  stratScore[0] = 0;
  stratScore[1] = 0;
  for(pla_t p = 0; p<2; p++)
  {
  	for(int i = 0; i<numPStrats[p]; i++)
  	{
  		Strat& strat = pStrats[p][i];
  		int valueDiff = strat.value - pieceThreats[strat.pieceLoc];
  		if(valueDiff <= 0)
  			valueDiff = 0;
  		strat.value = valueDiff;
  		pieceThreats[strat.pieceLoc] += valueDiff;
  		stratScore[p] += valueDiff;

  		//Reduce ele blockade eval in the case that other strats overlap and also pin the ele
  		if(strat.wasHostage) eleBlockOverlap[OPP(p)] += pValues[OPP(p)][b.pieces[strat.pieceLoc]] / 6;
  		else eleBlockOverlap[p] += b.pieces[strat.pieceLoc] == RAB ? 0 : pieceThreats[strat.pieceLoc] * 3/4;
  	}
  }

  stratScore[0] += eleBlockadeEval[0]/6 + max(0,eleBlockadeEval[0]*5/6 - eleBlockOverlap[0]);
  stratScore[1] += eleBlockadeEval[1]/6 + max(0,eleBlockadeEval[1]*5/6 - eleBlockOverlap[1]);
}



//CAPS--------------------------------------------------------------------------

eval_t Eval::evalCaps(Board& b, pla_t pla, int numSteps, const eval_t pValues[2][NUMTYPES],
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], const eval_t pieceThreats[64],
		const int numPStrats[2], const Strat pStrats[2][numStratsMax], eval_t& retRaw, loc_t& retCapLoc)
{
  //Find the biggest captures
	pla_t opp = OPP(pla);
  const int capThreatMax = 48;
  CapThreat plaCapThreats[capThreatMax];
  int numPlaCapThreats = 0;

  for(int i = 0; i<4; i++)
  {
    loc_t kt = Board::TRAPLOCS[i];
    int rdSteps;
    int pt = Threats::findCapThreats(b,pla,kt,plaCapThreats+numPlaCapThreats,ufDist,pStrongerMaps,4-b.step,capThreatMax-numPlaCapThreats,rdSteps);
    numPlaCapThreats += pt;
  }

	//Simulate making of the best capture
	loc_t bestCapLoc = ERRORSQUARE;
	int bestNetGain = 0;
	int bestRaw = 0;
	for(int i = 0; i<numPlaCapThreats; i++)
	{
		CapThreat& threat = plaCapThreats[i];
		if(threat.dist >= 5)
			continue;
		int rawValue = Eval::evalCapOccur(b,pValues[opp],threat.oloc,threat.dist);
		int gain = rawValue - pieceThreats[threat.oloc];
		int loss = 0;
		if(threat.ploc != ERRORSQUARE)
		{
			for(int j = 0; j<numPStrats[pla]; j++)
			{
				const Strat& strat = pStrats[pla][j];
				if(strat.pieceLoc == threat.oloc)
					continue;
				if(strat.holdingKT != ERRORSQUARE || strat.holdingKT != threat.kt)
				{
					if(strat.plaHolders.isOne(threat.ploc))
						loss += strat.value;
				}
			}
		}
		DEBUGASSERT(numSteps - threat.dist >= 0);
		loss += SearchParams::STEPS_LEFT_BONUS[numSteps] - SearchParams::STEPS_LEFT_BONUS[numSteps - threat.dist];
		int netGain = gain-loss;
		if(netGain > bestNetGain)
		{
			bestNetGain = netGain;
			bestRaw = rawValue;
			bestCapLoc = threat.oloc;
		}
	}

	retRaw = bestRaw;
	retCapLoc = bestCapLoc;
	return bestNetGain;
}







