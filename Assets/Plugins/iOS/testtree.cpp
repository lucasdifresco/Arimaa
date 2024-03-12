
/*
 * testtree.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <cstdlib>
#include <iostream>
#include "rand.h"
#include "bitmap.h"
#include "board.h"
#include "boardmovegen.h"
#include "boardtrees.h"
#include "gamerecord.h"
#include "tests.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;


void Tests::testElimTree(const vector<GameRecord>& games)
{
	cout << "Testing elim tree" << endl;
	//TODO automate this
	/*
	for(int i = 0; i<(int)boards.size(); i++)
	{
		cout << boards[i] << endl;
		cout << "P0 Can Elim in 2: " << BoardTrees::canElim(boards[i],SILV,2) << endl;
		cout << "P0 Can Elim in 3: " << BoardTrees::canElim(boards[i],SILV,3) << endl;
		cout << "P0 Can Elim in 4: " << BoardTrees::canElim(boards[i],SILV,4) << endl;
		cout << "P1 Can Elim in 2: " << BoardTrees::canElim(boards[i],GOLD,2) << endl;
		cout << "P1 Can Elim in 3: " << BoardTrees::canElim(boards[i],GOLD,3) << endl;
		cout << "P1 Can Elim in 4: " << BoardTrees::canElim(boards[i],GOLD,4) << endl;
		boards[i].testConsistency(cout);
	}*/
}


static move_t mv[4][512];
static int hm[4][512];

static move_t capmv[512];
static int caphm[512];

static void printCapMoves(ostream& out, const Board& b, int num);
static bool equalPos(const Board& b, const Board& c);
static int goalDist(Board& b, pla_t pla, int rdepth, int cdepth, bool trust, int trustDepth);
static bool canCapType(Board& b, pla_t pla, piece_t piece, int rdepth, bool trust, int trustDepth);
static bool canCapType(Board& b, pla_t pla, piece_t piece, int origCount, int rdepth, int cdepth, bool trust, int trustDepth);

static bool testGoals(const Board& b, int i, int trustDepth, int testDepth);
static void testGoals(const Board& b, int i, int trustDepth, int testDepth, int numRandomPerturbations, Rand& gameRand);

void Tests::testGoalTree(const vector<GameRecord>& games, int trustDepth, int testDepth, int numRandomPerturbations, uint64_t seed)
{
	cout << "Testing goal tree, " << numRandomPerturbations << " perturbations per position" << endl;
	cout << "Using seed " << seed << " for each game (adding some hash of the starting board and the move list)" << endl;

	int numGames = games.size();
	for(int i = 0; i<numGames; i++)
	{
		if(i % 100 == 0)
			cout << "Game " << i << endl;

		Board b = games[i].board;
		vector<move_t> moves = games[i].moves;

		//Make a random generator just for this game
		Rand gameRand(seed + b.sitCurrentHash + moves.size());

		testGoals(b,i,trustDepth,testDepth,numRandomPerturbations,gameRand);

		bool good = true;
		int numMoves = moves.size();
		for(int m = 0; m<numMoves && good; m++)
		{
			move_t move = moves[m];
			int numSteps = Board::numStepsInMove(move);
			for(int s = 0; s<numSteps && good; s++)
			{
				step_t step = Board::getStep(move,s);
				bool succ = b.makeStepLegal(step);
				if(!succ) {cout << "Illegal step " << writeStep(b,step) << endl; cout << b << endl; good = false; break;}

				testGoals(b,i,trustDepth,testDepth,numRandomPerturbations,gameRand);
			}
		}
	}
}

static void testGoals(const Board& b, int i, int trustDepth, int testDepth, int numRandomPerturbations, Rand& gameRand)
{
	bool succ = testGoals(b,i,trustDepth,testDepth);

	if(!succ)
	{
		cout << "GameSeed " << gameRand.getSeed() << endl;
		exit(0);
	}

	//Try some random pertrubations
	for(int rpt = 0; rpt < numRandomPerturbations; rpt++)
	{
		Board copy = b;

		int numRandomPieces = gameRand.nextInt(2,8);
		for(int j = 0; j<numRandomPieces; j++)
			copy.setPiece(gameRand.nextUInt(64),gameRand.nextUInt(2),gameRand.nextInt(RAB,ELE));

		//Resolve captures
		for(int trapIndex = 0; trapIndex<4; trapIndex++)
		{
			loc_t kt = Board::TRAPLOCS[trapIndex];
			if(copy.owners[kt] != NPLA && copy.trapGuardCounts[copy.owners[kt]][trapIndex] == 0)
				copy.setPiece(kt,NPLA,EMP);
		}

		//Remove rabbits that are winning the game
		for(int x = 0; x<8; x++)
		{
			if(b.owners[x] == SILV && b.pieces[x] == RAB) copy.setPiece(x,NPLA,EMP);
			if(b.owners[x+56] == GOLD && b.pieces[x+56] == RAB) copy.setPiece(x+56,NPLA,EMP);
		}

		succ = testGoals(copy,i,trustDepth,testDepth);
		if(!succ)
		{
			cout << "GameSeed " << gameRand.getSeed() << endl;
			exit(0);
		}
	}
}

