/*
 * searchthread.cpp
 * Author: davidwu
 */
#include "pch.h"

#include "global.h"
#include "search.h"
#include "searchparams.h"
#include "searchthread.h"
#include "searchutils.h"
#include "arimaaio.h"

using namespace std;
using namespace ArimaaIO;


//SEARCHTREE---------------------------------------------------------------------------

SplitPointBuffer::SplitPointBuffer(Searcher* searcher, int maxfd)
{
	DEBUGASSERT(maxfd >= 0);
	maxFDepth = maxfd;
	spts = new SplitPoint[(maxfd+1)*2];
	isUsed = new bool[(maxfd+1)*2];
	isFirst = new bool[(maxfd+1)*2];

	for(int depth = 0; depth <= maxfd; depth++)
	{
		spts[2*depth].searcher = searcher;
		spts[2*depth+1].searcher = searcher;
		spts[2*depth].fDepth = depth;
		spts[2*depth+1].fDepth = depth;
		spts[2*depth].buffer = this;
		spts[2*depth+1].buffer = this;
		spts[2*depth].bufferIdx = 2*depth;
		spts[2*depth+1].bufferIdx = 2*depth+1;

		isUsed[2*depth] = false;
		isUsed[2*depth+1] = false;

		isFirst[2*depth] = false;
		isFirst[2*depth+1] = false;
	}
}

SplitPointBuffer::~SplitPointBuffer()
{
	delete[] spts;
	delete[] isUsed;
	delete[] isFirst;
}

void SplitPointBuffer::assertEverythingUnused()
{
	for(int depth = 0; depth <= maxFDepth; depth++)
	{
		DEBUGASSERT(!isUsed[2*depth]);
		DEBUGASSERT(!isUsed[2*depth+1]);
	}
}

SplitPoint* SplitPointBuffer::acquireSplitPoint(SplitPoint* parent)
{
	int fDepth = (parent == NULL) ? 0 : parent->fDepth + 1;
	DEBUGASSERT(fDepth <= maxFDepth);

	int idx = fDepth*2;

	if(isUsed[idx])
	{
		DEBUGASSERT(!isUsed[idx+1]);
		idx += 1;
	}

	SplitPoint* spt = &(spts[idx]);

	isUsed[idx] = true;
	isFirst[idx] = (parent == NULL || parent->buffer != this);
	spt->parent = parent;

	DEBUGASSERT(!spt->isPublic);
	DEBUGASSERT(!spt->isAborted);
	DEBUGASSERT(!spt->isInitialized);

	return spt;
}

bool SplitPointBuffer::freeSplitPoint(SplitPoint* spt)
{
	//Ensure it's actually from this buffer!
	int idx = spt->bufferIdx;
	DEBUGASSERT(&spts[idx] == spt);
	DEBUGASSERT(!spt->isPublic);

	bool isEmpty = isFirst[idx];
	isUsed[idx] = false;
	isFirst[idx] = false;

	spt->parent = NULL;
	spt->isAborted = false;
	spt->isInitialized = false;

	return isEmpty;
}

//--------------------------------------------------------------------------------------------------

SearchTree::SearchTree(Searcher* s, int numThr, int maxMSearchDepth, int maxCDepth, const Board& b, const BoardHistory& hist)
{
	if(numThr <= 0 || numThr > SearchParams::MAX_THREADS)
		Global::fatalError(string("Invalid number of threads: ") + Global::intToString(numThr));

	searcher = s;
	numThreads = numThr;
	searchDone = false;
	iterationGoing = false;
	didTimeout = false;

	threads = new SearchThread[numThreads];
	threads[0].isMaster = true;
	for(int i = 0; i<numThreads; i++)
		threads[i].initRoot(i,searcher,b,hist,maxCDepth);

	//Overestimated worst case - each thread has an empty buffer, plus each other buffer in use is abandoned and
	//contains exactly one splitpoint
	initialNumFreeSptBufs = numThr + numThr*(maxMSearchDepth+1);
	numFreeSptBufs = initialNumFreeSptBufs;
	freeSptBufs = new SplitPointBuffer*[numFreeSptBufs];
	//The max fdepth of splitpoint we need is <= maxMsearchDepth since each fdepth must advance at least one msearch depth.
	int maxSplitPointFDepthNeeded = maxMSearchDepth;
	for(int i = 0; i<numFreeSptBufs; i++)
		freeSptBufs[i] = new SplitPointBuffer(s,maxSplitPointFDepthNeeded);
	rootSptBuf = new SplitPointBuffer(s,0);

	rootNode = NULL;

	//Circularly doubly linked list
	publicizedListHead = new SplitPoint();
	publicizedListHead->publicNext = publicizedListHead;
	publicizedListHead->publicPrev = publicizedListHead;

	iterationNumWaiting = 0;

	//Create a bunch of threads and start them going
	boostThreads = new boost::thread[numThreads];

#ifdef MULTITHREADING
	for(int i = 1; i<numThreads; i++)
		boostThreads[i] = boost::thread(&runChild,this,searcher,&threads[i]);
#endif
}

