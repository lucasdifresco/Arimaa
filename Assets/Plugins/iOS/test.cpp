
/*
 * test.cpp
 * Author: davidwu
 */
#include "pch.h"

#include <iostream>
#include <cmath>
#include <cstdlib>
#include "global.h"
#include "rand.h"
#include "bitmap.h"
#include "board.h"
#include "boardmovegen.h"
#include "setup.h"
#include "tests.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;

static void testBmpBits(uint64_t seed);
static void testBmpGetSet(uint64_t seed);
static void testBmpBoolOps(uint64_t seed);
static void testBmpShifty(uint64_t seed);
static void testBoardStepConsistency(uint64_t seed);
static void testBoardMoveGenConsistency(uint64_t seed);

void Tests::runBasicTests(uint64_t seed)
{
	Rand rand(seed + Global::getHash("runTests"));

	//Test bitmaps----------------------------------
	cout << "----Testing Bitmaps----" << endl;

	cout << "Constructing and retrieving bits" << endl;
	for(int i = 0; i<5000; i++)
	{testBmpBits(rand.nextUInt64());}

	cout << "Seting and retrieving bits" << endl;
	for(int i = 0; i<5000; i++)
	{testBmpGetSet(rand.nextUInt64());}

	cout << "Bitmap boolean operations" << endl;
	for(int i = 0; i<5000; i++)
	{testBmpBoolOps(rand.nextUInt64());}

	cout << "Bitmap shifty" << endl;
	for(int i = 0; i<5000; i++)
	{testBmpShifty(rand.nextUInt64());}

	cout << "----Testing Board----" << endl;

	cout << "Step consistency" << endl;
	for(int i = 0; i<50; i++)
	{testBoardStepConsistency(rand.nextUInt64());}

	cout << "Movegen consistency" << endl;
	for(int i = 0; i<400; i++)
	{testBoardMoveGenConsistency(rand.nextUInt64());}

	cout << "Testing complete!" << endl;
}

static void testBoardMoveGenConsistency(uint64_t seed)
{
	Rand rand(seed);

	move_t* mv = new move_t[512];
	int* hm = new int[512];

	Board b = Board();
	Setup::setupRandom(b,seed);

	for(int i = 0; i<400; i++)
	{
		int stepNum = BoardMoveGen::genSteps(b,b.player,mv);
		int pushNum = 0;
		if(b.step < 3)
		{pushNum += BoardMoveGen::genPushPulls(b,b.player,mv+stepNum);}

		if((stepNum != 0) != BoardMoveGen::canSteps(b,b.player))
		{cout << "Steps != canSteps" << endl; cout << b << endl; exit(0);}
		if(b.step < 3 && (pushNum != 0) != BoardMoveGen::canPushPulls(b,b.player))
		{cout << "Pushpulls != canPushPull" << endl; cout << b << endl; exit(0);}

		int num = stepNum+pushNum;

		if(b.step != 0 && b.posCurrentHash != b.posStartHash)
		{*(mv+num) = Board::getMove(PASSSTEP); *(hm+num) = 0; num++;}

		if(num == 0)
		{return;}

		for(int j = 0; j<num; j++)
		{
			Board copy = b;
			bool success = copy.makeMoveLegal(mv[j]);
			if(!success)
			{cout << "Illegal move generated: " << writeMove(b,mv[j]) << " " << b; exit(0);}
		}
		int r = rand.nextUInt(num);
		b.makeMove(mv[r]);
	}

	delete[] mv;
	delete[] hm;
}

