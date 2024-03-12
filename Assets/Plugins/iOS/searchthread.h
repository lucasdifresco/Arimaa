
/*
 * searchthread.h
 * Author: davidwu
 */

#ifndef SEARCHTHREAD_H_
#define SEARCHTHREAD_H_

#define BOOST_USE_WINDOWS_H
#include <vector>
#include <set>

#ifdef MULTITHREADING
#include <boost/thread.hpp>
#else
namespace boost
{
  class mutex
  { public:
    inline void lock() {};
    inline void unlock() {};
  };
  class thread
  { public:
    inline thread() {};
    inline void join() {};
  };
  template <class T>
  class unique_lock
  { public:
    unique_lock(T t) {};
    inline void lock() {};
    inline void unlock() {};
  };
  template <class T>
  class lock_guard
  { public:
    lock_guard(T t) {};
  };
  class condition_variable
  { public:
    inline void notify_all() {};
    template <class T>
    inline void wait(unique_lock<T> lock) {};
  };
}
#endif


#include "board.h"
#include "search.h"
#include "searchstats.h"

using namespace std;

struct SplitPointBuffer;
struct SearchThread;
struct SplitPoint;
class SearchTree;

//Notes:
//
//OVERVIEW OF PARALLEL SEARCH-----------------------------------------------------------------------
//
//SplitPoints------------------
//
//All of the main search is coordinated using SplitPoints.  SplitPoints track the ongoing status of the search
//at each node, such as the alpha and beta bounds, the number of moves searched, the best move found so far, etc.
//They also contain the data necessary for any thread that wishes to join in.
//
//Whenever a new node needs to be searched, a SplitPoint is allocated for that node, and when the node is
//finished, the SplitPoint is freed. This allocation is not done dynamically, but rather from preallocated
//SplitPointBuffers (see below).
//
//Whenever a thread wishes, it can publicize the SplitPoint it is working on, adding it to a global list
//of available work for free threads. Critical SplitPoint operations are performed with a per-SplitPoint mutex.
//
//Every SplitPoint must always have at least one thread working on it or a subtree extending from it. To maintain
//this invariant, the last thread to finish working at a SplitPoint and finish the SplitPoint must back up to the
//parent and then begin working on the parent. This may or may not be the thread that created the SplitPoint to
//begin with - there is no concept of a "parent" thread for a SplitPoint, in terms of the search.
//
//No SplitPoint is ever freed when it has children below it in the SearchTree, even during beta cutoffs. Instead,
//the policy is to simply mark the SplitPoint as aborted, and then threads periodically poll up the tree to see
//if any SplitPoint above them has been aborted.
//
//SplitPointBuffers------------
//
//SplitPoints are never dynamically allocated, but rather are allocated all at the start of search in buffers.
//Each buffer is designed for use by one thread, and contains two splitpoints per fDepth, which is the most that
//should ever be necessary (a second is needed, briefly, when finishing a splitpoint produces another at the
//same depth, such as during null move or lmr).
//
//A thread can request a buffer from the SearchTree, at which point it exclusively owns the buffer and can get
//and free SplitPoints without synchronization.
//
//When a thread becomes free and must find work elsewhere, because the current splitpoint has no more work but
//is not yet done, if the current splitpoint came from the thread's own buffer, it abandons ownership of the buffer
//and requests a new one from the tree, no longer owning the previous one. The thread that finishes the splitpoint
//is then responsible for freeing the splitpoint, and returning the buffer, if it is empty, back to the SearchTree.
//
//At all times, we maintain the following property: the allocated SplitPoints in a buffer must form a contiguous
//path in the game tree. Moreover, if the buffer is owned (has not yet been abandoned), it must be the case that
//the deepest splitpoint worked on by the owning thread is precisely the deepest SplitPoint in the buffer.
//This guarantees safety - while the buffer is owned, the current thread is the only one able to allocate and
//free splitpoints because it is always the one at the tip of the path.
//
//The metadata (isUsed, isFirst), for each SplitPoint is synchronized under the SplitPoint's mutex. This ensures
//that a consistent, synchronized view is maintained even if the buffer must be disowned and thereafter accessed
//by a different thread.
//
//

