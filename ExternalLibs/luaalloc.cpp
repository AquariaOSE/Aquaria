/* Small and fast memory allocator tailored for Lua.

License:
  Public domain, WTFPL, CC0 or your favorite permissive license; whatever is available in your country.

Dependencies:
  libc by default, change defines below to use your own functions
  Compiles as C99 or C++ code.

Thread safety:
  No global state. LuaAlloc instances are not thread-safe (same as Lua).

Background:
  Lua tends to make tiny allocations (4, 8, 16, generally less than 100 bytes) most of the time.
  malloc() & friends tend to be rather slow and also add some bytes of overhead for bookkeeping (typically 8 or 16 bytes),
  so a large percentage of the actually allocated memory is wasted.
  This allocator groups allocations of the same (small) size into blocks and passes through larger allocations.
  Small allocations have an overhead of 1 bit plus some bookkeeping information for each block.
  This allocator is also rather fast; in the typical case a block known to contain free slots is cached,
  and inside of this block, finding a free slot is a tiny loop checking 32 slots at once,
  followed by a CTZ (count trailing zeros) to locate the exact slot out of the 32.
  Freeing is similar, first do a binary search to locate the block containing the pointer to be freed,
  then flip the bit for that slot to mark it as unused. (Bitmap position and bit index is computed from the address, no loop there.)
  Once a block for a given size bin is full, other blocks in this bin are filled. A new block is allocated from the system if there is no free block.
  Unused blocks are free()d as soon as they are completely empty.

Origin:
  https://github.com/fgenesis/tinypile/blob/master/luaalloc.c

Inspired by:
  http://dns.achurch.org/cgi-bin/hg/aquaria-psp/file/tip/PSP/src/lalloc.c
  http://wiki.luajit.org/New-Garbage-Collector#arenas (--> LuaJIT has its own allocator. Don't use this one for LuaJIT.)

*/

/* ---- Configuration begin ---- */

/* Track allocation stats to get an overview of your memory usage. By default disabled in release mode. */
#ifndef NDEBUG
#  define LA_TRACK_STATS
#endif

/* Internal consistency checks. By default disabled in release mode. */
#ifdef NDEBUG
#  define LA_ASSERT(x)
#else
#  include <assert.h>
#  define LA_ASSERT(x) assert(x)
#endif

/* Required libc functions. Use your own if needed */
#include <string.h> /* for memcpy, memmove, memset */
#define LA_MEMCPY(dst, src, n) memcpy((dst), (src), (n))
#define LA_MEMMOVE(dst, src, n) memmove((dst), (src), (n))
#define LA_MEMSET(dst, val, n) memset((dst), (val), (n))

/* If you want to turn off the internal default system allocator, comment out the next line.
   If the default sysalloc is disabled, symbols for realloc()/free() won't be pulled in. */
#define LA_ENABLE_DEFAULT_ALLOC

/* Maximum size of allocations to handle. Any size beyond that will be redirected to the system allocator.
   Must be a multiple of LA_ALLOC_STEP */
#define LA_MAX_ALLOC 128

/* Provide pools in increments of this size, up to LA_MAX_ALLOC. 4 or 8 are good values. */
/* E.g. A value of 4 will create pools for size 4, 8, 12, ... 128; which is 32 distinct sizes. */
#define LA_ALLOC_STEP 4

/* Initial/Max. # of elements per block. Default growing behavior is to double the size for each full block until hitting LA_ELEMS_MAX.
   Note that each element requires 1 bit in the bitmap, the number of elements is rounded up so that no bit is unused,
   and the bitmap array is sized accordingly. Best is to use powers of 2. */
#define LA_ELEMS_MIN 64
#define LA_ELEMS_MAX 2048 /* Stored in u16, don't go higher than 0x8000 */
#define LA_GROW_BLOCK_SIZE(n) (n * 2)

typedef unsigned int u32;
typedef unsigned short u16;

/* Bitmap type. Default u32. If you want to use another unsigned type (e.g. uint64_t)
   you must provide a count-trailing-zeroes function.
   Note that the bitmap implicitly controls the data alignment -- the data area starts directly after the bitmap array,
   there is no explicit padding in between. */
