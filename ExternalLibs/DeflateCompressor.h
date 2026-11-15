#ifndef DEFLATE_COMPRESSOR_H
#define DEFLATE_COMPRESSOR_H

#include "ByteBuffer.h"

// implements a raw deflate stream, not zlib wrapped, and not checksummed.
class DeflateCompressor : public ByteBuffer
{
public:
    DeflateCompressor();
    virtual ~DeflateCompressor() {}
    virtual void Compress(uint8 level = 1);
    virtual void Decompress(void);

    bool Compressed(void) const { return _iscompressed; }
    void Compressed(bool b) { _iscompressed = b; }
    void SetForceCompression(bool f) { _forceCompress = f; }
    size_t RealSize(void) const { return _iscompressed ? _real_size : size(); }
    void RealSize(size_t realsize) { _real_size = realsize; }
    void clear(void) // not required to be strictly virtual; be careful not to mess up static types!
    {
        ByteBuffer::clear();
        _real_size = 0;
        _iscompressed = false;
    }

protected:
    int _windowBits; // read zlib docs to know what this means
    size_t _real_size;
    bool _forceCompress;
    bool _iscompressed;

private:
    static void decompress(void *dst, size_t *origsize, const void *src, size_t size, int wbits);
    static void compress(void* dst, size_t *dst_size, const void* src, size_t src_size,
        uint8 level, int wbits);

    int decompressBlockwise();
};

// implements deflate stream, zlib wrapped
class ZlibCompressor : public DeflateCompressor
{
public:
    ZlibCompressor();
    virtual ~ZlibCompressor() {}
};

// the output produced by this stream contains a minimal gzip header,
// and can be directly written to a .gz file.
class GzipCompressor : public DeflateCompressor
{
public:
    GzipCompressor();
    virtual ~GzipCompressor() {}
    virtual void Decompress(void);
};

#endif