SearchTree::~SearchTree()
{
	mutex.lock();
	DEBUGASSERT(!iterationGoing);
	DEBUGASSERT(iterationNumWaiting == numThreads-1);
	DEBUGASSERT(didTimeout || initialNumFreeSptBufs == numFreeSptBufs);
	DEBUGASSERT(didTimeout || publicizedListHead->publicNext == publicizedListHead);
	DEBUGASSERT(didTimeout || publicizedListHead->publicPrev == publicizedListHead);
	DEBUGASSERT(rootNode == NULL);

	//Mark search done and open the way for threads to exit
	searchDone = true;
	iterationCondvar.notify_all();
	mutex.unlock();

	//Wait for them all to exit
	for(int i = 1; i<numThreads; i++)
		boostThreads[i].join();

	DEBUGASSERT(iterationNumWaiting == 0);

	//Clean up memory
	delete publicizedListHead;
	delete[] threads;
	delete[] boostThreads;

	for(int i = 0; i<numFreeSptBufs; i++)
		delete freeSptBufs[i];
	delete[] freeSptBufs;

	delete rootSptBuf;
}

SplitPoint* SearchTree::acquireRootNode()
{
  DEBUGASSERT(rootNode == NULL);
	rootNode = rootSptBuf->acquireSplitPoint(NULL);
	return rootNode;
}

void SearchTree::freeRootNode(SplitPoint* spt)
{
	depublicize(spt);
	DEBUGASSERT(rootNode == spt);
	rootSptBuf->freeSplitPoint(spt);
	rootNode = NULL;
}

//Start one iteration of iterative deepening. Call from master thread before starting.
void SearchTree::beginIteration()
{
	boost::lock_guard<boost::mutex> lock(mutex);
	DEBUGASSERT(!didTimeout); //Cannot start search again if timed out
	iterationGoing = true;
	iterationCondvar.notify_all();
}

//End one iteration of iterative deepening, call from inside the main loop when the first thread finds itself done
//with the root node.
void SearchTree::endIterationInternal()
{
	boost::unique_lock<boost::mutex> lock(mutex);
	iterationGoing = false;

	//Notify threads waiting for public work to exit search
	publicWorkCondvar.notify_all();
}

//End one iteration of iterative deepening, signal threads to exit the main loop. Call from master
//thread after an iteration, including after timeout
void SearchTree::endIteration()
{
	DEBUGASSERT(!iterationGoing);

	boost::unique_lock<boost::mutex> lock(mutex);
	//Wait for all threads to exit the main running loop
	while(iterationNumWaiting != numThreads-1)
		iterationMasterCondvar.wait(lock);

	DEBUGASSERT(didTimeout || initialNumFreeSptBufs == numFreeSptBufs);
	ARIMAADEBUG(
		if(!didTimeout)
			for(int i = 0; i<numFreeSptBufs; i++)
				freeSptBufs[i]->assertEverythingUnused();
	);
}

//Call from within the main loop by any thread (or from externally) to report timeout, and that threads
//should exit the main loop
void SearchTree::timeout(SearchThread* curThread)
{
	boost::lock_guard<boost::mutex> lock(mutex);
	iterationGoing = false;
	didTimeout = true;

	rootNode->lock(curThread);
	for(int i = 0; i<numThreads; i++)
	  threads[i].isTerminated = true;
	rootNode->unlock(curThread);

	//TODO maybe this way is better? Beware - this could interfere with collecting stats,
	// determining if we should use the PV from the next iteration, etc.
	//Help everyone to quit in a natural way by aborting the root node.
  //rootNode->lock(curThread);
  //rootNode->isAborted = true;
  //rootNode->unlock(curThread);

  //Notify threads waiting for public work to exit search
	publicWorkCondvar.notify_all();
}

//Get a new buffer of splitpoints for this thread to use during search.
SplitPointBuffer* SearchTree::acquireSplitPointBuffer()
{
	boost::lock_guard<boost::mutex> lock(mutex);
	DEBUGASSERT(numFreeSptBufs > 0);

	numFreeSptBufs--;
	SplitPointBuffer* buf = freeSptBufs[numFreeSptBufs];
	freeSptBufs[numFreeSptBufs] = NULL;

	return buf;
}

//Free an unused buffer of splitpoints that has been disowned, or one's own buffer
void SearchTree::freeSplitPointBuffer(SplitPointBuffer* buf)
{
	boost::lock_guard<boost::mutex> lock(mutex);
	freeSptBufs[numFreeSptBufs] = buf;
	numFreeSptBufs++;

	DEBUGASSERT(numFreeSptBufs <= initialNumFreeSptBufs);
}

//Publicize the SplitPoint so that other threads can help
//The SplitPoint must be initialized with board data before this is called!
void SearchTree::publicize(SplitPoint* spt)
{
	boost::lock_guard<boost::mutex> lock(mutex);

	DEBUGASSERT(spt->isInitialized);
	DEBUGASSERT(!spt->isPublic);
	DEBUGASSERT(spt->publicNext == NULL);
	DEBUGASSERT(spt->publicPrev == NULL);
	spt->isPublic = true;

	//Insert into doubly linked public list
	spt->publicNext = publicizedListHead->publicNext;
	spt->publicPrev = publicizedListHead;
	publicizedListHead->publicNext->publicPrev = spt;
	publicizedListHead->publicNext = spt;

	publicWorkCondvar.notify_all();
}

