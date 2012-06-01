// VFSFile.h - basic file interface + classes
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFSFILE_H
#define VFSFILE_H

#include "VFSBase.h"
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
class VFSFile : public VFSBase
{
public:

    /** The ctor is expected to set both name() and fullname();
        The name must remain static throughout the object's lifetime. */
    VFSFile(const char *fn);

    virtual ~VFSFile();

    /** Open a file.
        Mode can be "r", "w", "rb", "rb", and possibly other things that fopen supports.
        It is the subclass's choice to support other modes. Default is "rb".
        If the file still has a buffer (as returned by getBuf()), it is deleted.
        Closes and reopens if already open (even in the same mode). */
    virtual bool open(const char *mode = NULL) { return false; }

    virtual bool isopen(void) const { return false; }
    virtual bool iseof(void) const { return true; }
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

    /** Return full file content in memory. Like size(), this may do other operations on the file,
        but after the function returns the file is expected to be in the same state it was before.
        If the file is not open before the call, it will be opened with default parameters (that is, "rb").
        Additional EOL mangling my happen if the file is opened in text mode before (= not binary).
        Calls to open() should delete this memory if the file was previously opened in a different mode.
        In the default implementation, the returned memory is not guaranteed to be writable without problems,
        so don't do it. Don't cast the const away. You have been warned.
        Supply your own allocator and deletor functions if required.
        NULL means new[] and delete[], respectively.
        Either both must be a valid function, or both NULL. Only one of them NULL will cause assertion fail. */
    virtual const void *getBuf(allocator_func alloc = NULL, delete_func del = NULL);

    /** If del is true, delete internal buffer. If false, unregister internal buffer from the file,
        but do not delete. Use free() or an appropriate deletion function later. */
    virtual void dropBuf(bool del);

    /** Basic RTTI, for debugging purposes */
    virtual const char *getType(void) const { return "virtual"; }


    // ---- non-virtual part ----

    /** Uses the deletion function earlier given to getBuf() to free the given memory,
        or delete [] if the function is NULL. Useful if the original function
        that was used to allocate the buffer is no longer known. */
    void delBuf(void *mem);


protected:

    void *_buf;
    delete_func _delfunc;
};

class VFSFileReal : public VFSFile
{
public:
    VFSFileReal(const char *name);
    virtual ~VFSFileReal();
    virtual bool open(const char *mode = NULL);
    virtual bool isopen(void) const;
    virtual bool iseof(void) const;
    virtual bool close(void);
    virtual bool seek(vfspos pos);
    virtual bool seekRel(vfspos offs);
    virtual bool flush(void);
    virtual vfspos getpos(void) const;
    virtual unsigned int read(void *dst, unsigned int bytes);
    virtual unsigned int write(const void *src, unsigned int bytes);
    virtual vfspos size(void);
    virtual const char *getType(void) const { return "disk"; }

    inline void *getFP() { return _fh; }

protected:
    
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

    /* Creates a virtual file from a memory buffer. By default, the memory is copied.
       A deletor function can be passed optionally, if its NULL (the default),
       delete[] (char*)buf will be used. For malloc()'d memory, pass free. (Only used if mode is TAKE_OVER) */
    VFSFileMem(const char *name, void *buf, unsigned int size, Mode m = COPY,
        allocator_func alloc = NULL, delete_func delfunc = NULL);
    virtual ~VFSFileMem();
    virtual bool open(const char *mode = NULL) { return true; }
    virtual bool isopen(void) const { return true; } // always open
    virtual bool iseof(void) const { VFS_GUARD_OPT(this); return _pos >= _size; }
    virtual bool close(void) { return true; } // cant close, but not a problem
    virtual bool seek(vfspos pos) { VFS_GUARD_OPT(this); _pos = pos; return true; }
    virtual bool seekRel(vfspos offs) { VFS_GUARD_OPT(this); _pos += offs; return true; }
    virtual bool flush(void) { return false; } // can't flush, if a successful file write is expected, this IS a problem.
    virtual vfspos getpos(void) const { VFS_GUARD_OPT(this); return _pos; }
    virtual unsigned int read(void *dst, unsigned int bytes);
    virtual unsigned int write(const void *src, unsigned int bytes);
    virtual vfspos size(void) { VFS_GUARD_OPT(this); return _size; }
    virtual const void *getBuf(allocator_func alloc = NULL, delete_func del = NULL) { VFS_GUARD_OPT(this); return _buf; }
    virtual void dropBuf(bool) {} // we can't simply drop the internal buffer, as the file is entirely memory based
    virtual const char *getType(void) const { return "mem"; }

protected:

    vfspos _pos;
    vfspos _size;
    bool _mybuf;
};

VFS_NAMESPACE_END

#endif