typedef u32 ubitmap;

/* CTZ for your bitmap type. */
#define bitmap_CTZ(x) ctz32(x)

/* ---- Configuration end ---- */


#include "luaalloc.h"

#include <stddef.h> /* for size_t, ptrdiff_t */
#include <limits.h> /* for CHAR_BIT */

#ifdef LA_ENABLE_DEFAULT_ALLOC
#include <stdlib.h> /* for realloc, free */
#endif

/* ---- Intrinsics ---- */

#define LA_RESTRICT __restrict

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM))
# include <intrin.h>
# define HAS_BITSCANFORWARD
#elif defined(__clang__)
# if __has_builtin(__builtin_ctz)
#  define HAS_BUILTIN_CTZ
# endif
#elif defined(__GNUC__)
# define HAS_BUILTIN_CTZ
#endif

inline static unsigned ctz32(u32 x)
{
#if defined(HAS_BUILTIN_CTZ)
    return __builtin_ctz(x);
#elif defined(HAS_BITSCANFORWARD)
    unsigned long r = 0;
    _BitScanForward(&r, x);
    return r;
#else /* bit magic */
    x = (x & -x) - 1;
    /* begin popcount32 */
    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);
    x &= 0x0000003f;
    /* end popcount32 */
    return x;
#endif
}

/* ---- Structs for internal book-keeping ---- */

#define BLOCK_ARRAY_SIZE  (LA_MAX_ALLOC / LA_ALLOC_STEP)

typedef struct Block Block;

struct Block
{
    u16 elemsfree;   /* dynamic */
    u16 elemstotal;  /* const */
    u16 elemSize;    /* const */
    u16 bitmapInts;  /* const */
    Block *next;     /* dynamic */
    Block *prev;     /* dynamic */

    ubitmap bitmap[1];
    /* bitmap area */
    /* data area */
};

typedef struct LuaAlloc
{
    Block *active[BLOCK_ARRAY_SIZE]; /* current work block for each size, that serves allocations until full */
    Block *chain[BLOCK_ARRAY_SIZE]; /* newest allocated block for each size (follow ->prev to get older block) */
    Block **all; /* All blocks in use, sorted by address */
    size_t allnum; /* number of blocks in use */
    size_t allcap; /* capacity of array */
    LuaSysAlloc sysalloc;
    void *user;
#ifdef LA_TRACK_STATS
    struct
    {
        /* Extra entry is for large allocations outside of this allocator */
        size_t alive[BLOCK_ARRAY_SIZE + 1]; /* How many allocations of each size bin are currently in use */
        size_t total[BLOCK_ARRAY_SIZE + 1]; /* How many allocations of each size bin were done in total */
        size_t blocks_alive[BLOCK_ARRAY_SIZE + 1]; /* How many blocks for each size bin do currently exist */
    } stats;
#endif
} LuaAlloc;

/* ---- Helper functions ---- */

static const u16 BITMAP_ELEM_SIZE = sizeof(ubitmap) * CHAR_BIT;

inline static ubitmap *getbitmap(Block *b)
{
    return &b->bitmap[0];
}

inline static void *getdata(Block *b)
{
    return ((char*)getbitmap(b)) + (b->bitmapInts * sizeof(ubitmap));
}

inline static void *getdataend(Block *b)
{
    return ((char*)getdata(b)) + ((size_t)b->elemSize * b->elemstotal);
}

inline static unsigned sizeindex(u16 elemSize)
{
    LA_ASSERT(elemSize && elemSize <= LA_MAX_ALLOC);
    return (elemSize - 1) / LA_ALLOC_STEP;
}

inline static unsigned bsizeindex(const Block *b)
{
    return sizeindex(b->elemSize);
}

static int contains(Block * b, const void *p)
{
    return getdata(b) <= p && p < getdataend(b);
}