//Depublicize the SplitPoint. Call before this splitpoint back to buffer.
//Okay to call for splitpoints that aren't public
void SearchTree::depublicize(SplitPoint* spt)
{
	//if(spt->isPublic) //TODO is this double check a good idea?
	boost::lock_guard<boost::mutex> lock(mutex); //TODO is this lock necessary?
	if(spt->isPublic)
	{
		DEBUGASSERT(spt->publicNext != NULL);
		DEBUGASSERT(spt->publicPrev != NULL);

		//Remove from doubly linked list
		spt->publicNext->publicPrev = spt->publicPrev;
		spt->publicPrev->publicNext = spt->publicNext;
		spt->publicNext = NULL;
		spt->publicPrev = NULL;
		spt->isPublic = false;
	}
}

//Find a good publicized SplitPoint and get work from it for the given thread.
//If none, blocks until there is some. Threads only return without work when
//the iteration of search is done or has been terminated (including on timeout).
void SearchTree::getPublicWork(SearchThread* curThread)
{
	DEBUGASSERT(curThread->lockedSpt == NULL);
	DEBUGASSERT(!curThread->hasWork());
	boost::unique_lock<boost::mutex> lock(mutex);

	while(true)
	{
		//We're done, folks
		if(!iterationGoing || curThread->isTerminated)
		{DEBUGASSERT(!curThread->hasWork()); return;}

		SplitPoint* bestSpt = NULL;
		for(SplitPoint* spt = publicizedListHead->publicNext; spt != publicizedListHead; spt = spt->publicNext)
		{
			spt->lock(curThread);

			//Get the first public splitpoint with work
			if(spt->getWork(curThread))
			{
				//Woohoo, we got work!
				bestSpt = spt;
				spt->unlock(curThread);
				curThread->syncWithCurSplitPointDistant();
				curThread->stats.publicWorkRequests++;
				curThread->stats.publicWorkDepthSum += spt->cDepth;
				break;
			}
			spt->unlock(curThread);
		}

		//Found work!
		if(bestSpt != NULL)
		{DEBUGASSERT(curThread->hasWork()); return;}

		//Don't have to check if(!iterationGoing) because we've held the mutex the whole time and checked at the top
		publicWorkCondvar.wait(lock);
	}
}

//Return the master thread
SearchThread* SearchTree::getMasterThread()
{
	return &threads[0];
}

//Get stats from all threads combined together
SearchStats SearchTree::getCombinedStats()
{
	SearchStats stats;
	for(int i = 0; i<numThreads; i++)
		stats += threads[i].stats;
	return stats;
}


void SearchTree::runChild(SearchTree* tree, Searcher* searcher, SearchThread* curThread)
{
	boost::unique_lock<boost::mutex> lock(tree->mutex);
	while(true)
	{
		if(tree->searchDone)
			break;
		else if(tree->iterationGoing)
		{
		  DEBUGASSERT(!(curThread->isTerminated));
		  lock.unlock();
			searcher->mainLoop(curThread);
			lock.lock();
		}
		else
		{
			tree->iterationNumWaiting++;

			//If all the threads are here waiting, notify the master that we're ready
			//All threads should be guaranteed to wait here at the start
			if(tree->iterationNumWaiting == tree->numThreads-1)
				tree->iterationMasterCondvar.notify_all();

			tree->iterationCondvar.wait(lock);
			tree->iterationNumWaiting--;
		}
	}
}

void SearchTree::copyKillersUnsynchronized(int fromThreadId, move_t* killerMoves, int& killerMovesLen)
{
  SearchThread* otherThread = &threads[fromThreadId];
  int len = otherThread->killerMovesLen;
  for(int i = 0; i<len; i++)
    killerMoves[i] = otherThread->killerMoves[i];
  killerMovesLen = len;
}

//SPLITPOINT ---------------------------------------------------------------------

SplitPoint::SplitPoint()
{
	searcher = NULL;
	fDepth = 0;
	buffer = NULL;
	bufferIdx = 0;

	publicNext = NULL;
	publicPrev = NULL;
	isPublic = false;

	parent = NULL;
	isAborted = false;

	isInitialized = false;
	//board = Board()

	moveHistory = NULL;
	moveHistoryCapacity = 0;
	moveHistoryLen = 0;

	parentMoveIndex = -1;
	parentMove = ERRORMOVE;
	searchMode = MODE_INVAL;
	changedPlayer = false;

	cDepth = 0;
	rDepth4 = 0;

	nodeType = TYPE_INVAL;
	nodeAllness = 0;

	alpha = 0;
	beta = 0;

	bestEval = Eval::LOSE-1;
	bestMove = ERRORMOVE;
	bestFlag = Flags::FLAG_NONE;
	bestIndex = -1;

	pv = new move_t[SearchParams::PV_ARRAY_SIZE];;
	pvLen = 0;

	mv = NULL;
	hm = NULL;
	mvCapacity = 0;
	numMoves = 0;

	areMovesUngen = true;
	numMovesStarted = 0;
	numMovesDone = 0;
	hashMoveIndex = -1;
	killerMoveIndex = -1;

	copyKillersFromThread = -1;

	pvsThreshold = 0x3FFFFFFF;
	r1Threshold = 0x3FFFFFFF;
	r2Threshold = 0x3FFFFFFF;
	pruneThreshold = 0x3FFFFFFF;

}

