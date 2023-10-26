#include "VertexBuffer.h"
#include "RenderBase.h"
#include "Base.h"
#include <assert.h>
#include "Texture.h" // TexCoordBox

bool DynamicGPUBuffer::_HasARB = false;

static unsigned s_lastVertexBuffer = 0;
static unsigned s_lastIndexBuffer = 0;
static void *s_lastHostPtr = NULL;
static BufDataType s_lastDataType = BufDataType(-1);
static unsigned s_lastState = 0; // StateBits

enum StateBits
{
    SB_COLOR_FROM_BUFFER = 0x01
};

static unsigned toGlUsage(unsigned usage)
{
    if(usage & GPUBUF_STATIC)
        return GL_STATIC_DRAW_ARB;
    return GL_DYNAMIC_DRAW;
}

void DynamicGPUBuffer::StaticInit()
{
    _HasARB = glGenBuffersARB && glDeleteBuffersARB
        && glBufferDataARB && glBufferSubDataARB
        && glBindBufferARB && glMapBufferARB && glUnmapBufferARB;
}

DynamicGPUBuffer::DynamicGPUBuffer(unsigned usage)
    : _bufid(0)
    , _binding((usage & GPUBUF_INDEXBUF) ? GL_ELEMENT_ARRAY_BUFFER_ARB : GL_ARRAY_BUFFER_ARB)
    , _size(0)
    , _h_cap(0)
    , _h_data(NULL)
    , _d_cap(0)
    , _d_map(NULL)
    , _usage(toGlUsage(usage))
    , _datatype(BufDataType(-1))
{
}

DynamicGPUBuffer::~DynamicGPUBuffer()
{
    dropBuffer();
}

void* DynamicGPUBuffer::_allocBytes(size_t bytes)
{
    if(s_lastHostPtr == _h_data)
        s_lastHostPtr = NULL;

    void *p = realloc(_h_data, bytes);
    if(p)
    {
        _h_cap = bytes;
        _h_data = p;
    }
    return p;
}

void* DynamicGPUBuffer::_ensureBytes(size_t bytes)
{
    if(bytes < _h_cap)
        return _h_data;

    size_t newsize = 2 * _h_cap;
    if(newsize < bytes)
        newsize += bytes;

    return _allocBytes(newsize);
}

void* DynamicGPUBuffer::beginWrite(BufDataType type, size_t newsize, unsigned access)
{
    assert(!_d_map);
    _size = newsize;
    _datatype = type;

    if(_HasARB)
    {
        glBindBufferARB(_binding, _ensureDBuf());
        if(!(access & GPUACCESS_HOSTCOPY))
        {
            _d_cap = newsize;
            glBufferDataARB(_binding, newsize, NULL, _usage); // orphan buffer
            void *p = glMapBufferARB(_binding, GL_WRITE_ONLY_ARB);
            _d_map = p;
            if(p)
                return p;
        }
    }

    return _ensureBytes(newsize);
}

bool DynamicGPUBuffer::commitWrite()
{
    return _commitWrite(_size);
}

bool DynamicGPUBuffer::commitWrite(size_t used)
{
    _size = used;
    return _commitWrite(used);
}

bool DynamicGPUBuffer::_commitWrite(size_t used)
{
    if(_HasARB)
    {
        if(_d_map)
        {
            assert(used <= _d_cap);
            bool ok = glUnmapBufferARB(_binding); // can fail
            if(ok)
                _d_map = NULL;
            return ok;
        }
        // otherwise, the prev. call to glMapBufferARB failed (or GPUACCESS_HOSTCOPY was set).
        // -> didn't map, but wrote to host memory. upload it.
        assert(_h_data);
        assert(used <= _h_cap);
        if(used <= _d_cap)
            glBufferSubDataARB(_binding, 0, used, _h_data); // update existing buffer
        else
        {
            _d_cap = used;
            glBufferDataARB(_binding, used, _h_data, _usage); // alloc new buffer
        }
    }
    // else nothing to do

    assert(used <= _h_cap);
    return true;
}

void DynamicGPUBuffer::upload(BufDataType type, const void* data, size_t size)
{
    _datatype = type;

    if(_HasARB)
    {
        glBindBufferARB(_binding, _ensureDBuf());
        glBufferDataARB(_binding, size, data, _usage);
    }
    else
        memcpy(_ensureBytes(size), data, size);
}

static const unsigned s_gltype[] =
{
    GL_SHORT,
    GL_FLOAT,
};

struct BufPtrConfig
{
    uintptr_t bufidOrPtr;
    BufDataType datatype;

};

void DynamicGPUBuffer::apply(BufDataType usetype) const
{
    if(!usetype)
        usetype = _datatype;

    const unsigned bufid = this->_bufid;

    void *p;
    if(bufid)
    {
        p = NULL;
        //if(bufid != s_lastVertexBuffer)
        //    glBindBufferARB(GL_ARRAY_BUFFER_ARB, bufid);
    }
    else
    {
        p = (void*)this->_h_data;
        //assert(p != s_lastHostPtr); // check that it's no redundant state change
        //if(p == s_lastHostPtr && usetype == s_lastDataType) // don't need to check for datatype since that's const for the buffer with that ptr
        //    return;
    }

    assert(bufid || p);

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, bufid);

    //if(bufid == s_lastVertexBuffer && usetype == s_lastDataType && p == s_lastHostPtr)
    //    return;

    s_lastDataType = usetype;
    s_lastVertexBuffer = bufid;

    unsigned u = usetype; // always want unsigned shifts
    assert((u & 0xf) < Countof(s_gltype));
    const unsigned gltype = s_gltype[u & 0xf];
    const unsigned scalars = (u >> 4u) & 0xf;
    const unsigned stride = (u >> 8u) & 0xff;
    const unsigned tcoffset = (u >> 16u) & 0xff;
    const unsigned coloroffset = u >> 24u;

    // vertex and texcoords are always enabled
    glVertexPointer(scalars, gltype, stride, p);
    if(tcoffset)
        glTexCoordPointer(2, gltype, stride, (void*)((uintptr_t)p + tcoffset));

    unsigned wantedstate = 0;
    if(coloroffset)
    {
        wantedstate |= SB_COLOR_FROM_BUFFER;
        glColorPointer(4, gltype, stride, (void*)((uintptr_t)p + coloroffset));
    }

    unsigned wrongbits = wantedstate ^ s_lastState;
    if(wrongbits)
    {
        if(wrongbits & SB_COLOR_FROM_BUFFER)
        {
            if(wantedstate & SB_COLOR_FROM_BUFFER)
                glEnableClientState(GL_COLOR_ARRAY);
            else
                glDisableClientState(GL_COLOR_ARRAY);
        }
        s_lastState = wantedstate;
    }
}