inline static u16 roundToFullBitmap(u16 n) 
{
#if CHAR_BIT == 8
    return (n + BITMAP_ELEM_SIZE - 1) & -BITMAP_ELEM_SIZE; /* Fast round if BITMAP_ELEM_SIZE is a power of 2 */
#else
#  error Weird hardware detected! CHAR_BIT != 8, does this mean BITMAP_ELEM_SIZE is not a power of 2? Check this, and CTZ function.
#endif
}

inline static void checkblock(Block *b)
{
    LA_ASSERT(b->elemSize && (b->elemSize % LA_ALLOC_STEP) == 0);
    LA_ASSERT(b->bitmapInts * BITMAP_ELEM_SIZE == b->elemstotal);
    LA_ASSERT(b->elemsfree <= b->elemstotal);
    LA_ASSERT(b->elemstotal >= LA_ELEMS_MIN);
    LA_ASSERT(b->elemstotal <= LA_ELEMS_MAX);
}

inline static size_t blocksize(Block *b)
{
    return (char*)getdataend(b) - (char*)b;
}

inline static u16 nextblockelems(Block *b)
{
    if(!b)
        return LA_ELEMS_MIN;
    u32 n = LA_GROW_BLOCK_SIZE(b->elemstotal);
    return (u16)(n < LA_ELEMS_MAX ? n : LA_ELEMS_MAX);
}

/* ---- System allocator interface ---- */

inline static void *sysmalloc(LuaAlloc *LA, size_t osize, size_t nsize)
{
    LA_ASSERT(nsize);
    return LA->sysalloc(LA->user, NULL, osize, nsize);
}

inline static void sysfree(LuaAlloc * LA, void * p, size_t osize)
{
    LA_ASSERT(p && osize);
    LA->sysalloc(LA->user, p, osize, 0); /* ignore return value */
}

inline static void *sysrealloc(LuaAlloc * LA_RESTRICT LA, void * LA_RESTRICT p, size_t osize, size_t nsize)
{
    LA_ASSERT(osize && nsize); /* This assert is correct even if an AllocType enum value is passed as osize. */
    return LA->sysalloc(LA->user, p, osize, nsize);
}

/* ---- Allocator internals ---- */

static Block *_allocblock(LuaAlloc *LA, u16 nelems, u16 elemsz)
{
    elemsz = ((elemsz + LA_ALLOC_STEP-1) / LA_ALLOC_STEP) * LA_ALLOC_STEP; /* round up */
    nelems = roundToFullBitmap(nelems); /* The bitmap array must not have any unused bits */
    const u16 nbitmap = nelems / BITMAP_ELEM_SIZE;

    void *ptr = sysmalloc(LA, LUAALLOC_TYPE_BLOCK,
          (sizeof(Block) - sizeof(ubitmap)) /* block header without bitmap[1] */
        + (nbitmap * sizeof(ubitmap))       /* actual bitmap size */
        + (nelems * (size_t)elemsz)         /* data size */
    );

    if(!ptr)
        return NULL;

    Block *b = (Block*)ptr;
    b->elemsfree = nelems;
    b->elemstotal = nelems;
    b->elemSize = elemsz;
    b->bitmapInts = nbitmap;
    b->next = NULL;
    b->prev = NULL;
    LA_MEMSET(b->bitmap, -1, nbitmap * sizeof(ubitmap)); /* mark all as free */

    return b;
}

/* Given the sorting order of LA->all, find the right spot to insert p that preserves the sorting order.
   Returns the address of the block that is <= p, or one past the end if no such block was found.
   Use cases:
   1) Pass a block to get the address where this block is stored
   2) Pass any other pointer to get ONE BLOCK PAST the address of the block that would contain it (this is not checked)
*/
static Block **findspot(LuaAlloc * LA_RESTRICT LA, void * LA_RESTRICT p)
{
    Block **all = LA->all;

    /* Binary search to find leftmost element */
    size_t L = 0;
    size_t R = LA->allnum;
    while(L < R)
    {
        size_t m = (L + R) / 2u;
        if((void*)all[m] < p)
            L = m + 1;
        else
            R = m;
    }
    return all + L;
}

