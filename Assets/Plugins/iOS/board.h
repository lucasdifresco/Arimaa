
/*
 * board.h
 * Author: davidwu
 *
 * Welcome to the board!
 * General notes: copiable, stores current hash but not past state.
 * One player (gold, captial letters) starts at top (small indices) and his rabbits can move down (north, increase indices) and plays first.
 * Zero player (silver, lowercase lettors) starts at bottom (large indices) and his rabbits can move up (south, decrease indices) and plays second.
 * Directions and indices (S = -8, W = -1, E = 1, N = 8)
 *
 * MUST CALL initData() on Board to initalize arrays, before using this class!
 *
 * Steps - unsigned chars, encode one single step move
 * Moves - unsigned ints, encode up to 4 steps. Unused steps must be ERRORSTEP
 *
 * Special step values:
 * PASSSTEP 0 = pass step, no legality checking, but resets the step counter and toggles the player.
 * QPASSSTEP 1 = quiescence pass step, legal, but does exactly nothing
 * ERRORSTEP 255 = error step, use to mark illegality or as filler
 *
 * Special square values:
 * ERRORSQUARE 255 - indicates invalid board location
 *
 * Special move values:
 * PASSMOVE 0xFFFFFF00U = move consisting of an immediate pass step
 * QPASSMOVE 0xFFFFFF01U = move consisting of an immediate qpass step
 * ERRORMOVE 0xFFFFFFFFU = error move
 */

#ifndef BOARD_H
#define BOARD_H

#include <iostream>
#include <vector>
#include <string>
#include <stdint.h>
#include "bitmap.h"

using namespace std;


//TYPES---------------------------------------------------------------------------------------
typedef uint32_t loc_t;       //Location on the board [0-63] or ERRORSQUARE
typedef uint32_t step_t;      //A step on the board [0-255], but not all such values are valid. Includes PASSSTEP, ERRORSTEP
typedef uint32_t move_t;     //A move, 4 steps packed bitwise, least sig steps occur first, terminates with ERRORSTEPs if not all 4 steps used.
typedef int32_t pla_t;        //A player, 0(SILV), 1(GOLD), 2(NPLA)
typedef int32_t piece_t;      //A piece, 0(EMP),1(RAB),...,6(ELE)
typedef uint64_t hash_t;     //A 64 bit zobrist hash value

//DEFINES---------------------------------------------------------------------------------------

//Number of types of pieces (including empty)
#define NUMTYPES 7

//Possible pieces
#define EMP 0
#define RAB 1
#define CAT 2
#define DOG 3
#define HOR 4
#define CAM 5
#define ELE 6

//Possible players
#define GOLD 1
#define SILV 0
#define NPLA 2

//Macro to get inverse player. Use on GOLD or SILV only.
#define OPP(x) (1-(x))

//Special step and move values
#define PASSSTEP 0
#define ERRORSTEP 255
#define QPASSSTEP 1
#define ERRORSQUARE 255
#define PASSMOVE (0xFFFFFF00U)
#define ERRORMOVE (0xFFFFFFFFU)
#define QPASSMOVE (0xFFFFFF01U)

//On board predicates. CS1(x) is true if the location 1 step south of x is on the board, CW3(x) for 3 steps west, etc.
#define CS1(x) ((x) >= 8)
#define CW1(x) ((x)%8 > 0)
#define CE1(x) ((x)%8 < 7)
#define CN1(x) ((x) < 56)
#define CS2(x) ((x) >= 16)
#define CW2(x) ((x)%8 > 1)
#define CE2(x) ((x)%8 < 6)
#define CN2(x) ((x) < 48)
#define CS3(x) ((x) >= 24)
#define CW3(x) ((x)%8 > 2)
#define CE3(x) ((x)%8 < 5)
#define CN3(x) ((x) < 40)

//Same as above, except for advance/retreat, given the player
#define CA1(p,x) ((p) == 1 ? ((x) < 56) : ((x) >= 8))
#define CA2(p,x) ((p) == 1 ? ((x) < 48) : ((x) >= 16))
#define CA3(p,x) ((p) == 1 ? ((x) < 40) : ((x) >= 24))
#define CR1(p,x) ((p) != 1 ? ((x) < 56) : ((x) >= 8))
#define CR2(p,x) ((p) != 1 ? ((x) < 48) : ((x) >= 16))
#define CR3(p,x) ((p) != 1 ? ((x) < 40) : ((x) >= 24))


//TEMP MOVE AND UNDO CLASS-----------------------------------------------------------------------------
//Note! - after making a temp move, none of the bitmaps are updated!
//Nor are the hashes, step counter, etc updated. Do not trust isFrozen or isThawed.

class TempRecord
{
  public:
  loc_t capLoc;
  pla_t capOwner;
  piece_t capPiece;
  loc_t k0;
  loc_t k1;

  TempRecord()
  {}

  TempRecord(loc_t x, loc_t y)
  {
    k0 = x;
    k1 = y;
    capLoc = ERRORSQUARE;
    capOwner = NPLA;
    capPiece = EMP;
  }
};


//PRIMARY CLASS----------------------------------------------------------------------------------------

class Board
{
	public:

	//MAIN DATA--------------------------------------
	int8_t owners[64];    //Players who own each piece (0-1), and 2 if none.
	int8_t pieces[64];    //Piece powers (0-6)

	Bitmap pieceMaps[2][NUMTYPES];  //Bitmaps for each piece, 0 = all
	Bitmap frozenMap;               //Bitmap for frozen pieces

	int8_t player;   //Next player (0-1)
	int8_t step;      //Step phase (0-3)
	int turnNumber; //Number of turns finished in total up to this point (0 after setup)

	int8_t pieceCounts[2][NUMTYPES];  //Number of each type of piece, 0 = total
	int8_t trapGuardCounts[2][4]; //Number of pieces guarding each trap for each player

	//ZOBRIST HASHING--------------------------------
	hash_t posStartHash;		//Hash value for the situation (position + player to move + step) at the start of the move (last move if step = 0)
	hash_t posCurrentHash;	//Current hash value for the position (position only)
	hash_t sitCurrentHash;	//Current hash value for the situation (position + player to move + step)

	//GOAL TREE--------------------------------------
	move_t goalTreeMove;	//Move reported by the goal tree stored here

	//STATIC ARRAYS----------------------------------
	//TODO experiment with some of these being int or uint32_t rather than int8 for faster access?
	static hash_t HASHPIECE[2][NUMTYPES][64]; //Hash XOR'ed for piece on that position
	static hash_t HASHPLA[2];                 //Hash XOR'ed by player turn
	static hash_t HASHSTEP[4];                //Hash XOR'ed by step

	static const bool ISEDGE[64];			     //[loc]: Is this square at the edge?
	static const bool ISEDGE2[64];			   //[loc]: Is this square next to squares at the edge?
	static const int EDGEDIST[64];         //[loc]: How far is this from the edge?

