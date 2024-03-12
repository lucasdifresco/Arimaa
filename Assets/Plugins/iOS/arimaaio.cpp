
/*
 * arimaaio.cpp
 * Author: davidwu
 */
#include "pch.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <utility>
#include <cctype>
#include "global.h"
#include "board.h"
#include "gamerecord.h"
#include "timecontrol.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;

//HELPERS-----------------------------------------------------------------------
static string stripComments(const string& str);
static string unescapeGameStateString(const string& str);

static bool isDigit(char c);
static bool isAlpha(char c);
static bool isNumber(const string& str);
static bool isNumber(const string& str, int start, int end);
static int parseNumber(const string& str);
static int parseNumber(const string& str, int start, int end);
static bool isInChars(char c, const char* str);

//STRING AND CHAR DATA-------------------------------------------------------------

static const char PIECECHARS[3][7] =
{
    {'<', 'r', 'c', 'd', 'h', 'm', 'e'},
    {'>', 'R', 'C', 'D', 'H', 'M', 'E'},
    {'.', '@', '#', '$', '%', '^', '&'}
};

static const char PLACHARS[3] = {'s','g','n'};

static const char DIRCHARS[4] = {'s','w','e','n'};

static const char* LOCSTRINGS[64] =
{
"a1","b1","c1","d1","e1","f1","g1","h1",
"a2","b2","c2","d2","e2","f2","g2","h2",
"a3","b3","c3","d3","e3","f3","g3","h3",
"a4","b4","c4","d4","e4","f4","g4","h4",
"a5","b5","c5","d5","e5","f5","g5","h5",
"a6","b6","c6","d6","e6","f6","g6","h6",
"a7","b7","c7","d7","e7","f7","g7","h7",
"a8","b8","c8","d8","e8","f8","g8","h8",
};

static const char* VALIDBOARDCHARS = "rcdhmeRCDHME .xX*";
static const char* VALIDPIECECHARS = "rcdhmeRCDHME";

//FILE OPEN-----------------------------------------------------------------
static string defaultDir = string("");

void ArimaaIO::setDefaultDirectory(string dir)
{
	defaultDir = dir;
}

static bool openFile(ifstream& in, string s)
{
	in.open(s.c_str());
	if(!in.fail())
		return true;

	in.clear();

	if(defaultDir == string(""))
		return false;

	s = defaultDir + "/" + s;
	in.open(s.c_str());
	if(!in.fail())
		return true;

	return false;
}

static bool openFile(ifstream& in, const char* s)
{
	return openFile(in, string(s));
}


//OUTPUT--------------------------------------------------------------------

char ArimaaIO::writePla(pla_t pla)
{
	if(pla < 0 || pla > 2)
		return '?';
	return PLACHARS[pla];
}

char ArimaaIO::writePiece(pla_t pla, piece_t piece)
{
	if(pla < 0 || pla > 2 || piece < 0 || piece >= NUMTYPES)
		return '?';
	return PIECECHARS[pla][piece];
}

static const char* PIECE_WORDS[NUMTYPES] = {
"emp","rab","cat","dog","hor","cam","ele"
};
string ArimaaIO::writePiece(piece_t piece)
{
  if(piece < 0 || piece >= NUMTYPES)
    return string("?");
  return string(PIECE_WORDS[piece]);
}

string ArimaaIO::writeLoc(loc_t k)
{
	if(k >= 64)
		return string("errorsquare");
	return string(LOCSTRINGS[k]);
}

string ArimaaIO::writePlacement(loc_t k, pla_t pla, piece_t piece)
{
	if(k >= 64)
		return writePiece(pla,piece) + string("errorsquare");
	return writePiece(pla,piece) + string(LOCSTRINGS[k]);
}

string ArimaaIO::writePlaTurn(int turn)
{
	pla_t pla = (turn % 2 == 0) ? GOLD : SILV;
	return writePlaTurn(pla,turn);
}

string ArimaaIO::writePlaTurn(pla_t pla, int turn)
{
	if((pla == GOLD) != (turn % 2 == 0	))
		turn++;

	turn /= 2;
	turn += 2;

	string plaString;
	if(pla < 0 || pla > 2)
		plaString = "errorpla";
	else if(pla == GOLD)
		plaString = "g";
	else if (pla == SILV)
		plaString = "s";
	else
		plaString = "n";

	return Global::intToString(turn) + plaString;
}

string ArimaaIO::writeNum(int num)
{
	return Global::intToString(num);
}

string ArimaaIO::writeHash(hash_t hash)
{
	return Global::uint64ToHexString(hash);
}

string ArimaaIO::writeBoard(const Board& b)
{
	ostringstream out(ostringstream::out);
	out << "P = " << (int)b.player << ", S = " << (int)b.step << ", T = " << b.turnNumber << ", H = " << b.sitCurrentHash << endl;
	out << " +--------+" << endl;
	for(int y = 7; y>=0; y--)
	{
		out << (y+1) << "|";
		for(int x = 0; x<8; x++)
		{
			loc_t k = y*8+x;
			if(b.owners[k] == NPLA && Board::ISTRAP[k])
				out << "*";
			else
				out << PIECECHARS[b.owners[k]][b.pieces[k]];
		}
		out << "|" << endl;
	}
	out << " +--------+" << endl;

	out << "  ";
	for(char c = 'a'; c < 'i'; c++)
		out << c;
	out << " ";
	out << endl;
	return out.str();
}

string ArimaaIO::writeBoardSimple(const Board& b)
{
	ostringstream out(ostringstream::out);
	out << "P" << (int)b.player << " S" << (int)b.step << endl;
	for(int y = 7; y>=0; y--)
	{
		for(int x = 0; x<8; x++)
		{
			loc_t k = y*8+x;
			if(b.owners[k] == NPLA && Board::ISTRAP[k])
				out << "*";
			else
				out << PIECECHARS[b.owners[k]][b.pieces[k]];
		}
		out << endl;
	}
	return out.str();
}

string ArimaaIO::writeMaterial(const int8_t pieceCounts[2][NUMTYPES])
{
	ostringstream out(ostringstream::out);
  for(piece_t piece = ELE; piece >= RAB; piece--)
  {
    for(int i = 0; i<pieceCounts[1][piece]; i++)
      out << PIECECHARS[1][piece];
  }
  out << " ";
  for(piece_t piece = ELE; piece >= RAB; piece--)
  {
    for(int i = 0; i<pieceCounts[0][piece]; i++)
      out << PIECECHARS[0][piece];
  }
  return out.str();
}


string ArimaaIO::writePlacements(const Board& b, pla_t pla)
{
	bool first = true;
	string s;
	for(loc_t loc = 0; loc < 64; loc++)
	{
		if(b.owners[loc] == pla)
		{
			if(!first)
				s += " ";
			first = false;
		  s += writePlacement(loc,b.owners[loc],b.pieces[loc]);
		}
	}
	return s;
}


