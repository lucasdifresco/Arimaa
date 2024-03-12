
/*
 * evalthreats.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <cmath>
#include <cstdio>
#include "global.h"
#include "board.h"
#include "boardtreeconst.h"
#include "ufdist.h"
#include "evalparams.h"
#include "eval.h"
#include "arimaaio.h"

using namespace ArimaaIO;

//TRAP DEFENDER-------------------------------------------------------------

static const int TRAP_DEF_SCORE[5] = {0,0,80,100,100};
static const int TRAP_DEF_SCORE_BACK_BONUS = 60;
static const int TRAP_DEF_SCORE_SIDE_BONUS = 30;

eval_t Eval::getTrapDefenderScore(const Board& b)
{
	eval_t goldScore = 0;
	for(int i = 0; i<=1; i++)
	{
		goldScore += TRAP_DEF_SCORE[b.trapGuardCounts[GOLD][i]];
		if(b.owners[Board::TRAPBACKLOCS[i]] == GOLD)
			goldScore += TRAP_DEF_SCORE_BACK_BONUS;
		if(b.owners[Board::TRAPOUTSIDELOCS[i]] == GOLD)
			goldScore += TRAP_DEF_SCORE_SIDE_BONUS;
	}
	eval_t silvScore = 0;
	for(int i = 2; i<=3; i++)
	{
		silvScore += TRAP_DEF_SCORE[b.trapGuardCounts[SILV][i]];
		if(b.owners[Board::TRAPBACKLOCS[i]] == SILV)
			silvScore += TRAP_DEF_SCORE_BACK_BONUS;
		if(b.owners[Board::TRAPOUTSIDELOCS[i]] == SILV)
			silvScore += TRAP_DEF_SCORE_SIDE_BONUS;
	}
	return b.player == GOLD ? (goldScore-silvScore) : (silvScore-goldScore);
}


//TRAP CONTROL--------------------------------------------------------------

static const int TC_PIECE_RAD[16] =
{18,20,9,3,1,0,0,0,0,0,0,0,0,0,0,0}; //Out of 20
static const int TC_NS3_PIECE_RAD[16] =
{18,20,13,5,2,0,0,0,0,0,0,0,0,0,0,0}; //Out of 20

static const int TC_PIECE_STR[9] =
{21,16,14,12,11,10,9,9,9};
static const int TC_NS3_STR[9] =
{11,6,3,1,0,0,0,0,0};

static const int TC_ATTACKER_BONUS = 110;

static const int TC_NS1_FACTOR[2] =
{16,13}; //Out of 16
static const int TC_FREEZE_FACTOR[UFDist::MAX_UF_DIST+5] =
{40,36,32,28,25,21,20,20,20,20,20}; //Out of 40

static const int TC_DIV = 15*16*40;
static const int TC_TRAP_NUM_DEFS[5] =
{0,2,11,20,21};

static const loc_t TRAP_BACK_SQUARE[4] = {10,13,50,53};
static const loc_t TRAP_SIDE_SQUARE[4] = {17,22,41,46};

static const int TC_ATTACK_KEY_SQUARE = 7; //Bonuses for taking these squares with non-ELE
static const int TC_THREAT_KEY_SQUARE = 5; //Bonuses for threatening these squares with non-ELE
static const int TC_PUSHSAFER_KEY_SQUARE = 5; //Bonus for having a pushsafer key square
static const int TC_STUFF_TRAP_CENTER = 50;
static const int TC_STUFF_TRAP_SCALEDIV = 3;
static const int TC_STUFF_TRAP_MAX = 20;

static const int TC_STRONGEST_NONELE_RAD2_TRAP = 5;
static const int TC_STRONGEST_NONELE_RAD3_TRAP = 3;

static int LOGISTIC_ARR20[255];
static int LOGISTIC_ARR16[255];
static int LOGISTIC_INTEGRAL_ARR[255];

void Eval::initLogistic()
{
	for(int i = 0; i<255; i++)
	{
		int x = i-127;
		LOGISTIC_ARR20[i] = (int)(1000.0/(1.0+exp(-x/20.0)));
		LOGISTIC_ARR16[i] = (int)(1000.0/(1.0+exp(-x/16.0)));
		LOGISTIC_INTEGRAL_ARR[i] = (int)(1000.0*log(1.0+exp(x/20.0)));
	}
}

//Out of 1000,streched by 20
static int logisticProp20(int x)
{
	int i = x+127;
	if(i < 0)
		return LOGISTIC_ARR20[0];
	if(i > 254)
		return LOGISTIC_ARR20[254];
	return LOGISTIC_ARR20[i];
}

//Out of 1000,streched by 16
static int logisticProp16(int x)
{
	int i = x+127;
	if(i < 0)
		return LOGISTIC_ARR16[0];
	if(i > 254)
		return LOGISTIC_ARR16[254];
	return LOGISTIC_ARR16[i];
}

//Assumes not edge, assumes pla piece at loc!
static bool isPushSafer(const Board& b, pla_t pla, loc_t loc, const Bitmap pStrongerMaps[2][NUMTYPES])
{
	int plaCount = ISP(loc S) + ISP(loc W) + ISP(loc E) + ISP(loc N);
	if(plaCount >= 3)
		return true;
	return (pStrongerMaps[pla][b.pieces[loc]] & ~b.pieceMaps[OPP(pla)][ELE] & Board::DISK[2][loc] & ~b.frozenMap).isEmpty();
}

static bool canPushOffTrap(const Board& b, pla_t pla, loc_t trapLoc)
{
	pla_t opp = OPP(pla);
	if(ISO(trapLoc S) && GT(trapLoc, trapLoc S) && b.isOpen(trapLoc S)) return true;
	if(ISO(trapLoc W) && GT(trapLoc, trapLoc W) && b.isOpen(trapLoc W)) return true;
	if(ISO(trapLoc E) && GT(trapLoc, trapLoc E) && b.isOpen(trapLoc E)) return true;
	if(ISO(trapLoc N) && GT(trapLoc, trapLoc N) && b.isOpen(trapLoc N)) return true;
	return false;
}

static int numNonUndermined(const Board& b, pla_t pla, loc_t trapLoc)
{
	int num = 0;
	if(ISP(trapLoc S) && (!b.isDominatedByUF(trapLoc S) || !b.isOpen(trapLoc S))) num++;
	if(ISP(trapLoc W) && (!b.isDominatedByUF(trapLoc W) || !b.isOpen(trapLoc W))) num++;
	if(ISP(trapLoc E) && (!b.isDominatedByUF(trapLoc E) || !b.isOpen(trapLoc E))) num++;
	if(ISP(trapLoc N) && (!b.isDominatedByUF(trapLoc N) || !b.isOpen(trapLoc N))) num++;
	return num;
}

int Eval::elephantTrapControlContribution(const Board& b, pla_t pla, const int ufDists[64], int trapIndex)
{
	loc_t loc = b.findElephant(pla);
	if(loc == ERRORSQUARE)
		return 0;

	int mDist = Board::MANHATTANDIST[loc][Board::TRAPLOCS[trapIndex]];
	if(mDist > 4)
	  return 0;

	int strScore = TC_PIECE_STR[0];
  int strScoreNs3 = TC_NS3_STR[0];
  bool isNS1 = false;
  int freezeNS1Factor = TC_FREEZE_FACTOR[ufDists[loc]] * TC_NS1_FACTOR[isNS1];
  int tc = 0;
  tc += strScore * TC_PIECE_RAD[mDist];
  tc += strScoreNs3 * TC_NS3_PIECE_RAD[mDist];
  tc *= freezeNS1Factor;
  if(Board::ISPLATRAP[trapIndex][OPP(pla)])
    tc = tc * TC_ATTACKER_BONUS / 100;

  int tcVal = tc/TC_DIV;
  return tcVal;
}

void Eval::getBasicTrapControls(const Board& b, const int pStronger[2][NUMTYPES],
		const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDists[64], int tc[2][4])
{
	for(int i = 0; i<4; i++)
	{
		tc[0][i] = 0;
		tc[1][i] = 0;
	}

	//Piece strength and influence, numstronger, freezing
	for(loc_t loc = 0; loc < 64; loc++)
	{
		if(b.owners[loc] == NPLA)
			continue;
		pla_t owner = b.owners[loc];
		int strScore = TC_PIECE_STR[pStronger[owner][b.pieces[loc]]];
		int strScoreNs3 = TC_NS3_STR[(Board::DISK[3][loc] & pStrongerMaps[owner][b.pieces[loc]] & ~b.frozenMap).countBits()]; ///TODO count bits iterative?
		bool isNS1 = (Board::RADIUS[1][loc] & pStrongerMaps[owner][b.pieces[loc]] & ~b.frozenMap).hasBits();
		int freezeNS1Factor = TC_FREEZE_FACTOR[ufDists[loc]] * TC_NS1_FACTOR[isNS1];

		strScore *= freezeNS1Factor;
		strScoreNs3 *= freezeNS1Factor;
		for(int j = 0; j<4; j++)
		{
			int mDist = Board::MANHATTANDIST[loc][Board::TRAPLOCS[j]];
			if(mDist <= 4)
			{
				tc[owner][j] += strScore * TC_PIECE_RAD[mDist];
				tc[owner][j] += strScoreNs3 * TC_NS3_PIECE_RAD[mDist];
			}
		}
	}

	for(int i = 0; i<4; i++)
	{
		if(Board::ISPLATRAP[i][SILV]) tc[GOLD][i] = tc[GOLD][i] * TC_ATTACKER_BONUS / 100;
		else                          tc[SILV][i] = tc[SILV][i] * TC_ATTACKER_BONUS / 100;

		int tcVal = (tc[GOLD][i]-tc[SILV][i])/TC_DIV;
		loc_t trapLoc = Board::TRAPLOCS[i];

		for(piece_t piece = CAM; piece >= DOG; piece--)
		{
			bool g = (b.pieceMaps[GOLD][piece] & Board::DISK[2][trapLoc] & ~b.frozenMap).hasBits();
			bool s = (b.pieceMaps[SILV][piece] & Board::DISK[2][trapLoc] & ~b.frozenMap).hasBits();
			if(g && !s) tcVal += TC_STRONGEST_NONELE_RAD2_TRAP;
			else if (s && !g) tcVal -= TC_STRONGEST_NONELE_RAD2_TRAP;
			if(g || s)
				break;
		}
		for(piece_t piece = CAM; piece >= DOG; piece--)
		{
			bool g = (b.pieceMaps[GOLD][piece] & Board::DISK[3][trapLoc]).hasBits();
			bool s = (b.pieceMaps[SILV][piece] & Board::DISK[3][trapLoc]).hasBits();
			if(g && !s) tcVal += TC_STRONGEST_NONELE_RAD3_TRAP;
			else if (s && !g) tcVal -= TC_STRONGEST_NONELE_RAD3_TRAP;
			if(g || s)
				break;
		}

		//Key squares and phalanxes
		if(i <= 1)
		{
			loc_t keyloc = TRAP_BACK_SQUARE[i];
			if(b.owners[keyloc] == SILV && b.pieces[keyloc] != ELE)
				tcVal -= TC_ATTACK_KEY_SQUARE;
			else if(b.owners[keyloc] != SILV && (Board::RADIUS[1][keyloc] & pStrongerMaps[GOLD][b.pieces[keyloc]] & ~b.pieceMaps[SILV][ELE] & ~b.frozenMap).hasBits())
				tcVal -= TC_THREAT_KEY_SQUARE;

			keyloc = TRAP_SIDE_SQUARE[i];
			if(b.owners[keyloc] == SILV && b.pieces[keyloc] != ELE)
				tcVal -= TC_ATTACK_KEY_SQUARE;
			else if(b.owners[keyloc] != SILV && (Board::RADIUS[1][keyloc] & pStrongerMaps[GOLD][b.pieces[keyloc]] & ~b.pieceMaps[SILV][ELE] & ~b.frozenMap).hasBits())
				tcVal -= TC_THREAT_KEY_SQUARE;
		}
		else
		{
			loc_t keyloc = TRAP_BACK_SQUARE[i];
			if(b.owners[keyloc] == GOLD && b.pieces[keyloc] != ELE)
				tcVal += TC_ATTACK_KEY_SQUARE;
			else if(b.owners[keyloc] != GOLD && (Board::RADIUS[1][keyloc] & pStrongerMaps[SILV][b.pieces[keyloc]] & ~b.pieceMaps[GOLD][ELE] & ~b.frozenMap).hasBits())
				tcVal += TC_THREAT_KEY_SQUARE;

			keyloc = TRAP_SIDE_SQUARE[i];
			if(b.owners[keyloc] == GOLD && b.pieces[keyloc] != ELE)
				tcVal += TC_ATTACK_KEY_SQUARE;
			else if(b.owners[keyloc] != GOLD && (Board::RADIUS[1][keyloc] & pStrongerMaps[SILV][b.pieces[keyloc]] & ~b.pieceMaps[GOLD][ELE] & ~b.frozenMap).hasBits())
				tcVal += TC_THREAT_KEY_SQUARE;
		}

		//Number of defenders
		tcVal += TC_TRAP_NUM_DEFS[b.trapGuardCounts[GOLD][i]];
		tcVal -= TC_TRAP_NUM_DEFS[b.trapGuardCounts[SILV][i]];

		//Piece on trap
		int trapTcVal = 0;
		if(b.owners[trapLoc] == GOLD)
		{
			//Defense bonus for stuffing the trap, unless defenders are undermined
			if(b.trapGuardCounts[GOLD][i] >= 4 || numNonUndermined(b,GOLD,trapLoc) >= 2) {
				int v = tcVal < TC_STUFF_TRAP_CENTER ? (TC_STUFF_TRAP_CENTER - tcVal)/TC_STUFF_TRAP_SCALEDIV : 0;
				trapTcVal += v > TC_STUFF_TRAP_MAX ? TC_STUFF_TRAP_MAX : v;
			}
			//Attack penalty for stuffing the trap
			if((trapLoc == 42 || trapLoc == 45) && (b.trapGuardCounts[SILV][i] >= 2 || b.pieces[trapLoc] == RAB) && !canPushOffTrap(b,GOLD,trapLoc)) {
				int v = tcVal > TC_STUFF_TRAP_CENTER ? (TC_STUFF_TRAP_CENTER - tcVal)/TC_STUFF_TRAP_SCALEDIV : 0;
				trapTcVal += v < -TC_STUFF_TRAP_MAX ? -TC_STUFF_TRAP_MAX : v;
			}
			//Phalanx or pushsafe
			if(trapLoc == 18 || trapLoc == 21)
			{
				if(b.owners[TRAP_BACK_SQUARE[i]] == GOLD && b.trapGuardCounts[GOLD][i] >= 2 && isPushSafer(b,GOLD,TRAP_BACK_SQUARE[i],pStrongerMaps))
					trapTcVal += TC_PUSHSAFER_KEY_SQUARE;

				if(b.owners[TRAP_SIDE_SQUARE[i]] == GOLD && b.trapGuardCounts[GOLD][i] >= 2 && b.owners[trapLoc] == GOLD && isPushSafer(b,GOLD,TRAP_SIDE_SQUARE[i],pStrongerMaps))
					trapTcVal += TC_PUSHSAFER_KEY_SQUARE;
			}
		}
		else if(b.owners[trapLoc] == SILV)
		{
			//Defense bonus for stuffing the trap
			if(b.trapGuardCounts[SILV][i] >= 4 || numNonUndermined(b,SILV,trapLoc) >= 2) {
				int v = tcVal > -TC_STUFF_TRAP_CENTER ? (TC_STUFF_TRAP_CENTER + tcVal)/TC_STUFF_TRAP_SCALEDIV : 0;
				trapTcVal -= v > TC_STUFF_TRAP_MAX ? TC_STUFF_TRAP_MAX : v;
			}
			//Attack penalty for stuffing the trap
			if((trapLoc == 18 || trapLoc == 21) && (b.trapGuardCounts[GOLD][i] >= 2 || b.pieces[trapLoc] == RAB) && !canPushOffTrap(b,SILV,trapLoc)) {
				int v = tcVal < -TC_STUFF_TRAP_CENTER ? (TC_STUFF_TRAP_CENTER + tcVal)/TC_STUFF_TRAP_SCALEDIV : 0;
				trapTcVal -= v < -TC_STUFF_TRAP_MAX ? -TC_STUFF_TRAP_MAX : v;
			}

			if(trapLoc == 42 || trapLoc == 45)
			{
				//Phalanx or pushsafe
				if(b.owners[TRAP_BACK_SQUARE[i]] == SILV && b.trapGuardCounts[SILV][i] >= 2 && isPushSafer(b,SILV,TRAP_BACK_SQUARE[i],pStrongerMaps))
					trapTcVal -= TC_PUSHSAFER_KEY_SQUARE;

				//Phalanx or pushsafe
				if(b.owners[TRAP_SIDE_SQUARE[i]] == SILV && b.trapGuardCounts[SILV][i] >= 2 && b.owners[trapLoc] == SILV && isPushSafer(b,SILV,TRAP_SIDE_SQUARE[i],pStrongerMaps))
					trapTcVal -= TC_PUSHSAFER_KEY_SQUARE;
			}
		}
		int finalTrapVal = tcVal+trapTcVal;
		tc[GOLD][i] = finalTrapVal;
		tc[SILV][i] = -finalTrapVal;
	}
}


static const int TC_SCORE_VALUE = 850;
static const int TC_SCORE_CENTER = 25;


eval_t Eval::getTrapControlScore(pla_t pla, int trapIndex, int control)
{
	if((trapIndex < 2) == (pla == GOLD))
		return -logisticProp20(-control-TC_SCORE_CENTER) * TC_SCORE_VALUE / 1000;
	else
		return  logisticProp20(control-TC_SCORE_CENTER) * TC_SCORE_VALUE / 1000;
}

//PIECE THREATS---------------------------------------------------------------------


//Threat is from perspective of defending player. Evals are positive and should be _subtracted_ for the score.

static const int PTHREAT_TDIST_MAX = 12;
static const int PTHREAT_LOG_CENTER = 45;
static const int PTHREAT_TDIST_FACTOR[PTHREAT_TDIST_MAX+1] =
{41,40,39,37,34,22,19,14,12,6,4,2}; //Out of 100
static const int PTHREAT_TDIST_LOG_CENTER[12] =
{45,45,45,45,45,40,40,35,35,28,28,26};

static const int PTHREAT_HELPDIST_FACTOR[6] =
{38,52,66,80,90,100};

static const int TDIST_ADIST_MOD[6] =
{0,1,2,4,6,20};

//Adjusted piece value = (val + base)/prop
static const int PIECE_VAL_AVG_BASE = 500;
static const int PIECE_VAL_AVG_PROP = 15; //Out of 10

static const int PTHREAT_CHEAP_TC_REQ = 40;
static const int PTHREAT_CHEAP_CHECK[64] =
{
1,1,0,1,1,0,1,1,
1,0,0,0,0,0,0,1,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
1,0,0,0,0,0,0,1,
1,1,0,1,1,0,1,1,
};

static const int PTHREAT_CHEAP_TC_REQ2 = 55;
static const int PTHREAT_CHEAP_CHECK2[64] =
{
0,0,1,0,0,1,0,0,
0,1,0,1,1,0,1,0,
1,0,0,0,0,0,0,1,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
1,0,0,0,0,0,0,1,
0,1,0,1,1,0,1,0,
0,0,1,0,0,1,0,0,
};

void Eval::getBasicPieceThreats(const Board& b, const int pStronger[2][NUMTYPES],
		const Bitmap pStrongerMaps[2][NUMTYPES], const int pValues[2][NUMTYPES], const int ufDist[64],
		const int tc[2][4], eval_t trapThreats[2][4], eval_t pieceThreats[64])
{
	for(int i = 0; i<4; i++)
	{
		trapThreats[GOLD][i] = 0;
		trapThreats[SILV][i] = 0;
	}
	for(loc_t loc = 0; loc < 64; loc++)
	{
		pieceThreats[loc] = 0;
		if(b.owners[loc] == NPLA || pStronger[b.owners[loc]][b.pieces[loc]] == 0)
			continue;

		pla_t owner = b.owners[loc];
		if(PTHREAT_CHEAP_CHECK[loc] != 0 && tc[owner][Board::CLOSEST_TRAPINDEX[loc]] >= PTHREAT_CHEAP_TC_REQ)
			continue;
		if(PTHREAT_CHEAP_CHECK2[loc] != 0 && tc[owner][Board::CLOSEST_TRAPINDEX[loc]] >= PTHREAT_CHEAP_TC_REQ2 &&
				(Board::DISK[2][loc] & pStronger[owner][b.pieces[loc]]).isEmpty())
			continue;
		int aDist = Threats::attackDist(b,loc,5,pStrongerMaps,ufDist);
		if(aDist > 5)
			aDist = 5;
		int aDistMod = TDIST_ADIST_MOD[aDist];
		if(aDistMod > PTHREAT_TDIST_MAX)
			continue;
		int pieceValue = (pValues[owner][b.pieces[loc]]+PIECE_VAL_AVG_BASE)
		* PTHREAT_HELPDIST_FACTOR[ufDist[loc] > 5 ? 5 : ufDist[loc]]/(100*PIECE_VAL_AVG_PROP/10);
		for(int trapIndex = 0; trapIndex < 4; trapIndex++)
		{
			loc_t trapLoc = Board::TRAPLOCS[trapIndex];
			int tDist = aDistMod + Board::MANHATTANDIST[loc][trapLoc]*2;
			if(b.owners[trapLoc] == owner)
				tDist += 2;
			else if(b.owners[trapLoc] == OPP(owner) && b.pieces[trapLoc] <= b.pieces[loc])
				tDist += 1;
			tDist += b.trapGuardCounts[owner][trapIndex]*2 - Board::ISADJACENT[loc][trapLoc]*2;
			if(tDist > PTHREAT_TDIST_MAX)
				continue;
			tDist = tDist < 0 ? 0 : tDist;
			int prop = logisticProp16(-tc[owner][trapIndex]-PTHREAT_LOG_CENTER) * PTHREAT_TDIST_FACTOR[tDist] / 100;
			int tScore = prop * pieceValue / 1000;
			trapThreats[owner][trapIndex] += tScore;
			pieceThreats[loc] += tScore;
		}
	}
}

static const int PTHREAT2_BASE[2][64] = {
{
 70, 85, 85, 85, 85, 85, 85, 70,
 85, 90, 90,100,100, 90, 90, 85,
 90, 90, 72,100,100, 72, 90, 90,
 72, 76, 85, 87, 87, 85, 76, 72,
 43, 46, 48, 50, 50, 48, 46, 43,
 23, 24, 24, 28, 28, 24, 24, 23,
  7,  5,  0,  8,  8,  0,  5,  7,
  0,  0,  0,  0,  0,  0,  0,  0,
},
{
  0,  0,  0,  0,  0,  0,  0,  0,
  7,  5,  0,  8,  8,  0,  5,  7,
 23, 24, 24, 28, 28, 24, 24, 23,
 43, 46, 48, 50, 50, 48, 46, 43,
 72, 76, 85, 87, 87, 85, 76, 72,
 90, 90, 72,100,100, 72, 90, 90,
 85, 90, 90,100,100, 90, 90, 85,
 70, 85, 85, 85, 85, 85, 85, 70,
}};

static const double PTHREAT_PERCENT = 0.04;
static const int PTHREAT2_ADIST_FACTOR[5] =
{160,100,90,50,40};

static const double PTHREAT2_FROZEN_FACTOR = 1.1;
static const double PTHREAT2_AHEAD_WEAKER_FACTOR = 0.60;
static const double PTHREAT2_AHEAD_WEAKER_CLOSE_FACTOR = 0.35;
static const double PTHREAT2_AHEAD_WEAKER_CLOSEUF_FACTOR = 0.25;
static const double PTHREAT2_EQUAL_WEAKER_FACTOR = 0.85;
static const double PTHREAT2_PUSH_BLOCKED_FACTOR = 0.5;

void Eval::getBonusPieceThreats(const Board& b, const int pStronger[2][NUMTYPES],
		const Bitmap pStrongerMaps[2][NUMTYPES], const int pValues[2][NUMTYPES], const int ufDist[64],
		const int tc[2][4], eval_t trapThreats[2][4], eval_t pieceThreats[64])
{
	//Currently only for the camel...
	for(pla_t pla = 0; pla <= 1; pla++)
	{
		for(piece_t piece = CAM; piece >= HOR; piece--)
		{
			if(pStronger[pla][piece] != 1 || b.pieceCounts[pla][piece] <= 0)
				continue;

			Bitmap map = b.pieceMaps[pla][piece];
			while(map.hasBits())
			{
				loc_t loc = map.nextBit();
				int locx = loc%8;
				double baseValue = PTHREAT2_BASE[pla][loc];

				double tcFactor;
				if(locx <= 2)
					tcFactor = 0.35 - 0.012 * tc[pla][Board::PLATRAPINDICES[OPP(pla)][0]];
				else if(locx == 3)
					tcFactor = (2.0/3.0) * (0.35 - 0.012 * tc[pla][Board::PLATRAPINDICES[OPP(pla)][0]]) +
					           (1.0/3.0) * (0.35 - 0.012 * tc[pla][Board::PLATRAPINDICES[OPP(pla)][1]]);
				else if(locx == 1)
					tcFactor = (1.0/3.0) * (0.35 - 0.012 * tc[pla][Board::PLATRAPINDICES[OPP(pla)][0]]) +
					           (2.0/3.0) * (0.35 - 0.012 * tc[pla][Board::PLATRAPINDICES[OPP(pla)][1]]);
				else
					tcFactor = 0.35 - 0.012 * tc[pla][Board::PLATRAPINDICES[OPP(pla)][1]];

				if(tcFactor <= 0)
					continue;
				if(tcFactor >= 1)
					tcFactor = 1;

				baseValue *= tcFactor;

				int aDist = Threats::attackDist(b,loc,5,pStrongerMaps,ufDist);
				if(aDist >= 5)
					continue;
				baseValue *= PTHREAT2_ADIST_FACTOR[aDist];
				baseValue *= pValues[pla][piece];
				baseValue *= PTHREAT_PERCENT / 10000.0;

				if(ufDist[loc] > 0)
					baseValue *= PTHREAT2_FROZEN_FACTOR;

				int advOff = Board::ADVANCE_OFFSET[pla];
				for(int dx = -1; dx <= 1; dx++)
				{
					if(locx + dx < 0 || locx + dx > 7)
						continue;
					if(CA2(pla,loc) && b.owners[loc+dx+advOff*2] == pla && b.pieces[loc+dx+advOff*2] < piece)
						baseValue *= PTHREAT2_AHEAD_WEAKER_FACTOR;
					if(CA1(pla,loc) && b.owners[loc+dx+advOff] == pla && b.pieces[loc+dx+advOff] < piece)
						baseValue *= ufDist[loc+dx+advOff] <= 0 ? PTHREAT2_AHEAD_WEAKER_CLOSEUF_FACTOR : PTHREAT2_AHEAD_WEAKER_CLOSE_FACTOR;
					if(dx != 0 && b.owners[loc+dx] == pla && b.pieces[loc+dx] < piece)
						baseValue *= PTHREAT2_EQUAL_WEAKER_FACTOR;
				}
				bool noEmptySpace = (Board::ADVANCEMENT[pla][loc] <= 2 &&
						(!CW1(loc) || b.owners[loc-1+advOff] != NPLA) &&
						(             b.owners[loc  +advOff] != NPLA) &&
						(             b.owners[loc+2*advOff] != NPLA) &&
						(!CE1(loc) || b.owners[loc+1+advOff] != NPLA));
				if(noEmptySpace)
					baseValue *= PTHREAT2_PUSH_BLOCKED_FACTOR;

				int ktID = locx < 4 ? Board::PLATRAPINDICES[OPP(pla)][0] : Board::PLATRAPINDICES[OPP(pla)][1];
				pieceThreats[loc] += (int)baseValue;
				trapThreats[pla][ktID] += (int)baseValue;
			}
		}
	}
}

//CAMEL ADVANCEMENT--------------------------------------------------------------------
//Some slightly hacky extra code to reason about when and when not it is good to advance a camel
// (or generally the second strongest piece)
//It's good to advance when...
// Other pieces are in front of you.
// You have a piece either at A7 or C7
// You control the diagonal trap (or it's contested, where your tc is good and especially if your noeletc is good.
// The opponent's elephant is not near.
//
//It's not necessarily so great to advance if
//  The same-side trap is deadlocked Ele-Ele
//
//It's bad to advance when...
// You have no friends nearby
// You're the most advanced non-ele piece.

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

static const int HDIST_PENALTY[8] = {120,100,80,60,45,20,5,0};
static const int MDIST_PENALTY[16] = {190,190,170,150,110,75,40,20,10,0,0,0,0,0,0,0};

//Out of 64
static const int SHERIFF_PENALTY_MOD[64] = {
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  6,  8,  8, 12, 12,  8,  8,  6,
 20, 22, 28, 26, 26, 28, 22, 20,
 40, 46, 52, 54, 54, 52, 46, 40,
 68, 72, 64, 80, 80, 64, 72, 68,
 54, 66, 60, 78, 78, 60, 66, 54,
 16, 36, 42, 56, 56, 42, 36, 16,
};

//Assuming camel is on the left and the trap is on the right
//How much does the diagonal trap "count" for reducing the penalty?
static const int DIAGONAL_TRAP_MOD[64] = {
   64, 64, 64, 50, 38, 22, 20, 20,
   64, 64, 64, 50, 38, 22, 20, 20,
   64, 64, 60, 50, 38, 22, 20, 20,
   64, 64, 56, 40, 32, 22, 14, 12,
   64, 64, 56, 40, 20, 24, 10, 10,
   64, 64, 64, 30, 10,  0, 16,  8,
   64, 64, 64, 40, 24, 30,  8,  8,
   64, 64, 64, 40, 24, 20, 10,  8,
};

//Incrementing by 5s, from -40 to 60
static const int TC_MOD[21] = {
 64,64,64,64,62, 59,55,51,47,43, 39, 35,31,27,23,20, 16,12,9,7,5,
};
//Incrementing by 5s, from -20 to 80
static const int TC_NOE_MOD[21] = {
 64,64,64,64,62, 59,55,51,47,43, 39, 35,31,27,23,20, 16,12,9,7,5,
};

//Number of pieces in front, piece in front roughly scores 6, side scores 4, behind scores 2
//Elephant also counts as a piece in front
static const int IN_FRONT_MOD[26] = {
  64,61,58,55,52,49,46,43,41,38,36,33,30,27,24,21,19,17,15,14,13,12,12,12,12
};
static const int PELE_MDIST_MOD[16] = {48,48,50,52,54,58,62,64,67,68,69,70,71,72,72,72};

static const int FROZEN_MOD = 72;


int Eval::getSheriffAdvancementThreat(const Board& b, pla_t pla, const int ufDist[64], const int tc[2][4], bool print)
{
  piece_t piece = CAM;
  while(piece > RAB && b.pieceCounts[pla][piece] == 0)
    piece--;
  if(piece <= RAB)
    return 0;

  loc_t locs[2];
  int numLocs = getLocs(b,pla,piece,locs);
  if(numLocs >= 2 || numLocs <= 0) return 0;
  loc_t loc = locs[0];

  pla_t opp = OPP(pla);
  loc_t plaEleLoc = b.findElephant(pla);
  if(plaEleLoc == ERRORSQUARE)
    return 0;
  loc_t oppEleLoc = b.findElephant(opp);
  if(oppEleLoc == ERRORSQUARE)
    return 0;

  int gydist = Board::GOALYDIST[pla][loc];
  int spmod = SHERIFF_PENALTY_MOD[pla == GOLD ? loc : 63-loc];
  if(spmod <= 0 || gydist >= 7)
    return 0;
  int base = HDIST_PENALTY[hdist(oppEleLoc,loc)] + MDIST_PENALTY[Board::MANHATTANDIST[oppEleLoc][loc]];

  int infront = 0;
  int gy = (pla == GOLD) ? 8 : -8;
  int dyEnd = gydist;
  if(CW1(loc) && b.owners[loc-1-gy] == pla)
    infront += 2;
  if(b.owners[loc-gy] == pla)
    infront += 2;
  if(CE1(loc) && b.owners[loc+1-gy] == pla)
    infront += 2;

  if(CW2(loc) && b.owners[loc-2] == pla)
    infront += 2;
  if(CW1(loc) && b.owners[loc-1] == pla)
    infront += 4;
  if(CE1(loc) && b.owners[loc+1] == pla)
    infront += 4;
  if(CE2(loc) && b.owners[loc+2] == pla)
    infront += 2;

  for(int dy = 1; dy <= dyEnd; dy++)
  {
    int infrontTemp = 0;
    if(CW2(loc) && b.owners[loc-2+dy*gy] == pla)
      infrontTemp += 3;
    if(CW1(loc) && b.owners[loc-1+dy*gy] == pla)
      infrontTemp += 6;
    if(b.owners[loc+dy*gy] == pla)
      infrontTemp += 6;
    if(CE1(loc) && b.owners[loc+1+dy*gy] == pla)
      infrontTemp += 6;
    if(CE2(loc) && b.owners[loc+2+dy*gy] == pla)
      infrontTemp += 3;

    //Score extra in-frontness for pieces on the second line
    if(dy == dyEnd-1)
      infrontTemp = infrontTemp*3/2;
    infront += infrontTemp;
  }

  int ifmod = IN_FRONT_MOD[min(infront,25)];
  int pemod = PELE_MDIST_MOD[Board::MANHATTANDIST[plaEleLoc][loc]];

  int x = loc % 8;
  int tcmod0;
  {
    int tcidx = Board::PLATRAPINDICES[opp][0];
    int tcontrol = tc[pla][tcidx];
    int tcnoele = tcontrol + elephantTrapControlContribution(b,opp,ufDist,tcidx);
    int idx = tcontrol/5+8;
    if(idx < 0) idx = 0;
    if(idx > 20) idx = 20;
    int idx2 = tcnoele/5+4;
    if(idx2 < 0) idx2 = 0;
    if(idx2 > 20) idx2 = 20;
    int tcmod = (TC_MOD[idx] + TC_NOE_MOD[idx2])/2;
    tcmod0 = 64-((64-tcmod) * DIAGONAL_TRAP_MOD[(7-x)+(7-gydist)*8])/64;
  }
  int tcmod1;
  {
    int tcidx = Board::PLATRAPINDICES[opp][1];
    int tcontrol = tc[pla][tcidx];
    int tcnoele = tcontrol + elephantTrapControlContribution(b,opp,ufDist,tcidx);
    int idx = tcontrol/5+8;
    if(idx < 0) idx = 0;
    if(idx > 20) idx = 20;
    int idx2 = tcnoele/5+4;
    if(idx2 < 0) idx2 = 0;
    if(idx2 > 20) idx2 = 20;
    int tcmod = (TC_MOD[idx] + TC_NOE_MOD[idx2])/2;
    tcmod1 = 64-((64-tcmod) * DIAGONAL_TRAP_MOD[x+(7-gydist)*8])/64;
  }
  int frozenMod = b.isFrozen(loc) ? FROZEN_MOD : 64;
  int score = (base * spmod * ifmod * pemod)/4096 * tcmod0 * tcmod1 / 4096 * frozenMod / 4096 * 4/3;

  ARIMAADEBUG(if(print)
  {
    cout << "base " << -base << endl;
    cout << "spmod " << spmod << endl;
    cout << "ifmod " << ifmod << endl;
    cout << "pemod " << pemod << endl;
    cout << "tcmod0 " << tcmod0 << endl;
    cout << "tcmod1 " << tcmod1 << endl;
    cout << "score " << -score << endl;
    cout << endl;
  })

  return -score;
}



//RABBIT THREATS---------------------------------------------------------------

//Second try...

//So, what matters for rabbit threats?
//Advancement - decreases the distance to goal and increases the branchiness and width of possible threats
//Blockers
  //Increases the distance to goal and interferes with various branches of threat
  //Cats and stronger are extra good because of freezing and pushpulling. Rabbit defenders are not nearly as good.
  //Rabbits are less effective on the sides, much less effective when passed.
  //Less effective if the blocker is threatened, or dominated, or frozen.
  //Less effective if the rabbit cannot be perma-pushed or pulled back, or permafrozen
  //Less effective if the blocker simply can't reach
  //Less effective if blockers cannot reach (especially when own rabbits block way, or wouldbeUF on the path)
	//Blockers are more easily push-out-of-wayable when the helper is on the side, rather than in front.
//Empty space
  //All else equal, more empty space is better. In the extreme case, even if pieces are friendly, if they are all
  //packed it, it can be tricky.
//Helpers
  //Having the absolute strongest or equally strongest piece in an area is a huge bonus.
  //More helpers is usually good.
  //If the helper is first-line pincered, it may actually be a detrminent to goal attack.
//Trap control
  //Having local trap control is good.

//Pairwise interactions
  //Trap control is better when there are too many blockers. Much less useful when there is a ton of empty space.
  //More helpers are better when there are more blockers.
  //Advancement is better than other ways of decreasing goal distance.

//Sum in front of the advanced rabbit - weighted distance to reach for nearby pieces, taking into account blocking



//RABBIT THREATS----------------------------------------------------------------

static const int RTHREAT_SCALE_FACTOR = 120;
static const int RTHREAT_GDIST_BASE_PTS[8] =
{2000,1700,1200,900,650,400,200,100};

static const int RTHREAT_UFDIST_FACTOR[5] =
{100,85,70,50,25};
static const int RTHREAT_YINFLUENCE[4] =
{100,100,70,40};
static const int RTHREAT_INFLUENCE_PCT = 70;
static const int RTHREAT_TC_THREAT_PCT = 30;
static const int RTHREAT_TC_FACTOR[4][64] =
{
{
20,20,20,20,20,20,20,20,
20,20,20,16, 8, 0, 0, 0,
20,20,20,14, 8, 0, 0, 0,
20,20,20,12, 8, 0, 0, 0,
14,14,12,10, 8, 0, 0, 0,
10,10,10, 8, 6, 0, 0, 0,
 4, 4, 4, 4, 2, 0, 0, 0,
 2, 2, 2, 2, 2, 0, 0, 0,
},
{
20,20,20,20,20,20,20,20,
 0, 0, 0, 8,16,20,20,20,
 0, 0, 0, 8,14,20,20,20,
 0, 0, 0, 8,12,20,20,20,
 0, 0, 0, 8,10,12,14,14,
 0, 0, 0, 6, 8,10,10,10,
 0, 0, 0, 2, 4, 4, 4, 4,
 0, 0, 0, 2, 2, 2, 2, 2,
},
{
 2, 2, 2, 2, 2, 0, 0, 0,
 4, 4, 4, 4, 2, 0, 0, 0,
10,10,10, 8, 6, 0, 0, 0,
14,14,12,10, 8, 0, 0, 0,
20,20,20,12, 8, 0, 0, 0,
20,20,20,14, 8, 0, 0, 0,
20,20,20,16, 8, 0, 0, 0,
20,20,20,20,20,20,20,20,
},
{
 0, 0, 0, 2, 2, 2, 2, 2,
 0, 0, 0, 2, 4, 4, 4, 4,
 0, 0, 0, 6, 8,10,10,10,
 0, 0, 0, 8,10,12,14,14,
 0, 0, 0, 8,12,20,20,20,
 0, 0, 0, 8,14,20,20,20,
 0, 0, 0, 8,16,20,20,20,
20,20,20,20,20,20,20,20,
},
};

eval_t Eval::getRabbitThreats(const Board& b, pla_t pla, const int ufDist[64],
		const int tc[2][4], const int influence[2][64])
{
	pla_t opp = OPP(pla);
	Bitmap rMap = b.pieceMaps[pla][RAB];
	int dy = pla == GOLD ? 8 : -8;
	loc_t endBase = pla == GOLD ? 56 : 0;
	int totalScore = 0;
	int bestScore = 0;
	while(rMap.hasBits())
	{
		loc_t rloc = rMap.nextBit();
		loc_t goalLoc = (rloc % 8) + endBase;

		int influenceSum = 0;
		int div = 4;
		for(int j = 0; j<4; j++)
			influenceSum += RTHREAT_YINFLUENCE[j] * (influence[pla][goalLoc-j*dy] + (b.owners[goalLoc-j*dy] != opp ? 0 : -20));

		if(CW1(rloc))
		{
			div += 4;
			for(int j = 0; j<4; j++)
				influenceSum += RTHREAT_YINFLUENCE[j] * (influence[pla][goalLoc-j*dy-1] + (b.owners[goalLoc-j*dy-1] != opp ? 0 : -20));
		}
		if(CE1(rloc))
		{
			div += 4;
			for(int j = 0; j<4; j++)
				influenceSum += RTHREAT_YINFLUENCE[j] * (influence[pla][goalLoc-j*dy+1] + (b.owners[goalLoc-j*dy+1] != opp ? 0 : -20));
		}
		int influenceAvg = influenceSum/(div*100);
		int influencePct = (influenceAvg+50);
		if(influencePct < 0)
			influencePct = 0;
		else if(influencePct > 100)
			influencePct = 100;

		int trapPct = 0;
		for(int i = 0; i<2; i++)
		{
			loc_t trapLoc = Board::PLATRAPLOCS[opp][i];
			int trapIndex = Board::TRAPINDEX[trapLoc];
			int pct = tc[pla][trapIndex]+30;
			if(pct < 0)
				continue;
			else if(pct > 100)
				pct = 100;
			pct = pct * RTHREAT_TC_FACTOR[trapIndex][rloc] / 20;
			trapPct += pct;
		}

		int totalPctTimes100 = influencePct * RTHREAT_INFLUENCE_PCT + trapPct * RTHREAT_TC_THREAT_PCT;
		int basePts = RTHREAT_GDIST_BASE_PTS[Board::GOALYDIST[pla][rloc]];
		int score = basePts * totalPctTimes100 / 10000;

		score = score * RTHREAT_UFDIST_FACTOR[ufDist[rloc] > 4 ? 4 : ufDist[rloc]]/100;

		totalScore += score;
		if(score > bestScore)
			bestScore = score;

	}
	return (bestScore/2 + totalScore/2)*RTHREAT_SCALE_FACTOR/100;
}

//CAPTHREATS----------------------------------------------------------------------

static const int CAPTHREATDIV = 1000;

static const int capThreatPropLen = 24;
static const int capOccurPropLen = 24;
static const int capThreatProp[capThreatPropLen] =
{800,230,200,170,140, 70, 56, 44, 34, 24, 18,13, 8, 4, 2,1,0,0,0,0,0,0,0,0};
static const int capOccurProp[capOccurPropLen] =
{800,760,750,740,730,520,500,450,400,300,230,170,90,20,12,6,3,1,0,0,0,0,0,0};

int Eval::evalCapThreat(const Board& b, const eval_t values[NUMTYPES], loc_t loc, int dist)
{
  if(dist < 0) dist = 0;
  return dist >= capThreatPropLen ? 0 : (capThreatProp[dist]*values[b.pieces[loc]])/CAPTHREATDIV;
}

int Eval::evalCapOccur(const Board& b, const eval_t values[NUMTYPES], loc_t loc, int dist)
{
  if(dist < 0) dist = 0;
  return dist >= capOccurPropLen ? 0 : (capOccurProp[dist]*values[b.pieces[loc]])/CAPTHREATDIV;
}



//SANDBOX-------------------------------------------------------------------------

//TODO if we're only calling this behind the traps, maybe we can make it slightly asymmetric towards the edges?
static const int RABBIT_SFP_REGION[6][9] =
{
		{ 3,18,35,45,48,45,35,18, 3},
		{ 4,20,38,47,50,47,38,20, 4},
    { 4,20,38,47,50,47,38,20, 4},
		{ 2,13,26,40,46,40,26,13, 2},
		{ 1, 5,12,24,32,24,12, 5, 1},
		{ 0, 1, 4, 9,13, 9, 4, 1, 0},
};
static const int RABBIT_SFP_CENTRAL_REGION[6][10] =
{
    { 3,21,53,80,93,93,80,53,21, 3},
    { 4,24,58,85,97,97,85,58,24, 4},
    { 4,24,58,85,97,97,85,58,24, 4},
    { 2,15,39,66,86,86,66,39,15, 2},
    { 1, 6,17,36,56,56,36,17, 6, 1},
    { 0, 1, 5,13,22,22,13, 5, 1, 0},
};

static const int RABBIT_SFP_PIECE_VALUES[64] =
{95,60,45,36,30,25,21,19,17,15,14,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,};

static int getRabbitSFP(const Board& b, pla_t pla, int locx, bool central)
{
	unsigned int pieceCounts[2][NUMTYPES];
	for(int i = 0; i<NUMTYPES; i++)
		pieceCounts[0][i] = pieceCounts[1][i] = 0;

	int locy = Board::GOALY[pla] * 8;
	int goallocinc =  Board::GOALLOCINC[pla];
	if(central)
	{
	  int leftBound = locx >= 4 ? locx - 4 : 0;
	  int rightBound = locx >= 3 ? 7 : locx + 5;

    for(int gy = 0; gy<6; (gy++,locy -= goallocinc))
    {
      for(int x = leftBound; x <= rightBound; x++)
      {
        int k = locy + x;
        if(b.owners[k] == NPLA)
          continue;
        int dx = x - locx;
        int pct = RABBIT_SFP_CENTRAL_REGION[gy][dx+4];
        pieceCounts[b.owners[k]][b.pieces[k]] += pct*2;
      }
    }
	}
	else
  {
	  int leftBound = locx >= 4 ? locx - 4 : 0;
	  int rightBound = locx >= 4 ? 7 : locx + 4;

    for(int gy = 0; gy<6; (gy++,locy -= goallocinc))
    {
      for(int x = leftBound; x <= rightBound; x++)
      {
        int k = locy + x;
        if(b.owners[k] == NPLA)
          continue;
        int dx = x - locx;
        int pct = RABBIT_SFP_REGION[gy][dx+4];
        pieceCounts[b.owners[k]][b.pieces[k]] += pct*4;
      }
    }
  }

	pieceCounts[0][RAB] = pieceCounts[0][RAB] * 5/8;
  pieceCounts[1][RAB] = pieceCounts[1][RAB] * 5/8;

	int sfpValue = 0;
	int pieceIndex = 0;
	int pieceIndexPct = 95;
	int piecePla = ELE;
	int pieceOpp = ELE;
	while(true)
	{
		while(piecePla > EMP && pieceCounts[pla][piecePla] == 0)
		{piecePla--;}
		while(pieceOpp > EMP && pieceCounts[OPP(pla)][pieceOpp] == 0)
		{pieceOpp--;}

		if(piecePla <= EMP && pieceOpp <= EMP)
			break;

		int dec;
		int sign;
		if(piecePla > pieceOpp)
		{
			dec = pieceCounts[pla][piecePla];
			sign = +1;
			piecePla--;
		}
		else if(pieceOpp > piecePla)
		{
			dec = pieceCounts[OPP(pla)][pieceOpp];
			sign = -1;
			pieceOpp--;
		}
		else
		{
			dec = min(pieceCounts[pla][piecePla],pieceCounts[OPP(pla)][pieceOpp]);
			sign = 0;
			pieceCounts[pla][piecePla] -= dec;
			pieceCounts[OPP(pla)][pieceOpp] -= dec;
		}

		while(dec > 0)
		{
			int decdec = min(dec,pieceIndexPct);
			sfpValue += decdec * RABBIT_SFP_PIECE_VALUES[pieceIndex] * sign;
			pieceIndexPct -= decdec;
			dec -= decdec;
			if(pieceIndexPct == 0)
			{
				pieceIndexPct = 95;
				pieceIndex++;
			}
		}
	}

	return sfpValue/192;
}

/*
static void computeGoalDistances(const Board& b, const Bitmap pStrongerMaps[2][NUMTYPES], const int ufDist[64], int goalDist[2][64], int max)
{
  loc_t queueBuf[32];
  loc_t queueBuf2[32];
  int extraSteps[64];

  for(int i = 0; i<64; i++)
    goalDist[0][i] = max;
  for(int i = 0; i<64; i++)
    goalDist[1][i] = max;

  for(int pla = 0; pla <= 1; pla++)
  {
    for(int i = 0; i<64; i++)
      extraSteps[i] = -1;

    int goalLocInc = Board::GOALLOCINC[pla];

    int numInQueue = 0;
    loc_t* oldQueue = queueBuf;
    loc_t* newQueue = queueBuf2;

    int goalOffset = (pla == GOLD) ? 56 : 0;
    for(int x = 0; x<8; x++)
    {
      loc_t loc = goalOffset+x;
      pla_t owner = b.owners[loc];

      int extra = 0;
      if(owner == pla)
      {
        if(ufDist[loc] > 0 && !b.isOpen2(loc))
          extra = 3;// + 2*(!b.isOpenToPush(loc));
        //else if(ufDist[loc] == 0 && !b.isOpen(loc) && !b.isOpenToPush(loc))
        //  extra = 4;
        else
          extra = 1 + (!b.isOpenToStep(loc));
      }
      else if(owner == OPP(pla))
        extra = 2
        + (pStrongerMaps[OPP(pla)][b.pieces[loc]] & Board::RADIUS[1][loc] & ~b.frozenMap).isEmpty()
        + (pStrongerMaps[OPP(pla)][b.pieces[loc]] & (Board::RADIUS[1][loc] | (Board::RADIUS[2][loc] & ~b.frozenMap))).isEmpty()
        + !((CW1(loc) && b.owners[loc-1] == NPLA) || (CE1(loc) && b.owners[loc+1] == NPLA));

      goalDist[pla][loc] = 0;
      extraSteps[loc] = extra;
      oldQueue[numInQueue++] = loc;
    }

    for(int dist = 0; dist < max-1; dist++)
    {
      int newNumInQueue = 0;
      for(int i = 0; i<numInQueue; i++)
      {
        loc_t oldLoc = oldQueue[i];
        int oldLocDist = goalDist[pla][oldLoc] + extraSteps[oldLoc];
        if(goalDist[pla][oldLoc] + extraSteps[oldLoc] != dist)
        {
          if(oldLocDist < max-1) //TODO omit this check for speed?
            newQueue[newNumInQueue++] = oldLoc;
          continue;
        }

        for(int dir = 0; dir<4; dir++)
        {
          if(!Board::ADJOKAY[dir][oldLoc]) //TODO make player specific directions only including forward but not back? Or vice versa?
            continue;
          loc_t loc = oldLoc + Board::ADJOFFSETS[dir];
          if(extraSteps[loc] >= 0)
            continue;

          goalDist[pla][loc] = dist+1;

          if(dist < max-2)
          {
            pla_t owner = b.owners[loc];
            int extra = 0;
            if(owner == NPLA)
              extra = (!b.isTrapSafe2(pla,loc) || (b.wouldRabbitBeDomAt(pla,loc) && !b.isGuarded2(pla,loc))) ? 1 : 0;
            else if(owner == pla)
            {
              if(ufDist[loc] > 0 && !b.isOpen2(loc))
                extra = 3;// + 2*(!b.isOpenToPush(loc));
              //else if(ufDist[loc] == 0 && !b.isOpen(loc) && !b.isOpenToPush(loc))
              //  extra = 4;
              else
                extra = 1 + (!b.isOpenToStep(loc,loc+goalLocInc));
            }
            else
              extra = 2
              + (pStrongerMaps[OPP(pla)][b.pieces[loc]] & Board::RADIUS[1][loc] & ~b.frozenMap).isEmpty()
              + (pStrongerMaps[OPP(pla)][b.pieces[loc]] & (Board::RADIUS[1][loc] | (Board::RADIUS[2][loc] & ~b.frozenMap))).isEmpty()
              + (b.pieces[loc] != RAB);

            extraSteps[loc] = extra;
            newQueue[newNumInQueue++] = loc;
          }
        }
      }
      loc_t* temp = newQueue;
      newQueue = oldQueue;
      oldQueue = temp;
      numInQueue = newNumInQueue;
    }
  }

}*/