static size_t enlarge(LuaAlloc *LA)
{
    const size_t incr = (LA->allcap / 2) + 16;
    const size_t newcap = LA->allcap + incr; /* Rough guess */
    Block **newall = (Block**)sysrealloc(LA, LA->all, LA->all ? LA->allcap : LUAALLOC_TYPE_INTERNAL, sizeof(Block*) * newcap);
    if(newall)
    {
        LA->all = newall;
        LA->allcap = newcap;
        return newcap;
    }
    return 0;
}

static Block *insertblock(LuaAlloc * LA_RESTRICT LA, Block * LA_RESTRICT b)
{
    /* Enlarge central block storage if necessary */
    if(LA->allcap == LA->allnum && !enlarge(LA))
    { 
        sysfree(LA, b, blocksize(b)); /* Can't fit block, kill it and fail */
        return NULL;
    }

    /* Find correct spot to insert */
    /* Invariant: Array is already sorted */
    Block **spot = findspot(LA, b);
    Block **end = LA->all + LA->allnum;

    /* inserting in the middle? Must preserve sort order */
    if(spot < end)
    {
        /* move other pointers up */
        LA_MEMMOVE(spot+1, spot, (end - spot) * sizeof(Block*));
    }

    *spot = b;
    ++LA->allnum;

    /* Link in chain */
    const unsigned si = bsizeindex(b);
    Block *top = LA->chain[si];
    LA->chain[si] = b;
    if(top)
    {
        LA_ASSERT(!top->next);
        top->next = b;
    }
    b->prev = top;

#ifdef LA_TRACK_STATS
    LA->stats.blocks_alive[si]++;
#endif

    checkblock(b);

    return b;
}

static void freeblock(LuaAlloc * LA_RESTRICT LA, Block ** LA_RESTRICT spot)
{
    LA_ASSERT(LA->allnum);
    Block *b = *spot;
    checkblock(b);

    /* Remove from central list */
    Block **end = LA->all + LA->allnum;
    if(spot+1 < end)
    {
        /* Move other pointers down */
        LA_MEMMOVE(spot, spot+1, (end - (spot+1)) * sizeof(Block*));
    }
    --LA->allnum;
    /* Invariant: Array is still sorted after removing an element */

    /* Remove from chain */
    unsigned si = bsizeindex(b);
    if(LA->chain[si] == b)
    {
        LA_ASSERT(!b->next);
        LA->chain[si] = b->prev;
    }

    if(LA->active[si] == b)
        LA->active[si] = NULL;

    /* Unlink from linked list */
    if(b->next)
    {
        LA_ASSERT(b->next->prev == b);
        b->next->prev = b->prev;
    }
    if(b->prev)
    {
        LA_ASSERT(b->prev->next == b);
        b->prev->next = b->next;
    }

#ifdef LA_TRACK_STATS
    LA->stats.blocks_alive[si]--;
#endif

    sysfree(LA, b, blocksize(b)); /* free it */
}

static Block *newblock(LuaAlloc *LA, u16 nelems, u16 elemsz)
{
    Block *b = _allocblock(LA, nelems, elemsz);
    return b ? insertblock(LA, b) : NULL;
}

static void *_Balloc(Block *b)
{
    LA_ASSERT(b->elemsfree);
    ubitmap *bitmap = b->bitmap;
    unsigned i = 0, bm;
    for( ; !((bm = bitmap[i])); ++i) {} /* as soon as one isn't all zero, there's a free slot */
    LA_ASSERT(i < b->bitmapInts); /* And there must've been a free slot because b->elemsfree != 0 */
    ubitmap bitIdx = bitmap_CTZ(bm); /* Get exact location of free slot */
    LA_ASSERT(bm & ((ubitmap)1 << bitIdx)); /* make sure this is '1' (= free) */
    bm &= ~((ubitmap)1 << bitIdx); /* put '0' where '1' was (-> mark as non-free) */
    bitmap[i] = bm;
    --b->elemsfree;
    const size_t where = (i * (size_t)BITMAP_ELEM_SIZE) + bitIdx;
    void *ret = ((char*)getdata(b)) + (where * b->elemSize);
    LA_ASSERT(contains(b, ret));
    return ret;
}