string ArimaaIO::writeStartingPlacementsAndTurns(const Board& b)
{
	string s;
	
	int totalPieces = 0;
	for (int i = 0; i < NUMTYPES; ++i) { totalPieces += b.pieceCounts[GOLD][i]; }
	if (totalPieces <= 0) { return s; }

	//if(b.pieceCounts[GOLD] <= 0)
		//return s;
	s += "1g ";
	s += writePlacements(b,GOLD);
	s += "\n";
	
	totalPieces = 0;
	for (int i = 0; i < NUMTYPES; ++i) { totalPieces += b.pieceCounts[SILV][i]; }
	if (totalPieces <= 0) { return s; }
	//if(b.pieceCounts[SILV] <= 0)
		//return s;
	s += "1s ";
	s += writePlacements(b,SILV);
	s += "\n";
	return s;
}


string ArimaaIO::writeStep(step_t s0, bool showPass)
{
	ostringstream out(ostringstream::out);
	if(s0 == PASSSTEP)
	{if(showPass) out << "pass";}
	else if(s0 == ERRORSTEP)
	{if(showPass) out << "errorstep";}
	else if(s0 == QPASSSTEP)
	{if(showPass) out << "qpss";}
	else
	{
		//The main step
		loc_t k0 = Board::K0INDEX[s0];

		if(k0 == ERRORSQUARE)
			out << "errorstep";
		else
		{
			uint8_t dir = s0 / 64;
			out << LOCSTRINGS[k0] << DIRCHARS[dir];
		}
	}
	return out.str();
}

string ArimaaIO::writeStep(const Board& b, step_t s0, bool showPass)
{
	ostringstream out(ostringstream::out);
	if(s0 == PASSSTEP)
	{if(showPass) out << "pass";}
	else if(s0 == ERRORSTEP)
	{if(showPass) out << "errorstep";}
	else if(s0 == QPASSSTEP)
	{if(showPass) out << "qpss";}
	else
	{
		//The main step
		loc_t k0 = Board::K0INDEX[s0];
		loc_t k1 = Board::K1INDEX[s0];

		if(k0 == ERRORSQUARE)
			out << "errorstep";
		else
		{
			uint8_t dir = s0 / 64;
			out << PIECECHARS[b.owners[k0]][b.pieces[k0]] << LOCSTRINGS[k0] << DIRCHARS[dir];

			//Captures
			loc_t kt = Board::ADJACENTTRAP[k0];
			if(kt != ERRORSQUARE)
			{
				if(kt == k1 && !b.isGuarded2(b.owners[k0],kt))
					out << " " << PIECECHARS[b.owners[k0]][b.pieces[k0]] << LOCSTRINGS[kt] << "x";
				else if(b.owners[kt] == b.owners[k0] && !b.isGuarded2(b.owners[k0],kt))
					out << " " << PIECECHARS[b.owners[kt]][b.pieces[kt]] << LOCSTRINGS[kt] << "x";
			}
		}
	}
	return out.str();
}

string ArimaaIO::writeMove(move_t move, bool showPass)
{
	string str;
	for(int i = 0; i<4; i++)
	{
	  step_t s = Board::getStep(move,i);
		if(s == ERRORSTEP) break;
		if(i > 0) str += string(" ");
		str += writeStep(s,showPass);
		if(s == PASSSTEP || s == QPASSSTEP) break;
	}
	return str;
}

string ArimaaIO::writeMove(const Board& b, move_t move, bool showPass)
{
	Board copy = b;
	string str;
	for(int i = 0; i<4; i++)
	{
	  step_t s = Board::getStep(move,i);
		if(s == ERRORSTEP) break;
		if(i > 0) str += string(" ");
		str += writeStep(copy,s,showPass);
		copy.makeStepLegal(s);
		if(s == PASSSTEP || s == QPASSSTEP) break;
	}
	return str;
}

string ArimaaIO::writeMoves(const vector<move_t>& moves)
{
	string s;
	bool first = true;
	for(int i = 0; i<(int)moves.size(); i++)
	{
		if(!first)
			s += " ";
		first = false;
		s += writeMove(moves[i]);
	}
	return s;
}

string ArimaaIO::writeMoves(const Board& b, const vector<move_t>& moves)
{
	string s;
	bool first = true;
	Board copy = b;
	for(int i = 0; i<(int)moves.size(); i++)
	{
		if(!first && copy.step == 0)
		  s += "  ";
		else if(!first)
			s += " ";
		first = false;
		s += writeMove(copy, moves[i]);

		//Use makeMoveLegal to guard vs corrupt moves but ignore failures
		copy.makeMoveLegal(moves[i]);
	}
	return s;
}

string ArimaaIO::writeMoves(const move_t* moves, int numMoves)
{
	string s;
	bool first = true;
	for(int i = 0; i<numMoves; i++)
	{
		if(!first)
			s += " ";
		first = false;
		s += writeMove(moves[i]);
	}
	return s;
}

string ArimaaIO::writeMoves(const Board& b, const move_t* moves, int numMoves)
{
	string s;
	bool first = true;
	Board copy = b;
	for(int i = 0; i<numMoves; i++)
	{
		if(!first && copy.step == 0)
		  s += "  ";
		else if(!first)
			s += " ";
		first = false;
		s += writeMove(copy, moves[i]);

		//Use makeMoveLegal to guard vs corrupt moves but ignore failures
		copy.makeMoveLegal(moves[i]);
	}
	return s;
}


string ArimaaIO::writeGame(const Board& b, const vector<move_t>& moves)
{
	string s;
	s += writeStartingPlacementsAndTurns(b);

	Board copy = b;
	for(int i = 0; i<(int)moves.size(); i++)
	{
		s += writePlaTurn(copy.player,copy.turnNumber);
		s += " ";

		if(moves[i] == PASSMOVE)
			s += writeMove(copy,moves[i],true);
		else
			s += writeMove(copy,moves[i],false);

		s += "\n";

		//Use makeMoveLegal to guard vs corrupt moves but ignore failures
		copy.makeMoveLegal(moves[i]);
	}
	return s;
}

static const int NUM_BOARDKEYS = 4;
static const string BOARDKEYS[NUM_BOARDKEYS] = {
		string("P"),
		string("S"),
		string("T"),
		string("H"),
};

string ArimaaIO::writeBoardRecord(const BoardRecord& record)
{
	string s;
	for(map<string,string>::const_iterator iter = record.keyValues.begin(); iter != record.keyValues.end(); ++iter)
	{
		bool isBoardKey = false;
		for(int i = 0; i<NUM_BOARDKEYS; i++)
			if(iter->first == BOARDKEYS[i])
				{isBoardKey = true; break;}
		if(isBoardKey)
			continue;

		s += iter->first + " = " + iter->second + "\n";
	}
	s += writeBoard(record.board) + "\n";
	return s;
}

string ArimaaIO::writeGameRecord(const GameRecord& record)
{
	string s;
	for(map<string,string>::const_iterator iter = record.keyValues.begin(); iter != record.keyValues.end(); ++iter)
		s += iter->first + " = " + iter->second + "\n";
	s += writeMoves(record.board,record.moves) + "\n";
	return s;
}

string ArimaaIO::write64(int* arr, const char* fmt)
{
	string s;
	for(int y = 7; y>=0; y--)
	{
		for(int x = 0; x<8; x++)
			s += Global::strprintf(fmt,arr[x+y*8]);
		s += Global::strprintf("\n");
	}
	return s;
}

