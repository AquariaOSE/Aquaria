#ifndef BBGE_DATASTRUCTURES_H
#define BBGE_DATASTRUCTURES_H

#include <stddef.h>
#include <vector>
#include <algorithm>
#include <assert.h>

template<typename T>
class Array2d
{
protected:
    std::vector<T> _v;
    size_t _w, _h;

public:
    Array2d() : _w(0), _h(0) {}
    Array2d(size_t w, size_t h) : _w(w), _h(h), _v(w*h) {}

    size_t width() const {return _w;}
    size_t height() const {return _h;}
    void init(size_t w, size_t h)
    {
        _w = w;
        _h = h;
        _v.resize(w*h);
    }

    void clear()
    {
        _w = _h = 0;
        _v.clear();
    }

    bool empty() const
    {
        return _v.empty();
    }

    size_t linearsize() const
    {
        return _v.size();
    }

    void fill(const T& v)
    {
        std::fill(_v.begin(), _v.end(), v);
    }

    void copy2d(size_t dstx, size_t dsty, const Array2d<T>& src, size_t srcx, size_t srcy, size_t w, size_t h)
    {
        assert(dstx + w <= width());
        assert(dsty + h <= height());
        assert(srcx + w <= src.width());
        assert(srcy + h <= src.height());

        for(size_t y = 0; y < h; ++y)
        {
            T *dstrow = row(dsty + y);
            const T *srcrow = src.row(srcy + y);
            std::copy(srcrow + srcx, srcrow + srcx + w, dstrow + dstx);
        }
    }

    const T& at(size_t x, size_t y, const T& def) const
    {
        return x < _w && y < _h ? _v[y * _w + x] : def;
    }

    inline T& operator()(size_t x, size_t y)
    {
        return _v[y * _w + x];
    }
    inline const T& operator()(size_t x, size_t y) const
    {
        return _v[y * _w + x];
    }

    const T *data() const { return _v.empty() ? NULL : &_v[0]; }
          T *data()       { return _v.empty() ? NULL : &_v[0]; }

    const T *row(size_t y) const { return &_v[y * _w]; }
          T *row(size_t y)       { return &_v[y * _w]; }
};


#endif