static void testBoardStepConsistency(uint64_t seed)
{
	Rand rand(seed);

	Board b = Board();
	Setup::setupRandom(b,seed);

	if(!b.testConsistency(cout)) {exit(0);}
	for(int i = 0; i<1500; i++)
	{
		int s = rand.nextUInt(256);
		loc_t k0 = Board::K0INDEX[s];
		loc_t k1 = Board::K1INDEX[s];

		if(k0 == ERRORSQUARE)
		{continue;}

		if(b.owners[k0] != NPLA && b.isThawed(k0) && (b.pieces[k0] != RAB || Board::RABBITVALID[b.owners[k0]][s]) && b.owners[k1] == NPLA)
		{
			bool success = b.makeStepLegal(Board::STEPINDEX[k0][k1]);
			if(!success)
			{cout << "Illegal step: " << s << " = (" << k0 << "," << k1 << ")" << endl; cout << b; exit(0);}
		}

		if(!b.testConsistency(cout)) {exit(0);}
	}
}

static void testBmpBits(uint64_t seed)
{
	Rand rand(seed);
  uint64_t x = rand.nextUInt64();

	Bitmap b = Bitmap(x);
	Bitmap copy = b;
	bool set[64];

	int manualCount = 0;
	for(int i = 0; i<64; i++)
	{
		set[i] = (((x >> i) & 1ULL) == 1ULL);
		if(set[i])
			manualCount++;
	}

	if(manualCount != b.countBits())
	{
		cout << "manualCount = " << manualCount << endl;
		cout << "b.countBits() = " << b.countBits() << endl;
		copy.print(cout);
		exit(0);
	}

	if(manualCount != b.countBitsIterative())
	{
		cout << "manualCount = " << manualCount << endl;
		cout << "b.countBitsIterative() = " << b.countBitsIterative() << endl;
		copy.print(cout);
		exit(0);
	}

	int last = -1;
	while(b.hasBits())
	{
		unsigned char k = b.nextBit();
		if(k <= last || k > 63 || !set[k])
		{
			cout << "x = " << x << endl;
			cout << "k = " << (int)k << endl;
			copy.print(cout);
			exit(0);
		}
		last = k;
		set[k] = false;
	}

	for(int i = 0; i<64; i++)
	{
		if(set[i])
		{
			cout << "x = " << x << endl;
			cout << "k = " << i << endl;
			copy.print(cout);
			exit(0);
		}
	}
}

static void testBmpGetSet(uint64_t seed)
{
	Rand rand(seed);

	Bitmap b;
	bool set[64];

	for(int i = 0; i<64; i++)
	{set[i] = false;}

	for(int i = 0; i<400; i++)
	{
		if((rand.nextUInt() & 0x8000) == 0)
		{
			int k = rand.nextUInt(64);
			b.setOff(k);
			set[k] = false;
		}
		else
		{
			int k = rand.nextUInt(64);
			b.setOn(k);
			set[k] = true;
		}
	}

	for(int i = 0; i<400; i++)
	{
		int k = rand.nextUInt(64);
		b.toggle(k);
		set[k] = !set[k];
	}

	for(int i = 0; i<64; i++)
	{
		if(set[i] != b.isOne(i))
		{
			cout << "i = " << i << endl;
			cout << b << endl;
			exit(0);
		}
	}

	Bitmap copy = b;

	int last = -1;
	while(b.hasBits())
	{
		unsigned char k = b.nextBit();
		if(k <= last || k > 63 || !set[k])
		{
			cout << copy << endl;
			exit(0);
		}
		set[k] = false;
	}

	for(int i = 0; i<64; i++)
	{
		if(set[i])
		{
			cout << copy << endl;
			exit(0);
		}
	}
}