string ArimaaIO::write64(double* arr, const char* fmt)
{
	string s;
	for(int y = 7; y>=0; y--)
	{
		for(int x = 0; x<8; x++)
			s += Global::strprintf(fmt,arr[x+y*8]);
		s += Global::strprintf("\n");
	}
	return s;
}

string ArimaaIO::write64(char* arr, const char* fmt)
{
	string s;
	for(int y = 7; y>=0; y--)
	{
		for(int x = 0; x<8; x++)
			s += Global::strprintf(fmt,arr[x+y*8]);
		s += Global::strprintf("\n");
	}
	return s;
}

string ArimaaIO::write64(float* arr, const char* fmt)
{
	string s;
	for(int y = 7; y>=0; y--)
	{
		for(int x = 0; x<8; x++)
			s += Global::strprintf(fmt,arr[x+y*8]);
		s += Global::strprintf("\n");
	}
	return s;
}


//BASIC INPUT---------------------------------------------------------------------

pla_t ArimaaIO::readPla(char c)
{
	switch(c)
	{
		case 's': case 'b': case '0': return SILV;
		case 'g': case 'w': case '1': return GOLD;
		default: return NPLA;
	}
}

pla_t ArimaaIO::readPla(const string& s)
{
	if(s.length() == 1)
		return readPla(s[0]);

	string t = Global::toLower(s);
	if(t == string("gold"))
		return GOLD;
	if(t == string("silv") || t == string("silver"))
		return SILV;

	return NPLA;
}

pla_t ArimaaIO::readPieceOwner(char c)
{
	switch(c)
	{
		case 'r': case 'c': case 'd': case 'h': case 'm': case 'e': return SILV;
		case 'R': case 'C': case 'D': case 'H': case 'M': case 'E': return GOLD;
		default: return NPLA;
	}
}

piece_t ArimaaIO::readPiece(char c)
{
	switch(c)
	{
		case 'r': case 'R': return RAB;
		case 'c': case 'C': return CAT;
		case 'd': case 'D': return DOG;
		case 'h': case 'H': return HOR;
		case 'm': case 'M': return CAM;
		case 'e': case 'E': return ELE;
		default: return EMP;
	}
}

piece_t ArimaaIO::readPiece(const string& s)
{
  if(s.length() == 1)
    return readPiece(s[0]);

  string t = Global::toLower(s);
  if(t == string("ele")) return ELE;
  if(t == string("cam")) return CAM;
  if(t == string("hor")) return HOR;
  if(t == string("dog")) return DOG;
  if(t == string("cat")) return CAT;
  if(t == string("rab")) return RAB;
  return EMP;
}

loc_t ArimaaIO::readLoc(const char* s)
{
	if(!(s[0] != 0 && s[1] != 0))
		Global::fatalError(string("ArimaaIO: invalid loc: ") + s);

	char x = s[0]-'a';
  char y = s[1]-'1';

  if(!(x >= 0 && x < 8) || !(y >= 0 && y < 8))
		Global::fatalError(string("ArimaaIO: invalid loc: ") + s);

  return x + y*8;
}

loc_t ArimaaIO::readLoc(const string& s)
{
	return readLoc(s.c_str());
}

bool ArimaaIO::tryReadLoc(const char* s, loc_t& loc)
{
	if(!(s[0] != 0 && s[1] != 0))
		return false;
  char x = s[0]-'a';
  char y = s[1]-'1';
  if(!(x >= 0 && x < 8) || !(y >= 0 && y < 8))
  	return false;
  loc = x + y*8;
  return true;
}

bool ArimaaIO::tryReadLoc(const string& s, loc_t& loc)
{
	if(!(s.length() >= 2))
		return false;
  char x = s[0]-'a';
  char y = s[1]-'1';
  if(!(x >= 0 && x < 8) || !(y >= 0 && y < 8))
  	return false;
  loc = x + y*8;
  return true;
}

int ArimaaIO::readPlaTurn(const string& s)
{
	int turn = 0;
	if(!tryReadPlaTurn(s,turn))
		Global::fatalError(string("ArimaaIO: invalid platurn: ") + s);
	return turn;
}

bool ArimaaIO::tryReadPlaTurn(const string& s, int& turn)
{
	int len = s.length();
	if(len < 2)
		return false;
	pla_t pla = readPla(s[len-1]);

	if(pla == NPLA)
		return false;

	if(!isNumber(s,0,len-1))
		return false;

	int t = parseNumber(s,0,len-1);

	if((t & 0x3FFFFFFF) != t)
		return false;

	turn = (t-2) * 2 + (pla == SILV ? 1 : 0);
	return true;
}


//KEY VALUE PAIRS-------------------------------------------------------------

static map<string,string> readKeyValues(const string& contents)
{
	istringstream lineIn(contents);
	string line;
	map<string,string> keyValues;
	while(getline(lineIn,line))
	{
		if(line.length() <= 0) continue;
		istringstream commaIn(line);
		string commaChunk;
		while(getline(commaIn,commaChunk,','))
		{
			if(commaChunk.length() <= 0) continue;
			size_t equalsPos = commaChunk.find_first_of('=');
			if(equalsPos == string::npos) continue;
			string leftChunk = Global::trim(commaChunk.substr(0,equalsPos));
			string rightChunk = Global::trim(commaChunk.substr(equalsPos+1));
			if(leftChunk.length() == 0)
				Global::fatalError("ArimaaIO: key value pair without key: " + line);
			if(rightChunk.length() == 0)
				Global::fatalError("ArimaaIO: key value pair without value: " + line);
			if(keyValues.find(leftChunk) != keyValues.end())
				Global::fatalError("ArimaaIO: duplicate key: " + leftChunk);
			keyValues[leftChunk] = rightChunk;
		}
	}
	return keyValues;
}

//BOARD----------------------------------------------------------------------

static bool tryFillBoardRow(char bchars[64], int y, const string& boardLine, bool allowSpaces)
{
  //Ensure we have a valid index
  if(y < 0 || y >= 8)
    return false;
  //Ensure it is the right size to be a board row - 8 exactly, or around 16
  int size = boardLine.size();
  if(size != 8 && size != 15 && size != 16 && size != 17)
    return false;
  //Ensure it is composed only of board charcters
  for(int i = 0; i<size; i++)
  {
  	if(!isInChars(boardLine[i],VALIDBOARDCHARS))
  		return false;
  }

  int skip = (boardLine.size()+1)/8;
  int start = (size >= 16) ? 1 : 0;

  //Scan across and check for spaces in the board input
  if(!allowSpaces)
  	for(int x = 0; x<8; x++)
  		if(boardLine[start+x*skip] == ' ')
  			return false;

  //Ensure that everything else is empty
  for(int i = 0; i<size; i++)
  	if((i-start+skip)%skip != 0 && boardLine[i] != ' ')
  		return false;

	//Scan across and set pieces!
  for(int x = 0; x<8; x++)
  	bchars[y*8+x] = boardLine[start+x*skip];
  return true;
}

static bool isIsolatedToken(const string& line, size_t pos, size_t endPos)
{
	if(pos > 0 && pos <= line.length() && (isAlpha(line[pos-1]) || isDigit(line[pos-1])))
		return false;
	if(endPos < line.length() && (isAlpha(line[endPos]) || isDigit(line[endPos])))
		return false;
	return true;
}

