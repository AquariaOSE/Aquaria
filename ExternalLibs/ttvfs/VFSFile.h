// VFSFile.h - basic file interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFSFILE_H
#define VFSFILE_H

#include "VFSDefines.h"
#include "VFSSelfRefCounter.h"
#include <string>

VFS_NAMESPACE_START


/** -- VFSFile basic interface --
  * All functions that return bool should return true on success and false on failure.
  * If an operation is not necessary or irrelevant (for example, files in memory can't be closed),
  *    it is useful to return true anyways, because this operation did not fail, technically.
  *    (Common sense here!)
  * An int/vfspos value of 0 indicates failure, except the size/seek/getpos functions, where npos means failure.
  * Only the functions required or applicable need to be implemented, for unsupported operations
  *    the default implementation should be sufficient.
  **/
class VFSFile
{
public:

    /** The ctor is expected to set both name() and fullname(). */
    VFSFile();

    virtual ~VFSFile() {};

    /** Open a file. If fn is NULL (the default), open fullname().
        Mode can be "r", "w", "rb", "rb", and possibly other things that fopen supports.
        It is the subclass's choice to support other modes. Default is "rb". */
    virtual bool open(const char *fn = NULL, const char *mode = NULL) { return false; }
    virtual bool isopen(void) const { return false; }
    virtual bool iseof(void) const { return true; }

    /** Returns the plain file name. Never NULL. */
    virtual const char *name(void) const { return ""; }

    /** Returns the file name with full path. Never NULL. */
    virtual const char *fullname(void) const { return ""; }

    virtual bool close(void) { return true; }
    virtual bool seek(vfspos pos) { return false; }

    /** Seek relative to current position. Negative numbers will seek backwards.
        (In most cases, the default implementation does not have to be changed) */
    virtual bool seekRel(vfspos offs) { VFS_GUARD_OPT(this); return seek(getpos() + offs); }

    virtual bool flush(void) { return true; }

    /** Current offset in file. Return npos if NA. */
    virtual vfspos getpos(void) const { return npos; }

    virtual unsigned int read(void *dst, unsigned int bytes) { return 0; }
    virtual unsigned int write(const void *src, unsigned int bytes) { return 0; }

    /** Return file size. If NA, return npos. If size is not yet known,
        open() and close() may be called (with default args) to find out the size.
        The file is supposed to be in its old state when the function returns,
        that is in the same open state and seek position.
        The pointer returned by getBuf() must not change. */
    virtual vfspos size(void) { return npos; }

    /** Attempt to increase file size. Returns new size after completion.
        May return any size. Failure is indicated by a size() that didn't change. */
    virtual vfspos size(vfspos newsize) { return size(); }

    /** Return full file content in memory. Like size(), this may do other operations on the file,
        but after the function returns the file is expected to be in the same state it was before.
        If the file is not open before the call, it will be opened with default parameters (that is, "rb").
        Addition EOL mangling my happen if the file is opened in text mode before (= not binary).
        Calls to open() should delete this memory if the file was previously opened in a different mode.
        The returned memory is not guaranteed to be writable without problems, so don't do it.
        Don't cast the const away. You have been warned.
        This memory can be freed with free(), after calling dropBuf(false). */
    virtual const void *getBuf(void) { return NULL; }

    /** If del is true, delete internal buffer. If false, unregister internal buffer from the file,
        but do not delete. Use free() later. */
    virtual void dropBuf(bool del) {}

    /** Basic RTTI, for debugging purposes */
    virtual const char *getType(void) const { return "<BASE>"; }


    /** Reference count, if the pointer to this file is stored somewhere it is advisable to increase
        (ref++) it. If it reaches 0, this file is deleted automatically. */
    SelfRefCounter<VFSFile> ref;


    inline void lock() { _mtx.Lock(); }
    inline void unlock() { _mtx.Unlock(); }
    inline Mutex& mutex() const { return _mtx; }

protected:

    mutable Mutex _mtx;
};

class VFSFileReal : public VFSFile
{
public:
    VFSFileReal(const char *name = NULL);
    virtual ~VFSFileReal();
    virtual bool open(const char *fn = NULL, const char *mode = NULL);
    virtual bool isopen(void) const;
    virtual bool iseof(void) const;
    virtual const char *name(void) const;
    virtual const char *fullname(void) const;
    virtual bool close(void);
    virtual bool seek(vfspos pos);
    virtual bool seekRel(vfspos offs);
    virtual bool flush(void);
    virtual vfspos getpos(void) const;
    virtual unsigned int read(void *dst, unsigned int bytes);
    virtual unsigned int write(const void *src, unsigned int bytes);
    virtual vfspos size(void);
    virtual const void *getBuf(void);
    virtual void dropBuf(bool del);
    virtual const char *getType(void) const { return "disk"; }

    inline void *getFP() { return _fh; }

protected:
    void _setName(const char *n);
    std::string _fullname;
    const char *_name;
    void *_fh; // FILE*
    vfspos _size;
    void *_buf;
};

class VFSFileMem : public VFSFile
{
public:
    enum Mode
    {
        COPY, //- Make a copy of the buffer (default action).
        REUSE,  //- Use the passed-in buffer as is.  Requires the pointer
                //  to remain valid over the life of this object.
        TAKE_OVER, //- Take over the passed-in buffer; it will be deleted on object destruction.
    };
    typedef void (*delete_func)(void*);

    /* Creates a virtual file from a memory buffer. By default, the memory is copied.
       A deletor function can be passed optionally, if its NULL (the default),
       delete[] (char*)buf will be used. For malloc()'d memory, pass free. (Only used if mode is TAKE_OVER) */
    VFSFileMem(const char *name, void *buf, unsigned int size, Mode m = COPY, delete_func delfunc = NULL);
    virtual ~VFSFileMem();
    virtual bool open(const char *fn = NULL, const char *mode = NULL) { return true; }
    virtual bool isopen(void) const { return true; } // always open
    virtual bool iseof(void) const { VFS_GUARD_OPT(this); return _pos >= _size; }
    virtual const char *name(void) const { VFS_GUARD_OPT(this); return _name; }
    virtual const char *fullname(void) const { VFS_GUARD_OPT(this); return _fullname.c_str(); }
    virtual bool close(void) { return true; } // cant close, but not a problem
    virtual bool seek(vfspos pos) { VFS_GUARD_OPT(this); _pos = pos; return true; }
    virtual bool seekRel(vfspos offs) { VFS_GUARD_OPT(this); _pos += offs; return true; }
    virtual bool flush(void) { return false; } // can't flush, if a successful file write is expected, this IS a problem.
    virtual vfspos getpos(void) const { VFS_GUARD_OPT(this); return _pos; }
    virtual unsigned int read(void *dst, unsigned int bytes);
    virtual unsigned int write(const void *src, unsigned int bytes);
    virtual vfspos size(void) { VFS_GUARD_OPT(this); return _size; }
    virtual const void *getBuf(void) { VFS_GUARD_OPT(this); return _buf; }
    virtual void dropBuf(bool) {} // we can't simply drop the internal buffer, as the file is entirely memory based
    virtual const char *getSource(void) const { return "mem"; }

protected:
    void _setName(const char *n);
    std::string _fullname;
    const char *_name;
    vfspos _pos;
    vfspos _size;
    void *_buf;
    delete_func _delfunc;
    bool _mybuf;
};

VFS_NAMESPACE_END

#endif