static bool testGoals(const Board& b, int i, int trustDepth, int testDepth)
{
	Board copy = b;
	copy.setPlaStep(0,0);
	int searchDist = goalDist(copy,0,testDepth,0,true,trustDepth); if(searchDist > testDepth) {searchDist = 5;}
	int treeDist = BoardTrees::goalDist(copy,0,testDepth);
	unsigned int gmove = copy.goalTreeMove;
	int gmovelen = Board::numStepsInMove(gmove);
	if(!equalPos(b,copy))
	{cout << i << " " << "Tree modified position! p0 " << endl; cout << b; cout << copy; return false;}
	if(searchDist != treeDist)
	{cout << i << " " << "Search p0: " << searchDist << "  Treep0: " << treeDist << endl; cout << b; return false;}
	if(searchDist <= testDepth) //If there was indeed a goal
	{
		if(searchDist == 0)
		{
			if(gmove != ERRORMOVE) {cout << i << " " << "goalTreeMove is not ERRORMOVE but searchDist = 0! p0 " << searchDist << " " << treeDist << " " << writeMove(b,gmove) << endl; cout << b; return false;}
		}
		else
		{
			Board copycopy = copy;
			bool suc = copycopy.makeMoveLegal(gmove);
			if(!suc)
			{cout << i << " " << "goalTreeMove is illegal! p0 " << searchDist << " " << treeDist << " " << writeMove(b,gmove) << endl; cout << b; return false;}
			if((gmovelen <= 0 && searchDist > 0) || goalDist(copycopy,0,searchDist-gmovelen,0,false,trustDepth) != searchDist-gmovelen)
			{cout << i << " " << "goalTreeMove doesn't goal! p0 " << searchDist << " " << treeDist << " " << writeMove(b,gmove) << endl; cout << b; return false;}
		}
	}

	copy = b;
	copy.setPlaStep(1,0);
	searchDist = goalDist(copy,1,testDepth,0,true,trustDepth); if(searchDist > testDepth) {searchDist = 5;}
	treeDist = BoardTrees::goalDist(copy,1,testDepth);
	gmove = copy.goalTreeMove;
	gmovelen = Board::numStepsInMove(gmove);
	if(!equalPos(b,copy))
	{cout << i << " " << "Tree modified position! p1 " << endl; cout << b; cout << copy; return false;}
	if(searchDist != treeDist)
	{cout << i << " " << "Search p1: " << searchDist << "  Treep1: " << treeDist << endl; cout << b; return false;}
	if(searchDist <= testDepth) //If there was indeed a goal
	{
		if(searchDist == 0)
		{
			if(gmove != ERRORMOVE) {cout << i << " " << "goalTreeMove is not ERRORMOVE but searchDist = 0! p1 " << searchDist << " " << treeDist << " " << writeMove(b,gmove) << endl; cout << b; return false;}
		}
		else
		{
			Board copycopy = copy;
			bool suc = copycopy.makeMoveLegal(gmove);
			if(!suc)
			{cout << i << " " << "goalTreeMove is illegal! p1 " << searchDist << " " << treeDist << " " << writeMove(b,gmove) << endl; cout << b; return false;}
			if((gmovelen <= 0 && searchDist > 0) || goalDist(copycopy,1,searchDist-gmovelen,0,false,trustDepth) != searchDist-gmovelen)
			{cout << i << " " << "goalTreeMove doesn't goal! p1 " << searchDist << " " << treeDist << " " << writeMove(b,gmove) << endl; cout << b; return false;}
		}
	}
	return true;
}

