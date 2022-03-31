
#include <miniz.h>

#include "DeflateCompressor.h"

// for weird gcc/mingw hackfix below
#include <string.h>


DeflateCompressor::DeflateCompressor()
:   _windowBits(-MAX_WBITS), // negative, because we want a raw deflate stream, and not zlib-wrapped
    _real_size(0),
    _forceCompress(false),
    _iscompressed(false)
{
}

ZlibCompressor::ZlibCompressor()
: DeflateCompressor()
{
    _windowBits = MAX_WBITS; // positive, means we use a zlib-wrapped deflate stream
}

GzipCompressor::GzipCompressor()
: DeflateCompressor()
{
    _windowBits = MAX_WBITS + 16; // this makes zlib wrap a minimal gzip header around the stream
    _forceCompress = true; // we want this for gzip
}

void DeflateCompressor::compress(void* dst, uint32 *dst_size, const void* src, uint32 src_size,
                                 uint8 level, int wbits)
{
    z_stream c_stream;

    c_stream.zalloc = (alloc_func)Z_NULL;
    c_stream.zfree = (free_func)Z_NULL;
    c_stream.opaque = (voidpf)Z_NULL;

    if (Z_OK != deflateInit2(&c_stream, level, Z_DEFLATED, wbits, 8, Z_DEFAULT_STRATEGY))
    {
        *dst_size = 0;
        return;
    }

    c_stream.next_out = (Bytef*)dst;
    c_stream.avail_out = *dst_size;
    c_stream.next_in = (Bytef*)src;
    c_stream.avail_in = (uInt)src_size;

    int ret = deflate(&c_stream, Z_FINISH);

    switch(ret)
    {
        case Z_STREAM_END:
            break; // all good
        case Z_OK:
            *dst_size = 0;
            return;
        default:
            *dst_size = 0;
            return;
    }

    if (Z_OK != deflateEnd(&c_stream))
    {
        *dst_size = 0;
        return;
    }

    *dst_size = c_stream.total_out;
}

void DeflateCompressor::decompress(void *dst, uint32 *origsize, const void *src, uint32 size, int wbits)
{
    z_stream stream;
    int err;

    stream.next_in = (Bytef*)src;
    stream.avail_in = (uInt)size;
    stream.next_out = (Bytef*)dst;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_out = *origsize;
    stream.total_out = 0;

    err = inflateInit2(&stream, wbits);
    if (err != Z_OK)
    {
        *origsize = 0;
        return;
    }

    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END)
    {
        inflateEnd(&stream);
        *origsize = 0;
        return;
    }
    *origsize = (uint32)stream.total_out;

    err = inflateEnd(&stream);
    if(err != Z_OK)
        *origsize = 0;
}


void DeflateCompressor::Compress(uint8 level)
{
    if(!_forceCompress && (!level || _iscompressed || (!size())))
        return;

    char *buf;


    uint32 oldsize = size();
    uint32 newsize = compressBound(oldsize) + 30; // for optional gzip header

    buf = new char[newsize];

    compress((void*)buf, &newsize, (void*)contents(), oldsize, level, _windowBits);
    if(!newsize || (!_forceCompress && newsize > oldsize)) // only allow more data if compression is forced (which is the case for gzip)
    {
        delete [] buf;
        return;
    }

    resize(newsize);
    rpos(0);
    wpos(0);
    append(buf,newsize);
    delete [] buf;

    _iscompressed = true;

    _real_size = oldsize;
}

void DeflateCompressor::Decompress(void)
{
    if( (!_iscompressed) || (!size()))
        return;

    if(!_real_size)
    {
        if(decompressBlockwise() == Z_OK)
            _iscompressed = false;
    }
    else
    {
        uint32 rs = (uint32)_real_size;
        uint32 origsize = rs;
        uint8 *target = new uint8[rs];
        wpos(0);
        rpos(0);
        decompress((void*)target, &origsize, (const void*)contents(), size(), _windowBits);
        if(origsize != rs)
        {
            delete [] target;
            return;
        }
        clear();
        append(target, origsize);
        delete [] target;
        _real_size = 0;
        _iscompressed = false;
    }

}

#define CHUNK 16384

int DeflateCompressor::decompressBlockwise()
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, _windowBits);
    if (ret != Z_OK)
        return ret;

    ByteBuffer bb;

    strm.avail_in = size();
    strm.next_in = contents();

    /* decompress until deflate stream ends or end of file */
    do {
        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
            case Z_NEED_DICT:
            case Z_STREAM_ERROR:
            case Z_BUF_ERROR:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            bb.append(out, have);
        } while (strm.avail_out == 0);
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);

    if (ret != Z_STREAM_END)
        return Z_DATA_ERROR;

    // exchange pointer
    clear();
    init(bb, TAKE_OVER);

    return Z_OK;
}

void GzipCompressor::Decompress(void)
{
    uint32 t = 0;
    rpos(size() - sizeof(uint32)); // according to RFC 1952, input size are the last 4 bytes at the end of the file, in little endian
    *this >> t;
    _real_size = t;

    // !! NOTE: this fixes a gcc/mingw bug where _real_size would be set incorrectly
#if __GNUC__
    char xx[20];
    sprintf(xx, "%u", t);
#endif

    DeflateCompressor::Decompress(); // will set rpos back anyway
}
