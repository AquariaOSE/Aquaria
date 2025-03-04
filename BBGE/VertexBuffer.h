#ifndef BBGE_VERTEXBUFFER_H
#define BBGE_VERTEXBUFFER_H

#include <stdlib.h> // size_t

struct TexCoordBox;

enum BufUsage
{
    // binding point
    GPUBUF_VERTEXBUF  = 0x00,
    GPUBUF_INDEXBUF   = 0x01,
    GPUBUF_BINDING_MASK = 0x01,
    // usage
    GPUBUF_DYNAMIC    = 0x00,
    GPUBUF_STATIC     = 0x10
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
    GPUBUFTYPE_VEC2_TC_RGBA   = 0x10082021,  // xyuvrgba xyuvrgba xyuvrgba
    GPUBUFTYPE_VEC2_RGBA      = 0x08001821,  // xyrgba xyrgba xyrgba
    //                            ccoossnt
    GPUBUFTYPE_UVEC2          = 0x00000420
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
public:
    static void StaticInit();
    DynamicGPUBuffer(unsigned usage);
    ~DynamicGPUBuffer();
    void dropBuffer();

    size_t size() const { return _size; }

    // beginWrite(), then write exactly newsize bytes, then commit
    void *beginWrite(BufDataType type, size_t newsize, unsigned access); // AccessFlags
    bool commitWrite(); // used same size as passed to beginWrite()
    bool commitWrite(size_t used); // explicitly specify used size (may be less than initially requested)
    bool commitWriteExact(const void *p); // asserts that as many bytes as allocated were written

    void upload(BufDataType type, const void *data, size_t size);

    // uses own data for indexing and prev. applied buffer for the data to draw
    void drawElements(unsigned glmode, size_t n, size_t first = 0) const;


    void apply(BufDataType usetype = GPUBUFTYPE_NONE) const;

    // Inteded for use with DrawArrays(4) and GL_TRIANGLE_FAN or GL_QUADS (both work)
    void initQuadVertices(const TexCoordBox& tc, unsigned access);

    // Init indices for drawing a grid, like this 4x3 grid:
    // 0---1---2---3
    // |   |   |   |
    // 4---5---6---7
    // |   |   |   |
    // 8---9---10--11
    // Returns the number of triangles to use with GL_TRIANGLES.
    // Pass invert==true to draw from bottom to top.
    size_t initGridIndices_Triangles(size_t w, size_t h, bool invert, unsigned access);


    // For debugging only
    inline unsigned _glBufferId() const { return _bufid; }

private:

    void* _allocBytes(size_t bytes);
    void* _ensureBytes(size_t bytes);
    unsigned _ensureDBuf();
    bool _commitWrite(size_t used);

    unsigned _bufid;
    unsigned _gl_binding;
    size_t _size;
    size_t _h_cap;
    void *_h_data;
    size_t _d_cap;
    void *_d_map;
    const unsigned _gl_usage;
    const unsigned _usage;
    BufDataType _datatype;

    static bool _HasARB;

    DynamicGPUBuffer(const DynamicGPUBuffer&); // no copy
    DynamicGPUBuffer& operator=(const DynamicGPUBuffer&); // no assign
};


#endif // BBGE_VERTEXBUFFER_H
