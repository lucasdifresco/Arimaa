
/*
 * featuremove.h
 * Author: davidwu
 */

#ifndef FEATUREMOVE_H_
#define FEATUREMOVE_H_

#include <vector>
#include <string>
#include "bitmap.h"
#include "board.h"
#include "boardhistory.h"
#include "threats.h"
#include "feature.h"
#include "featurearimaa.h"

struct FeaturePosData;

namespace MoveFeature
{
  extern bool IS_INITIALIZED;

  void initFeatureSet();

  const FeatureSet& getFeatureSet();

  ArimaaFeatureSet getArimaaFeatureSet();

  void getPosData(const Board& b, const BoardHistory& hist, pla_t pla, FeaturePosData& data);

  void getFeatures(const Board& b, const FeaturePosData& data,
  		pla_t pla, move_t move, const BoardHistory& hist,
  		void (*handleFeature)(findex_t,void*), void* handleFeatureData);

}

namespace MoveFeatureLite
{
  extern bool IS_INITIALIZED;

  void initFeatureSet();

  const FeatureSet& getFeatureSet();

  ArimaaFeatureSet getArimaaFeatureSetSrcDestOnly();
  ArimaaFeatureSet getArimaaFeatureSet();
  ArimaaFeatureSet getArimaaFeatureSetFullMove();

  void getPosDataSrcDestOnly(const Board& constB, const BoardHistory& hist, pla_t pla, FeaturePosData& dataBuf);
  void getPosData(const Board& b, const BoardHistory& hist, pla_t pla, FeaturePosData& data);
  void getPosDataFullMove(const Board& b, const BoardHistory& hist, pla_t pla, FeaturePosData& data);

  void getFeatures(const Board& b, const FeaturePosData& data,
  		pla_t pla, move_t move, const BoardHistory& hist,
  		void (*handleFeature)(findex_t,void*), void* handleFeatureData);

}

struct MoveFeaturePosData
{
  //"Construct" using a FeaturePosData - takes the bytes and uses them as a MoveFeaturePosData
  static MoveFeaturePosData& convert(FeaturePosData& data);
  static const MoveFeaturePosData& convert(const FeaturePosData& data);

	int ufDist[64];
  CapThreat oppCapThreats[Threats::maxCapsPerPla];
  int numOppCapThreats;

  Bitmap plaCapMap;
  Bitmap oppCapMap;

  int plaGoalDist;
  int oppGoalDist;

  int pStronger[2][NUMTYPES];
  int pieceIndex[2][NUMTYPES];

  int influence[2][64]; //TODO flatten this

  int oldTrapStates[2][4];

  int numLastPushed;
  loc_t lastPushed[2];
  int lastMoveSum[64];
  int lastLastMoveSum[64];

  bool oppHoldsFrameAtTrap[4];
  bool oppFrameIsPartial[4];
  bool oppFrameIsEleEle[4];

  //Disable regular constructor
  private:
  MoveFeaturePosData();
};

struct MoveFeatureLitePosData
{
  //"Construct" using a FeaturePosData - takes the bytes and uses them as a MoveFeatureLitePosData
  static MoveFeatureLitePosData& convert(FeaturePosData& data);
  static const MoveFeatureLitePosData& convert(const FeaturePosData& data);

	//Modes
	bool srcDestOnly;   //Only do src and dest features?
  bool useDependency; //Use the dependency features?

  int pStronger[2][NUMTYPES];
  int pieceIndex[2][NUMTYPES];

  //Array of the maximum strength opponent piece around each spot, EMP if none
  piece_t oppMaxStrAdj[64];

  //Array of the minimum strength opponent piece around each spot, ELE+1 if none
  piece_t oppMinStrAdj[64];

  //Own frozen pieces
  Bitmap plaFrozen;

  //Own pieces threatened by opponent with capture
  Bitmap plaCapThreatenedPlaTrap;
  Bitmap plaCapThreatenedOppTrap;

  //Threatened traps - can lose piece here
  bool isTrapCapThreatened[4];

  //Opponent trap state (0, 1, 1E, 2, 2E,...)
  int oppTrapState[4];

  //Squares where we are likely threatened with capture in the adjacent trap if the trap has 1 defenders
  //and we are leq the specified strength
  piece_t likelyDangerAdjTrapStr[64];

  //Squares where we are likely threatened with capture in the trap 2 steps away if the trap has 0 defenders
  //and we are leq the specified strength
  piece_t likelyDangerAdj2TrapStr[64];

  //Traps whose #defenders could be increased to defend against capture
  bool trapNeedsMoreDefs[4];

  //Number of defenders next to trap needed for us to be likely capsafe there
  int minDefendersToBeLikelySafe[4];

  //Squares where we will likely threaten an opp with capture if we put a piece geq the specified strength
  piece_t likelyThreatStr[64];

  //Squares where we will likely loosely threaten an opp with capture if we put a piece geq the specified strength
  piece_t likelyLooseThreatStr[64];

  //Squares where we will likely threaten an opp with capture if we pushpull an opp piece there
  Bitmap likelyThreatPushPullLocs;
  Bitmap likelyThreatPushPullLocsLoose;

  //Estimated steps to goal if a rabbit steps here.
  //int rabbitGoalDist[64];

  //Opponent pieces threatening ours with cap
  //Bitmap oppThreateners;
  //Squares where we would likely threaten goal if we moved a rabbit there
  //Bitmap likelyGoalThreat;

  //Disable regular constructor
  private:
  MoveFeatureLitePosData();
};

//TODO use a union
#define __FEATUREPOSDATA_STATIC_MAX__(a,b) ((a) > (b) ? (a) : (b))
struct FeaturePosData
{
	char bytes[__FEATUREPOSDATA_STATIC_MAX__(sizeof(MoveFeaturePosData), sizeof(MoveFeatureLitePosData))];
};


#endif
