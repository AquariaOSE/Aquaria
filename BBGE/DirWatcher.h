#pragma once

#include "Base.h"

namespace DirWatcher {

enum Flags
{
	NONE = 0,
	RECURSIVE = 0x01
};

enum Action
{
	UNKNOWN,
	CREATED,
	MODIFIED,
};

typedef void (*Callback)(const std::string& fn, Action act, void *ud);


bool Init();
void Shutdown();
void Pump(); // call from mainloop

size_t AddWatch(const char *path, Flags flags, Callback cb, void *ud); // returns > 0 on success
void RemoveWatch(size_t idx);


} // end namespace DirWatcher
