
/*
 * featuremove.cpp
 * Author: davidwu
 */
#include "pch.h"

#define BOOST_STATIC_ASSERT(x) ;

#include <algorithm>
#include "board.h"
#include "boardmovegen.h"
#include "boardtrees.h"
#include "eval.h"
#include "threats.h"
#include "feature.h"
#include "featuremove.h"
#include "featurearimaa.h"
#include "searchutils.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;

//FEATURESET-----------------------------------------------------------------------------------------

static FeatureSet fset;

//PARAMS----------------------------------------------------------------------------------------------
static const int NUMPIECEINDICES = 8;

//BASE------------------------------------------------------------------------------------------------

//Prior feature
static fgrpindex_t PRIOR;

//Pass feature
static fgrpindex_t PASS;

//SRCS AND DESTS--------------------------------------------------------------------------------------
static const bool ENABLE_SRCDEST = true;

//Source locations of step sequence for pla/opp. (pieceindex 0-7)(symloc 0-31)
static fgrpindex_t SRC;
static fgrpindex_t SRC_O;
//Destination locations of step sequence for pla/opp. (pieceindex 0-7)(symloc 0-32)
static fgrpindex_t DEST;
static fgrpindex_t DEST_O;

//DOMINATION-----------------------------------------------------------------------------------
static const bool ENABLE_DOMINATE = true;

//Pla moves to dest square that would dominate a piece (dominated pieceindex 0-7) or (symloc 0-31)
static fgrpindex_t DOMINATES_STR;
static fgrpindex_t DOMINATES_AT;

//Pla moves to dest square that would become dominated (plapieceindex 0-7) or (symloc 0-31)
static fgrpindex_t DOMINATED_STR;
static fgrpindex_t DOMINATED_AT;

//FREEZING-----------------------------------------------------------------------------------
static const bool ENABLE_FREEZE = true;

//Moves frozen pla piece (plapieceindex 0-7) or (symloc 0-31)
static fgrpindex_t MOVES_FROZEN_STR;
static fgrpindex_t MOVES_FROZEN_AT;

//CAPTHREATENED-----------------------------------------------------------------------------------
static const bool ENABLE_CAPTHREATENED = true;

//Moves frozen pla piece (isplatrap 0-1) (plapieceindex 0-7)  or (isplatrap 0-1) (symloc 0-31)
static fgrpindex_t MOVES_CAPTHREATENED_STR;
static fgrpindex_t MOVES_CAPTHREATENED_AT;

//Defends capthreated trap (isplatrap 0-1) (newdefcount 0-2) (opptrapstate 0-7) (plapieceindex 0-7) or (isplatrap 0-1) (symloc 0-31)
static fgrpindex_t DEFENDS_CAPTHREATED_TRAP_STR;
static fgrpindex_t DEFENDS_CAPTHREATED_TRAP_AT;

//LIKELY CAPTURES-------------------------------------------------------------------------------
static const bool ENABLE_LIKELY_CAPTHREAT = true;

//(isplatrap 0-1)
static fgrpindex_t LIKELY_CAPDANGER_ADJ;
static fgrpindex_t LIKELY_CAPDANGER_ADJ2;
static fgrpindex_t LIKELY_CAPTRAPDEF;

static fgrpindex_t LIKELY_CAPHANG;

static fgrpindex_t LIKELY_CAPTHREAT_STATIC;
static fgrpindex_t LIKELY_CAPTHREAT_LOOSE;
static fgrpindex_t LIKELY_CAPTHREAT_PUSHPULL;
static fgrpindex_t LIKELY_CAPTHREAT_PUSHPULL_LOOSE;

//RABBIT GOAL DIST-------------------------------------------------------------------------------
//static const bool ENABLE_RABGDIST = true;

//Advances rabbit (distestimate 0-9)
//static fgrpindex_t RAB_GOAL_DIST_SRC;
//static fgrpindex_t RAB_GOAL_DIST_DEST;