//---------------------------------------------------------------------------------------

//Buffer for holding allocated SplitPoint objects. Given out to each thread, and recycled when empty
//Each fDepth gets 2 SplitPoints currently.
struct SplitPointBuffer
{
	private:
	int maxFDepth;        //Max fDepth that we have SplitPoints for
	SplitPoint* spts;     //Array of buffered SplitPoints
	bool* isUsed;         //Indicates which SplitPoints are used
	bool* isFirst;        //Indicates whether this SplitPoint is the first allocated in the buffer

	public:
	SplitPointBuffer(Searcher* searcher, int maxFDepth);
	~SplitPointBuffer();

	//Error checking function
	void assertEverythingUnused();

	//Get a SplitPoint for the given fDepth.
	SplitPoint* acquireSplitPoint(SplitPoint* parent);

	//Free the SplitPoint, indicating that it is done
	//Returns true if this buffer is now empty, else false
	bool freeSplitPoint(SplitPoint* spt);

};

//Structure for managing SplitPoints during a search.
//Implements a monitor, all of the synchronization is done for you, just use the interface.
//IMPORTANT:
//Should not call ANY searchtree functions while a SplitPoint is locked, to maintain lock acquisition order.
class SearchTree
{
	//Backreference to searcher
	Searcher* searcher;

	//Search data
	int numThreads;         //Number of threads
	bool searchDone;        //Is the current entire search done? Controls gate for threads to completely exit
	bool iterationGoing;    //Is an iteration of search in progress? Controls transition in and out of main loop
	bool didTimeout;        //Indicates whether timeout occured or not. Search cannot be restarted after timeout

	SearchThread* threads; 	//Array of all threads

	//SplitPoint buffers
	int initialNumFreeSptBufs;      //How many are there initially?
	int numFreeSptBufs;             //How many are there?
	SplitPointBuffer** freeSptBufs; //Array of SplitPoint buffers available to be handed out
	SplitPointBuffer* rootSptBuf;   //Special buffer whose only job is to contain the root node

	//Root node of search
	SplitPoint* rootNode;

	//Publicized splitpoints - SplitPoints that are available work for free threads.
	//Intrusive list using publicNext and publicPrev pointers in splitpoint
	//Head node is a dummy splitpoint, unused.
	SplitPoint* publicizedListHead;

	//Actual boost library thread objects
	boost::thread* boostThreads;

	//Synchronization
	boost::mutex mutex;
	boost::condition_variable publicWorkCondvar;

	int iterationNumWaiting;
	boost::condition_variable iterationCondvar;
	boost::condition_variable iterationMasterCondvar;

	public:
	SearchTree(Searcher* searcher, int numThreads, int maxMSearchDepth, int maxCDepth, const Board& b, const BoardHistory& hist);
	~SearchTree();

	//SEARCH INTERFACE------------------------------------------------

	//Start all the child threads, can call just after construction
	void startThreads();

	//Stop all the child threads, can call just before destruction
	void stopThreads();

	//Get the root node, for the purposes of initialization
	SplitPoint* acquireRootNode();

	//Free the root node
	void freeRootNode(SplitPoint* spt);

	//INTERNAL to searchthread.cpp-------------------------------------

	//Start one iteration of iterative deepening
	void beginIteration();

	//End one iteration of iterative deepening, call from inside the main loop when the first thread finds itself done
	//with the root node.
	void endIterationInternal();

	//End one iteration of iterative deepening, signal threads to exit the main loop
	void endIteration();

	//Call from within the main loop by any thread (or from externally) to report timeout, and that threads
	//should exit the main loop
	void timeout(SearchThread* curThread);

	//Get a new buffer of splitpoints for a thread to use during search.
	SplitPointBuffer* acquireSplitPointBuffer();

