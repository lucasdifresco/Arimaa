
/*
 * featurearimaa.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <vector>
#include "bitmap.h"
#include "board.h"
#include "threats.h"
#include "feature.h"
#include "featurearimaa.h"

ArimaaFeatureSet::ArimaaFeatureSet()
:fset(NULL),getFeaturesFunc(NULL),getPosDataFunc(NULL)
{}

ArimaaFeatureSet::ArimaaFeatureSet(const FeatureSet* fset, GetFeaturesFunc getFeaturesFunc, GetPosDataFunc getPosDataFunc)
:fset(fset),getFeaturesFunc(getFeaturesFunc),getPosDataFunc(getPosDataFunc)
{}

struct FeatureAccum
{
	const vector<double>* featureWeights;
	double accum;

	FeatureAccum(const vector<double>& featureWeights)
	:featureWeights(&featureWeights),accum(0)
	{}
};
static void addFeatureValue(findex_t feature, void* a)
{
	FeatureAccum* accum = (FeatureAccum*)a;
	accum->accum += (*(accum->featureWeights))[feature];
};


double ArimaaFeatureSet::getFeatureSum(const Board& b, const FeaturePosData& data,
		pla_t pla, move_t move, const BoardHistory& hist, const vector<double>& featureWeights) const
{
	FeatureAccum accum(featureWeights);
	getFeaturesFunc(b,data,pla,move,hist,&addFeatureValue,&accum);
	return accum.accum;
}

static void addFeatureToVector(findex_t x, void* v)
{
	vector<findex_t>* vec = (vector<findex_t>*)v;
	vec->push_back(x);
}

vector<findex_t> ArimaaFeatureSet::getFeatures(const Board& b, const FeaturePosData& data,
		pla_t pla, move_t move, const BoardHistory& hist) const
{
	vector<findex_t> featuresVec;
	getFeaturesFunc(b,data,pla,move,hist,&addFeatureToVector,&featuresVec);
  return featuresVec;
}


void ArimaaFeatureSet::getPosData(const Board& b, const BoardHistory& hist, pla_t pla, FeaturePosData& data) const
{
	getPosDataFunc(b,hist,pla,data);
}







static const int PIECEINDEXTABLE[7][9] = {
{0,0,0,0,0,0,0,0,0},
{7,7,7,7,7,6,6,5,5},
{0,1,2,3,3,4,4,4,4},
{0,1,2,3,3,4,4,4,4},
{0,1,2,3,3,4,4,4,4},
{0,1,2,3,3,4,4,4,4},
{0,1,2,3,3,4,4,4,4},
};

int ArimaaFeature::getPieceIndexApprox(pla_t owner, piece_t piece, const int pStronger[2][NUMTYPES])
{
  int ns = pStronger[owner][piece];
  return PIECEINDEXTABLE[piece][ns];
}

static int RAB_INDEX_TABLE[25] =
{4,4,4,4,4,4,4,4,4,4,4,4,4,3,3,3,2,2,2,1,1,1,0,0,0};
int ArimaaFeature::getPieceIndex(const Board& b, pla_t owner, piece_t piece, const int pStronger[2][NUMTYPES])
{
	if(piece != RAB)
		return pStronger[owner][piece];
	else
		return 7+RAB_INDEX_TABLE[pStronger[owner][RAB]+b.pieceCounts[OPP(owner)][0]];
}

int ArimaaFeature::getTrapState(const Board& b, pla_t pla, loc_t kt)
{
  loc_t plaEleLoc = b.findElephant(pla);
  int defCount = b.trapGuardCounts[pla][Board::TRAPINDEX[kt]];
  switch(defCount)
  {
  case 0: return TRAPSTATE_0;
  case 1: return (plaEleLoc != ERRORSQUARE && Board::ISADJACENT[plaEleLoc][kt]) ? TRAPSTATE_1E : TRAPSTATE_1;
  case 2: return (plaEleLoc != ERRORSQUARE && Board::ISADJACENT[plaEleLoc][kt]) ? TRAPSTATE_2E : TRAPSTATE_2;
  case 3: return (plaEleLoc != ERRORSQUARE && Board::ISADJACENT[plaEleLoc][kt]) ? TRAPSTATE_3E : TRAPSTATE_3;
  case 4: return TRAPSTATE_4;
  default: DEBUGASSERT(false);
  }
  return 0;
}

bool ArimaaFeature::isSinglePhalanx(const Board& b, pla_t pla, loc_t oloc, loc_t adj)
{
	pla_t opp = OPP(pla);
	if(b.owners[oloc] != opp)
		return false;
	if(b.owners[adj] == pla && b.pieces[adj] >= b.pieces[oloc])
		return true;
	if(b.owners[adj] == opp && !b.isOpen(adj) && !b.isOpenToPush(adj))
		return true;
	return false;
}

bool ArimaaFeature::isMultiPhalanx(const Board& b, pla_t pla, loc_t oloc, loc_t adj)
{
	pla_t opp = OPP(pla);
	if(b.owners[oloc] != opp)
		return false;
	if(b.owners[adj] == NPLA || (b.owners[adj] == opp && b.isOpenToPush(adj)))
		return false;
	for(int dir = 0; dir < 4; dir++)
	{
		if(!Board::ADJOKAY[dir][adj])
			continue;
		loc_t adjadj = adj + Board::ADJOFFSETS[dir];
		if(b.owners[adjadj] == NPLA || (b.owners[adjadj] == opp && !b.isFrozen(adjadj) && b.isOpenToStep(adjadj)))
			return false;
	}
	return true;
}


