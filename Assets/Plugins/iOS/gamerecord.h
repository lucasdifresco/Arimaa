
/*
 * gamerecord.h
 * Author: davidwu
 */

#ifndef GAMERECORD_H
#define GAMERECORD_H

#include <vector>
#include <map>
#include "board.h"

using namespace std;

class BoardRecord
{
	public:
	Board board;
	map<string,string> keyValues;

	BoardRecord();
	BoardRecord(const Board& b, const map<string,string>& keyValues);
};

class GameRecord
{
	public:

	Board board;
	vector<move_t> moves;
	pla_t winner;
	map<string,string> keyValues;

	GameRecord();
	GameRecord(const Board& b, const vector<move_t>& moves, pla_t winner, const map<string,string>& keyValues);
};


#endif