	static const bool ISTRAP[64];			     //[loc]: Is this square a trap?
	static const uint8_t TRAPLOCS[4];        //[index]: What locations are traps?
	static const int TRAPINDEX[64];        //[loc]: What index is this trap? ERRORSQUARE for nontrap locations.
	static const uint8_t TRAPDEFLOCS[4][4];  //[index][index2]: What locations are defenses to traps?
  static const int PLATRAPINDICES[2][2]; //[pla][index]: What trapindices are traps on pla's side?
  static const uint8_t PLATRAPLOCS[2][2];  //[pla][index]: What locations are traps on pla's side?
	static const bool ISPLATRAP[4][2];     //[trapID][pla]: Is this a trap on pla's side?
  static const uint8_t CLOSEST_TRAP[64];   //[loc]: What is the closest trap to this location?
  static const int CLOSEST_TRAPINDEX[64];//[loc]: What is the closest trap's index to this location?
  static const int CLOSEST_TDIST[64];    //[loc]: What is the distance to the closest trap to this location?

	static const uint8_t TRAPBACKLOCS[4];    //[index]: What is the square behind this trap?
	static const uint8_t TRAPOUTSIDELOCS[4]; //[index]: What is the square on the outer side this trap?
	static const uint8_t TRAPINSIDELOCS[4];  //[index]: What is the square on the inner side this trap?
	static const uint8_t TRAPTOPLOCS[4];     //[index]: What is the square on the top of this trap?

  static const uint8_t ADJACENTTRAP[64];	 //[loc]: The trap location adjacent, else ERRORSQUARE if none
  static const uint8_t RAD2TRAP[64];       //[loc]: The trap location at exactly radius 2, else ERRORSQUARE if none

	static const int ADJOFFSETS[4];        //[index]: What offsets should be added to get the adjacent location in each direction?
  static const bool ADJOKAY[4][64];      //[index][loc]: Is it safe to take ADJOFFSETS[index] from the given square (not go off board)?
	static const int STEPOFFSETS[4];       //[index]: What offsets should be added to indicate a step in each direction?

	static const int STARTY[2];            //[pla]: Y coordinate of back starting row for player pla
	static const int GOALY[2];             //[pla]: Y coordinate of goal for player pla
	static const int GOALYINC[2];          //[pla]: Direction of advancement for player pla (+1 or -1)
	static const int GOALLOCINC[2];        //[pla]: Offset to add for advancement for player pla (+8 or -8)
  static const bool ISGOAL[2][64];		   //[pla][loc]: Is this square a goal for this player?
  static const int GOALYDIST[2][64];     //[pla][loc]: What is the vertical distance from this loc to goal?
	static const Bitmap PGOALMASKS[8][2];	 //[index][pla]: From what locations can rabbits attempt to goal in n steps, for each player?
	static Bitmap GOALMASKS[2];            //[pla]: Where are the goals?
	static Bitmap GPRUNEMASKS[2][64][5];   //[pla][loc][gdist]: What are the possible locs that matter to stop pla goal?

  static const int ADVANCEMENT[2][64];   //[pla][loc]: Array indicating roughly how 'advanced' a region is for a player
  static const int ADVANCE_OFFSET[2];    //[pla]: What is the offset corresponding to moving forward for this player?
	static const int CENTRALITY[64];       //[loc]: Distance from the center

	static uint8_t STEPINDEX[64][64];       //[startloc][endloc]: Look up the step index given the start and end locations.
	static uint8_t K0INDEX[256];	           //[step]: Look up the start square given the step index
	static uint8_t K1INDEX[256];	           //[step]: Look up the end square given the step index
	static bool RABBITVALID[2][256];       //[pla][step]: Can a rabbit make such a step?

	static bool ISADJACENT[64][64];        //[loc1][loc2]: Are the two given locations adjacent?
	static int MANHATTANDIST[64][64];      //[loc1][loc2]: What is the manhattan distance between two locations?
	static uint8_t SPIRAL[64][64];				   //[loc1][index]: For every loc, a list of every other loc by increasing manhattan distance
	static Bitmap RADIUS[16][64];			     //[rad][loc]: For each location, the set of locations at a given manhattan radius from it
	static Bitmap DISK[16][64];				     //[rad][loc]: For each location, the set of locations leq to a given manhattan radius from it

	static const int NUMSYMLOCS = 32;
	static const int NUMSYMDIR = 3;
	static const int SYMDIR_FRONT = 0;
	static const int SYMDIR_BACK = 1;
	static const int SYMDIR_SIDE = 2;
	static const uint8_t PSYMLOC[2][64];     //[pla][loc]: What is a symmetrical player-indepdendent index for this location?
	static const uint8_t SYMLOC[2][64];      //[pla][loc]: What is a left-right symmetrical player-indepdendent index for this location?
	static const int SYMTRAPINDEX[2][4];   //[pla][trapID]: What is a player-indepdendent index for this trap?
	static const int PSYMDIR[2][4];        //[pla][dir]: What is a player-independent direction?
	static const int SYMDIR[2][4];         //[pla][dir]: What is a player-independent left-right independent direction?

	//INITIALIZATION------------------------------------------------------------------

  /**
   * Initialize all the necessary data. Call this first before using this class!
   */
	static void initData();

	//CONSTRUCTOR---------------------------------------------------------------------

	/**
	 * Constructs an empty board.
	 */
	Board();

	//SETUP---------------------------------------------------------------------------

	//Note: To set up an initial position, call these methods as needed to create the board configuration and
	//then call refreshStartHash.

	/**
	 * Set the number of turns that have finished since setup.
	 * @param num - the number of turns
	 */
	void setTurnNumber(int num);

	/**
	 * Set the player and the step, updating the situation hash only. Does NOT update the position start hash.
	 * @param p - the player
   * @param s - the step
	 */
	void setPlaStep(pla_t p, char s);

	/**
	 * Set a piece, updating all the bitmaps and hashes, except the position start hash.
	 * Does NOT resolve trap captures - assumes that it is valid or that another piece will be set afterwards to defend.
	 * @param k - the location
	 * @param owner - the owner of the piece (or NPLA)
	 * @param piece - the piece (or EMP)
	 */
	void setPiece(loc_t k, pla_t owner, piece_t piece);

	/**
	 * Set the position start hash to the current position hash.
	 */
	void refreshStartHash();


	//MOVEMENT------------------------------------------------------------------------

	/**
	 * Perform a basic legality check. Checks that the step is actually a valid index, that the source has a piece,
	 * that the destination is empty. If owned by the current player, checks that the source piece is not a retreating
	 * rabbit or a frozen piece. Does NOT check pushpulling constraints.
	 * PASSSTEP and QPASSSTEP is always legal.
	 * @param s - the step
	 * @return true if legal, false otherwise
	 */
	bool isStepLegal(step_t s) const;

	/**
	 * Make a step, assuming that s is legal. Updates the hashes, step, and player appropriately, and resolves captures.
	 * If PASSSTEP, then skips all remaining steps in the turn, swaps the player, and updates everything appropriately.
	 * If QPASSSTEP, does nothing.
	 * @param s - the step
	 */
	void makeStep(step_t s);

  /**
   * Make a step, but performs the check isLegal first. If the step is not legal, then does nothing and returns false.
   * @param s - the step
   * @return true if legal, false if not
   */
  bool makeStepLegal(step_t s);

