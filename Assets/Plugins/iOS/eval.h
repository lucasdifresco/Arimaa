
/*
 * eval.h
 * Author: davidwu
 */

#ifndef EVAL_H
#define EVAL_H

#include "threats.h"
#include "strats.h"
#include "evalparams.h"

typedef int eval_t;

namespace Eval
{
	//CONSTANTS--------------------------------------------------------------------
	//Evaluation approximately in millirabbits

	const eval_t LOSE = -1000000;                //LOSE + X = Lose in X steps
	const eval_t WIN = 1000000;                  //WIN - X = Win in X steps
	const eval_t LOSE_INDEFINITE = LOSE + 1000;  //Value for a loss, but no idea when
	const eval_t WIN_INDEFINITE = WIN - 1000;    //Value for a win, but no idea when
	const eval_t LOSE_TERMINAL = LOSE + 99999;   //Highest value that still is a loss
	const eval_t WIN_TERMINAL = WIN - 99999;     //Lowest value that still is a win

  /**
   * Evaluate the current board position and return the score.
   * @param b - the board
   * @param alpha - lower bound on score that we are interested in
   * @param beta - upper bound on score that we are interested in
   * @param print - print evaluation data to stdout?
   * @return the score for the board position
   */
	eval_t evaluate(Board& b, eval_t alpha, eval_t beta, bool print);

  eval_t evaluateWithParams(Board& b, pla_t mainPla, eval_t alpha, eval_t beta, const EvalParams& params, bool print);

  //INITIALIZATION-------------------------------------------------------------------

  /**
   * Initialize eval tables. Call this before using Eval!
   */
  void init();

  //Called by init
  void initMaterial();
  void initLogistic();

  //MATERIAL--------------------------------------------------------------

	//Compute an integer that uniquely represents the current material distribution for pla's pieces alone.
  int getMaterialIndex(const Board& b, pla_t pla);

  //Get the material score from pla's perspective on the given board.
  eval_t getMaterialScore(const Board& b, pla_t pla);

  //Fill arrays indicating for each player and piece type, its base material value.
  void initializeValues(const Board& b, eval_t pValues[2][NUMTYPES]);

  //PIECE SQUARE SCORE---------------------------------------------------------------

  eval_t getPieceSquareScore(const Board& b, const int* plaStronger, const int* oppStronger, bool print);

  //PIECE ALIGNMENT SCORE-----------------------------------------------------

  eval_t getPieceAlignmentScore(const Board& b, pla_t pla);

  //TRAP DEFENDER------------------------------------------------------------
  eval_t getTrapDefenderScore(const Board& b);

  //TRAP CONTROL-------------------------------------------------------------
  //Basic trap control evaluation
  //100 to 70 Iron control
  //40 to 60 Decent control
  //30 to 20 Tenuous control
  //0 to 10 - Shared

  void getBasicTrapControls(const Board& b, const int pStronger[2][NUMTYPES],
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDists[64], int tc[2][4]);

  //Approx contribution of the elephant to trap control
  int elephantTrapControlContribution(const Board& b, pla_t pla, const int ufDists[64], int trapIndex);

  eval_t getTrapControlScore(pla_t pla, int trapIndex, int control);

  //PIECE THREAT--------------------------------------------------------------

  void getBasicPieceThreats(const Board& b, const int pStronger[2][NUMTYPES],
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int pValues[2][NUMTYPES], const int ufDist[64],
  		const int tc[2][4], eval_t trapThreats[2][4], eval_t pieceThreats[64]);

  void getBonusPieceThreats(const Board& b, const int pStronger[2][NUMTYPES],
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int pValues[2][NUMTYPES], const int ufDist[64],
  		const int tc[2][4], eval_t trapThreats[2][4], eval_t pieceThreats[64]);

  int getSheriffAdvancementThreat(const Board& b, pla_t pla, const int ufDist[64], const int tc[2][4], bool print);

  void getInfluence(const Board& b, const int pStronger[2][NUMTYPES], int influence[2][64]);

  eval_t getRabbitThreats(const Board& b, pla_t pla, const int ufDist[64],
  		const int tc[2][4], const int influence[2][64]);


  //CAP THREATS--------------------------------------------------------------------------
  int evalCapThreat(const Board& b, const eval_t values[NUMTYPES], loc_t loc, int dist);

  int evalCapOccur(const Board& b, const eval_t values[NUMTYPES], loc_t loc, int dist);

  //STRATS--------------------------------------------------------------------------------
  const int frameThreatMax = Strats::maxFramesPerKT*4;
  const int hostageThreatMax = Strats::maxHostagesPerKT*2;
  const int blockadeThreatMax = Strats::maxBlockadesPerPla;
  const int numStratsMax = frameThreatMax + hostageThreatMax + blockadeThreatMax;

  //The top level function. Compute all strats and eval them, storing the count in numPStrats, the strats themselves
  //in pStrats, and the total score if all strats are used in stratScore. Updates pieceThreats with the new
  //value for strats that threaten pieces. Note that the returned strats values are only the value that they
  //add above and beyond the value in pieceThreats.
  //ufDist is not const because it is temporarily modified for the computation
  void getStrats(Board& b, pla_t mainPla, const eval_t pValues[2][NUMTYPES], const int pStronger[2][NUMTYPES],
  		const Bitmap pStrongerMaps[2][NUMTYPES], int ufDist[64], const int tc[2][4],
  		eval_t pieceThreats[64], int numPStrats[2], Strat pStrats[2][numStratsMax], eval_t stratScore[2], bool print);

