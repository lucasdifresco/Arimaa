
/*
 * maingetmove.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdlib>
#include "global.h"
#include "rand.h"
#include "board.h"
#include "boardhistory.h"
#include "learner.h"
#include "search.h"
#include "searchparams.h"
#include "setup.h"
#include "arimaaio.h"
#include "command.h"
#include "init.h"
#include "main.h"

using namespace std;
using namespace ArimaaIO;

static const string USAGE_STRING = string(
""
"bot_sharp Usage:\n\
sharp <posFile> <moveFile> <stateFile> <-flags>  (use files)\n\
sharp <-flags>  (no files, specify board and time control in flags)\n\
For the first usage, posFile and moveFile are strictly optional.\n\
\n\
Flags are:\n\
-b B: override board position to B, where B is a position string (see below)\n\
-p P: override current player to P, where P is 1 if Gold, 0 if Silver\n\
-d D: limit maximum nominal search depth to D steps\n\
-t T: set maximum search time to T seconds, or 0 for unbounded time.\n\
-threads N: search using N parallel threads \n\
-seed S: use fixed seed S, rather than randomizing \n\
-hashmem H: use at most H memory for the main hashtable (ex: 512MB). \n\
            (memory usage may be a little more than this due to things \n\
            other than the main hashtable) \n\
\n\
Note that when using the -d flag, the bot will also respect any time control as well.\n\
To force a search of that depth, use -t 0 to make the time unbounded.\n\
\n\
Valid position string consists of:\n\
{RCDHME} - gold (1st player) pieces\n\
{rcdhme} - silver (2nd player) pieces\n\
{.,*xX}  - empty spaces\n\
Board should be listed from the northwest corner to the southeast corner in\n\
scanline order. Other non-whitespace characters will be ignored, and can\n\
optionally be used as line delimiters. For instance:\n\
rrrddrrr/r.c.mc.r/.h*..*h./...eE.../......../..*..*H./RMCH.C.R/RRRDDRRR\n\
");

static bool tryGetMove(int argc, const char* const *argv);

int MainFuncs::getMove(int argc, const char* const *argv)
{
	if(argc <= 1)
	{
		cout << USAGE_STRING << endl;
		return EXIT_SUCCESS;
	}
	else if(!tryGetMove(argc,argv))
	{
		cout << "Run this program with no arguments for usage" << endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static bool tryGetMove(int argc, const char* const *argv)
{
	string sfilename;
	bool bfilename = false;
	vector<string> args = Command::parseCommand(argc, argv);
	if(args.size() > 1)
	{
		sfilename = args[args.size()-1];
		bfilename = true;
	}

  string requiredFlags;
  string allowedFlags;
  string emptyFlags;
  string nonemptyFlags;
  if(Init::ARIMAA_DEV)
  {
    requiredFlags = string("");
    allowedFlags = string("b p t d s evalparams threads seed hashmem");
    emptyFlags = string("");
    nonemptyFlags = string("b p t d s evalparams threads seed hashmem");
  }
  else
  {
    requiredFlags = string("");
    allowedFlags = string("b p t d threads seed hashmem");
    emptyFlags = string("");
    nonemptyFlags = string("b p t d threads seed hashmem");
  }


	map<string,string> flags = Command::parseFlags(argc, argv, requiredFlags, allowedFlags, emptyFlags, nonemptyFlags);

	string sboard = map_try_get(flags,"b");
	bool bboard = map_contains(flags,"b");
	string splayer = map_try_get(flags,"p");
	bool bplayer = map_contains(flags,"p");
	string stime = map_try_get(flags,"t");
	bool btime = map_contains(flags,"t");
	string sdepth = map_try_get(flags,"d");
	bool bdepth = map_contains(flags,"d");
	string ssetup = map_try_get(flags,"s");
	bool bsetup = map_contains(flags,"s");

	string sevalparams = map_try_get(flags,"evalparams");
	bool bevalparams = map_contains(flags,"evalparams");
  string shashmem = map_try_get(flags,"hashmem");
  bool bhashmem = map_contains(flags,"hashmem");
  string sthreads = map_try_get(flags,"threads");
  bool bthreads = map_contains(flags,"threads");

	if(map_contains(flags,"seed"))
	{
		string seedString = flags["seed"];
		Init::init(Global::stringToUInt64(seedString),false);
	}

	//Process game state file?
	map<string,string> gameState;
	if(bfilename)
	{
		ifstream stateStream;
		stateStream.open(sfilename.c_str());
		if(!stateStream.good())
		{cout << "File " << sfilename << " not found" << endl; return false;}
		gameState = ArimaaIO::readGameState(stateStream);
	}

	//Parse board...
	if(!bboard && !bfilename)
	{cout << "No board state specified (no gamestate and no -b flag)" << endl; return false;}

	Board board;
	BoardHistory hist;
	//...from gamestate file
	if(bfilename)
	{
		GameRecord record = readMoves(gameState["moves"]);
		hist = BoardHistory(record);
		board = hist.turnBoard[hist.maxTurnNumber];
	}
	//...from command line
	if(bboard)
	{
		board = ArimaaIO::readBoard(sboard);
		hist.reset(board);
	}

	//Parse player
	if(bplayer)
	{
		if(splayer[0] == '0')
			board.setPlaStep(0,board.step);
		else if(splayer[0] == '1')
			board.setPlaStep(1,board.step);
		else
		{cout << "-p: Invalid player" << endl; return false;}
	}

	//Ensure player determined
	if(!bplayer && bboard)
	{cout << "No player to move specified" << endl; return false;}

	//Parse max depth
	int maxDepth = SearchParams::AUTO_DEPTH;
	if(bdepth)
	{
		istringstream in(sdepth);
		in >> maxDepth;
		if(in.fail())
		{cout << "-d: Invalid depth" << endl; return false;}
		if(maxDepth > SearchParams::AUTO_DEPTH)
		{cout << "-d: Depth greater than " << SearchParams::AUTO_DEPTH << endl; return false;}
	}

	//Parse time control
	bool maxTimeOnly = true;
	bool stopEarlyWhenLittleTime = false;
	double maxTime = SearchParams::MAX_SEARCH_TIME;
	TimeControl tc;
	if(btime)
	{
		istringstream in(stime);
		in >> maxTime;
		if(in.fail())
		{cout << "-t: Invalid time" << endl; return false;}
		if(maxTime > SearchParams::MAX_SEARCH_TIME)
		{cout << "-t: Time greater than " << SearchParams::MAX_SEARCH_TIME << endl; return false;}

		if(maxTime <= 0)
			maxTime = -1; //Infinite time
	}
	else if(bfilename)
	{
		tc = ArimaaIO::getTCFromGameState(gameState);
		stopEarlyWhenLittleTime = true;
		maxTimeOnly = false;
	}
	else
	{cout << "No time control specified" << endl; return false;}

	//Setup!
	if(board.pieceCounts[0][0] == 0 || board.pieceCounts[1][0] == 0)
	{
		int setupMode = 0;
		if(bsetup)
		{
			istringstream in(ssetup);
			in >> setupMode;
		}
		if(setupMode == 1)
			Setup::setupRatedRandom(board,Init::getSeed());
		else if(setupMode == 2)
			Setup::setupPartialRandom(board,Init::getSeed());
		else if(setupMode == 3)
			Setup::setupRandom(board,Init::getSeed());
		else
			Setup::setupNormal(board,Init::getSeed());

		cout << writePlacements(board,OPP(board.player)) << endl;
		return true;
	}

	//Build searcher---------------------------------------------------
	SearchParams params;
	if(!bboard)
	  params.setAvoidEarlyTrade(true,1000);
	params.setRandomize(true,20,Rand::rand.nextUInt64());
	params.stopEarlyWhenLittleTime = stopEarlyWhenLittleTime;

	if(bthreads)
	  params.numThreads = Global::stringToInt(sthreads);

	//Hash memory
	if(bhashmem)
	{
	  uint64_t hashMem = ArimaaIO::readMem(shashmem);
	  int exp = SearchHashTable::getExp(hashMem);
	  params.mainHashExp = exp;
	  if(params.fullMoveHashExp > exp-1)
	    params.fullMoveHashExp = max(exp-1,0);
	}

	//Bradley Terry Move ordering?
	BradleyTerry learner = BradleyTerry::inputFromDefault(MoveFeature::getArimaaFeatureSet());
  params.initRootMoveFeatures(learner);
  params.setRootFancyPrune(true);

  if(Init::ARIMAA_DEV)
  {
    if(bevalparams)
    {
      EvalParams evalParams;
      evalParams = EvalParams::inputFromFile(sevalparams);
      params.useEvalParams = true;
      params.evalParams = evalParams;
    }
  }
  else
  {
    params.useEvalParams = true;
    params.evalParams = EvalParams();
  }

	Searcher searcher(params);

	//Search!
	if(maxTimeOnly)
		searcher.searchID(board,hist,maxDepth,maxTime,false);
	else
		searcher.searchByTimeControl(board,hist,maxDepth,maxTime,tc,false);

	//Retrieve and print best move
	move_t bestMove = searcher.getMove();
	cout << writeMove(board,bestMove,false) << endl;

	//Output!
	ostringstream sout;
	sout << "Seed " << Init::getSeed() << " Threads " << params.numThreads << " Hashexp " << params.mainHashExp << endl;
	sout << searcher.stats << endl;
	sout << board << endl;
	string verboseData = sout.str();

	//To log file
	bool printToFile = true;
	if(printToFile)
	{
		string filename;
		if(bfilename)
		{
			filename += "g";
			filename += gameState["tableId"];
		}
		else
		{
			filename += "t";
			char str[100];
			sprintf(str, "%d", (int)time(NULL));
			filename += str;
		}

		{
      ofstream out;
      string fname = filename + ".log";
      out.open(fname.c_str(), ios::app);

      out << endl;
      out << gameState["plycount"];
      out << gameState["s"];
      out << endl;
      out << writeMove(board,bestMove,false) << endl;
      out << verboseData << endl;

      out.close();
		}

		//Warn if already used is > 10
		if(!maxTimeOnly && tc.alreadyUsed > 10)
		{
      ofstream out;
      string fname = filename + ".time";
      out.open(fname.c_str(), ios::app);

      out << endl;
      out << "WARNING ";
      out << gameState["plycount"];
      out << gameState["s"];
      out << ": large wused or bused: " << tc.alreadyUsed;
      out << endl;
      out.close();
		}

	}

	return true;

}