	/**
	 * Make a move, assuming that it is legal. Makes each step in the move in sequence, terminating if it hits ERRORSTEP or PASSSTEP.
	 * @param m - the move
	 */
	void makeMove(move_t m);

	/**
	 * Make a move, performing normal and pushpull legality checks.
	 * Does NOT check for board repetition or for the fact that a 4 step move must alter the board.
	 * Makes each step in the move in sequence, terminating if it hits ERRORSTEP or PASSSTEP or QPASSSTEP or an illegal move.
	 * Note that for illegal moves, all steps up to the point of illegality will still be made!
   * @param m - the move
   * @return true if legal, false if not
	 */
	bool makeMoveLegal(move_t m);

	/**
	 * Same as makeMoveLegal, but stops after making numSteps of the move. Will, however, go one farther if needed to complete
	 * a push, and ALSO if the step the second half of a legal pull.
	 */
	bool makeMoveLegalUpTo(move_t m, int numSteps);

	/**
	 * Same as makeMoveLegal, but makes a sequence of moves [start,end).
	 * All moves up to the point of illegality will still be made!
	 */
	bool makeMovesLegal(vector<move_t> moves, int start = 0, int end = 0x7FFFFFFF);

	private:
	//Makes a step, assuming that s is legal, and updates everything EXCEPT the freeze bitmap.
	void makeStepRaw(step_t s);
	//Updates the freeze bitmap.
	void recalculateFreezeMap();
	//Check if k is a trap, and resolve captures, recording the hash change in hashDiff
	bool checkCaps(loc_t k, hash_t &hashDiff);
	public:

	//STATIC MOVE/STEP RETRIEVAL------------------------------------------------------
	//Returns the move composed of the specified steps
	static inline move_t getMove(step_t s0)
	{return ((move_t)s0) | ((move_t)0xFFFFFF00U);}
	static inline move_t getMove(step_t s0, step_t s1)
	{return ((move_t)s0) | (((move_t)s1) << 8) | ((move_t)0xFFFF0000U);}
	static inline move_t getMove(step_t s0, step_t s1, step_t s2)
	{return ((move_t)s0) | (((move_t)s1) << 8) | (((move_t)s2) << 16) | ((move_t)0xFF000000U);}
	static inline move_t getMove(step_t s0, step_t s1, step_t s2, step_t s3)
	{return ((move_t)s0) | (((move_t)s1) << 8) | (((move_t)s2) << 16) | (((move_t)s3) << 24);}

	//Returns the move composed of the specified steps, followed by the move m.
  static inline move_t preConcatMove(step_t s0, move_t m)
  {return ((move_t)s0) | (m << 8);}
  static inline move_t preConcatMove(step_t s0, step_t s1, move_t m)
  {return ((move_t)s0) | (((move_t)s1) << 8) | (m << 16);}
  static inline move_t preConcatMove(step_t s0, step_t s1, step_t s2, move_t m)
  {return ((move_t)s0) | (((move_t)s1) << 8) | (((move_t)s2) << 16) | (m << 24);}

  //Returns the move m of the given length, except with the last steps overwritten by the given steps
  static inline move_t postConcatMove(move_t m, step_t s3)
  {return (m & (move_t)0x00FFFFFFU) | (((move_t)s3) << 24);}
  static inline move_t postConcatMove(move_t m, step_t s2, step_t s3)
  {return (m & (move_t)0x0000FFFFU) | (((move_t)s2) << 16) | (((move_t)s3) << 24);}
  static inline move_t postConcatMove(move_t m, step_t s1, step_t s2, step_t s3)
  {return (m & (move_t)0x000000FFU) | (((move_t)s1) <<  8) | (((move_t)s2) << 16) | (((move_t)s3) << 24);}

  //Returns the move m except with the steps starting with step <index> overwritten by m2. Behaviour is undefined if index is not an integer 0-3
  static inline move_t concatMoves(move_t m, move_t m2, int index)
  {m &= ((1U << (index*8))-1); m |= (m2 << (index*8)); return m;}

  //Returns the move m except with the steps at step <index> overwritten by s. Behaviour is undefined if index is not an integer 0-3
  static inline move_t setStep(move_t m, step_t s, int index)
  {m &= ~(0XFFU << (index*8)); m |= ((move_t)s << (index*8)); return m;}

  //Returns the move consisting of the first index steps of the move. Behaviour is undefined if index is not an integer 0-3
  static inline move_t getPreMove(move_t m, int index)
  {m = m | (~((1U<<(index*8))-1)); return m;}

  //Returns the move, deleting the steps before index. Behaviour is undefined if index is not an integer 0-3
  static inline move_t getPostMove(move_t m, int index)
  {m = m >> (index*8); m = m | (~((~0U)>>(index*8))); return m;}

  //Returns the indexth step in the move. Behaviour is undefined if index is not an integer 0-3
	static inline step_t getStep(move_t move, int index)
	{return (step_t)(move >> (index * 8)) & 0xFF;}

	//Returns the number of steps in the move
	static inline int numStepsInMove(move_t move)
	{
    if(move == (move_t)0xFFFFFFFFU) return 0;
    if((move & (move_t)0xFFFFFF00U) == (move_t)0xFFFFFF00U) return 1;
    if((move & (move_t)0xFFFF0000U) == (move_t)0xFFFF0000U) return 2;
    if((move & (move_t)0xFF000000U) == (move_t)0xFF000000U) return 3;
    return 4;
	}

	//Returns the number of steps in the move, taking into account that a pass completes the move to 4 steps
	static inline int numRealStepsInMove(move_t move)
	{
		int ns = numStepsInMove(move);
		if(ns <= 0)
			return 0;
		step_t lastStep = getStep(move,ns-1);
		if(lastStep == PASSSTEP || lastStep == QPASSSTEP)
			return 4;
    return ns;
	}

	//Checks if move1 is a prefix of move2
	static inline bool isPrefix(move_t move1, move_t move2)
	{
	  for(int i = 0; i<4; i++)
	  {
	    step_t step = getStep(move1,i);
	    if(step == ERRORSTEP) return true;
	    if(step != getStep(move2,i)) return false;
	  }
	  return true;
	}

	//Complete the move so that it finishes off a whole 4-step turn
	static inline move_t completeTurn(move_t move)
	{
		int ns = numStepsInMove(move);
		if(ns == 0)
			return PASSMOVE;
		if(ns == 4)
			return move;
		if(getStep(move,ns-1) == PASSSTEP)
			return move;
		if(getStep(move,ns-1) == QPASSSTEP)
			return concatMoves(move,PASSMOVE,ns-1);
		return concatMoves(move,PASSMOVE,ns);
	}

	//Strips any passstep or qpassstep that might occur in this move (and anything following it)
	static inline move_t stripPasses(move_t move)
	{
		//Prune pass/qpass
		for(int j = 0; j<4; j++)
		{
			step_t step = Board::getStep(move,j);
			if(step == ERRORSTEP)
				return move;
			if(step == PASSSTEP || step == QPASSSTEP)
				return Board::getPreMove(move,j);
		}
		return move;
	}

	//MISC--------------------------------------------------------------------------

