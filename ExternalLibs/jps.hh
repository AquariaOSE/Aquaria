#pragma once

/*
Public domain Jump Point Search implementation -- very fast pathfinding for uniform cost grids.
Scroll down for compile config, usage tips, example code.

License:
  Public domain, WTFPL, CC0 or your favorite permissive license; whatever is available in your country.

Dependencies:
  libc (stdlib.h, math.h) by default, change defines below to use your own functions. (realloc(), free(), sqrt())
  Compiles as C++98, does not require C++11 nor the STL.
  Does not throw exceptions, works without RTTI, does not contain any virtual methods.

Thread safety:
  No global state. Searcher instances are not thread-safe. Grid template class is up to you.
  If your grid access is read-only while pathfinding you may have many threads compute paths at the same time,
  each with its own Searcher instance.

Background:
  If you want to generate paths on a map with the following properties:
  - You have a 2D grid (exactly two dimensions!), where each tile has exactly 8 neighbors (up, down, left, right + diagonals)
  - There is no "cost" -- a tile is either walkable, or not.
  then you may want to avoid full fledged A* and go for Jump Point Search (this lib).
  JPS is usually much faster than plain old A*, as long as your tile traversability check function is fast.

Origin:
  https://github.com/fgenesis/tinypile/blob/master/jps.hh

Based on my older implementation:
  https://github.com/fgenesis/jps/blob/master/JPS.h
  (For changes compared to that version go to the end of this file)

Inspired by:
  http://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-aaai11.pdf (The original paper)
  https://github.com/Yonaba/Jumper
  https://github.com/qiao/PathFinding.js

Usage:

  Define a class that overloads `operator()(x, y) const`, returning a value that can be treated as boolean.
  You are responsible for bounds checking!
  You want your operator() to be as fast and small as possible, as it will be called a LOT.
  Ask your compiler to force-inline it if possible.

// --- Begin example code ---

struct MyGrid
{
    inline bool operator()(unsigned x, unsigned y) const // coordinates must be unsigned; method must be const
    {
        if(x < width && y < height) // Unsigned will wrap if < 0
            ... return true if terrain at (x, y) is walkable.
        // return false if terrain is not walkable or out-of-bounds.
    }
    unsigned width, height;
};

// Then you can retrieve a path:

MyGrid grid(... set grid width, height, map data, whatever);

const unsigned step = 0; // 0 compresses the path as much as possible and only records waypoints.
                         // Set this to 1 if you want a detailed single-step path
                         // (e.g. if you plan to further mangle the path yourself),
                         // or any other higher value to output every Nth position.
                         // (Waypoints are always output regardless of the step size.)

JPS::PathVector path; // The resulting path will go here.
                      // You may also use std::vector or whatever, as long as your vector type
                      // has push_back(), begin(), end(), resize() methods (same semantics as std::vector).
                      // Note that the path will NOT include the starting position!
                      // --> If called with start == end it will report that a path has been found,
                      //     but the resulting path vector will be empty!

// Single-call interface:
// (Further remarks about this function can be found near the bottom of this file.)
// Note that the path vector is NOT cleared! New path points are appended at the end.
bool found = JPS::findPath(path, grid, startx, starty, endx, endy, step);


// --- Alternatively, if you want more control & efficiency for repeated pathfinding runs: ---

// Use a Searcher instance (can be a class member, on the stack, ...)
// Make sure the passed grid reference stays valid throughout the searcher's lifetime.
// If you need control over memory allocation, you may pass an extra pointer that will be
// forwarded to your own JPS_realloc & JPS_free if you've set those. Otherwise it's ignored.
JPS::Searcher<MyGrid> search(grid, userPtr = NULL);

// build path incrementally from waypoints:
JPS::Position a, b, c, d = <...>; // set some waypoints
if (search.findPath(path, a, b)
 && search.findPath(path, b, c)
 && search.findPath(path, c, d))
{
    // found path: a->b->c->d
}

// keep re-using existing pathfinder instance
while(whatever)
{
    // Set startx, starty, endx, endy = <...>
    if(!search.findPath(path, JPS::Pos(startx, starty), JPS::Pos(endx, endy), step))
    {
        // ...handle failure...
    }
}

// If necessary, you may free internal memory -- this is never required; neither for performance, nor for correct function.
// If you do pathfinding after freeing memory, it'll allocate new memory.
// Note that freeing memory aborts any incremental search currently ongoing.
search.freeMemory();

// If you need to know how much memory is internally allocated by a searcher:
unsigned bytes = search.getTotalMemoryInUse();


// -------------------------------
// --- Incremental pathfinding ---
// -------------------------------

Calling JPS::findPath() or Searcher<>::findPath() always computes an entire path or returns failure.
If the path is long or costly and you have a tight CPU budget per frame you may want to perform pathfinding incrementally,
stretched over multiple frames.

First, call
  ### JPS_Result res = search.findPathInit(Position start, Position end) ###
Don't forget to check the return value, as it may return:
- JPS_NO_PATH if one or both of the points are obstructed
- JPS_EMPTY_PATH if the points are equal and not obstructed
- JPS_FOUND_PATH if the initial greedy heuristic could find a path quickly.
- JPS_OUT_OF_MEMORY if... well yeah.
If it returns JPS_NEED_MORE_STEPS then the next part can start.

Repeatedly call
  ### JPS_Result res = search.findPathStep(int limit) ###
until it returns JPS_NO_PATH or JPS_FOUND_PATH, or JPS_OUT_OF_MEMORY.
For consistency, you will want to ensure that the grid does not change between subsequent calls;
if the grid changes, parts of the path may go through a now obstructed area or may be no longer optimal.
If limit is 0, it will perform the pathfinding in one go. Values > 0 pause the search
as soon as possible after the number of steps was exceeded, returning NEED_MORE_STEPS.
Use search.getStepsDone() after some test runs to find a good value for the limit.

After getting JPS_FOUND_PATH, generate the actual path points via
  ### JPS_Result res = search.findPathFinish(PathVector& path, unsigned step = 0) ###
As described above, path points are appended, and granularity can be adjusted with the step parameter.
Returns JPS_FOUND_PATH if the path was successfully built and appended to the path vector.
Returns JPS_NO_PATH if the pathfinding did not finish or generating the path failed.
May return JPS_OUT_OF_MEMORY if the path vector must be resized but fails to allocate.

If findPathInit() or findPathStep() return JPS_OUT_OF_MEMORY, the current searcher progress becomes undefined.
To recover, free some memory elsewhere and call findPathInit() to try again.

If findPathFinish() returns out-of-memory but previous steps finished successfully,
then the found path is still valid for generating the path vector.
In that case you may call findPathFinish() again after making some memory available.

If you do not worry about memory, treat JPS_OUT_OF_MEMORY as if JPS_NO_PATH was returned.

You may pass JPS::PathVector, std::vector, or your own to findPathFinish().
Note that if the path vector type you pass throws exceptions in case of allocation failures (std::vector does, for example),
you'll get that exception, and the path vector will be in whatever state it was in when the last element was successfully inserted.
If no exception is thrown (ie. you used JPS::PathVector) then the failure cases do not modify the path vector.

You may abort a search anytime by starting a new one via findPathInit(), calling freeMemory(), or by destroying the searcher instance.
Aborting or starting a search resets the values returned by .getStepsDone() and .getNodesExpanded() to 0.

*/