static void _Bfree(Block * LA_RESTRICT b, void * LA_RESTRICT p)
{
    LA_ASSERT(b->elemsfree < b->elemstotal);
    LA_ASSERT(contains(b, p));
    const ptrdiff_t offs = (char*)p - (char*)getdata(b);
    LA_ASSERT(offs % b->elemSize == 0);
    const unsigned idx = (unsigned)(offs / b->elemSize);
    const unsigned bitmapIdx = idx / BITMAP_ELEM_SIZE;
    const ubitmap bitIdx = idx % BITMAP_ELEM_SIZE;
    LA_ASSERT(bitmapIdx < b->bitmapInts);
    LA_ASSERT(!(b->bitmap[bitmapIdx] & ((ubitmap)1 << bitIdx))); /* make sure this is '0' (= used) */
    b->bitmap[bitmapIdx] |= ((ubitmap)1 << bitIdx); /* put '1' where '0' was (-> mark as free) */
    ++b->elemsfree;
}

/* returns block with at least 1 free slot, NULL only in case of allocation fail */
static Block *getfreeblock(LuaAlloc *LA, u16 size)
{
    unsigned si = sizeindex(size);
    Block *b = LA->active[si];
    if(b && b->elemsfree) /* Good case: Currently active block is free, use that */
        return b;

    /* Not-so-good case: Active block is full or doesn't exist, try an older block in the chain */
    b = LA->chain[si];
    while(b && !b->elemsfree)
        b = b->prev;

    /* Still no good? Allocate new block */
    if(!b || !b->elemsfree)
        b = newblock(LA, nextblockelems(LA->chain[si]), size); /* Use newest block in chain to compute size */

    /* Use this block for further allocation requests */
    LA->active[si] = b;

    return b;
}

static void *_Alloc(LuaAlloc *LA, size_t size)
{
    LA_ASSERT(size);

    if(size <= LA_MAX_ALLOC)
    {
        Block *b = getfreeblock(LA, (u16)size);
        if(b)
        {
            checkblock(b);
            void *p = _Balloc(b);
            LA_ASSERT(p); /* Can't fail -- block was known to be free */

#ifdef LA_TRACK_STATS
            unsigned si = bsizeindex(b);
            LA->stats.alive[si]++;
            LA->stats.total[si]++;
#endif
            return p;
        }
        /* else try the alloc below */
    }

    void *p = sysmalloc(LA, LUAALLOC_TYPE_LARGELUA, size); /* large Lua allocation */

#ifdef LA_TRACK_STATS
    if(p)
    {
        LA->stats.alive[BLOCK_ARRAY_SIZE]++;
        LA->stats.total[BLOCK_ARRAY_SIZE]++;
    }
#endif
    return p;
}

static void freefromspot(LuaAlloc * LA_RESTRICT LA, Block ** LA_RESTRICT spot, void *p)
{
    Block *b = *spot;
#ifdef LA_TRACK_STATS
    unsigned si = bsizeindex(b);
    LA->stats.alive[si]--;
#endif
    if(b->elemsfree + 1 == b->elemstotal)
        freeblock(LA, spot); /* Freeing last element in the block -> just free the whole thing */
    else
        _Bfree(b, p);
}

static void _Free(LuaAlloc * LA_RESTRICT LA , void * LA_RESTRICT p, size_t oldsize)
{
    LA_ASSERT(p);

    if(oldsize <= LA_MAX_ALLOC)
    {
        Block **spot = findspot(LA, p); /* Here, spot might point one past the end */
        spot -= (spot > LA->all); /* One back unless we're already at the front -- now spot is always valid */
        Block *b = *spot;
        checkblock(b);
        if(contains(b, p))
        {
            freefromspot(LA, spot, p);
            return;
        }
        /* else p is outside of any block area. This case is unlikely but possible:
           - alloc large size (falling through to system alloc),
           - then, try to shrink it to fit inside LA_MAX_ALLOC,
           - ... but there is no block free for that size...
           - try to alloc new block and fail (out of memory)
           - then _Realloc() uses the original, still valid pointer since by spec shrink requests must not fail
           - Lua sees the "reallocated" (actually the old) pointer and records the new, smaller size;
           - when this pointer is freed, we're here in this situation.
           Therefore fall through to free a large allocation. */
    }

#ifdef LA_TRACK_STATS
    LA->stats.alive[BLOCK_ARRAY_SIZE]--;
#endif

    sysfree(LA, p, oldsize); /* large Lua free */
}

