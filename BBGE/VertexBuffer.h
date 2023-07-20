#ifndef BBGE_VERTEXBUFFER_H
#define BBGE_VERTEXBUFFER_H

#include <stdlib.h> // size_t

enum BufUsage
{
    // usage
    GPUBUF_DYNAMIC    = 0x00,
    GPUBUF_STATIC     = 0x01,
    // binding point
    GPUBUF_VERTEXBUF  = 0x00,
    GPUBUF_INDEXBUF   = 0x02
};

enum BufDataType
{
    GPUBUFTYPE_NONE = 0,
/*                                ccoossnt --
                                         ^-- type of each scalar
                                        ^-- number of coordinate scalars (eg. xyz would be 3)
                                      ^^-- stride (distance to next vertex), in bytes
                                    ^^-- offset of texcoords, in bytes, if present
                                  ^^-- offset of colors, if present */
    GPUBUFTYPE_U16            = 0x00000010,  // densely packed u16, for indexing
    GPUBUFTYPE_VEC2_TC        = 0x00081021,  // xyuv xyuv xyuv
    GPUBUFTYPE_VEC2_TC_RGBA   = 0x10082021,   // xyuvrgba xyuvrgba xyuvrgba
    //                            ccoossnt
    GPUBUFTYPE_VEC2_TC_RGBA_BUT_NO_COLOR = GPUBUFTYPE_VEC2_TC_RGBA & 0xffffff
};

enum AccessFlags
{
    // Use whatever works. May directly map in GPU memory so that there is no
    // copy on the host; or if there is a previous copy, it may remain untouched.
    GPUACCESS_DEFAULT    = 0x00,

    // Don't use memory mapping. Prepare buffer on the host and upload it in one go.
    // The copy remains on the host.
    GPUACCESS_HOSTCOPY   = 0x01
};


class DynamicGPUBuffer
{
    friend class BufMapW;
public:
    void StaticInit();
    DynamicGPUBuffer(unsigned usage);
    ~DynamicGPUBuffer();
    void dropBuffer();

    size_t size() const { return _size; }

    // beginWrite(), then write exactly newsize bytes, then commit
    void *beginWrite(BufDataType type, size_t newsize, unsigned access); // AccessFlags
    bool commitWrite();

    void upload(BufDataType type, const void *data, size_t size);

    static void DrawArrays(unsigned glmode, size_t n, size_t first = 0); // uses last applied buffer for drawing

    // uses own data for indexing and prev. applied buffer for the data to draw
    void drawElements(unsigned glmode, size_t n, size_t first = 0);


    void apply(BufDataType usetype = GPUBUFTYPE_NONE) const;

    // Inteded for use with DrawArrays(4) and GL_TRIANGLE_FAN or GL_QUADS (both work)
    void initQuadVertices(float tu1, float tu2, float tv1, float tv2);

private:

    void* _allocBytes(size_t bytes);
    void* _ensureBytes(size_t bytes);
    unsigned _ensureDBuf();

    unsigned _bufid;
    unsigned _binding;
    size_t _size;
    size_t _cap;
    void *_h_data;
    void *_d_map;
    const unsigned _usage;
    BufDataType _datatype;

    static bool _HasARB;
};


#endif // BBGE_VERTEXBUFFER_H