// ============================
// ====== COMPILE CONFIG ======
// ============================


// If you want to avoid sqrt() or floats in general, define this.
// Turns out in some testing this was ~12% faster, so it's the default.
#define JPS_NO_FLOAT


// ------------------------------------------------

#include <stddef.h> // for size_t (needed for operator new)

// Assertions
#ifndef JPS_ASSERT
# ifdef _DEBUG
#  include <assert.h>
#  define JPS_ASSERT(cond) assert(cond)
# else
#  define JPS_ASSERT(cond)
# endif
#endif

// The default allocator uses realloc(), free(). Change if necessary.
// You will get the user pointer that you passed to findPath() or the Searcher ctor.
#if !defined(JPS_realloc) || !defined(JPS_free)
# include <stdlib.h> // for realloc, free
# ifndef JPS_realloc
#  define JPS_realloc(p, newsize, oldsize, user) realloc(p, newsize)
# endif
# ifndef JPS_free
#  define JPS_free(p, oldsize, user) free(p)
# endif
#endif

#ifdef JPS_NO_FLOAT
#define JPS_HEURISTIC_ACCURATE(a, b) (Heuristic::Chebyshev(a, b))
#else
# ifndef JPS_sqrt
// for Euclidean heuristic.
#  include <math.h>
#  define JPS_sqrt(x) sqrtf(float(x)) // float cast here avoids a warning about implicit int->float cast
# endif
#endif

// Which heuristics to use.
// Basic property: Distance estimate, returns values >= 0. Smaller is better.
// The accurate heuristic should always return guesses less or equal than the estimate heuristic,
// otherwise the resulting paths may not be optimal.
// (The rule of thumb is that the estimate is fast but can overestimate)
// For the implementation of heuristics, scroll down.
#ifndef JPS_HEURISTIC_ACCURATE
#define JPS_HEURISTIC_ACCURATE(a, b) (Heuristic::Euclidean(a, b))
#endif

#ifndef JPS_HEURISTIC_ESTIMATE
#define JPS_HEURISTIC_ESTIMATE(a, b) (Heuristic::Manhattan(a, b))
#endif


// --- Data types ---
namespace JPS {

// unsigned integer type wide enough to store a position on one grid axis.
// Note that on x86, u32 is actually faster than u16.
typedef unsigned PosType;

// Result of heuristics. can also be (unsigned) int but using float by default since that's what sqrtf() returns
// and we don't need to cast float->int that way. Change if you use integer-only heuristics.
// (Euclidean heuristic using sqrt() works fine even if cast to int. Your choice really.)
#ifdef JPS_NO_FLOAT
typedef int ScoreType;
#else
typedef float ScoreType;
#endif

// Size type; used internally for vectors and the like. You can set this to size_t if you want, but 32 bits is more than enough.
typedef unsigned SizeT;

} // end namespace JPS


// ================================
// ====== COMPILE CONFIG END ======
// ================================
// ----------------------------------------------------------------------------------------

typedef unsigned JPS_Flags;
enum JPS_Flags_
{
    // No special behavior
    JPS_Flag_Default       = 0x00,

    // If this is defined, disable the greedy direct-short-path check that avoids the large area scanning that JPS does.
    // This is just a performance tweak. May save a lot of CPU when constantly re-planning short paths without obstacles
    // (e.g. an entity follows close behind another).
    // Does not change optimality of results. If you perform your own line-of-sight checks
    // before starting a pathfinding run you can disable greedy since checking twice isn't needed,
    // but otherwise it's better to leave it enabled.
    JPS_Flag_NoGreedy      = 0x01,

    // If this is set, use standard A* instead of JPS (e.g. if you want to compare performance in your scenario).
    // In most cases this will be MUCH slower, but might be beneficial if your grid lookup
    // is slow (aka worse than O(1) or more than a few inlined instructions),
    // as it avoids the large area scans that the JPS algorithm does.
    // (Also increases memory usage as each checked position is expanded into a node.)
    JPS_Flag_AStarOnly     = 0x02,

    // Don't check whether start position is walkable.
    // This makes the start position always walkable, even if the map data say otherwise.
    JPS_Flag_NoStartCheck  = 0x04,

    // Don't check whether end position is walkable.
    JPS_Flag_NoEndCheck    = 0x08,
};

enum JPS_Result
{
    JPS_NO_PATH,
    JPS_FOUND_PATH,
    JPS_NEED_MORE_STEPS,
    JPS_EMPTY_PATH,
    JPS_OUT_OF_MEMORY
};

// operator new() without #include <new>
// Unfortunately the standard mandates the use of size_t, so we need stddef.h the very least.
// Trick via https://github.com/ocornut/imgui
// "Defining a custom placement new() with a dummy parameter allows us to bypass including <new>
// which on some platforms complains when user has disabled exceptions."
struct JPS__NewDummy {};
inline void* operator new(size_t, JPS__NewDummy, void* ptr) { return ptr; }
inline void  operator delete(void*, JPS__NewDummy, void*)       {}
#define JPS_PLACEMENT_NEW(p) new(JPS__NewDummy(), p)


