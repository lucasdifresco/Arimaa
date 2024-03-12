
/*
 * boardhistory.h
 * Author: davidwu
 *
 * Tracks the history of a board over the course of a game.
 */

#ifndef BOARDHISTORY_H
#define BOARDHISTORY_H

#include <vector>
#include "board.h"
#include "gamerecord.h"

using namespace std;

class BoardHistory
{
  public:

  //Note that if the BoardHistory was initialized on step != 0, then we could have:
  //minStepNumber not a multiple of 4,
  //minTurnNumber*4 not a valid step number.
  //turnBoard[minTurnNumber] has step != 0
  //etc.

  int minTurnNumber;			    //Starting board's turn num.
  int maxTurnNumber;          //Current board's turn num
  //int minStepNumber;			    //Starting board's step num.
  //int maxStepNumber;          //Current board's step num

  vector<Board> turnBoard;       //The board at the start of the turn [min,max]
  vector<hash_t> turnPosHash;    //The position hash at the start of the turn [min,max]
  vector<hash_t> turnSitHash;    //The situaiton hash at the start of the turn [min,max]
  vector<move_t> turnMove;       //The full move made (possibly so far) this turn [min,max]
  vector<int> turnPieceCount[2]; //The number of pieces of each player at the start of this turn [min,max]

  //vector<hash_t> stepPosHash;  //The position hash on this step [min,max] (only on steps where stepMove exists, and current)
  //vector<hash_t> stepSitHash;  //The situation hash on this step [min,max] (only on steps where stepMove exists, and current)
  //vector<move_t> stepMove;     //The full move made beginning on this step [min,max-K] (exists only on steps where a move made)
  //vector<step_t> stepStep;     //The step made on this step [min,max-1]

  //Returns the step number of the start of this turn
  //static inline int getStepNumber(int turnNumber)
  //{return turnNumber*4;}

  //Returns the turn number on which this step occurs
  //static inline int getTurnNumber(int stepNumber)
  //{return stepNumber/4;}

  BoardHistory();
  BoardHistory(const Board& b);
  BoardHistory(const Board& b, const vector<move_t>& moves); //Creates a BoardHistory with all the moves played out and recorded
  BoardHistory(const GameRecord& record); //Creates a BoardHistory with all the moves played out and recorded
  ~BoardHistory();

  //Indicate that move m was made, resulting in board b, and the step number was lastStep prior to m
  //Invalidates all history that occurs in any turns occuring after the turnNumber of b, and appends the results of m to the current
  //turnNumber.
  //At all times, this will result in the history matching the board up to the new position after the move.
  void reportMove(const Board& b, move_t m, int lastStep);

  //Indicate that move m was the full move made on the given old turn number
  //Invalidates all history that occurs in any turns occuring after the old turn number + 1
  void reportMove(int oldTurnNumber, move_t m);

  //Resets to the given board as the starting position, with the minimum turn and step numbers being given by the
  //board's turnNumber field
  void reset(const Board& b);

  //Output the sequence of positions of the game to a file.
  static void outputPositions(const char* filename, const BoardHistory& hist);

  //Output the moves of the game to a file.
  static void outputMoves(const char* filename, const BoardHistory& hist);

  //Returns true if the current situation is the third occurrence in the history
  //Always false when steps have been made this turn.
  static bool isThirdRepetition(const Board& b, const BoardHistory& hist);

  friend ostream& operator<<(ostream& out, const BoardHistory& hist);

  private:
  void initMoves(const Board& b, const vector<move_t>& moves);
  void resizeIfTooSmall();
};



#endif