	//Free an unused buffer of splitpoints that has been disowned, or one's own buffer
	void freeSplitPointBuffer(SplitPointBuffer* buf);

	//Publicize the SplitPoint so that other threads can help
	//The SplitPoint must be initialized with board data before this is called!
	void publicize(SplitPoint* spt);

	//Depublicize the SplitPoint. Call only when returning this splitpoint back to buffer.
	//Okay to call for splitpoints that aren't public
	void depublicize(SplitPoint* spt);

	//Find a good publicized SplitPoint and grab work from it for the given thread.
	//If none, blocks until there is some. Threads only return without work when:
	// 1) They are the master thread and the search is done
	// 2) The thread has been outright terminated
	void getPublicWork(SearchThread* thread);

	//Return the master thread
	SearchThread* getMasterThread();

	//Get stats from all threads combined together
	SearchStats getCombinedStats();

	//Copy killer moves and the len from another thread into the given array and the len reference
	//Not synchronized.
	void copyKillersUnsynchronized(int fromThreadId, move_t* killerMoves, int& killerMovesLen);

	private:
	static void runChild(SearchTree* tree, Searcher* searcher, SearchThread* curThread);
};

//A single node in the seach tree at which we are parallelizing
//All member arrays are owned by this SplitPoint.
struct SplitPoint
{
	//Constants--------------------------------

	//Node types
	static const int TYPE_INVAL = 0;
	static const int TYPE_ROOT = 1;  //Root node of whole search
	static const int TYPE_PV = 2;    //Expects exact value
	static const int TYPE_CUT = 3;   //Expects beta cutoff
	static const int TYPE_ALL = 4;   //Expects alpha fail

	//Modes of search
	static const int MODE_INVAL = 0;
	static const int MODE_NORMAL = 1;    //Normal search, back up the value when we finish
	static const int MODE_REDUCED = 2;   //Reduced search, re-search unless we fail low
	static const int MODE_NULLMOVE = 3;  //Null move search, re-search unless we fail high

	//Constant--------------------------------------------------------------------------------------

	//Guaranteed constant throughout the search
	Searcher* searcher;       //The searcher running this search
	int fDepth;               //Search tree recursion depth of this splitpoint.
	SplitPointBuffer* buffer; //The SplitPointBuffer containing this splitpoint
	int bufferIdx;            //The index of this splitpoint in the buffer

	//Data synchronized under Searchtree------------------------------------------------------------

	//Intrusive doubly-linked list for SearchTree::publicizedListHead
	SplitPoint* publicNext;
	SplitPoint* publicPrev;
	bool isPublic;

	//Data synchronized under SplitPoint------------------------------------------------------------

	//Mutex---------------------------
	private:
	boost::mutex mutex;
	public:

	//Splitpoint Buffer Data-------------------
	//This is guaranteed constant after a SplitPoint is gotten from the buffer.
	SplitPoint* parent; //The parent splitpoint above this one in the tree, if any.

	//This may change, but begin false and only ever change from false -> true while the SplitPoint is gotten from the tree
	bool isAborted; //Has this SplitPoint been terminated? (beta cutoff here or above, or timeout, or we just finished it regularly)

	//Constant after initialization--------------
	bool isInitialized; //Set to false when freed, set true after initialization

	Board board; //The board position at this node

	move_t* moveHistory; //Indexed by relative full turn, tracks the moves made from the start of search.
	int moveHistoryCapacity;
	int moveHistoryLen;

	int parentMoveIndex; //The index of the move made in the parent to get to this move
	move_t parentMove; //The move of the parent
	int searchMode; //The mode of search this is from the parent
	bool changedPlayer; //Is the player to move different from the parent's?

	int cDepth;  //Depth in Arimaa steps from the start of search
	int rDepth4; //Depth remaining * 4 until the end of search.

	//Mutable after initialization---------------------
	int nodeType; //Estimated type of node
	double nodeAllness; //For when we are an ALL node. To what degree does this node seem to be an ALL node?