namespace JPS {

struct Position
{
    PosType x, y;

    inline bool operator==(const Position& p) const
    {
        return x == p.x && y == p.y;
    }
    inline bool operator!=(const Position& p) const
    {
        return x != p.x || y != p.y;
    }

    inline bool isValid() const { return x != PosType(-1); }
};

// The invalid position. Used internally to mark non-walkable points.
static const Position npos = {PosType(-1), PosType(-1)};
static const SizeT noidx = SizeT(-1);

// ctor function to keep Position a real POD struct.
inline static Position Pos(PosType x, PosType y)
{
    Position p;
    p.x = x;
    p.y = y;
    return p;
}

template<typename T> inline static T Max(T a, T b) { return a < b ? b : a; }
template<typename T> inline static T Min(T a, T b) { return a < b ? a : b; }
template<typename T> inline static T Abs(T a)      { return a < T(0) ? -a : a; }
template<typename T> inline static int Sgn(T val)  { return (T(0) < val) - (val < T(0)); }


// Heuristics. Add new ones if you need them.
namespace Heuristic
{
    inline ScoreType Manhattan(const Position& a, const Position& b)
    {
        const int dx = Abs(int(a.x - b.x));
        const int dy = Abs(int(a.y - b.y));
        return static_cast<ScoreType>(dx + dy);
    }

    inline ScoreType Chebyshev(const Position& a, const Position& b)
    {
        const int dx = Abs(int(a.x - b.x));
        const int dy = Abs(int(a.y - b.y));
        return static_cast<ScoreType>(Max(dx, dy));
    }
#ifdef JPS_sqrt
    inline ScoreType Euclidean(const Position& a, const Position& b)
    {
        const int dx = (int(a.x - b.x));
        const int dy = (int(a.y - b.y));
        return static_cast<ScoreType>(JPS_sqrt(dx*dx + dy*dy));
    }
#endif
} // end namespace heuristic



// --- Begin infrastructure, data structures ---

namespace Internal {

// Never allocated outside of a PodVec<Node> --> All nodes are linearly adjacent in memory.
struct Node
{
    ScoreType f, g; // heuristic distances
    Position pos;
    int parentOffs; // no parent if 0
    unsigned _flags;

    inline int hasParent() const { return parentOffs; }
    inline void setOpen() { _flags |= 1; }
    inline void setClosed() { _flags |= 2; }
    inline unsigned isOpen() const { return _flags & 1; }
    inline unsigned isClosed() const { return _flags & 2; }

    // We know nodes are allocated sequentially in memory, so this is fine.
    inline       Node& getParent()       { JPS_ASSERT(parentOffs); return this[parentOffs]; }
    inline const Node& getParent() const { JPS_ASSERT(parentOffs); return this[parentOffs]; }
    inline const Node *getParentOpt() const { return parentOffs ? this + parentOffs : 0; }
    inline void setParent(const Node& p) { JPS_ASSERT(&p != this); parentOffs = static_cast<int>(&p - this); }
};

template<typename T>
class PodVec
{
public:
    PodVec(void *user = 0)
        : _data(0), used(0), cap(0), _user(user)
    {}
    ~PodVec() { dealloc(); }
    inline void clear()
    {
        used = 0;
    }
    void dealloc()
    {
        JPS_free(_data, cap * sizeof(T), _user);
        _data = 0;
        used = 0;
        cap = 0;
    }
    T *alloc()
    {
        T *e = 0;
        if(used < cap || _grow())
        {
            e = _data + used;
            ++used;
        }
        return e;
    }
    inline void push_back(const T& e)
    {
        if(T *dst = alloc()) // yes, this silently fails when OOM. this is handled internally.
            *dst = e;
    }
    inline void pop_back() { JPS_ASSERT(used); --used; }
    inline T& back() { JPS_ASSERT(used); return _data[used-1]; }
    inline SizeT size() const { return used; }
    inline bool empty() const { return !used; }
    inline T *data() { return _data; }
    inline const T *data() const { return _data; }
    inline T& operator[](size_t idx) const { JPS_ASSERT(idx < used); return _data[idx]; }
    inline SizeT getindex(const T *e) const
    {
        JPS_ASSERT(e && _data <= e && e < _data + used);
        return static_cast<SizeT>(e - _data);
    }

    void *_reserve(SizeT newcap) // for internal use
    {
        return cap < newcap ? _grow(newcap) : _data;
    }
    void resize(SizeT sz)
    {
        if(_reserve(sz))
            used = sz;
    }
    SizeT _getMemSize() const
    {
        return cap * sizeof(T);
    }

    // minimal iterator interface
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef SizeT size_type;
    typedef T value_type;
    inline iterator begin() { return data(); }
    inline iterator end() { return data() + size(); }
    inline const_iterator cbegin() const { return data(); }
    inline const_iterator cend() const { return data() + size(); }

private:
    void *_grow(SizeT newcap)
    {
        void *p = JPS_realloc(_data, newcap * sizeof(T), cap * sizeof(T), _user);
        if(p)
        {
            _data = (T*)p;
            cap = newcap;
        }
        return p;
    }
    void * _grow()
    {
        const SizeT newcap = cap + (cap / 2) + 32;
        return _grow(newcap);
    }
    T *_data;
    SizeT used, cap;

public:
    void * const _user;

private:
    // forbid ops
    PodVec<T>& operator=(const PodVec<T>&);
    PodVec(const PodVec<T>&);
};

template<typename T>
inline static void Swap(T& a, T& b)
{
    const T tmp = a;
    a = b;
    b = tmp;
}

template<typename IT>
inline static void Reverse(IT first, IT last)
{
    while((first != last) && (first != --last))
    {
        Swap(*first, *last);
        ++first;
    }
}

typedef PodVec<Node> Storage;


class NodeMap
{
private:
    static const unsigned LOAD_FACTOR = 8; // estimate: {CPU cache line size (64)} / sizeof(HashLoc)
    static const unsigned INITIAL_BUCKETS = 16; // must be > 1 and power of 2

