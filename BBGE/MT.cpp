#include "MT.h"
#include "Base.h"


// If more threads are idle than this, start killing excess threads.
// Note: This has nothing to do with the amount of CPU cores.
//       The thread pool will create and destroy threads on demand.
const int spareThreads = 8;


// --------- Lockable ----------

Lockable::Lockable()
: _mtx(NULL)
{
#ifdef BBGE_BUILD_SDL
	_mtx = SDL_CreateMutex();
#endif
}

Lockable::~Lockable()
{
#ifdef BBGE_BUILD_SDL
	SDL_DestroyMutex((SDL_mutex*)_mtx);
#endif
}

void Lockable::lock()
{
#ifdef BBGE_BUILD_SDL
	SDL_LockMutex((SDL_mutex*)_mtx);
#endif
}

void Lockable::unlock()
{
#ifdef BBGE_BUILD_SDL
	SDL_UnlockMutex((SDL_mutex*)_mtx);
#endif
}

// --------- Waitable ----------

Waitable::Waitable()
: _cond(NULL)
{
#ifdef BBGE_BUILD_SDL
	_cond = SDL_CreateCond();
#endif
}

Waitable::~Waitable()
{
#ifdef BBGE_BUILD_SDL
	SDL_DestroyCond((SDL_cond*)_cond);
#endif
}

void Waitable::wait()
{
#ifdef BBGE_BUILD_SDL
	SDL_CondWait((SDL_cond*)_cond, (SDL_mutex*)mutex());
#endif
}

void Waitable::signal()
{
#ifdef BBGE_BUILD_SDL
	SDL_CondSignal((SDL_cond*)_cond);
#endif
}

void Waitable::broadcast()
{
#ifdef BBGE_BUILD_SDL
	SDL_CondBroadcast((SDL_cond*)_cond);
#endif
}

// --------- Runnable ----------

void Runnable::wait()
{
	MTGuard(this);
	while (!_done)
		Waitable::wait();
}

void Runnable::run()
{
	_run(); // this is the job's entry point

	lock();
		_done = true;
		// we may get deleted  by another thread directly after unlock(), have to save this on the stack
		volatile bool suicide = _suicide; 
		broadcast(); // this accesses _cond, and must be done before unlock() too, same reason
	unlock();
	
	if (suicide)
		delete this;
}


// --------- ThreadPool ----------

static int threadpool_runner(void *ptr)
{
	ThreadPool *pool = (ThreadPool*)ptr;
	pool->_runThread();
	return 0;
}

ThreadPool::ThreadPool()
: _quit(false), _idleThreads(0)
{
	// If we fail to create a few threads right from the start, fall back to single-threaded mode
	if (!ensureFreeThreads(spareThreads))
	{
		debugLog("ThreadPool: Failed to spawn initial threads");
		shutdown();
	}
}

void ThreadPool::shutdown()
{
	lock();
	_quit = true;
	while (_threads.size())
	{
		_jobs.push(NULL);
		wait();
	}
	unlock();

	cleanupDeadThreads();
}

ThreadPool::~ThreadPool()
{
	shutdown();
}

void ThreadPool::waitForThread(void *th)
{
#ifdef BBGE_BUILD_SDL
	SDL_WaitThread((SDL_Thread*)th, NULL);
#endif
}

void *ThreadPool::u_createThread()
{
	void *th = NULL;
#ifdef BBGE_BUILD_SDL
	th = SDL_CreateThread(threadpool_runner, this);
#endif
	if (!th)
		return NULL;
	u_addThread(th);
	return th;
}

bool ThreadPool::u_ensureFreeThreads(int c)
{
	if (_quit)
		return false;

	for (int i = _idleThreads; i < c; ++i)
		if (!u_createThread())
			return false;

	return true;
}

bool ThreadPool::ensureFreeThreads(int c)
{
	MTGuard(this);
	return u_ensureFreeThreads(c);
}

bool ThreadPool::addJob(Runnable *job, RunnablePriority rp /* = RP_START */)
{
	if (!job)
		return false;

	{
		MTGuard(this);
		if (_quit)
			return false;
		if (rp == RP_START)
		{
			if (!u_ensureFreeThreads(1))
				return false; // unable to start job right away, reject
		}
		if (_idleThreads > spareThreads)
			_jobs.push(NULL); // kill one excess thread
	}

	_jobs.push(job);
	cleanupDeadThreads();
	return true;
}

void ThreadPool::incrIdle()
{
	MTGuard(this);
	_idleThreads++;
}

void ThreadPool::decrIdle()
{
	MTGuard(this);
	_idleThreads--;
}

void ThreadPool::u_addThread(void *th)
{
#ifdef BBGE_BUILD_SDL
	_threads[SDL_GetThreadID((SDL_Thread*)th)] = th;
#endif
}

// called by one thread to notify the pool that it is about to die off
void ThreadPool::removeSelf()
{
	int id = -1;
#if BBGE_BUILD_SDL
	id = SDL_ThreadID();
#endif
	
	MTGuard(this);
	std::map<int, void*>::iterator it = _threads.find(id);
	if (it != _threads.end())
	{
		// still needs to be collected by *another* thread.
		// the problem here is that only using SDL_WaitThread() frees resources,
		// but this thread can't wait for itself (otherwise deadlock).
		_deadThreads.push_back(it->second);
		_threads.erase(it); // no longer active
	}
}

void ThreadPool::cleanupDeadThreads()
{
	lock();
	if (_deadThreads.size() > 1)
	{
		std::vector<void*> copy = _deadThreads;
		_deadThreads.clear();
		unlock();

		// we know these threads are dead, but to minimize potential wait time
		// while a thread is still dying, operate on a copy.
		for (size_t i = 0; i < copy.size(); ++i)
			waitForThread(copy[i]);
	}
	else if (_deadThreads.size() == 1) // this case is very likely, treat this specially to keep copying down
	{
		void *th = _deadThreads[0];
		_deadThreads.clear();
		unlock();
		waitForThread(th);
	}
	else
		unlock();
}

void ThreadPool::_runThread()
{
	while (!_quit)
	{
		incrIdle(); // going to wait until we get a job
		Runnable *job = _jobs.pop(); // this blocks until a job is available
		decrIdle(); // .. time to work.
		if (job)
			job->run();
		else
			break; // use NULL as suicide signal
	}
	removeSelf(); // register thread for garbage
	broadcast(); // tell the pool we are dying off
}