static void testBmpBoolOps(uint64_t seed)
{
	Rand rand(seed);

  uint64_t x = rand.nextUInt64();
  uint64_t y = rand.nextUInt64();

	Bitmap bx = Bitmap(x);
	Bitmap by = Bitmap(y);
	Bitmap band = Bitmap(x & y);
	Bitmap bor = Bitmap(x | y);
	Bitmap bnot = Bitmap(~x);
	Bitmap bxor = Bitmap(x ^ y);

	if(!(bx == bx))  {cout << "==" << endl; cout << bx << endl; exit(0);}
	if((bx == by) != (x == y))  {cout << "==" << endl; cout << bx << endl; cout << by << endl; exit(0);}
	if((bx != by) != (x != y))  {cout << "!=" << endl; cout << bx << endl; cout << by << endl; exit(0);}

	Bitmap temp;

	temp = bx;
	temp &= by;
	if(band != temp)  {cout << "and=" << endl; cout << bx << endl; cout << by << endl; cout << temp << endl; exit(0);}

	temp = bx;
	temp |= by;
	if(!(bor == temp))  {cout << "or=" << endl; cout << bx << endl; cout << by << endl; cout << temp << endl; exit(0);}

	temp = bx;
	temp ^= by;
	if(bxor != temp)  {cout << "xor=" << endl; cout << bx << endl; cout << by << endl; cout << temp << endl; exit(0);}

	temp = ~bx;
	if(!(bnot == temp))  {cout << "not" << endl; cout << bx << endl; cout << by << endl; cout << temp << endl; exit(0);}

	if(band != (bx & by))  {cout << "and" << endl; cout << bx << endl; cout << by << endl; exit(0);}
	if(bor != (bx | by))  {cout << "or" << endl; cout << bx << endl; cout << by << endl; exit(0);}
	if(bxor != (bx ^ by))  {cout << "xor" << endl; cout << bx << endl; cout << by << endl; exit(0);}

}

static void testBmpShifty(uint64_t seed)
{
	Rand rand(seed);

  uint64_t x = rand.nextUInt64();

	Bitmap orig = Bitmap(x);
	Bitmap bx;
	Bitmap cc;

	//SOUTH-----------------
	bx = Bitmap::shiftS(orig);
	for(int i = 56; i<64; i++)
	{if(bx.isOne(i)) {cout << "s\nx = " << x << endl; exit(0);}}

	cc = orig & ~Bitmap::BMPY0;
	while(cc.hasBits())
	{if(!bx.hasBits() || cc.nextBit()-8 != bx.nextBit()) {cout << "s\nx = " << x << endl; exit(0);}}
	if(bx.hasBits()) {cout << "s\nx = " << x << endl; exit(0);}

	//NORTH-----------------
	bx = Bitmap::shiftN(orig);
	for(int i = 0; i<8; i++)
	{if(bx.isOne(i)) {cout << "n\nx = " << x << endl; exit(0);}}

	cc = orig & ~Bitmap::BMPY7;
	while(cc.hasBits())
	{if(!bx.hasBits() || cc.nextBit()+8 != bx.nextBit()) {cout << "n\nx = " << x << endl; exit(0);}}
	if(bx.hasBits()) {cout << "n\nx = " << x << endl; exit(0);}

	//WEST-----------------
	bx = Bitmap::shiftW(orig);
	for(int i = 7; i<64; i+=8)
	{if(bx.isOne(i)) {cout << "w\nx = " << x << endl; exit(0);}}

	cc = orig & ~Bitmap::BMPX0;
	while(cc.hasBits())
	{if(!bx.hasBits() || cc.nextBit()-1 != bx.nextBit()) {cout << "w\nx = " << x << endl; exit(0);}}
	if(bx.hasBits()) {cout << "w\nx = " << x << endl; exit(0);}

	//EAST-----------------
	bx = Bitmap::shiftE(orig);
	for(int i = 0; i<64; i+=8)
	{if(bx.isOne(i)) {cout << "e\nx = " << x << endl; exit(0);}}

	cc = orig & ~Bitmap::BMPX7;
	while(cc.hasBits())
	{if(!bx.hasBits() || cc.nextBit()+1 != bx.nextBit()) {cout << "e\nx = " << x << endl; exit(0);}}
	if(bx.hasBits()) {cout << "e\nx = " << x << endl; exit(0);}

	//ADJ------------------
	bx = Bitmap::adj(orig);
	Bitmap c;
	c |= Bitmap::shiftS(orig);
	c |= Bitmap::shiftW(orig);
	c |= Bitmap::shiftE(orig);
	c |= Bitmap::shiftN(orig);

	if(bx != c) {cout << "a\nx = " << x << endl; exit(0);}

}


