#ifndef MEMORY_ALLOCATOR_SMALL_BLOCK_H
#define MEMORY_ALLOCATOR_SMALL_BLOCK_H

/* Optimized memory allocator for small & frequent (de-)allocations.
 * Low memory overhead. Used for Lua.
 * Inspired by http://dns.achurch.org/cgi-bin/hg/aquaria-psp/file/tip/PSP/src/lalloc.c
*/

// Originally made for LV3proj_ng (https://github.com/fgenesis/lv3proj_ng)
// Hacked in shape for use in Aquaria
// Public domain


#include <vector>

class SmallBlockAllocator
{
public:

    SmallBlockAllocator(unsigned int blockSizeMin, unsigned int blockSizeMax, unsigned int blockSizeIncr = 8,
                        unsigned int elemsPerBlockMin = 64, unsigned int elemsPerBlockMax = 2048);

    ~SmallBlockAllocator();

    void *Alloc(void *ptr, size_t newsize, size_t oldsize);


private:

    void *_Alloc(unsigned int size);
    void *_Realloc(void *ptr, unsigned int newsize, unsigned int oldsize);
    void _Free(void* ptr, unsigned int size);

    void *_FallbackAlloc(unsigned int size);
    void _FallbackFree(void *ptr);

    struct Block
    {
        // block header start
        Block *next;
        Block *prev;
        unsigned short maxElems;
        unsigned short freeElems;
        unsigned short elemSize;
        unsigned short bitmapInts;
        // block header end

        unsigned int bitmap[1]; // variable sized
        // actual storage memory starts after bitmap[bitmapInts]

        inline unsigned char *getPtr()
        {
            return reinterpret_cast<unsigned char *>(&bitmap[bitmapInts]);
        }
        inline const unsigned char *getPtr() const
        {
            return reinterpret_cast<const unsigned char *>(&bitmap[bitmapInts]);
        }
        inline unsigned char *getEndPtr()
        {
            return getPtr() + (maxElems * elemSize);
        }
        inline const unsigned char *getEndPtr() const
        {
            return getPtr() + (maxElems * elemSize);
        }

        void *allocElem();
        void freeElem(unsigned char *ptr);
        bool contains(unsigned char *ptr) const;
    };

    Block *_AllocBlock(unsigned int elemCount, unsigned int elemSize);
    void _FreeBlock(Block *blk);
    Block *_AppendBlock(unsigned int elemSize);
    Block *_GetFreeBlock(unsigned int elemSize); // NULL if none free

    Block *_FindBlockContainingPtr(void *ptr);

    inline unsigned int GetIndexForElemSize(unsigned int elemSize)
    {


        return ((elemSize + (_blockSizeIncr - 1)) / _blockSizeIncr) - 1;
    }

    static bool Block_ptr_cmp(const Block *blk,  const void *ptr);

    Block **_blocks;
    std::vector<Block*> _allblocks; // always sorted by pointer address

    unsigned int _blockSizeMin;
    unsigned int _blockSizeMax;
    unsigned int _blockSizeIncr;

    unsigned int _elemsPerBlockMin;
    unsigned int _elemsPerBlockMax;
};

#endif
