
/*
 * rand.h
 * Author: davidwu
 *
 * Basic class for random number generation.
 * Not threadsafe!
 * Note: Signed integer functions might not be portable to other architectures.
 *
 * Combines:
 * 2 multiply-with-carry generators (32 bits state),
 * xorshift generator (32 bits state)
 * linear congruential generator (32 bits state),
 * complementary multiply-with-carry generator (2048+32 bits state)
 */

#ifndef RAND_H
#define RAND_H

#include <cassert>
#include <cmath>
#include <stdint.h>

using namespace std;

class Rand
{
	public:
	static Rand rand;

	private:
	static const int CMWC_LEN = 64;
	static const int CMWC_MASK = CMWC_LEN-1;
	static const uint64_t CMWC_MULT = 987651206ULL;

	uint32_t z; //MWC1
	uint32_t w; //MWC2
	uint32_t j; //Xorshift
	uint32_t c; //LCONG

	uint32_t q[CMWC_LEN]; //Long CMWC
	uint32_t q_c;         //Long CMWC constant add
	int q_idx;            //Long CMWC index

	bool hasGaussian;
	double storedGaussian;

	uint64_t initSeed;
	uint64_t numCalls;

	public:

	//Initializes according to system time and some other unique junk
	Rand();

	//Intializes according to the provided seed
	Rand(int seed);

	//Intializes according to the provided seed
	Rand(uint64_t seed);

	//Reinitialize according to system time and some other unique junk
	void init();

	//Reinitialize according to the provided seed
	void init(uint64_t seed);

	~Rand();

	//HELPERS---------------------------------------------
	private:

	//Generate a random value without the CMWC
	uint32_t nextUIntRaw();

	public:

	//SEED RETRIEVAL---------------------------------------

	uint64_t getSeed();

	uint64_t getNumCalls();

	//UNSIGNED INTEGER-------------------------------------

	//Returns a random integer in [0,2^32)
	uint32_t nextUInt();

	//Returns a random integer in [0,n)
	uint32_t nextUInt(uint32_t n);

	//Returns a random integer according to the given frequency distribution
	uint32_t nextUInt(const int* freq, int n);

	//SIGNED INTEGER---------------------------------------

	//Returns a random integer in [-2^31,2^31)
	int32_t nextInt();

	//Returns a random integer in [a,b]
	int32_t nextInt(int32_t a, int32_t b);

	//64-BIT INTEGER--------------------------------------

	//Returns a random integer in [0,2^64)
	uint64_t nextUInt64();

	//Returns a random integer in [0,n)
	uint64_t nextUInt64(uint64_t n);

	//DOUBLE----------------------------------------------

	//Returns a random double in [0,1)
	double nextDouble();

	//Returns a random double in [0,n). Note: Rarely, it may be possible for n to occur.
	double nextDouble(double n);

	//Returns a random double in [a,b). Note: Rarely, it may be possible for b to occur.
	double nextDouble(double a, double b);

	//Returns a normally distributed double with mean 0 stdev 1
	double nextGaussian();
};

inline uint64_t Rand::getSeed()
{
	return initSeed;
}

inline uint64_t Rand::getNumCalls()
{
	return numCalls;
}

inline uint32_t Rand::nextUIntRaw()
{
	z = 36969 * (z & 0xFFFF) + (z >> 16);
	w = 18000 * (w & 0xFFFF) + (w >> 16);
	j ^= j << 17;
	j ^= j >> 13;
	j ^= j << 5;
	c = 69069*c + 0x1372B295;

	return (((z<<16)+w)^c)+j;
}

inline uint32_t Rand::nextUInt()
{
	q_idx = (q_idx + 1) & CMWC_MASK;
	uint64_t t = CMWC_MULT * q[q_idx] + q_c;
	q_c = (uint32_t)(t >> 32);
	uint32_t x = (uint32_t)t+q_c;
	if(x < q_c) {x++; q_c++;}
	if(x+1 == 0) {x = 0; q_c++;}

	uint32_t m = 0xFFFFFFFEU;
	uint32_t result = m-x;
	q[q_idx] = result;
	return result + nextUIntRaw();
}


inline uint32_t Rand::nextUInt(uint32_t n)
{
	assert(n > 0);
	uint32_t bits, val;
  do {
  	bits = nextUInt();
  	val = bits % n;
  } while((uint32_t)(bits - val + (n-1)) < (uint32_t)(bits - val)); //If adding (n-1) overflows, no good.
  return val;
}

inline int32_t Rand::nextInt()
{
	return (int32_t)nextUInt();
}

inline int32_t Rand::nextInt(int32_t a, int32_t b)
{
	assert(b >= a);
	uint32_t max = (uint32_t)b-(uint32_t)a+(uint32_t)1;
	if(max == 0)
		return (int32_t)(nextUInt());
	else
		return (int32_t)(nextUInt(max)+(uint32_t)a);
}

inline uint64_t Rand::nextUInt64()
{
	return ((uint64_t)nextUInt()) | ((uint64_t)nextUInt() << 32);
}

inline uint64_t Rand::nextUInt64(uint64_t n)
{
	assert(n > 0);
	uint64_t bits, val;
  do {
  	bits = nextUInt64();
  	val = bits % n;
  } while((uint64_t)(bits - val + (n-1)) < (uint64_t)(bits - val)); //If adding (n-1) overflows, no good.
  return val;
}

inline uint32_t Rand::nextUInt(const int* freq, int n)
{
	int sum = 0;
	for(int i = 0; i<n; i++)
	{sum += freq[i];}

	int r = nextUInt(sum);
	for(int i = 0; i<n; i++)
	{
		r -= freq[i];
		if(r < 0)
		{return i;}
	}
	//Should not get to here
	assert(false);
	return n-1;
}

inline double Rand::nextDouble()
{
  double x;
  do
  {
  	uint64_t bits = nextUInt64() & ((1ULL << 53)-1ULL);
    x = (double)bits / (double)(1ULL << 53);
  }
  //Catch loss of precision of long --> double conversion
  while (!(x>=0.0 && x<1.0));

  return x;
}

inline double Rand::nextDouble(double n)
{
	assert(n >= 0);
	return nextDouble()*n;
}

inline double Rand::nextDouble(double a, double b)
{
	assert(b >= a);
	return a+nextDouble(b-a);
}

inline double Rand::nextGaussian()
{
	if(hasGaussian)
	{
		hasGaussian = false;
		return storedGaussian;
	}
	else
	{
		double v1, v2, s;
		do
		{
			v1 = 2 * nextDouble(-1.0,1.0);
			v2 = 2 * nextDouble(-1.0,1.0);
			s = v1 * v1 + v2 * v2;
		} while (s >= 1 || s == 0);

		double multiplier = sqrt(-2 * log(s)/s);
		storedGaussian = v2 * multiplier;
		hasGaussian = true;
		return v1 * multiplier;
	}
}


#endif