//MOVE DEPENDENCY--------------------------------------------------------------------------------
static const bool ENABLE_DEPENDENCY = true;

//How many of the steps of this move were independent? (numtotal 0-4)(numindep 0-4)
static fgrpindex_t NUM_INDEP_STEPS;


//Helpers---------------------------------------------------------------------------------------
static void initPriors();

const FeatureSet& MoveFeatureLite::getFeatureSet()
{
	return fset;
}

ArimaaFeatureSet MoveFeatureLite::getArimaaFeatureSetSrcDestOnly()
{
	return ArimaaFeatureSet(&fset,MoveFeatureLite::getFeatures,MoveFeatureLite::getPosDataSrcDestOnly);
}

ArimaaFeatureSet MoveFeatureLite::getArimaaFeatureSet()
{
	return ArimaaFeatureSet(&fset,MoveFeatureLite::getFeatures,MoveFeatureLite::getPosData);
}

ArimaaFeatureSet MoveFeatureLite::getArimaaFeatureSetFullMove()
{
	return ArimaaFeatureSet(&fset,MoveFeatureLite::getFeatures,MoveFeatureLite::getPosDataFullMove);
}

bool MoveFeatureLite::IS_INITIALIZED = false;
void MoveFeatureLite::initFeatureSet()
{
	if(IS_INITIALIZED)
		return;

  PRIOR = fset.add("Prior");
  PASS = fset.add("Pass");

  if(ENABLE_SRCDEST)
  {
    SRC = fset.add("Src",NUMPIECEINDICES,Board::NUMSYMLOCS);
    SRC_O = fset.add("SrcO",NUMPIECEINDICES,Board::NUMSYMLOCS);
    DEST = fset.add("Dest",NUMPIECEINDICES,Board::NUMSYMLOCS+1);
    DEST_O = fset.add("DestO",NUMPIECEINDICES,Board::NUMSYMLOCS+1);
  }

  if(ENABLE_DOMINATE)
  {
  	DOMINATES_STR = fset.add("DominatesStr",NUMPIECEINDICES);
  	DOMINATES_AT = fset.add("DominatesAt",Board::NUMSYMLOCS);
  	DOMINATED_STR = fset.add("DominatedStr",NUMPIECEINDICES);
  	DOMINATED_AT = fset.add("DominatedAt",Board::NUMSYMLOCS);
  }

  if(ENABLE_FREEZE)
  {
  	MOVES_FROZEN_STR = fset.add("MovesFrozenStr",NUMPIECEINDICES);
  	MOVES_FROZEN_AT = fset.add("MovesFrozenAt",Board::NUMSYMLOCS);
  }

  if(ENABLE_CAPTHREATENED)
  {
  	MOVES_CAPTHREATENED_STR = fset.add("MovesCapthreatedStr",2,NUMPIECEINDICES);
  	MOVES_CAPTHREATENED_AT = fset.add("MovesCapthreatedAt",2,Board::NUMSYMLOCS);
  	DEFENDS_CAPTHREATED_TRAP_STR = fset.add("DefendsCapThreatedTrapStr",2,3,8,NUMPIECEINDICES);
  	DEFENDS_CAPTHREATED_TRAP_AT = fset.add("DefendsCapThreatedTrapAt",2,Board::NUMSYMLOCS);
  }

  if(ENABLE_LIKELY_CAPTHREAT)
  {
  	LIKELY_CAPDANGER_ADJ = fset.add("LikelyCapDangerAdj",2);
  	LIKELY_CAPDANGER_ADJ2 = fset.add("LikelyCapDangerAdj2",2);
  	LIKELY_CAPTRAPDEF = fset.add("LikelyCapTrapdef",2);

  	LIKELY_CAPHANG = fset.add("LikelyCapHang",2);

  	LIKELY_CAPTHREAT_STATIC = fset.add("LikelyCapthreatStatic");
  	LIKELY_CAPTHREAT_LOOSE = fset.add("LikelyCapthreatLoose");
  	LIKELY_CAPTHREAT_PUSHPULL = fset.add("LikelyCapthreatPushpull");
  	LIKELY_CAPTHREAT_PUSHPULL_LOOSE = fset.add("LikelyCapthreatPushpullLoose");
  }

  //if(ENABLE_RABGDIST)
  //{
  //	RAB_GOAL_DIST_SRC = fset.add("RabGoalDistSrc",10);
  //	RAB_GOAL_DIST_DEST = fset.add("RabGoalDistDest",10);
  //}

  if(ENABLE_DEPENDENCY)
  {
  	NUM_INDEP_STEPS = fset.add("NumIndepSteps",5,5);
  }

  initPriors();

  IS_INITIALIZED = true;
}

