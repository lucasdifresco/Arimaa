/*
 * ufdist.h
 * Author: davidwu
 */

#ifndef UFDIST_H
#define UFDIST_H

#include "board.h"

namespace UFDist
{
  const int MAX_UF_DIST = 6;

  //Fill array with estimated distance to unfreeze each frozen or immo piece.
  //Includes pieces that are immo/blockaded but not technically frozen.
  void get(const Board& b, int ufDist[64]);
}


#endif /* UFDIST_H */
