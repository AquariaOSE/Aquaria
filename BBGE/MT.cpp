#include "MT.h"
#include "Base.h"

#ifdef BBGE_BUILD_SDL

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

#endif // BBGE_BUILD_SDL
