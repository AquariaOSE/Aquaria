#include "VertexBuffer.h"
#include "RenderBase.h"
#include "Base.h"
#include <assert.h>

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
    , _cap(0)
    , _h_data(NULL)
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
        _cap = bytes;
        _h_data = p;
    }
    return p;
}

void* DynamicGPUBuffer::_ensureBytes(size_t bytes)
{
    if(bytes < _cap)
        return _h_data;

    size_t newsize = 2 * _size;
    if(newsize < bytes)
        newsize += bytes;

    return _allocBytes(newsize);
}

void* DynamicGPUBuffer::beginWrite(BufDataType type, size_t newsize, unsigned access)
{
    _size = newsize;
    _datatype = type;

    if(_HasARB)
    {
        glBindBufferARB(_binding, _ensureDBuf());
        glBufferDataARB(_binding, newsize, NULL, _usage); // orphan buffer
        if(!(access & GPUACCESS_HOSTCOPY))
        {
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
    if(_HasARB)
    {
        if(_d_map)
        {
            _d_map = NULL;
            return glUnmapBufferARB(_binding); // can fail
        }
        // otherwise, the prev. call to glMapBufferARB failed (or GPUACCESS_NOMAP was set).
        // -> didn't map, but wrote to host memory. upload it.
        assert(_h_data);
        glBufferSubDataARB(_binding, 0, _size, _h_data);
    }
    // else nothing to do

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

    void *p;
    if(_HasARB)
    {
        unsigned bufid = this->_bufid;
        assert(bufid != s_lastVertexBuffer); // check that it's no redundant state change
        if(bufid == s_lastVertexBuffer && usetype == s_lastDataType)
            return;
        p = NULL;
        s_lastVertexBuffer = bufid;
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, bufid);
    }
    else
    {
        p = (void*)this->_h_data;
        assert(p != s_lastHostPtr); // check that it's no redundant state change
        if(p == s_lastHostPtr && usetype == s_lastDataType) // don't need to check for datatype since that's const for the buffer with that ptr
            return;
    }

    s_lastDataType = usetype;

    assert((ty & 0xf) < Countof(s_gltype));
    const unsigned gltype = s_gltype[usetype & 0xf];
    const unsigned scalars = (usetype >> 4) & 0xf;
    const unsigned stride = (usetype >> 8) & 0xff;
    const unsigned tcoffset = (usetype >> 16) & 0xff;
    const unsigned coloroffset = usetype >> 24;

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

void DynamicGPUBuffer::DrawArrays(unsigned glmode, size_t n, size_t first)
{
    glDrawArrays(glmode, first, n);
}

void DynamicGPUBuffer::drawElements(unsigned glmode, size_t n, size_t first)
{
    assert(_binding == GL_ELEMENT_ARRAY_BUFFER_ARB);
    assert(s_gltype[_datatype & 0xf] == GL_SHORT);

    if(s_lastIndexBuffer != _bufid)
    {
        s_lastIndexBuffer = _bufid;
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, _bufid);
    }

    glDrawElements(glmode, n, GL_SHORT, NULL);
}

void DynamicGPUBuffer::initQuadVertices(float tu1, float tv1, float tu2, float tv2)
{
    do
    {
        float *p = (float*)beginWrite(GPUBUFTYPE_VEC2_TC, (4*4) * sizeof(float), GPUACCESS_DEFAULT);
        *p++ = -0.5f; *p++ = +0.5f;     // xy
	    *p++ = tu1;   *p++ = 1.0f-tv1;  //   uv
	    *p++ = +0.5f; *p++ = +0.5f;     // xy
	    *p++ = tu2;   *p++ = 1.0f-tv1;  //   uv
	    *p++ = +0.5f; *p++ = -0.5f;     // xy
	    *p++ = tu2;   *p++ = 1.0f-tv2;  //   uv
	    *p++ = -0.5f; *p++ = -0.5f;     // xy
	    *p++ = tu1;   *p++ = 1.0f-tv2;   //   uv
    }
    while(!commitWrite());
}
