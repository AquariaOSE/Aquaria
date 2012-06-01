// VFSHashmap.h - minimalist but fast STL compatible hashmap implementation.
// For conditions of distribution and use, see copyright notice in VFS.h

#ifndef VFS_HASHMAP_H
#define VFS_HASHMAP_H

#include <vector>
#include "VFSDefines.h"

VFS_NAMESPACE_START


template<typename KEY> struct hash_cast
{
    inline size_t operator()(const KEY& t) const
    {
        return (size_t)t;
    }
    inline size_t operator()(const KEY *t) const
    {
        return (size_t)*t;
    }
};

template<typename KEY, typename T> struct equal
{
    inline bool operator()(const KEY& a, const KEY& b, size_t /*hash*/, const T& /*value*/) const
    {
        return a == b;
    }
};

// (http://graphics.stanford.edu/~seander/bithacks.html)
inline size_t nextPowerOf2(size_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    v += (v == 0);
    return v;
}

template
<
    typename KEY,
    typename T,
    typename HASH,
    typename CMP,
    typename BucketType
>
class _HashMapBase
{
    // ---- supplemental stuff ----

public:

    typedef T value_type;
    typedef std::size_t size_type;
    typedef typename BucketType::iterator ITR;

    class iterator : public std::iterator<std::bidirectional_iterator_tag, typename ITR::value_type>
    {
    public:

        typedef typename ITR::value_type value_type;
        typedef typename ITR::reference reference;
        typedef typename ITR::pointer pointer;


        iterator()
            : _bucket(-1), _hm(NULL)
        {
        }

        iterator(size_t bucket, const ITR& it, _HashMapBase *hm)
            : _bucket(bucket), _it(it), _hm(hm)
        {
            _nextbucket();
        }

        iterator(const iterator& it)
            : _bucket(it._bucket), _it(it._it), _hm(it._hm)
        {
            _nextbucket();
        }

        iterator& operator=(const iterator& o)
        {
            if(this == &o)
                return *this;
            _bucket = o._bucket;
            _it = o._it;
            _hm = o._hm;
            return *this;
        }

        void _nextbucket()
        {
            while(_bucket+1 < _hm->v.size() && _it == _hm->v[_bucket].end())
                _it = _hm->v[++_bucket].begin();
        }

        void _prevbucket()
        {
            while(_bucket > 0 && _it == _hm->v[_bucket].begin())
                _it = _hm->v[--_bucket].back();
        }

        iterator& operator++(void) // pre-increment
        {
            ++_it;
            _nextbucket();
            return *this;
        }

        iterator operator++(int) // post-increment
        {
            iterator it = *this;
            ++*this;
            return it;
        }

        iterator& operator--(void) // pre-decrement
        {
            --_it;
            _prevbucket();
            return *this;
        }

        iterator operator--(int) // post-decrement
        {
            iterator it = *this;
            --*this;
            return it;
        }

        bool operator==(const iterator& o) const
        {
            return _hm == o._hm && _bucket == o._bucket && _it == o._it;
        }

        bool operator!=(const iterator& o) const
        {
            return !(*this == o);
        }

        bool operator<(const iterator& o) const
        {
            return _bucket < o._bucket || (_bucket == o._bucket && _it < o._it);
        }

        bool operator<=(const iterator& o) const
        {
            return !(*this > o);
        }

        bool operator>(const iterator& o) const
        {
            return _bucket > o._bucket || (_bucket == o._bucket && _it > o._it);
        }

        bool operator>=(const iterator& o) const
        {
            return !(*this < o);
        }

        reference operator*()
        {
            return _it.operator*();
        }

        pointer operator->()
        {
            return _it.operator->();
        }

        size_t _bucket;
        ITR _it;
        _HashMapBase *_hm;
    };


protected:

    // ---- Main class start ----

    _HashMapBase(size_t buckets, int loadFactor, const CMP& c, const HASH& h)
        : v(nextPowerOf2(buckets)), _size(0), _loadFactor(loadFactor), cmp(c), hash(h)
    {}

    _HashMapBase(const _HashMapBase& o)
        : v(o.v), _size(o._size), _loadFactor(o._loadFactor), cmp(o.cmp), hash(o.hash)
    {}

public:

    _HashMapBase& operator=(const _HashMapBase& o)
    {
        if(this == &o)
            return *this;
        v = o.v;
        cmp = o.cmp;
        hash = o.hash;
        _size = o._size;
        _loadFactor = o._loadFactor;
    }


    inline iterator begin()
    {
        return _MakeIter(0, v[0].begin());
    }

    inline iterator end()
    {
        return _MakeIter(v.size()-1, v[v.size()-1].end());
    }

