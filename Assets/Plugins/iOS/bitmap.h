
/*
 * bitmap.h
 * Author: davidwu
 *
 * Basic 64-bit bitmap class for an Arimaa board.
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <iostream>
#include <stdint.h>
using namespace std;

static const int nextBitArr[64] =
{
	63, 30,  3, 32, 59, 14, 11, 33,
	60, 24, 50,  9, 55, 19, 21, 34,
	61, 29,  2, 53, 51, 23, 41, 18,
	56, 28,  1, 43, 46, 27,  0, 35,
	62, 31, 58,  4,  5, 49, 54,  6,
	15, 52, 12, 40,  7, 42, 45, 16,
	25, 57, 48, 13, 10, 39,  8, 44,
	20, 47, 38, 22, 17, 37, 36, 26
};

struct Bitmap
{
	public:
	uint64_t bits;

  static const Bitmap BMPZEROS;
  static const Bitmap BMPONES;
  static const Bitmap BMPTRAPS;

  //Columns
  static const Bitmap BMPX0;
  static const Bitmap BMPX1;
  static const Bitmap BMPX2;
  static const Bitmap BMPX3;
  static const Bitmap BMPX4;
  static const Bitmap BMPX5;
  static const Bitmap BMPX6;
  static const Bitmap BMPX7;

  //Rows
  static const Bitmap BMPY0;
  static const Bitmap BMPY1;
  static const Bitmap BMPY2;
  static const Bitmap BMPY3;
  static const Bitmap BMPY4;
  static const Bitmap BMPY5;
  static const Bitmap BMPY6;
  static const Bitmap BMPY7;

  inline Bitmap()
	:bits(0)
	{}

	inline Bitmap(uint64_t b)
	:bits(b)
	{}

	inline static Bitmap makeLoc(int k)
	{return Bitmap((uint64_t)1 << k);}

	inline bool operator==(const Bitmap b) const
	{return bits == b.bits;}

	inline bool operator!=(const Bitmap b) const
	{return bits != b.bits;}

	inline static Bitmap shiftS(const Bitmap b)
	{return Bitmap(b.bits >> 8);}

	inline static Bitmap shiftN(const Bitmap b)
	{return Bitmap(b.bits << 8);}

	inline static Bitmap shiftW(const Bitmap b)
	{return Bitmap((b.bits & (uint64_t)0xFEFEFEFEFEFEFEFEULL) >> 1);}

	inline static Bitmap shiftE(const Bitmap b)
	{return Bitmap((b.bits & (uint64_t)0x7F7F7F7F7F7F7F7FULL) << 1);}

	inline const Bitmap operator&(const Bitmap b) const
	{return Bitmap(bits & b.bits);}

	inline const Bitmap operator|(const Bitmap b) const
	{return Bitmap(bits | b.bits);}

	inline const Bitmap operator^(const Bitmap b) const
	{return Bitmap(bits ^ b.bits);}

	inline const Bitmap operator~() const
	{return Bitmap(~bits);}

	inline Bitmap& operator&=(const Bitmap b)
	{bits &= b.bits; return *this;}

	inline Bitmap& operator|=(const Bitmap b)
	{bits |= b.bits; return *this;}

	inline Bitmap& operator^=(const Bitmap b)
	{bits ^= b.bits; return *this;}

	inline static Bitmap adj(const Bitmap b)
	{
		return Bitmap(
			(b.bits >> 8) |
			(b.bits << 8) |
			((b.bits & (uint64_t)0xFEFEFEFEFEFEFEFEULL) >> 1) |
			((b.bits & (uint64_t)0x7F7F7F7F7F7F7F7FULL) << 1)
			);
	}

  inline static Bitmap adj2(const Bitmap b)
  {
    return Bitmap(
      (((b.bits & (uint64_t)0xFEFEFEFEFEFEFEFEULL) >> 1) & (
         (b.bits >> 8) |
         (b.bits << 8) |
         ((b.bits & (uint64_t)0x7F7F7F7F7F7F7F7FULL) << 1)
      )) |
      (((b.bits & (uint64_t)0x7F7F7F7F7F7F7F7FULL) << 1) & (
         (b.bits >> 8) |
         (b.bits << 8)
      )) |
      ((b.bits >> 8) & (b.bits << 8))
      );
  }

	inline bool isOne(int index) const
	{return (int)(bits >> index) & 0x1;}

	inline void toggle(int index)
	{bits = bits ^ ((uint64_t)1 << index);}

	inline void setOff(int index)
	{bits = bits & (~((uint64_t)1 << index));}

	inline void setOn(int index)
	{bits = bits | ((uint64_t)1 << index);}

	inline bool hasBits() const
	{return bits != 0;}

	inline bool isEmpty() const
	{return bits == 0;}

	inline bool isSingleBit() const
	{return bits != 0 && (bits & (bits-1)) == 0;}

	//Sets dest = dest | src, then src = 0
	//Warning: note what happens if src = dest.
	inline void transfer(int src, int dest)
	{
	  bits |= (uint64_t)((bits >> src) & 0x1) << dest;
	  bits &= ~((uint64_t)1 << src);
	}

	// http://chessprogramming.wikispaces.com/BitScan
	inline int nextBit()
	{
		//int bit =  __builtin_ctzll(bits);
	  uint64_t b = bits ^ (bits-1);
	  uint32_t folded = (int)(b ^ (b >> 32));
	  int bit = nextBitArr[(folded * 0x78291ACF) >> 26];
		bits &= (bits-1);
		return bit;
	}

	inline int countBitsIterative() const
	{
		uint64_t v = bits;
		int count = 0;
		while (v)
		{
			 count++;
			 v &= v - 1;
		}
		return count;
	}

	inline int countBits() const //TODO this might be speedupable by switiching to a 64 bit version?
	{
		//return __builtin_popcountll(bits);
	  uint32_t v = (uint32_t)bits;
		v = v - ((v >> 1) & 0x55555555U);
		v = (v & 0x33333333U) + ((v >> 2) & 0x33333333U);
		uint32_t c = (v + (v >> 4)) & 0x0F0F0F0FU;

		uint32_t w = (uint32_t)(bits >> 32);
		w = w - ((w >> 1) & 0x55555555U);
		w = (w & 0x33333333U) + ((w >> 2) & 0x33333333U);
		uint32_t d = (w + (w >> 4)) & 0x0F0F0F0FU;

		return (int)(((c + d) * 0x01010101U) >> 24);
	}

	inline void print(ostream& out) const
	{
		for(int y = 7; y>=0; y--)
		{
			for(int x = 0; x<8; x++)
			{
				int k = y*8+x;
				out << (isOne(k) ? 1 : 0);
			}
			out << endl;
		}
	}

  inline friend ostream& operator<<(ostream& out, const Bitmap& map)
  {
    map.print(out);
    return out;
  }

};


#endif






