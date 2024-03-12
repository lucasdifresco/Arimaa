
/*
 * arimaaio.h
 * Author: davidwu
 *
 * Functions for parsing files containing position, move, or gamestate data.
 *
 * BOARD AND MOVE FORMATS------------------------------------------------------
 *
 * ---Common--------------------
 * Board files contain specific board positions.
 * Move files contain gamerecords.
 * Both formats may have multiple boards/gamerecords contained in the same file, if they are semicolon separated.
 *
 * Whenever '#' character is encountered, subsequent characters on that line are ignored, EXCEPT FOR ';'!!!
 *
 * ---Key-value pairs------------
 * Any particular board file or game record may have key-value pairs associated with it.
 * Key value pairs are of the form "x=y" or "x = y".
 * Multiple key value pairs are allowed on one line if comma separated.
 * Key value pairs are also broken by newlines.

 * ---Board---------------------
 * Valid board line formats are:
 * 'xxxxxxxx' x = valid board char excluding spaces
 * '|xxxxxxxx|' x = valid board char
 * '|x x x x x x x x|' x = valid board char
 * '|x x x x x x x x |' x = valid board char
 * '| x x x x x x x x |' x = valid board char
 *
 * A board consists of 8 contiguous board lines.
 * Header and footer rows, like "+--------+" are allowed.
 *
 * Special key-value pairs P, S, and T are recognized, for player (0-1), step (0-3), and turn number.
 * Special keywords like '42g' or '8s', or '7b', or '10w' at the start of a line will also specify a turn and side to move
 * Other special keywords:
 *   N_TO_S, S_TO_N - will control the board orientation. Default is N_TO_S.
 *   LR_MIRROR - will flip left and right for the board.
 * All other lines and characters not occuring during the board are ignored.
 *
 * ---Moves--------------------
 * A single move follows standard arimaa move notation, such as "4s Eb5w Md6e Me7n Ca3n"
 * Special keywords:
 *   pass appends a PASSSTEP
 *   takeback undoes the move made the previous (not current!) turn
 *   resigns terminates the move list
 *
 * ---Gamestate ---------------
 * Standard Arimaa gamestate notation
 *
 */

#ifndef ARIMAAIO_H
#define ARIMAAIO_H

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include "global.h"
#include "board.h"
#include "gamerecord.h"
#include "timecontrol.h"

using namespace std;

namespace ArimaaIO
{
	void setDefaultDirectory(string dir);

	//OUTPUT--------------------------------------------------------------------
	char writePla(pla_t pla);
  char writePiece(pla_t pla, piece_t piece);
  string writePiece(piece_t piece);
	string writeLoc(loc_t k);
	string writePlacement(loc_t k, pla_t pla, piece_t piece);
	string writePlaTurn(int turn);
	string writePlaTurn(pla_t pla, int turn);
	string writeNum(int num);
	string writeHash(hash_t hash);

	string writeBoard(const Board& b);
	string writeBoardSimple(const Board& b);
	string writeMaterial(const int8_t pieceCounts[2][NUMTYPES]);

	string writePlacements(const Board& b, pla_t pla);
	string writeStartingPlacementsAndTurns(const Board& b);

	string writeStep(step_t step, bool showPass = true);
	string writeStep(const Board& b, step_t step, bool showPass=true);

	string writeMove(move_t move, bool showPass = true);
	string writeMove(const Board& b, move_t move, bool showPass=true);

	string writeMoves(const vector<move_t>& moves);
	string writeMoves(const Board& b, const vector<move_t>& moves);
	string writeMoves(const move_t* moves, int numMoves);
	string writeMoves(const Board& b, const move_t* moves, int numMoves);

	string writeGame(const Board& b, const vector<move_t>& moves);

	//Does not include the semicolon at the end
	string writeBoardRecord(const BoardRecord& record);
	string writeGameRecord(const GameRecord& record);