    struct HashLoc
    {
        unsigned hash2; // for early-out check only
        SizeT idx; // index in central storage
    };
    typedef PodVec<HashLoc> Bucket;

    // hash function to determine bucket. only uses lower few bits. should jumble lower bits nicely.
    static inline unsigned Hash(PosType x, PosType y)
    {
        return x ^ y;
    }

    // hash function designed to lose as little data as possible. for early-out checks. all bits used.
    static inline unsigned Hash2(PosType x, PosType y)
    {
        return (y << 16) ^ x;
    }

public:

    NodeMap(Storage& storage)
        : _storageRef(storage), _buckets(storage._user)
    {}

    ~NodeMap()
    {
        dealloc();
    }

    void dealloc()
    {
        for(SizeT i = 0; i < _buckets.size(); ++i)
            _buckets[i].~Bucket();
        _buckets.dealloc();
    }
    void clear()
    {
        // clear the buckets, but *not* the bucket vector
        for(SizeT i = 0; i <  _buckets.size(); ++i)
            _buckets[i].clear();
    }

    Node *operator()(PosType x, PosType y)
    {
        const unsigned h = Hash(x, y);
        const unsigned h2 = Hash2(x, y);
        const SizeT ksz = _buckets.size(); // known to be power-of-2
        Bucket *b = 0; // MSVC /W4 complains that this was uninitialized and used, so we init it...
        if (ksz)
        {
            b = &_buckets[h & (ksz - 1)];
            const SizeT bsz = b->size();
            const HashLoc * const bdata = b->data();
            for (SizeT i = 0; i < bsz; ++i)
            {
                // this is the only place where HashLoc::hash2 is used; it *could*be removed, which means:
                // - twice as much space for indexes per cache line
                // - but also higher chances for a cache miss because for each entry in the bucket we still need to check the node's X/Y coords,
                //   and we'll likely end up in a random location in RAM for each node.
                // Quick benchmarking showed that *with* the hash2 check it's almost immeasurably (less than 1%) faster.
                if (bdata[i].hash2 == h2)
                {
                    Node &n = _storageRef[bdata[i].idx];
                    if(n.pos.x == x && n.pos.y == y)
                        return &n;
                }
            }
        }

        // enlarge hashmap if necessary; fix bucket if so
        SizeT newbsz = _enlarge();
        if(newbsz > 1)
            b = &_buckets[h & (newbsz - 1)];
        else if(newbsz == 1) // error case
            return 0;

        HashLoc *loc = b->alloc(); // ... see above. b is always initialized here. when ksz==0, _enlarge() will do its initial allocation, so it can never return 0.

        if(!loc)
            return 0;

        loc->hash2 = h2;
        loc->idx = _storageRef.size();

        // no node at (x, y), create new one
        Node *n = _storageRef.alloc();
        if(n)
        {
            n->f = 0;
            n->g = 0;
            n->pos.x = x;
            n->pos.y = y;
            n->parentOffs = 0;
            n->_flags = 0;
        }
        return n;
    }

    SizeT _getMemSize() const
    {
        SizeT sum = _buckets._getMemSize();
        for(Buckets::const_iterator it = _buckets.cbegin(); it != _buckets.cend(); ++it)
            sum += it->_getMemSize();
        return sum;
    }

private:

    // return values: 0 = nothing to do; 1 = error; >1: internal storage was enlarged to this many buckets
    SizeT _enlarge()
    {
        const SizeT n = _storageRef.size();
        const SizeT oldsz = _buckets.size();
        if (n < oldsz * LOAD_FACTOR)
            return 0;

        // pre-allocate bucket storage that we're going to use
        const SizeT newsz = oldsz ? oldsz * 2 : INITIAL_BUCKETS; // stays power of 2

        if(!_buckets._reserve(newsz))
            return 0; // early out if realloc fails; this not a problem and we can continue.

        // forget everything
        for(SizeT i = 0; i < oldsz; ++i)
            _buckets[i].clear();

        // resize and init
        for(SizeT i = oldsz; i < newsz; ++i)
        {
            void *p = _buckets.alloc(); // can't fail since the space was reserved
            JPS_PLACEMENT_NEW(p) PodVec<HashLoc>(_buckets._user);
        }

        const SizeT mask = _buckets.size() - 1;
        for(SizeT i = 0; i < n; ++i)
        {
            const Position p = _storageRef[i].pos;
            HashLoc *loc = _buckets[Hash(p.x, p.y) & mask].alloc();
            if(!loc)
                return 1; // error case

            loc->hash2 = Hash2(p.x, p.y);
            loc->idx = i;
        }
        return newsz;
    }

    Storage& _storageRef;
    typedef PodVec<Bucket> Buckets;
    Buckets _buckets;
};

class OpenList
{
private:
    const Storage& _storageRef;
    PodVec<SizeT> idxHeap;

public:

    OpenList(const Storage& storage)
        : _storageRef(storage), idxHeap(storage._user)
    {}


    inline void pushNode(Node *n)
    {
        _heapPushIdx(_storageRef.getindex(n));
    }

    inline Node& popNode()
    {
        return _storageRef[_popIdx()];
    }

    // re-heapify after node changed its order
    inline void fixNode(const Node& n)
    {
        const unsigned ni = _storageRef.getindex(&n);
        const unsigned sz = idxHeap.size();
        unsigned *p = idxHeap.data();
        for(unsigned i = 0; i < sz; ++i) // TODO: if this ever becomes a perf bottleneck: make it so that each node knows its heap index
            if(p[i] == ni)
            {
                _fixIdx(i);
                return;
            }
            JPS_ASSERT(false); // expect node to be found
    }

    inline void dealloc() { idxHeap.dealloc(); }
    inline void clear()   { idxHeap.clear(); }
    inline bool empty() const { return idxHeap.empty(); }

    inline SizeT _getMemSize() const
    {
        return idxHeap._getMemSize();
    }

private:

    inline bool _heapLess(SizeT a, SizeT b)
    {
        return _storageRef[idxHeap[a]].f > _storageRef[idxHeap[b]].f;
    }

    inline bool _heapLessIdx(SizeT a, SizeT idx)
    {
        return _storageRef[idxHeap[a]].f > _storageRef[idx].f;
    }

