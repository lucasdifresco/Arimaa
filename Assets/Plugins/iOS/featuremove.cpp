
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
#include "ufdist.h"
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

//ACTUAL STEPS---------------------------------------------------------------------------------------
static const bool ENABLE_STEPS = false;

//Actual step made (pieceindex 0-7)(src * 4 + symdir 0-127)
static fgrpindex_t ACTUAL_STEP;
static fgrpindex_t ACTUAL_STEP_O;


static const bool ENABLE_CAPTURED = false;

//Piece captured (pieceindex 0-7)
static fgrpindex_t CAPTURED;
static fgrpindex_t CAPTURED_O;

//SPECIAL CASE FRAME SAC-------------------------------------------------------------------------------
static const bool ENABLE_STRATS = true;

//Sacrifices a framed pla piece. (pieceindex 0-7)(iseleele 0-1)
static fgrpindex_t FRAMESAC;
//Sacrifices a hostaged pla piece. (pieceindex 0-7)(iseleele 0-1)  TODO NOT IMPLEMENTED
static fgrpindex_t HOSTAGESAC;
//After move, there is a pla framing a piece (pieceindex 0-7)(iseleele 0-1)
static fgrpindex_t FRAME_P;
//After move, there is an opp framing a piece (pieceindex 0-7)(iseleele 0-1)
static fgrpindex_t FRAME_O;
//After move, there is a hostage held by pla (pieceindex 0-7)(holderindex 0-7)  TODO NOT IMPLEMENTED
static fgrpindex_t HOSTAGE_P;
//After move, there is a hostage held by opp (pieceindex 0-7)(holderindex 0-7)  TODO NOT IMPLEMENTED
static fgrpindex_t HOSTAGE_O;
//Elephant blockade (centrality 0-6)(usesele 0-1)
static fgrpindex_t EBLOCKADE_CENT_P;
//Elephant blockade (centrality 0-6)(usesele 0-1)
static fgrpindex_t EBLOCKADE_CENT_O;
//Elephant blockade (gydist 0-7)(usesele 0-1)
static fgrpindex_t EBLOCKADE_GYDIST_P;
//Elephant blockade (gydist 0-7)(usesele 0-1)
static fgrpindex_t EBLOCKADE_GYDIST_O;

//TRAP STATE-----------------------------------------------------------------------------------------
static const bool ENABLE_TRAPSTATE = true;

//Changes the pla/opp trapstate at a trap. (isopptrap 0-1)(oldstate 0-7)(newstate 0-7)
static fgrpindex_t TRAP_STATE_P;
static fgrpindex_t TRAP_STATE_O;

//TRAP CONTROL---------------------------------------------------------------------------------------
static const bool ENABLE_TRAPCONTROL = true;

//Move results in the specified trap control for a trap (isopptrap 0-1)(tc 0-16 (/10 +8))
static fgrpindex_t TRAP_CONTROL;

//CAPTURE DEFENSE---------------------------------------------------------------------------------------
static const bool ENABLE_CAP_DEF = true;

//Adds an elephant defender to a trap that the opponent could capture at. (isopptrap 0-1)(threat 0-4)
static fgrpindex_t CAP_DEF_ELEDEF;
//Adds a non-elephant defender to a trap that the opponent could capture at. (isopptrap 0-1)(threat 0-4)
static fgrpindex_t CAP_DEF_TRAPDEF;
//Moves a pla piece that was threatened with capture. (isopptrap 0-1)(threat 0-4)
static fgrpindex_t CAP_DEF_RUNAWAY;
//Moves or freezes an opp piece that was threatening to capture. (isopptrap 0-1)(threat 0-4)
static fgrpindex_t CAP_DEF_INTERFERE;

//GOAL THREATENING--------------------------------------------------------------------------------------
static const bool ENABLE_GOAL_THREAT = true;
//Wins the game
static fgrpindex_t WINS_GAME;
//Threatens goal when we couldn't goal previously. (threat 1-4)
static fgrpindex_t THREATENS_GOAL;
//Allows the opponent to goal
static fgrpindex_t ALLOWS_GOAL;

//CAPTURE THREATENING-----------------------------------------------------------------------------------
static const bool ENABLE_CAP_THREAT = true;

//Threatens to capture a formerly safe piece of the opponent. (pieceindex 0-7)(dist 0-4)(isopptrap 0-1)
static fgrpindex_t THREATENS_CAP;
//Allows an opponent to capture a formerly safe piece that we just moved. (pieceindex 0-7)(dist 0-4)(isopptrap 0-1)
static fgrpindex_t INVITES_CAP_MOVED;
//Allows an opponent to capture a formerly safe piece that we didn't just move. (pieceindex 0-7)(dist 0-4)(isopptrap 0-1)
static fgrpindex_t INVITES_CAP_UNMOVED;
//Prevents the opponent from capturing a piece (without sacrificing it) (pieceindex 0-7)(piecesymloc 0-31)
static fgrpindex_t PREVENTS_CAP;

//ADVANCEMENT AND INFLUENCE-----------------------------------------------------------------------------
static const bool ENABLE_INFLUENCE = true;

//Moves rabbit. (finaldistance 0-7) (influence 0-8 (+4))
static fgrpindex_t RABBIT_ADVANCE;
//Advances piece (pieceindex 0-7) (influencetrap 0-8 (+4))
static fgrpindex_t PIECE_ADVANCE;
//Retreats piece (pieceindex 0-7) (influencetrap 0-8 (+4))
static fgrpindex_t PIECE_RETREAT;

//Increases distance from nearest dominating piece (pieceindex 0-7) (influencesrc 0-8 (+4)) (rad 0-4)
static fgrpindex_t ESCAPE_DOMINATOR;
//Decreases distance from nearest dominating piece (pieceindex 0-7) (influencesrc 0-8 (+4)) (rad 0-4)
static fgrpindex_t APPROACH_DOMINATOR;
//Dominates a piece by stepping adjacent (pieceindex 0-7) (influencedom 0-8 (+4))
static fgrpindex_t DOMINATES_ADJ;

//Steps on to a trap with 2 or more defenders or the ele or not (isopptrap 0-1)(pieceindex 0-7)
static fgrpindex_t SAFE_STEP_ON_TRAP;
static fgrpindex_t UNSAFE_STEP_ON_TRAP;


//FREEZING-----------------------------------------------------------------------------------------------
static const bool ENABLE_FREEZE = true;

//Freezes opponent piece of the given type (pieceindex 0-7)
static fgrpindex_t FREEZES_OPP_STR;
//Freezes pla piece of the given type (pieceindex 0-7)
static fgrpindex_t FREEZES_PLA_STR;
//Thaws opponent piece of the given type (pieceindex 0-7)
static fgrpindex_t THAWS_OPP_STR;
//Thaws pla piece of the given type (pieceindex 0-7)
static fgrpindex_t THAWS_PLA_STR;

//Freezes opponent piece at (symloc 0-31)
static fgrpindex_t FREEZES_OPP_AT;
//Freezes pla piece of the given type (symloc 0-31)
static fgrpindex_t FREEZES_PLA_AT;
//Thaws opponent piece of the given type (symloc 0-31)
static fgrpindex_t THAWS_OPP_AT;
//Thaws pla piece of the given type (symloc 0-31)
static fgrpindex_t THAWS_PLA_AT;

//PHALANXES---------------------------------------------------------------------------------------------
static const bool ENABLE_PHALANX = true;

//This move creates a phalanx against a piece (pieceindex 0-7) (issingle 0-1)
static fgrpindex_t CREATES_PHALANX_VS;
//This move creates a phalanx against a piece at (symloc 0-31) (dir 0-2) (issingle 0-1)
static fgrpindex_t CREATES_PHALANX_AT;

//This move releases a phalanx against a piece (pieceindex 0-7) (issingle 0-1)
static fgrpindex_t RELEASES_PHALANX_VS;
//This move releases a phalanx against a piece at (symloc 0-31) (dir 0-2) (issingle 0-1)
static fgrpindex_t RELEASES_PHALANX_AT;

//BLOCKING UFS/ESCAPES----------------------------------------------------------------------------------
static const bool ENABLE_BLOCKING = false;