SplitPoint::~SplitPoint()
{
	delete[] moveHistory;
	delete[] mv;
	delete[] hm;
	delete[] pv;
}

void SplitPoint::init(int pMoveIdx, move_t pMove, int sMode, const Board& b, bool chgPlyr,
		int cD, int rD4, eval_t al, eval_t bt, bool isRoot, move_t hashMove, move_t killerMove, int moveArraySize,
		int copyKillersFrom)
{
	DEBUGASSERT(!isInitialized);

	board = b;

	//Handle move history
	SearchUtils::ensureMoveHistoryArray(moveHistory,moveHistoryCapacity,fDepth+1);

	//Root node - no move history
	if(parent == NULL)
	{
		moveHistoryLen = 0;
		if(pMove != ERRORMOVE) {moveHistory[0] = pMove; moveHistoryLen++;}
	}
	//Child of root node
	else if(parent->moveHistoryLen == 0)
	{
		moveHistory[0] = pMove;
		moveHistoryLen = 1;
	}
	//Other node, or child of root node but root node has history (search didn't start on step 0)
	else
	{
		//Copy over move history except the last move
		moveHistoryLen = parent->moveHistoryLen;
		for(int i = 0; i<moveHistoryLen-1; i++)
			moveHistory[i] = parent->moveHistory[i];

		//See if we need to compact the last move
		//Last move was full? So no compacting
		int parentBoardStep = parent->board.step;
		if(parentBoardStep == 0)
		{
			moveHistory[moveHistoryLen-1] = parent->moveHistory[moveHistoryLen-1];
			moveHistory[moveHistoryLen] = pMove;
			moveHistoryLen++;
		}
		//Else compact it
		else
		{
			moveHistory[moveHistoryLen-1] = Board::concatMoves(parent->moveHistory[moveHistoryLen-1],pMove,parentBoardStep);
		}
	}

	parentMoveIndex = pMoveIdx;
	parentMove = pMove;
	searchMode = sMode;
	changedPlayer = chgPlyr;

	cDepth = cD;
	rDepth4 = rD4;

	alpha = al;
	beta = bt;

	//bestEval = Eval::LOSE + cDepth; //If we have nothing to do, we lose immediately
	//TODO the commented line sometimes creates problems. If we freecapture prune
	//and all the moves at a step get pruned, due to free capture, then the eval will be
	//better than an illegality terminal eval, so we record a losing value in the hash.
	//If we later trust this losing value... can bad things happen?
	//TODO grep for Eval::LOSE-1 because there are other places where we assume no legal move means this score
	bestEval = Eval::LOSE-1;
	bestMove = ERRORMOVE;
	bestFlag = Flags::FLAG_ALPHA;
	bestIndex = -1;

	pvLen = 0;

	SearchUtils::ensureMoveArray(mv,hm,mvCapacity,moveArraySize);
	int nMoves = 0;
	if(hashMove != ERRORMOVE)
	{
		mv[nMoves] = hashMove;
		hm[nMoves] = SearchParams::HASH_SCORE;
		hashMoveIndex = nMoves;
		nMoves++;
	}
	else
		hashMoveIndex = -1;
	if(killerMove != ERRORMOVE && killerMove != hashMove)
	{
		DEBUGASSERT(SearchParams::KILLER_ENABLE || SearchParams::QKILLER_ENABLE)
		mv[nMoves] = killerMove;
		hm[nMoves] = SearchParams::KILLER_SCORE;
		killerMoveIndex = nMoves;
		nMoves++;
	}
	else
		killerMoveIndex = -1;

	numMoves = nMoves;

	areMovesUngen = true;
	numMovesStarted = 0;
	numMovesDone = 0;

	copyKillersFromThread = copyKillersFrom;

	pvsThreshold = 0x3FFFFFFF;
	r1Threshold = 0x3FFFFFFF;
	r2Threshold = 0x3FFFFFFF;
	pruneThreshold = 0x3FFFFFFF;

	//Node type estimation
	initNodeType(isRoot);

	isInitialized = true;
}

void SplitPoint::initNodeType(bool isRoot)
{
	nodeAllness = 0; //TODO unused

	if(isRoot)
		nodeType = TYPE_ROOT;
	else if(parentMoveIndex == 0 && (parent->nodeType == TYPE_ROOT || parent->nodeType == TYPE_PV))
		nodeType = TYPE_PV;
	else if(parent->nodeType != TYPE_ALL)
		nodeType = TYPE_ALL;
	else
		nodeType = TYPE_CUT;
}