static bool lookForKeyValueNumber(const map<string,string>& keyValues, const string& key, int lowerBound, int upperBound, int& number)
{
	map<string,string>::const_iterator iter = keyValues.find(key);
	if(iter == keyValues.end())
		return false;
	const string& value = iter->second;
	if(!isNumber(value))
		Global::fatalError(string("ArimaaIO: could not parse value for override: ") + key + " " + value);
	int num = parseNumber(value);
	if(num < lowerBound || num >= upperBound)
		Global::fatalError(string("ArimaaIO: could not parse value for override: ") + key + " " + value);
	number = num;
	return true;
}

static BoardRecord readBoardRecord(const string& arg, bool ignoreConsistency)
{
	string str = stripComments(arg);

  Board b = Board();
	char bchars[64];
	for(int i = 0; i<64; i++)
		bchars[i] = '.';

  int linesSucceeded = 0; //Track # of rows of board successfully read in
  bool readingBoard = false; //Have we found where the board encoding begins?
  istringstream in(str);  //Turn str into a stream so we can go line by line

  //Special orientation overrides
  bool overrideNToS = false;
  bool overrideSToN = false;
  bool overrideLRMirror = false;

  //Board rows must be contiguous
  //Read each row...
  //Pound characters act as comments
  string line;
  while(getline(in,line))
  {
  	//If it has exactly 64 valid non-space characters
  	if(line.length() >= 64)
  	{
  		int numValid = 0;
  		for(int i = 0; i<(int)line.length(); i++)
  			if(line[i] != ' ' && isInChars(line[i],VALIDBOARDCHARS))
  				numValid++;
  		if(numValid == 64)
  		{
  			loc_t loc = 0;
    		for(int i = 0; i<(int)line.length(); i++)
    			if(line[i] != ' ' && isInChars(line[i],VALIDBOARDCHARS))
    				bchars[loc++] = line[i];
  			readingBoard = true;
  			linesSucceeded += 8;
  			continue;
  		}
  	}

    //If it's has vertical bars, maybe it's a board row delimited by them?
    if(line.find('|') != string::npos)
    {
      //Find the boundaries
      size_t start = line.find_first_of('|')+1;
      size_t end = line.find_last_of('|');

      //Figure out which row of the board to fill.
      //If there's a number before the vertical bars, use that to determine it, 1-indexed
      //Else use the #of lines succeeded
      int y;
      if(start > 0 && isDigit(line[start-1]))
        y = line[start-1] - '1';
      else
        y = linesSucceeded;

      //Try to fill it as a board row
      if(y >= 0 && y < 8 && tryFillBoardRow(bchars,y,line.substr(start,end-start),true))
      {linesSucceeded++; readingBoard = true; continue;}
    }

    //If it's exactly 8 characters, maybe it's a board row then?
    if(line.size() == 8)
    {
      if(linesSucceeded < 8 && tryFillBoardRow(bchars,linesSucceeded,line,false))
      {linesSucceeded++; readingBoard = true; continue;}
    }

    //If we got here, we didn't find a board line, but if we're reading the board and not done...
    if(readingBoard && linesSucceeded < 8)
      Global::fatalError("ArimaaIO: error reading board:\n" + str);

    //If it begins with something line 4b or 22g, then it specifies a side to move...
    int i = 0;
    while(i < (int)line.size() && isDigit(line[i])) {i++;}
    if(i < (int)line.size() && i > 0)
    {
    	int turnNumber = parseNumber(line,0,i);
      if(line[i] == 'b' || line[i] == 's') {b.setPlaStep(0,b.step); b.turnNumber = max(turnNumber*2-4,0);}
      else if(line[i] == 'w' || line[i] == 'g') {b.setPlaStep(1,b.step); b.turnNumber = max(turnNumber*2-3,0);}
    }

    //Scan through the string looking for special codes
    static const string nToSKeyword = "N_TO_S";
    static const string sToNKeyword = "S_TO_N";
    static const string lrMirrorKeyword = "LRMIRROR";
    size_t pos;

    pos = line.find(nToSKeyword);
    if(pos != string::npos && isIsolatedToken(line, pos, pos + nToSKeyword.length()))
    	overrideNToS = true;

    pos = line.find(sToNKeyword);
    if(pos != string::npos && isIsolatedToken(line, pos, pos + sToNKeyword.length()))
    	overrideSToN = true;

    pos = line.find(lrMirrorKeyword);
    if(pos != string::npos && isIsolatedToken(line, pos, pos + lrMirrorKeyword.length()))
    	overrideLRMirror = true;
  }

  if(linesSucceeded != 8)
  	Global::fatalError("ArimaaIO: error reading board:\n" + str);

  if(overrideNToS && overrideSToN)
  	Global::fatalError("ArimaaIO: both overrides specified:\n" + str);

  //Flip board
  if(!overrideSToN)
  {
  	if(!overrideLRMirror)
  	{
			for(int y = 0; y<8; y++)
				for(int x = 0; x<8; x++)
					b.setPiece((7-y)*8+x, readPieceOwner(bchars[y*8+x]), readPiece(bchars[y*8+x]));
  	}
  	else
  	{
			for(int y = 0; y<8; y++)
				for(int x = 0; x<8; x++)
					b.setPiece((7-y)*8+(7-x), readPieceOwner(bchars[y*8+x]), readPiece(bchars[y*8+x]));
  	}
  }
  else
  {
  	if(!overrideLRMirror)
  	{
			for(int y = 0; y<8; y++)
				for(int x = 0; x<8; x++)
					b.setPiece(y*8+x, readPieceOwner(bchars[y*8+x]), readPiece(bchars[y*8+x]));
  	}
  	else
  	{
			for(int y = 0; y<8; y++)
				for(int x = 0; x<8; x++)
					b.setPiece(y*8+(7-x), readPieceOwner(bchars[y*8+x]), readPiece(bchars[y*8+x]));
  	}
  }

  map<string,string> keyValues = readKeyValues(str);

  //Special keywords
  int plaOverride = 0;
  static const string plaKeyword = "P";
  if(lookForKeyValueNumber(keyValues,plaKeyword,0,2,plaOverride))
  	b.setPlaStep(plaOverride,b.step);

  int stepOverride = 0;
  static const string stepKeyword = "S";
  if(lookForKeyValueNumber(keyValues,stepKeyword,0,4,stepOverride))
  	b.setPlaStep(b.player,stepOverride);

  int turnOverride = 0;
  static const string turnKeyword = "T";
  if(lookForKeyValueNumber(keyValues,turnKeyword,0,1000000000,turnOverride))
  	b.turnNumber = turnOverride;

  b.refreshStartHash();

  if(!ignoreConsistency && !b.testConsistency(cout))
  	Global::fatalError("ArimaaIO: inconsistent board:\n" + str);

  BoardRecord bret;
  bret.board = b;
  bret.keyValues = keyValues;
  return bret;
}

Board ArimaaIO::readBoard(const char* str, bool ignoreConsistency)
{
	return readBoardRecord(string(str),ignoreConsistency).board;
}