static void initPriors()
{
	//Add a prior over everything
	fset.setPriorIndex(fset.get(PRIOR));
	fset.addUniformPrior(3.0,3.0);
}


void MoveFeatureLite::getFeatures(const Board& b, const FeaturePosData& dataBuf, pla_t pla, move_t move, const BoardHistory& hist,
		void (*handleFeature)(findex_t,void*), void* handleFeatureData)
{
  //Handle Pass Move
  if(move == PASSMOVE || move == QPASSMOVE)
  {
    (*handleFeature)(fset.get(PASS),handleFeatureData);
    return;
  }

	const MoveFeatureLitePosData& data = MoveFeatureLitePosData::convert(dataBuf);

  //Source and destination piece movement features
	pla_t opp = OPP(pla);
  loc_t src[8];
  loc_t dest[8];
  int8_t newTrapGuardCounts[2][4];
  int numChanges = b.getChanges(move,src,dest,newTrapGuardCounts);
  for(int i = 0; i<numChanges; i++)
  {
    //Retrieve piece stats
    pla_t owner = b.owners[src[i]];
    piece_t piece = b.pieces[src[i]];
    int pieceIndex = data.pieceIndex[owner][piece];

    //SRCDEST features
    if(ENABLE_SRCDEST)
    {
      findex_t srcFeature;
      findex_t destFeature;
      if(owner == pla) {srcFeature = SRC; destFeature = DEST;}
      else             {srcFeature = SRC_O; destFeature = DEST_O;}

      (*handleFeature)(fset.get(srcFeature,pieceIndex,Board::SYMLOC[pla][src[i]]),handleFeatureData);
      (*handleFeature)(fset.get(destFeature,pieceIndex,(dest[i] == ERRORSQUARE) ? 32 : Board::SYMLOC[pla][dest[i]]),handleFeatureData);
    }
  }

  if(data.srcDestOnly)
  	return;

	Bitmap moved;
  for(int i = 0; i<numChanges; i++)
  {
    pla_t owner = b.owners[src[i]];
    piece_t piece = b.pieces[src[i]];
    int pieceIndex = data.pieceIndex[owner][piece];
    moved.setOn(src[i]);

    if(owner == pla && dest[i] != ERRORSQUARE)
    {
      if(ENABLE_DOMINATE)
  		{
  			if(piece < data.oppMaxStrAdj[dest[i]])
  			{
  				(*handleFeature)(fset.get(DOMINATED_STR,pieceIndex),handleFeatureData);
  				(*handleFeature)(fset.get(DOMINATED_AT,Board::SYMLOC[pla][dest[i]]),handleFeatureData);
  			}
  			if(piece > data.oppMinStrAdj[dest[i]])
  			{
  				(*handleFeature)(fset.get(DOMINATES_STR,data.pieceIndex[opp][data.oppMinStrAdj[dest[i]]]),handleFeatureData);
  				(*handleFeature)(fset.get(DOMINATES_AT,Board::SYMLOC[pla][dest[i]]),handleFeatureData);
  			}
  		}

			if(ENABLE_FREEZE)
			{
				if(data.plaFrozen.isOne(src[i]))
				{
  				(*handleFeature)(fset.get(MOVES_FROZEN_STR,pieceIndex),handleFeatureData);
  				(*handleFeature)(fset.get(MOVES_FROZEN_AT,Board::SYMLOC[pla][src[i]]),handleFeatureData);
				}
			}

			if(ENABLE_CAPTHREATENED)
			{
				if(data.plaCapThreatenedPlaTrap.isOne(src[i]))
				{
					bool isPlaTrap = true;
  				(*handleFeature)(fset.get(MOVES_CAPTHREATENED_STR,isPlaTrap,pieceIndex),handleFeatureData);
  				(*handleFeature)(fset.get(MOVES_CAPTHREATENED_AT,isPlaTrap,Board::SYMLOC[pla][src[i]]),handleFeatureData);
				}
				if(data.plaCapThreatenedOppTrap.isOne(src[i]))
				{
					bool isPlaTrap = false;
  				(*handleFeature)(fset.get(MOVES_CAPTHREATENED_STR,isPlaTrap,pieceIndex),handleFeatureData);
  				(*handleFeature)(fset.get(MOVES_CAPTHREATENED_AT,isPlaTrap,Board::SYMLOC[pla][src[i]]),handleFeatureData);
				}
				loc_t adjTrap = Board::ADJACENTTRAP[dest[i]];
				if(adjTrap != ERRORSQUARE)
				{
					int trapIndex = Board::TRAPINDEX[adjTrap];
					if(data.isTrapCapThreatened[trapIndex])
					{
						bool isPlaTrap = Board::ISPLATRAP[trapIndex][pla];
						int defCount = newTrapGuardCounts[pla][trapIndex];
	  				(*handleFeature)(fset.get(DEFENDS_CAPTHREATED_TRAP_STR,isPlaTrap,defCount,data.oppTrapState[trapIndex],pieceIndex),handleFeatureData);
	  				(*handleFeature)(fset.get(DEFENDS_CAPTHREATED_TRAP_AT,isPlaTrap,Board::SYMLOC[pla][dest[i]]),handleFeatureData);
					}
				}
			}

	    if(ENABLE_LIKELY_CAPTHREAT)
	    {
	    	loc_t adjTrap = Board::ADJACENTTRAP[dest[i]];
	    	if(adjTrap != ERRORSQUARE)
	    	{
	    		int trapIndex = Board::TRAPINDEX[adjTrap];
	    		if(newTrapGuardCounts[pla][trapIndex] == 1 && piece <= data.likelyDangerAdjTrapStr[dest[i]])
	  				(*handleFeature)(fset.get(LIKELY_CAPDANGER_ADJ,Board::ISPLATRAP[trapIndex][pla]),handleFeatureData);
	    	}
	    	loc_t adjTrap2 = Board::RAD2TRAP[dest[i]];
	    	if(adjTrap2 != ERRORSQUARE)
	    	{
	    		int trapIndex2 = Board::TRAPINDEX[adjTrap2];
	    		if(newTrapGuardCounts[pla][trapIndex2] == 0 && piece <= data.likelyDangerAdj2TrapStr[dest[i]])
	  				(*handleFeature)(fset.get(LIKELY_CAPDANGER_ADJ2,Board::ISPLATRAP[trapIndex2][pla]),handleFeatureData);
	    	}

	    	if(piece >= data.likelyThreatStr[dest[i]])
  				(*handleFeature)(fset.get(LIKELY_CAPTHREAT_STATIC),handleFeatureData);
	    	else if(piece >= data.likelyLooseThreatStr[dest[i]])
  				(*handleFeature)(fset.get(LIKELY_CAPTHREAT_LOOSE),handleFeatureData);
	    }

	    /*
	    if(ENABLE_RABGDIST && piece == RAB)
	    {
	    	(*handleFeature)(fset.get(RAB_GOAL_DIST_SRC,data.rabbitGoalDist[src[i]]),handleFeatureData);
	    	(*handleFeature)(fset.get(RAB_GOAL_DIST_DEST,data.rabbitGoalDist[dest[i]]),handleFeatureData);
	    }
	    */
    }

    else if(owner == opp && dest[i] != ERRORSQUARE)
    {
	    if(ENABLE_LIKELY_CAPTHREAT)
	    {
	    	if(data.likelyThreatPushPullLocs.isOne(dest[i]))
  				(*handleFeature)(fset.get(LIKELY_CAPTHREAT_PUSHPULL),handleFeatureData);
	    	else if(data.likelyThreatPushPullLocsLoose.isOne(dest[i]))
  				(*handleFeature)(fset.get(LIKELY_CAPTHREAT_PUSHPULL_LOOSE),handleFeatureData);
	    }
    }
  }

  if(ENABLE_LIKELY_CAPTHREAT)
  {
  	for(int trapIndex = 0; trapIndex < 4; trapIndex++)
  	{
    	if(data.trapNeedsMoreDefs[trapIndex] && newTrapGuardCounts[pla][trapIndex] > b.trapGuardCounts[pla][trapIndex])
				(*handleFeature)(fset.get(LIKELY_CAPTRAPDEF,Board::ISPLATRAP[trapIndex][pla]),handleFeatureData);
    	else if(!data.trapNeedsMoreDefs[trapIndex] && newTrapGuardCounts[pla][trapIndex] < b.trapGuardCounts[pla][trapIndex] &&
							newTrapGuardCounts[pla][trapIndex] < data.minDefendersToBeLikelySafe[trapIndex])
    	{
    		loc_t kt = Board::TRAPLOCS[trapIndex];
    		bool foundHanging = false;

    		if(newTrapGuardCounts[pla][trapIndex] == 1)
    		{
    			for(int dir = 0; dir < 4; dir++)
					{
						loc_t loc = kt + Board::ADJOFFSETS[dir];
						if(b.owners[loc] == pla && !moved.isOne(loc) && b.pieces[loc] <= data.likelyDangerAdjTrapStr[loc])
						{foundHanging = true; break;}
					}
    		}
    		else if(newTrapGuardCounts[pla][trapIndex] == 0)
    		{
    			for(int i = 5; i<13; i++)
    			{
    				loc_t loc = Board::SPIRAL[kt][i];
						if(b.owners[loc] == pla && !moved.isOne(loc) && b.pieces[loc] <= data.likelyDangerAdj2TrapStr[loc])
						{foundHanging = true; break;}
    			}
    		}

    		if(foundHanging)
  				(*handleFeature)(fset.get(LIKELY_CAPHANG,Board::ISPLATRAP[trapIndex][pla]),handleFeatureData);
    	}
  	}
  }

  if(ENABLE_DEPENDENCY && data.useDependency)
	{
		int numIndep = 0;
		int numTotal = 0;
		Bitmap dependDest;
		Bitmap dependSrc;
		for(int i = 0; i<4; i++)
		{
			step_t s = Board::getStep(move,i);
			if(s == PASSSTEP || s == ERRORSTEP)
				break;

			numTotal++;
			loc_t src = Board::K0INDEX[s];
			loc_t dest = Board::K1INDEX[s];
			if(!dependSrc.isOne(src) && !dependDest.isOne(dest))
				numIndep++;

			if(Board::ADJACENTTRAP[src] != ERRORSQUARE)
			{
				loc_t trapLoc = Board::ADJACENTTRAP[src];
				dependSrc |= Board::RADIUS[1][trapLoc];
				dependDest |= Board::RADIUS[1][trapLoc];
			}
			dependSrc |= Board::RADIUS[1][dest];
			dependDest |= Board::RADIUS[1][dest];

		}
		(*handleFeature)(fset.get(NUM_INDEP_STEPS,numTotal,numIndep),handleFeatureData);
	}

}