    iterator find(const KEY& k)
    {
        size_t h = hash(k);
        size_t i = h & (v.size()-1); // assume power of 2
        BucketType& b = v[i];
        for(typename BucketType::iterator it = b.begin(); it != b.end(); ++it)
            if(cmp(k, it->first, h, it->second))
                return _MakeIter(i, it);

        return end();
    }

    void erase(const KEY& k)
    {
        size_t h = hash(k);
        BucketType& b = v[h & (v.size()-1)]; // assume power of 2
        for(typename BucketType::iterator it = b.begin(); it != b.end(); ++it)
        {
            if(cmp(k, it->first, h, it->second))
            {
                b.erase(it);
                --_size;
                return;
            }
        }
    }

    inline iterator erase(const iterator& it)
    {
        --_size;
        return _MakeIter(it._bucket, v[it._bucket].erase(it._it));
    }

    T& operator[] (const KEY& k)
    {
        size_t h = hash(k);
        size_t i = h & (v.size()-1); // assume power of 2
        {
            BucketType& b = v[i];
            for(typename BucketType::iterator it = b.begin(); it != b.end(); ++it)
                if(cmp(k, it->first, h, it->second))
                    return it->second;
        }
        ++_size;
        if(_enlargeIfNecessary())
            i = h & (v.size()-1);
        v[i].push_back(std::make_pair(k, T()));
        return v[i].back().second;
    }

    inline size_t size() const
    {
        return _size;
    }

    /* "Because map containers do not allow for duplicate key values, the insertion operation
    checks for each element inserted whether another element exists already in the container
    with the same key value, if so, the element is not inserted and its mapped value
    is not changed in any way." */ // Oh well.
    /*void insert(std::pair<KEY, T>& p)
    {
        size_t h = hash(p.first);
        size_t i = h & (v.size()-1);
        {
            BucketType& b = v[i]; // assume power of 2
            for(typename BucketType::iterator it = b.begin(); it != b.end(); ++it)
                if(cmp(p.first, it->first, h, it->second))
                    return _MakeIter(i, it);
        }
        ++_size;
        if(_enlargeIfNecessary())
            i = h & (v.size()-1);
        v[i].push_back(std::make_pair(k, T()));
        return _MakeIter(i, b.end()-1);
    }*/ // -- not used in ttvfs currently



private:

    inline iterator _MakeIter(size_t bucket, const typename BucketType::iterator& it)
    {
        return iterator(bucket, it, this);
    }

    inline bool _enlargeIfNecessary()
    {
        if(_loadFactor < 0)
            return false;

        if(_size > v.size() * _loadFactor)
        {
            _enlarge();
            return true;
        }
        return false;
    }

    void _enlarge()
    {
        size_t oldsize = v.size();
        v.resize(oldsize * 2);
        BucketType cp;
        for(size_t i = 0; i < oldsize; ++i)
        {
            cp.clear();
            std::swap(cp, v[i]); // use efficient swap
            // v[i] is now empty
            // this way can possibly copy elements 2 times, but means less container copying overall
            for(typename BucketType::iterator it = cp.begin(); it != cp.end(); ++it)
                v[hash(it->first) & (v.size()-1)].push_back(*it); // assume power of 2
        }
    }

    inline void swap(_HashMapBase& hm)
    {
        if(this == &hm)
            return;
        std::swap(v, hm.v);
        std::swap(_size, hm._size);
        std::swap(_loadFactor, hm._loadFactor);
        std::swap(hash, hm.hash);
        std::swap(cmp, hm.cmp);
    }

    std::vector<BucketType> v;
    size_t _size;
    int _loadFactor;

    HASH hash; // hash functor
    CMP cmp;   // compare functor
};

template
<
    typename KEY,
    typename T,
    typename HASH = hash_cast<KEY>,
    typename CMP = equal<KEY, T>,
    typename BucketType = std::vector<std::pair<KEY,T> >
>
class HashMap : public _HashMapBase<KEY, T, HASH, CMP, BucketType>
{
public:
    HashMap(size_t buckets = 16, int loadFactor = 5)
        : _HashMapBase<KEY, T, HASH, CMP, BucketType>(buckets, loadFactor, CMP(), HASH())
    {
    }


};

VFS_NAMESPACE_END

namespace std
{
    template
    <
        typename KEY,
        typename T,
        typename HASH,
        typename CMP,
        typename BucketType
    >
    inline static void swap(VFS_NAMESPACE_IMPL HashMap<KEY, T, HASH, CMP, BucketType>& a,
                            VFS_NAMESPACE_IMPL HashMap<KEY, T, HASH, CMP, BucketType>& b)
    {
        a.swap(b);
    }
};

#endif