//Steps a piece adjacent to one frozen (pieceindex 0-7)(blockerweaker 0-1)(sidefrontback 0-3)
static fgrpindex_t BLOCKS_FROZEN;
//Steps a piece adjacent to one just pushed/pulled (pieceindex 0-7)(blockerweaker 0-1)(sidefrontback 0-3)
static fgrpindex_t BLOCKS_PUSHPULLED;
//Steps a piece adjacent to one under capthreat (pieceindex 0-7)(blockerweaker 0-1)(sidefrontback 0-3)
static fgrpindex_t BLOCKS_CAPTHREATED;

//REVERSIBLES and FREECAPS------------------------------------------------------------------------------
static const bool ENABLE_REVFREECAPS = true;

//4-2 reversible, 3-1 reversible, and pulls/pushes a piece that can immediately step back.
static fgrpindex_t IS_FULL_REVERSIBLE;
static fgrpindex_t IS_MOSTLY_REVERSIBLE;
static fgrpindex_t PUSHPULL_OPP_REVERSIBLE;

//Only-moved piece can immediately be captured for free, or only 1 step for other pieces
static fgrpindex_t IS_FREE_CAPTURABLE;
static fgrpindex_t IS_MOSTLY_FREE_CAPTURABLE;


//LAST MOVE-------------------------------------------------------------------------------------------
static const bool ENABLE_LAST = true;

//Moves a piece that the opponent just pushpulled
static fgrpindex_t MOVES_LAST_PUSHED;
//Moves near the opponent's last move (nearness 0-63)
static fgrpindex_t MOVES_NEAR_LAST;
//Moves near the pla's move his last turn (nearness 0-63)
static fgrpindex_t MOVES_NEAR_LAST_LAST;

//MOVE DEPENDENCY--------------------------------------------------------------------------------
static const bool ENABLE_DEPENDENCY = true;

//How many of the steps of this move were independent? (numtotal 0-4)(numindep 0-4)
static fgrpindex_t NUM_INDEP_STEPS;

//How contiguous is this move? (contiguousity 0-24)
//static fgrpindex_t CONTIGUOUSITY;

//IDEAS------------------------------------------------------------------------------------------
//This move places a piece next to a piece that could be captured (blocking enemy runaway defense)
//Changes distance of camel from elephant
//Advances pieces on a side of the board where one has control around the opponent's trap.
//Moves pieces in the vicinity of a control swap (opp controls pla trap or vice versa)
//In the vicinity of an advanced rabbit - towards it, or blocking it
//Creates an E-M hostage
//Creates an M-H hostage

//Helpers---------------------------------------------------------------------------------------
static void initPriors();
static int getSymDir(pla_t pla, loc_t loc, loc_t adj);

const FeatureSet& MoveFeature::getFeatureSet()
{
	return fset;
}

ArimaaFeatureSet MoveFeature::getArimaaFeatureSet()
{
	return ArimaaFeatureSet(&fset,MoveFeature::getFeatures,MoveFeature::getPosData);
}

bool MoveFeature::IS_INITIALIZED = false;
void MoveFeature::initFeatureSet()
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

  if(ENABLE_STEPS)
  {
  	ACTUAL_STEP = fset.add("Step",NUMPIECEINDICES,128);
  	ACTUAL_STEP_O = fset.add("StepO",NUMPIECEINDICES,128);
  }

  if(ENABLE_CAPTURED)
  {
  	CAPTURED = fset.add("Captured",NUMPIECEINDICES);
  	CAPTURED_O = fset.add("CapturedO",NUMPIECEINDICES);
  }

  if(ENABLE_STRATS)
  {
    FRAMESAC = fset.add("FrameSac",NUMPIECEINDICES, 2);
    HOSTAGESAC = fset.add("HostageSac",NUMPIECEINDICES, 2);

    FRAME_P = fset.add("Frame_P",NUMPIECEINDICES, 2);
    FRAME_O = fset.add("Frame_O",NUMPIECEINDICES, 2);

    HOSTAGE_P = fset.add("Hostage_P",NUMPIECEINDICES,NUMPIECEINDICES);
    HOSTAGE_O = fset.add("Hostage_O",NUMPIECEINDICES,NUMPIECEINDICES);

    EBLOCKADE_CENT_P = fset.add("EBlockCent_P",7, 2);
    EBLOCKADE_CENT_O = fset.add("EBlockCent_O",7, 2);
    EBLOCKADE_GYDIST_P = fset.add("EBlockGYDist_P",8, 2);
    EBLOCKADE_GYDIST_O = fset.add("EBlockGYDist_O",8, 2);
  }

  if(ENABLE_TRAPSTATE)
  {
  	TRAP_STATE_P = fset.add("TrapStateP",2,8,8);
    TRAP_STATE_O = fset.add("TrapStateO",2,8,8);
  }

  if(ENABLE_TRAPCONTROL)
  {
  	TRAP_CONTROL = fset.add("TrapControl",2,17);
  }

  if(ENABLE_CAP_DEF)
  {
    CAP_DEF_ELEDEF = fset.add("CapDefEleDef",2,5);
    CAP_DEF_TRAPDEF = fset.add("CapDefTrapDef",2,5);
    CAP_DEF_RUNAWAY = fset.add("CapDefRunaway",2,5);
    CAP_DEF_INTERFERE = fset.add("CapDefInterfere",2,5);
  }

  if(ENABLE_GOAL_THREAT)
  {
    WINS_GAME = fset.add("WinsGame");
    THREATENS_GOAL = fset.add("ThreatensGoal",5);
    ALLOWS_GOAL = fset.add("InvitesGoal");
  }

  if(ENABLE_CAP_THREAT)
  {
    THREATENS_CAP = fset.add("ThreatensCap",NUMPIECEINDICES,5,2);
    INVITES_CAP_MOVED = fset.add("InvitesCapMoved",NUMPIECEINDICES,5,2);
    INVITES_CAP_UNMOVED = fset.add("InvitesCapUnmoved",NUMPIECEINDICES,5,2);
    PREVENTS_CAP = fset.add("PreventsCap",NUMPIECEINDICES,Board::NUMSYMLOCS);
  }

  if(ENABLE_FREEZE)
  {
  	FREEZES_OPP_STR = fset.add("FreezesOppStr", NUMPIECEINDICES);
  	FREEZES_PLA_STR = fset.add("FreezesPlaStr", NUMPIECEINDICES);
  	THAWS_OPP_STR = fset.add("ThawsOppStr", NUMPIECEINDICES);
  	THAWS_PLA_STR = fset.add("ThawsPlaStr", NUMPIECEINDICES);

  	FREEZES_OPP_AT = fset.add("FreezesOppAt", 32);
  	FREEZES_PLA_AT = fset.add("FreezesPlaAt", 32);
  	THAWS_OPP_AT = fset.add("ThawsOppAt", 32);
  	THAWS_PLA_AT = fset.add("ThawsPlaAt", 32);
  }

  if(ENABLE_PHALANX)
  {
  	CREATES_PHALANX_VS = fset.add("CreatesPhalanxVs",NUMPIECEINDICES,2);
  	CREATES_PHALANX_AT = fset.add("CreatesPhalanxAtLoc",32,Board::NUMSYMDIR,2);
  	RELEASES_PHALANX_VS = fset.add("ReleasesPhalanxVs",NUMPIECEINDICES,2);
  	RELEASES_PHALANX_AT = fset.add("ReleasesPhalanxAtLoc",32,Board::NUMSYMDIR,2);
  }

  if(ENABLE_INFLUENCE)
  {
    RABBIT_ADVANCE = fset.add("RabbitAdvance",8,9);
    PIECE_ADVANCE = fset.add("PieceAdvance",NUMPIECEINDICES,9);
    PIECE_RETREAT = fset.add("PieceRetreat",NUMPIECEINDICES,9);
    ESCAPE_DOMINATOR = fset.add("EscapeDominator",NUMPIECEINDICES,9,5);
    APPROACH_DOMINATOR = fset.add("ApproachDominator",NUMPIECEINDICES,9,5);
    DOMINATES_ADJ = fset.add("DominatesAdj",NUMPIECEINDICES,9);
    SAFE_STEP_ON_TRAP = fset.add("SafeStepOnTrap",2,NUMPIECEINDICES);
    UNSAFE_STEP_ON_TRAP = fset.add("UnafeStepOnTrap",2,NUMPIECEINDICES);
  }

  if(ENABLE_BLOCKING)
  {
  	BLOCKS_FROZEN = fset.add("BlocksFrozen",NUMPIECEINDICES,2,4);
  	BLOCKS_PUSHPULLED = fset.add("BlocksPushpulled",NUMPIECEINDICES,2,4);
  	BLOCKS_CAPTHREATED = fset.add("BlocksCapthreated",NUMPIECEINDICES,2,4);
  }

  if(ENABLE_REVFREECAPS)
  {
  	IS_FULL_REVERSIBLE = fset.add("IsFullReversible");
  	IS_MOSTLY_REVERSIBLE = fset.add("IsMostlyReversible");
  	PUSHPULL_OPP_REVERSIBLE = fset.add("PushpullOppReversible");
  	IS_FREE_CAPTURABLE = fset.add("IsFreeCapturable");
  	IS_MOSTLY_FREE_CAPTURABLE = fset.add("IsMostlyFreeCapturable");
  }

  if(ENABLE_LAST)
  {
  	MOVES_LAST_PUSHED = fset.add("MovesLastPushed");
  	MOVES_NEAR_LAST = fset.add("MovesNearLast",64);
  	MOVES_NEAR_LAST_LAST = fset.add("MovesNearLastLast",64);
  }

  if(ENABLE_DEPENDENCY)
  {
  	NUM_INDEP_STEPS = fset.add("NumIndepSteps",5,5);
  	//CONTIGUOUSITY = fset.add("Contiguousity",25);
  }

  initPriors();

  IS_INITIALIZED = true;
}