BOOST_STATIC_ASSERT(sizeof(MoveFeatureLitePosData) <= sizeof(FeaturePosData));
MoveFeatureLitePosData& MoveFeatureLitePosData::convert(FeaturePosData& data)
{
	return *((MoveFeatureLitePosData*)(&data));
}

const MoveFeatureLitePosData& MoveFeatureLitePosData::convert(const FeaturePosData& data)
{
	return *((const MoveFeatureLitePosData*)(&data));
}

static void getPosDataHelper(const Board& constB, const BoardHistory& hist, pla_t pla, FeaturePosData& dataBuf, bool srcDestOnly, bool fullMove);

void MoveFeatureLite::getPosData(const Board& constB, const BoardHistory& hist, pla_t pla, FeaturePosData& dataBuf)
{
	getPosDataHelper(constB,hist,pla,dataBuf,false,false);
}

void MoveFeatureLite::getPosDataSrcDestOnly(const Board& constB, const BoardHistory& hist, pla_t pla, FeaturePosData& dataBuf)
{
	getPosDataHelper(constB,hist,pla,dataBuf,true,false);
}

void MoveFeatureLite::getPosDataFullMove(const Board& constB, const BoardHistory& hist, pla_t pla, FeaturePosData& dataBuf)
{
	getPosDataHelper(constB,hist,pla,dataBuf,false,true);
}


