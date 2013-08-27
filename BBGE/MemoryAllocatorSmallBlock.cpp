// Public domain

// Aquaria specific...
#include "Base.h"

#include "algorithmx.h"
#include "MemoryAllocatorSmallBlock.h"
#include "bithacks.h"

#include <assert.h>

//#define DD(...) fprintf(stderr, __VA_ARGS__)
#define DD(...)
#define logdev(...)
#define logerror(...)

#ifdef NDEBUG
# define ASSERT(x)
#else
# define ASSERT(x) assert(x)
#endif


SmallBlockAllocator::SmallBlockAllocator(unsigned int blockSizeMin,
                                         unsigned int blockSizeMax,
                                         unsigned int blockSizeIncr /* = 8 */,
                                         unsigned int elemsPerBlockMin /* = 64 */,
                                         unsigned int elemsPerBlockMax /* = 2048 */)
 : _blockSizeMin(blockSizeMin)
 , _blockSizeMax(blockSizeMax)
 , _blockSizeIncr(blockSizeIncr)
 , _elemsPerBlockMin(elemsPerBlockMin)
 , _elemsPerBlockMax(elemsPerBlockMax)
{
    ASSERT(_blockSizeIncr % 4 == 0); // less than 4 bytes makes no sense
    ASSERT(_blockSizeMin % _blockSizeIncr == 0);
    ASSERT(_blockSizeMax % _blockSizeIncr == 0);
    ASSERT((_blockSizeMax - _blockSizeMin) % _blockSizeIncr == 0);
    unsigned int c = ((_blockSizeMax - _blockSizeMin) / _blockSizeIncr) + 1;
    logdev("SBA: Using %u distinct block sizes from %u - %u bytes", c, _blockSizeMin, _blockSizeMax);
    _blocks = new Block*[c]; // TODO: Do we really want to use dynamic allocation here?
    memset(_blocks, 0, c * sizeof(Block*));
}

SmallBlockAllocator::~SmallBlockAllocator()
{
    while(_allblocks.size())
    {
        Block *blk = _allblocks.back();
        logerror("~SmallBlockAllocator(): Warning: Leftover block with %u/%u elements, %uB each",
            blk->maxElems, blk->maxElems - blk->freeElems, blk->elemSize);
        _FreeBlock(blk);
    }
    delete [] _blocks;
}

void *SmallBlockAllocator::Alloc(void *ptr, size_t newsize, size_t oldsize)
{
    DD("SBA::Alloc() ptr = %p; newsize = %u, oldsize = %u", ptr, newsize, oldsize);

    if(ptr)
    {
        if(!newsize)
        {
            _Free(ptr, oldsize);
            return NULL;
        }
        else if(newsize == oldsize)
            return ptr;
        else
            return _Realloc(ptr, newsize, oldsize);
    }
    else
    {
        if(newsize)
            return _Alloc(newsize);
    }
    return NULL;
}

SmallBlockAllocator::Block *SmallBlockAllocator::_AllocBlock(unsigned int elemCount, unsigned int elemSize)
{
    DD("SBA: _AllocBlock: elemCount = %u, elemSize = %u", elemCount, elemSize);

    const unsigned int bitsPerInt = (sizeof(unsigned int) * 8); // 32
    unsigned int bitmapInts = (elemCount + (bitsPerInt - 1)) / bitsPerInt;
    void *ptr = malloc(
              (sizeof(Block) - sizeof(unsigned int)) // block header without bitmap[1]
            + (bitmapInts * sizeof(unsigned int))    // actual bitmap size
            + (elemCount * elemSize)                 // data size
        );

    if(!ptr)
        return NULL;
    Block *blk = (Block*)ptr;
    memset(&blk->bitmap[0], 0xff, bitmapInts * sizeof(unsigned int)); // all free
    blk->elemSize = elemSize;
    blk->maxElems = elemCount;
    blk->freeElems = elemCount;
    blk->bitmapInts = bitmapInts;
    blk->next = NULL;
    blk->prev = NULL;

    // using insertion sort
    std::vector<Block*>::iterator insertit = std::lower_bound(_allblocks.begin(), _allblocks.end(), blk);
    _allblocks.insert(insertit, blk);

    return blk;
}

void SmallBlockAllocator::_FreeBlock(Block *blk)
{
    DD("SBA: _FreeBlock: elemCount = %u, elemSize = %u", blk->maxElems, blk->elemSize);

    if(blk->prev)
        blk->prev->next = blk->next;
    else
        _blocks[GetIndexForElemSize(blk->elemSize)] = blk->next;

    if(blk->next)
        blk->next->prev = blk->prev;

    free(blk);

    // keeps the vector sorted
    _allblocks.erase(std::remove(_allblocks.begin(), _allblocks.end(), blk), _allblocks.end());
}


SmallBlockAllocator::Block *SmallBlockAllocator::_AppendBlock(unsigned int elemSize)
{
    unsigned int idx = GetIndexForElemSize(elemSize);
    Block *blk = _blocks[idx];
    unsigned int elemsPerBlock = _elemsPerBlockMin;
    if(blk)
    {
        while(blk->next)
            blk = blk->next;
        elemsPerBlock = blk->maxElems * 2; // new block is double the size
        if(elemsPerBlock > _elemsPerBlockMax)
            elemsPerBlock = _elemsPerBlockMax;
    }

    unsigned int blockElemSize = ((elemSize + (_blockSizeIncr - 1)) / _blockSizeIncr) * _blockSizeIncr;
    ASSERT(blockElemSize >= elemSize);

    Block *newblk = _AllocBlock(elemsPerBlock, blockElemSize);
    if(!newblk)
        return NULL;

    if(blk)
    {
        blk->next = newblk; // append to list
        newblk->prev = blk;
    }
    else
        _blocks[idx] = newblk; // list head

    return newblk;
}

