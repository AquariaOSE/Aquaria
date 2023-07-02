#ifndef BBGE_RENDER_HIGH_H
#define BBGE_RENDER_HIGH_H

// High-level API for the renderer

#include "RenderAPI.h"
#include "Base.h"
#include <assert.h>

// Stores data for later submission to the renderer.
// Workflow:
// Either: reserve() some, then commit()
// Or: add() some
// Finally: pass (data(), size()) to the renderer
class ObjectList
{
public:
    ObjectList(size_t minsize);
    ~ObjectList();

    RenderAPI::ObjectData *reserve(size_t n); // reserve at least n elements and return pointer to writable area
    void commit(size_t n); // acknowledge writing n elements to previously reserve()d

    RenderAPI::ObjectData& push(const RenderAPI::ObjectData& a); // add one element
    void reset();
    void clear();

    const RenderAPI::ObjectData *data() const { return _ptr; }
    size_t size() const { return _size; }

private:
    RenderAPI::ObjectData *_ptr;
    size_t _size, _cap;
    size_t _minsize;

    RenderAPI::ObjectData *_ensure(size_t n); // ensures that there's space for at least n entries in total
};

template<typename T, unsigned N>
class StaticMesh : public RenderAPI::BufferBase
{
public:
    StaticMesh()
    {
        this->_bufid = 0;
        this->_data = &data[0];
        this->_size = sizeof(data);
    }
    T data[N];

    inline       T& operator[](size_t i)       { assert(i < Countof(data)); return data[i]; }
    inline const T& operator[](size_t i) const { assert(i < Countof(data)); return data[i]; }

    inline void upload() { this->upload(BUFFER_STATIC); }
};

class DynamicBuffer : public RenderAPI::BufferBase
{
public:
    typedef RenderAPI::BufferBase Base;
    DynamicBuffer();
    ~DynamicBuffer();
    Hint usage;
    inline void upload() { Base::upload(usage); }
    void *resizeBytes(size_t bytes); // resize to exactly this size
protected:
    void *_ensureBytes(size_t bytes); // ensures that there's space for at least that many bytes
};

template<typename T>
class DynamicMesh : public DynamicBuffer
{
public:
    T& push(const T& val)
    {
        size_t sz = _size;
        T *p = _ensure(sz + 1);
        _size = sz + 1
        return (p[sz] = val);
    }

    inline T *resize(size_t n) // ptr to start
    {
        return (T*)this->resizeBytes(n);
    }

    inline T *reserve(size_t n) // ptr to writable region of n elems
    {
        return _ensure(n) + _size;
    }

    inline void commit(size_t n) // commit prev. reserve()d
    {
        _size += n;
    }

    inline void reset()
    {
        _size = 0;
    }

    inline       T& operator[](size_t i)       { assert(i < _size); return ((      T*)_data)[i]; }
    inline const T& operator[](size_t i) const { assert(i < _size); return ((const T*)_data)[i]; }

private:
    T *_ensure(size_t n)
    {
        return (T*)this->_ensureBytes(n * sizeof(T));
    }

    size_t _size; // in elements
};


#endif // BBGE_RENDER_HIGH_H