    void _percolateUp(SizeT i)
    {
        const SizeT idx = idxHeap[i];
        SizeT p;
        goto start;
        do
        {
            idxHeap[i] = idxHeap[p]; // parent is smaller, move it down
            i = p;                   // continue with parent
start:
            p = (i - 1) >> 1;
        }
        while(i && _heapLessIdx(p, idx));
        idxHeap[i] = idx; // found correct place for idx
    }

    void _percolateDown(SizeT i)
    {
        const SizeT idx = idxHeap[i];
        const SizeT sz = idxHeap.size();
        SizeT child;
        goto start;
        do
        {
            // pick right sibling if exists and larger or equal
            if(child + 1 < sz && !_heapLess(child+1, child))
                ++child;
            idxHeap[i] = idxHeap[child];
            i = child;
start:
            child = (i << 1) + 1;
        }
        while(child < sz);
        idxHeap[i] = idx;
        _percolateUp(i);
    }

    void _heapPushIdx(SizeT idx)
    {
        SizeT i = idxHeap.size();
        idxHeap.push_back(idx);
        _percolateUp(i);
    }

    SizeT _popIdx()
    {
        SizeT sz = idxHeap.size();
        JPS_ASSERT(sz);
        const SizeT root = idxHeap[0];
        idxHeap[0] = idxHeap[--sz];
        idxHeap.pop_back();
        if(sz > 1)
            _percolateDown(0);
        return root;
    }

    // re-heapify node at index i
    inline void _fixIdx(SizeT i)
    {
        _percolateDown(i);
        _percolateUp(i);
    }
};

#undef JPS_PLACEMENT_NEW

// --- End infrastructure, data structures ---

// All those things that don't depend on template parameters...
class SearcherBase
{
protected:
    Storage storage;
    OpenList open;
    NodeMap nodemap;

    Position endPos;
    SizeT endNodeIdx;
    JPS_Flags flags;
    int stepsRemain;
    SizeT stepsDone;


    SearcherBase(void *user)
        : storage(user)
        , open(storage)
        , nodemap(storage)
        , endPos(npos), endNodeIdx(noidx)
        , flags(0)
        , stepsRemain(0), stepsDone(0)
    {}

    void clear()
    {
        open.clear();
        nodemap.clear();
        storage.clear();
        endNodeIdx = noidx;
        stepsDone = 0;
    }

    void _expandNode(const Position jp, Node& jn, const Node& parent)
    {
        JPS_ASSERT(jn.pos == jp);
        ScoreType extraG = JPS_HEURISTIC_ACCURATE(jp, parent.pos);
        ScoreType newG = parent.g + extraG;
        if(!jn.isOpen() || newG < jn.g)
        {
            jn.g = newG;
            jn.f = jn.g + JPS_HEURISTIC_ESTIMATE(jp, endPos);
            jn.setParent(parent);
            if(!jn.isOpen())
            {
                open.pushNode(&jn);
                jn.setOpen();
            }
            else
                open.fixNode(jn);
        }
    }

public:

    template <typename PV>
    JPS_Result generatePath(PV& path, unsigned step) const;

    void freeMemory()
    {
        open.dealloc();
        nodemap.dealloc();
        storage.dealloc();
        endNodeIdx = noidx;
    }

    // --- Statistics ---

    inline SizeT getStepsDone() const { return stepsDone; }
    inline SizeT getNodesExpanded() const { return storage.size(); }

