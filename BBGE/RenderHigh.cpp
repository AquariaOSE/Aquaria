#include "RenderHigh.h"

#include <stdlib.h>
#include <assert.h>

ObjectList::ObjectList(size_t minsize)
    : _ptr(NULL), _size(0), _cap(0), _minsize(minsize)
{
}

ObjectList::~ObjectList()
{
    this->clear();
}

RenderAPI::ObjectData* ObjectList::reserve(size_t n)
{
    return _ensure(n) + _size;
}

void ObjectList::commit(size_t n)
{
    _size += n;
    assert(_size <= _cap);
}

RenderAPI::ObjectData& ObjectList::push(const RenderAPI::ObjectData& a)
{
    size_t cursize = _size;
    size_t newsize = _size + 1;
    _size = newsize;
    return (_ensure(newsize)[cursize] = a);
}

void ObjectList::reset()
{
    _size = 0;
}

void ObjectList::clear()
{
    free(_ptr);
    _ptr = NULL;
    _size = 0;
    _cap = 0;
}

RenderAPI::ObjectData* ObjectList::_ensure(size_t n)
{
    if(n < _cap)
        return _ptr;

    size_t newsize = 2 * _cap;
    if(newsize < n)
        newsize += n;
    if(newsize < _minsize)
        newsize = _minsize;

    RenderAPI::ObjectData *p = (RenderAPI::ObjectData*)realloc(_ptr, sizeof(RenderAPI::ObjectData) * newsize);
    if(!p)
        return NULL;

    _ptr = p;
    _cap = newsize;
    return p;
}

DynamicBuffer::DynamicBuffer()
    : usage(BUFFER_DYNAMIC)
{
    _bufid = 0;
    _data = NULL;
    _bytes = 0;
}

DynamicBuffer::~DynamicBuffer()
{
    free(_data);
}

void* DynamicBuffer::resizeBytes(size_t bytes)
{
    _bytes = bytes;
    return realloc(_data, bytes);
}

void* DynamicBuffer::_ensureBytes(size_t bytes)
{
    if(bytes < _bytes)
        return _data;

    size_t newsize = 2 * _bytes;
    if(newsize < bytes)
        newsize += bytes;

    return resizeBytes(newsize);
}