void Tests::testCapTree(const vector<GameRecord>& games, int trustDepth, int testDepth)
{
	cout << "Testing cap tree" << endl;
	int numGames = games.size();
	for(int i = 0; i<numGames; i++)
	{
		if(i % 100 == 0)
			cout << "Game " << i << endl;

		Board b = games[i].board;
		vector<move_t> moves = games[i].moves;
		bool good = true;
		int numMoves = moves.size();
		for(int m = 0; m<numMoves && good; m++)
		{
			move_t move = moves[m];
			int numSteps = Board::numStepsInMove(move);
			for(int s = 0; s<numSteps && good; s++)
			{
				step_t step = Board::getStep(move,s);
				bool succ = b.makeStepLegal(step);
				if(!succ) {cout << i << " " << "Illegal step " << writeStep(b,step) << endl; cout << b << endl; good = false; break;}

				for(int pla = 0; pla <= 1; pla++)
				{
					Board copy = b;
					copy.setPlaStep(pla,0);
					int num = BoardTrees::genCaps(copy,pla,testDepth,capmv,caphm);
					if(!equalPos(b,copy))
					{cout << i << " " << "Gen Tree modified position!" << pla << endl; cout << b; cout << copy; exit(0);}

					if(num < 0)
					{cout << i << " " << pla << "Num = " << num << endl; cout << b; cout << copy; exit(0);}


					bool canCapTree = BoardTrees::canCaps(copy,pla,testDepth);
					if(!equalPos(b,copy))
					{cout << i << " " << "Can Tree modified position!" << pla << endl; cout << b; cout << copy; exit(0);}

					bool canCap = canCapType(copy,pla,0,testDepth,true,trustDepth);

					if(canCapTree != (num > 0))
					{
						cout << i << " " << pla << "canCapTree = " << canCapTree << " " << num << endl;
						printCapMoves(cout, b, num);
						cout << b;
						exit(0);
					}

					if(canCap != (num > 0))
					{
						cout << i << " " << pla << "canCapSearch = " << canCap << " " << num << endl;
						printCapMoves(cout, b, num);
						cout << b;
						exit(0);
					}

					for(int piece = RAB; piece <= ELE; piece++)
					{
						bool canCapTypeSearch = canCapType(copy,pla,piece,testDepth,true,trustDepth);
						bool canCapTypeTree = false;
						for(int j = 0; j<num; j++)
						{
							if(caphm[j] == piece)
							{canCapTypeTree = true; break;}
						}

						if(canCapTypeSearch != canCapTypeTree)
						{
							cout << i << " " << pla << "canCapSearch" << piece << " = " << canCapTypeSearch << endl;
							printCapMoves(cout, b, num);
							cout << b;
							exit(0);
						}
					}
				}
			}
		}
	}
}

static void printCapMoves(ostream& out, const Board& b, int num)
{
	for(int i = 0; i<num; i++)
		out << writeMove(b,capmv[i]);
}

static bool equalPos(const Board& b, const Board& c)
{
	for(int i = 0; i<64; i++)
	{
		if(b.owners[i] != c.owners[i] || b.pieces[i] != c.pieces[i])
		{return false;}
	}
	return true;
}

static int goalDist(Board& b, pla_t pla, int rdepth, int cdepth, bool trust, int trustDepth)
{
	if(b.isGoal(pla))
	{return cdepth;}

	if(rdepth <= 0)
	{return cdepth+1;}

	//Trust the goal tree
	if(trust && trustDepth >= 1 && rdepth <= trustDepth)
	{
		int dist = cdepth + BoardTrees::goalDist(b,pla,rdepth);
		return dist > 5 ? 5 : dist;
	}

	if(rdepth <= 4)
	{
		Bitmap rabbits = b.pieceMaps[pla][RAB];
		Bitmap all = b.pieceMaps[pla][0] | b.pieceMaps[OPP(pla)][0];
		Bitmap allBlock;
		if(pla == 0)
		{
			allBlock = Bitmap::shiftN(all);
			allBlock = allBlock | Bitmap::shiftN(allBlock);
			allBlock = allBlock | Bitmap::shiftN(allBlock);
		}
		else
		{
			allBlock = Bitmap::shiftS(all);
			allBlock = allBlock | Bitmap::shiftS(allBlock);
			allBlock = allBlock | Bitmap::shiftS(allBlock);
		}

		Bitmap uBRabbits = rabbits & ~allBlock;
		Bitmap uFRabbits = rabbits & ~b.frozenMap;

		Bitmap uBuFRabbits = uBRabbits & uFRabbits;

		if(!(Board::PGOALMASKS[rdepth][pla] & uBuFRabbits).hasBits()
		&& !(Board::PGOALMASKS[rdepth-1][pla] & (uBRabbits | uFRabbits)).hasBits()
		&& (rdepth < 2 || !(Board::PGOALMASKS[rdepth-2][pla] & rabbits).hasBits()))
		{return cdepth+rdepth+1;}
	}

	int num = 0;

	num += BoardMoveGen::genSteps(b,pla,mv[cdepth]+num);
	if(rdepth > 1)
	{num += BoardMoveGen::genPushPulls(b,pla,mv[cdepth]+num);}

	if(num == 0)
	{return cdepth+rdepth+1;}

	int best = cdepth+rdepth+1;

	for(int j = 0; j<num; j++)
	{
		Board copy = b;
		bool success = copy.makeMoveLegal(mv[cdepth][j]);
		if(!success)
		{
			cout << "Illegal move generated: " << endl;
			cout << writeMove(b,mv[cdepth][j]) << endl;
			cout << b;
			for(int k = 0; k<num; k++)
				cout << writeMove(b,mv[cdepth][k]) << endl;
			exit(0);
		}

		int ns = (((copy.step - b.step) + 3) & 0x3) + 1;
		int val = goalDist(copy, pla, best-cdepth-ns-1, cdepth+ns, trust,trustDepth);

		if(val < best)
		{
			best = val;
			if(best <= cdepth+1)
			{return best;}
		}
	}

	return best;
}

