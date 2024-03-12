#include "pch.h"
#include "bitmap.h"

const Bitmap Bitmap::BMPZEROS = Bitmap();
const Bitmap Bitmap::BMPONES = Bitmap(0xFFFFFFFFFFFFFFFFULL);
const Bitmap Bitmap::BMPTRAPS = Bitmap(0x0000240000240000ULL);

const Bitmap Bitmap::BMPX0 = Bitmap(0x0101010101010101ULL);
const Bitmap Bitmap::BMPX1 = Bitmap(0x0202020202020202ULL);
const Bitmap Bitmap::BMPX2 = Bitmap(0x0404040404040404ULL);
const Bitmap Bitmap::BMPX3 = Bitmap(0x0808080808080808ULL);
const Bitmap Bitmap::BMPX4 = Bitmap(0x1010101010101010ULL);
const Bitmap Bitmap::BMPX5 = Bitmap(0x2020202020202020ULL);
const Bitmap Bitmap::BMPX6 = Bitmap(0x4040404040404040ULL);
const Bitmap Bitmap::BMPX7 = Bitmap(0x8080808080808080ULL);

const Bitmap Bitmap::BMPY0 = Bitmap(0x00000000000000FFULL);
const Bitmap Bitmap::BMPY1 = Bitmap(0x000000000000FF00ULL);
const Bitmap Bitmap::BMPY2 = Bitmap(0x0000000000FF0000ULL);
const Bitmap Bitmap::BMPY3 = Bitmap(0x00000000FF000000ULL);
const Bitmap Bitmap::BMPY4 = Bitmap(0x000000FF00000000ULL);
const Bitmap Bitmap::BMPY5 = Bitmap(0x0000FF0000000000ULL);
const Bitmap Bitmap::BMPY6 = Bitmap(0x00FF000000000000ULL);
const Bitmap Bitmap::BMPY7 = Bitmap(0xFF00000000000000ULL);
