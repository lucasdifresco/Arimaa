
/*
 * init.h
 * Author: davidwu
 */

#ifndef INIT_H_
#define INIT_H_

#include <stdint.h>

//Call to initialize/reinitialize the board, eval tables, etc, with a new random seed
namespace Init
{
  extern bool ARIMAA_DEV; //This should ONLY be used to affect the behavior of interfacey things, not of anything algorithmic

	void init(bool isDev);
	void init(bool isDev, uint64_t seed, bool print = true);
	uint64_t getSeed();
}

#endif /* INIT_H_ */