static void getPosDataHelper(const Board& constB, const BoardHistory& hist, pla_t pla, FeaturePosData& dataBuf, bool srcDestOnly, bool fullMove)
{
	MoveFeatureLitePosData& data = MoveFeatureLitePosData::convert(dataBuf);
	Board b = constB;

	if(fullMove)
		data.useDependency = true;
	else
		data.useDependency = false;

  b.initializeStronger(data.pStronger);

  for(piece_t piece = RAB; piece <= ELE; piece++)
	{
  	data.pieceIndex[GOLD][piece] = ArimaaFeature::getPieceIndexApprox(GOLD,piece,data.pStronger);
  	data.pieceIndex[SILV][piece] = ArimaaFeature::getPieceIndexApprox(SILV,piece,data.pStronger);
  }

  if(srcDestOnly)
  {
  	data.srcDestOnly = true;
  	return;
  }
  else
  	data.srcDestOnly = false;

  pla_t opp = OPP(pla);
  for(int i = 0; i<64; i++)
  {
  	data.oppMaxStrAdj[i] = EMP;
  	data.oppMinStrAdj[i] = ELE+1;
  }

  for(int i = 0; i<64; i++)
	{
  	if(b.owners[i] != opp)
  		continue;

  	int str = b.pieces[i];
  	if(CS1(i) && data.oppMaxStrAdj[i-8] < str) {data.oppMaxStrAdj[i-8] = str;}
  	if(CW1(i) && data.oppMaxStrAdj[i-1] < str) {data.oppMaxStrAdj[i-1] = str;}
  	if(CE1(i) && data.oppMaxStrAdj[i+1] < str) {data.oppMaxStrAdj[i+1] = str;}
  	if(CN1(i) && data.oppMaxStrAdj[i+8] < str) {data.oppMaxStrAdj[i+8] = str;}

  	if(CS1(i) && data.oppMinStrAdj[i-8] > str) {data.oppMinStrAdj[i-8] = str;}
  	if(CW1(i) && data.oppMinStrAdj[i-1] > str) {data.oppMinStrAdj[i-1] = str;}
  	if(CE1(i) && data.oppMinStrAdj[i+1] > str) {data.oppMinStrAdj[i+1] = str;}
  	if(CN1(i) && data.oppMinStrAdj[i+8] > str) {data.oppMinStrAdj[i+8] = str;}
	}

  data.plaFrozen = b.frozenMap & b.pieceMaps[pla][0];

  data.plaCapThreatenedPlaTrap = Bitmap();
  data.plaCapThreatenedOppTrap = Bitmap();
  for(int i = 0; i<4; i++)
  {
  	data.isTrapCapThreatened[i] = false;

    int capDist;
    Bitmap map;
    loc_t kt = Board::TRAPLOCS[i];
    if(BoardTrees::canCaps(b,opp,4,kt,map,capDist))
    {
    	data.isTrapCapThreatened[i] = true;
    	if(Board::ISPLATRAP[i][pla])
				data.plaCapThreatenedPlaTrap |= map;
    	else
				data.plaCapThreatenedOppTrap |= map;
    }

    data.oppTrapState[i] = ArimaaFeature::getTrapState(b,opp,kt);
  }

	Bitmap pStrongerMaps[2][NUMTYPES];
  b.initializeStrongerMaps(pStrongerMaps);

	for(int i = 0; i<64; i++)
	{
		data.likelyDangerAdjTrapStr[i] = EMP;
		data.likelyDangerAdj2TrapStr[i] = EMP;
	}

	for(int trapIndex = 0; trapIndex < 4; trapIndex++)
	{
		loc_t kt = Board::TRAPLOCS[trapIndex];
		bool haveDangerPieceRad1 = false;
		for(int dir = 0; dir < 4; dir++)
		{
			loc_t loc = kt + Board::ADJOFFSETS[dir];
			data.likelyDangerAdjTrapStr[loc] = EMP;

			//If we are the only defender, then we are likely threatened with capture in the adjacent trap if
			//there is a stronger piece within radius 3 of us unfrozen, or a stronger piece within radius 2 of us at all
			//So find the strongest piece in such a set
			Bitmap relevantSet = Board::RADIUS[2][loc] | (Board::RADIUS[3][loc] & ~b.frozenMap);
			for(piece_t piece = ELE; piece >= CAT; piece--)
			{
				if((pStrongerMaps[pla][piece-1] & relevantSet).hasBits())
				{
					data.likelyDangerAdjTrapStr[loc] = piece-1;

					//If we already have a piece here, then this means that we are in trouble if the number of defenders
					//goes to 1
					if(b.owners[loc] == pla && b.pieces[loc] <= piece-1)
						haveDangerPieceRad1 = true;

					break;
				}
			}
		}

		//Radius 2 squares around trap
		bool haveDangerPieceRad2 = false;
		for(int i = 5; i<13; i++)
		{
			loc_t loc = Board::SPIRAL[kt][i];
			//If there are no trap defenders, then we are likely threatened with capture only if there is an adjacent
			//unfrozen piece
			Bitmap relevantSet = Board::RADIUS[1][loc] & ~b.frozenMap;
			for(piece_t piece = ELE; piece >= CAT; piece--)
			{
				if((pStrongerMaps[pla][piece-1] & relevantSet).hasBits())
				{
					data.likelyDangerAdj2TrapStr[loc] = piece-1;

					//If we already have a piece here, then this means that we are in trouble if the number of defenders
					//goes to zero
					if(b.owners[loc] == pla && b.pieces[loc] <= piece-1)
						haveDangerPieceRad2 = true;

					break;
				}
			}
		}

		if(haveDangerPieceRad1)
			data.minDefendersToBeLikelySafe[trapIndex] = 2;
		else if(haveDangerPieceRad2)
			data.minDefendersToBeLikelySafe[trapIndex] = 1;
		else
			data.minDefendersToBeLikelySafe[trapIndex] = 0;
	}

	for(int i = 0; i<4; i++)
		data.trapNeedsMoreDefs[i] = data.isTrapCapThreatened[i];

	for(int i = 0; i<64; i++)
	{
		data.likelyThreatStr[i] = ELE+1;
		data.likelyLooseThreatStr[i] = ELE+1;
	}

	data.likelyThreatPushPullLocs = Bitmap();
	data.likelyThreatPushPullLocsLoose = Bitmap();

	for(int trapIndex = 0; trapIndex<4; trapIndex++)
	{
		loc_t kt = Board::TRAPLOCS[trapIndex];

		//Relevant pieces to threaten are those within radius 2
		if(b.trapGuardCounts[opp][trapIndex] == 0)
		{
			//While we're at it, if we pushpull an opp adjacent to an undefended trap, it's a likely threat
			data.likelyThreatPushPullLocs |= Board::RADIUS[1][kt];

			//And it's a loose threat if we do it at a farther radius
			data.likelyThreatPushPullLocsLoose |= Board::RADIUS[2][kt];

			Bitmap oppMap = b.pieceMaps[opp][0] & Board::RADIUS[2][kt];
			while(oppMap.hasBits())
			{
				loc_t oloc = oppMap.nextBit();
				piece_t neededStr = b.pieces[oloc] + 1;
				for(int dir = 0; dir<4; dir++)
				{
					if(!Board::ADJOKAY[dir][oloc])
						continue;
					loc_t adj = oloc + Board::ADJOFFSETS[dir];
					if(neededStr < data.likelyThreatStr[adj])
						data.likelyThreatStr[adj] = neededStr;
				}
			}
		}

		//Relevant pieces to threaten are the single defenders
		else if(b.trapGuardCounts[opp][trapIndex] == 1)
		{
			loc_t oloc = b.findGuard(opp,kt);
			piece_t neededStr = b.pieces[oloc] + 1;
			//Let's say that we need to be within radius 1 to really be threatening
			//Even though radius 3 would sometimes suffice
			for(int dir = 0; dir<4; dir++)
			{
				loc_t adj = oloc + Board::ADJOFFSETS[dir];
				if(neededStr < data.likelyThreatStr[adj])
					data.likelyThreatStr[adj] = neededStr;
			}

			//But radius 2 would threaten loosely
			for(int i = 5; i<13; i++)
			{
				loc_t adj = Board::SPIRAL[oloc][i];
				if(neededStr < data.likelyLooseThreatStr[adj])
					data.likelyLooseThreatStr[adj] = neededStr;
			}
		}

		//Relevant pieces to threaten are defenders
		//To do so, we need to actually put a piece greater than both strengths on one of the
		//defenders current squares (indicating that we pushed one of them).
		else if(b.trapGuardCounts[opp][trapIndex] == 2)
		{
			int numDefenderLocs = 0;
			loc_t defenderLocs[2];
			for(int dir = 0; dir<4; dir++)
			{
				loc_t adj = kt + Board::ADJOFFSETS[dir];
				if(b.owners[adj] == opp)
					defenderLocs[numDefenderLocs++] = adj;
			}

			piece_t maxNeededStr;
			if(b.pieces[defenderLocs[0]] < b.pieces[defenderLocs[1]])
				maxNeededStr = b.pieces[defenderLocs[1]];
			else
				maxNeededStr = b.pieces[defenderLocs[0]];

			//If the other piece is dominated, then putting a piece here at all is a strong threat
			if((Board::RADIUS[1][defenderLocs[1]] & pStrongerMaps[opp][b.pieces[defenderLocs[1]]]).hasBits())
				data.likelyThreatStr[defenderLocs[0]] = RAB;
			//Else if it has a radius 2 dominator, then it's a weak threat
			else if((Board::RADIUS[2][defenderLocs[1]] & pStrongerMaps[opp][b.pieces[defenderLocs[1]]]).hasBits())
				data.likelyLooseThreatStr[defenderLocs[0]] = RAB;
			//Else we can make a loose threat simply by putting a new piece here >= both defenders
			else if(maxNeededStr < data.likelyLooseThreatStr[defenderLocs[0]])
				data.likelyLooseThreatStr[defenderLocs[0]] = maxNeededStr;

			//Repeat for the other piece
			if((Board::RADIUS[1][defenderLocs[0]] & pStrongerMaps[opp][b.pieces[defenderLocs[0]]]).hasBits())
				data.likelyThreatStr[defenderLocs[1]] = RAB;
			else if((Board::RADIUS[2][defenderLocs[0]] & pStrongerMaps[opp][b.pieces[defenderLocs[0]]]).hasBits())
				data.likelyLooseThreatStr[defenderLocs[1]] = RAB;
			else if(maxNeededStr < data.likelyLooseThreatStr[defenderLocs[1]])
				data.likelyLooseThreatStr[defenderLocs[1]] = maxNeededStr;
		}
	}

	//Threats::getGoalDistEst(b,pla,data.rabbitGoalDist,9);

}