    SizeT getTotalMemoryInUse() const
    {
        return storage._getMemSize()
             + nodemap._getMemSize()
             + open._getMemSize();
    }
};

template <typename GRID> class Searcher : public SearcherBase
{
public:
    Searcher(const GRID& g, void *user = 0)
        : SearcherBase(user), grid(g)
    {}

    // single-call
    template<typename PV>
    bool findPath(PV& path, Position start, Position end, unsigned step, JPS_Flags flags = JPS_Flag_Default);

    // incremental pathfinding
    JPS_Result findPathInit(Position start, Position end, JPS_Flags flags = JPS_Flag_Default);
    JPS_Result findPathStep(int limit);
    // generate path after one was found
    template<typename PV>
    JPS_Result findPathFinish(PV& path, unsigned step) const;

private:

    const GRID& grid;

    Node *getNode(const Position& pos);
    bool identifySuccessors(const Node& n);

    bool findPathGreedy(Node *start, Node *end);
    
    unsigned findNeighborsAStar(const Node& n, Position *wptr);

    unsigned findNeighborsJPS(const Node& n, Position *wptr) const;
    Position jumpP(const Position& p, const Position& src);
    Position jumpD(Position p, int dx, int dy);
    Position jumpX(Position p, int dx);
    Position jumpY(Position p, int dy);

    // forbid any ops
    Searcher& operator=(const Searcher<GRID>&);
    Searcher(const Searcher<GRID>&);
};


// -----------------------------------------------------------------------

template<typename PV> JPS_Result SearcherBase::generatePath(PV& path, unsigned step) const
{
    if(endNodeIdx == noidx)
        return JPS_NO_PATH;
    const SizeT offset = path.size();
    SizeT added = 0;
    const Node& endNode = storage[endNodeIdx];
    const Node *next = &endNode;
    if(!next->hasParent())
        return JPS_NO_PATH;
    if(step)
    {
        const Node *prev = endNode.getParentOpt();
        if(!prev)
            return JPS_NO_PATH;
        do
        {
            const unsigned x = next->pos.x, y = next->pos.y;
            int dx = int(prev->pos.x - x);
            int dy = int(prev->pos.y - y);
            const int adx = Abs(dx);
            const int ady = Abs(dy);
            JPS_ASSERT(!dx || !dy || adx == ady); // known to be straight, if diagonal
            const int steps = Max(adx, ady);
            dx = int(step) * Sgn(dx);
            dy = int(step) * Sgn(dy);
            int dxa = 0, dya = 0;
            for(int i = 0; i < steps; i += step)
            {
                path.push_back(Pos(x+dxa, y+dya));
                ++added;
                dxa += dx;
                dya += dy;
            }
            next = prev;
            prev = prev->getParentOpt();
        }
        while (prev);
    }
    else
    {
        do
        {
            JPS_ASSERT(next != &next->getParent());
            path.push_back(next->pos);
            ++added;
            next = &next->getParent();
        }
        while (next->hasParent());
    }

    // JPS::PathVector silently discards push_back() when memory allocation fails;
    // detect that case and roll back.
    if(path.size() != offset + added)
    {
        path.resize(offset);
        return JPS_OUT_OF_MEMORY;
    }

    // Nodes were traversed backwards, fix that
    Reverse(path.begin() + offset, path.end());
    return JPS_FOUND_PATH;
}

//-----------------------------------------

template <typename GRID> inline Node *Searcher<GRID>::getNode(const Position& pos)
{
    JPS_ASSERT(grid(pos.x, pos.y));
    return nodemap(pos.x, pos.y);
}

template <typename GRID> Position Searcher<GRID>::jumpP(const Position &p, const Position& src)
{
    JPS_ASSERT(grid(p.x, p.y));

    int dx = int(p.x - src.x);
    int dy = int(p.y - src.y);
    JPS_ASSERT(dx || dy);

    if(dx && dy)
        return jumpD(p, dx, dy);
    else if(dx)
        return jumpX(p, dx);
    else if(dy)
        return jumpY(p, dy);

    // not reached
    JPS_ASSERT(false);
    return npos;
}

template <typename GRID> Position Searcher<GRID>::jumpD(Position p, int dx, int dy)
{
    JPS_ASSERT(grid(p.x, p.y));
    JPS_ASSERT(dx && dy);

    const Position endpos = endPos;
    unsigned steps = 0;

    while(true)
    {
        if(p == endpos)
            break;

        ++steps;
        const PosType x = p.x;
        const PosType y = p.y;

        if( (grid(x-dx, y+dy) && !grid(x-dx, y)) || (grid(x+dx, y-dy) && !grid(x, y-dy)) )
            break;

        const bool gdx = !!grid(x+dx, y);
        const bool gdy = !!grid(x, y+dy);

        if(gdx && jumpX(Pos(x+dx, y), dx).isValid())
            break;

        if(gdy && jumpY(Pos(x, y+dy), dy).isValid())
            break;

        if((gdx || gdy) && grid(x+dx, y+dy))
        {
            p.x += dx;
            p.y += dy;
        }
        else
        {
            p = npos;
            break;
        }
    }
    stepsDone += steps;
    stepsRemain -= steps;
    return p;
}

template <typename GRID> inline Position Searcher<GRID>::jumpX(Position p, int dx)
{
    JPS_ASSERT(dx);
    JPS_ASSERT(grid(p.x, p.y));

    const PosType y = p.y;
    const Position endpos = endPos;
    unsigned steps = 0;

    unsigned a = ~((!!grid(p.x, y+1)) | ((!!grid(p.x, y-1)) << 1));

    while(true)
    {
        const unsigned xx = p.x + dx;
        const unsigned b = (!!grid(xx, y+1)) | ((!!grid(xx, y-1)) << 1);

        if((b & a) || p == endpos)
            break;
        if(!grid(xx, y))
        {
            p = npos;
            break;
        }

        p.x += dx;
        a = ~b;
        ++steps;
    }

    stepsDone += steps;
    stepsRemain -= steps;
    return p;
}

template <typename GRID> inline Position Searcher<GRID>::jumpY(Position p, int dy)
{
    JPS_ASSERT(dy);
    JPS_ASSERT(grid(p.x, p.y));

    const PosType x = p.x;
    const Position endpos = endPos;
    unsigned steps = 0;

    unsigned a = ~((!!grid(x+1, p.y)) | ((!!grid(x-1, p.y)) << 1));

    while(true)
    {
        const unsigned yy = p.y + dy;
        const unsigned b = (!!grid(x+1, yy)) | ((!!grid(x-1, yy)) << 1);

        if((a & b) || p == endpos)
            break;
        if(!grid(x, yy))
        {
            p = npos;
            break;
        }

        p.y += dy;
        a = ~b;
        ++steps;
    }

    stepsDone += steps;
    stepsRemain -= steps;
    return p;
}

#define JPS_CHECKGRID(dx, dy) (grid(x+(dx), y+(dy)))
#define JPS_ADDPOS(dx, dy)     do { *w++ = Pos(x+(dx), y+(dy)); } while(0)
#define JPS_ADDPOS_CHECK(dx, dy) do { if(JPS_CHECKGRID(dx, dy)) JPS_ADDPOS(dx, dy); } while(0)
#define JPS_ADDPOS_NO_TUNNEL(dx, dy) do { if(grid(x+(dx),y) || grid(x,y+(dy))) JPS_ADDPOS_CHECK(dx, dy); } while(0)

template <typename GRID> unsigned Searcher<GRID>::findNeighborsJPS(const Node& n, Position *wptr) const
{
    Position *w = wptr;
    const unsigned x = n.pos.x;
    const unsigned y = n.pos.y;

    if(!n.hasParent())
    {
        // straight moves
        JPS_ADDPOS_CHECK(-1, 0);
        JPS_ADDPOS_CHECK(0, -1);
        JPS_ADDPOS_CHECK(0, 1);
        JPS_ADDPOS_CHECK(1, 0);

        // diagonal moves + prevent tunneling
        JPS_ADDPOS_NO_TUNNEL(-1, -1);
        JPS_ADDPOS_NO_TUNNEL(-1, 1);
        JPS_ADDPOS_NO_TUNNEL(1, -1);
        JPS_ADDPOS_NO_TUNNEL(1, 1);

        return unsigned(w - wptr);
    }
    const Node& p = n.getParent();
    // jump directions (both -1, 0, or 1)
    const int dx = Sgn<int>(x - p.pos.x);
    const int dy = Sgn<int>(y - p.pos.y);

    if(dx && dy)
    {
        // diagonal
        // natural neighbors
        const bool walkX = !!grid(x+dx, y);
        if(walkX)
            *w++ = Pos(x+dx, y);
        const bool walkY = !!grid(x, y+dy);
        if(walkY)
            *w++ = Pos(x, y+dy);

        if(walkX || walkY)
            JPS_ADDPOS_CHECK(dx, dy);

        // forced neighbors
        if(walkY && !JPS_CHECKGRID(-dx,0))
            JPS_ADDPOS_CHECK(-dx, dy);

        if(walkX && !JPS_CHECKGRID(0,-dy))
            JPS_ADDPOS_CHECK(dx, -dy);
    }
    else if(dx)
    {
        // along X axis
        if(JPS_CHECKGRID(dx, 0))
        {
            JPS_ADDPOS(dx, 0);

             // Forced neighbors (+ prevent tunneling)
            if(!JPS_CHECKGRID(0, 1))
                JPS_ADDPOS_CHECK(dx, 1);
            if(!JPS_CHECKGRID(0,-1))
                JPS_ADDPOS_CHECK(dx,-1);
        }
    }
    else if(dy)
    {
        // along Y axis
        if(JPS_CHECKGRID(0, dy))
        {
            JPS_ADDPOS(0, dy);

            // Forced neighbors (+ prevent tunneling)
            if(!JPS_CHECKGRID(1, 0))
                JPS_ADDPOS_CHECK(1, dy);
            if(!JPS_CHECKGRID(-1, 0))
                JPS_ADDPOS_CHECK(-1,dy);
        }
    }

    return unsigned(w - wptr);
}

//-------------- Plain old A* search ----------------
template <typename GRID> unsigned Searcher<GRID>::findNeighborsAStar(const Node& n, Position *wptr)
{
    Position *w = wptr;
    const int x = n.pos.x;
    const int y = n.pos.y;
    const int d = 1;
    JPS_ADDPOS_NO_TUNNEL(-d, -d);
    JPS_ADDPOS_CHECK    ( 0, -d);
    JPS_ADDPOS_NO_TUNNEL(+d, -d);
    JPS_ADDPOS_CHECK    (-d,  0);
    JPS_ADDPOS_CHECK    (+d,  0);
    JPS_ADDPOS_NO_TUNNEL(-d, +d);
    JPS_ADDPOS_CHECK    ( 0, +d);
    JPS_ADDPOS_NO_TUNNEL(+d, +d);
    stepsDone += 8;
    return unsigned(w - wptr);
}

//-------------------------------------------------
#undef JPS_ADDPOS
#undef JPS_ADDPOS_CHECK
#undef JPS_ADDPOS_NO_TUNNEL
#undef JPS_CHECKGRID


template <typename GRID> bool Searcher<GRID>::identifySuccessors(const Node& n_)
{
    const SizeT nidx = storage.getindex(&n_);
    const Position np = n_.pos;
    Position buf[8];

    const int num = (flags & JPS_Flag_AStarOnly)
        ? findNeighborsAStar(n_, &buf[0])
        : findNeighborsJPS(n_, &buf[0]);

    for(int i = num-1; i >= 0; --i)
    {
        // Invariant: A node is only a valid neighbor if the corresponding grid position is walkable (asserted in jumpP)
        Position jp;
        if(flags & JPS_Flag_AStarOnly)
            jp = buf[i];
        else
        {
            jp = jumpP(buf[i], np);
            if(!jp.isValid())
                continue;
        }
        // Now that the grid position is definitely a valid jump point, we have to create the actual node.
        Node *jn = getNode(jp); // this might realloc the storage
        if(!jn)
            return false; // out of memory

        Node& n = storage[nidx]; // get valid ref in case we realloc'd
        JPS_ASSERT(jn != &n);
        if(!jn->isClosed())
            _expandNode(jp, *jn, n);
    }
    return true;
}

template <typename GRID> template<typename PV> bool Searcher<GRID>::findPath(PV& path, Position start, Position end, unsigned step, JPS_Flags flags)
{
    JPS_Result res = findPathInit(start, end, flags);

    // If this is true, the resulting path is empty (findPathFinish() would fail, so this needs to be checked before)
    if(res == JPS_EMPTY_PATH)
        return true;

    while(true)
    {
        switch(res)
        {
            case JPS_NEED_MORE_STEPS:
                res = findPathStep(0);
                break; // the switch

            case JPS_FOUND_PATH:
                return findPathFinish(path, step) == JPS_FOUND_PATH;

            case JPS_EMPTY_PATH:
                JPS_ASSERT(false); // can't happen
                // fall through
            case JPS_NO_PATH:
            case JPS_OUT_OF_MEMORY:
                return false;
        }
    }
}

template <typename GRID> JPS_Result Searcher<GRID>::findPathInit(Position start, Position end, JPS_Flags flags)
{
    // This just resets a few counters; container memory isn't touched
    this->clear();

    this->flags = flags;
    endPos = end;

    // FIXME: check this
    if(start == end && !(flags & (JPS_Flag_NoStartCheck|JPS_Flag_NoEndCheck)))
    {
        // There is only a path if this single position is walkable.
        // But since the starting position is omitted in the output, there is nothing to do here.
        return grid(end.x, end.y) ? JPS_EMPTY_PATH : JPS_NO_PATH;
    }

    if(!(flags & JPS_Flag_NoStartCheck))
        if(!grid(start.x, start.y))
            return JPS_NO_PATH;

    if(!(flags & JPS_Flag_NoEndCheck))
        if(!grid(end.x, end.y))
            return JPS_NO_PATH;

    Node *endNode = getNode(end); // this might realloc the internal storage...
    if(!endNode)
        return JPS_OUT_OF_MEMORY;
    endNodeIdx = storage.getindex(endNode); // .. so we keep this for later

    Node *startNode = getNode(start); // this might also realloc
    if(!startNode)
        return JPS_OUT_OF_MEMORY;
    endNode = &storage[endNodeIdx]; // startNode is valid, make sure that endNode is valid too in case we reallocated

    if(!(flags & JPS_Flag_NoGreedy))
    {
        // Try the quick way out first
        if(findPathGreedy(startNode, endNode))
            return JPS_FOUND_PATH;
    }

    open.pushNode(startNode);

    return JPS_NEED_MORE_STEPS;
}

template <typename GRID> JPS_Result Searcher<GRID>::findPathStep(int limit)
{
    stepsRemain = limit;
    do
    {
        if(open.empty())
            return JPS_NO_PATH;
        Node& n = open.popNode();
        n.setClosed();
        if(n.pos == endPos)
            return JPS_FOUND_PATH;
        if(!identifySuccessors(n))
            return JPS_OUT_OF_MEMORY;
    }
    while(stepsRemain >= 0);
    return JPS_NEED_MORE_STEPS;
}

template<typename GRID> template<typename PV> JPS_Result Searcher<GRID>::findPathFinish(PV& path, unsigned step) const
{
    return this->generatePath(path, step);
}

template<typename GRID> bool Searcher<GRID>::findPathGreedy(Node *n, Node *endnode)
{
    Position midpos = npos;
    PosType x = n->pos.x;
    PosType y = n->pos.y;
    const Position endpos = endnode->pos;

    JPS_ASSERT(x != endpos.x || y != endpos.y); // must not be called when start==end
    JPS_ASSERT(n != endnode);

    int dx = int(endpos.x - x);
    int dy = int(endpos.y - y);
    const int adx = Abs(dx);
    const int ady = Abs(dy);
    dx = Sgn(dx);
    dy = Sgn(dy);

    // go diagonally first
    if(x != endpos.x && y != endpos.y)
    {
        JPS_ASSERT(dx && dy);
        const int minlen = Min(adx, ady);
        const PosType tx = x + dx * minlen;
        while(x != tx)
        {
            if(grid(x, y) && (grid(x+dx, y) || grid(x, y+dy))) // prevent tunneling as well
            {
                x += dx;
                y += dy;
            }
            else
                return false;
        }

        if(!grid(x, y))
            return false;

        midpos = Pos(x, y);
    }

    // at this point, we're aligned to at least one axis
    JPS_ASSERT(x == endpos.x || y == endpos.y);

    if(!(x == endpos.x && y == endpos.y))
    {
        while(x != endpos.x)
            if(!grid(x += dx, y))
                return false;

        while(y != endpos.y)
            if(!grid(x, y += dy))
                return false;

        JPS_ASSERT(x == endpos.x && y == endpos.y);
    }

    if(midpos.isValid())
    {
        const unsigned nidx = storage.getindex(n);
        Node *mid = getNode(midpos); // this might invalidate n, endnode
        if(!mid)
            return false;
        n = &storage[nidx]; // reload pointers
        endnode = &storage[endNodeIdx];
        JPS_ASSERT(mid && mid != n);
        mid->setParent(*n);
        if(mid != endnode)
            endnode->setParent(*mid);
    }
    else
        endnode->setParent(*n);

    return true;
}

#undef JPS_ASSERT
#undef JPS_realloc
#undef JPS_free
#undef JPS_sqrt
#undef JPS_HEURISTIC_ACCURATE
#undef JPS_HEURISTIC_ESTIMATE


} // end namespace Internal

using Internal::Searcher;

typedef Internal::PodVec<Position> PathVector;

// Single-call convenience function. For efficiency, do NOT use this if you need to compute paths repeatedly.
//
// Returns: 0 if failed or no path could be found, otherwise number of steps taken.
//
// path: If the function returns success, the path is appended to this vector.
//       The path does NOT contain the starting position, i.e. if start and end are the same,
//       the resulting path has no elements.
//       The vector does not have to be empty. The function does not clear it;
//       instead, the new path positions are appended at the end.
//       This allows building a path incrementally.
//
// grid: Functor, expected to overload operator()(x, y), return true if position is walkable, false if not.
//
// step: If 0, only return waypoints.
//       If 1, create exhaustive step-by-step path.
//       If N, put in one position for N blocks travelled, or when a waypoint is hit.
//       All returned points are guaranteed to be on a straight line (vertically, horizontally, or diagonally),
//       and there is no obstruction between any two consecutive points.
//       Note that this parameter does NOT influence the pathfinding in any way;
//       it only controls the coarseness of the output path.
template <typename GRID, typename PV>
SizeT findPath(PV& path, const GRID& grid, PosType startx, PosType starty, PosType endx, PosType endy,
               unsigned step = 0, // optional
               JPS_Flags flags = JPS_Flag_Default,
               void *user = 0)    // memory allocation userdata
{
    Searcher<GRID> search(grid, user);
    if(!search.findPath(path, Pos(startx, starty), Pos(endx, endy), step, flags))
        return 0;
    const SizeT done = search.getStepsDone();
    return done + !done; // report at least 1 step; as 0 would indicate failure
}

} // end namespace JPS