void SplitPoint::lock(SearchThread* thread)
{
	DEBUGASSERT(thread->lockedSpt == NULL);
	thread->lockedSpt = this;
	mutex.lock();
}

void SplitPoint::unlock(SearchThread* thread)
{
	mutex.unlock();
	DEBUGASSERT(thread->lockedSpt == this);
	thread->lockedSpt = NULL;
}

//Does this SplitPoint need to be publicized now for parallelization?
bool SplitPoint::needsPublication()
{
	//TODO parallelize when? After killer move? After areMovesUnGen is false?
	return !isPublic && numMovesDone >= 1;
}

//Add all the fsearch moves.
void SplitPoint::addFSearchMoves(const SearchThread* curThread)
{
	DEBUGASSERT(areMovesUngen && numMoves == 0);
	areMovesUngen = false;

	numMoves += curThread->searcher->genFSearchMoves(curThread,board,cDepth,rDepth4,mv,hm,
			pvsThreshold,r1Threshold,r2Threshold,pruneThreshold);
}

//Add all the msearch moves.
//SearchThread is used purely for stats like killer table, etc.
void SplitPoint::addMSearchMoves(SearchThread* curThread)
{
	DEBUGASSERT(areMovesUngen && numMoves >= 0 && numMoves <= 2);
	areMovesUngen = false;

	int oldNumMoves = numMoves;
	numMoves += curThread->searcher->genMSearchMoves(curThread,board,cDepth,rDepth4,oldNumMoves,mv+oldNumMoves,hm+oldNumMoves,
			pvsThreshold,r1Threshold,r2Threshold,pruneThreshold);

	//Eliminate the hashmove, killer move, etc, if it already exists
	for(int j = 0; j<oldNumMoves; j++)
	{
		int i;
		for(i = oldNumMoves; i < numMoves; i++)
			if(mv[i] == mv[j])
				break;

		//Found, already exists, so remove it
		if(i < numMoves)
		{
			for(; i<numMoves-1; i++)
			{
				mv[i] = mv[i+1];
				hm[i] = hm[i+1];
			}
			numMoves--;
		}
	}
}

//Check if this splitpoint is aborted or any of its parents are aborted and set abortion all the way down the chain
//to this splitpoint if abortion is found.
//Does NOT lock parents - safe because aborted can only toggle one way.
bool SplitPoint::checkAborted()
{
	SplitPoint* spt = this;
	while(spt != NULL && !spt->isAborted)
		spt = spt->parent;

	if(spt != NULL)
	{
		SplitPoint* spt2 = this;
		while(spt2 != spt)
		{
			spt2->isAborted = true;
			spt2 = spt2->parent;
		}
		return true;
	}
	return false;
}

//Does this have work to be done still?
//May return an ERRORMOVE, if there actually wasn't any work. This might rarely occur when areMovesUngen is actually the only
//legal move, or some such event like this. Returns true if actual work was obtained, else false.
//SearchThread is used purely for stats like killer table, etc.
bool SplitPoint::getWork(SearchThread* curThread)
{
	if(checkAborted() || bestFlag == Flags::FLAG_BETA)
	{
		curThread->stats.threadAborts++;
		curThread->curSplitPointMoveIdx = -1;
		curThread->curSplitPointMove = ERRORMOVE;
		curThread->curSplitPoint = NULL;
		return false;
	}

	int idx = numMovesStarted;
	DEBUGASSERT(idx >= 0);
	if(idx >= numMoves && areMovesUngen)
		addMSearchMoves(curThread);

	if(idx >= numMoves)
	{
		curThread->curSplitPointMoveIdx = -1;
		curThread->curSplitPointMove = ERRORMOVE;
		curThread->curSplitPoint = NULL;
		return false;
	}

	curThread->curSplitPointMoveIdx = idx;
	if(nodeType == SplitPoint::TYPE_ROOT)
		curThread->curSplitPointMove = curThread->searcher->getRootMove(mv,hm,numMoves,idx);
	else
		curThread->curSplitPointMove = curThread->searcher->getMove(mv,hm,numMoves,idx);
	curThread->curSplitPoint = this;

	numMovesStarted++;

	return true;
}


//Is this SplitPoint done?
//Note that on abort, we still have to wait for numMovesStarted == numMovesDone.
//Because we need to make sure all the threads notice the abort, before we recycle this SplitPoint.
//As usual, the last thread out cleans up.
bool SplitPoint::isDone()
{
	return numMovesStarted == numMovesDone && (numMovesDone == numMoves || checkAborted());
}

//Does this SplitPoint need to be aborted?
bool SplitPoint::needsAbort()
{
	return !isAborted && bestFlag == Flags::FLAG_BETA;
}