unsigned DynamicGPUBuffer::_ensureDBuf()
{
    assert(_HasARB);
    if(!_bufid)
        glGenBuffersARB(1, &_bufid);
    return _bufid;
}

void DynamicGPUBuffer::dropBuffer()
{
    if(s_lastHostPtr == _h_data)
        s_lastHostPtr = NULL;
    free(_h_data);
    _h_data = NULL;
    if(_bufid)
    {
        if(s_lastVertexBuffer == _bufid)
            s_lastVertexBuffer = 0;
        if(s_lastIndexBuffer == _bufid)
            s_lastIndexBuffer = 0;

        glDeleteBuffersARB(1, &_bufid);
        _bufid = 0;
    }
    _size = 0;
}

/*static unsigned getBoundBuffer(unsigned target)
{
    int id = 0;
    glGetIntegerv(target, &id);
    return id;
}*/

void DynamicGPUBuffer::drawElements(unsigned glmode, size_t n, size_t first) const
{
    assert(_binding == GL_ELEMENT_ARRAY_BUFFER_ARB);
    assert(s_gltype[_datatype & 0xf] == GL_SHORT);
    //assert(getBoundBuffer(GL_ARRAY_BUFFER_BINDING)); // FIXME: this assert is wrong if indices are on the host

    unsigned id = _bufid;

    //if(s_lastIndexBuffer != id)
    //{
    //    s_lastIndexBuffer = id;
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, id);
    //}

    const unsigned short *p = (unsigned short*)(id ? NULL : _h_data);
    assert(p || id);

    glDrawElements(glmode, n, GL_UNSIGNED_SHORT, p + first);
}

void DynamicGPUBuffer::initQuadVertices(const TexCoordBox& tc, unsigned access)
{
    do
    {
        float *p = (float*)beginWrite(GPUBUFTYPE_VEC2_TC, (4*4 + 4) * sizeof(float), access);
        *p++ = -0.5f; *p++ = +0.5f;     // xy
	    *p++ = tc.u1;   *p++ = tc.v1;  //   uv
	    *p++ = +0.5f; *p++ = +0.5f;     // xy
	    *p++ = tc.u2;   *p++ = tc.v1;  //   uv
	    *p++ = +0.5f; *p++ = -0.5f;     // xy
	    *p++ = tc.u2;   *p++ = tc.v2;  //   uv
	    *p++ = -0.5f; *p++ = -0.5f;     // xy
	    *p++ = tc.u1;   *p++ = tc.v2;  //   uv

        for(size_t i = 0; i < 4; ++i)
            *p++ = 0; // zero/center xy uv (uv isn't used)
    }
    while(!commitWrite());
}

// 0---1---2---3
// |   |   |   |
// 4---5---6---7
// |   |   |   |
// 8---9---10--11
// This is a 4x3 grid
// Which is 3*2 = 6 quads
// That's 12 triangles
// Each triangle is 3 indices, so we get 36 indices in total
size_t DynamicGPUBuffer::initGridIndices_Triangles(size_t w, size_t h, bool invert, unsigned access)
{
    assert(w * h < 0xffff);

    const size_t quadsx = w - 1;
    const size_t quadsy = h - 1;
    const size_t quads = quadsx * quadsy;
    assert(quads);
    const size_t border = 4; // for GL_LINE_LOOP
    do
    {
        unsigned short *p = (unsigned short*)beginWrite(GPUBUFTYPE_U16, (6*quads + border) * sizeof(short), access);

        if(!invert)
        {
            // top to bottom
            for(size_t y = 0; y < quadsy; ++y)
            {
                for(size_t x = 0, i = y * w; x < quadsx; ++x, ++i)
                {
                    *p++ = (unsigned short)(i);         // 0
                    *p++ = (unsigned short)(i + 1);     // 1
                    *p++ = (unsigned short)(i + w);     // 4

                    *p++ = (unsigned short)(i + 1);     // 1
                    *p++ = (unsigned short)(i + w + 1); // 5
                    *p++ = (unsigned short)(i + w);     // 4
                }
            }
        }
        else
        {
            // bottom to top
            for(size_t y = quadsy; y --> 0; )
            {
                for(size_t x = 0, i = y * w; x < quadsx; ++x, ++i)
                {
                    *p++ = (unsigned short)(i);         // 0
                    *p++ = (unsigned short)(i + 1);     // 1
                    *p++ = (unsigned short)(i + w);     // 4

                    *p++ = (unsigned short)(i + 1);     // 1
                    *p++ = (unsigned short)(i + w + 1); // 5
                    *p++ = (unsigned short)(i + w);     // 4
                }
            }
        }
    }
    while(!commitWrite());
    return quads * 6; // each quad is 2 triangles x 3 verts
}
