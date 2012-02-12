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
	T pop() // blocks if empty
	{
		lock();
		while(_q.empty())
			wait();
		T ret = _q.front();
		_q.pop();
		unlock();
		return ret;
	}
private:
	std::queue<T> _q;
};

class ThreadPool;

// To be distributed to workers in ThreadPool.
class Runnable : public Waitable
{
public:
	Runnable(bool suicide) : _done(false), _shouldquit(false), _suicide(suicide) {};
	virtual ~Runnable() {}

	void run();
	inline void quit() { _shouldquit = true; }
	inline bool shouldQuit() const { return _shouldquit; }
	inline bool isDone() const { return _done; }

	virtual void wait();

protected:
	virtual void _run() = 0; // to be overloaded

private:
	volatile bool _done;
	volatile bool _shouldquit;
	bool _suicide;
};


class FunctionRunnable : public Runnable
{
public:
	typedef void (*Func)(void*);

	FunctionRunnable(bool suicide, Func f, void *ptr = NULL)
		: Runnable(suicide), _func(f), _param1(ptr) {}

	virtual ~FunctionRunnable() {}

protected:
	virtual void _run()
	{
		_func(_param1);
	}

	Func _func;
	void *_param1;
};

enum RunnablePriority
{
	RP_ENQUEUE = -1, // just enqueue and let a thread run it when one gets free
	RP_START = 0, // execute right away, start another thread if none is free
};

class ThreadPool : protected Waitable
{
public:
	ThreadPool();
	~ThreadPool();

	// Push a job to the queue, by default starting a thread to execute it, if necessary.
	// If adding the job failed for some reason, the caller can use job->run() to execute it manually.
	// When a job is done, it will broadcast.
	bool addJob(Runnable *job, RunnablePriority rp = RP_START);

	// Have at least c threads idle, otherwise spawn some until c are idle.
	bool ensureFreeThreads(int c);

	// The ThreadPool will no longer accept any jobs after this was called, and signal threads to stop.
	// All jobs still left in the queue will be executed.
	// Waits for all threads to exit.
	void shutdown();

	void _runThread();

private:
	// these lock 'this', do not call while already holding the lock
	void incrIdle();
	void decrIdle();
	void waitForThread(void*);
	void cleanupDeadThreads();
	void removeSelf();
	
	// need to have 'this' locked when calling these
	void *u_createThread();
	void u_addThread(void*);
	bool u_ensureFreeThreads(int c);

	BlockingQueue<Runnable*> _jobs;
	std::map<int, void*> _threads;
	std::vector<void*> _deadThreads; // necessary to clean up
	volatile bool _quit;
	volatile int _idleThreads;
};


#endif
