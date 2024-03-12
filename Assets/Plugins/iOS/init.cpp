/*
 * init.cpp
 *
 *  Created on: May 9, 2011
 *      Author: davidwu
 */
#include "pch.h"

#include "global.h"
#include "rand.h"
#include "board.h"
#include "featuremove.h"
#include "evalparams.h"
#include "eval.h"
#include "init.h"

static bool initCalled = false;

bool Init::ARIMAA_DEV = false;

void Init::init(bool isDev)
{
	if(initCalled)
		cout << "INITIALIZING AGAIN!" << endl;
	initCalled = true;

	ARIMAA_DEV = isDev;
	Rand::rand.init();
	Board::initData();
	Eval::init();
	MoveFeature::initFeatureSet();
	MoveFeatureLite::initFeatureSet();
	EvalParams::init();
}

void Init::init(bool isDev, uint64_t seed, bool print)
{
	if(initCalled && print)
		cout << "INITIALIZING AGAIN!" << endl;
	initCalled = true;

  ARIMAA_DEV = isDev;

  Rand::rand.init(seed);
	Board::initData();
	Eval::init();
	MoveFeature::initFeatureSet();
	MoveFeatureLite::initFeatureSet();
	EvalParams::init();

}

uint64_t Init::getSeed()
{
	return Rand::rand.getSeed();
}