static void addWLPriorFor(fgrpindex_t group, double weightWin, double weightLoss)
{
	findex_t priorIndex = fset.get(PRIOR);
	for(int i = 0; i<fset.groups[group].size; i++)
	{
		findex_t feature = i + fset.groups[group].baseIdx;
		fset.addPriorMatch(FeatureSet::PriorMatch(priorIndex,feature, weightWin));
		fset.addPriorMatch(FeatureSet::PriorMatch(feature,priorIndex, weightLoss));
	}
}

static void initPriors()
{
	//Add a weak 1W1L prior over everything
	fset.setPriorIndex(fset.get(PRIOR));
	fset.addUniformPrior(1.0,1.0);

	//Add additional prior strength for most things, and special priors for others
	double epr = 1.5;

	addWLPriorFor(PASS,epr,epr);

  if(ENABLE_SRCDEST)
  {
		addWLPriorFor(SRC,epr,epr);
		addWLPriorFor(SRC_O,epr,epr);
		addWLPriorFor(DEST,epr,epr);
		addWLPriorFor(DEST_O,epr,epr);
  }

  if(ENABLE_STEPS)
  {
		addWLPriorFor(ACTUAL_STEP,epr,epr);
		addWLPriorFor(ACTUAL_STEP_O,epr,epr);
  }

  if(ENABLE_CAPTURED)
  {
		addWLPriorFor(CAPTURED,epr,epr);
		addWLPriorFor(CAPTURED_O,epr,epr);
  }

  if(ENABLE_STRATS)
  {
		addWLPriorFor(FRAMESAC,epr,epr);
		addWLPriorFor(HOSTAGESAC,epr,epr);
		addWLPriorFor(FRAME_P,epr,epr);
		addWLPriorFor(FRAME_O,epr,epr);
		addWLPriorFor(HOSTAGE_P,epr,epr);
		addWLPriorFor(HOSTAGE_O,epr,epr);
		addWLPriorFor(EBLOCKADE_CENT_P,epr,epr);
		addWLPriorFor(EBLOCKADE_CENT_O,epr,epr);
		addWLPriorFor(EBLOCKADE_GYDIST_P,epr,epr);
		addWLPriorFor(EBLOCKADE_GYDIST_O,epr,epr);
  }

  if(ENABLE_TRAPSTATE)
  {
  	addWLPriorFor(TRAP_STATE_P,epr,epr);
  	addWLPriorFor(TRAP_STATE_O,epr,epr);
  }

  if(ENABLE_TRAPCONTROL)
  {
		//Higher trap controls are better - give each one a 5W4L against the previous
		for(int j = 0; j<16; j++)
		{
			fset.addPriorMatch(FeatureSet::PriorMatch(fset.get(TRAP_CONTROL,0,j),fset.get(TRAP_CONTROL,0,j+1), 4));
			fset.addPriorMatch(FeatureSet::PriorMatch(fset.get(TRAP_CONTROL,0,j+1),fset.get(TRAP_CONTROL,0,j), 5));
			fset.addPriorMatch(FeatureSet::PriorMatch(fset.get(TRAP_CONTROL,1,j),fset.get(TRAP_CONTROL,1,j+1), 4));
			fset.addPriorMatch(FeatureSet::PriorMatch(fset.get(TRAP_CONTROL,1,j+1),fset.get(TRAP_CONTROL,1,j), 5));
		}
  }

  if(ENABLE_CAP_DEF)
  {
		addWLPriorFor(CAP_DEF_ELEDEF,epr,epr);
		addWLPriorFor(CAP_DEF_TRAPDEF,epr,epr);
		addWLPriorFor(CAP_DEF_TRAPDEF,epr,epr);
		addWLPriorFor(CAP_DEF_INTERFERE,epr,epr);
  }

  if(ENABLE_GOAL_THREAT)
  {
		addWLPriorFor(WINS_GAME,epr,0);
		addWLPriorFor(THREATENS_GOAL,epr,0);
		addWLPriorFor(ALLOWS_GOAL,0,epr);
  }

  if(ENABLE_CAP_THREAT)
  {
		addWLPriorFor(THREATENS_CAP,epr,epr);
		addWLPriorFor(INVITES_CAP_MOVED,epr,epr);
		addWLPriorFor(INVITES_CAP_UNMOVED,epr,epr);
		addWLPriorFor(PREVENTS_CAP,epr,epr);
  }

  if(ENABLE_FREEZE)
  {
		addWLPriorFor(FREEZES_OPP_STR,epr,epr);
		addWLPriorFor(FREEZES_PLA_STR,epr,epr);
		addWLPriorFor(THAWS_OPP_STR,epr,epr);
		addWLPriorFor(THAWS_PLA_STR,epr,epr);
		addWLPriorFor(FREEZES_OPP_AT,epr,epr);
		addWLPriorFor(FREEZES_PLA_AT,epr,epr);
		addWLPriorFor(THAWS_OPP_AT,epr,epr);
		addWLPriorFor(THAWS_PLA_AT,epr,epr);
  }

  if(ENABLE_PHALANX)
  {
		addWLPriorFor(CREATES_PHALANX_VS,epr,epr);
		addWLPriorFor(CREATES_PHALANX_AT,epr,epr);
		addWLPriorFor(RELEASES_PHALANX_VS,epr,epr);
		addWLPriorFor(RELEASES_PHALANX_AT,epr,epr);
  }

  if(ENABLE_INFLUENCE)
  {
		addWLPriorFor(RABBIT_ADVANCE,epr,epr);
		addWLPriorFor(PIECE_ADVANCE,epr,epr);
		addWLPriorFor(PIECE_RETREAT,epr,epr);
		addWLPriorFor(ESCAPE_DOMINATOR,epr,epr);
		addWLPriorFor(APPROACH_DOMINATOR,epr,epr);
		addWLPriorFor(DOMINATES_ADJ,epr,epr);
		addWLPriorFor(SAFE_STEP_ON_TRAP,epr,epr);
		addWLPriorFor(UNSAFE_STEP_ON_TRAP,epr,epr);
  }

  if(ENABLE_BLOCKING)
  {
  	addWLPriorFor(BLOCKS_FROZEN,epr,epr);
  	addWLPriorFor(BLOCKS_PUSHPULLED,epr,epr);
  	addWLPriorFor(BLOCKS_CAPTHREATED,epr,epr);
  }

  if(ENABLE_REVFREECAPS)
  {
		addWLPriorFor(IS_FULL_REVERSIBLE,epr,epr);
		addWLPriorFor(IS_MOSTLY_REVERSIBLE,epr,epr);
		addWLPriorFor(PUSHPULL_OPP_REVERSIBLE,epr,epr);
		addWLPriorFor(IS_FREE_CAPTURABLE,epr,epr);
		addWLPriorFor(IS_MOSTLY_FREE_CAPTURABLE,epr,epr);
  }

  if(ENABLE_LAST)
  {
		addWLPriorFor(MOVES_LAST_PUSHED,epr,epr);

		//Each step is similar - give each one a 4.2W 4L against the previous
		for(int j = 0; j<63; j++)
		{
			fset.addPriorMatch(FeatureSet::PriorMatch(fset.get(MOVES_NEAR_LAST,j),fset.get(MOVES_NEAR_LAST,j+1), 4));
			fset.addPriorMatch(FeatureSet::PriorMatch(fset.get(MOVES_NEAR_LAST,j+1),fset.get(MOVES_NEAR_LAST,j), 4.2));
			fset.addPriorMatch(FeatureSet::PriorMatch(fset.get(MOVES_NEAR_LAST_LAST,j),fset.get(MOVES_NEAR_LAST_LAST,j+1), 4));
			fset.addPriorMatch(FeatureSet::PriorMatch(fset.get(MOVES_NEAR_LAST_LAST,j+1),fset.get(MOVES_NEAR_LAST_LAST,j), 4.2));
		}
  }

  if(ENABLE_DEPENDENCY)
  {
  	addWLPriorFor(NUM_INDEP_STEPS,epr,epr);
  	//addWLPriorFor(CONTIGUOUSITY,epr,epr);
  }
}


