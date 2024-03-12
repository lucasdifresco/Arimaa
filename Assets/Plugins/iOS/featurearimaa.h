
/*
 * featurearimaa.h
 * Author: davidwu
 */

#ifndef FEATUREARIMAA_H
#define FEATUREARIMAA_H

#include "board.h"
#include "boardhistory.h"
#include "feature.h"

struct FeaturePosData; //In featuremove.h
typedef void (*GetFeaturesFunc)(const Board& b, const FeaturePosData&, pla_t, move_t, const BoardHistory&, void (*handleFeature)(findex_t,void*), void*);
typedef void (*GetPosDataFunc)(const Board& b, const BoardHistory& hist, pla_t pla, FeaturePosData& data);

struct ArimaaFeatureSet
{
	const FeatureSet* fset;
	GetFeaturesFunc getFeaturesFunc;
	GetPosDataFunc getPosDataFunc;

	ArimaaFeatureSet();
	ArimaaFeatureSet(const FeatureSet* fset, GetFeaturesFunc getFeaturesFunc, GetPosDataFunc getPosDataFunc);

  double getFeatureSum(const Board& b, const FeaturePosData& data,
  		pla_t pla, move_t move, const BoardHistory& hist, const vector<double>& featureWeights) const;

  vector<findex_t> getFeatures(const Board& b, const FeaturePosData& data,
  		pla_t pla, move_t move, const BoardHistory& hist) const;

  void getPosData(const Board& b, const BoardHistory& hist, pla_t pla, FeaturePosData& data) const;
};

namespace ArimaaFeature
{
	//PIECE INDEXING---------------------------------------------------------
	//Get an index corresponding to the strength/type of the piece, 0 to 7
	int getPieceIndexApprox(pla_t owner, piece_t piece, const int pStronger[2][NUMTYPES]);
	//Get an index corresponding to the strength/type of the piece, 0 to 11
	int getPieceIndex(const Board& b, pla_t owner, piece_t piece, const int pStronger[2][NUMTYPES]);


	//TRAPSTATE---------------------------------------------------------------
	const int TRAPSTATE_0 = 0;
	const int TRAPSTATE_1 = 1;
	const int TRAPSTATE_1E = 2;
	const int TRAPSTATE_2 = 3;
	const int TRAPSTATE_2E = 4;
	const int TRAPSTATE_3 = 5;
	const int TRAPSTATE_3E = 6;
	const int TRAPSTATE_4 = 7;

	int getTrapState(const Board& b, pla_t pla, loc_t kt);

	//PHALANXES---------------------------------------------------------------
	//Is there a phalanx by pla against oloc at the adjacent location adj, using a single piece?
	bool isSinglePhalanx(const Board& b, pla_t pla, loc_t oloc, loc_t adj);
	//Is there a phalanx by pla against oloc at the adjacent location adj, using a group of pieces?
	bool isMultiPhalanx(const Board& b, pla_t pla, loc_t oloc, loc_t adj);


}

#endif
