#include "VertexBuffer.h"
#include "RenderBase.h"
#include "Base.h"
#include <assert.h>
#include "Texture.h" // TexCoordBox

bool DynamicGPUBuffer::_HasARB = false;

static unsigned s_lastBuffer[GPUBUF_BINDING_MASK + 1]; // index via (usage & GPUBUF_BINDING_MASK)
static void *s_lastPtr = NULL;
static BufDataType s_lastDataType = BufDataType(-1);
static unsigned s_lastState = 0; // StateBits

enum StateBits
{
    SB_TC_FROM_BUFFER = 0x01,
    SB_COLOR_FROM_BUFFER = 0x02
};

static unsigned toGlUsage(unsigned usage)
{
    if(usage & GPUBUF_STATIC)
        return GL_STATIC_DRAW_ARB;
    if(usage & GPUBUF_STREAM)
        return GL_STREAM_DRAW_ARB;
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
    , _gl_binding((usage & GPUBUF_INDEXBUF) ? GL_ELEMENT_ARRAY_BUFFER_ARB : GL_ARRAY_BUFFER_ARB)
    , _size(0)
    , _h_cap(0)
    , _h_data(NULL)
    , _d_cap(0)
    , _d_map(NULL)
    , _gl_usage(toGlUsage(usage))
    , _usage(usage)
    , _datatype(BufDataType(-1))
{
}

DynamicGPUBuffer::~DynamicGPUBuffer()
{
    dropBuffer();
}

void* DynamicGPUBuffer::_allocBytes(size_t bytes)
{
    if(s_lastPtr == _h_data)
        s_lastPtr = NULL;

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
        const unsigned id = _ensureDBuf();
        unsigned& last = s_lastBuffer[_usage & GPUBUF_BINDING_MASK];
        if(id != last)
        {
            last = id;
            glBindBufferARB(_gl_binding, id);
        }
        if(!(access & GPUACCESS_HOSTCOPY))
        {
            if(_d_cap != newsize)
            {
                _d_cap = newsize;
                glBufferDataARB(_gl_binding, newsize, NULL, _gl_usage); // orphan buffer
            }
            void *p = glMapBufferARB(_gl_binding, GL_WRITE_ONLY_ARB);
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

bool DynamicGPUBuffer::commitWriteExact(const void * p)
{
    const void *origin = _d_map ? _d_map : _h_data;
    ptrdiff_t d = (const char*)p - (const char*)origin;
    assert(d == _size);
    return commitWrite();
}

bool DynamicGPUBuffer::_commitWrite(size_t used)
{
    if(_HasARB)
    {
        if(_d_map)
        {
            assert(used <= _d_cap);
            _d_map = NULL;
            return glUnmapBufferARB(_gl_binding); // can fail
        }
        // otherwise, the prev. call to glMapBufferARB failed (or GPUACCESS_HOSTCOPY was set).
        // -> didn't map, but wrote to host memory. upload it.
        assert(_h_data);
        assert(used <= _h_cap);
        _uploadFromHost(_h_data, used);
    }
    // else nothing to do

    assert(used <= _h_cap);
    return true;
}

void DynamicGPUBuffer::upload(BufDataType type, const void* data, size_t size)
{
    _datatype = type;
    _size = size;

    if(_HasARB)
    {
        const unsigned id = _Bind(_ensureDBuf(), _gl_binding, _usage);
        _uploadFromHost(data, size);
    }
    else
    {
        void *dst = _ensureBytes(size);
        if(data)
            memcpy(dst, data, size);
    }
}

unsigned DynamicGPUBuffer::_Bind(unsigned id, unsigned binding, unsigned usage)
{
    unsigned& last = s_lastBuffer[usage & GPUBUF_BINDING_MASK];
    if(id != last)
    {
        last = id;
        glBindBufferARB(binding, id);
    }
    return id;
}

// Assumes buffer is already bound
void DynamicGPUBuffer::_uploadFromHost(const void * data, size_t size)
{
    assert(_HasARB);
    if(size)
    {
        if(size <= _d_cap)
            glBufferSubDataARB(_gl_binding, 0, size, data); // update existing buffer
        else
        {
            _d_cap = size;
            glBufferDataARB(_gl_binding, size, data, _gl_usage); // alloc new buffer
        }
    }
}

void DynamicGPUBuffer::updatePartial(size_t offset, const void * data, size_t size)
{
    if(_HasARB)
    {
        assert(_bufid); // Must have been previously allocated
        assert(offset + size <= _d_cap);
        _Bind(_bufid, _gl_binding, _usage);
        glBufferSubDataARB(_gl_binding, offset, size, data);
    }

    if(_h_data)
    {
        assert(offset + size <= _h_cap);
        memcpy((char*)_h_data + offset, data, size);
    }
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
    assert(_datatype != BufDataType(-1)); // if this triggers, then the buffer was never filled with data
    if(!usetype)
        usetype = _datatype;

    const unsigned bufid = this->_bufid;

    void *p = bufid ? NULL : (void*)this->_h_data;

    assert(bufid || p);

    if(bufid != s_lastBuffer[GPUBUF_VERTEXBUF])
    {
        s_lastBuffer[GPUBUF_VERTEXBUF] = bufid;
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, bufid);
    }
    else if(p == s_lastPtr && usetype == s_lastDataType)
        return;

    // --- something is different compared to last time, setup pointers ---

    s_lastDataType = usetype;
    s_lastPtr = p;

    unsigned u = usetype; // always want unsigned shifts
    assert((u & 0xf) < Countof(s_gltype));
    const unsigned gltype = s_gltype[u & 0xf];
    const unsigned scalars = (u >> 4u) & 0xf;
    const unsigned stride = (u >> 8u) & 0xff;
    const unsigned tcoffset = (u >> 16u) & 0xff;
    const unsigned coloroffset = u >> 24u;

    // vertices are always enabled
    glVertexPointer(scalars, gltype, stride, p);

    unsigned wantedstate = 0;
    if(tcoffset)
    {
        wantedstate |= SB_TC_FROM_BUFFER;
        glTexCoordPointer(2, gltype, stride, (void*)((uintptr_t)p + tcoffset));
    }
    if(coloroffset)
    {
        wantedstate |= SB_COLOR_FROM_BUFFER;
        glColorPointer(4, gltype, stride, (void*)((uintptr_t)p + coloroffset));
    }

    unsigned wrongbits = wantedstate ^ s_lastState;
    if(wrongbits)
    {
        if(wrongbits & SB_TC_FROM_BUFFER)
        {
            if(wantedstate & SB_TC_FROM_BUFFER)
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            else
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
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
    if(s_lastPtr == _h_data)
        s_lastPtr = NULL;
    free(_h_data);
    _h_data = NULL;
    if(_bufid)
    {
        if(s_lastBuffer[GPUBUF_VERTEXBUF] == _bufid)
            s_lastBuffer[GPUBUF_VERTEXBUF] = 0;
        if(s_lastBuffer[GPUBUF_INDEXBUF] == _bufid)
            s_lastBuffer[GPUBUF_INDEXBUF] = 0;

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
    assert(_gl_binding == GL_ELEMENT_ARRAY_BUFFER_ARB);
    assert(s_gltype[_datatype & 0xf] == GL_SHORT);
    //assert(getBoundBuffer(GL_ARRAY_BUFFER_BINDING)); // FIXME: this assert is wrong if indices are on the host

    const unsigned id = _bufid;

    if(s_lastBuffer[GPUBUF_INDEXBUF] != id)
    {
        s_lastBuffer[GPUBUF_INDEXBUF] = id;
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, id);
    }

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