	eval_t alpha; //The current alpha bound at this node
	eval_t beta; //The current beta bound at this node

	eval_t bestEval; //The best eval so far at this node
	move_t bestMove; //The best move found so far at this node
	flag_t bestFlag; //Eval flag indicating current alpha-beta bound status of this node
	int bestIndex;   //The index of the best move found so far

	move_t* pv; //A pv array, the best pv at this node
	int pvLen;  //The length of the best pv at this node.

	move_t* mv;     //A moves array
	int* hm;        //A move ordering heuristic array, or null
	int mvCapacity; //Capacity of the moves and hm array
	int numMoves;   //How many moves are there to search?

	bool areMovesUngen;  //Have the main bulk of moves not yet been generated? (only the hashmove/killer moves)
	int numMovesStarted; //The number of moves we have started searching at this splitpoint
	int numMovesDone;    //The number of moves that have been searched and finished (or aborted, and the thread returned)
	int hashMoveIndex;   //The index of the hash move, or -1.
	int killerMoveIndex; //The index of the killer move, or -1.

	int copyKillersFromThread; //Copy killer moves from this thread when joining this splitpoint, if not -1.

	//Different pruning amounts. Perform this when moveIdx >= threshold
	int pvsThreshold;
	int r1Threshold;
	int r2Threshold;
	int pruneThreshold;


	SplitPoint();
	~SplitPoint();

	//Initialize to be the split point for a given position
	//The last three parameters specify some starting moves to load it with, and are optional. Can be used
	//to load in a hashMove, or at the root, the full moves.
	//moveArraySize - how big to ensure the move array
	void init(int parentMoveIdx, move_t parentMove, int searchMode, const Board& b, bool changedPlayer,
			int cDepth, int rDepth4, eval_t alpha, eval_t beta, bool isRoot, move_t hashMove, move_t killerMove, int moveArraySize,
			int copyKillersFrom);

	//Helper
	void initNodeType(bool isRoot);

	//Synchronization--------------------------------------
	void lock(SearchThread* thread);
	void unlock(SearchThread* thread);

	//Post-Initialization----------------------------------
	//Must be locked to use any of these!

	//Does this SplitPoint need to be publicized now for parallelization?
	bool needsPublication();

	//Add all the fsearch moves.
	void addFSearchMoves(const SearchThread* curThread);

	//Add all the msearch moves.
	//SearchThread is used purely for stats like killer table, etc.
	void addMSearchMoves(SearchThread* curThread);

	//Get work for the current thread and record the appropriate fields.
	//Returns true if actual work was obtained, else false.
	bool getWork(SearchThread* curThread);

	//Is this SplitPoint done?
	//Note that on abort, we still have to wait for numMovesStarted == numMovesDone.
	//Because we need to make sure all the threads notice the abort, before we recycle this SplitPoint.
	//As usual, the last thread out cleans up.
	bool isDone();

	//Does this SplitPoint need to be aborted? (FLAG_BETA found, beta cutoff)
	bool needsAbort();

	//Report that a child search was finished, with the given result
	//Pass in the move and move index made that obtained this result, and also the pv of the child (not including the move)
	void reportResult(eval_t result, int moveIdx, move_t move, move_t* childPv, int childPvLen, SearchThread* curThread);

	//Check if this splitpoint is aborted or any of its parents are aborted and set abortion all the way down the chain
	//to this splitpoint if abortion is found.
	//Does NOT lock parents - safe because aborted can only toggle one way.
	bool checkAborted();

};

struct SearchThread
{
	//Thread id
	int id;

	//All members are local to the thread only, and should generally only be accessed by the owning thread,
	//unless stated otherwise

	//BACK REFERCENCE TO SEARCHER---------------------------------------------------
	Searcher* searcher;

  //STATISTICS -------------------------------------------------------------------
	SearchStats stats; //Stats individual to each thread

