#ifndef CG_CORE_REFCOUNTED_H
#define CG_CORE_REFCOUNTED_H

#include <stddef.h>
#include <assert.h>
#include <algorithm>


class Refcounted
{
protected:

	Refcounted() : _refcount(0)
	{
	}
	virtual ~Refcounted()
	{
		assert(_refcount == 0 && "Object was deleted with refcount != 0");
	}

public:

	inline void incref()
	{
		++_refcount;
	}
	inline void decref()
	{
		if (!--_refcount)
			delete this;
	}
	inline unsigned refcount() const
	{
		return _refcount;
	}

private:
	unsigned _refcount;
};


template<typename T> class CountedPtr
{
public:
	inline ~CountedPtr()
	{
		if(_p)
			_p->decref();
	}
	inline CountedPtr() : _p(NULL)
	{}
	inline CountedPtr(T* p) : _p(p)
	{
		if(p)
			p->incref();
	}
	inline CountedPtr(const CountedPtr& ref) : _p(ref._p)
	{
		if (_p)
			_p->incref();
	}

	// intentionally not a reference -- see http://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
	CountedPtr& operator=(CountedPtr ref)
	{
		CountedPtr::swap(*this, ref);
		return *this;
	}

	const T* operator->() const  { return _p; }
	      T* operator->()        { return _p; }

	bool operator!() const { return !_p; }

	// Safe for use in if statements
	operator bool() const  { return !!_p; }

	      T* content ()       { return _p; }
	const T* content () const { return _p; }

	bool operator<(const CountedPtr& ref) const { return _p < ref._p; }
	bool operator<=(const CountedPtr& ref) const { return _p <= ref._p; }
	bool operator==(const CountedPtr& ref) const { return _p == ref._p; }
	bool operator!=(const CountedPtr& ref) const { return _p != ref._p; }
	bool operator>=(const CountedPtr& ref) const { return _p >= ref._p; }
	bool operator>(const CountedPtr& ref) const { return _p > ref._p; }

	inline static void swap(CountedPtr& a, CountedPtr& b)
	{
		std::swap(a._p, b._p);
	}

private:

	T *_p;
};


#endif
