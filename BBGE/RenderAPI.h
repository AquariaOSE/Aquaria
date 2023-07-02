#ifndef BBGE_RENDER_API_H
#define BBGE_RENDER_API_H

#include "EngineEnums.h"

enum RenderAPIConstants
{
    //-- primitives to draw
    RAPI_PRIM_POINTS = 0,
    RAPI_PRIM_LINES = 1,
    RAPI_PRIM_LINE_STRIP = 2,
    RAPI_PRIM_QUADS = 3,
    RAPI_PRIM_QUAD_STRIP = 4,

    //-- per-vertex layout
    RAPI_LAYOUT_2D = 0,           // (u,v,x,y)          aka Vertex2D
    RAPI_LAYOUT_2D_COLOR = 1      // (u,v,x,y,r,g,b,a)  aka Vertex2DColor
};

namespace RenderAPI {

struct Vertex2D
{
    float u, v, x, y;
};
struct Vertex2DColor : public Vertex2D
{
    float  r, g, b, a;
};


class BufferBase
{
public:
    enum Hint
    {
        BUFFER_STATIC,
        BUFFER_DYNAMIC
    };

    unsigned bufid() const { return _bufid; }
    const void *data() const { return _data; }
    size_t bytes() const { return _bytes; }
    void destroy();
    void upload(Hint usage);

protected:

    unsigned _bufid;
    void *_data; // pointer to data
    size_t _bytes;
};

// real POD struct
struct ObjectData
{
    const float *pmat; // pointer to float[16]
    struct
    {
        float r, g, b, a; // used only when no per-vertex colors are used
    } color;
    const BufferBase *verts;
    const BufferBase *indices; // always uint16 if present
    union
    {
        unsigned texid;  // if tris/quads
        float linewidth; // if lines/points: point size or line width
    } u;
    char blend;
    unsigned char prim;
    unsigned char layout;
};


void render(const ObjectData *objs, size_t n);
void updateBuffer(unsigned *pbufid, const void *data, size_t bytes, BufferBase::Hint usage);
void deleteBuffer(unsigned *pbufid);

}

#endif // BBGE_RENDER_API_H