Board ArimaaIO::readBoard(const string& str, bool ignoreConsistency)
{
	return readBoardRecord(str,ignoreConsistency).board;
}

vector<BoardRecord> ArimaaIO::readBoardRecordFile(const string& boardFile, bool ignoreConsistency)
{
	return readBoardRecordFile(boardFile.c_str(),ignoreConsistency);
}

vector<BoardRecord> ArimaaIO::readBoardRecordFile(const char* boardFile, bool ignoreConsistency)
{
  ifstream in;
  if(!openFile(in,boardFile))
  	Global::fatalError(string("ArimaaIO: could not open file: ") + boardFile);

  string str;
  vector<BoardRecord> boardRecords;
  while(getline(in,str,';'))
  {
    if(str.find_first_not_of(" \n\t\r") == string::npos)
      continue;
    boardRecords.push_back(readBoardRecord(str, ignoreConsistency));
  }
  in.close();
  return boardRecords;
}

BoardRecord ArimaaIO::readBoardRecordFile(const string& boardFile, int idx, bool ignoreConsistency)
{
	return readBoardRecordFile(boardFile.c_str(),idx,ignoreConsistency);
}

BoardRecord ArimaaIO::readBoardRecordFile(const char* boardFile, int idx, bool ignoreConsistency)
{
  if(idx < 0)
  	Global::fatalError(string("ArimaaIO: idx is negative: ") + Global::intToString(idx));

  ifstream in;
  if(!openFile(in,boardFile))
  	Global::fatalError(string("ArimaaIO: could not open file: ") + boardFile);

  string str;
  BoardRecord boardRecord;
  int i = 0;
  while(getline(in,str,';'))
  {
    if(str.find_first_not_of(" \n\t\r") == string::npos)
      continue;
    if(i < idx)
    {i++; continue;}
    boardRecord = readBoardRecord(str, ignoreConsistency);
    i++;
    break;
  }
  if(i <= idx)
  	Global::fatalError(string("ArimaaIO: could not find idx ") + Global::intToString(idx) + " in file: " + boardFile);

  in.close();
  return boardRecord;
}

vector<Board> ArimaaIO::readBoardFile(const string& boardFile, bool ignoreConsistency)
{
	return readBoardFile(boardFile.c_str(),ignoreConsistency);
}

vector<Board> ArimaaIO::readBoardFile(const char* boardFile, bool ignoreConsistency)
{
	vector<BoardRecord> boardRecords = readBoardRecordFile(boardFile,ignoreConsistency);
	vector<Board> boards;
	boards.reserve(boardRecords.size());
	for(int i = 0; i<(int)boardRecords.size(); i++)
		boards.push_back(boardRecords[i].board);
	return boards;
}

Board ArimaaIO::readBoardFile(const string& boardFile, int idx, bool ignoreConsistency)
{
	return readBoardRecordFile(boardFile,idx,ignoreConsistency).board;
}

Board ArimaaIO::readBoardFile(const char* boardFile, int idx, bool ignoreConsistency)
{
	return readBoardRecordFile(boardFile,idx,ignoreConsistency).board;
}

static bool isToken(const string& str, int pos, int len)
{
	return (pos <= 0 || (!isalnum(str[pos-1]) && str[pos-1] != '_'))
			&& (pos+len >= (int)str.length() || (!isalnum(str[pos+len]) && str[pos+len] != '_'));
}

static size_t findToken(const string& str, const string& token)
{
	size_t pos = 0;
	while(true)
	{
		pos = str.find(token,pos);
		if(pos == string::npos)
			return string::npos;
		if(isToken(str,pos,token.length()))
			return pos;
		pos = pos+token.length();
		if(pos >= str.length())
			return string::npos;
	}
}

static pair<BoardRecord,BoardRecord> readBadGoodPair(const string& arg)
{
	string str = stripComments(arg);
	size_t badpos = findToken(str,string("BAD"));
	size_t goodpos = findToken(str,string("GOOD"));

	if(badpos == string::npos || goodpos == string::npos)
		Global::fatalError(string("ArimaaIO: did not find BAD or GOOD in parsing badgoodpair"));
	if(badpos >= goodpos)
		Global::fatalError(string("ArimaaIO: found GOOD before BAD in badgoodpair"));

	string badstr = str.substr(badpos+3,goodpos);
	string goodstr = str.substr(goodpos+4);

	BoardRecord badrecord = readBoardRecord(badstr,false);
	BoardRecord goodrecord = readBoardRecord(goodstr,false);

	return std::make_pair(badrecord,goodrecord);
}

vector<pair<BoardRecord,BoardRecord> > ArimaaIO::readBadGoodPairFile(const char* pairFile)
{
  ifstream in;
  if(!openFile(in,pairFile))
  	Global::fatalError(string("ArimaaIO: could not open file: ") + pairFile);

  string str;
  vector<pair<BoardRecord,BoardRecord> > pairs;
  while(getline(in,str,';'))
  {
    if(str.find_first_not_of(" \n\t\r") == string::npos)
      continue;
    pairs.push_back(readBadGoodPair(str));
  }
  in.close();
  return pairs;
}

vector<pair<BoardRecord,BoardRecord> > ArimaaIO::readBadGoodPairFile(const string& pairFile)
{
	return readBadGoodPairFile(pairFile.c_str());
}

pair<BoardRecord,BoardRecord> ArimaaIO::readBadGoodPairFile(const char* pairFile, int idx)
{
  if(idx < 0)
  	Global::fatalError(string("ArimaaIO: idx is negative: ") + Global::intToString(idx));

  ifstream in;
  if(!openFile(in,pairFile))
  	Global::fatalError(string("ArimaaIO: could not open file: ") + pairFile);

  string str;
  pair<BoardRecord,BoardRecord> pair;
  int i = 0;
  while(getline(in,str,';'))
  {
    if(str.find_first_not_of(" \n\t\r") == string::npos)
      continue;
    if(i < idx)
    {i++; continue;}
    pair = readBadGoodPair(str);
    i++;
    break;
  }
  if(i <= idx)
  	Global::fatalError(string("ArimaaIO: could not find idx ") + Global::intToString(idx) + " in file: " + pairFile);

  in.close();
  return pair;
}

pair<BoardRecord,BoardRecord> ArimaaIO::readBadGoodPairFile(const string& pairFile, int idx)
{
	return readBadGoodPairFile(pairFile.c_str(),idx);
}

//MOVES---------------------------------------------------------------------

