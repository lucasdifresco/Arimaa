
/*
 * boardhistory.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <fstream>
#include <vector>
#include "board.h"
#include "boardhistory.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;

BoardHistory::BoardHistory()
{
  minTurnNumber = 0;
  maxTurnNumber = -1;
  //minStepNumber = 0;
  //maxStepNumber = -1;
}

BoardHistory::BoardHistory(const Board& b)
{
  reset(b);
}

BoardHistory::~BoardHistory()
{

}

BoardHistory::BoardHistory(const Board& b, const vector<move_t>& moves)
{
	initMoves(b,moves);
}

BoardHistory::BoardHistory(const GameRecord& record)
{
	initMoves(record.board, record.moves);
}

void BoardHistory::initMoves(const Board& b, const vector<move_t>& moves)
{
	reset(b);
	int numMoves = moves.size();
	Board copy = b;
	for(int i = 0; i<numMoves; i++)
	{
		step_t oldStep = copy.step;
		bool success = copy.makeMoveLegal(moves[i]);
		if(!success)
		{
			Global::fatalError(string("BoardHistory: Illegal move ") +
					writeMove(turnBoard[maxTurnNumber],moves[i]) + "\n" + writeBoard(turnBoard[maxTurnNumber]));
		}
		reportMove(copy,moves[i],oldStep);
	}
}


void BoardHistory::resizeIfTooSmall()
{
  if((int)turnPosHash.size() < maxTurnNumber+1)
  {
  	turnBoard.resize(maxTurnNumber+1,Board());
    turnPosHash.resize(maxTurnNumber+1,0);
    turnSitHash.resize(maxTurnNumber+1,0);
    turnMove.resize(maxTurnNumber+1,ERRORMOVE);
    turnPieceCount[0].resize(maxTurnNumber+1,0);
    turnPieceCount[1].resize(maxTurnNumber+1,0);
  }
  //if((int)stepPosHash.size() < maxStepNumber+5)
  //{
  //  stepPosHash.resize(maxStepNumber+5,0);
  //  stepSitHash.resize(maxStepNumber+5,0);
  //  stepMove.resize(maxStepNumber+5,ERRORMOVE);
  //  stepStep.resize(maxStepNumber+5,ERRORSTEP);
  //}
}

void BoardHistory::reset(const Board& b)
{
	minTurnNumber = b.turnNumber;
	maxTurnNumber = minTurnNumber;
	//minStepNumber = getStepNumber(b.turnNumber)+b.step;
	//maxStepNumber = minStepNumber;

  resizeIfTooSmall();

	for(int i = 0; i<minTurnNumber; i++)
	{
		turnPosHash[i] = 0;
		turnSitHash[i] = 0;
		turnMove[i] = ERRORMOVE;
		turnPieceCount[0][i] = 0;
		turnPieceCount[1][i] = 0;
	}
	//for(int i = 0; i<minStepNumber; i++)
	//{
	//	stepPosHash[i] = 0;
	//	stepSitHash[i] = 0;
	//	stepMove[i] = ERRORMOVE;
	//	stepStep[i] = ERRORSTEP;
	//}

	turnBoard[minTurnNumber] = b;
	turnPosHash[minTurnNumber] = b.posStartHash;
	turnSitHash[minTurnNumber] = b.sitCurrentHash;
	turnMove[minTurnNumber] = ERRORMOVE;
	turnPieceCount[0][minTurnNumber] = b.pieceCounts[0][0];
	turnPieceCount[1][minTurnNumber] = b.pieceCounts[1][0];
	//stepPosHash[minStepNumber] = b.posCurrentHash;
	//stepSitHash[minStepNumber] = b.sitCurrentHash;
}

//Indicate that move m was made, resulting in board b, and the step number was lastStep prior to m
//Invalidates all history that occurs in any turns occuring after the turnNumber of b, and appends the results of m to the current
//turnNumber.
//At all times, this will result in the history matching the board up to the new position after the move.
void BoardHistory::reportMove(const Board& b, move_t m, int lastStep)
{
	DEBUGASSERT(m != ERRORMOVE && m != QPASSMOVE);
  int turnNumber = b.turnNumber;
  int oldTurnNumber = b.step == 0 ? turnNumber-1 : turnNumber;
  //int stepNumber = b.turnNumber*4+b.step;
  //int oldStepNumber = oldTurnNumber*4+lastStep;
  maxTurnNumber = turnNumber;
  //maxStepNumber = stepNumber;

  resizeIfTooSmall();

  if(b.step == 0)
  {
  	turnBoard[turnNumber] = b;
    turnPosHash[turnNumber] = b.posCurrentHash;
    turnSitHash[turnNumber] = b.sitCurrentHash;
    turnMove[turnNumber] = ERRORMOVE;
    turnPieceCount[0][turnNumber] = b.pieceCounts[0][0];
    turnPieceCount[1][turnNumber] = b.pieceCounts[1][0];
  }

  turnMove[oldTurnNumber] = Board::concatMoves(turnMove[oldTurnNumber],m,lastStep);

  //stepPosHash[stepNumber] = b.posCurrentHash;
  //stepSitHash[stepNumber] = b.sitCurrentHash;
  //stepMove[oldStepNumber] = m;

  //int ns = Board::numStepsInMove(m);
  //for(int i = 0; i<ns; i++)
  //{
  //  step_t s = Board::getStep(m,i);
	//	if(s == ERRORSTEP)
	//		break;
  //  stepStep[oldStepNumber+i] = s;
  //  if(s == PASSSTEP)
  //  	break;
  //}
}

//Indicate that move m was the full move (so far) made on the given turn number
//Invalidates all history that occurs in any turns occuring after the turnNumber
void BoardHistory::reportMove(int oldTurnNumber, move_t m)
{
	DEBUGASSERT(m != ERRORMOVE && m != QPASSMOVE);
	DEBUGASSERT(oldTurnNumber >= minTurnNumber);

  turnMove[oldTurnNumber] = m;

  int turnNumber = oldTurnNumber+1;
  maxTurnNumber = turnNumber;

  resizeIfTooSmall();

	turnBoard[turnNumber] = turnBoard[oldTurnNumber];
	bool suc = turnBoard[turnNumber].makeMoveLegal(m);
	//DEBUGASSERT(turnBoard[oldTurnNumber].step == 0); //Wrong if you search using a starting step nonzero
	DEBUGASSERT(suc);

	const Board& b = turnBoard[turnNumber];

	turnPosHash[turnNumber] = b.posCurrentHash;
	turnSitHash[turnNumber] = b.sitCurrentHash;
	turnMove[turnNumber] = ERRORMOVE;
	turnPieceCount[0][turnNumber] = b.pieceCounts[0][0];
	turnPieceCount[1][turnNumber] = b.pieceCounts[1][0];
}

void BoardHistory::outputPositions(const char* filename, const BoardHistory& hist)
{
	ofstream out;
	out.open(filename,ios::out);
	out.precision(14);

	for(int turn = hist.minTurnNumber; turn <= hist.maxTurnNumber; turn++)
	{
		out << "# " << writeMove(hist.turnBoard[turn],hist.turnMove[turn]) << endl;
		out << hist.turnBoard[turn] << endl;
		out << ";" << endl;
	}
	out.close();
}

void BoardHistory::outputMoves(const char* filename, const BoardHistory& hist)
{
	ofstream out;
	out.open(filename,ios::out);
	out.precision(14);

	Board startBoard = hist.turnBoard[hist.minTurnNumber];
	out << "1g ";
	for(int i = 0; i<16; i++)
		out << writePlacement(i,startBoard.owners[i],startBoard.pieces[i]) << " ";
	out << endl;
	out << "1s ";
	for(int i = 48; i<64; i++)
		out << writePlacement(i,startBoard.owners[i],startBoard.pieces[i]) << " ";
	out << endl;

	if(hist.turnBoard[hist.minTurnNumber].player == SILV)
		out << "2g " << endl;

	int gameTurn = 2;
	for(int turn = hist.minTurnNumber; turn <= hist.maxTurnNumber; turn++)
	{
		out << gameTurn << (hist.turnBoard[turn].player == GOLD ? "g" : "s") << " ";
		out << writeMove(hist.turnBoard[turn],hist.turnMove[turn],false) << endl;

		if(hist.turnBoard[turn].player == SILV)
			gameTurn++;
	}

	out.close();
}

bool BoardHistory::isThirdRepetition(const Board& b, const BoardHistory& hist)
{
	if(b.step != 0)
		return false;

	hash_t currentHash = b.sitCurrentHash;
	int count = 0;
	int currentTurn = b.turnNumber;

	DEBUGASSERT(currentTurn >= hist.minTurnNumber && currentTurn <= hist.maxTurnNumber);
	DEBUGASSERT(hist.turnSitHash[currentTurn] == currentHash);

	//Compute the start of the boards we care about. In the corner case where the initial board
	//did not begin on step 0, we add one to skip it.
	int start = hist.minTurnNumber + (hist.turnBoard[hist.minTurnNumber].step > 0 ? 1 : 0);

	//TODO compute piece count?

	//Walk backwards, checking for a previous occurrences
	for(int i = currentTurn-2; i >= start; i -= 2)
	{
		if(hist.turnSitHash[i] == currentHash)
		{
			count++;
			if(count >= 2)
				return true;
		}
	}
	return false;
}

ostream& operator<<(ostream& out, const BoardHistory& hist)
{
	cout << "BoardHistory:" << endl;
	if(hist.minTurnNumber <= hist.maxTurnNumber)
	{
		vector<move_t> moves;
		for(int i = hist.minTurnNumber; i<= hist.maxTurnNumber; i++)
			moves.push_back(hist.turnMove[i]);
		out << writeGame(hist.turnBoard[hist.minTurnNumber],moves);
	}
	return out;
}