	//Parses the move into steps in the format of up to 8 src and dest locations, indicating the accumulated movement.
	//The arrays src and dest must be at least 8 steps long, and this functions returns the number of steps
	//in the move.
	//When pieces become captured, their destinations are ERRORSQUARE
	//8 src and dest locations are necessary because we could have 4 moves and 4 captures in one turn in the worst case
	int getChanges(move_t move, loc_t* src, loc_t* dest) const;
	int getChanges(move_t move, loc_t* src, loc_t* dest, int8_t newTrapGuardCounts[2][4]) const;

	//Get the direction for the best approach to the dest
	static int getApproachDir(loc_t src, loc_t dest);

	//Get the direction for best retreat from dest. If equal, sets nothing. If only one retreat dir, sets nothing for dir2
	static void getRetreatDirs(loc_t src, loc_t dest, int& dir1, int& dir2);

	//END CONDITIONS------------------------------------------------------------------

	pla_t getWinner() const;

	inline bool isGoal(pla_t pla) const
	{return (pieceMaps[pla][RAB] & GOALMASKS[pla]).hasBits();}

	inline bool isRabbitless(pla_t pla) const
	{return pieceMaps[pla][RAB].isEmpty();}

	bool noMoves(pla_t pla) const;

	//MISC----------------------------------------------------------------------------

	//How many opposing pieces are stronger than this piece?
	int numStronger(pla_t pla, piece_t piece) const;

	//How many opposing pieces are the same strength as this piece?
	int numEqual(pla_t pla, piece_t piece) const;

	//How many opposing pieces are weaker than this piece?
	int numWeaker(pla_t pla, piece_t piece) const;

	//Returns the location of the elephant owned by pla, or ERRORSQUARE if none
	loc_t findElephant(pla_t pla) const;

	//Fill arrays with, for each piece type, the number of opp pieces stronger than it
	void initializeStronger(int pStronger[2][NUMTYPES]) const;

	//Fill arrays with, for each piece types, a bitmap of the opp pieces stronger than it
	void initializeStrongerMaps(Bitmap pStrongerMaps[2][NUMTYPES]) const;

	//Find the nearest opponent piece stronger than piece this from loc within maxRad, or ERRORSQUARE if none
	loc_t nearestDominator(pla_t pla, piece_t piece, loc_t loc, int maxRad) const;

	//OUTPUT-------------------------------------------------------------------------------

  friend ostream& operator<<(ostream& out, const Board& b);

  template <typename T>
  inline static void printInt64(ostream& out, T array[64])
	{
  	for(int y = 7; y >= 0; y--)
  	{
  		for(int x = 0; x < 8; x++)
  			out << (int)array[y*8+x] << " ";
  		out << endl;
  	}
	}

  //ERROR CHECKING-----------------------------------------------------------------------

  //Perform a few tests to try to detect if this board's data is inconsistent or corrupt
  bool testConsistency(ostream& out) const;

	//USEFUL CONDITIONS----------------------------
	//Is the square a trap?
	inline bool isTrap(loc_t k) const
	{return ISTRAP[k];}

	//Is the square against the edge?
	inline bool isEdge(loc_t k) const
	{return ISEDGE[k];}

	//Is the square against the edge?
	inline bool isEdge2(loc_t k) const
	{return ISEDGE2[k];}

	//Is square k empty?
	inline bool isEmpty(loc_t k) const
	{return owners[k] == NPLA;}

	//Is there a piece at square k?
	inline bool isPiece(loc_t k) const
	{return owners[k] != NPLA;}

	//Is the piece at square k a rabbit?
	inline bool isRabbit(loc_t k) const
	{return pieces[k] == RAB;}

	//Are the two locations adjacent?
	inline bool isAdjacent(loc_t k, loc_t j) const
	{return k < 64 && j < 64 && ISADJACENT[k][j];}

	//Is the piece at square k frozen?
	inline bool isFrozen(loc_t k) const
	{return frozenMap.isOne(k);}

	//Is the piece at square k unfrozen?
	inline bool isThawed(loc_t k) const
	{return !frozenMap.isOne(k);}

	//Is the piece at square k frozen?
	inline bool isFrozenC(loc_t k) const
	{
		pla_t pla = owners[k];
		pla_t opp = OPP(pla);

		return
		!(
			(CS1(k) && owners[k-8] == pla) ||
			(CW1(k) && owners[k-1] == pla) ||
			(CE1(k) && owners[k+1] == pla) ||
			(CN1(k) && owners[k+8] == pla)
		)
		&&
		(
			(CS1(k) && owners[k-8] == opp && pieces[k-8] > pieces[k]) ||
			(CW1(k) && owners[k-1] == opp && pieces[k-1] > pieces[k]) ||
			(CE1(k) && owners[k+1] == opp && pieces[k+1] > pieces[k]) ||
			(CN1(k) && owners[k+8] == opp && pieces[k+8] > pieces[k])
		);
	}

	//Is the piece at square k unfrozen?
	inline bool isThawedC(loc_t k) const
	{
	  pla_t pla = owners[k];
	  pla_t opp = OPP(pla);
		return
		(
			(CS1(k) && owners[k-8] == pla) ||
			(CW1(k) && owners[k-1] == pla) ||
			(CE1(k) && owners[k+1] == pla) ||
			(CN1(k) && owners[k+8] == pla)
		)
		||
		!(
			(CS1(k) && owners[k-8] == opp && pieces[k-8] > pieces[k]) ||
			(CW1(k) && owners[k-1] == opp && pieces[k-1] > pieces[k]) ||
			(CE1(k) && owners[k+1] == opp && pieces[k+1] > pieces[k]) ||
			(CN1(k) && owners[k+8] == opp && pieces[k+8] > pieces[k])
		);
	}

	//Is the piece at square k adjacent to any stronger opponent?
	inline bool isDominated(loc_t k) const
	{
	  pla_t pla = owners[k];
	  pla_t opp = OPP(pla);
		if(ISEDGE[k])
		{
			return
			(CS1(k) && owners[k-8] == opp && pieces[k-8] > pieces[k]) ||
			(CW1(k) && owners[k-1] == opp && pieces[k-1] > pieces[k]) ||
			(CE1(k) && owners[k+1] == opp && pieces[k+1] > pieces[k]) ||
			(CN1(k) && owners[k+8] == opp && pieces[k+8] > pieces[k]);
		}
		else
		{
			return
			(owners[k-8] == opp && pieces[k-8] > pieces[k]) ||
			(owners[k-1] == opp && pieces[k-1] > pieces[k]) ||
			(owners[k+1] == opp && pieces[k+1] > pieces[k]) ||
			(owners[k+8] == opp && pieces[k+8] > pieces[k]);
		}
	}

	//Is the piece at square k adjacent to any stronger unfrozen opponent?
  inline bool isDominatedByUF(loc_t k) const
  {
    pla_t pla = owners[k];
    pla_t opp = OPP(pla);
    return
    (CS1(k) && owners[k-8] == opp && pieces[k-8] > pieces[k] && isThawedC(k-8)) ||
    (CW1(k) && owners[k-1] == opp && pieces[k-1] > pieces[k] && isThawedC(k-1)) ||
    (CE1(k) && owners[k+1] == opp && pieces[k+1] > pieces[k] && isThawedC(k+1)) ||
    (CN1(k) && owners[k+8] == opp && pieces[k+8] > pieces[k] && isThawedC(k+8));
  }

