#include "pch.h"

#include <ctime>
#include "rand.h"
#include "timer.h"
#include "global.h"

Rand Rand::rand = Rand();


static int inits = 0;
Rand::Rand()
{
	init();
}

Rand::Rand(int seed)
{
	init(seed);
}

Rand::Rand(uint64_t seed)
{
	init(seed);
}

Rand::~Rand()
{

}

//TODO inits++ not threadsafe
void Rand::init()
{
	uint64_t seed0 = time(NULL);
	uint64_t seed1 = Global::jenkinsMix((~ time(NULL)) * 69069 + 12345, 0x7331BABEU ^ clock(), 0xFEEDCAFEU + (inits++) + (int32_t)ClockTimer::getPrecisionSystemTime());
	init(seed0 + (seed1 << 32));
}

void Rand::init(uint64_t seed)
{
	initSeed = seed;
	z = w = j = c = 0;

	while(z == 0 || w == 0 || j == 0)
	{
		seed = 2862933555777941757ULL*seed + 3037000493ULL;
		z = (uint32_t) (seed & 0x00000000FFFFFFFFULL);
		seed = 2862933555777941757ULL*seed + 3037000493ULL;
		w = (uint32_t) (seed & 0x00000000FFFFFFFFULL);
		seed = 2862933555777941757ULL*seed + 3037000493ULL;
		c = (uint32_t) (seed & 0x00000000FFFFFFFFULL);
		seed = 2862933555777941757ULL*seed + 3037000493ULL;
		j = (uint32_t) (seed & 0x00000000FFFFFFFFULL);
	}

	for(int i = 0; i<CMWC_LEN; i++)
	{
		seed = 2862933555777941757ULL*seed + 3037000493ULL;
		q[i] = (uint32_t)(seed % 0x00000000FFFFFFFFULL);
	}
	seed = 2862933555777941757ULL*seed + 3037000493ULL;
	q_c = (uint32_t)(seed % CMWC_MULT);
	q_idx = Rand::CMWC_LEN-1;

	for (int i = 0; i < 150; i++) { nextUInt(); }

	hasGaussian = false;
	storedGaussian = 0.0;
	numCalls = 0;
}

