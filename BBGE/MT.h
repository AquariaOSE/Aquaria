#ifndef BBGE_MT_H
#define BBGE_MT_H

#include <cstddef>
#include <queue>
#include <map>

class Lockable
{
public:
	Lockable();
	virtual ~Lockable();
	void lock();
	void unlock();

protected:
	inline void *mutex() { return _mtx; }

private:
	void *_mtx;
};

class Waitable : public Lockable
{
public:
	Waitable();
	virtual ~Waitable();
	void wait(); // releases the associated lock while waiting
	void signal(); // signal a single waiting thread
	void broadcast(); // signal all waiting threads

private:
	void *_cond;
};

class MTGuard
{
public:
	MTGuard(Lockable& x) : _obj(&x) { _obj->lock(); }
	MTGuard(Lockable* x) : _obj(x)  { _obj->lock(); }
	~MTGuard() { _obj->unlock(); }
	Lockable *_obj;
};

template <typename T> class BlockingQueue : public Waitable
{
public:
	void push(const T& e)
	{
		lock();
		_q.push(e);
		unlock();
		signal();
	}
	bool pop(T& e) // blocks if empty
	{
		lock();
		while(_q.empty())
			wait();
		e = _q.front();
		_q.pop();
		unlock();
		return true;
	}
private:
	std::queue<T> _q;
};

template <typename T> class LockedQueue : public Lockable
{
public:
	void push(const T& e)
	{
		lock();
		_q.push(e);
		unlock();
	}
	bool pop(T& e) // continues if empty
	{
		lock();
		if(_q.empty())
		{
			unlock();
			return false;
		}
		e = _q.front();
		_q.pop();
		unlock();
		return true;
	}
	bool empty()
	{
		lock();
		bool e = _q.empty();
		unlock();
		return e;
	}

private:
	std::queue<T> _q;
};


#endif
