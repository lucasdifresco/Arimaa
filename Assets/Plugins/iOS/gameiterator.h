/*
 * gameiterator.h
 * Author: davidwu
 */

#ifndef GAMEITERATOR_H_
#define GAMEITERATOR_H_

#include <vector>
#include "board.h"
#include "boardhistory.h"
#include "gamerecord.h"
#include "feature.h"
#include "featuremove.h"

using namespace std;

class GameIterator
{
  private:

  vector<GameRecord> games;
  int currentGameIdx;
  int currentGameNumMoves;
  int currentTurn;
  int nextStep;

  int moveType;

  vector<bool> turnFiltered;
  bool doFiltering;

  public:

  static const int STEP_MOVES = 0;
  static const int SIMPLE_CHAIN_MOVES = 1;
  static const int LOCAL_COMBO_MOVES = 2;
  static const int FULL_MOVES = 3;

  static double GAME_KEEP_PROP;
  static double MOVE_KEEP_PROP;
  static int MOVE_KEEP_THRESHOLD;

  static bool DO_PRINT;

  Board board;
  BoardHistory hist;
  move_t move;

  GameIterator(const char* filename, int moveType, bool doFiltering);
  ~GameIterator();

  bool next();

  void computeNextMoves(int& winningMoveIdx, vector<move_t>& moves);
  void computeMoveFeatures(ArimaaFeatureSet afset, int& winningTeam, vector<vector<findex_t> >& teams);

  //Return a vector indicating any moves that seem to be obviously bad play and should be filtered
  //hist should be the board history constructed directly from the game record
  static vector<bool> getFiltering(const GameRecord& game, const BoardHistory& hist);

  private:
  bool nextHelper();

};

#endif /* GAMEITERATOR_H_ */
