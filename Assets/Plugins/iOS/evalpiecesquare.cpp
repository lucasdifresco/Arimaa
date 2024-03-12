
/*
 * evalpiecesquare.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <cstdio>
#include "global.h"
#include "board.h"
#include "eval.h"

//Multiply by 2.5
static const int PSSCOREFACTOR = 5;
static const int PSSCOREDIV = 2;

//Top is the player's side, bottom the opponent's side
static const int PIECETABLES[][64] =
{{
//ELEPHANT
-450,-350,-250,-180,-180,-250,-350,-450,
-160,-130, -95, -70, -70, -95,-130,-160,
 -90, -45,   0,   0,   0,   0, -45, -90,
 -60,  -5, +15, +30, +30, +15,  -5, -60,
 -60,  +8, +35, +35, +35, +35,  +8, -60,
 -90, -70, -10, +42, +42, -10, -70, -90,
-180,-140,-115, -70, -70,-115,-140,-180,
-450,-350,-250,-180,-180,-250,-350,-450,
},{
//CAMEL or HORSE w CAMEL captured
-150, -80, -70, -50, -50, -70, -80,-150,
 -35, -11, -13, -10, -10, -13, -11, -35,
  -4,  +2,  +1,  +1,  +1,  +1,  +2,  -4,
   0,  +5,  +5,  +1,  +1,  +5,  +5,   0,
  +6, +15, +10,   0,   0, +10, +15,  +6,
  +6, +25, +10,  -5,  -5, +10, +25,  +6,
   0,  +5, +20,  -5,  -5, +20,  +5,   0,
 -50, -25, -15, -15, -15, -15, -25, -50,
},{
//HORSE or DOG with much stuff captured
-150, -90, -80, -60, -60, -80, -90,-150,
 -35, -15, -10, -10, -10, -10, -15, -35,
 -20, +11,  -5,  +5,  +5,  -5, +11, -20,
	 0, +10, +10,  +5,  +5, +10, +10,   0,
	+5, +25, +20, +10, +10, +20, +25,  +5,
 +15, +50, +15, +12, +12, +15, +50, +15,
	 0, +25, +40, +15, +15, +40, +25,   0,
 -40, -15, -15, -15, -15, -15, -15, -40,
},{
//DOG with stuff captured
-100, -75, -65, -42, -42, -65, -75,-100,
 -22, -10,  -9,  -9,  -9,  -9, -10, -22,
  -5,  +6,  -2,  +3,  +3,  -2,  +6,  -5,
   0, +10, +10,  +6,  +6, +10, +10,   0,
  +8, +14, +16,  +8,  +8, +16, +14,  +8,
 +10, +20,  +8,  +6,  +6,  +8, +20, +10,
   0, +12, +12,  +6,  +6, +12, +12,   0,
 -25, -12, -12, -12, -12, -12, -12, -25,
},{
//DOG or CAT with much stuff captured
 -80, -60, -45, -25, -25, -45, -60, -80,
 -16,  -4,  +2,  -4,  -4,  +2,  -4, -16,
  -4,  +8,   0,  +4,  +4,   0,  +8,  -4,
	 0,  +8,  +8,  +6,  +6,  +8,  +8,   0,
	+4, +12, +12,  +8,  +8, +12, +12,  +4,
  +8, +12,  +4,  +6,  +6,  +4, +12,  +8,
	 0,  +4,  +8,  +4,  +4,  +8,  +4,   0,
 -20, -12, -12, -12, -12, -12, -12, -20,
},{
//CAT with some stuff captured
 -60, -45, -35, -25, -25, -35, -45, -60,
 -12,  -6,  +6,  -4,  -4,  +6,  -6, -12,
	-2,  +6,   0,  +6,  +6,   0,  +6,  -2,
	 0,  +6,  +6,  +4,  +4,  +6,  +6,   0,
	+4,  +6,  +4,  +2,  +2,  +4,  +6,  +4,
	 0,  +4,   0,  -2,  -2,   0,  +2,   0,
	-8,  -8,   0,  -8,  -8,   0,  -8,  -8,
 -20, -12, -12, -12, -12, -12, -12, -20,
},{
//CAT
 -50, -35, -28, -21, -21, -28, -35, -50,
  -8,  -4,  +8,   0,   0,  +8,  -4,  -8,
	-2,  +6,   0,  +4,  +4,   0,  +6,  -2,
	 0,  +6,  +6,  +3,  +3,  +6,  +6,   0,
	+2,  +4,  +4,  +2,  +2,  +4,  +4,  +2,
	 0,  +2,  -2,  -4,  -4,  -2,  +2,   0,
 -10, -10,  -2, -10, -10,  -2, -10, -10,
 -20, -12, -12, -12, -12, -12, -12, -20,
}};

static const int RABBITTABLE[64] =
{
  +1,  +1,   0,  -1,  -1,   0,  +1,  +1,
  +2,  +1,  +8,  -6,  -6,  +8,  +1,  +2,
  +4,  +4, -15,  -5,  -5, -15,  +4,  +4,
  +6,  +2,   0, -10, -10,   0,  +2,  +6,
  +8,  +4,   0, -12, -12,   0,  +4,  +8,
 +10,  +6, -10, -20, -20, -10,  +6, +10,
 +12, +12,   0,  +6,  +6,   0, +10, +12,
+999,+999,+999,+999,+999,+999,+999,+999,
};

//Modifier by numOppPieces*2 + numOppRabs, by row of advancement
static const int RABBITADVANCETABLE[25][8] =
{
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0},
{0, 9, 20, 33, 49, 68,148,0}, //4P7R or 5P5R or 6P3R or 7P1R
{0, 9, 20, 33, 49, 68,148,0}, //4P6R or 5P4R or 6P2R or 7P
{0, 9, 20, 33, 49, 68,148,0}, //3P7R or 4P5R or 5P3R or 6P1R
{0, 8, 17, 29, 43, 60,130,0}, //3P6R or 4P4R or 5P2R or 6P
{0, 7, 15, 25, 38, 53,114,0}, //2P7R or 3P5R or 4P3R or 5P1R
{0, 6, 13, 22, 34, 48,100,0}, //2P6R or 3P4R or 4P2R or 5P
{0, 5, 11, 19, 29, 41, 88,0}, //1P7R or 2P5R or 3P3R or 4P1R
{0, 4,  9, 16, 24, 34, 77,0}, //1P6R or 2P4R or 3P2R or 4P
{0, 3,  7, 13, 20, 29, 67,0}, //7R or 1P5R or 2P3R or 3P1R
{0, 3,  6, 11, 17, 25, 58,0}, //6R or 1P4R or 2P2R or 3P
{0, 2,  5,  9, 13, 18, 50,0}, //5R or 1P3R or 2P1R
{0, 1,  3,  6,  8, 11, 43,0}, //4R or 1P2R or 2P
{0, 1,  2,  3,  4,  5, 36,0}, //3R or 1P1R
{0, 0, -1, -1, -2, -2, 30,0}, //2R or 1P
{0,-1, -2, -4, -5, -7, 25,0}, //1R
{0,-2, -4, -6, -8,-10, 20,0}, //All
};

eval_t Eval::getPieceSquareScore(const Board& b, const int* plaStronger, const int* oppStronger, bool print)
{
	ARIMAADEBUG(if(print) cout << "Piece-Square" << endl;)

  //Swap based on player
  const int* goldStronger;
  const int* silvStronger;
  if(b.player == GOLD) {goldStronger = plaStronger; silvStronger = oppStronger;}
  else                 {goldStronger = oppStronger; silvStronger = plaStronger;}

  int goldScore = 0;
  for(int i = 0; i<64; i++)
  {
    if(b.owners[i] == GOLD)
    {
      piece_t piece = b.pieces[i];
      int dScore = 0;
      if(piece == RAB)
      {dScore = RABBITTABLE[i] + RABBITADVANCETABLE[goldStronger[piece]*2+b.pieceCounts[SILV][RAB]][i/8];}
      else
      {dScore = PIECETABLES[goldStronger[piece]][i];}

      goldScore += dScore;
      ARIMAADEBUG(if(print) {fprintf(stdout,"%+3d ",dScore);})
    }
    else if(b.owners[i] == SILV)
    {
      piece_t piece = b.pieces[i];
      int dScore = 0;
      if(piece == RAB)
      {dScore = RABBITTABLE[(i%8) + (7-i/8)*8] + RABBITADVANCETABLE[silvStronger[piece]*2+b.pieceCounts[GOLD][RAB]][7-i/8];}
      else
      {dScore = PIECETABLES[silvStronger[piece]][(i%8) + (7-i/8)*8];}

      goldScore -= dScore;
      ARIMAADEBUG(if(print) {fprintf(stdout,"%+3d ",dScore);})
    }
    else
    {
    	ARIMAADEBUG(if(print) {fprintf(stdout,"%+3d ",0);})
    }
    ARIMAADEBUG(if(print && i%8 == 7) {cout << endl;})
  }

  goldScore = goldScore*PSSCOREFACTOR/PSSCOREDIV;

  if(b.player == GOLD)
    return goldScore;
  else
    return -goldScore;
}

static int getLocs(const Board& b, pla_t pla, piece_t piece, loc_t locs[2])
{
  Bitmap pieceMap = b.pieceMaps[pla][piece];
  if(pieceMap.isEmpty())
    return 0;
  locs[0] = pieceMap.nextBit();
  if(pieceMap.isEmpty())
    return 1;
  locs[1] = pieceMap.nextBit();
  if(pieceMap.isEmpty())
    return 2;
  return 3;
}

//[bigpiece][smallpiece]
static const int HDIST[8][8] =
{
    {0,0,2,3,5,6,7,7},
    {0,0,2,3,5,6,7,7},
    {1,1,0,2,4,5,6,6},
    {3,3,1,0,2,4,5,5},
    {5,5,4,2,0,1,3,3},
    {6,6,5,4,2,0,1,1},
    {7,7,6,5,3,2,0,0},
    {7,7,6,5,3,2,0,0},
};

static int hdist(loc_t bigLoc, loc_t smallLoc)
{
  return HDIST[bigLoc&0x7][smallLoc&0x7];
}

//static const int ELE_PSHERIFF_HDIST_SCORE[8] =    {-22,-13, -1,  9, 18, 22, 22, 22};
static const int ELE_OSHERIFF_HDIST_SCORE[8] =    { 71, 60, 43, 30, 19, 12,  8,  7};
static const int SHERIFF_ODEPUTY_HDIST_SCORE[8] = { 64, 55, 41, 31, 22, 12,  5,  0};
static const int SHERIFF_OGEQ_HDIST_SCORE[9] =    {  0,  0,  1,  11, 22, 35, 47, 59, 65};
static const int DEPUTY_OGEQ_HDIST_SCORE[9] =     {  0,  0,  0,   7, 16, 23, 30, 36, 40};

//If the sheriff is advanced or the deputy is advanced, how much it's committed to its bonus/penalty, out of 64.
static const int DEFENSE_ADVANCE_COMMITMENT_SHERIFF[8] = {82,82,78,66,56,46,26,26};
static const int DEFENSE_ADVANCE_COMMITMENT[8] = {82,82,78,66,56,46,40,40};
static const int ATTACK_ADVANCE_COMMITMENT[8] = {40,40,44,56,64,72,74,74};
static const int DEFENSE_ADVANCE_COMMITMENT_SHERIFF_REV[8] = {26,26,46,56,66,78,82,82};
static const int DEFENSE_ADVANCE_COMMITMENT_REV[8] = {40,40,46,56,66,78,82,82};
static const int ATTACK_ADVANCE_COMMITMENT_REV[8] = {74,74,72,64,56,44,40,40};


//Pieces in the center are not as committed either way
static const int XPOS_COMMITMENT[8] = {64,64,52,24,24,52,64,64};

eval_t Eval::getPieceAlignmentScore(const Board& b, pla_t pla)
{
  pla_t opp = OPP(pla);

  loc_t plaEleLocs[2];
  loc_t oppEleLocs[2];
  loc_t plaSheriffLocs[2];
  loc_t oppSheriffLocs[2];
  loc_t plaDeputyLocs[2];
  loc_t oppDeputyLocs[2];

  //Ele and Sheriff must be nonzero, but deputies can be 0.
  //Rabbits may not be sherrifs or depties.

  if(getLocs(b,pla,ELE,plaEleLocs) != 1) return 0;
  piece_t plaPiece = CAM;
  while(plaPiece > RAB && b.pieceCounts[pla][plaPiece] == 0) plaPiece--;
  if(plaPiece <= RAB) return 0;
  piece_t plaSheriff = plaPiece;
  int numPlaSheriffs = getLocs(b,pla,plaPiece,plaSheriffLocs);
  plaPiece--;
  while(plaPiece > RAB && b.pieceCounts[pla][plaPiece] == 0) plaPiece--;
  piece_t plaDeputy = plaPiece;
  int numPlaDeputies;
  if(plaPiece <= RAB) {numPlaDeputies = 0;}
  else {numPlaDeputies = getLocs(b,pla,plaPiece,plaDeputyLocs);}

  if(getLocs(b,opp,ELE,oppEleLocs) != 1) return 0;
  piece_t oppPiece = CAM;
  while(oppPiece > RAB && b.pieceCounts[opp][oppPiece] == 0) oppPiece--;
  if(oppPiece <= RAB) return 0;
  piece_t oppSheriff = oppPiece;
  int numOppSheriffs = getLocs(b,opp,oppPiece,oppSheriffLocs);
  oppPiece--;
  while(oppPiece > RAB && b.pieceCounts[opp][oppPiece] == 0) oppPiece--;
  piece_t oppDeputy = oppPiece;
  int numOppDeputies;
  if(oppPiece <= RAB) {numOppDeputies = 0;}
  else {numOppDeputies = getLocs(b,opp,oppPiece,oppDeputyLocs);}

  DEBUGASSERT(numPlaSheriffs > 0 && numOppSheriffs > 0);

  int plaScore = 0;
  const int* DACS; const int* DAC; const int* AAC;
  {
    if(pla == GOLD) {DACS = DEFENSE_ADVANCE_COMMITMENT_SHERIFF; DAC = DEFENSE_ADVANCE_COMMITMENT; AAC = ATTACK_ADVANCE_COMMITMENT;}
    else            {DACS = DEFENSE_ADVANCE_COMMITMENT_SHERIFF_REV; DAC = DEFENSE_ADVANCE_COMMITMENT_REV; AAC = ATTACK_ADVANCE_COMMITMENT_REV;}
    //Keep ele away from own sheriff, if only one sheriff.
    //if(numPlaSheriffs == 1)
    //  plaScore += ELE_PSHERIFF_HDIST_SCORE[hdist(plaEleLocs[0],plaSheriffLocs[0])] * XPOS_COMMITMENT[plaSheriffLocs[0]&7] / 64;

    //Keep ele with opponent sheriff(s). Halve the value if we can also dominate the sheriff with our own
    if(numOppSheriffs == 1)
      plaScore += ELE_OSHERIFF_HDIST_SCORE[hdist(plaEleLocs[0],oppSheriffLocs[0])] / (plaSheriff > oppSheriff ? 2 : 1) *
        XPOS_COMMITMENT[oppSheriffLocs[0]&7] * DACS[oppSheriffLocs[0]/8] / 4096;
    else if(numOppSheriffs == 2)
      plaScore += (ELE_OSHERIFF_HDIST_SCORE[hdist(plaEleLocs[0],oppSheriffLocs[0])] *
                   XPOS_COMMITMENT[oppSheriffLocs[0]&7] * DACS[oppSheriffLocs[0]/8] +
                   ELE_OSHERIFF_HDIST_SCORE[hdist(plaEleLocs[0],oppSheriffLocs[1])] *
                   XPOS_COMMITMENT[oppSheriffLocs[1]&7] * DACS[oppSheriffLocs[1]/8])
                   * 3/2  / (plaSheriff > oppSheriff ? 2 : 1) / 4096;
    //Keep sheriff(s) with opponent deputies(s).
    {
      //Account for differences in strength
      int effectiveNumOppDeputies;
      loc_t* effectiveOppDeputyLocs;
      if(plaSheriff > oppSheriff)
      {effectiveNumOppDeputies = numOppSheriffs; effectiveOppDeputyLocs = oppSheriffLocs;}
      else
      {effectiveNumOppDeputies = numOppDeputies; effectiveOppDeputyLocs = oppDeputyLocs;}

      int temp = 0;
      for(int i = 0; i<numPlaSheriffs; i++)
      {
        if(effectiveNumOppDeputies == 1)
          temp += SHERIFF_ODEPUTY_HDIST_SCORE[hdist(plaSheriffLocs[i],effectiveOppDeputyLocs[0])] * DAC[effectiveOppDeputyLocs[0]/8] *
                      XPOS_COMMITMENT[plaSheriffLocs[i]&7] * AAC[plaSheriffLocs[i]/8] / 4096 / 64;
        else if(effectiveNumOppDeputies == 2)
          temp += (SHERIFF_ODEPUTY_HDIST_SCORE[hdist(plaSheriffLocs[i],effectiveOppDeputyLocs[0])] * DAC[effectiveOppDeputyLocs[0]/8] +
                       SHERIFF_ODEPUTY_HDIST_SCORE[hdist(plaSheriffLocs[i],effectiveOppDeputyLocs[1])] * DAC[effectiveOppDeputyLocs[1]/8]) *
                       XPOS_COMMITMENT[plaSheriffLocs[i]&7] * AAC[plaSheriffLocs[i]/8] / 4096 / 64;
      }
      plaScore += (numPlaSheriffs == 2 ? temp * 2/3 : temp);
    }

    //Bonus for sherrifs that are near nobody geq than them
    for(int i = 0; i<numPlaSheriffs; i++)
    {
      int minDist = hdist(oppEleLocs[0],plaSheriffLocs[i]);
      if(oppSheriff >= plaSheriff) {for(int j = 0; j<numOppSheriffs; j++) minDist = min(minDist, hdist(oppSheriffLocs[j],plaSheriffLocs[i]) + (oppSheriff == plaSheriff));}
      if(oppDeputy >= plaSheriff)  {for(int j = 0; j<numOppDeputies; j++) minDist = min(minDist, hdist(oppDeputyLocs[j],plaSheriffLocs[i]) + (oppDeputy == plaSheriff));}
      plaScore += SHERIFF_OGEQ_HDIST_SCORE[minDist] * AAC[plaSheriffLocs[i]/8]/64;
    }

    //Bonus for deputies that are near nobody geq than them
    for(int i = 0; i<numPlaDeputies; i++)
    {
      int minDist = hdist(oppEleLocs[0],plaDeputyLocs[i]);
      if(oppSheriff >= plaDeputy) {for(int j = 0; j<numOppSheriffs; j++) minDist = min(minDist, hdist(oppSheriffLocs[j],plaDeputyLocs[i]) + (oppSheriff == plaDeputy));}
      if(oppDeputy >= plaDeputy)  {for(int j = 0; j<numOppDeputies; j++) minDist = min(minDist, hdist(oppDeputyLocs[j],plaDeputyLocs[i]) + (oppDeputy == plaDeputy));}
      plaScore += DEPUTY_OGEQ_HDIST_SCORE[minDist] * AAC[plaDeputyLocs[i]/8]/64;
    }
  }

  //SAME THING, REFLECTED FOR THE OPPONENT--------------------
  int oppScore = 0;
  {
    if(opp == GOLD) {DACS = DEFENSE_ADVANCE_COMMITMENT_SHERIFF; DAC = DEFENSE_ADVANCE_COMMITMENT; AAC = ATTACK_ADVANCE_COMMITMENT;}
    else            {DACS = DEFENSE_ADVANCE_COMMITMENT_SHERIFF_REV; DAC = DEFENSE_ADVANCE_COMMITMENT_REV; AAC = ATTACK_ADVANCE_COMMITMENT_REV;}
    //Keep ele away from own sheriff, if only one sheriff.
    //if(numOppSheriffs == 1)
    //  oppScore += ELE_PSHERIFF_HDIST_SCORE[hdist(oppEleLocs[0],oppSheriffLocs[0])] * XPOS_COMMITMENT[oppSheriffLocs[0]&7] / 64;

    //Keep ele with plaonent sheriff(s). Halve the value if we can also dominate the sheriff with our own
    if(numPlaSheriffs == 1)
      oppScore += ELE_OSHERIFF_HDIST_SCORE[hdist(oppEleLocs[0],plaSheriffLocs[0])] / (oppSheriff > plaSheriff ? 2 : 1) *
        XPOS_COMMITMENT[plaSheriffLocs[0]&7] * DACS[plaSheriffLocs[0]/8] / 4096;
    else if(numPlaSheriffs == 2)
      oppScore += (ELE_OSHERIFF_HDIST_SCORE[hdist(oppEleLocs[0],plaSheriffLocs[0])] *
                   XPOS_COMMITMENT[plaSheriffLocs[0]&7] * DACS[plaSheriffLocs[0]/8] +
                   ELE_OSHERIFF_HDIST_SCORE[hdist(oppEleLocs[0],plaSheriffLocs[1])] *
                   XPOS_COMMITMENT[plaSheriffLocs[1]&7] * DACS[plaSheriffLocs[1]/8])
                   * 3/2  / (oppSheriff > plaSheriff ? 2 : 1) / 4096;
    //Keep sheriff(s) with plaonent deputies(s).
    {
      //Account for differences in strength
      int effectiveNumPlaDeputies;
      loc_t* effectivePlaDeputyLocs;
      if(oppSheriff > plaSheriff)
      {effectiveNumPlaDeputies = numPlaSheriffs; effectivePlaDeputyLocs = plaSheriffLocs;}
      else
      {effectiveNumPlaDeputies = numPlaDeputies; effectivePlaDeputyLocs = plaDeputyLocs;}

      int temp = 0;
      for(int i = 0; i<numOppSheriffs; i++)
      {
        if(effectiveNumPlaDeputies == 1)
          temp += SHERIFF_ODEPUTY_HDIST_SCORE[hdist(oppSheriffLocs[i],effectivePlaDeputyLocs[0])] * DAC[effectivePlaDeputyLocs[0]/8] *
                      XPOS_COMMITMENT[oppSheriffLocs[i]&7] * AAC[oppSheriffLocs[i]/8] / 4096 / 64;
        else if(effectiveNumPlaDeputies == 2)
          temp += (SHERIFF_ODEPUTY_HDIST_SCORE[hdist(oppSheriffLocs[i],effectivePlaDeputyLocs[0])] * DAC[effectivePlaDeputyLocs[0]/8] +
                       SHERIFF_ODEPUTY_HDIST_SCORE[hdist(oppSheriffLocs[i],effectivePlaDeputyLocs[1])] * DAC[effectivePlaDeputyLocs[1]/8]) *
                       XPOS_COMMITMENT[oppSheriffLocs[i]&7] * AAC[oppSheriffLocs[i]/8] / 4096 / 64;
      }
      oppScore += (numOppSheriffs == 2 ? temp * 2/3 : temp);
    }

    //Bonus for sherrifs that are near nobody geq than them
    for(int i = 0; i<numOppSheriffs; i++)
    {
      int minDist = hdist(plaEleLocs[0],oppSheriffLocs[i]);
      if(plaSheriff >= oppSheriff) {for(int j = 0; j<numPlaSheriffs; j++) minDist = min(minDist, hdist(plaSheriffLocs[j],oppSheriffLocs[i]) + (plaSheriff == oppSheriff));}
      if(plaDeputy >= oppSheriff)  {for(int j = 0; j<numPlaDeputies; j++) minDist = min(minDist, hdist(plaDeputyLocs[j],oppSheriffLocs[i]) + (plaDeputy == oppSheriff));}
      oppScore += SHERIFF_OGEQ_HDIST_SCORE[minDist] * AAC[oppSheriffLocs[i]/8]/64;
    }

    //Bonus for deputies that are near nobody geq than them
    for(int i = 0; i<numOppDeputies; i++)
    {
      int minDist = hdist(plaEleLocs[0],oppDeputyLocs[i]);
      if(plaSheriff >= oppDeputy) {for(int j = 0; j<numPlaSheriffs; j++) minDist = min(minDist, hdist(plaSheriffLocs[j],oppDeputyLocs[i]) + (plaSheriff == oppDeputy));}
      if(plaDeputy >= oppDeputy)  {for(int j = 0; j<numPlaDeputies; j++) minDist = min(minDist, hdist(plaDeputyLocs[j],oppDeputyLocs[i]) + (plaDeputy == oppDeputy));}
      oppScore += DEPUTY_OGEQ_HDIST_SCORE[minDist] * AAC[oppDeputyLocs[i]/8]/64;
    }
  }
  return (plaScore - oppScore) * 2;
}


