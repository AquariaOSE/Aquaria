// VFSAtomic.h - atomic operations and thread locking
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFS_ATOMIC_H
#define VFS_ATOMIC_H

#include "VFSDefines.h"

VFS_NAMESPACE_START

int Atomic_Incr(volatile int &i);
int Atomic_Decr(volatile int &i);

// generic Mutex class, needs to be reentrant/recursive.
class Mutex
{
public:
    Mutex();
    ~Mutex();
    void Lock();
    void Unlock();

protected:
    // add own stuff if needed
};

class Guard
{
public:
    Guard(Mutex& m);
    ~Guard();

protected:
    Mutex& _m;
};

VFS_NAMESPACE_END

#endif