  //Is the piece at square k adjacent to any weaker opponent piece?
	inline bool isDominating(loc_t k) const
	{
	  pla_t pla = owners[k];
	  pla_t opp = OPP(pla);
    if(ISEDGE[k])
    {
      return
      (CS1(k) && owners[k-8] == opp && pieces[k-8] < pieces[k]) ||
      (CW1(k) && owners[k-1] == opp && pieces[k-1] < pieces[k]) ||
      (CE1(k) && owners[k+1] == opp && pieces[k+1] < pieces[k]) ||
      (CN1(k) && owners[k+8] == opp && pieces[k+8] < pieces[k]);
    }
    else
    {
      return
      (owners[k-8] == opp && pieces[k-8] < pieces[k]) ||
      (owners[k-1] == opp && pieces[k-1] < pieces[k]) ||
      (owners[k+1] == opp && pieces[k+1] < pieces[k]) ||
      (owners[k+8] == opp && pieces[k+8] < pieces[k]);
    }
	}

	//Is the square k adjacent to any friendly piece?
	inline bool isGuarded(pla_t pla, loc_t k) const
	{
		if(ISEDGE[k])
		{
			return
			(CS1(k) && owners[k-8] == pla) ||
			(CW1(k) && owners[k-1] == pla) ||
			(CE1(k) && owners[k+1] == pla) ||
			(CN1(k) && owners[k+8] == pla);
		}
		else
		{
			return
			(owners[k-8] == pla) ||
			(owners[k-1] == pla) ||
			(owners[k+1] == pla) ||
			(owners[k+8] == pla);
		}
	}

	//Is the square k adjacent to 2 or more friendly pieces?
	inline bool isGuarded2(pla_t pla, loc_t k) const
	{
		if(ISEDGE[k])
		{
			return
			(CS1(k) && owners[k-8] == pla) +
			(CW1(k) && owners[k-1] == pla) +
			(CE1(k) && owners[k+1] == pla) +
			(CN1(k) && owners[k+8] == pla)
			>= 2;
		}
		else
		{
			return
			(owners[k-8] == pla) +
			(owners[k-1] == pla) +
			(owners[k+1] == pla) +
			(owners[k+8] == pla)
			>= 2;
		}
	}

	//Is the square k adjacent to 3 or more friendly pieces?
	inline bool isGuarded3(pla_t pla, loc_t k) const
	{
		if(ISEDGE[k])
		{
			return
			(CS1(k) && owners[k-8] == pla) +
			(CW1(k) && owners[k-1] == pla) +
			(CE1(k) && owners[k+1] == pla) +
			(CN1(k) && owners[k+8] == pla)
			>= 3;
		}
		else
		{
			return
			(owners[k-8] == pla) +
			(owners[k-1] == pla) +
			(owners[k+1] == pla) +
			(owners[k+8] == pla)
			>= 3;
		}
	}

  //Atempts to find a piece of pla guarding k. ERRORSQUARE if none.
  inline loc_t findGuard(pla_t pla, loc_t k) const
  {
    if(CS1(k) && owners[k-8] == pla) return k-8;
    if(CW1(k) && owners[k-1] == pla) return k-1;
    if(CE1(k) && owners[k+1] == pla) return k+1;
    if(CN1(k) && owners[k+8] == pla) return k+8;
    return ERRORSQUARE;
  }

  //Is the square regular or guarded by at least 1 friendly piece?
  inline bool isTrapSafe1(pla_t pla, loc_t k) const
  {
    return !ISTRAP[k] || trapGuardCounts[pla][TRAPINDEX[k]] >= 1;
  }

  //Is the square regular or guarded by at least 2 friendly pieces?
  inline bool isTrapSafe2(pla_t pla, loc_t k) const
  {
    return !ISTRAP[k] || trapGuardCounts[pla][TRAPINDEX[k]] >= 2;
  }

  //Is the square regular or guarded by at least 3 friendly pieces?
  inline bool isTrapSafe3(pla_t pla, loc_t k) const
  {
    return !ISTRAP[k] || trapGuardCounts[pla][TRAPINDEX[k]] >= 3;
  }

	//Can the given piece step anywhere, assuming it is unfrozen?
	inline bool isOpenToStep(loc_t k) const
	{
		if(ISEDGE[k])
		{
			return
			(CS1(k) && owners[k-8] == NPLA && isRabOkayS(owners[k],k)) ||
			(CW1(k) && owners[k-1] == NPLA) ||
			(CE1(k) && owners[k+1] == NPLA) ||
			(CN1(k) && owners[k+8] == NPLA && isRabOkayN(owners[k],k));
		}
		else
		{
			return
			(owners[k-8] == NPLA && isRabOkayS(owners[k],k)) ||
			(owners[k-1] == NPLA) ||
			(owners[k+1] == NPLA) ||
			(owners[k+8] == NPLA && isRabOkayN(owners[k],k));
		}
	}

	//Can the given piece step anywhere, assuming it is unfrozen?
	inline bool isOpenToStep(loc_t k, loc_t ign) const
	{
		if(ISEDGE[k])
		{
			return
			(CS1(k) && ign != k-8 && owners[k-8] == NPLA && isRabOkayS(owners[k],k)) ||
			(CW1(k) && ign != k-1 && owners[k-1] == NPLA) ||
			(CE1(k) && ign != k+1 && owners[k+1] == NPLA) ||
			(CN1(k) && ign != k+8 && owners[k+8] == NPLA && isRabOkayN(owners[k],k));
		}
		else
		{
			return
			(ign != k-8 && owners[k-8] == NPLA && isRabOkayS(owners[k],k)) ||
			(ign != k-1 && owners[k-1] == NPLA) ||
			(ign != k+1 && owners[k+1] == NPLA) ||
			(ign != k+8 && owners[k+8] == NPLA && isRabOkayN(owners[k],k));
		}
	}

  //Can the given piece push out in 2 steps, assuming it is unfrozen?
  inline bool isOpenToPush(loc_t k) const
  {
    pla_t pla = owners[k];
    pla_t opp = OPP(pla);
    if(pieces[k] == RAB) {return false;}
    if(CS1(k) && owners[k-8] == opp && pieces[k-8] < pieces[k] && isOpen(k-8)) {return true;}
    if(CW1(k) && owners[k-1] == opp && pieces[k-1] < pieces[k] && isOpen(k-1)) {return true;}
    if(CE1(k) && owners[k+1] == opp && pieces[k+1] < pieces[k] && isOpen(k+1)) {return true;}
    if(CN1(k) && owners[k+8] == opp && pieces[k+8] < pieces[k] && isOpen(k+8)) {return true;}
    return false;
  }

