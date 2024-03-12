#include "pch.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <cstring>  // for strerror
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
#include "library.h"

static bool initialized = false;
static bool running = false;

static Searcher* searcher;

static void Initialize()
{
	if (initialized) return;
	initialized = true;

	bool isDev = false;
	Init::init(isDev);

    ArimaaFeatureSet arimaFeatureSet = MoveFeature::getArimaaFeatureSet();
    BradleyTerry learner = BradleyTerry::inputFromDefault(arimaFeatureSet);
	
    SearchParams params;
	params.setAvoidEarlyTrade(true, 1000);
	params.stopEarlyWhenLittleTime = false;
	params.numThreads = 1;
	params.mainHashExp = 19;
	params.fullMoveHashExp = 18;
	params.initRootMoveFeatures(learner);
	params.setRootFancyPrune(true);
	params.useEvalParams = true;
	params.evalParams = EvalParams();
	params.setRandomize(true, 20, Rand::rand.nextUInt64());
	
    searcher = new Searcher(params);
}
static const char* Move(const char* moveJString, int difficultyJ)
{
    if (!initialized) Initialize();
    if (running) return ("Already running!");
    running = true;

    const char* moveStringBuf = moveJString;
    if (moveStringBuf == NULL) return NULL; /* OutOfMemoryError */
    
    string moveString = string(moveStringBuf);
    
    int difficulty = difficultyJ;
    if (difficulty < 0) difficulty = 0;
    if (difficulty > 10) difficulty = 10;

    Board board;
    BoardHistory hist;
    GameRecord record = ArimaaIO::readMoves(moveString);
    hist = BoardHistory(record);
    board = hist.turnBoard[hist.maxTurnNumber];

    //Setup!
    if (board.pieceCounts[0][0] == 0 || board.pieceCounts[1][0] == 0)
    {
        int pr = 1;
        int rr = 1;
        int no = 1;
        switch (difficulty)
        {
        case 0:  pr = 3; rr = 1; no = 0; break;
        case 1:  pr = 1; rr = 1; no = 0; break;
        case 2:  pr = 1; rr = 3; no = 0; break;
        case 3:  pr = 1; rr = 4; no = 2; break;
        case 4:  pr = 0; rr = 3; no = 2; break;
        case 5:  pr = 0; rr = 1; no = 2; break;
        case 6:  pr = 0; rr = 1; no = 5; break;
        default: pr = 0; rr = 0; no = 1; break;
        }
        int rand = Rand::rand.nextUInt(pr + rr + no);
        if (rand < pr) Setup::setupPartialRandom(board, Rand::rand.nextInt());
        else if (rand < pr + rr) Setup::setupRatedRandom(board, Rand::rand.nextInt());
        else Setup::setupNormal(board, Rand::rand.nextInt());

        string placements = ArimaaIO::writePlacements(board, OPP(board.player));
        //placements = "\n" + placements;
        running = false;
        
        char* returnValue = new char[placements.length() + 1];
        strncpy(returnValue, placements.c_str(), placements.length() + 1);
        returnValue[placements.length()] = '\0';
        return returnValue;
    }

    int maxDepth = 5;
    double hardMinSecs = 0.2;
    double targetSecs = 0.4;
    double hardMaxSecs = 0.8;
    int randomStdev = 20;
    bool stupidPrune = false;
    bool fixedPrune = false;
    double pruneAllBut = 1.0;
    bool enableQSearch = true;
    double evalCapScaleFactor = 1.0;
    double tcScoreScaleFactor = 1.0;
    double rabYDistFactor = 1.0;
    double newRecklessScale = 0.0;
    bool enableGoalTree = true;
    bool otherReallyStupidThings = false;
    bool otherStupidThings = false;
    bool otherBadThings = false;
    bool otherSillyThings = false;

    switch (difficulty)
    {
    case 0: maxDepth = 4; hardMinSecs = 0.0; targetSecs = 0.01; hardMaxSecs = 1; randomStdev = 1400; stupidPrune = true; pruneAllBut = 0.35;
        enableQSearch = false; evalCapScaleFactor = -0.10; tcScoreScaleFactor = 0.0; rabYDistFactor = 0.0; newRecklessScale = 2.0;
        enableGoalTree = false;
        otherReallyStupidThings = true; break;
    case 1: maxDepth = 4; hardMinSecs = 0.0; targetSecs = 0.01; hardMaxSecs = 1; randomStdev = 1400; stupidPrune = true; pruneAllBut = 0.35;
        enableQSearch = false; evalCapScaleFactor = 0.0; tcScoreScaleFactor = 0.00; rabYDistFactor = 0.1; newRecklessScale = 2.0;
        enableGoalTree = false;
        otherStupidThings = true; break;
    case 2: maxDepth = 4; hardMinSecs = 0.0; targetSecs = 0.01; hardMaxSecs = 1; randomStdev = 1000; stupidPrune = true; pruneAllBut = 0.65;
        enableQSearch = false; evalCapScaleFactor = 0.0; tcScoreScaleFactor = 0.00; rabYDistFactor = 0.1; newRecklessScale = 2.0;
        enableGoalTree = Rand::rand.nextUInt(10) <= 7;
        otherBadThings = true; break;
    case 3: maxDepth = 4; hardMinSecs = 0.0; targetSecs = 0.01; hardMaxSecs = 1; randomStdev = 700; stupidPrune = true; pruneAllBut = 0.90;
        enableQSearch = false; evalCapScaleFactor = 0.01; tcScoreScaleFactor = 0.10; rabYDistFactor = 0.2; newRecklessScale = 1.5;
        otherSillyThings = true; break;
    case 4: maxDepth = 4; hardMinSecs = 0.0; targetSecs = 0.01; hardMaxSecs = 1; randomStdev = 480;
        enableQSearch = false; evalCapScaleFactor = 0.08 + (Rand::rand.nextUInt(5) >= 3 ? 0.1 : 0.0); tcScoreScaleFactor = 0.23; rabYDistFactor = 0.4; newRecklessScale = 1.2; break;
    case 5: maxDepth = 4; hardMinSecs = 0.0; targetSecs = 0.01; hardMaxSecs = 1; randomStdev = 380;
        enableQSearch = false; evalCapScaleFactor = 0.55; tcScoreScaleFactor = 0.29; rabYDistFactor = 0.6; newRecklessScale = 0.7; break;
    case 6: maxDepth = 4; hardMinSecs = 0.0; targetSecs = 0.01; hardMaxSecs = 1; randomStdev = 280;
        tcScoreScaleFactor = 0.45; rabYDistFactor = 0.8; break;
    case 7: maxDepth = 4; hardMinSecs = 0.0; targetSecs = 0.01; hardMaxSecs = 1; randomStdev = 140;
        tcScoreScaleFactor = 0.75; break;
    case 8: maxDepth = 5; hardMinSecs = 3.5; targetSecs = 7.00; hardMaxSecs = 14; randomStdev = 90; break;
    case 9: maxDepth = 8; hardMinSecs = 6.0; targetSecs = 12.00; hardMaxSecs = 24; randomStdev = 55; break;
    case 10: maxDepth = 24; hardMinSecs = 13.0; targetSecs = 20.00; hardMaxSecs = 30; randomStdev = 40; break;
    }

    if (stupidPrune) searcher->params.setStupidPrune(true, pruneAllBut);
    else if (fixedPrune) searcher->params.setRootFixedPrune(true, pruneAllBut, false);
    else searcher->params.setRootFancyPrune(true);

    searcher->params.qEnable = enableQSearch;
    searcher->params.enableGoalTree = enableGoalTree;
    searcher->params.evalParams = EvalParams();

    searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::RECKLESS_ADVANCE_SCALE)] = newRecklessScale;
    searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::CAP_SCORE_SCALE)] *= evalCapScaleFactor;
    searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::TC_SCORE_SCALE)] *= tcScoreScaleFactor;

    for (int i = 0; i < 8; i++)
    {
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::RAB_YDIST_VALUES)] *= rabYDistFactor;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::RAB_YDIST_TC_VALUES)] *= (tcScoreScaleFactor + rabYDistFactor) / 2.0;
    }

    if (otherReallyStupidThings)
    {
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::PIECE_SQUARE_SCALE)] = 0.35;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::TRAPDEF_SCORE_SCALE)] = -0.2;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::STRAT_SCORE_SCALE)] = 0.0;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::THREAT_SCORE_SCALE)] = -0.2;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::RABBIT_SCORE_SCALE)] = 0.0;
    }
    else if (otherStupidThings)
    {
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::PIECE_SQUARE_SCALE)] = 0.75;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::TRAPDEF_SCORE_SCALE)] = 0.2;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::STRAT_SCORE_SCALE)] = 0.0;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::THREAT_SCORE_SCALE)] = 0.4;
    }
    else if (otherBadThings)
    {
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::STRAT_SCORE_SCALE)] = 0.15;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::THREAT_SCORE_SCALE)] = 0.6;
    }
    else if (otherSillyThings)
    {
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::STRAT_SCORE_SCALE)] = 0.4;
        searcher->params.evalParams.featureWeights[searcher->params.evalParams.fset.get(EvalParams::THREAT_SCORE_SCALE)] = 0.7;
    }

    searcher->params.setRandomize(true, randomStdev, Rand::rand.nextUInt64());
    searcher->searchID(board, hist, maxDepth, hardMinSecs, targetSecs, hardMaxSecs, false);

    move_t bestMove = searcher->getMove();
    string moveStr = ArimaaIO::writeMove(board, bestMove, false);
    
    running = false;

    char* returnValue = new char[moveStr.length() + 1];
    strncpy(returnValue, moveStr.c_str(), moveStr.length() + 1);
    returnValue[moveStr.length()] = '\0';
    return returnValue;
}
static void Interrupt() { if(searcher != NULL) { searcher->interruptExternal(); } }

void InitBot() { Initialize(); }
void InterruptBot() { Interrupt(); }
void MoveBot(const char* state, int difficulty, char* move, int length) { strncpy(move, Move(state, difficulty), length); }
const char* MoveBot2(const char* state, int difficulty) { return Move(state, difficulty); }

int main() 
{

    const char* moveJString = "1g Ra1 Rb1 Rc1 Rd1 Ce1 Rf1 Dg1 Rh1 Da2 Cb2 Rc2 Hd2 He2 Ef2 Mg2 Rh2 \n1s ra7 hb7 hc7 ed7 de7 df7 mg7 ch7 ra8 rb8 cc8 rd8 re8 rf8 rg8 rh8 \n2g ";
    int difficultyJ = 1;

    const char* result = nullptr;

     result = Move(moveJString, difficultyJ);
     std::cout << "Result: " << result << std::endl;

     std::cin.get();

   return 0;
}