void SplitPoint::reportResult(eval_t eval, int moveIdx, move_t move, move_t* childPv, int childPvLen, SearchThread* curThread)
{
	DEBUGASSERT(move != ERRORMOVE);

	numMovesDone++;
	DEBUGASSERT(numMovesDone <= numMoves);

	if(checkAborted() || bestFlag == Flags::FLAG_BETA)
		return;

	if(searcher->params.viewing(board) && childPv != NULL) //Could be null if exceeded max pv array len
		searcher->params.viewMove(board, move, hm[moveIdx], eval, bestEval, childPv, childPvLen, curThread->stats);

	//Beta cutoff
	if(eval >= beta)
	{
		curThread->stats.betaCuts++;

		bestMove = move;
		bestEval = eval;
		bestFlag = Flags::FLAG_BETA;
		bestIndex = moveIdx;

		//Unconditional copy extend, for killer moves
		//childPv might be null here and in other copyExtendPVs, but only when childPvLen is also 0, so it's okay
    SearchUtils::copyExtendPV(move,childPv,childPvLen,pv,pvLen);
		return;
	}

	//Update best and alpha
	if(eval > bestEval)
	{
		bestMove = move;
		bestEval = eval;
		bestIndex = moveIdx;

		if(eval > alpha)
		{
			alpha = eval;
			bestFlag = Flags::FLAG_EXACT;
      SearchUtils::copyExtendPV(move,childPv,childPvLen,pv,pvLen);

			//Store best in hm for root node, for root move ordering
			if(nodeType == TYPE_ROOT)
			{
				hm[moveIdx] = eval;
				if(searcher->doOutput)
				{
				  double floatdepth = rDepth4/SearchParams::DEPTH_DIV - 1 + (double)(numMovesDone-1)/numMoves;
				  double timeUsed = searcher->currentTimeUsed();
				  double desiredTime = searcher->currentDesiredTime();
	        cout << Global::strprintf("FS Depth: +%-5.4g Eval: % 6d Time: (%.2f/%.2f)", floatdepth, eval, timeUsed, desiredTime)
				       << " PV: " << writeMoves(board,pv,pvLen) << endl;
				}
			}
		}
		else if(searcher->params.viewOn)
			SearchUtils::copyExtendPV(move,childPv,childPvLen,pv,pvLen);
	}

  if(nodeType == TYPE_ROOT)
    searcher->updateDesiredTime(eval,rDepth4/4,numMovesDone,numMoves);
}


//SEARCHTHREAD --------------------------------------------------------

SearchThread::SearchThread()
{
	id = -1;
	searcher = NULL;

	pv = new move_t*[SearchParams::PV_ARRAY_SIZE];
	for(int i = 0; i<SearchParams::PV_ARRAY_SIZE; i++)
		pv[i] = new move_t[SearchParams::PV_ARRAY_SIZE];
	pvLen = new int[SearchParams::PV_ARRAY_SIZE];

	mainBoardTurnNumber = 0;
	curSplitPoint = NULL;
	curSplitPointMoveIdx = -1;
	curSplitPointMove = ERRORMOVE;
	isTerminated = false;
	isMaster = false;
	timeCheckCounter = 0;
	lockedSpt = NULL;

	mvListCapacity = SearchParams::QMAX_FDEPTH * SearchParams::QSEARCH_MOVE_CAPACITY;
	mvList = new move_t[mvListCapacity];
	hmList = new int[mvListCapacity];
	mvListCapacityUsed = 0;
}

SearchThread::~SearchThread()
{
	delete[] mvList;
	delete[] hmList;

	delete[] killerMoves;

	for(int i = 0; i<SearchParams::PV_ARRAY_SIZE; i++)
		delete[] pv[i];
	delete[] pv;
	delete[] pvLen;
}

//Initialize this search thread to be ready to search for the given root position
void SearchThread::initRoot(int i, Searcher* s, const Board& b, const BoardHistory& hist, int maxCDepth)
{
	id = i;
	searcher = s;
	mainBoardTurnNumber = b.turnNumber;
	boardHistory = hist;
	curSplitPoint = NULL;
	isTerminated = false;
	timeCheckCounter = 0;

	int killerLen = maxCDepth+1;
	killerMoves = new move_t[killerLen];
	for(int j = 0; j<killerLen; j++)
	  killerMoves[j] = ERRORMOVE;
	killerMovesLen = 0;
	killerMovesArrayLen = killerLen;
}

bool SearchThread::hasWork()
{
	return curSplitPoint != NULL;
}

//Initialize or reinitialize this search thread to be ready to search from the current point
//Called whenever we step downward in the tree and begin searching at any new splitpoint.
//Call after getting work, but no need for split point to be locked.
//However, NOT called (and there is no corresponding function that is called) when we walk back up
//the tree, to a parent splitpoint (such as after finishing the children).
void SearchThread::syncWithCurSplitPoint()
{
	DEBUGASSERT(curSplitPoint != NULL);

	int startTurn = mainBoardTurnNumber;
	int diffTurn = curSplitPoint->moveHistoryLen;

	//Sync board histories
	DEBUGASSERT(startTurn >= boardHistory.minTurnNumber);
	int i;
	for(i = 0; i<diffTurn; i++)
	{
		if(startTurn+i > boardHistory.maxTurnNumber || boardHistory.turnMove[startTurn+i] != curSplitPoint->moveHistory[i])
			break;
	}

	//Histories differ from here on, so sync them
	for(; i<diffTurn; i++)
		boardHistory.reportMove(startTurn+i, curSplitPoint->moveHistory[i]);
}