  //double getFrameMobLoss(pla_t pla, loc_t loc, int mobilityLevel);

  //ufDist is not const because it is temporarily modified for the computation
  Strat evalFrame(const Board& b, pla_t pla, FrameThreat threat, const eval_t pValues[2][NUMTYPES],
  		const Bitmap pStrongerMaps[2][NUMTYPES], int ufDist[64], bool print);

  //int getFrameBestSFP(const Board& b, pla_t pla, FrameThreat threat, const int ufDist[64], double* advancementGood = NULL, bool print = false);

  int fillBlockadeRotationArrays(const Board& b, pla_t pla, loc_t* holderRotLocs, int* holderRotDists,
      loc_t* holderRotBlockers, bool* holderIsSwarm, int* minStrNeededArr, Bitmap heldMap, Bitmap holderMap, Bitmap rotatableMap,
      const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64]);

  void fillBlockadeRotationBonusSteps(Bitmap heldMap, Bitmap holderMap, int singleBonusSteps[64], int multiBonusSteps[64]);
  BlockadeRotation getBlockadeSingleRotation(const Board& b, pla_t pla, loc_t ploc, Bitmap heldMap, Bitmap holderMap,
      const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], int singleBonusSteps[64], int multiBonusSteps[64],
      bool& tryMultiSwarm, piece_t& minStrNeeded);
  BlockadeRotation getBlockadeMultiRotation(const Board& b, pla_t pla, loc_t ploc, Bitmap holderMap,
      const Bitmap pStrongerMaps[2][NUMTYPES]);


  int getFrameBreakDistance(const Board& b, pla_t pla, loc_t bloc, Bitmap defenseMap,
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64]);

  double getHostageMobLoss(pla_t pla, loc_t loc);

  Strat evalHostage(const Board& b, pla_t pla, HostageThreat threat, const eval_t pValues[2][NUMTYPES],
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], const int tc[2][4], bool print);

  //Get the proportion of the piece value that the hostage is supposed to be worth
  double evalHostageProp(const Board& b, pla_t pla, HostageThreat threat,
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], const int tc[2][4], bool& indefensible,
  		double* advancementGood = NULL, bool print = false);

  bool isHostageIndefensible(const Board& b, pla_t pla, HostageThreat threat, int runawayBS, int holderAttackDist,
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64]);

  int getHostageRunawayBreakSteps(const Board& b, pla_t pla, loc_t hostageLoc, loc_t holderLoc, const int ufDist[64]);
  int getHostageHolderBreakSteps(const Board& b, pla_t pla, loc_t holderLoc, int holderAttackDist);
  int getHostageNoETrapControl(const Board& b, pla_t pla, loc_t kt, const int ufDist[64], const int tc[2][4]);

  double getBlockadeMobLoss(pla_t pla, loc_t loc);

  Strat evalBlockade(const Board& b, pla_t pla, BlockadeThreat threat,
  		const Bitmap pStrongerMaps[2][NUMTYPES], int ufDist[64], bool print);

  int getBlockadeBestSFP(const Board& b, pla_t pla, BlockadeThreat threat,
      const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], int* baseSFP = NULL,
  		double* advancementGood = NULL, bool print = false);

  int getBlockadeBreakDistance(const Board& b, pla_t pla, loc_t bloc, Bitmap defenseMap,
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64]);

  void getMobility(const Board& b, pla_t pla, loc_t ploc, const Bitmap pStrongerMaps[2][NUMTYPES],
  		const int ufDist[64], Bitmap mobMap[5]);

  eval_t getEMobilityScore(const Board& b, pla_t pla, const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], bool print);



  //b is not const because the blockade detection code needs to plop down fake elephants to detect the tightness
  //of various loose blockades
  int evalEleBlockade(Board& b, pla_t mainPla, pla_t pla, const int pStronger[2][NUMTYPES], const Bitmap pStrongerMaps[2][NUMTYPES],
      const int tc[2][4], int ufDist[64], bool print);


  //CAPS---------------------------------------------------------------------

  //Find the best capture, above and beyond the existing threat on the piece
  //b is not const because tempstepping the board is used
  eval_t evalCaps(Board& b, pla_t pla, int numSteps, const eval_t pValues[2][NUMTYPES],
  		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], const eval_t pieceThreats[64],
  		const int numPStrats[2], const Strat pStrats[2][numStratsMax], eval_t& retRaw, loc_t& retCapLoc);


  //SANDBOX--------------------------------------------------------------------

  //value[ydist] * value[xpos] * tc
  //value[gdist] * value[xpos] * value[attackdist] * value[attackdistfront] * value[blocker] * value[sfpgoal]
  const int RAB_Y_DIST = 0;
  const int RAB_X_POS = 1;
  const int RAB_G_DIST = 2;
  const int RAB_UF_DIST = 3;
  const int RAB_ATTACK_DIST = 4;
  const int RAB_ATTACK_DIST_FRONT = 5;
  const int RAB_SFP_GOAL = 6;
  const int RAB_BLOCKER_AVG = 7;
  const int RAB_TC = 8;

  const int RAB_ARRAY_LEN = 9;

  double getRabbitThreatScore(const Board& b, pla_t pla, const int ufDist[64], const int tc[2][4], const int influence[2][64],
   		bool print, const EvalParams& params);
}


#endif
