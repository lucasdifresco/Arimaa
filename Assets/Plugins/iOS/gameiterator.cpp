
/*
 * gameiterator.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include "global.h"
#include "rand.h"
#include "board.h"
#include "boardmovegen.h"
#include "boardtrees.h"
#include "gamerecord.h"
#include "gameiterator.h"
#include "feature.h"
#include "featuremove.h"
#include "learner.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;

double GameIterator::GAME_KEEP_PROP = 1.0;
double GameIterator::MOVE_KEEP_PROP = 1.0;
int GameIterator::MOVE_KEEP_THRESHOLD = 1;
bool GameIterator::DO_PRINT = false;

static int getNextMoves(Board& b, move_t recordedMove, int move_type, move_t& nextMove, move_t* nextMoves);
static move_t rearrangeMoveToJoinCombos(Board& b, move_t move);
static int genFullMoves(Board& b, move_t* moves, int maxSteps);
static int genFullMoveHelper(Board& b, move_t* moves, move_t moveSoFar, int stepIndex, int maxSteps);

static int moveBufSize = -1;
static move_t* moveBuf = NULL;

static void ensureMoveBuf(int moveType)
{
	int size = 0;
	if(moveType == GameIterator::FULL_MOVES)
		size = 4000000;
	else if(moveType == GameIterator::LOCAL_COMBO_MOVES)
		size = 400000;
	else if(moveType == GameIterator::SIMPLE_CHAIN_MOVES)
		size = 4096;
	else if(moveType == GameIterator::STEP_MOVES)
		size = 256;

	if(moveBufSize >= 0 && moveBufSize < size)
	{
		moveBufSize = -1;
		delete[] moveBuf;
		moveBuf = NULL;
	}

	if(moveBuf == NULL)
	{
		moveBufSize = size;
		moveBuf = new move_t[size];
	}
}

GameIterator::GameIterator(const char* filename, int mType, bool filter)
{
	games = ArimaaIO::readMovesFile(filename);
	currentGameIdx = -1;
	currentGameNumMoves = 0;
	currentTurn = 0;
	nextStep = 0;
	doFiltering = filter;

	moveType = mType;

  move = ERRORMOVE;

  //Convert all the moves to join the comboed steps together.
  for(int i = 0; i<(int)games.size(); i++)
  {
    Board b = games[i].board;
    for(int j = 0; j<(int)games[i].moves.size(); j++)
    {
      move_t m = rearrangeMoveToJoinCombos(b, games[i].moves[j]);
      games[i].moves[j] = m;
      bool suc = b.makeMoveLegal(m);
      if(!suc)
        cout << "MatchIterator: comboJoining error" << endl;
    }
  }
}

GameIterator::~GameIterator()
{

}

vector<bool> GameIterator::getFiltering(const GameRecord& game, const BoardHistory& hist)
{
	vector<bool> filter;
	int numMoves = game.moves.size();
	filter.resize(game.moves.size());

	DEBUGASSERT(hist.minTurnNumber == 0 && hist.maxTurnNumber == numMoves);

	//Filter opening sacrifices
	for(pla_t pla = 0; pla <= 1; pla++)
	{
		for(int i = (pla == GOLD ? 0 : 1); i<numMoves; i+=2)
		{
			DEBUGASSERT(game.moves[i] == hist.turnMove[i]);
			DEBUGASSERT(hist.turnBoard[i].player == pla);

			const Board& b = hist.turnBoard[i];
			loc_t src[8];
			loc_t dest[8];
			move_t move = game.moves[i];
			int num = b.getChanges(move,src,dest);

			bool hasSac = false;
			for(int j = 0; j<num; j++)
			{
				if(dest[j] == ERRORSQUARE && b.owners[src[j]] == pla)
				{hasSac = true; break;}
			}

			if(hasSac)
				filter[i] = true;
			else
				break;
		}
	}

	if(game.winner != NPLA)
	{
		//Filter concession sacrifices
		for(int i = numMoves - 5; i < numMoves; i++)
		{
			const Board& b = hist.turnBoard[i];
			if(b.player == OPP(game.winner))
			{
				loc_t src[8];
				loc_t dest[8];
				move_t move = game.moves[i];
				int num = b.getChanges(move,src,dest);

				bool hasSac = false;
				for(int j = 0; j<num; j++)
				{
					if(dest[j] == ERRORSQUARE && b.owners[src[j]] == b.player)
					{hasSac = true; break;}
				}

				if(hasSac)
					filter[i] = true;
			}
		}

		//Filter the last move by the losing player, since it's likely to be nonsense
		for(int i = numMoves - 1; i >= 0; i--)
		{
			const Board& b = hist.turnBoard[i];
			if(b.player == OPP(game.winner))
			{
				filter[i] = true;
				break;
			}
		}
	}

	//Filter missed win in 1 and deliberate game prolonging
	bool filterAllNonWin = false; //Do we filter all moves subsequently?
	int numPlaMissedWins[2] = {0,0};
	for(int i = 0; i < numMoves; i++)
	{
		const Board& b = hist.turnBoard[i];
		pla_t pla = b.player;
		Board copy = b;

		//Possible to win
		if(BoardTrees::goalDist(copy,pla,4) < 5 || BoardTrees::canElim(copy,pla,4))
		{
			bool suc = copy.makeMoveLegal(game.moves[i]);
			DEBUGASSERT(suc);
			pla_t winner = copy.getWinner();

			//We failed to win
			if(winner != pla)
			{
				numPlaMissedWins[pla]++;
				filter[i] = true;

				//If you miss a win in 1 more than a total of two times
				//then we assume you are deliberately prolonging the game.
				//So we filter everything except the move that finally wins
				if(numPlaMissedWins[pla] > 2)
					filterAllNonWin = true;
			}

			//Else we did win, don't filter
		}
		//Probably not possible to win (we aren't checking if forcing immo is possible)
		else
		{
			//Filter if desired
			if(filterAllNonWin)
			{
				//Check if someone won this move
				bool suc = copy.makeMoveLegal(game.moves[i]);
				DEBUGASSERT(suc);
				pla_t winner = copy.getWinner();

				//We didn't win
				if(winner != b.player)
					filter[i] = true;
			}
		}
	}

	return filter;
}

bool GameIterator::next()
{
	while(true)
	{
		bool suc = nextHelper();
		if(suc)
		{
			if(Rand::rand.nextDouble() < GAME_KEEP_PROP)
				return true;
			else
				continue;
		}
		return false;
	}
}

bool GameIterator::nextHelper()
{
	if(currentGameIdx >= (int)games.size())
		return false;

	if(nextStep != 0)
	{
		DEBUGASSERT(moveType == GameIterator::STEP_MOVES || moveType == GameIterator::SIMPLE_CHAIN_MOVES || moveType == GameIterator::LOCAL_COMBO_MOVES);
		DEBUGASSERT(currentTurn >= hist.minTurnNumber && currentTurn < hist.maxTurnNumber);
		DEBUGASSERT(hist.turnMove[currentTurn] != ERRORMOVE);

	  //Compute the data for this spot
		Board copy = hist.turnBoard[currentTurn];
		move_t premove = Board::getPreMove(hist.turnMove[currentTurn],nextStep);
		copy.makeMove(premove);

		board = copy;

	  move_t nextMove;
		ensureMoveBuf(moveType);
		getNextMoves(copy,hist.turnMove[currentTurn],moveType,nextMove,moveBuf);

		move = nextMove;

		bool suc = copy.makeMoveLegal(nextMove);
		if(!suc)
			Global::fatalError(string("Illegal move: ") + writeMove(nextMove) + writeBoard(board));
		nextStep = copy.step;

		return true;
	}

	currentTurn++;
	while(true)
	{
		if(currentTurn >= currentGameNumMoves)
		{
			currentGameIdx++;

			if(DO_PRINT)
				cout << "Iterating game " << currentGameIdx << endl;

			if(currentGameIdx >= (int)games.size())
			{
				board = Board();
				hist = BoardHistory();
				move = ERRORMOVE;
				return false;
			}

			currentGameNumMoves = games[currentGameIdx].moves.size();
			currentTurn = 0;
			hist = BoardHistory(games[currentGameIdx]);
			if(doFiltering)
				turnFiltered = getFiltering(games[currentGameIdx],hist);
			continue;
		}

		DEBUGASSERT(!(doFiltering && currentTurn >= (int)turnFiltered.size()));
		if(doFiltering && turnFiltered[currentTurn])
		{
			currentTurn++;
			continue;
		}

		DEBUGASSERT(currentTurn >= hist.minTurnNumber && currentTurn < hist.maxTurnNumber);
		DEBUGASSERT(hist.turnMove[currentTurn] != ERRORMOVE);

		if(moveType == GameIterator::FULL_MOVES)
		{
			board = hist.turnBoard[currentTurn];
			move = hist.turnMove[currentTurn];
			nextStep = 0;
		}
		else
		{
		  //Compute the data for this spot
			Board copy = hist.turnBoard[currentTurn];
			board = copy;

		  move_t nextMove;
			ensureMoveBuf(moveType);
			getNextMoves(copy,hist.turnMove[currentTurn],moveType,nextMove,moveBuf);

			move = nextMove;

			bool suc = copy.makeMoveLegal(nextMove);
			if(!suc)
				Global::fatalError(string("Illegal move: ") + writeMove(nextMove) + writeBoard(board));
			nextStep = copy.step;
		}

		return true;
	}
}

void GameIterator::computeNextMoves(int& winningMoveIdx, vector<move_t>& moves)
{
	Board copy = board;

  move_t nextMove;
	ensureMoveBuf(moveType);
	int nextMovesLen = getNextMoves(copy,move,moveType,nextMove,moveBuf);

	moves.clear();
	moves.reserve(nextMovesLen);
	for(int i = 0; i<nextMovesLen; i++)
	{
		moves.push_back(moveBuf[i]);
	}

  //Locate the winner
	winningMoveIdx = -1;
  for(int i = 0; i<nextMovesLen; i++)
  {
    if(moveBuf[i] == nextMove)
    {
    	winningMoveIdx = i;
      break;
    }
  }
  if(winningMoveIdx == -1)
  	Global::fatalError(string("computeMoveFeatures: winningMoveIdx not found!") + writeBoard(board) + writeMove(move));
}

void GameIterator::computeMoveFeatures(ArimaaFeatureSet afset, int& winningTeam, vector<vector<findex_t> >& teams)
{
  //Compute the data for this spot
	pla_t pla = board.player;
	Board copy = board;

  move_t nextMove;
	ensureMoveBuf(moveType);
	int nextMovesLen = getNextMoves(copy,move,moveType,nextMove,moveBuf);

  //Build all the feature teams!
  FeaturePosData data;
  afset.getPosData(copy,hist,pla,data);
  teams.clear();
  for(int i = 0; i<nextMovesLen; i++)
    teams.push_back(afset.getFeatures(copy,data,pla,moveBuf[i],hist));

  //Locate the winner
  winningTeam = -1;
  for(int i = 0; i<nextMovesLen; i++)
  {
    if(moveBuf[i] == nextMove)
    {
      winningTeam = i;
      break;
    }
  }
  if(winningTeam == -1)
  	Global::fatalError(string("computeMoveFeatures: winning team not found!") + writeBoard(board) + writeMove(move));
}

static int getNextMoves(Board& b, move_t recordedMove, int move_type, move_t& nextMove, move_t* nextMoves)
{
  int maxSteps = 4-b.step;

  //Generate possible moves!
  int num = 0;
  if(move_type == GameIterator::STEP_MOVES)
  {
    if(maxSteps >= 2)
      num += BoardMoveGen::genPushPulls(b,b.player,nextMoves+num);
    num += BoardMoveGen::genSteps(b,b.player,nextMoves+num);
    if(b.step > 0 && b.posCurrentHash != b.posStartHash)
      nextMoves[num++] = PASSMOVE;
  }
  else if(move_type == GameIterator::SIMPLE_CHAIN_MOVES)
  {
    num += BoardMoveGen::genSimpleChainMoves(b,b.player,maxSteps,nextMoves+num);
    if(b.step > 0 && b.posCurrentHash != b.posStartHash)
      nextMoves[num++] = PASSMOVE;
  }
  else if(move_type == GameIterator::LOCAL_COMBO_MOVES)
  {
    num += BoardMoveGen::genLocalComboMoves(b,b.player,maxSteps,nextMoves+num);
    if(b.step > 0 && b.posCurrentHash != b.posStartHash)
      nextMoves[num++] = PASSMOVE;
  }
  else if(move_type == GameIterator::FULL_MOVES)
  {
  	num += genFullMoves(b,nextMoves+num,maxSteps);
  	if(num > 2000000)
  		cout << "ArimaaFeature::genMoves .. fullMoves .. Num = " << num << endl;

  	//Find a move equivalent to this one and replace it
  	move_t postMove = Board::getPostMove(recordedMove,b.step);
  	Board temp = b;
    bool suc = temp.makeMoveLegal(postMove);
    if(!suc)
    {
      cout << "ArimaaFeature::genMoves .. fullMoves - illegal Move!" << endl;
      cout << b << endl;
      cout << writeMove(b,postMove);
    }
    hash_t postMoveHash = temp.sitCurrentHash;
    bool found = false;
    for(int i = 0; i<num; i++)
    {
    	temp = b;
      suc = temp.makeMoveLegal(nextMoves[i]);
      if(!suc)
      {
        cout << "ArimaaFeature::genMoves .. fullMoves - illegal Move!" << endl;
        cout << b << endl;
        cout << writeMove(b,nextMoves[i]);
      }
      hash_t thisMoveHash = temp.sitCurrentHash;
      if(thisMoveHash == postMoveHash)
      {
      	nextMoves[i] = postMove;
      	found = true;
      	break;
      }
    }
    if(!found)
    {
    	cout << "ArimaaFeature::genMoves .. fullMoves - move not found!" << endl;
    	cout << b << endl;
    	cout << writeMove(b,postMove) << endl;
    	nextMoves[num++] = postMove;
    }
  }
  else
  {
  	Global::fatalError("Unknown move type!");
  }

  //Locate the maximal prefix of the recorded move
  int bestLen = 0;
  move_t postMove = Board::getPostMove(recordedMove,b.step);
  nextMove = postMove;
  for(int i = 0; i<num; i++)
  {
    if(Board::isPrefix(nextMoves[i], postMove))
    {
      int len = Board::numStepsInMove(nextMoves[i]);
      if(len > bestLen)
      {
        bestLen = len;
        nextMove = nextMoves[i];
      }
    }
  }

  //Discard moves randomly
  if(GameIterator::MOVE_KEEP_PROP < 1.0 && num > GameIterator::MOVE_KEEP_THRESHOLD+1)
  {
  	//The -1 is because we are always keeping the next move - numToKeep does not include it.
    int numToKeep = (int)round(max(num-GameIterator::MOVE_KEEP_THRESHOLD-1,0) * GameIterator::MOVE_KEEP_PROP
    		                       + GameIterator::MOVE_KEEP_THRESHOLD);
    int newNum = 0;
    for(int i = 0; i<num; i++)
    {
      if(nextMoves[i] == nextMove)
        nextMoves[newNum++] = nextMoves[i];
      else if(Rand::rand.nextDouble() < (double)numToKeep/(num-i))
      {
        nextMoves[newNum++] = nextMoves[i];
        numToKeep--;
      }
    }
    num = newNum;
  }

  //Uniquify moves, taking care to retain the one selected
  if(move_type == GameIterator::SIMPLE_CHAIN_MOVES || move_type == GameIterator::LOCAL_COMBO_MOVES)
  {
    //Uniquify moves
    int newNum = 0;
    hash_t* posHashes = new hash_t[num];
    int* numSteps = new int[num];
    for(int i = 0; i<num; i++)
    {
      //Always keep the pass move
      if(nextMoves[i] == PASSMOVE)
      {
        posHashes[newNum] = posHashes[i];
        numSteps[newNum] = numSteps[i];
        nextMoves[newNum] = nextMoves[i];
        newNum++;
        continue;
      }

      //Try the move and see the resulting hash
      Board tempBoard = b;
      bool suc = tempBoard.makeMoveLegal(nextMoves[i]);
      if(!suc)
      {
        cout << "ArimaaFeature::genMoves - illegal Move!" << endl;
        cout << b << endl;
        cout << writeMove(b,nextMoves[i]);
      }

      //Record data about the move
      posHashes[i] = tempBoard.posCurrentHash;
      numSteps[i] = Board::numStepsInMove(nextMoves[i]);

      //Move changes nothing! - skip it unless it is the move we are going to make
      if(tempBoard.posCurrentHash == b.posCurrentHash && nextMoves[i] != nextMove)
        continue;

      //Check to see if this move is equivalent a previous move
      bool equivalent = false;
      for(int j = 0; j<newNum; j++)
      {
        //Equivalent move!
        if(posHashes[i] == posHashes[j] && nextMoves[i] != PASSMOVE)
        {
          //Keep the one with the smaller number of steps made, except that we always keep the move that is the nextMove
          if(nextMoves[i] == nextMove || (numSteps[i] < numSteps[j] && nextMoves[j] != nextMove))
          {
            posHashes[j] = posHashes[i];
            numSteps[j] = numSteps[i];
            nextMoves[j] = nextMoves[i];
          }
          equivalent = true;
          break;
        }
      }
      //Found no equivalents? Then we add it!
      if(!equivalent)
      {
        posHashes[newNum] = posHashes[i];
        numSteps[newNum] = numSteps[i];
        nextMoves[newNum] = nextMoves[i];
        newNum++;
      }
    }
    num = newNum;
	delete[] posHashes;
	delete[] numSteps;
  }


  return num;
}

static move_t rearrangeMoveToJoinCombos(Board& b, move_t move)
{
  int dependencyStrength[4][4];
  for(int i = 0; i<4; i++)
    for(int j = 0; j<4; j++)
      dependencyStrength[i][j] = 0;

  loc_t k0[4];
  loc_t k1[4];
  int num = Board::numStepsInMove(move);
  int actualStepNum = num;
  for(int i = 0; i<num; i++)
  {
    step_t step = Board::getStep(move,i);
    if(step == PASSSTEP)
    {
      actualStepNum--;
      break;
    }
    k0[i] = Board::K0INDEX[step];
    k1[i] = Board::K1INDEX[step];
  }

  for(int i = 0; i<actualStepNum; i++)
  {
    for(int j = 0; j<i; j++)
    {
      if(k0[i] == k1[j])
        dependencyStrength[i][j] = 1000;
      else if(k1[i] == k0[j])
        dependencyStrength[i][j] = 150;
      else
        dependencyStrength[i][j] = 30-Board::MANHATTANDIST[k1[i]][k1[j]];
      dependencyStrength[j][i] = dependencyStrength[i][j];
    }
  }

  Board actualBoard = b;
  actualBoard.makeMove(move);

  move_t bestMove = ERRORMOVE;
  int bestTension = 0x7FFFFFFF;
  int permutations[4] = {0,1,2,3};
  do
  {
    move_t permutedMove = Board::getMove(
        Board::getStep(move,permutations[0]),
        Board::getStep(move,permutations[1]),
        Board::getStep(move,permutations[2]),
        Board::getStep(move,permutations[3]));

    int tension = 0;
    for(int i = 0; i<actualStepNum; i++)
    {
      for(int j = 0; j<actualStepNum; j++)
      {
        if(i == j) continue;
        tension += dependencyStrength[permutations[i]][permutations[j]] * abs(i-j);
      }
    }

    if(tension < bestTension)
    {
      Board copy = b;
      bool suc = copy.makeMoveLegal(permutedMove);
      if(suc)
      {
        if(copy.sitCurrentHash == actualBoard.sitCurrentHash)
        {
          bestTension = tension;
          bestMove = permutedMove;
        }
      }
    }

  } while(next_permutation(permutations,permutations+actualStepNum));

  if(bestMove == ERRORMOVE)
  {
    cout << "ArimaaFeature::rearrangeMoveToJoinCombos - Successful permutation not found!" << endl;
    bestMove = move;
  }
  return bestMove;
}

static const hash_t MV_FULL_HASH_SIZE = 1ULL<<21;
static const hash_t MV_FULL_HASH_MASK = (1ULL<<21)-1ULL;
static hash_t* mvFullHashKeys;
static bool* mvFullHashExists;

static int genFullMoveHelper(Board& b, move_t* moves, move_t moveSoFar, int stepIndex, int maxSteps)
{
	hash_t hash = b.sitCurrentHash;
	int hashIndex = (int)(hash & MV_FULL_HASH_MASK);
	int hashIndex2 = (int)((hash >> 21) & MV_FULL_HASH_MASK);
	int hashIndex3 = (int)((hash >> 42) & MV_FULL_HASH_MASK);
	if((mvFullHashExists[hashIndex] && mvFullHashKeys[hashIndex] == hash) ||
		 (mvFullHashExists[hashIndex2] && mvFullHashKeys[hashIndex2] == hash) ||
		 (mvFullHashExists[hashIndex3] && mvFullHashKeys[hashIndex3] == hash))
		return 0;
	mvFullHashExists[hashIndex] = true;
	mvFullHashKeys[hashIndex] = hash;
	mvFullHashExists[hashIndex2] = true;
	mvFullHashKeys[hashIndex2] = hash;
	mvFullHashExists[hashIndex3] = true;
	mvFullHashKeys[hashIndex3] = hash;

	if(moveSoFar != ERRORMOVE && (b.posCurrentHash == b.posStartHash))
		return 0;

	if(maxSteps <= 0)
	{
		moves[0] = moveSoFar;
		return 1;
	}
	move_t mv[256];
	pla_t pla = b.player;

	int num = 0;
	if(b.step < 3)
		num += BoardMoveGen::genPushPulls(b,pla,mv+num);
	num += BoardMoveGen::genSteps(b,pla,mv+num);
	if(b.step != 0)
		mv[num++] = PASSMOVE;

	int numTotalMoves = 0;
	for(int i = 0; i<num; i++)
	{
		Board copy = b;
		copy.makeMove(mv[i]);
		int ns = Board::numStepsInMove(mv[i]);
		numTotalMoves += genFullMoveHelper(copy,moves+numTotalMoves,Board::concatMoves(moveSoFar,mv[i],stepIndex),stepIndex+ns,maxSteps-ns);
	}
	return numTotalMoves;
}

static int genFullMoves(Board& b, move_t* moves, int maxSteps)
{
	if(mvFullHashKeys == NULL)
		mvFullHashKeys = new hash_t[MV_FULL_HASH_SIZE];
	if(mvFullHashExists == NULL)
		mvFullHashExists = new bool[MV_FULL_HASH_SIZE];

	for(int i = 0; i<(int)MV_FULL_HASH_SIZE; i++)
		mvFullHashExists[i] = false;

	return genFullMoveHelper(b, moves, ERRORMOVE, 0, maxSteps);
}
