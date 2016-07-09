#include "MT.h"
#include "SDL.h"


// --------- Lockable ----------

Lockable::Lockable()
: _mtx(NULL)
{
	_mtx = SDL_CreateMutex();
}

Lockable::~Lockable()
{
	SDL_DestroyMutex((SDL_mutex*)_mtx);
}

void Lockable::lock()
{
	SDL_LockMutex((SDL_mutex*)_mtx);
}

void Lockable::unlock()
{
	SDL_UnlockMutex((SDL_mutex*)_mtx);
}

// --------- Waitable ----------

Waitable::Waitable()
: _cond(NULL)
{
	_cond = SDL_CreateCond();
}

Waitable::~Waitable()
{
	SDL_DestroyCond((SDL_cond*)_cond);
}

void Waitable::wait()
{
	SDL_CondWait((SDL_cond*)_cond, (SDL_mutex*)mutex());
}

void Waitable::signal()
{
	SDL_CondSignal((SDL_cond*)_cond);
}

void Waitable::broadcast()
{
	SDL_CondBroadcast((SDL_cond*)_cond);
}