//Accounting for the fact that the edges help block rabbits
//TODO make multiplicative instead of additive?
//Account for piece attackdist when computing blocker?
static const int intrinsicBlockerVal[2][64] =	{
		{
				0,0,0,0,0,0,0,0,
				7,4,2,0,0,2,4,7,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
		},
		{
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				9,5,2,0,0,2,5,9,
				7,4,2,0,0,2,4,7,
				0,0,0,0,0,0,0,0,
		},
};

//[dy+3][dx+4]
static const int blockerContrib[11][9] =
{
		{0,0,0,1,1,1,0,0,0},
		{0,0,1,1,2,1,1,0,0},
		{0,1,1,2,4,2,1,1,0},
		{1,2,3,7,0,7,3,2,1},
		{1,2,4,7,10,7,4,2,1},
		{1,2,3,6,7,6,3,2,1},
		{1,2,3,5,7,5,3,2,1},
		{1,2,3,5,6,5,3,2,1},
		{1,2,3,5,6,5,3,2,1},
		{1,2,3,5,6,5,3,2,1},
		{1,2,3,5,6,5,3,2,1},
};
static const int rabBlockerContrib[11][9] =
{
		{0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0},
		{0,0,1,3,0,3,1,0,0},
		{0,0,1,2,6,2,1,0,0},
		{0,0,1,2,5,2,1,0,0},
		{0,0,1,2,4,2,1,0,0},
		{0,0,1,2,4,2,1,0,0},
		{0,0,1,2,4,2,1,0,0},
		{0,0,1,2,4,2,1,0,0},
		{0,0,1,2,4,2,1,0,0},
};

