#ifndef SELFREFCOUNTER_H
#define SELFREFCOUNTER_H

#include "VFSDefines.h"
#include "VFSAtomic.h"

VFS_NAMESPACE_START

// self must point to the object that holds the counter.
template <class T, bool DELSELF = true> class SelfRefCounter
{
private:
    T *self;
    volatile int c;
    SelfRefCounter(SelfRefCounter& r); // forbid copy constructor
    inline unsigned int _deref(void)
    {
        unsigned int cc = (unsigned int)Atomic_Decr(c); // copy c, in case we get deleted
        if(DELSELF && !cc)
        {
            delete self;
        }

        return cc;
    }

public:
    SelfRefCounter(T *p): self(p), c(1) {}
    ~SelfRefCounter() { /* DEBUG(ASSERT(c <= 1)); */ } // its ok if the last reference calls delete instead of _deref()
    inline unsigned int count(void) { return c; }

    // post-increment (dummy int)
    inline unsigned int operator++(int) { unsigned int cc = c; Atomic_Incr(c); return cc; }
    inline unsigned int operator--(int) { unsigned int cc = c; _deref(); return cc; }

    // pre-increment
    inline unsigned int operator++(void) { return (unsigned int)Atomic_Incr(c); }
    inline unsigned int operator--(void) { return _deref(); }
};

VFS_NAMESPACE_END


#endif
