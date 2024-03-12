
/*
 * boardtreeconst.h
 * Author: David Wu
 *
 * Useful macros for the capture and defense trees
 */

#ifndef BOARDTREECONST_H_
#define BOARDTREECONST_H_

#define HMVALUE(x) (x)
#define TS(x,y,a) b.tempStep((x),(y)); a; b.tempStep((y),(x));
#define TP(x,y,z,a) b.tempPP((x),(y),(z)); a; b.tempPP((z),(y),(x));
#define TSC(x,y,a) TempRecord xtmprc = b.tempStepC((x),(y)); a; b.undoTemp(xtmprc);
#define TPC(x,y,z,a) TempRecord xtmprc1 = b.tempStepC((y),(z)); TempRecord xtmprc2 = b.tempStepC((x),(y)); a; b.undoTemp(xtmprc2); b.undoTemp(xtmprc1);
#define TR(x,a) char xtmpow = b.owners[(x)]; char xtmppi = b.pieces[(x)]; b.owners[(x)] = 2; b.pieces[(x)] = 0; a; b.owners[(x)] = xtmpow; b.pieces[(x)] = xtmppi;

#define ISE(x) (b.owners[(x)] == 2)
#define ISP(x) (b.owners[(x)] == pla)
#define ISO(x) (b.owners[(x)] == opp)

#define GT(x,y) (b.pieces[(x)] > b.pieces[(y)])
#define GEQ(x,y) (b.pieces[(x)] >= b.pieces[(y)])
#define LEQ(x,y) (b.pieces[(x)] <= b.pieces[(y)])
#define LT(x,y) (b.pieces[(x)] < b.pieces[(y)])

#define ADDMOVEPP(m,h) *(mv++) = m; *(hm++) = h; num++;
#define ADDMOVE(m,h) mv[num] = m; hm[num] = h; num++;

#define RIF(x) if((x)) {return true;}

#define SETGM(x) b.goalTreeMove = Board::getMove x;
#define PREGM(x) b.goalTreeMove = Board::preConcatMove x;
#define PSTGM(x) b.goalTreeMove = Board::postConcatMove x;

#define RIFGM(x,y) if((x)) {b.goalTreeMove = Board::getMove y; return true;}

#define S -8
#define W -1
#define E +1
#define N +8
#define SS -16
#define WW -2
#define EE +2
#define NN +16
#define SW -9
#define SE -7
#define NW +7
#define NE +9
#define SSS -24
#define WWW -3
#define EEE +3
#define NNN +24
#define SSW -17
#define SSE -15
#define NNW +15
#define NNE +17
#define SWW -10
#define SEE -6
#define NWW +6
#define NEE +10

#define F -dy
#define G +dy
#define FW -1-dy
#define FE +1-dy
#define GW -1+dy
#define GE +1+dy
#define FF -dy-dy
#define GG +dy+dy
#define FWW -2-dy
#define FEE +2-dy
#define GWW -2+dy
#define GEE +2+dy
#define FFW -1-dy-dy
#define FFE +1-dy-dy
#define FWWW -3-dy
#define FEEE +3-dy
#define FFWW -2-dy-dy
#define FFEE +2-dy-dy
#define FFFW -1-dy-dy-dy
#define FFFE +1-dy-dy-dy
#define GGW -1+dy+dy
#define GGE +1+dy+dy
#define FFF -dy-dy-dy
#define GGG +dy+dy+dy
#define GWWW -3+dy
#define GEEE +3+dy
#define GGGW -1+dy+dy+dy
#define GGGE +1+dy+dy+dy
#define GGGG +dy+dy+dy+dy

#define MS +0
#define MW +64
#define ME +128
#define MN +192

#define MG +DYDIR[pla]
#define MF +DYDIR[1-pla]


#endif