static bool tryReadStep(const string& str, step_t& step)
{
	string wrd = Global::trim(str);

	if(wrd == string("pass"))
	{step = PASSSTEP; return true;}

	if(wrd == string("qpss"))
	{step = QPASSSTEP; return true;}

	char xchar;
	char ychar;
	char dirchar;
	if(wrd.size() == 4)
	{
		//Ensure the first character really is valid
		if(!isInChars(wrd[0],VALIDPIECECHARS))
			return false;

		xchar = wrd[1];
		ychar = wrd[2];
		dirchar = wrd[3];
	}
	else if(wrd.size() == 3)
	{
		xchar = wrd[0];
		ychar = wrd[1];
		dirchar = wrd[2];
	}
	else
		return false;

	//Ensure the characters specify a location
	if(xchar < 'a' || xchar > 'h')
		return false;
	if(ychar < '1' || ychar > '8')
		return false;

	//Parse out the location
	int x = xchar-'a';
	int y = ychar-'1';
	loc_t k = x+y*8;

	//Create the step
	if     (dirchar == 's' && y > 0) {step = Board::STEPINDEX[k][k-8];}
	else if(dirchar == 'w' && x > 0) {step = Board::STEPINDEX[k][k-1];}
	else if(dirchar == 'e' && x < 7) {step = Board::STEPINDEX[k][k+1];}
	else if(dirchar == 'n' && y < 7) {step = Board::STEPINDEX[k][k+8];}
	else if(dirchar == 'x'         ) {step = ERRORSTEP;} // Capture token
	else {return false;}

	return true;
}

step_t ArimaaIO::readStep(const string& str)
{
	step_t step;
	bool suc = tryReadStep(str,step);
	if(!suc)
		Global::fatalError(string("ArimaaIO: invalid step: ") + str);
	return step;
}

static bool tryReadMove(const string& str, move_t& ret)
{
  istringstream in(str);
  string wrd;
  move_t move = ERRORMOVE;
  int ns = 0;
	while(in >> wrd)
	{
		if(wrd.length() == 0)
			break;
		step_t step;
		bool suc = tryReadStep(wrd,step);
		if(!suc)
			return false;

		if(step == ERRORSTEP)
			continue;

		if(ns >= 4)
			return false;

		move = Board::setStep(move,step,ns);
		ns++;
	}

	ret = move;
	return true;
}

move_t ArimaaIO::readMove(const string& str)
{
	move_t move = ERRORMOVE;
	bool suc = tryReadMove(str,move);
	if(!suc)
		Global::fatalError(string("ArimaaIO: invalid move: ") + str);
	return move;
}

static bool tryReadPlacement(const string& str, Placement& ret)
{
	string wrd = Global::trim(str);

	if(wrd.size() != 3)
		return false;

	char piecechar = wrd[0];
	char xchar = wrd[1];
	char ychar = wrd[2];

	if(!isInChars(piecechar,VALIDPIECECHARS))
		return false;
	if(xchar < 'a' || xchar > 'h')
		return false;
	if(ychar < '1' || ychar > '8')
		return false;

	//Parse out the location
	int x = xchar-'a';
	int y = ychar-'1';
	loc_t k = x+y*8;

	pla_t owner = readPieceOwner(piecechar);
	piece_t piece = readPiece(piecechar);

	ret.loc = k;
	ret.owner = owner;
	ret.piece = piece;
	return true;
}

Placement ArimaaIO::readPlacement(const string& str)
{
	Placement p;
	bool suc = tryReadPlacement(str,p);
	if(!suc)
		Global::fatalError(string("ArimaaIO: invalid placement: ") + str);
	return p;
}

vector<move_t> ArimaaIO::readMoveSequence(const string& arg)
{
	string str = stripComments(arg);
	str = Global::trim(str);

	vector<move_t> moveList;
	move_t move = ERRORMOVE;

	//Read token by token
	istringstream in(str);
	string wrd;
	while(in >> wrd)
	{
		int size = wrd.size();

		//Tokens like 2w, 34b - indicates turn change, ignore them
		if(size >= 2 && isNumber(wrd,0,size-1) && (wrd[size-1] == 'g' || wrd[size-1] == 'w' || wrd[size-1] == 's' || wrd[size-1] == 'b'))
			continue;

		step_t step;
		if(tryReadStep(wrd,step))
		{
			if(step == ERRORSTEP)
				continue;

			int ns = Board::numStepsInMove(move);
			if(ns >= 4)
			{
				moveList.push_back(move);
				move = ERRORMOVE;
				ns = 0;
			}
			move = Board::setStep(move,step,ns);
			if(step == PASSSTEP || step == QPASSSTEP)
			{
				moveList.push_back(move);
				move = ERRORMOVE;
			}

			continue;
		}

		cout << "ArimaaIO: unknown move token: " << wrd << endl;
		cout << str << endl;
		break;
	}

	//Append a finishing move if one exists
	if(move != ERRORMOVE)
	{
		moveList.push_back(move);
	}
	return moveList;
}