static bool canCapType(Board& b, pla_t pla, piece_t piece, int rdepth, bool trust, int trustDepth)
{
  pla_t opp = OPP(pla);
	int origCount = b.pieceMaps[opp][piece].countBits();
	int cdepth = 0;

	return canCapType(b,pla,piece,origCount,rdepth,cdepth, trust, trustDepth);
}

//Piece = 0 for any type
static bool canCapType(Board& b, pla_t pla, piece_t piece, int origCount, int rdepth, int cdepth, bool trust, int trustDepth)
{
  pla_t opp = OPP(pla);

	if(b.pieceMaps[opp][piece].countBits() < origCount)
	{return true;}

	if(rdepth <= 1)
	{return false;}

	if(trust && trustDepth >= 1 && rdepth <= trustDepth)
	{
		if(piece == 0)
		{return BoardTrees::canCaps(b,pla,rdepth);}
		else
		{
			int num = BoardTrees::genCaps(b,pla,rdepth,mv[cdepth],hm[cdepth]);
			for(int i = 0; i<num; i++)
			{
				if(hm[cdepth][i] == piece)
				{return true;}
			}
			return false;
		}
	}

	if(rdepth <= 3)
	{
		//All opponent pieces
		Bitmap oppMap = b.pieceMaps[opp][0];

		//All squares S,W,E,N of an opponent piece
		Bitmap oppMapS = Bitmap::shiftS(oppMap);
		Bitmap oppMapW = Bitmap::shiftW(oppMap);
		Bitmap oppMapE = Bitmap::shiftE(oppMap);
		Bitmap oppMapN = Bitmap::shiftN(oppMap);

		//All squares guarded by an odd number of opponent pieces
		Bitmap oppOneDef = oppMapS ^ oppMapW ^ oppMapE ^ oppMapN;

		//All squares with exactly one opponent guard (if 3 either S and W, or E and N are both opps)
		oppOneDef &= ~(oppMapS & oppMapW);
		oppOneDef &= ~(oppMapE & oppMapN);

		//All traps with exactly one opponent guard
		oppOneDef &= Bitmap::BMPTRAPS;

		//All opponents of the relvant type adjacent to an unsafe trap
		oppOneDef |= Bitmap::adj(oppOneDef);
		oppOneDef &= b.pieceMaps[opp][piece];

		if(!oppOneDef.hasBits())
		{return false;}
	}

	int num = 0;

	if(rdepth >= 3)
	{num += BoardMoveGen::genSteps(b,pla,mv[cdepth]+num);}

	num += BoardMoveGen::genPushPulls(b,pla,mv[cdepth]+num);

	if(num == 0)
	{return false;}

	for(int j = 0; j<num; j++)
	{
		Board copy = b;
		bool success = copy.makeMoveLegal(mv[cdepth][j]);
		if(!success)
		{cout << "Illegal move generated: " << writeMove(b,mv[cdepth][j]) << endl; cout << b; exit(0);}

		int ns = (((copy.step - b.step) + 3) & 0x3) + 1;
		bool suc = canCapType(copy, pla, piece, origCount, rdepth-ns, cdepth+ns, trust, trustDepth);

		if(suc)
		{return true;}
	}

	return false;
}