//Called instead of syncWithCurSplitPoint whenever we begin searching at a new splitpoint that was NOT a
//continuation of any existing splitpoint. That is, called when we begin searching at the root, as well
//as when we jump to a different splitpoint in the tree and join in.
//Call after getting work, but no need for split point to be locked.
void SearchThread::syncWithCurSplitPointDistant()
{
  //Copy killer moves from the appropriate thread
  int killerMovesThreadId = curSplitPoint->copyKillersFromThread;
  if(killerMovesThreadId != id)
  {
    if(killerMovesThreadId >= 0)
      searcher->searchTree->copyKillersUnsynchronized(killerMovesThreadId,killerMoves,killerMovesLen);
    else
      killerMovesLen = 0;
  }
	syncWithCurSplitPoint();
}

//Record any pv from the fdepth below us into the current one
void SearchThread::copyExtendPVFromNextFDepth(move_t bestMove, int fDepth)
{
  if(fDepth < SearchParams::PV_ARRAY_SIZE - 1)
    SearchUtils::copyExtendPV(bestMove,pv[fDepth+1],pvLen[fDepth+1],pv[fDepth],pvLen[fDepth]);
  else if(fDepth == SearchParams::PV_ARRAY_SIZE - 1)
	{
		pv[fDepth][0] = bestMove;
		pvLen[fDepth] = 1;
	}
}

void SearchThread::endPV(int fDepth)
{
	if(fDepth >= SearchParams::PV_ARRAY_SIZE)
		return;
	SearchUtils::endPV(pv[fDepth], pvLen[fDepth]);
}

move_t* SearchThread::getPV(int fDepth)
{
	if(fDepth >= SearchParams::PV_ARRAY_SIZE)
		return NULL;
	return pv[fDepth];
}

int SearchThread::getPVLen(int fDepth)
{
	if(fDepth >= SearchParams::PV_ARRAY_SIZE)
		return 0;
	return pvLen[fDepth];
}


//MAIN LOGIC---------------------------------------------------------------------------

//Free SplitPoint and handle buffer return
void Searcher::freeSplitPoint(SearchThread* curThread, SplitPoint* spt)
{
	//SplitPoint is from our buffer
	SplitPointBuffer* buf = spt->buffer;
	if(buf == curThread->curSplitPointBuffer)
	{
		//Free it and do nothing else
		buf->freeSplitPoint(spt);
	}
	//SplitPoint is not from our buffer
	//Presumably this buffer has been disowned then
	else
	{
		bool isEmpty = buf->freeSplitPoint(spt);
		//Return empty buffers to SearchTree
		if(isEmpty)
			searchTree->freeSplitPointBuffer(buf);
	}
}

//Begin a parallel search on a root split point. Call this after retrieving the split point from the search tree
//and calling init on it
void Searcher::parallelFSearch(SearchThread* curThread, SplitPoint* spt)
{
	DEBUGASSERT(spt->nodeType == SplitPoint::TYPE_ROOT);
	DEBUGASSERT(searchTree != NULL);
	DEBUGASSERT(curThread == searchTree->getMasterThread());

	//Add the moves
	spt->addFSearchMoves(curThread);

	//Get work and make sure that there actually is work
	spt->lock(curThread);
	bool hasWork = spt->getWork(curThread);
	spt->unlock(curThread);
	if(!hasWork)
		return;

	curThread->syncWithCurSplitPointDistant();

	//Go!
	searchTree->beginIteration();
	mainLoop(curThread);
	searchTree->endIteration();
}

void Searcher::mainLoop(SearchThread* curThread)
{
	//Get split point buffer
	curThread->curSplitPointBuffer = searchTree->acquireSplitPointBuffer();

	while(true)
	{
	  //This check provides a little bit faster reaction to termination, but is not strictly necessary
	  //(redundant with the check at the bottom of this loop)
	  if(curThread->isTerminated)
	    break;

		//Thread has current splitpoint working on?
		if(curThread->hasWork())
		{
			work(curThread);
			continue;
		}
		//Find good split point, or go to sleep if there is none. On wakeup, will return so we can run this loop again.
	  searchTree->getPublicWork(curThread);

	  //Search must be over?
	  if(!curThread->hasWork())
	  	break;
	}

	//Free split point buffer, give back to tree
	searchTree->freeSplitPointBuffer(curThread->curSplitPointBuffer);
	curThread->curSplitPointBuffer = NULL;
}

