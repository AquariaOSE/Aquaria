// VFSBase.h - bass class for VFSDir and VFSFile
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFS_BASE_H
#define VFS_BASE_H

#include <string>
#include "VFSDefines.h"
#include "VFSSelfRefCounter.h"

VFS_NAMESPACE_START

// Used internally. No special properties, just holds some common code.
class VFSBase
{
public:
    virtual ~VFSBase() {}

    /** Returns the plain file name. Never NULL. */
    inline const char *name() const { VFS_GUARD_OPT(this); return _name; }

    /** Returns the file name with full path. Never NULL. */
    inline const char *fullname() const { VFS_GUARD_OPT(this); return _fullname.c_str(); }

    /** To avoid strlen() */
    inline size_t fullnameLen() const { VFS_GUARD_OPT(this); return _fullname.length(); }
    // We know that mem addr of _name > _fullname:
    // _fullname: "abc/def/ghi/hjk.txt" (length = 19)
    // _name:                 "hjk.txt" <-- want that length
    // ptr diff: 12
    // so in total: 19 - 12 == 7
    inline size_t nameLen() const { VFS_GUARD_OPT(this); return _fullname.length() - (_name - _fullname.c_str()); }

    /** Basic RTTI, for debugging purposes */
    virtual const char *getType() const { return "<BASE>"; }

    /** Can be overloaded to close resources this object keeps open */
    virtual bool close() { return true; }

    /** Returns an object this object depends on. (used internally, by extensions) */
    inline VFSBase *getOrigin() const { return _origin; }

    inline void lock() const { _mtx.Lock(); }
    inline void unlock() const { _mtx.Unlock(); }
    inline Mutex& mutex() const { return _mtx; }

#ifdef VFS_USE_HASHMAP
    inline size_t hash() const { return _hash; }
#endif

    // For internal use
    inline void _setOrigin(VFSBase *origin) { _origin = origin; }

protected:
    VFSBase();
    void _setName(const char *n);

private:

#ifdef VFS_USE_HASHMAP
    size_t _hash;
#endif

    const char *_name; // must point to an address constant during object lifetime (like _fullname.c_str() + N)
                       // (not necessary to have an additional string copy here, just wastes memory)
    std::string _fullname;

    mutable Mutex _mtx;

    VFSBase *_origin; // May store a pointer if necessary. NOT ref-counted, because this would create cycles in almost all cases.

public:

    /** Reference count, if the pointer to this file is stored somewhere it is advisable to increase
    (ref++) it. If it reaches 0, this file is deleted automatically. */
    SelfRefCounter<VFSBase> ref;
};



VFS_NAMESPACE_END

#endif