static const int RABBIT_SCORE_CONVOLUTION_LEN = 7;
static const double RABBIT_SCORE_CONVOLUTION[RABBIT_SCORE_CONVOLUTION_LEN] =
{0.07,0.13,0.19,0.22,0.19,0.13,0.07};

double Eval::getRabbitThreatScore(const Board& b, pla_t pla, const int ufDist[64], const int tc[2][4], const int influence[2][64],
 		bool print, const EvalParams& params)
{
	//Debugging output
	//int inflarr[64]; for(int i = 0; i<64; i++) inflarr[i] = 0;
	//int blockerarr[64]; for(int i = 0; i<64; i++) blockerarr[i] = 0;
	//int sfpgoalarr[64]; for(int i = 0; i<64; i++) sfpgoalarr[i] = 0;
  //double tscorearr[64]; for(int i = 0; i<64; i++) tscorearr[i] = 0;
  //double gscorearr[64]; for(int i = 0; i<64; i++) gscorearr[i] = 0;

	//double score = 0;
  const int poweredScoresLen = 8+RABBIT_SCORE_CONVOLUTION_LEN-1;
  double poweredScores[2][poweredScoresLen];
  for(int i = 0; i<poweredScoresLen; i++)
    poweredScores[0][i] = 0;
  for(int i = 0; i<poweredScoresLen; i++)
    poweredScores[1][i] = 0;

	//Compute sfp near the goal region for each column and each player
	//Actually column 0 and 7 are not used, so we do only 6 columns
	//and access by [pla][x-1]
	int sfpX[2][8];
	int sfpLeft = getRabbitSFP(b,SILV,1,false);
	int sfpCenter = getRabbitSFP(b,SILV,3,true);
  int sfpRight = getRabbitSFP(b,SILV,6,false);
  sfpX[SILV][0] = sfpLeft;
  sfpX[SILV][1] = sfpLeft;
  sfpX[SILV][2] = (sfpLeft * 5 + sfpCenter * 3)/8;
  sfpX[SILV][3] = (sfpLeft * 2 + sfpCenter * 6)/8;
  sfpX[SILV][4] = (sfpRight * 2 + sfpCenter * 6)/8;
  sfpX[SILV][5] = (sfpRight * 5 + sfpCenter * 3)/8;
  sfpX[SILV][6] = sfpRight;
  sfpX[SILV][7] = sfpRight;

  sfpLeft = getRabbitSFP(b,GOLD,1,false);
  sfpCenter = getRabbitSFP(b,GOLD,3,true);
  sfpRight = getRabbitSFP(b,GOLD,6,false);

  sfpX[GOLD][0] = sfpLeft;
  sfpX[GOLD][1] = sfpLeft;
  sfpX[GOLD][2] = (sfpLeft * 5 + sfpCenter * 3)/8;
  sfpX[GOLD][3] = (sfpLeft * 2 + sfpCenter * 6)/8;
  sfpX[GOLD][4] = (sfpRight * 2 + sfpCenter * 6)/8;
  sfpX[GOLD][5] = (sfpRight * 5 + sfpCenter * 3)/8;
  sfpX[GOLD][6] = sfpRight;
  sfpX[GOLD][7] = sfpRight;

	for(int loc = 0; loc < 64; loc++)
	{
		pla_t owner = b.owners[loc];
		if(b.pieces[loc] != RAB || Board::GOALYDIST[owner][loc] == 0)
			continue;

		int rx = loc % 8;
		int ry = loc / 8;
		int gy = owner == SILV ? -1 : 1;
		int gyd = Board::GOALYDIST[owner][loc];

		int xpos = rx >= 4 ? 7 - rx : rx;
		int ydist = Board::GOALYDIST[owner][loc];
		int rabtc =
				rx <= 2 ? tc[owner][Board::PLATRAPINDICES[OPP(owner)][0]] :
				rx >= 5 ? tc[owner][Board::PLATRAPINDICES[OPP(owner)][1]] :
				rx == 3 ? (tc[owner][Board::PLATRAPINDICES[OPP(owner)][0]]*2 + tc[owner][Board::PLATRAPINDICES[OPP(owner)][1]])/3 :
					        (tc[owner][Board::PLATRAPINDICES[OPP(owner)][1]]*2 + tc[owner][Board::PLATRAPINDICES[OPP(owner)][0]])/3;

		int tcIdx = (rabtc * 15)/50 + 30;
		if(tcIdx < 0) tcIdx = 0;
		if(tcIdx > 60) tcIdx = 60;

		int isFrozenRab = b.isFrozen(loc);
		int ufDistRab = ufDist[loc] >= 2 ? 2 : ufDist[loc];
		int frozenUFDistIdx = isFrozenRab * 3 + ufDistRab;

		double tcScore =
				params.get(EvalParams::RAB_YDIST_TC_VALUES,ydist) *
				params.get(EvalParams::RAB_XPOS_TC,xpos) *
        params.get(EvalParams::RAB_FROZEN_UFDIST,frozenUFDistIdx) *
				params.get(EvalParams::RAB_TC,tcIdx);

		//TODO add something for trap squares?
		//TODO decrease blocker a little when an in-front blocking piece is low attackdist?
		int blockedness = 0;
		int startY = (owner == SILV) ? min(ry+3,7) : max(ry-3,0);
		int endYPassed = (owner == SILV ? -1 : 8);
		int startX = max(rx-4,0);
		int endX = min(rx+4,7);

		//TODO is a straight loop over 0-63 faster?
		for(int y = startY; y != endYPassed; y+=gy)
		{
		  for(int x = startX; x <= endX; x++)
		  {
		    int loc = y*8+x;
	      if(b.owners[loc] != OPP(owner))
	        continue;
	      int dx = x - rx;
	      int dy = (y - ry)*gy;

	      if(b.pieces[loc] == RAB)
        {
	        blockedness += rabBlockerContrib[dy+3][dx+4];
	        if(dy == gyd) blockedness++;
        }
	      else
        {
          if(dy == gyd) blockedness += rabBlockerContrib[dy+3][dx+4] + 1;
          else          blockedness += blockerContrib[dy+3][dx+4];
        }
		  }
		}
		blockedness += intrinsicBlockerVal[owner][loc];

		//TODO weights distant heavy pieces too much.
		int sfp = sfpX[owner][rx];

		int goalLoc = Board::GOALY[owner]*8 + rx;
    int influenceSum = 0;
    int div = 5;
    influenceSum += 3*influence[owner][goalLoc] + 2*influence[owner][goalLoc-gy];
    if(rx > 0) { div += 5; influenceSum += 3*influence[owner][goalLoc-1] + 2*influence[owner][goalLoc-1-gy];}
    if(rx < 7) { div += 5; influenceSum += 3*influence[owner][goalLoc+1] + 2*influence[owner][goalLoc+1-gy];}
    int infl = influenceSum*2/div/3;
    infl = infl + 40;
    if(infl < 0) infl = 0;
    if(infl > 40) infl = 40;
    static const double YDIST_INFL_FACTOR[8] = {1.0,1.0,0.9,0.65,0.45,0.35,0.30,0.30};
    infl = (int)(infl * YDIST_INFL_FACTOR[ydist]);

		double goalScore =
        params.get(EvalParams::RAB_YDIST_VALUES,ydist) *
				params.get(EvalParams::RAB_XPOS,xpos) *
        params.get(EvalParams::RAB_FROZEN_UFDIST,frozenUFDistIdx) *
        params.get(EvalParams::RAB_INFLFRONT,infl) *
				params.get(EvalParams::RAB_BLOCKER,max(min(blockedness,60),0)) *
				params.get(EvalParams::RAB_SFPGOAL,max(min(sfp+20,40),0));

    double rabScore = tcScore + goalScore;

    //score += owner == pla ? rabScore : -rabScore;
    for(int i = 0; i<RABBIT_SCORE_CONVOLUTION_LEN; i++)
      poweredScores[owner][rx+i] += (rabScore * rabScore) * (RABBIT_SCORE_CONVOLUTION[i]*RABBIT_SCORE_CONVOLUTION[i]);

		//Debugging output
		//inflarr[loc] = infl;
		//blockerarr[loc] = blockedness;
		//sfpgoalarr[loc] = sfp;
    //tscorearr[loc] = tcScore;
    //gscorearr[loc] = goalScore;
	}
	if(print)
	{
		//cout << "infl" << endl; cout << ArimaaIO::write64(inflarr,"%3d") << endl;
		//cout << "blocker" << endl; cout << ArimaaIO::write64(blockerarr,"%4d") << endl;
	  //cout << "sfpgoal" << endl; cout << ArimaaIO::write64(sfpgoalarr,"%5d") << endl;
	  //cout << "tscore" << endl; cout << ArimaaIO::write64(tscorearr,"%5.0f") << endl;
    //cout << "gscore" << endl; cout << ArimaaIO::write64(gscorearr,"%5.0f") << endl;
	}

  double score = 0;
  for(int i = 0; i<poweredScoresLen; i++)
    score += sqrt(poweredScores[pla][i]) - sqrt(poweredScores[OPP(pla)][i]);

  return score;
}