	//BOARD HISTORY -----------------------------------------------------------------
	//Tracks data about the history of the board and moves up to the current point of search.
	int mainBoardTurnNumber; //The turn number of the mainBoard, the starting pt of the search
	BoardHistory boardHistory;

	//MOVE FEATURE DATA-------------------------------------------------------------
	vector<hash_t> featurePosDataHash;     //Hash of turnBoard from which featurePosData computed
	vector<FeaturePosData> featurePosData; //Stack of FeaturePosDatas for each turn.

	//KILLERS----------------------------------------------------------------------
	//READ BY OTHER THREADS!!!
	//This is okay because we don't really care if a race causes a killer move to be copied incorrectly
	//since it's just for ordering. But we this means we should also never reallocate the killerMoves array.
	move_t* killerMoves;  //Killer moves for each cdepth
	int killerMovesLen;   //Deepest killer move marked in the array
	int killerMovesArrayLen;  //Maximum size of the array

	//MOVE GENERATION --------------------------------------------------------------
	//Move buffers for each fdepth of search, in case we don't use a SplitPoint (such as in qsearch)
	move_t* mvList;
	int* hmList;
	int mvListCapacity;
	int mvListCapacityUsed;

	//PV REPORTING -----------------------------------------------------------------
	move_t** pv;   //Scratchwork array for intermediate pvs, indexed [fdepth][move]
	int* pvLen;    //Length of pvs in scratchwork array, indexed [fdepth]

	//THREAD STATE -----------------------------------------------------------------
	SplitPoint* curSplitPoint; //The current splitpoint we are working on
	int curSplitPointMoveIdx;
	move_t curSplitPointMove;

  bool isTerminated; //Really try to exit, leaving stuff undone. SYNCHRONIZED UNDER SEARCHTREE!
	bool isMaster; //Am I the master thread of the search?

	//The current buffer of SplitPoints that we are using when we get splitpoints
	SplitPointBuffer* curSplitPointBuffer;

	//TIME CHECK-------------------------------------------------------------------
	int timeCheckCounter;  //Incremented every mnode or qnode, for determining when to check time

	//SYNCHRONIZATION--------------------------------------------------------------
	SplitPoint* lockedSpt; //If locking or about to lock an spt, store here


	//METHODS========================================================================
	SearchThread();
	~SearchThread();

	//Initialize this search thread to be ready to search for the given root position
	//Called only once, at the start of search before any iterations
	void initRoot(int id, Searcher* searcher, const Board& b, const BoardHistory& hist, int maxCDepth);

	//Search control logic------------------------------------

	//Do I currently have work that I grabbed?
	bool hasWork();

	//Initialize or reinitialize this search thread to be ready to search from the current point
	//Called whenever we step downward in the tree and begin searching at any new splitpoint.
	//However, NOT called (and there is no corresponding function that is called) when we walk back up
	//the tree, to a parent splitpoint (such as after finishing the children).
	//Call after grabbing work, but no need for split point to be locked.
	void syncWithCurSplitPoint();

	//Called instead of syncWithCurSplitPoint whenever we begin searching at a new splitpoint that was NOT a
	//continuation of any existing splitpoint. That is, called when we begin searching at the root, as well
	//as when we jump to a different splitpoint in the tree and join in.
	//Call after grabbing work, but no need for split point to be locked.
	void syncWithCurSplitPointDistant();

	//Misc-----------------------------------------------------

	//Initialize or reinitialize this search thread to be ready to search from the given point
	//Must pass in the turn number of the initial board, so we can sync histories
	void init(SplitPoint* pt, int mainBoardTurnNumber);

	//Record any pv from the fdepth below us into the current one
	void copyExtendPVFromNextFDepth(move_t bestMove, int fDepth);

	//Record a PV of length 0 at this depth
	void endPV(int fDepth);

	//fDepth-bounds-checked way of getting the pv
	move_t* getPV(int fDepth);

	//fDepth-bounds-checked way of getting the pv
	int getPVLen(int fDepth);
};




#endif /* SEARCHTHREAD_H_ */