	string write64(char* arr, const char* fmt);
	string write64(int* arr, const char* fmt);
	string write64(float* arr, const char* fmt);
	string write64(double* arr, const char* fmt);

	//BASIC INPUT---------------------------------------------------------------------

	//Read player (silver, gold, g, s, w, b)
	pla_t readPla(char c);
	pla_t readPla(const string& s);

	//Read piece and owner from [rcdhmeRCDHME.]
	pla_t readPieceOwner(char c);
	piece_t readPiece(char c);
  piece_t readPiece(const string& s);

	//Read board locations (a1, f3, ...)
	loc_t readLoc(const char* s);
	loc_t readLoc(const string& s);
	bool tryReadLoc(const char* s, loc_t& loc);
	bool tryReadLoc(const string& s, loc_t& loc);

	//Read player and turn indicators (1s, 2g, ...)
	int readPlaTurn(const string& s);
	bool tryReadPlaTurn(const string& s, int& turn);

	Board readBoard(const char* str, bool ignoreConsistency = false);
	Board readBoard(const string& str, bool ignoreConsistency = false);

	vector<Board> readBoardFile(const string& boardFile,bool ignoreConsistency = false);
	vector<Board> readBoardFile(const char* boardFile, bool ignoreConsistency = false);
	Board readBoardFile(const string& boardFile, int idx, bool ignoreConsistency = false);
	Board readBoardFile(const char* boardFile, int idx, bool ignoreConsistency = false);

	vector<BoardRecord> readBoardRecordFile(const string& boardFile, bool ignoreConsistency = false);
	vector<BoardRecord> readBoardRecordFile(const char* boardFile, bool ignoreConsistency = false);
	BoardRecord readBoardRecordFile(const string& boardFile, int idx, bool ignoreConsistency = false);
	BoardRecord readBoardRecordFile(const char* boardFile, int idx, bool ignoreConsistency = false);

	vector<pair<BoardRecord,BoardRecord> > readBadGoodPairFile(const char* pairFile);
	vector<pair<BoardRecord,BoardRecord> > readBadGoodPairFile(const string& pairFile);
	pair<BoardRecord,BoardRecord> readBadGoodPairFile(const char* pairFile, int idx);
	pair<BoardRecord,BoardRecord> readBadGoodPairFile(const string& pairFile, int idx);

	//MOVES---------------------------------------------------------------------

	//readStep succeeds on capture move tokens but return ERRORSTEP
	step_t readStep(const string& str);
	move_t readMove(const string& str);

	STRUCT_NAMED_TRIPLE(loc_t,loc,pla_t,owner,piece_t,piece,Placement);
	Placement readPlacement(const string& str);

	//Read a sequence of moves (no placements, turn indicators are ignored)
	//Assumed to start on step 0, rather than partway through a turn.
	vector<move_t> readMoveSequence(const string& str);

	//On error, will not terminate the program, but will rather truncate the move list so that what is there is good.
	GameRecord readMoves(const string& str);

	//On error, will not terminate the program, but will rather truncate the move list so that what is there is good.
	vector<GameRecord> readMovesFile(const string& moveFile);
	vector<GameRecord> readMovesFile(const char* moveFile);
	GameRecord readMovesFile(const string& moveFile, int idx);
	GameRecord readMovesFile(const char* moveFile, int idx);

	//GAME STATE-----------------------------------------------------------------

  //Parses a given gamestate file into key-value pairs and unescapes the characters in the values.
  map<string,string> readGameState(istream& in);
  map<string,string> readGameStateFile(const char* fileName);

  //Retrieves the time control specified by the given gameState
  TimeControl getTCFromGameState(map<string,string> gameState);

  //EXPERIMENTAL---------------------------------------------------------------

  vector<hash_t> readHashListFile(const char* file);
  vector<hash_t> readHashListFile(const string& file);

  //MISC----------------------------------------------------------------------
  uint64_t readMem(const char* str);
  uint64_t readMem(const string& str);
}


#endif