SmallBlockAllocator::Block *SmallBlockAllocator::_GetFreeBlock(unsigned int elemSize)
{
    unsigned int idx = GetIndexForElemSize(elemSize);
    Block *blk = _blocks[idx];
    while(blk && !blk->freeElems)
        blk = blk->next;
    return blk;
}

void *SmallBlockAllocator::Block::allocElem()
{
    ASSERT(freeElems);
    unsigned int i = 0;
    for( ; !bitmap[i]; ++i) // as soon as one isn't all zero, there's a free slot
        ASSERT(i < bitmapInts);
    ASSERT(i < bitmapInts);
    int freeidx = bithacks::ctz(bitmap[i]);
    ASSERT(bitmap[i] & (1 << freeidx)); // make sure this is '1' (= free)
    bitmap[i] &= ~(1 << freeidx); // put '0' where '1' was (-> mark as non-free)
    --freeElems;
    const unsigned int offs = (i * sizeof(unsigned int) * 8 * elemSize); // skip forward i bitmaps (32 elems each)
    unsigned char *ret = getPtr() + offs + (elemSize * freeidx);
    ASSERT(contains(ret));
    return ret;
}

bool SmallBlockAllocator::Block::contains(unsigned char *ptr) const
{
    const unsigned char *pp = getPtr();

    if(ptr < pp)
        return false; // pointer is out of range (1)
    if(ptr >= pp + (maxElems * elemSize))
        return false;  // pointer is out of range (2)

    return true;
}

void SmallBlockAllocator::Block::freeElem(unsigned char *ptr)
{
    ASSERT(contains(ptr));
    ASSERT(freeElems < maxElems); // make sure the block is not all free

    const ptrdiff_t p = ptr - getPtr();
    ASSERT((p % elemSize) == 0); // make sure alignment is right
    const unsigned int idx = p / elemSize;
    const unsigned int bitsPerInt = sizeof(unsigned int) * 8; // 32
    const unsigned int bitmapIdx = idx / bitsPerInt;
    const unsigned int bitIdx = idx % bitsPerInt;
    ASSERT(bitmapIdx < bitmapInts);
    ASSERT(!(bitmap[bitmapIdx] & (1 << bitIdx))); // make sure this is '0' (= used)

    bitmap[bitmapIdx] |= (1 << bitIdx); // put '1' where '0' was (-> mark as free)
    ++freeElems;

#ifdef _DEBUG
    memset(ptr, 0xfa, elemSize);
#endif
}

void *SmallBlockAllocator::_FallbackAlloc(unsigned int size)
{
    return malloc(size);
}

void SmallBlockAllocator::_FallbackFree(void *ptr)
{
    free(ptr);
}

void *SmallBlockAllocator::_Alloc(unsigned int size)
{
    if(size > _blockSizeMax)
        return _FallbackAlloc(size);

    Block *blk = _GetFreeBlock(size);
    ASSERT(!blk || blk->freeElems);
    if(!blk)
    {
        blk = _AppendBlock(size);
        if(!blk)
            return _FallbackAlloc(size);
    }
    return blk->allocElem();
}

bool SmallBlockAllocator::Block_ptr_cmp(const Block *blk, const void *ptr)
{
    return blk->getEndPtr() < ((unsigned char*)ptr);
}

SmallBlockAllocator::Block *SmallBlockAllocator::_FindBlockContainingPtr(void *ptr)
{
    // MSVC's std::lower_bound uses iterator debug checks in debug mode,
    // which breaks Block_ptr_cmp() because the left and right types are different.
    std::vector<Block*>::iterator it = stdx_fg::lower_bound(_allblocks.begin(), _allblocks.end(), ptr, Block_ptr_cmp);
    return (it != _allblocks.end() && (*it)->contains((unsigned char*)ptr)) ? *it : NULL;
}

void SmallBlockAllocator::_Free(void *ptr, unsigned int size)
{
    if(size <= _blockSizeMax)
    {
        Block *blk = _FindBlockContainingPtr(ptr);
        if(blk)
        {
            ASSERT(blk->elemSize >= size); // ptr might be from a larger block in case _Realloc() failed to shrink
            blk->freeElem((unsigned char*)ptr);
            if(blk->freeElems == blk->maxElems)
                _FreeBlock(blk); // remove if completely unused
            return;
        }
    }
    _FallbackFree(ptr);
}

void *SmallBlockAllocator::_Realloc(void *ptr, unsigned int newsize, unsigned int oldsize)
{
    void *newptr = _Alloc(newsize);

    // If the new allocation failed, just re-use the old pointer if it was a shrink request
    // This also satisfies Lua, which assumes that realloc() shrink requests cannot fail
    if(!newptr)
        return newsize <= oldsize ? ptr : NULL;

    memcpy(newptr, ptr, std::min(oldsize, newsize));
    _Free(ptr, oldsize);
    return newptr;
}