void MoveFeature::getFeatures(const Board& b, const FeaturePosData& dataBuf, pla_t pla, move_t move, const BoardHistory& hist,
		void (*handleFeature)(findex_t,void*), void* handleFeatureData)
{
  //Handle Pass Move
  if(move == PASSMOVE || move == QPASSMOVE)
  {
    (*handleFeature)(fset.get(PASS),handleFeatureData);
    return;
  }

	const MoveFeaturePosData& data = MoveFeaturePosData::convert(dataBuf);

  //Source and destination piece movement features
  pla_t opp = OPP(pla);
  loc_t src[8];
  loc_t dest[8];
  int numChanges = b.getChanges(move,src,dest);
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

    if(ENABLE_CAPTURED && (dest[i] == ERRORSQUARE))
    {
      (*handleFeature)(fset.get(owner == pla ? CAPTURED : CAPTURED_O,pieceIndex),handleFeatureData);
    }

    //FRAMESAC features
    if(ENABLE_STRATS)
    {
      if(dest[i] == ERRORSQUARE && owner == pla && Board::ISTRAP[src[i]] && data.oppHoldsFrameAtTrap[Board::TRAPINDEX[src[i]]])
      {
      	bool isEleEle = data.oppFrameIsEleEle[Board::TRAPINDEX[src[i]]];
      	(*handleFeature)(fset.get(FRAMESAC,pieceIndex,isEleEle),handleFeatureData);
      }
    }
  }

  if(ENABLE_STEPS)
	{
  	Board bb = b;
		for(int i = 0; i<4; i++)
		{
			step_t s = Board::getStep(move,i);
			if(s == PASSSTEP || s == ERRORSTEP)
				break;

			loc_t k0 = Board::K0INDEX[s];
			loc_t k1 = Board::K1INDEX[s];
			int symLoc = Board::SYMLOC[pla][k0];
			int symDir = getSymDir(pla,k0,k1);
			int pieceIndex = data.pieceIndex[bb.owners[k0]][bb.pieces[k0]];
			if(bb.owners[k0] == pla)
				(*handleFeature)(fset.get(ACTUAL_STEP,pieceIndex,symDir*32 + symLoc),handleFeatureData);
			else
				(*handleFeature)(fset.get(ACTUAL_STEP_O,pieceIndex,symDir*32 + symLoc),handleFeatureData);

			bb.makeStep(s);

		}
	}

  //Actually make the move on a copy of the board, so we can compute some additional features easily
  Board copy = b;
  copy.makeMove(move);

  if(ENABLE_STRATS)
	{
  	for(int trapid = 0; trapid < 4; trapid++)
  	{
  		FrameThreat ft;
  		loc_t trap = Board::TRAPLOCS[trapid];
			if(Strats::findFrame(copy,pla,trap,&ft))
			{
				bool isEleEle = (Board::RADIUS[1][trap] & b.pieceMaps[0][ELE]).hasBits() &&
					              (Board::RADIUS[1][trap] & b.pieceMaps[1][ELE]).hasBits();
				(*handleFeature)(fset.get(FRAME_P,data.pieceIndex[pla][copy.pieces[trap]],isEleEle),handleFeatureData);
			}
			else if(Strats::findFrame(copy,opp,trap,&ft))
			{
				bool isEleEle = (Board::RADIUS[1][trap] & b.pieceMaps[0][ELE]).hasBits() &&
					              (Board::RADIUS[1][trap] & b.pieceMaps[1][ELE]).hasBits();
				(*handleFeature)(fset.get(FRAME_O,data.pieceIndex[opp][copy.pieces[trap]],isEleEle),handleFeatureData);
			}
  	}

  	BlockadeThreat bt;
  	if(Strats::findBlockades(copy,pla,&bt))
  	{
  		int centrality = Board::CENTRALITY[bt.pinnedLoc];
  		int gydist = Board::GOALYDIST[pla][bt.pinnedLoc];
  		bool usesEle = (bt.holderMap & copy.pieceMaps[pla][ELE]).hasBits();
  		(*handleFeature)(fset.get(EBLOCKADE_CENT_P,centrality,usesEle),handleFeatureData);
  		(*handleFeature)(fset.get(EBLOCKADE_GYDIST_P,gydist,usesEle),handleFeatureData);
  	}
  	bt = BlockadeThreat();
  	if(Strats::findBlockades(copy,opp,&bt))
  	{
  		int centrality = Board::CENTRALITY[bt.pinnedLoc];
  		int gydist = Board::GOALYDIST[opp][bt.pinnedLoc];
  		bool usesEle = (bt.holderMap & copy.pieceMaps[opp][ELE]).hasBits();
  		(*handleFeature)(fset.get(EBLOCKADE_CENT_O,centrality,usesEle),handleFeatureData);
  		(*handleFeature)(fset.get(EBLOCKADE_GYDIST_O,gydist,usesEle),handleFeatureData);
  	}
	}

  //Trap State Features
  if(ENABLE_TRAPSTATE)
  {
    for(int trapIndex = 0; trapIndex < 4; trapIndex++)
    {
      loc_t kt = Board::TRAPLOCS[trapIndex];
      int isOppTrap = !Board::ISPLATRAP[trapIndex][pla];
      int oldPlaTrapState = data.oldTrapStates[pla][trapIndex];
      int newPlaTrapState = ArimaaFeature::getTrapState(copy,pla,kt);
      int oldOppTrapState = data.oldTrapStates[opp][trapIndex];
      int newOppTrapState = ArimaaFeature::getTrapState(copy,opp,kt);

      if(oldPlaTrapState != newPlaTrapState)
      	(*handleFeature)(fset.get(TRAP_STATE_P,isOppTrap,oldPlaTrapState,newPlaTrapState),handleFeatureData);

      if(oldOppTrapState != newOppTrapState)
      	(*handleFeature)(fset.get(TRAP_STATE_O,isOppTrap,oldOppTrapState,newOppTrapState),handleFeatureData);
    }
  }

  if(ENABLE_TRAPCONTROL)
  {
  	int ufDist[64];
  	//We could use the one in data, but maybe a capture happened in the process of making the move
  	UFDist::get(copy,ufDist);
    int pStronger[2][NUMTYPES];
    Bitmap pStrongerMaps[2][NUMTYPES];
    copy.initializeStronger(pStronger);
    copy.initializeStrongerMaps(pStrongerMaps);

  	int tc[2][4];
  	Eval::getBasicTrapControls(copy,pStronger,pStrongerMaps,ufDist,tc);

  	for(int trapIndex = 0; trapIndex<4; trapIndex++)
  	{
  		int isOppTrap = !Board::ISPLATRAP[trapIndex][pla];
  		int val = tc[pla][trapIndex]/10 + 8;
  		if(val < 0) val = 0;
  		else if(val > 16) val = 16;

  		(*handleFeature)(fset.get(TRAP_CONTROL,isOppTrap,val),handleFeatureData);
  	}
  }

  //Capture Defense Features
  if(ENABLE_CAP_DEF)
  {
    loc_t plaEleLoc = copy.findElephant(pla);
    for(int j = 0; j<data.numOppCapThreats; j++)
    {
      unsigned char dist = data.oppCapThreats[j].dist;
      if(dist > 4)
        continue;
      loc_t kt = data.oppCapThreats[j].kt;
      int trapIndex = Board::TRAPINDEX[kt];
  		int isOppTrap = !Board::ISPLATRAP[trapIndex][pla];

      if(plaEleLoc != ERRORSQUARE && Board::ISADJACENT[plaEleLoc][kt])
      	(*handleFeature)(fset.get(CAP_DEF_ELEDEF,isOppTrap,dist),handleFeatureData);
      else if(copy.trapGuardCounts[pla][trapIndex] > b.trapGuardCounts[pla][trapIndex])
      	(*handleFeature)(fset.get(CAP_DEF_TRAPDEF,isOppTrap,dist),handleFeatureData);
      else
      {
        for(int i = 0; i<numChanges; i++)
        {
          if(src[i] == data.oppCapThreats[j].oloc && dest[i] != ERRORSQUARE)
          	(*handleFeature)(fset.get(CAP_DEF_RUNAWAY,isOppTrap,dist),handleFeatureData);
          else if(data.oppCapThreats[j].ploc != ERRORSQUARE && src[i] == data.oppCapThreats[j].ploc && dest[i] != ERRORSQUARE)
          	(*handleFeature)(fset.get(CAP_DEF_INTERFERE,isOppTrap,dist),handleFeatureData);
          else if(data.oppCapThreats[j].ploc != ERRORSQUARE && b.isThawed(data.oppCapThreats[j].ploc) && copy.isFrozen(data.oppCapThreats[j].ploc))
          	(*handleFeature)(fset.get(CAP_DEF_INTERFERE,isOppTrap,dist),handleFeatureData);
        }
      }
    }
  }

  if(ENABLE_GOAL_THREAT)
  {
    if(copy.getWinner() == pla)
    	(*handleFeature)(fset.get(WINS_GAME),handleFeatureData);

    int newPlaGoalDist = BoardTrees::goalDist(copy,pla,4);
    int newOppGoalDist = BoardTrees::goalDist(copy,opp,4);

    if(newPlaGoalDist < data.plaGoalDist && newPlaGoalDist <= 4 && newPlaGoalDist > 0)
    	(*handleFeature)(fset.get(THREATENS_GOAL,newPlaGoalDist),handleFeatureData);

    if(newOppGoalDist <= 4)
    	(*handleFeature)(fset.get(ALLOWS_GOAL),handleFeatureData);
  }

  //Where was the piece at this location prior to this move?
  loc_t priorLocation[64];
  for(int i = 0; i<64; i++)
    priorLocation[i] = i;
  for(int i = 0; i<numChanges; i++)
  {
    if(dest[i] != ERRORSQUARE)
      priorLocation[dest[i]] = src[i];
  }

  Bitmap capThreatMap;
  if(ENABLE_CAP_THREAT)
  {
    //Check pla capture threats around each trap
    for(int i = 0; i<4; i++)
    {
      int capDist;
      Bitmap map;
      //If we can capture...
      if(BoardTrees::canCaps(copy,pla,4,Board::TRAPLOCS[i],map,capDist))
      {
      	//Record capture threats outside for other stuff to use
        capThreatMap |= map;

        //Locate the biggest piece capturable
        loc_t biggestLoc = ERRORSQUARE;
        piece_t biggestPiece = EMP;
        while(map.hasBits())
        {
          loc_t loc = map.nextBit();
          if(copy.pieces[loc] > biggestPiece)
          {
            biggestLoc = loc;
            biggestPiece = copy.pieces[loc];
          }
        }

        int isOppTrap = !Board::ISPLATRAP[i][pla];

        //If it wasn't already been threatened, add it!
        if(!data.plaCapMap.isOne(priorLocation[biggestLoc]))
        	(*handleFeature)(fset.get(THREATENS_CAP,data.pieceIndex[opp][biggestPiece],capDist,isOppTrap),handleFeatureData);
      }
    }
    //Check for opponent capture threats around each trap
    Bitmap preventedMap = data.oppCapMap;
    for(int i = 0; i<4; i++)
    {
      int capDist;
      Bitmap map;
      //If opp can capture...
      if(BoardTrees::canCaps(copy,opp,4,Board::TRAPLOCS[i],map,capDist))
      {
      	//Record capture threats outside for other stuff to use
        capThreatMap |= map;

        //Locate the biggest piece capturable
        loc_t biggestLoc = ERRORSQUARE;
        piece_t biggestPiece = EMP;
        while(map.hasBits())
        {
          loc_t loc = map.nextBit();
          preventedMap.setOff(priorLocation[loc]);
          if(copy.pieces[loc] > biggestPiece)
          {
            biggestLoc = loc;
            biggestPiece = copy.pieces[loc];
          }
        }

        //If it wasn't already been threatened, it's an invitiation to capture
        if(!data.oppCapMap.isOne(priorLocation[biggestLoc]))
        {
        	int isOppTrap = !Board::ISPLATRAP[i][pla];

          //Was not moved
          if(priorLocation[biggestLoc] == biggestLoc)
          	(*handleFeature)(fset.get(INVITES_CAP_UNMOVED,data.pieceIndex[pla][biggestPiece],capDist,isOppTrap),handleFeatureData);
          else
          	(*handleFeature)(fset.get(INVITES_CAP_MOVED,data.pieceIndex[pla][biggestPiece],capDist,isOppTrap),handleFeatureData);
        }
      }
    }
    for(int i = 0; i<numChanges; i++)
    {
      if(dest[i] == ERRORSQUARE)
        preventedMap.setOff(src[i]);
    }
    //Did we successfully prevent any captures? Add them all in!
    while(preventedMap.hasBits())
    {
      loc_t loc = preventedMap.nextBit();
      (*handleFeature)(fset.get(PREVENTS_CAP,data.pieceIndex[pla][b.pieces[loc]],Board::SYMLOC[pla][loc]),handleFeatureData);
    }

  }

  if(ENABLE_FREEZE)
  {
  	Bitmap oppMap = (b.pieceMaps[opp][0] & b.frozenMap) ^ (copy.pieceMaps[opp][0] & copy.frozenMap);
  	while(oppMap.hasBits())
  	{
  		loc_t loc = oppMap.nextBit();
  		if(b.owners[loc] == opp && b.isFrozen(loc))
  		{
  			(*handleFeature)(fset.get(THAWS_OPP_STR,data.pieceIndex[opp][b.pieces[loc]]),handleFeatureData);
  			(*handleFeature)(fset.get(THAWS_OPP_AT,Board::SYMLOC[opp][loc]),handleFeatureData);
  		}
  		else
  		{
  			(*handleFeature)(fset.get(FREEZES_OPP_STR,data.pieceIndex[opp][copy.pieces[loc]]),handleFeatureData);
  			(*handleFeature)(fset.get(FREEZES_OPP_AT,Board::SYMLOC[opp][loc]),handleFeatureData);
  		}
  	}
  	Bitmap plaMap = (b.pieceMaps[pla][0] & b.frozenMap) ^ (copy.pieceMaps[pla][0] & copy.frozenMap);
  	while(plaMap.hasBits())
  	{
  		loc_t loc = plaMap.nextBit();
  		if(b.owners[loc] == pla && b.isFrozen(loc))
  		{
  			(*handleFeature)(fset.get(THAWS_PLA_STR,data.pieceIndex[pla][b.pieces[loc]]),handleFeatureData);
  			(*handleFeature)(fset.get(THAWS_PLA_AT,Board::SYMLOC[pla][loc]),handleFeatureData);
  		}
  		else
  		{
  			(*handleFeature)(fset.get(FREEZES_PLA_STR,data.pieceIndex[pla][copy.pieces[loc]]),handleFeatureData);
  			(*handleFeature)(fset.get(FREEZES_PLA_AT,Board::SYMLOC[pla][loc]),handleFeatureData);
  		}
  	}
  }


  if(ENABLE_PHALANX)
  {
  	//Phalanx can only be released if it was within radius 3 of a space we affected
  	Bitmap rad3AffectMap;
    for(int i = 0; i<numChanges; i++)
    {
    	rad3AffectMap |= Board::DISK[3][src[i]];
    	if(dest[i] != ERRORSQUARE)
    		rad3AffectMap |= Board::DISK[3][dest[i]];
    }
  	Bitmap oppMap = b.pieceMaps[opp][0] & copy.pieceMaps[opp][0] & rad3AffectMap;
  	while(oppMap.hasBits())
  	{
  		loc_t oloc = oppMap.nextBit();
  		for(int dir = 0; dir < 4; dir++)
  		{
  			if(!Board::ADJOKAY[dir][oloc])
  				continue;
  			loc_t adj = oloc + Board::ADJOFFSETS[dir];
  			if(ArimaaFeature::isSinglePhalanx(b,pla,oloc,adj) && !ArimaaFeature::isSinglePhalanx(copy,pla,oloc,adj))
  			{
  				(*handleFeature)(fset.get(RELEASES_PHALANX_VS,data.pieceIndex[opp][b.pieces[oloc]],1),handleFeatureData);
  				(*handleFeature)(fset.get(RELEASES_PHALANX_AT,Board::SYMLOC[pla][oloc],Board::SYMDIR[opp][3-dir],1),handleFeatureData);
  			}
  			else if(ArimaaFeature::isMultiPhalanx(b,pla,oloc,adj) && !ArimaaFeature::isMultiPhalanx(copy,pla,oloc,adj))
  			{
  				(*handleFeature)(fset.get(RELEASES_PHALANX_VS,data.pieceIndex[opp][b.pieces[oloc]],0),handleFeatureData);
  				(*handleFeature)(fset.get(RELEASES_PHALANX_AT,Board::SYMLOC[pla][oloc],Board::SYMDIR[opp][3-dir],0),handleFeatureData);
  			}
  		}
  	}

  	oppMap = copy.pieceMaps[opp][0] & rad3AffectMap;
  	while(oppMap.hasBits())
  	{
  		loc_t oloc = oppMap.nextBit();
  		for(int dir = 0; dir < 4; dir++)
  		{
  			if(!Board::ADJOKAY[dir][oloc])
  				continue;
  			loc_t adj = oloc + Board::ADJOFFSETS[dir];
  			if(!ArimaaFeature::isSinglePhalanx(b,pla,oloc,adj) && ArimaaFeature::isSinglePhalanx(copy,pla,oloc,adj))
  			{
					(*handleFeature)(fset.get(CREATES_PHALANX_VS,data.pieceIndex[opp][copy.pieces[oloc]],1),handleFeatureData);
					(*handleFeature)(fset.get(CREATES_PHALANX_AT,Board::SYMLOC[pla][oloc],Board::SYMDIR[opp][3-dir],1),handleFeatureData);
				}
  			else if(!ArimaaFeature::isMultiPhalanx(b,pla,oloc,adj) && ArimaaFeature::isMultiPhalanx(copy,pla,oloc,adj))
				{
  				(*handleFeature)(fset.get(CREATES_PHALANX_VS,data.pieceIndex[opp][copy.pieces[oloc]],0),handleFeatureData);
					(*handleFeature)(fset.get(CREATES_PHALANX_AT,Board::SYMLOC[pla][oloc],Board::SYMDIR[opp][3-dir],0),handleFeatureData);
				}
  		}
  	}
  }

  if(ENABLE_INFLUENCE)
  {
  	for(int i = 0; i<numChanges; i++)
		{
			if(b.owners[src[i]] == pla && dest[i] != ERRORSQUARE)
			{
				if(b.pieces[src[i]] == RAB)
				{
					int dist = Board::GOALYDIST[pla][dest[i]];
					int dy = (pla == GOLD ? 8 : -8);
					loc_t goalTarget = dest[i] + dist * dy;
					int influence = data.influence[pla][goalTarget] * 2;
					if(CW1(goalTarget)) {influence += data.influence[pla][goalTarget-1];}
					if(CE1(goalTarget)) {influence += data.influence[pla][goalTarget+1];}
					influence /= 4; //Averaging
					(*handleFeature)(fset.get(RABBIT_ADVANCE,dist,influence),handleFeatureData);
				}

				int advdest = Board::ADVANCEMENT[pla][dest[i]];
				int advsrc = Board::ADVANCEMENT[pla][src[i]];

				int influence;
				int file = dest[i] % 8;
				if(file <= 2) influence = data.influence[pla][Board::PLATRAPLOCS[OPP(pla)][0]];
				else if(file >= 5) influence = data.influence[pla][Board::PLATRAPLOCS[OPP(pla)][1]];
				else influence = (data.influence[pla][Board::PLATRAPLOCS[OPP(pla)][0]] + data.influence[pla][Board::PLATRAPLOCS[OPP(pla)][1]])/2;

				if(advdest > advsrc)
					(*handleFeature)(fset.get(
							PIECE_ADVANCE,data.pieceIndex[pla][b.pieces[src[i]]],influence),handleFeatureData);
				else if(advdest < advsrc)
					(*handleFeature)(fset.get(
							PIECE_RETREAT,data.pieceIndex[pla][b.pieces[src[i]]],influence),handleFeatureData);

				loc_t nearestDomSrc = b.nearestDominator(pla,b.pieces[src[i]],src[i],4);
				loc_t nearestDomDest = b.nearestDominator(pla,b.pieces[src[i]],dest[i],4);
				if(nearestDomSrc != ERRORSQUARE)
				{
					int rad = Board::MANHATTANDIST[src[i]][nearestDomSrc];
					if(nearestDomDest == ERRORSQUARE || rad < Board::MANHATTANDIST[dest[i]][nearestDomDest])
					{
						influence = data.influence[pla][src[i]];
						(*handleFeature)(fset.get(
								ESCAPE_DOMINATOR,data.pieceIndex[pla][b.pieces[src[i]]],influence,rad),handleFeatureData);
					}
				}
				else if(nearestDomDest != ERRORSQUARE)
				{
					int rad = Board::MANHATTANDIST[dest[i]][nearestDomDest];
					if(nearestDomSrc == ERRORSQUARE || rad < Board::MANHATTANDIST[src[i]][nearestDomSrc])
					{
						influence = data.influence[pla][dest[i]];
						(*handleFeature)(fset.get(
								APPROACH_DOMINATOR,data.pieceIndex[pla][b.pieces[src[i]]],influence,rad),handleFeatureData);
					}
				}

				if(Board::ISTRAP[dest[i]])
				{
					int trapIndex = Board::TRAPINDEX[dest[i]];
					int isOppTrap = !Board::ISPLATRAP[trapIndex][pla];
					if(copy.trapGuardCounts[pla][trapIndex] >= 2)
						(*handleFeature)(fset.get(SAFE_STEP_ON_TRAP,isOppTrap,data.pieceIndex[pla][b.pieces[src[i]]]),handleFeatureData);
					else
						(*handleFeature)(fset.get(UNSAFE_STEP_ON_TRAP,isOppTrap,data.pieceIndex[pla][b.pieces[src[i]]]),handleFeatureData);
				}
			}
		}

		Bitmap oppMap = b.pieceMaps[opp][0] & copy.pieceMaps[opp][0];
		while(oppMap.hasBits())
		{
			loc_t loc = oppMap.nextBit();
			if(copy.isDominated(loc))
			{
				loc_t priorLoc = priorLocation[loc];
				if(!b.isDominated(priorLoc))
				{
					int influence = data.influence[pla][loc];
					(*handleFeature)(fset.get(DOMINATES_ADJ,data.pieceIndex[opp][copy.pieces[loc]],influence),handleFeatureData);
				}
			}
		}
  }

  if(ENABLE_BLOCKING)
  {
  	//Find all opponent pieces pushpulled - should be exactly one
  	int numOppsPushpulled = 0;
  	loc_t oppPPSrc = ERRORSQUARE;
  	loc_t oppPPDest = ERRORSQUARE;
  	Bitmap plaDests;
    for(int i = 0; i<numChanges; i++)
    {
    	if(dest[i] == ERRORSQUARE)
    		continue;
      pla_t owner = b.owners[src[i]];
      if(owner == opp)
      {
      	numOppsPushpulled++;
      	oppPPSrc = src[i];
      	oppPPDest = dest[i];
      }
      else
      	plaDests.setOn(dest[i]);
    }

    //Exactly one opp piece pushed one space
    if(numOppsPushpulled == 1 && Board::ISADJACENT[oppPPSrc][oppPPDest])
    {
    	bool firstStrongerFound = false;
      for(int i = 0; i<numChanges; i++)
      {
      	if(dest[i] == ERRORSQUARE)
      		continue;
      	if(b.owners[src[i]] != pla)
      		continue;

      	//When are we eligible to be considered a helping blocker?

      	//Not eligible unless we are adjacent to its final location
      	if(!Board::ISADJACENT[oppPPDest][dest[i]])
      		continue;

      	//Not eligible if we started in its final location or were already adjacent to it
      	if(Board::MANHATTANDIST[src[i]][oppPPDest] <= 1)
      		continue;

      	//Not eligible if we were stronger than it and we ended in its start location
      	//or began adjacent to its start location or are the only piece that moved next to it
      	if(b.pieces[src[i]] > b.pieces[oppPPSrc] &&
      			(dest[i] == oppPPSrc || Board::ISADJACENT[oppPPSrc][src[i]] || (plaDests & ~Bitmap::makeLoc(dest[i])).isEmpty()))
      		continue;

      	//Not eligible if we are the first piece stronger passing the previous tests that it that moved next to it
      	if(!firstStrongerFound && b.pieces[src[i]] > b.pieces[oppPPSrc])
      	{
      		firstStrongerFound = true;
      		continue;
      	}

      	//Make sure we didn't step on a trap - that's weird.
      	if(Board::ISTRAP[dest[i]])
      		continue;

      	//So we stepped adjacent to it, yay.
      	int pieceIndex = data.pieceIndex[opp][b.pieces[oppPPSrc]];
      	bool isWeaker = b.pieces[src[i]] < b.pieces[oppPPSrc];
      	int symDir = getSymDir(pla,oppPPDest,dest[i]);
    		(*handleFeature)(fset.get(BLOCKS_PUSHPULLED,pieceIndex,isWeaker,symDir),handleFeatureData);

    		//While we're at it, if it's frozen...
    		if(copy.isFrozen(oppPPDest))
    			(*handleFeature)(fset.get(BLOCKS_FROZEN,pieceIndex,isWeaker,symDir),handleFeatureData);

    		//And if it's under capture threat...
    		if(capThreatMap.isOne(oppPPDest))
      		(*handleFeature)(fset.get(BLOCKS_CAPTHREATED,pieceIndex,isWeaker,symDir),handleFeatureData);
      }
    }

		//Check all other frozen opponent pieces
		Bitmap frozenOpp = copy.frozenMap & copy.pieceMaps[opp][0];
		if(oppPPDest != ERRORSQUARE)
			frozenOpp.setOff(oppPPDest);
		while(frozenOpp.hasBits())
		{
			loc_t oloc = frozenOpp.nextBit();
			Bitmap plaDestsAdj = plaDests & Board::RADIUS[1][oloc];
			while(plaDestsAdj.hasBits())
			{
				loc_t ploc = plaDestsAdj.nextBit();
				if(copy.pieces[ploc] > copy.pieces[oloc])
					continue;

				int pieceIndex = data.pieceIndex[opp][copy.pieces[oloc]];
				bool isWeaker = copy.pieces[ploc] < copy.pieces[oloc];
				int symDir = getSymDir(pla,oloc,ploc);
				(*handleFeature)(fset.get(BLOCKS_FROZEN,pieceIndex,isWeaker,symDir),handleFeatureData);
			}
		}

		//Check all other capturethreated opponent pieces
		Bitmap capThreatedOpp = capThreatMap & copy.pieceMaps[opp][0];
		if(oppPPDest != ERRORSQUARE)
			capThreatedOpp.setOff(oppPPDest);
		while(capThreatedOpp.hasBits())
		{
			loc_t oloc = capThreatedOpp.nextBit();
			if(!copy.isDominated(oloc))
				continue;

			Bitmap plaDestsAdj = plaDests & Board::RADIUS[1][oloc];
			while(plaDestsAdj.hasBits())
			{
				loc_t ploc = plaDestsAdj.nextBit();
				if(copy.pieces[ploc] > copy.pieces[oloc])
					continue;

				int pieceIndex = data.pieceIndex[opp][copy.pieces[oloc]];
				bool isWeaker = copy.pieces[ploc] < copy.pieces[oloc];
				int symDir = getSymDir(pla,oloc,ploc);
				(*handleFeature)(fset.get(BLOCKS_CAPTHREATED,pieceIndex,isWeaker,symDir),handleFeatureData);
			}
		}
  }

  if(ENABLE_REVFREECAPS)
  {
  	move_t reverseMove;
  	int rev = SearchUtils::isReversible(b,move,copy,reverseMove);
  	if(rev == 2)
  		(*handleFeature)(fset.get(IS_FULL_REVERSIBLE),handleFeatureData);
  	else if(rev == 1)
  		(*handleFeature)(fset.get(IS_MOSTLY_REVERSIBLE),handleFeatureData);
  	else
  	{
  		int numOppMoved = 0;
  		loc_t oppSrc = ERRORSQUARE;
  		loc_t oppDest = ERRORSQUARE;
  		for(int i = 0; i<numChanges; i++)
  		{
  			if(b.owners[src[i]] == opp && dest[i] != ERRORSQUARE)
  			{
  				numOppMoved++;
  				oppSrc = src[i];
  				oppDest = dest[i];
  			}
  		}

  		//Exactly one opp moved - see if it is able to step back
  		if(numOppMoved == 1)
  		{
  			if(Board::ISADJACENT[oppSrc][oppDest] && copy.owners[oppSrc] == NPLA &&
  					copy.isThawed(oppDest) && copy.isRabOkay(opp,oppDest,oppSrc))
  			{
  				(*handleFeature)(fset.get(PUSHPULL_OPP_REVERSIBLE),handleFeatureData);
  			}
  		}
  	}

  	int fc = SearchUtils::isFreeCapturable(b,move,copy);
  	if(fc == 2)
  		(*handleFeature)(fset.get(IS_FREE_CAPTURABLE),handleFeatureData);
  	else if(fc == 1)
  		(*handleFeature)(fset.get(IS_MOSTLY_FREE_CAPTURABLE),handleFeatureData);
  }

  if(ENABLE_LAST)
  {
  	int lastLastTurnNum = b.turnNumber-2;
  	if(lastLastTurnNum >= hist.minTurnNumber)
  	{
  		for(int i = 0; i<data.numLastPushed; i++)
  		{
  			loc_t lastPushed = data.lastPushed[i];
  			if(b.owners[lastPushed] == pla && (copy.owners[lastPushed] != pla || copy.pieces[lastPushed] != b.pieces[lastPushed]))
  			{
  				(*handleFeature)(fset.get(MOVES_LAST_PUSHED),handleFeatureData);
  				break;
  			}
  		}

  		int ns = Board::numStepsInMove(move);
    	int lastNearness = 0;
    	int lastLastNearness = 0;
  	  for(int i = 0; i<ns; i++)
  	  {
  	  	step_t s = Board::getStep(move,i);
  	  	if(s == PASSSTEP)
  	  		break;
  	  	loc_t loc = Board::K1INDEX[s];
  	  	lastNearness += data.lastMoveSum[loc];
  	  	lastLastNearness += data.lastLastMoveSum[loc];
  	  }
  	  if(lastNearness >= 64)
  	  	lastNearness = 63;
  	  if(lastLastNearness >= 64)
  	  	lastLastNearness = 63;

  	  (*handleFeature)(fset.get(MOVES_NEAR_LAST,lastNearness),handleFeatureData);
    	(*handleFeature)(fset.get(MOVES_NEAR_LAST_LAST,lastLastNearness),handleFeatureData);
  	}
  }

  if(ENABLE_DEPENDENCY)
  {
  	int numIndep = 0;
  	int numTotal = 0;
  	Bitmap dependDest;
  	Bitmap dependSrc;
  	//int contigScore = 0;
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

			//for(int j = 0; j<i; j++)
			//{
			//	loc_t othersrc = Board::K0INDEX[Board::getStep(move,j)];
			//	int contig = 6 - Board::MANHATTANDIST[src][othersrc];
			//	if(contig < 0)
			//		contig = 0;
			//	if(contig > 4)
			//		contig = 4;
			//	contigScore += contig;
			//}
	  }
	  (*handleFeature)(fset.get(NUM_INDEP_STEPS,numTotal,numIndep),handleFeatureData);

	  //if(contigScore > 24)
	  //	contigScore = 24;
	  //(*handleFeature)(fset.get(CONTIGUOUSITY,contigScore),handleFeatureData);
  }

  /*
  if(IS_PATTERN_INITIALIZED)
  {
  	int ns = Board::numStepsInMove(move);
  	TempRecord recs[ns];

  	int i;
  	for(i = 0; i<ns; i++)
  	{
  		step_t step = Board::getStep(move,i);
  		if(step == PASSSTEP || step == ERRORSTEP || step == QPASSSTEP)
  			break;

			loc_t k0 = Board::K0INDEX[step];
			int feature0 = ptree->lookup(b,pla,k0,data.pStronger);
			if(feature0 >= 0)
				(*handleFeature)(fset.get(PATTERN,feature0),handleFeatureData);

			loc_t k1 = Board::K1INDEX[step];
			int feature1 = ptree->lookup(b,pla,k1,data.pStronger);
			if(feature1 >= 0)
				(*handleFeature)(fset.get(PATTERN,feature1),handleFeatureData);

		  recs[i] = b.tempStepC(k0,k1);
  	}
  	i--;
  	for(; i>=0; i--)
  	{
  		b.undoTemp(recs[i]);
  	}
  }*/
}