	//Can the given piece push or step out anywhere in 2 steps, assuming it is unfrozen?
  //Does NOT consider trapsafety!
	inline bool isOpenToMove(loc_t k) const
	{
	  pla_t pla = owners[k];
	  pla_t opp = OPP(pla);
	  if(isOpenToStep(k)) {return true;}
	  if(pieces[k] == RAB) {return false;}
	  if(CS1(k) && owners[k-8] == opp && pieces[k-8] < pieces[k] && isOpen(k-8)) {return true;}
    if(CW1(k) && owners[k-1] == opp && pieces[k-1] < pieces[k] && isOpen(k-1)) {return true;}
    if(CE1(k) && owners[k+1] == opp && pieces[k+1] < pieces[k] && isOpen(k+1)) {return true;}
    if(CN1(k) && owners[k+8] == opp && pieces[k+8] < pieces[k] && isOpen(k+8)) {return true;}
    if(CS1(k) && owners[k-8] == pla && isOpenToStep(k-8)) {return true;}
    if(CW1(k) && owners[k-1] == pla && isOpenToStep(k-1)) {return true;}
    if(CE1(k) && owners[k+1] == pla && isOpenToStep(k+1)) {return true;}
    if(CN1(k) && owners[k+8] == pla && isOpenToStep(k+8)) {return true;}
    return false;
	}

	//Is the given square adjacent to any open space?
	inline bool isOpen(loc_t k) const
	{
		if(ISEDGE[k])
		{
			return
			(CS1(k) && owners[k-8] == NPLA) ||
			(CW1(k) && owners[k-1] == NPLA) ||
			(CE1(k) && owners[k+1] == NPLA) ||
			(CN1(k) && owners[k+8] == NPLA);
		}
		else
		{
			return
			(owners[k-8] == NPLA) ||
			(owners[k-1] == NPLA) ||
			(owners[k+1] == NPLA) ||
			(owners[k+8] == NPLA);
		}
	}

	inline bool isOpen(loc_t k, loc_t ign) const
  {
    if(ISEDGE[k])
    {
      return
      (CS1(k) && k-8 != ign && owners[k-8] == NPLA) ||
      (CW1(k) && k-1 != ign && owners[k-1] == NPLA) ||
      (CE1(k) && k+1 != ign && owners[k+1] == NPLA) ||
      (CN1(k) && k+8 != ign && owners[k+8] == NPLA);
    }
    else
    {
      return
      (k-8 != ign && owners[k-8] == NPLA) ||
      (k-1 != ign && owners[k-1] == NPLA) ||
      (k+1 != ign && owners[k+1] == NPLA) ||
      (k+8 != ign && owners[k+8] == NPLA);
    }
  }

	//Count the number of adjacent empty squares
	inline int countOpen(loc_t k) const
	{
		return
		(CS1(k) && owners[k-8] == NPLA) +
		(CW1(k) && owners[k-1] == NPLA) +
		(CE1(k) && owners[k+1] == NPLA) +
		(CN1(k) && owners[k+8] == NPLA);
	}

	//Find empty square adjacent
	inline loc_t findOpen(loc_t k) const
	{
    if(CS1(k) && owners[k-8] == NPLA) {return k-8;}
    if(CW1(k) && owners[k-1] == NPLA) {return k-1;}
    if(CE1(k) && owners[k+1] == NPLA) {return k+1;}
    if(CN1(k) && owners[k+8] == NPLA) {return k+8;}
    return ERRORSQUARE;
	}

	//Find empty square adjacent other than ign
	inline loc_t findOpenIgnoring(loc_t k, loc_t ign) const
	{
    if(CS1(k) && k-8 != ign && owners[k-8] == NPLA) {return k-8;}
    if(CW1(k) && k-1 != ign && owners[k-1] == NPLA) {return k-1;}
    if(CE1(k) && k+1 != ign && owners[k+1] == NPLA) {return k+1;}
    if(CN1(k) && k+8 != ign && owners[k+8] == NPLA) {return k+8;}
    return ERRORSQUARE;
	}

	//Find empty square adjacent to step
  inline loc_t findOpenToStep(loc_t k) const
  {
    if(CS1(k) && owners[k-8] == NPLA && isRabOkayS(owners[k],k)) return k-8;
    if(CW1(k) && owners[k-1] == NPLA)                            return k-1;
    if(CE1(k) && owners[k+1] == NPLA)                            return k+1;
    if(CN1(k) && owners[k+8] == NPLA && isRabOkayN(owners[k],k)) return k+8;
    return ERRORSQUARE;
  }

	//Is the given square adjacent to at least two spaces?
	inline bool isOpen2(loc_t k) const
	{
		if(ISEDGE[k])
		{
			return
			(CS1(k) && owners[k-8] == NPLA) +
			(CW1(k) && owners[k-1] == NPLA) +
			(CE1(k) && owners[k+1] == NPLA) +
			(CN1(k) && owners[k+8] == NPLA)
			>= 2;
		}
		else
		{
			return
			(owners[k-8] == NPLA) +
			(owners[k-1] == NPLA) +
			(owners[k+1] == NPLA) +
			(owners[k+8] == NPLA)
			>= 2;
		}
	}

	//Can we make a trap safe for ploc in one step? By stepping a piece adjacent to it
	inline bool canMakeTrapSafeFor(pla_t pla, loc_t kt, loc_t ploc) const
	{
		if(kt-8 != ploc && owners[kt-8] == NPLA)
		{
			loc_t loc = kt-8;
			if(loc-8 != ploc && owners[loc-8] == pla && isThawed(loc-8) && isRabOkayN(pla,loc-8)) return true;
			if(loc-1 != ploc && owners[loc-1] == pla && isThawed(loc-1)) return true;
			if(loc+1 != ploc && owners[loc+1] == pla && isThawed(loc+1)) return true;
		}
		if(kt-1 != ploc && owners[kt-8] == NPLA)
		{
			loc_t loc = kt-1;
			if(loc-8 != ploc && owners[loc-8] == pla && isThawed(loc-8) && isRabOkayN(pla,loc-8)) return true;
			if(loc-1 != ploc && owners[loc-1] == pla && isThawed(loc-1)) return true;
			if(loc+8 != ploc && owners[loc+8] == pla && isThawed(loc+8) && isRabOkayS(pla,loc+8)) return true;
		}
		if(kt+1 != ploc && owners[kt-8] == NPLA)
		{
			loc_t loc = kt+1;
			if(loc-8 != ploc && owners[loc-8] == pla && isThawed(loc-8) && isRabOkayN(pla,loc-8)) return true;
			if(loc+1 != ploc && owners[loc+1] == pla && isThawed(loc+1)) return true;
			if(loc+8 != ploc && owners[loc+8] == pla && isThawed(loc+8) && isRabOkayS(pla,loc+8)) return true;
		}
		if(kt+8 != ploc && owners[kt-8] == NPLA)
		{
			loc_t loc = kt+8;
			if(loc-1 != ploc && owners[loc-1] == pla && isThawed(loc-1)) return true;
			if(loc+1 != ploc && owners[loc+1] == pla && isThawed(loc+1)) return true;
			if(loc+8 != ploc && owners[loc+8] == pla && isThawed(loc+8) && isRabOkayS(pla,loc+8)) return true;
		}
		return false;
	}

	//Is the piece at square k surrounded on all 4 sides?
	inline bool isBlocked(loc_t k) const
	{return !isOpen(k);}