static void *_Realloc(LuaAlloc * LA_RESTRICT LA, void * LA_RESTRICT p, size_t newsize, size_t oldsize)
{
    LA_ASSERT(p);
    void *newptr = _Alloc(LA, newsize);

    /* If the new allocation failed, just re-use the old pointer if it was a shrink request.
       This also satisfies Lua, which assumes that shrink requests cannot fail */
    if(!newptr)
        return newsize <= oldsize ? p : NULL;

    const size_t minsize = oldsize < newsize ? oldsize : newsize;
    LA_MEMCPY(newptr, p, minsize);
    _Free(LA, p, oldsize);
    return newptr;
}

/* ---- Default system allocator ---- */

#ifdef LA_ENABLE_DEFAULT_ALLOC
static void *defaultalloc(void *user, void *ptr, size_t osize, size_t nsize)
{
    (void)user;
    (void)osize;
    if(nsize)
        return realloc(ptr, nsize);
    free(ptr);
    return NULL;
}
#endif

/* ---- Public API ---- */

#ifdef __cplusplus
extern "C" {
#endif

LUAALLOC_EXPORT void *luaalloc(void * ud, void *ptr, size_t oldsize, size_t newsize)
{
    LuaAlloc *LA = (LuaAlloc*)ud;
    if(ptr)
    {
        if(!newsize)
            _Free(LA, ptr, oldsize);
        else if(newsize != oldsize)
            return _Realloc(LA, ptr, newsize, oldsize);
        else
            return ptr;
    }
    else if(newsize)
        return _Alloc(LA, newsize);

    return NULL;
}

LUAALLOC_EXPORT LuaAlloc * luaalloc_create(LuaSysAlloc sysalloc, void *user)
{
    if(!sysalloc)
    {
#ifdef LA_ENABLE_DEFAULT_ALLOC
        sysalloc = defaultalloc;
#else
        LA_ASSERT(sysalloc);
        return NULL;
#endif
    }

    LuaAlloc *LA = (LuaAlloc*)sysalloc(user, NULL, LUAALLOC_TYPE_INTERNAL, sizeof(LuaAlloc));
    if(LA)
    {
        LA_MEMSET(LA, 0, sizeof(LuaAlloc));
        LA->sysalloc = sysalloc;
        LA->user = user;
    }
    return LA;
}

LUAALLOC_EXPORT void luaalloc_delete(LuaAlloc *LA)
{
    LA_ASSERT(LA->allnum == 0); /* If this fails the Lua state didn't GC everything, which is a bug */
    if(LA->all)
        sysfree(LA, LA->all, LA->allcap * sizeof(Block*));
    sysfree(LA, LA, sizeof(LuaAlloc)); /* free self */
}

/* ---- Optional stats tracking ---- */

LUAALLOC_EXPORT unsigned luaalloc_getstats(const LuaAlloc *LA, const size_t ** alive, const size_t ** total, const size_t ** blocks, unsigned *pbinstep)
{
    if(pbinstep)
        *pbinstep = LA_ALLOC_STEP;

#ifdef LA_TRACK_STATS
    if(alive)
        *alive = LA->stats.alive;
    if(total)
        *total = LA->stats.total;
    if(blocks)
        *blocks = LA->stats.blocks_alive;
    return BLOCK_ARRAY_SIZE + 1;
#else
    if(alive)
        *alive = NULL;
    if(total)
        *total = NULL;
    if(blocks)
        *blocks = NULL;
    return 0;
#endif
}

#ifdef __cplusplus
}
#endif
