#include "DirWatcher.h"
#include <string>
#include <assert.h>
#include <sstream>

#include <SDL_mutex.h>
#include "dmon.h"

namespace DirWatcher {




struct WatchEntry
{
	WatchEntry() : valid(false) {}
	Callback cb;
	void *ud;
	dmon_watch_id watch;
	std::string path;
	bool valid;
};

struct Pending
{
	std::string fn;
	size_t idx;
	Action act;
};

static std::vector<WatchEntry> s_watches;
static std::vector<Pending> s_pending, s_processing;
static SDL_mutex *s_mtx;

bool Init()
{
	s_mtx = SDL_CreateMutex();
	dmon_init();
	return true;
}

void Shutdown()
{
	if(!s_mtx)
		return;
	for(size_t i = 0; i < s_watches.size(); ++i)
		if(s_watches[i].valid)
			dmon_unwatch(s_watches[i].watch);
	s_watches.clear();
	s_pending.clear();
	dmon_deinit();
	SDL_DestroyMutex(s_mtx);
	s_mtx = NULL;
}

void Pump()
{
	if(!s_mtx)
		return;

	SDL_LockMutex(s_mtx);
	const size_t N = s_pending.size();
	if(!N)
	{
		SDL_UnlockMutex(s_mtx);
		return;
	}

	s_processing.swap(s_pending);
	assert(s_pending.empty());
	SDL_UnlockMutex(s_mtx);

	std::ostringstream os;
	os << "Processing " << N << " queued file modification notifications...";
	debugLog(os.str());

	std::string tmp;

	for(size_t i = 0; i < N; ++i)
	{
		const WatchEntry& we = s_watches[s_processing[i].idx];
		if(!we.valid)
			continue;

		tmp.clear();
		tmp += we.path;
		if(we.path[we.path.length() - 1] != '/')
			tmp += '/';
		tmp += s_processing[i].fn;

		// Only look at files that exist on disk (filter out create->rename sequences)
		if(!exists(tmp))
		{
			tmp += " -- no longer exists, skipping";
			debugLog(tmp);
			continue;
		}

		debugLog(tmp);

		we.cb(tmp, s_processing[i].act, we.ud);
	}
	s_processing.clear();

	debugLog("... callbacks done.");
}

static void _watch_cb(dmon_watch_id watch_id, dmon_action action,
	const char* rootdir, const char* filepath,
	const char* oldfilepath, void* user)
{
	size_t idx = (size_t)(uintptr_t)user;

	Pending p;
	p.fn = filepath;
	p.idx = idx;
	p.act = UNKNOWN;
	switch(action)
	{
		case DMON_ACTION_CREATE:
			p.act = CREATED;
			break;
		case DMON_ACTION_MODIFY:
		case DMON_ACTION_MOVE:
			p.act = MODIFIED;
			break;
		default:
			return; // ignore
	}

	SDL_LockMutex(s_mtx);
	s_pending.push_back(p);
	SDL_UnlockMutex(s_mtx);
}

size_t AddWatch(const char* path, Flags flags, Callback cb, void* ud)
{
	const size_t N = s_watches.size();
	if(!N && !s_mtx)
		if(!Init())
			return 0;

	size_t idx = N;
	for(size_t i = 0; i < N; ++i)
		if(!s_watches[i].valid)
		{
			idx = i;
			break;
		}
	WatchEntry we;
	we.cb = cb;
	we.valid = false;
	we.path = path;
	if(idx == N)
		s_watches.push_back(we);
	else
		s_watches[idx] = we;

	unsigned flg = DMON_WATCHFLAGS_FOLLOW_SYMLINKS;
	if(flags & RECURSIVE)
		flg = DMON_WATCHFLAGS_RECURSIVE;

	s_watches[idx].watch = dmon_watch(path, _watch_cb, (dmon_watch_flags_t)flg, (void*)(uintptr_t)idx);
	if(!s_watches[idx].watch.id)
	{
		std::ostringstream os;
		os << "Failed to attach dir watcher to [" << path << "]";
		errorLog(os.str());
		return 0;
	}
	s_watches[idx].valid = true;
	return idx + 1;
}

void RemoveWatch(size_t idx)
{
	if(!idx)
		return;
	--idx;

	assert(s_watches[idx].valid);
	dmon_unwatch(s_watches[idx].watch);
	s_watches[idx].valid = false;
}



} // end namespace DirWatcher