	inline bool wouldRabbitBeDomAt(pla_t pla, loc_t k) const
	{
	  pla_t opp = OPP(pla);
	  return
			(CS1(k) && owners[k-8] == opp && pieces[k-8] > RAB) ||
			(CW1(k) && owners[k-1] == opp && pieces[k-1] > RAB) ||
			(CE1(k) && owners[k+1] == opp && pieces[k+1] > RAB) ||
			(CN1(k) && owners[k+8] == opp && pieces[k+8] > RAB);
	}

	inline bool wouldRabbitBeUFAt(pla_t pla, loc_t k) const
	{
	  pla_t opp = OPP(pla);
		return
		(
			(CS1(k) && owners[k-8] == pla) ||
			(CW1(k) && owners[k-1] == pla) ||
			(CE1(k) && owners[k+1] == pla) ||
			(CN1(k) && owners[k+8] == pla)
		)
		||
		!(
			(CS1(k) && owners[k-8] == opp && pieces[k-8] > RAB) ||
			(CW1(k) && owners[k-1] == opp && pieces[k-1] > RAB) ||
			(CE1(k) && owners[k+1] == opp && pieces[k+1] > RAB) ||
			(CN1(k) && owners[k+8] == opp && pieces[k+8] > RAB)
		);
	}

	//Would a piece of the given power be unfrozen at square k?
	inline bool wouldBeUF(pla_t pla, loc_t loc, loc_t k) const
	{
	  piece_t piece = pieces[loc];
	  pla_t opp = OPP(pla);
		return
		(
			(CS1(k) && owners[k-8] == pla) ||
			(CW1(k) && owners[k-1] == pla) ||
			(CE1(k) && owners[k+1] == pla) ||
			(CN1(k) && owners[k+8] == pla)
		)
		||
		!(
			(CS1(k) && owners[k-8] == opp && pieces[k-8] > piece) ||
			(CW1(k) && owners[k-1] == opp && pieces[k-1] > piece) ||
			(CE1(k) && owners[k+1] == opp && pieces[k+1] > piece) ||
			(CN1(k) && owners[k+8] == opp && pieces[k+8] > piece)
		);
	}

	//Would a piece of the given power be unfrozen at square k, ignoring anything on squares ign1?
	inline bool wouldBeUF(pla_t pla, loc_t loc, loc_t k, loc_t ign1) const
	{
	  piece_t piece = pieces[loc];
	  pla_t opp = OPP(pla);
		return
		(
			(CS1(k) && k-8 != ign1 && owners[k-8] == pla) ||
			(CW1(k) && k-1 != ign1 && owners[k-1] == pla) ||
			(CE1(k) && k+1 != ign1 && owners[k+1] == pla) ||
			(CN1(k) && k+8 != ign1 && owners[k+8] == pla)
		)
		||
		!(
			(CS1(k) && k-8 != ign1 && owners[k-8] == opp && pieces[k-8] > piece) ||
			(CW1(k) && k-1 != ign1 && owners[k-1] == opp && pieces[k-1] > piece) ||
			(CE1(k) && k+1 != ign1 && owners[k+1] == opp && pieces[k+1] > piece) ||
			(CN1(k) && k+8 != ign1 && owners[k+8] == opp && pieces[k+8] > piece)
		);
	}

	//Would a piece of the given power be unfrozen at square k, ignoring anything on squares ign1, ign2?
	inline bool wouldBeUF(pla_t pla, loc_t loc, loc_t k, loc_t ign1, loc_t ign2) const
	{
	  piece_t piece = pieces[loc];
	  pla_t opp = OPP(pla);
		return
		(
			(CS1(k) && k-8 != ign1 && k-8 != ign2 && owners[k-8] == pla) ||
			(CW1(k) && k-1 != ign1 && k-1 != ign2 && owners[k-1] == pla) ||
			(CE1(k) && k+1 != ign1 && k+1 != ign2 && owners[k+1] == pla) ||
			(CN1(k) && k+8 != ign1 && k+8 != ign2 && owners[k+8] == pla)
		)
		||
		!(
			(CS1(k) && k-8 != ign1 && k-8 != ign2 && owners[k-8] == opp && pieces[k-8] > piece) ||
			(CW1(k) && k-1 != ign1 && k-1 != ign2 && owners[k-1] == opp && pieces[k-1] > piece) ||
			(CE1(k) && k+1 != ign1 && k+1 != ign2 && owners[k+1] == opp && pieces[k+1] > piece) ||
			(CN1(k) && k+8 != ign1 && k+8 != ign2 && owners[k+8] == opp && pieces[k+8] > piece)
		);
	}

	//Would a piece be guarded at square k?
	inline bool wouldBeG(pla_t pla, loc_t k) const
	{
		return
		(
			(CS1(k) && owners[k-8] == pla) ||
			(CW1(k) && owners[k-1] == pla) ||
			(CE1(k) && owners[k+1] == pla) ||
			(CN1(k) && owners[k+8] == pla)
		);
	}

	//Would a piece be guarded at square k, ignoring anything on squares ign1?
	inline bool wouldBeG(pla_t pla, loc_t k, loc_t ign1) const
	{
		return
		(
			(CS1(k) && k-8 != ign1 && owners[k-8] == pla) ||
			(CW1(k) && k-1 != ign1 && owners[k-1] == pla) ||
			(CE1(k) && k+1 != ign1 && owners[k+1] == pla) ||
			(CN1(k) && k+8 != ign1 && owners[k+8] == pla)
		);
	}

	//Would a piece be guarded at square k, ignoring anything on squares ign1, ign2?
	inline bool wouldBeG(pla_t pla, loc_t k, loc_t ign1, loc_t ign2) const
	{
		return
		(
			(CS1(k) && k-8 != ign1 && k-8 != ign2 && owners[k-8] == pla) ||
			(CW1(k) && k-1 != ign1 && k-1 != ign2 && owners[k-1] == pla) ||
			(CE1(k) && k+1 != ign1 && k+1 != ign2 && owners[k+1] == pla) ||
			(CN1(k) && k+8 != ign1 && k+8 != ign2 && owners[k+8] == pla)
		);
	}

	//Would a piece currently at loc be dominated at square k?
  inline bool wouldBeDom(pla_t pla, loc_t loc, loc_t k) const
  {
    pla_t opp = OPP(pla);
    return
    (
      (CS1(k) && owners[k-8] == opp && pieces[k-8] > pieces[loc]) ||
      (CW1(k) && owners[k-1] == opp && pieces[k-1] > pieces[loc]) ||
      (CE1(k) && owners[k+1] == opp && pieces[k+1] > pieces[loc]) ||
      (CN1(k) && owners[k+8] == opp && pieces[k+8] > pieces[loc])
    );
  }

  //Would a piece currently at loc be dominated at square k, ignoring square ign?
  inline bool wouldBeDom(pla_t pla, loc_t loc, loc_t k, loc_t ign) const
  {
    pla_t opp = OPP(pla);
    return
    (
      (CS1(k) && k-8 != ign && owners[k-8] == opp && pieces[k-8] > pieces[loc]) ||
      (CW1(k) && k-1 != ign && owners[k-1] == opp && pieces[k-1] > pieces[loc]) ||
      (CE1(k) && k+1 != ign && owners[k+1] == opp && pieces[k+1] > pieces[loc]) ||
      (CN1(k) && k+8 != ign && owners[k+8] == opp && pieces[k+8] > pieces[loc])
    );
  }

