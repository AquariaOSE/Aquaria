#ifndef VFS_FILE_STREAM_H
#define VFS_FILE_STREAM_H

#include "SimpleIStringStream.h"

class VFSTextStreamIn : public SimpleIStringStream
{
    /* This class is an adapter to support STL-like read-only file streams for VFS files,
     * using the SimpleIStringStream for performance reasons.
     * 
     * strmode: one of COPY, REUSE, TAKE_OVER, see SimpleIStringStream.h
     */

public:
    VFSTextStreamIn(const char *fn, SimpleIStringStream::Mode strmode = TAKE_OVER);
    VFSTextStreamIn(const std::string& fn, SimpleIStringStream::Mode strmode = TAKE_OVER);
    void close() {}

private:
    void _init(const char *fn, SimpleIStringStream::Mode strmode);
};

class VFSTextStdStreamIn : public std::istringstream
{
    /* This class is an adapter to support STL-like read-only file streams for VFS files,
    * using std::istringstream.
    * 
    * strmode: one of COPY, REUSE, TAKE_OVER, see SimpleIStringStream.h
    * - Note: The file's content will always be copied, regardless of strmode setting.
    *         However, TAKE_OVER will drop the internal buffer.
    */

public:
    VFSTextStdStreamIn(const char *fn, SimpleIStringStream::Mode strmode = SimpleIStringStream::TAKE_OVER);
    VFSTextStdStreamIn(const std::string& fn, SimpleIStringStream::Mode strmode = SimpleIStringStream::TAKE_OVER);
    void close() {}

private:
    void _init(const char *fn, SimpleIStringStream::Mode strmode);
};


#endif
