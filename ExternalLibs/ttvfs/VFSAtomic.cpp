// VFSAtomic.cpp - atomic operations and thread locking
// For conditions of distribution and use, see copyright notice in VFS.h

/** --- Atomic operations and thread safety ---
  * You may want to add your own implementation if thread safety is needed.
  * If not, just leave everything like it is.

  * If you are on windows, Interlocked[In/De]crement is faster than
    explicit mutex locking for integer operations.

  * TODO: The actual locking that is done in the tree when VFS_THREADSAFE is defined
    is rather crude for the time beeing; a somewhat more efficient ReadWriteLock
    implementation would be nice to have, someday.

  * If you can, leave VFS_THREADSAFE undefined and do the locking externally,
    it will probably have much better performance than if each and every operation
    does a lock and unlock call.
    (For a rather I/O based library this should not really make a difference, anyway.
    But don't say you haven't been warned :) )
*/

#include "VFSInternal.h"
#include "VFSAtomic.h"

// for Interlocked[In/De]crement, if required
#if defined(_WIN32) && defined(VFS_THREADSAFE)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#endif

VFS_NAMESPACE_START

#ifdef VFS_THREADSAFE
static Mutex mtx;
#endif

int Atomic_Incr(volatile int &i)
{
#ifdef VFS_THREADSAFE
#   ifdef _WIN32
        volatile LONG* dp = (volatile LONG*) &i;
        return InterlockedIncrement( dp );
#   else
        Guard g(mtx);
#   endif
#endif
    return ++i;
}

int Atomic_Decr(volatile int &i)
{
#ifdef VFS_THREADSAFE
#   ifdef _WIN32
        volatile LONG* dp = (volatile LONG*) &i;
        return InterlockedDecrement( dp );
#   else
        Guard g(mtx);
#   endif
#endif
    return --i;
}

/* Implement your Mutex class here.
   Important: The mutex must be re-entrant/recursive,
   means it must be possible to lock it from the same thread multiple times.
*/
Mutex::Mutex()
{
    // implement your own if needed. Remove the trap below when you are done.
    // This is to prevent people from defining VFS_THREADSAFE and expecting everything to work just like that :)
#ifdef VFS_THREADSAFE
#error VFSAtomic: Hey, you forgot to implement the mutex class, cant guarantee thread safety! Either undef VFS_THREADSAFE or read the docs and get your hands dirty.
#endif
}

Mutex::~Mutex()
{
    // implement your own if needed
}

void Mutex::Lock(void)
{
    // implement your own if needed
}

void Mutex::Unlock(void)
{
    // implement your own if needed
}

Guard::Guard(Mutex& m)
: _m(m)
{
    _m.Lock();
}

Guard::~Guard()
{
    _m.Unlock();
}


VFS_NAMESPACE_END