/*
Changes compared to the older JPS.h at https://github.com/fgenesis/jps:

- Explicitly freeing memory is no longer necessary. The freeMemory() method is still there
  and does its thing (drop all internal storage), but you never have to call it explicitly.
  Unlike the old version, there will be no performance degradation if you don't free memory every now and then.
  Actually it'll be slightly slower if you free memory and pathfind again for the first time,
  as it has to re-allocate internal data structures.

- Searcher::getNodesExpanded() is now reset to 0 upon starting a search.

- Added optional JPS_Flags parameter to pathfind (-init) functions to control search
  behavior. Compile-time #defines are gone.

- Removed skip parameter. Imho that one just added confusion and no real benefit.
  If you want it back for some reason: poke me, open an issue, whatever.

- Renamed JPS::Result to JPS_Result. Enum values gained JPS_ prefix, so JPS::NO_PATH is now JPS_NO_PATH, and so on.

- Added one more JPS_Result value: JPS_OUT_OF_MEMORY. See info block at the top how to handle this.

- Changed signature of Searcher<>::findPathFinish() to return JPS_Result (was bool).
  This is more in line with the other 2 methods, as it can now return JPS_OUT_OF_MEMORY.

- Changed signature of JPS::findPath(). Nonzero return is still success. Pointers to output stats are gone.
  Use a Searcher instance if you need the details.

- This version no longer depends on the C++ STL: <algorithm>, <vector>, <map>, operator new(), all gone.
  Makes things more memory- and cache-friendly, and quite a bit faster, too.

- The canonical file name is now "jps.hh" instead of "JPS.h"
*/


/*
TODO:
- make int -> DirType
- make possible to call findPathStep()/findPathFinish() even when JPS_EMPTY_PATH was returned on init (simplifies switch-case)
- make node know its heap index
- optional diagonals (make runtime param)
*/