GameRecord ArimaaIO::readMoves(const string& arg)
{
	string str = stripComments(arg);
	str = Global::trim(str);

  Board b;
  vector<move_t> moveList;

  //Tracking vars
  //-3: before 1w. -2: during 1w. -1: during 1b. 0: during 2g
  int moveIndex = -3; //Begins -3 because we have to pass 1w and 1b and 2w to begin actual moves
  move_t move = ERRORMOVE;
  pla_t activePla = SILV;

  //Read line by line...
  string line;
  istringstream in_line(str);
  while(getline(in_line,line))
  {
  	//Skip key value pairs
  	if(line.find_first_of('=') != string::npos)
  		continue;

    //Read token by token
    istringstream in(line);
    string wrd;
    while(in >> wrd)
    {
			int size = wrd.size();

			if(wrd == string("takeback"))
			{
				//Takeback! So we need to unwind the last move made.
				//Decrement twice because the next turn change token will increment again
				//Like 2w <move> 2b takeback 2w <new move>
				moveIndex -= 2;
				activePla = ((moveIndex+4) % 2 == 0) ? GOLD : SILV;

				//Need to clear the board of placements
				if(moveIndex <= -3)
					b = Board();

				//Need to clear just silver
				if(moveIndex == -2)
					for(int x = 0; x<64; x++)
						if(b.owners[x] == SILV)
							b.setPiece(x,NPLA,EMP);

				//Clear move
				move = ERRORMOVE;
				continue;
			}

			if(wrd == string("resigns") || wrd == string("resign"))
			{
				//Clear move
				move = ERRORMOVE;
				break;
			}

			//Tokens like 2w, 34b - indicates turn change
			if(size >= 2 && isNumber(wrd,0,size-1) && (wrd[size-1] == 'g' || wrd[size-1] == 'w' || wrd[size-1] == 's' || wrd[size-1] == 'b'))
			{
				//Numbers can't be too large
				if(size > 10)
				{cout << "ArimaaIO: value too large: " << wrd << endl; cout << str << endl; break;}

				//Count up
				moveIndex++;
				activePla = ((moveIndex+4) % 2 == 0) ? GOLD : SILV;

				//Ensure things match
				int wrdnum = parseNumber(wrd,0,size-1);
				if(wrdnum != (moveIndex+4)/2 || ((wrd[size-1] == 'g' || wrd[size-1] == 'w') != (activePla == GOLD)))
				{cout << "ArimaaIO: turn number not valid: " << wrd << endl; cout << str << endl; break;}

				//Append move, except if there were no steps at all, this probably is an empty move or follows a takeback or something
				if(moveIndex >= 1 && move != ERRORMOVE)
				{
					//Append a pass if not all steps taken and no pass
					move = Board::completeTurn(move);

					//Expand list if needed
					if((int)moveList.size() < moveIndex)
						moveList.resize(moveIndex);
					moveList[moveIndex-1] = move;
				}

				//Clear move
				move = ERRORMOVE;

				continue;
			}

			Placement placement;
			if(tryReadPlacement(wrd,placement))
			{
				//Fail on placements which happen after 1w/1b
				if(moveIndex < 0)
					b.setPiece(placement.loc,placement.owner,placement.piece);
				else
				{cout << "ArimaaIO: illegal placement after first turn" << endl; cout << str << endl; break;}

				continue;
			}

			step_t step;
			if(tryReadStep(wrd,step))
			{
				if(step == ERRORSTEP)
					continue;

				int ns = Board::numStepsInMove(move);
				if(ns >= 4)
				{cout << "ArimaaIO: Too many steps in move!" << endl; cout << str << endl; break;}
				move = Board::setStep(move,step,ns);

				continue;
			}

			cout << "ArimaaIO: Unknown move token: " << wrd << endl;
			cout << str << endl;
			break;
		}
  }

  //Append a finishing move if one exists
  if(move != ERRORMOVE && moveIndex >= 0)
  {
    //Append a pass if not all steps taken and no pass
  	move = Board::completeTurn(move);

    //Expand list if needed
    if((int)moveList.size() < moveIndex+1)
      moveList.resize(moveIndex+1);
    moveList[moveIndex] = move;
    moveIndex++;
    activePla = ((moveIndex+4) % 2 == 0) ? GOLD : SILV;
  }

  if(moveIndex >= 0)
    moveList.resize(moveIndex);

  //Set appropriate player
  if(b.pieceCounts[GOLD][0] == 0 && b.pieceCounts[SILV][0] == 0)
  	b.setPlaStep(moveIndex == -1 ? SILV : GOLD,0);
  else if(b.pieceCounts[SILV][0] == 0)
  	b.setPlaStep(moveIndex == 0 ? GOLD : SILV,0);
  else
  	b.setPlaStep(GOLD,0);

  //Done updating board
  b.refreshStartHash();

  //Verify legality
  pla_t winner = NPLA;
  Board copy = b;
  for(int i = 0; i<(int)moveList.size(); i++)
  {
  	//Game should not be over yet
  	if(winner != NPLA)
  	{
    	cout << "ArimaaIO: Game continues after a player has won: " << writePlaTurn(i) << endl;
    	//cout << str << endl;

    	//Limit the damage
    	moveList.resize(i);
    	break;
  	}

  	//Move must be legal
    bool suc = copy.makeMoveLegal(moveList[i]);
    if(!suc)
    {
    	cout << "ArimaaIO: Illegal move " << writePlaTurn(i) << " : " << writeMove(copy,moveList[i]) << endl;
    	//cout << str << endl;

    	//Limit the damage
    	moveList.resize(i);
    	break;
    }

    //Check winning conditions
		pla_t pla = copy.player;
		pla_t opp = OPP(pla);
		if(copy.isGoal(opp)) winner = opp;
		else if(copy.isGoal(pla)) winner = pla;
		else if(copy.isRabbitless(pla)) winner = opp;
		else if(copy.isRabbitless(opp)) winner = pla;
		else if(copy.noMoves(pla)) winner = opp;
  }

  map<string,string> keyValues = readKeyValues(str);

  return GameRecord(b,moveList,winner,keyValues);
}

vector<GameRecord> ArimaaIO::readMovesFile(const string& moveFile)
{
	return readMovesFile(moveFile.c_str());
}

vector<GameRecord> ArimaaIO::readMovesFile(const char* moveFile)
{
  ifstream in;
  if(!openFile(in,moveFile))
  	Global::fatalError(string("ArimaaIO: could not open file: ") + moveFile);

  string str;
  vector<GameRecord> records;
  while(getline(in,str,';'))
  {
    if(str.find_first_not_of(" \n\t\r") == string::npos)
      continue;
    records.push_back(readMoves(unescapeGameStateString(str)));
  }
  in.close();
  return records;
}

GameRecord ArimaaIO::readMovesFile(const string& moveFile, int idx)
{
	return readMovesFile(moveFile.c_str(), idx);
}

GameRecord ArimaaIO::readMovesFile(const char* moveFile, int idx)
{
  if(idx < 0)
  	Global::fatalError(string("ArimaaIO: idx is negative: ") + Global::intToString(idx));

  ifstream in;
  if(!openFile(in,moveFile))
  	Global::fatalError(string("ArimaaIO: could not open file: ") + moveFile);

  string str;
  GameRecord record;
  int i = 0;
  while(getline(in,str,';'))
  {
    if(str.find_first_not_of(" \n\t\r") == string::npos)
      continue;
    if(i < idx)
    {i++; continue;}
    record = readMoves(unescapeGameStateString(str));
    i++;
    break;
  }
  if(i <= idx)
  	Global::fatalError(string("ArimaaIO: could not find idx ") + Global::intToString(idx) + " in file: " + moveFile);

  in.close();
  return record;
}


//GAME STATE-----------------------------------------------------------------

map<string,string> ArimaaIO::readGameState(istream& in)
{
  map<string,string> properties;
  string str;

  //Iterate line by line until we run out:
  while(getline(in,str))
  {
    //Locate the '=' that divides the key and value
    size_t eqI = str.find('=');
    if(eqI == string::npos)
    {continue;}

    //Split!
    string prop = str.substr(0,eqI);
    string value = str.substr(eqI+1);

    //Unescape, and done!
    value = unescapeGameStateString(value);
    properties[prop] = value;
  }
  return properties;
}

map<string,string> ArimaaIO::readGameStateFile(const char* fileName)
{
  ifstream in;
  if(!openFile(in,fileName))
  	Global::fatalError("ArimaaIO: could not open gamestate file!");
  map<string,string> m = readGameState(in);
  in.close();
  return m;
}