//Work as the current thread
void Searcher::work(SearchThread* curThread)
{
	int moveIdx = curThread->curSplitPointMoveIdx;
	move_t move = curThread->curSplitPointMove;

	//Perform the desired work
	SplitPoint* spt = curThread->curSplitPoint;
	SplitPoint* newSpt = NULL;
	eval_t result = Eval::LOSE-1;
	mSearch(curThread,spt,newSpt,result);

	//No result yet, generated a new SplitPoint to work on
	if(newSpt != NULL)
	{
		newSpt->lock(curThread);
		bool hasWork = newSpt->getWork(curThread);
		newSpt->unlock(curThread);

		//Got new work from the new splitpoint, let's go!
		if(hasWork)
		{
			curThread->syncWithCurSplitPoint();

			//No rush to check for termination - there's a check after we return
			return;
		}

		//Generated a new SplitPoint but it doesn't have any work!
		//Note: Probably the only time this happens is if we're aborted, so long as
		//msearch does the immobilization check properly
		//Free it right away and report a losing eval for it
		result = Eval::LOSE-1;
		if(spt->changedPlayer)
			result = -result;

		freeSplitPoint(curThread,newSpt);
		newSpt = NULL;
	}

	//Result!
	while(true)
	{
		//Guards reportResult, so that if we're terminated, we won't report any bad results.
		//If this changes on us after this, it's okay, because if it was good up to this point, then
		//we obtained the result while it was good, and therefore is trustworthy for recording.
		if(curThread->isTerminated)
			return;

		//Lock the splitpoint
		spt->lock(curThread);

		//Report the result
		int fDepth = spt->fDepth;
		move_t* childPV = NULL;
		int childPVLen = 0;
		if(fDepth+1 < SearchParams::PV_ARRAY_SIZE)
		{childPV = curThread->pv[fDepth+1]; childPVLen = curThread->pvLen[fDepth+1];}
		spt->reportResult(result,moveIdx,move,childPV,childPVLen,curThread);

		//Beta cutoff?
		if(spt->needsAbort())
		{
			curThread->stats.threadAborts--; //Decrement because we're the one aborting, to cancel the increment when getWork fails below
			curThread->stats.abortedBranches += spt->numMovesStarted - moveIdx; //Every branch started beyond this one was wasted
			spt->isAborted = true;
		}

		//If there's more work to be done. then do it.
		if(spt->getWork(curThread))
		{
			//Does it need to be publicized?
			bool needsPublication = spt->needsPublication();
			spt->unlock(curThread);

			if(needsPublication)
				searchTree->publicize(spt);

			return;
		}

		//No more work for us, but if it's not done then another thread must still be working on it.
		//Then we have nothing to do, so look for work elsewhere.
		if(!spt->isDone())
		{
			//Is this SplitPoint from our buffer?
			bool fromOwnBuffer = (spt->buffer == curThread->curSplitPointBuffer);
			spt->unlock(curThread);

			//If so, disown it, someone else will clean it up when this SplitPoint is done.
			//We can outright drop the pointer because others can access it through the splitpoint storing
			//a reference to its own buffer
			if(fromOwnBuffer)
				curThread->curSplitPointBuffer = searchTree->acquireSplitPointBuffer(); //Disown old buffer, get new one

			return;
		}

		//It's actually done now, so we can unlock it. We're not going to modify it any more, and nobody else
		//should mess with it in the meantime, because it has no work left
		spt->unlock(curThread);

		//Depublicize the splitpoint
		searchTree->depublicize(spt);

		//If it's the root, then we're all the way done
		if(spt->parent == NULL)
		{
			DEBUGASSERT(spt->nodeType == SplitPoint::TYPE_ROOT);
			searchTree->endIterationInternal();
			return;
		}

		//Check back with the search functions to see if we need to keep working
		//(due to LMR, etc) or if we get a result for the parent.
		SplitPoint* nextSpt = NULL;
		eval_t nextResult = Eval::LOSE-1;
		int nextMoveIdx = spt->parentMoveIndex;
		move_t nextMove = spt->parentMove;
		mSearchDone(curThread, spt, nextSpt, nextResult);

		//Got a new SplitPoint, start working on it
		if(nextSpt != NULL)
		{
			nextSpt->lock(curThread);
			//Got work from new SplitPoint?
			if(nextSpt->getWork(curThread))
			{
				nextSpt->unlock(curThread);

				//We can unlock and free the old spt, it's done
				freeSplitPoint(curThread,spt);

				//Sync with the new one
				curThread->syncWithCurSplitPoint();
				return;
			}
			//We can fall into this case if we got aborted from above.
			//I suppose, if mSearchDone is weird, we could also fall here
			//if it returned a new search with no legal moves.
			else
			{
				//Return a losing score
				nextResult = Eval::LOSE-1;
				//Free the splitpoint immediately
				//TODO if we ever want mSearchDone to fail with no legal moves
				//but for there to be ANOTHER stage with actual legal moves or eval
				//then this will be wrong.
				nextSpt->unlock(curThread);
				freeSplitPoint(curThread,nextSpt);
			}
		}

		//Flip eval due to player change as we walk back up the tree
		if(spt->changedPlayer)
			nextResult = -nextResult;

		//We can free the current spt, it's done
		SplitPoint* parent = spt->parent;
		freeSplitPoint(curThread,spt);

		//We got an actual result. Propagate this to the parent!
		result = nextResult;
		moveIdx = nextMoveIdx;
		move = nextMove;
		spt = parent;
	}
}