//HELPERS-----------------------------------------------------------------

//0 = back, 1 = outside, 2 = inside, 3 = front
static int getSymDir(pla_t pla, loc_t loc, loc_t adj)
{
	int diff = adj-loc;
	if(pla == GOLD)
	{
		if(diff == -8) return 0;
		else if(diff == 8) return 3;
	}
	else
	{
		if(diff == 8) return 0;
		else if(diff == -8) return 3;
	}

	int x = loc & 0x7;
	if((x < 4 && diff == -1) || (x >= 4 && diff == 1))
		return 1;

	return 2;
}


//POSDATA----------------------------------------------------------------------

BOOST_STATIC_ASSERT(sizeof(MoveFeaturePosData) <= sizeof(FeaturePosData));
MoveFeaturePosData& MoveFeaturePosData::convert(FeaturePosData& data)
{
	//TODO dereferencing type-punned pointer might break strict-aliasing rules [-Wstrict-aliasing]
	return *((MoveFeaturePosData*)(&data));
}

const MoveFeaturePosData& MoveFeaturePosData::convert(const FeaturePosData& data)
{
	return *((const MoveFeaturePosData*)(&data));
}

void MoveFeature::getPosData(const Board& constB, const BoardHistory& hist, pla_t pla, FeaturePosData& dataBuf)
{
	MoveFeaturePosData& data = MoveFeaturePosData::convert(dataBuf);

	Board b = constB;
  pla_t opp = OPP(pla);

  if(ENABLE_CAP_DEF)
  {
  	UFDist::get(b,data.ufDist);
    Bitmap pStrongerMaps[2][NUMTYPES];
    b.initializeStrongerMaps(pStrongerMaps);

    data.numOppCapThreats = 0;
		int oppRDSteps[4] = {16,16,16,16};
		for(int j = 0; j<4; j++)
		{
			loc_t kt = Board::TRAPLOCS[j];
			data.numOppCapThreats += Threats::findCapThreats(b,opp,kt,data.oppCapThreats+data.numOppCapThreats,data.ufDist,pStrongerMaps,
					4,Threats::maxCapsPerPla-data.numOppCapThreats,oppRDSteps[j]);
		}
  }

  if(ENABLE_GOAL_THREAT)
  {
		data.plaGoalDist = BoardTrees::goalDist(b,pla,4);
		data.oppGoalDist = BoardTrees::goalDist(b,opp,4);
  }

  if(ENABLE_CAP_THREAT)
  {
		data.plaCapMap = Bitmap();
		data.oppCapMap = Bitmap();
		for(int i = 0; i<4; i++)
		{
			int capDist;
			BoardTrees::canCaps(b,pla,4,Board::TRAPLOCS[i],data.plaCapMap,capDist);
			BoardTrees::canCaps(b,opp,4,Board::TRAPLOCS[i],data.oppCapMap,capDist);
		}
  }

  b.initializeStronger(data.pStronger);

  for(piece_t piece = RAB; piece <= ELE; piece++)
	{
  	data.pieceIndex[GOLD][piece] = ArimaaFeature::getPieceIndexApprox(GOLD,piece,data.pStronger);
  	data.pieceIndex[SILV][piece] = ArimaaFeature::getPieceIndexApprox(SILV,piece,data.pStronger);
  }

  //Scale to 0-8 range
  if(ENABLE_INFLUENCE)
  {
		Eval::getInfluence(b, data.pStronger, data.influence);
		for(int i = 0; i<64; i++)
		{
			data.influence[0][i] /= 50;
			data.influence[0][i] += 4;
			if(data.influence[0][i] < 0)
				data.influence[0][i] = 0;
			else if(data.influence[0][i] > 8)
				data.influence[0][i] = 8;

			data.influence[1][i] /= 50;
			data.influence[1][i] += 4;
			if(data.influence[1][i] < 0)
				data.influence[1][i] = 0;
			else if(data.influence[1][i] > 8)
				data.influence[1][i] = 8;
		}
  }

  if(ENABLE_TRAPSTATE)
  {
		for(int i = 0; i<4; i++)
		{
			data.oldTrapStates[SILV][i] = ArimaaFeature::getTrapState(b,SILV,Board::TRAPLOCS[i]);
			data.oldTrapStates[GOLD][i] = ArimaaFeature::getTrapState(b,GOLD,Board::TRAPLOCS[i]);
		}
  }


  if(ENABLE_LAST)
  {
  	data.numLastPushed = 0;
  	int lastTurnNum = b.turnNumber-1;
  	if(lastTurnNum >= hist.minTurnNumber)
  	{
  	  loc_t src2[8];
  	  loc_t dest2[8];
  	  int num2 = hist.turnBoard[lastTurnNum].getChanges(hist.turnMove[lastTurnNum],src2,dest2);
  	  for(int i = 0; i<num2; i++)
  	  {
  	  	if(dest2[i] != ERRORSQUARE && hist.turnBoard[lastTurnNum].owners[src2[i]] == pla)
  	  		data.lastPushed[data.numLastPushed++] = dest2[i];
  	  }
  	  for(int i = 0; i<64; i++)
  	  {
  	  	data.lastMoveSum[i] = 0;
  	  	data.lastLastMoveSum[i] = 0;
  	  }

  	  {
				move_t move = hist.turnMove[lastTurnNum];
				int ns = Board::numStepsInMove(move);
				for(int i = 0; i<ns; i++)
				{
					step_t s = Board::getStep(move,i);
					if(s == PASSSTEP)
						break;
					loc_t loc = Board::K1INDEX[s];
					for(int j = 0;true;j++)
					{
						loc_t next = Board::SPIRAL[loc][j];
						int dist = Board::MANHATTANDIST[loc][next];
						if(dist >= 4)
							break;
						data.lastMoveSum[next] += 4-dist;
					}
				}
  	  }

  	  int lastLastTurnNum = lastTurnNum-1;
  	  if(lastLastTurnNum >= hist.minTurnNumber)
  	  {
				move_t move = hist.turnMove[lastLastTurnNum];
				int ns = Board::numStepsInMove(move);
				for(int i = 0; i<ns; i++)
				{
					step_t s = Board::getStep(move,i);
					if(s == PASSSTEP)
						break;
					loc_t loc = Board::K1INDEX[s];
					for(int j = 0;true;j++)
					{
						loc_t next = Board::SPIRAL[loc][j];
						int dist = Board::MANHATTANDIST[loc][next];
						if(dist >= 4)
							break;
						data.lastLastMoveSum[next] += 4-dist;
					}
				}
  	  }
  	}
  }

  if(ENABLE_STRATS)
	{
		for(int i = 0; i<4; i++)
		{
			FrameThreat ft;
			data.oppHoldsFrameAtTrap[i] = false;

			loc_t trapLoc = Board::TRAPLOCS[i];
			if(Strats::findFrame(b,OPP(pla),trapLoc,&ft))
			{
				data.oppHoldsFrameAtTrap[i] = true;
				data.oppFrameIsPartial[i] = ft.isPartial;
				data.oppFrameIsEleEle[i] =
						(Board::RADIUS[1][trapLoc] & b.pieceMaps[0][ELE]).hasBits() &&
						(Board::RADIUS[1][trapLoc] & b.pieceMaps[1][ELE]).hasBits();
			}
		}
	}

}