TimeControl ArimaaIO::getTCFromGameState(map<string,string> gameState)
{
  TimeControl tc = TimeControl();

  //Parse all the constant properties
  if(gameState.find("tcmove") != gameState.end()     && isNumber(gameState["tcmove"]))     tc.perMove =     parseNumber(gameState["tcmove"]);
  if(gameState.find("tcpercent") != gameState.end()  && isNumber(gameState["tcpercent"]))  tc.percent =     parseNumber(gameState["tcpercent"]);
  if(gameState.find("tcmax") != gameState.end()      && isNumber(gameState["tcmax"]))      tc.reserveMax =  parseNumber(gameState["tcmax"]);
  if(gameState.find("tcgame") != gameState.end()     && isNumber(gameState["tcgame"]))     tc.gameCurrent = parseNumber(gameState["tcgame"]);
  if(gameState.find("tctotal") != gameState.end()    && isNumber(gameState["tctotal"]))    tc.gameMax =     parseNumber(gameState["tctotal"]);
  if(gameState.find("tcturntime") != gameState.end() && isNumber(gameState["tcturntime"])) tc.perMoveMax =  parseNumber(gameState["tcturntime"]);
  if(tc.gameMax == 0) tc.gameMax = 86400;
  if(tc.perMoveMax == 0) tc.perMoveMax = 86400;

  //Find who the current player to move is, and parse out the reserve for that side
  string side = gameState["s"];
  if(side == string("w") || side == string("g"))
  {
    if(gameState.find("tcwreserve") != gameState.end() && isNumber(gameState["tcwreserve"]))
    	tc.reserveCurrent = parseNumber(gameState["tcwreserve"]);
    if(gameState.find("tcgreserve") != gameState.end() && isNumber(gameState["tcgreserve"]))
    	tc.reserveCurrent = parseNumber(gameState["tcgreserve"]);

    if(gameState.find("wused") != gameState.end() && isNumber(gameState["wused"]))
      tc.alreadyUsed = parseNumber(gameState["wused"]);
    if(gameState.find("gused") != gameState.end() && isNumber(gameState["gused"]))
      tc.alreadyUsed = parseNumber(gameState["gused"]);
  }
  else if(side == string("b") || side == string("s"))
  {
    if(gameState.find("tcbreserve") != gameState.end() && isNumber(gameState["tcbreserve"]))
    	tc.reserveCurrent = parseNumber(gameState["tcbreserve"]);
    if(gameState.find("tcsreserve") != gameState.end() && isNumber(gameState["tcsreserve"]))
    	tc.reserveCurrent = parseNumber(gameState["tcsreserve"]);

    if(gameState.find("bused") != gameState.end() && isNumber(gameState["bused"]))
      tc.alreadyUsed = parseNumber(gameState["bused"]);
    if(gameState.find("sused") != gameState.end() && isNumber(gameState["sused"]))
      tc.alreadyUsed = parseNumber(gameState["sused"]);
  }
  else
  {
  	Global::fatalError(string("ArimaaIO: unknown side:") + side);
  }

  if(tc.perMove < 0)
    Global::fatalError("ArimaaIO: negative per-move in time control");

  return tc;
}

//EXPERIMENTAL--------------------------------------------------------------------

vector<hash_t> ArimaaIO::readHashListFile(const char* file)
{
  ifstream in;
  if(!openFile(in,file))
  	Global::fatalError("ArimaaIO: could not open gamestate file!");

  vector<hash_t> hashes;
  string str;
  while(getline(in,str))
  {
  	size_t commentPos = str.find_first_of('#');
  	if(commentPos != string::npos)
  		str = str.substr(0,commentPos);
  	if(str.length() == 0)
  		continue;
  	hashes.push_back(Global::stringToUInt64(str));
  }
  return hashes;
}

vector<hash_t> ArimaaIO::readHashListFile(const string& file)
{
	return readHashListFile(file.c_str());
}

//OTHER--------------------------------------------------------------

uint64_t ArimaaIO::readMem(const string& str)
{
  if(str.size() < 2)
    Global::fatalError("ArimaaIO: Could not parse amount of memory: " + str);

  size_t end = str.size()-1;
  size_t snd = str.size()-2;

  string numericPart;
  int shiftFactor;
  if     (str.find_first_of("K") == end)  {shiftFactor = 10; numericPart = str.substr(0,end); }
  else if(str.find_first_of("KB") == snd) {shiftFactor = 10; numericPart = str.substr(0,snd); }
  else if(str.find_first_of("M") == end)  {shiftFactor = 20; numericPart = str.substr(0,end); }
  else if(str.find_first_of("MB") == snd) {shiftFactor = 20; numericPart = str.substr(0,snd); }
  else if(str.find_first_of("G") == end)  {shiftFactor = 30; numericPart = str.substr(0,end); }
  else if(str.find_first_of("GB") == snd) {shiftFactor = 30; numericPart = str.substr(0,snd); }
  else if(str.find_first_of("T") == end)  {shiftFactor = 40; numericPart = str.substr(0,end); }
  else if(str.find_first_of("TB") == snd) {shiftFactor = 40; numericPart = str.substr(0,snd); }
  else if(str.find_first_of("P") == end)  {shiftFactor = 50; numericPart = str.substr(0,end); }
  else if(str.find_first_of("PB") == snd) {shiftFactor = 50; numericPart = str.substr(0,snd); }
  else if(str.find_first_of("B") == end)  {shiftFactor = 0;  numericPart = str.substr(0,end); }
  else                                    {shiftFactor = 0;  numericPart = str; }

  if(!isNumber(numericPart))
    Global::fatalError("ArimaaIO: Could not parse amount of memory: " + str);
  uint64_t mem = 0;
  istringstream in(numericPart);
  in >> mem;
  if(in.bad())
    Global::fatalError("ArimaaIO: Could not parse amount of memory: " + str);

  for(int i = 0; i<shiftFactor; i++)
  {
    uint64_t newMem = mem << 1;
    if(newMem < mem)
      Global::fatalError("ArimaaIO: Could not parse amount of memory (too large): " + str);
    mem = newMem;
  }
  return mem;
}

uint64_t ArimaaIO::readMem(const char* str)
{
  return readMem(string(str));
}


//HELPERS-------------------------------------------------------------------------

static string unescapeGameStateString(const string& str)
{
  //Walk along the string, unescaping characters
	string result = str;
  for(int i = 0; i<(int)result.size()-1; i++)
  {
    if     (result[i] == '\\' && result[i+1] == 'n') result.replace(i,2,"\n");
    else if(result[i] == '\\' && result[i+1] == 't') result.replace(i,2,"\t");
    else if(result[i] == '\\' && result[i+1] == '\\') result.replace(i,2,"\\");
    else if(result[i] == '%' && result[i+1] == '1' && i < (int)result.size()-2 && result[i+2] == '3' ) result.replace(i,3,"\n");
  }
  return result;
}

static bool isNumber(const string& str)
{
  return isNumber(str,0,str.size());
}

static int parseNumber(const string& str)
{
  return parseNumber(str,0,str.size());
}

static bool isDigit(char c)
{
  return c >= '0' && c <= '9';
}

static bool isAlpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool isNumber(const string& str, int start, int end)
{
	//Too long to fit in integer for sure?
	if(end-start > 9)
		return false;

  int size = str.size();
  int64_t value = 0;
  for(int i = start; i<end && i<size; i++)
  {
    char c = str[i];
    if(!isDigit(c))
    	return false;
    value = value*10 + (c-'0');
  }

  if((value & 0x7FFFFFFFLL) != value)
  	return false;

  return true;
}

static int parseNumber(const string& str, int start, int end)
{
	//Too long to fit in integer for sure?
	if(end-start > 9)
		return 0;

	int size = str.size();
	int64_t value = 0;
  for(int i = start; i<end && i<size; i++)
  {
    char c = str[i];
    if(!isDigit(c))
    	return 0;
    value = value*10 + (c-'0');
  }

  if((value & 0x7FFFFFFFLL) != value)
  	return 0;

  return (int)value;
}

static string stripComments(const string& str)
{
	if(str.find_first_of('#') == string::npos)
		return str;

  //Turn str into a stream so we can go line by line
  istringstream in(str);
  string line;
  string result;

  while(getline(in,line))
  {
  	if(line.find('#') != string::npos)
  		line = line.substr(0,line.find_first_of('#'));

  	result += line + "\n";
  }
  return result;
}

static bool isInChars(char c, const char* str)
{
	for(int i = 0; str[i] != 0; i++)
		if(c == str[i])
			return true;
	return false;
}




//EXPERIMENTAL---------------------------------------------------------------
