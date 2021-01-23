/*
Small and fast Lua allocator, compatible with Lua 5.1 and up.
For more info and compile-time config, see luaalloc.c

Usage:
    LuaAlloc *LA = luaalloc_create(NULL, NULL);
    lua_State *L = lua_newstate(luaalloc, LA);
    ... use L ...
    lua_close(L);
    luaalloc_delete(LA);
*/

#pragma once

/* Every public API function is annotated with this */
#ifndef LUAALLOC_EXPORT
#define LUAALLOC_EXPORT
#endif

#include <stdlib.h> /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque allocator type */
typedef struct LuaAlloc LuaAlloc;

/* Main allocation callback. Lua will call this when it needs memory.
   'ud' must be a valid LuaAlloc context passed as user pointer to lua_newstate(). */
LUAALLOC_EXPORT void *luaalloc(void *ud, void *ptr, size_t osize, size_t nsize);

/* Block requests and large allocations will be forwarded to the system allocator.
   If you don't provide one, a suitable one based on realloc()/free() will be used.
   Details below. */
typedef void *(*LuaSysAlloc)(void *ud, void *ptr, size_t osize, size_t nsize);

/* Create allocator context. Pass custom system allocator if needed or NULL for the built-in default.
   Multiple Lua states can share a single LuaAlloc as long as they run on the same thread. */
LUAALLOC_EXPORT LuaAlloc *luaalloc_create(LuaSysAlloc sysalloc, void *ud);

/* Destroy allocator. Call after lua_close()ing each Lua state using the allocator. */
LUAALLOC_EXPORT void luaalloc_delete(LuaAlloc*);

/* Statistics tracking. Define LA_TRACK_STATS in luaalloc.c to use this. [Enabled by default in debug mode].
   Provides pointers to internal stats area. Each element corresponds to an internal allocation bin.
   - alive: How many allocations of a bin size are currently in use.
   - total: How many allocations of a bin size were ever made.
   - blocks: How many blocks currently exist for a bin.
   With the default config, index 0 corresponds to all allocations of 1-4 bytes, index 1 to those of 5-8 bytes, and so on.
   The bin size increment is returned in pbinstep (default: 4).
   All output pointers can be NULL if you're not interested in the thing.
   Returns the total number of bins. 0 when stats tracking is disabled.
   The last valid index is not an actual bin -- instead, large allocations that bypass the allocator are collected there.
   The returned pointers are owned by the LuaAlloc instance and stay valid throughout its lifetime.
   To iterate over the size bins, you can do:

    const size_t *alive, *total, *blocks;
    unsigned step, n = luaalloc_getstats(LA, &alive, &total, &blocks, &step);
    if(n)
    {
        for(unsigned i = 0, a = 1, b = step; i < n-1; ++i, a = b+1, b += step)
            printf("%zu blocks of %u..%u bytes: %zu allocations alive, %zu done all-time\n",
                    blocks[i],    a,  b,        alive[i],              total[i]);
        printf("large allocations: %zu alive, %zu done all-time\n", alive[n-1], total[n-1]);
    }
*/
LUAALLOC_EXPORT unsigned luaalloc_getstats(const LuaAlloc*, const size_t **alive, const size_t **total, const size_t **blocks, unsigned *pbinstep);



typedef enum
{
    LUAALLOC_TYPE_LARGELUA = 1,
    LUAALLOC_TYPE_BLOCK    = 2,
    LUAALLOC_TYPE_INTERNAL = 3
} AllocType;

#ifdef __cplusplus
}
#endif


/*
Details about the system allocator:

   typedef void *(*LuaSysAlloc)(void *ud, void *ptr, size_t osize, size_t nsize);

Block requests and large Lua allocations will be forwarded to the system allocator.
The function signature is (intentionally) the same as luaalloc() and the semantics are very similar.
The caller knows the size of each allocation so you do not have to track this yourself.
The system allocator must not fail shrink requests (same requirement as Lua).

You must handle the following cases:
    if(!ptr && nsize)
        return malloc(nsize); (osize encodes the type of allocation, see below)
    else if(ptr && !nsize)
        free(ptr); (osize is the previously allocated size; the return value is ignored)
    else if(ptr && nsize)
        return realloc(ptr, nsize); (must not fail shrink requests. osize is the previously allocated size; osize != nsize guaranteed)
    // never called with (!ptr && !nsize), can ignore this case

Types of allocations, in case (!ptr && nsize):
switch(osize)
{
    case LUAALLOC_TYPE_LARGELUA:
        passthrough/large Lua allocation (alloc'd/free'd/realloc'd incl. shrink requests)
    case LUAALLOC_TYPE_BLOCK:
        block allocation (alloc'd/free'd, but never realloc'd)
    case LUAALLOC_TYPE_INTERNAL:
        allocation of LuaAlloc-internal data (usually long-lived. alloc'd, realloc'd to enlarge, but never shrunk. free'd only in luaalloc_delete())
    case 0: default:
        some other allocation (not used by LuaAlloc. Maybe some other code uses this allocator as well?)
}

Lua allocations may fail and Lua usually handles this gracefully by running an emergency GC;
5.2 and up do this out-of-the box and there is a patch for 5.1 as well.
This block allocator is built to properly handle system allocator failures,
and return a failed allocation back to Lua as appropriate.
*/