	//Is it okay by the rabbit rule for this piece to step south?
	inline bool isRabOkayS(pla_t pla, loc_t k) const
	{return pieces[k] != RAB || pla == SILV;}

  //Is it okay by the rabbit rule for this piece to step north?
	inline bool isRabOkayN(pla_t pla, loc_t k) const
	{return pieces[k] != RAB || pla == GOLD;}

  //Is it okay by the rabbit rule for this piece at k0 to walk its way to k1 in any number of steps?
	inline bool isRabOkay(pla_t pla, loc_t k0, loc_t k1) const
	{return (pieces[k0] != RAB) || (pla == SILV && k1/8 <= k0/8) || (pla == GOLD && k1/8 >= k0/8);}

  //Would it be okay by the rabbit rule for this piece at ploc to go from k0 to k1 in any number of steps?
	inline bool isRabOkay(pla_t pla, loc_t ploc, loc_t k0, loc_t k1) const
	{return (pieces[ploc] != RAB) || (pla == SILV && k1/8 <= k0/8) || (pla == GOLD && k1/8 >= k0/8);}

	//Can a piece of pla step to loc?
	//Does not check trapsafety
	inline bool canStepAndOccupy(pla_t pla, loc_t loc) const
	{
	  if(CS1(loc) && owners[loc-8] == pla && isThawedC(loc-8) && isRabOkayN(pla,loc-8)) {return true;}
    if(CW1(loc) && owners[loc-1] == pla && isThawedC(loc-1)                         ) {return true;}
    if(CE1(loc) && owners[loc+1] == pla && isThawedC(loc+1)                         ) {return true;}
    if(CN1(loc) && owners[loc+8] == pla && isThawedC(loc+8) && isRabOkayS(pla,loc+8)) {return true;}
    return false;
	}

	//Get the location, if any, of an adjacent piece that will be sacrificed if this piece moves.
	inline loc_t getSacLoc(pla_t pla, loc_t loc) const
	{
		loc_t capLoc = ADJACENTTRAP[loc];
		if(capLoc == ERRORSQUARE)
			return ERRORSQUARE;
		if(owners[capLoc] == pla && trapGuardCounts[pla][Board::TRAPINDEX[capLoc]] <= 1)
			return capLoc;
		return ERRORSQUARE;
	}

	inline piece_t strongestAdjacentPla(pla_t pla, loc_t loc) const
	{
	  piece_t piece = EMP;
	  if(CS1(loc) && owners[loc-8] == pla) {piece = piece >= pieces[loc-8] ? piece : pieces[loc-8];}
    if(CW1(loc) && owners[loc-1] == pla) {piece = piece >= pieces[loc-1] ? piece : pieces[loc-1];}
    if(CE1(loc) && owners[loc+1] == pla) {piece = piece >= pieces[loc+1] ? piece : pieces[loc+1];}
    if(CN1(loc) && owners[loc+8] == pla) {piece = piece >= pieces[loc+8] ? piece : pieces[loc+8];}
    return piece;
	}

  //TEMPORARY CHANGES----------------------------------------------------------------------

	//Note! - after making a temp step/move, none of the bitmaps are updated!
	//Nor are the hashes, step counter, etc updated. Do not trust isFrozen or isThawed, use isFrozenC or isThawedC instead.

  inline void tempStep(loc_t k0, loc_t k1)
  {
    owners[k1] = owners[k0];
    pieces[k1] = pieces[k0];
    owners[k0] = NPLA;
    pieces[k0] = EMP;
    if(ADJACENTTRAP[k0] != ERRORSQUARE)
      trapGuardCounts[owners[k1]][TRAPINDEX[ADJACENTTRAP[k0]]]--;
    if(ADJACENTTRAP[k1] != ERRORSQUARE)
      trapGuardCounts[owners[k1]][TRAPINDEX[ADJACENTTRAP[k1]]]++;
  }

  inline void tempPP(loc_t k0, loc_t k1, loc_t k2)
  {
    owners[k2] = owners[k1];
    pieces[k2] = pieces[k1];
    owners[k1] = owners[k0];
    pieces[k1] = pieces[k0];
    owners[k0] = NPLA;
    pieces[k0] = EMP;
    if(ADJACENTTRAP[k0] != ERRORSQUARE)
      trapGuardCounts[owners[k1]][TRAPINDEX[ADJACENTTRAP[k0]]]--;
    if(ADJACENTTRAP[k1] != ERRORSQUARE)
      trapGuardCounts[owners[k1]][TRAPINDEX[ADJACENTTRAP[k1]]]++;
    if(ADJACENTTRAP[k1] != ERRORSQUARE)
      trapGuardCounts[owners[k2]][TRAPINDEX[ADJACENTTRAP[k1]]]--;
    if(ADJACENTTRAP[k2] != ERRORSQUARE)
      trapGuardCounts[owners[k2]][TRAPINDEX[ADJACENTTRAP[k2]]]++;
  }

  inline TempRecord tempStepC(loc_t k0, loc_t k1)
  {
    owners[k1] = owners[k0];
    pieces[k1] = pieces[k0];
    owners[k0] = NPLA;
    pieces[k0] = EMP;

    if(ADJACENTTRAP[k0] != ERRORSQUARE)
      trapGuardCounts[owners[k1]][TRAPINDEX[ADJACENTTRAP[k0]]]--;
    if(ADJACENTTRAP[k1] != ERRORSQUARE)
      trapGuardCounts[owners[k1]][TRAPINDEX[ADJACENTTRAP[k1]]]++;

    TempRecord rec = TempRecord(k0,k1);

    int caploc = ADJACENTTRAP[k0];
    if(caploc != ERRORSQUARE)
    {checkCapsR(caploc, rec);}

    return rec;
  }

  inline void undoTemp(TempRecord rec)
  {
    if(rec.capLoc != ERRORSQUARE)
    {
      owners[rec.capLoc] = rec.capOwner;
      pieces[rec.capLoc] = rec.capPiece;
    }

    owners[rec.k0] = owners[rec.k1];
    pieces[rec.k0] = pieces[rec.k1];

    owners[rec.k1] = NPLA;
    pieces[rec.k1] = EMP;

    if(ADJACENTTRAP[rec.k0] != ERRORSQUARE)
      trapGuardCounts[owners[rec.k0]][TRAPINDEX[ADJACENTTRAP[rec.k0]]]++;
    if(ADJACENTTRAP[rec.k1] != ERRORSQUARE)
      trapGuardCounts[owners[rec.k0]][TRAPINDEX[ADJACENTTRAP[rec.k1]]]--;
  }

  inline bool checkCapsR(loc_t k, TempRecord &rec)
  {
    pla_t owner = owners[k];
    if(ISTRAP[k] && owner != NPLA && trapGuardCounts[owner][TRAPINDEX[k]] == 0)
    {
      rec.capLoc = k;
      rec.capOwner = owner;
      rec.capPiece = pieces[k];
      owners[k] = NPLA;
      pieces[k] = EMP;
      return true;
    }
    return false;
  }

};


#endif
