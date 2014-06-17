#include <string>
#include <iostream>
#include <list>
#include <map>
#include <fstream>
#include <vector>

using namespace std;

#define UINT32 unsigned int
#define UINT64 long long 
#define ADDRINT unsigned int


enum Region
{
	FRAME = 0,
	HEAP,
	GLOBAL
};
struct MemBlock;
struct Object
{
	UINT32 _nID;
	UINT32 _nSize;
	Region _region;
	MemBlock* _block;
	std::map<UINT32, UINT64> _hOffset2W;   // assuming MEM_WIDTH data bus; store write stats for each offset
	
	Object(UINT32 id, UINT32 nSize, Region region)
	{
		_nID = id; _nSize = nSize; _region = region;
	}
};

struct TraceE
{
	struct Object *_obj;
	bool _entry;      // TRUE for begin; FALSE for end
	
	TraceE(Object *obj, bool entry) 
	{
		_obj = obj;
		_entry = entry;
	}
};



struct MemBlock
{
	ADDRINT _nStartAddr;
	Object *_obj;
	UINT32 _nSize;
	
	// specific for dynamic allocator
private:
	bool _bUsed;                     	
	MemBlock *_prevBlock;   // for list
	MemBlock *_nextBlock; 	
	
	MemBlock *_leftBlock;   // for constant coalescing. Physical adjacent blocks
	MemBlock *_rightBlock;  
public:
	MemBlock(Object *obj, ADDRINT nStartAddr, UINT32 nSize)
	{
		_obj = obj;
		_nStartAddr = nStartAddr;	
		_nSize = nSize;	
		_bUsed = true;
	}
	
	bool isUsed() { return _bUsed; }
	void setUsed(bool bUsed){ _bUsed = bUsed; }
	
	void setPrev(MemBlock *prev) {_prevBlock = prev;}
	void setNext(MemBlock *next) {_nextBlock = next;}
	MemBlock* getPrev() { return _prevBlock; }
	MemBlock* getNext() { return _nextBlock; }
	
	void setLeft(MemBlock *left) {_leftBlock = left;}
	void setRight(MemBlock *right) {_rightBlock = right;}
	MemBlock* getLeft() { return _leftBlock; }
	MemBlock* getRight() { return _rightBlock; }
};

class CAllocator
{
public:
	CAllocator(ADDRINT nStartAddr, UINT32 nSize, UINT32 nLineSize, string szTraceFile);	
	void run();
	void print(string szFile);
	virtual viod init(){};
	virtual void dump(){};
	
protected:
	virtual int allocate(TraceE *traceE){ return 0;};
	virtual void deallocate(TraceE *traceE){};

protected:
	// for output the stats
	map<ADDRINT,UINT64> m_32Addr2WriteCount;  // write count for each memory line
	map<ADDRINT,UINT64> m_32Addr2FrameCount;  // object count for each memory line
	
	ADDRINT m_nStartAddr;
	UINT32 m_nSizePower;
	UINT32 m_nSize;	// the size of the memory space
	UINT32 m_nLineSizeShift;	// the size of each memory line which consider the write count as a whole	
};

class CStackAllocator: public CAllocator
{
public:
	CStackAllocator(ADDRINT nStartAddr, ADDRINT nSize, ADDRINT nLineSize) : CAllocator(nStartAddr, nSize, nLineSize) {}
	void init() 
	{
		// add an initial free block
		m_StackTop = new MemBlock(0, m_nSize-1, m_nSize);
	}
private:
	virtual int allocate(TraceE *traceE);
	virtual void deallocate(TraceE *traceE);	
	virtual void dump();
private:
	MemBlock*  m_StackTop; // the free stack top
};

class CHeapAllocator: public CAllocator
{
public:
	CHeapAllocator(ADDRINT nStartAddr, ADDRINT nSize, ADDRINT nLineSize) : CAllocator(nStartAddr, nSize, nLineSize){}
	void init()
	{
		// add a boundary block for easier insert/delete operation
		MemBlock *boundary = new MemBlock(0, 0, 0);				
		MemBlock *block = new MemBlock(0, m_nStartAddr, m_nSize);
		block->setUsed(false);
		boundary->setPrev(block); boundary->setNext(block);
		block->setPrev(boundary); block->setNext(boundary);
		boundary->setLeft(block); boundary->setRight(block);
		block->setLeft(boundary); block->setRight(boundary);
				
		m_lastP = block;
	}
private:
	virtual int allocate(TraceE *traceE);
	virtual void deallocate(TraceE *traceE);
	virtual void dump();

private:
	MemBlock* m_lastP;        // the last position for next-fit
};

class CHybridAllocator
{
	
}
