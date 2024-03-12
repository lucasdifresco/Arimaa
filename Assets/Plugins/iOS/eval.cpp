
/*
 * eval.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <cstdio>
#include <cmath>
#include "global.h"
#include "bitmap.h"
#include "board.h"
#include "ufdist.h"
#include "threats.h"
#include "strats.h"
#include "evalparams.h"
#include "eval.h"
#include "searchparams.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;

eval_t Eval::evaluate(Board& b, eval_t alpha, eval_t beta, bool print)
{
	pla_t pla = b.player;
	pla_t opp = OPP(pla);

  //Initialization------------------------------------------------------------------------
  int pStronger[2][NUMTYPES];
  Bitmap pStrongerMaps[2][NUMTYPES];
  eval_t pValues[2][NUMTYPES];
  int ufDist[64];
	b.initializeStronger(pStronger);
	b.initializeStrongerMaps(pStrongerMaps);
	initializeValues(b,pValues);
	UFDist::get(b,ufDist);

  //Material, piece square---------------------------------
  eval_t materialScore = getMaterialScore(b, pla);
	eval_t psScore = getPieceSquareScore(b,pStronger[pla],pStronger[opp],false);
  eval_t alignmentScore = getPieceAlignmentScore(b,pla);

	//Traps--------------------------------------------------
  int tc[2][4];
  getBasicTrapControls(b,pStronger,pStrongerMaps,ufDist,tc);

	eval_t tDefScore = getTrapDefenderScore(b);
  eval_t tcScore = 0;
  for(int i = 0; i<4; i++)
  	tcScore += getTrapControlScore(pla,i,tc[pla][i]);

  //Numsteps bonus----------------------------------------
  int numSteps = 4-b.step;
  eval_t nsScore = SearchParams::STEPS_LEFT_BONUS[numSteps];

  //Piece threatening---------------------------------------------------------------------
  eval_t trapThreats[2][4];
  eval_t pieceThreats[64];
  getBasicPieceThreats(b,pStronger,pStrongerMaps,pValues,ufDist,tc,trapThreats,pieceThreats);
  getBonusPieceThreats(b,pStronger,pStrongerMaps,pValues,ufDist,tc,trapThreats,pieceThreats);

  eval_t threatScore = 0;
  for(int i = 0; i<4; i++)
  {
  	threatScore -= trapThreats[pla][i];
  	threatScore += trapThreats[opp][i];
  }

  //Frames, Hostages, Blockades-------------------------------------------------------------
  int numPStrats[2];
  Strat pStrats[2][numStratsMax];
  eval_t stratScore[2];
  getStrats(b,NPLA,pValues,pStronger,pStrongerMaps,ufDist,tc,
  		pieceThreats,numPStrats,pStrats,stratScore,false);

  //Captures------------------------------------------------------------------------------
  eval_t bestCapRaw;
  loc_t bestCapLoc;
  eval_t capScore = evalCaps(b,pla,numSteps,pValues,pStrongerMaps,ufDist,
  		pieceThreats,numPStrats,pStrats,bestCapRaw,bestCapLoc);

  //Goal threatening---------------------------------------------------------------------
  int influence[2][64];
  getInfluence(b, pStronger, influence);

  eval_t rabbitPlaScore = getRabbitThreats(b, pla, ufDist, tc, influence);
  eval_t rabbitOppScore = getRabbitThreats(b, opp, ufDist, tc, influence);

  //Add it all up!---------------------------------------------------------------------
	int finalScore = materialScore + psScore + nsScore + alignmentScore + tDefScore + tcScore +
			threatScore + stratScore[pla] - stratScore[opp] +
			capScore + rabbitPlaScore - rabbitOppScore;


	ARIMAADEBUG(if(print)
	{
		cout << "---Eval--------------------------" << endl;
		cout << b;
		cout << "Material: " << materialScore << " " << writeMaterial(b.pieceCounts) << endl;
		cout << "Piece-Square: " << psScore << endl;
    cout << "AlignmentScore: " << alignmentScore << endl;
  	cout << "TDefScore: " << tDefScore << endl;
    for(int i = 0; i<4; i++)
    	printf("Trap control %+4d Score %+4d\n",tc[pla][i],getTrapControlScore(pla,i,tc[pla][i]));
  	cout << "TCScore: " << tcScore << endl;
  	cout << "NSScore: " << nsScore << endl;
  	cout << ArimaaIO::write64(pieceThreats,"%+5d ") << endl;
		cout << "ThreatScore: " << threatScore << endl;
  	cout << "StratScorePla: " << stratScore[pla] << endl;
  	cout << "StratScoreOpp: " << stratScore[opp] << endl;
  	if(bestCapLoc != ERRORSQUARE)
  		cout << "Best cap " << (int)bestCapLoc << " Raw " << bestCapRaw << " Net " << capScore << endl;
  	else
  		cout << "No cap" << endl;
  	cout << "CapScore: " << capScore << endl;
  	cout << "RabbitPlaScore: " << rabbitPlaScore << endl;
  	cout << "RabbitOppScore: " << rabbitOppScore << endl;
  	cout << endl;
  	cout << "FinalScore: " << finalScore << endl;
  	cout << "---------------------------------" << endl;
	})

	return finalScore;
}

eval_t Eval::evaluateWithParams(Board& b, pla_t mainPla, eval_t alpha, eval_t beta, const EvalParams& params, bool print)
{
	pla_t pla = b.player;
	pla_t opp = OPP(pla);

  //Initialization------------------------------------------------------------------------
  int pStronger[2][NUMTYPES];
  Bitmap pStrongerMaps[2][NUMTYPES];
  eval_t pValues[2][NUMTYPES];
  int ufDist[64];
  b.initializeStronger(pStronger);
  b.initializeStrongerMaps(pStrongerMaps);
  initializeValues(b,pValues);
	UFDist::get(b,ufDist);

  //Material, piece square---------------------------------
  double materialScore = getMaterialScore(b, pla);
  eval_t alignmentScore = getPieceAlignmentScore(b,pla);
	double psScore = getPieceSquareScore(b,pStronger[pla],pStronger[opp],false)
			* params.get(EvalParams::PIECE_SQUARE_SCALE);

  double recklessScore = 0;
  double rsScale = params.get(EvalParams::RECKLESS_ADVANCE_SCALE);
  if(rsScale > 0.001)
  {
    int score = 0;
    for(int i = 0; i<64; i++)
    {
      if(b.owners[i] != mainPla)
        continue;
      int bonus = 0;
      if(b.owners[i] == pla)
        bonus = Board::ADVANCEMENT[pla][i] * (b.pieces[i] + 8);
      else
        bonus = -(Board::ADVANCEMENT[opp][i] * (b.pieces[i] + 8));
      if(b.pieces[i] == RAB)
        bonus = bonus * 3 / 5;
      if(b.pieces[i] == ELE)
        bonus = bonus * 2;
      score += bonus;
    }
    recklessScore = rsScale * score;
  }
      
	//Traps--------------------------------------------------
  int tc[2][4];
  getBasicTrapControls(b,pStronger,pStrongerMaps,ufDist,tc);

	double tDefScore = getTrapDefenderScore(b) * params.get(EvalParams::TRAPDEF_SCORE_SCALE);
  double tcScore = 0;
  for(int i = 0; i<4; i++)
  	tcScore += getTrapControlScore(pla,i,tc[pla][i]) * params.get(EvalParams::TC_SCORE_SCALE);

  //Numsteps bonus----------------------------------------
  int numSteps = 4-b.step;
  eval_t nsScore = SearchParams::STEPS_LEFT_BONUS[numSteps];

  //Piece threatening---------------------------------------------------------------------
  eval_t trapThreats[2][4];
  eval_t pieceThreats[64];
  getBasicPieceThreats(b,pStronger,pStrongerMaps,pValues,ufDist,tc,trapThreats,pieceThreats);
  getBonusPieceThreats(b,pStronger,pStrongerMaps,pValues,ufDist,tc,trapThreats,pieceThreats);

  eval_t thrScore = 0;
  for(int i = 0; i<4; i++)
  {
  	thrScore -= trapThreats[pla][i];
  	thrScore += trapThreats[opp][i];
  }
  double threatScore = thrScore * params.get(EvalParams::THREAT_SCORE_SCALE);

  //Frames, Hostages, Blockades-------------------------------------------------------------
  int numPStrats[2];
  Strat pStrats[2][numStratsMax];
  eval_t stratScore[2];
  getStrats(b,mainPla,pValues,pStronger,pStrongerMaps,ufDist,tc,
  		pieceThreats,numPStrats,pStrats,stratScore,print);
  double stratScorePla = stratScore[pla] * params.get(EvalParams::STRAT_SCORE_SCALE);
  double stratScoreOpp = stratScore[opp] * params.get(EvalParams::STRAT_SCORE_SCALE);

  //Camel Advancement------------------------------------------------------------------
  int psheriffScore = getSheriffAdvancementThreat(b,pla,ufDist,tc,false);
  int osheriffScore = getSheriffAdvancementThreat(b,opp,ufDist,tc,false);

  //Captures------------------------------------------------------------------------------
  eval_t bestCapRaw;
  loc_t bestCapLoc;
  double capScore = evalCaps(b,pla,numSteps,pValues,pStrongerMaps,ufDist,
  		pieceThreats,numPStrats,pStrats,bestCapRaw,bestCapLoc) * params.get(EvalParams::CAP_SCORE_SCALE);

  //Goal threatening---------------------------------------------------------------------
  int influence[2][64];
  getInfluence(b, pStronger, influence);

  //double rabbitPlaScore = getRabbitThreats(b, pla, ufDist, tc, influence) * params.get(EvalParams::RABBIT_SCORE_SCALE);
  //double rabbitOppScore = getRabbitThreats(b, opp, ufDist, tc, influence) * params.get(EvalParams::RABBIT_SCORE_SCALE);

  //TODO make this take into account whose turn it is, and also bonus threats far away from each other and
  //penalize threats close to each other
  double newRabbitScore = getRabbitThreatScore(b, pla, ufDist, tc, influence, print, params);

  //Add it all up!---------------------------------------------------------------------
	double finalScore = materialScore + psScore + nsScore + alignmentScore + tDefScore + tcScore +
	    psheriffScore - osheriffScore +
			threatScore + stratScorePla - stratScoreOpp +
			capScore + newRabbitScore + recklessScore;


	ARIMAADEBUG(if(print)
	{
		cout << "---Eval--------------------------" << endl;
		cout << b;
		cout << "Material: " << materialScore << " " << writeMaterial(b.pieceCounts) << endl;
		cout << "Piece-Square: " << psScore << endl;
    cout << "AlignmentScore: " << alignmentScore << endl;
  	cout << "tDefScore: " << tDefScore << endl;
    for(int i = 0; i<4; i++)
    	printf("Trap control %+4d Score %+4d\n",tc[pla][i],getTrapControlScore(pla,i,tc[pla][i]));
  	cout << "tcScore: " << tcScore << endl;
  	cout << "nsScore: " << nsScore << endl;
  	cout << ArimaaIO::write64(pieceThreats,"%+5d ") << endl;
		cout << "threatScore: " << threatScore << endl;
    cout << "sheriffThreatPla: " << psheriffScore << endl;
    cout << "sheriffThreatOpp: " << osheriffScore << endl;
  	cout << "stratScorePla: " << stratScore[pla] << endl;
  	cout << "stratScoreOpp: " << stratScore[opp] << endl;
  	if(bestCapLoc != ERRORSQUARE)
  		cout << "Best cap " << (int)bestCapLoc << " Raw " << bestCapRaw << " Net " << capScore << endl;
  	else
  		cout << "No cap" << endl;
  	cout << "capScore: " << capScore << endl;
  	//cout << "rabbitPlaScore: " << rabbitPlaScore << endl;
  	//cout << "rabbitOppScore: " << rabbitOppScore << endl;
    cout << "rabbitScore: " << newRabbitScore << endl;
    cout << endl;
  	cout << "finalScore: " << finalScore << endl;
  	cout << "---------------------------------" << endl;
	})

	return (eval_t)finalScore;
}



static const double INFLUENCE_SPREAD = 0.16;
static const int INFLUENCE_REPS = 4;
static const double INFLUENCE_NORM = 100.0/0.12/80.0;
static const double INFLUENCE_NUMSTRONGER[9] =
{75,55,50,45,40,35,30,25,15};
//100 units of influence is an elephant there
void Eval::getInfluence(const Board& b, const int pStronger[2][NUMTYPES], int influence[2][64])
{
	double infb1[64];
	double infb2[64];
	double* inf = infb1;
	double* other = infb2;
	for(int i = 0; i<64; i++)
	{
		if(b.owners[i] == GOLD)
			inf[i] = INFLUENCE_NUMSTRONGER[pStronger[GOLD][b.pieces[i]]];
		else if(b.owners[i] == SILV)
			inf[i] = -INFLUENCE_NUMSTRONGER[pStronger[SILV][b.pieces[i]]];
		else
			inf[i] = 0;
	}

	for(int reps = 0; reps < INFLUENCE_REPS; reps++)
	{
		other[0] = inf[0] * (1.0 - 4*INFLUENCE_SPREAD) + (inf[8]+inf[1]) * INFLUENCE_SPREAD;
		for(int x = 1; x < 7; x++)
		{
			other[x] = inf[x] * (1.0 - 4*INFLUENCE_SPREAD) + (inf[x+8]+(inf[x-1]+inf[x+1])) * INFLUENCE_SPREAD;
		}
		other[7] = inf[7] * (1.0 - 4*INFLUENCE_SPREAD) + (inf[15]+inf[6]) * INFLUENCE_SPREAD;

		for(int y = 1; y < 7; y++)
		{
			int i0 = y*8;
			other[i0] = inf[i0] * (1.0 - 4*INFLUENCE_SPREAD) + ((inf[i0-8]+inf[i0+8])+inf[i0+1]) * INFLUENCE_SPREAD;
			for(int x = 1; x < 7; x++)
			{
				int i = x+y*8;
				other[i] = inf[i] * (1.0 - 4*INFLUENCE_SPREAD) + ((inf[i-8]+inf[i+8])+(inf[i-1]+inf[i+1])) * INFLUENCE_SPREAD;
			}
			int i7 = y*8+7;
			other[i7] = inf[i7] * (1.0 - 4*INFLUENCE_SPREAD) + ((inf[i7-8]+inf[i7+8])+inf[i7-1]) * INFLUENCE_SPREAD;
		}

		other[56] = inf[56] * (1.0 - 4*INFLUENCE_SPREAD) + (inf[48]+inf[57]) * INFLUENCE_SPREAD;
		for(int x = 1; x < 7; x++)
		{
			int i = x+56;
			other[i] = inf[i] * (1.0 - 4*INFLUENCE_SPREAD) + (inf[i-8]+(inf[i-1]+inf[i+1])) * INFLUENCE_SPREAD;
		}
		other[63] = inf[63] * (1.0 - 4*INFLUENCE_SPREAD) + (inf[55]+inf[62]) * INFLUENCE_SPREAD;

		double* temp = other;
		other = inf;
		inf = temp;
	}
	for(int i = 0; i<64; i++)
	{
		influence[GOLD][i] = (int)(INFLUENCE_NORM * inf[i]);
		influence[SILV][i] = -influence[GOLD][i];
	}

}
