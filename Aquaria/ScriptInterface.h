/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef SCRIPTINTERFACE_H
#define SCRIPTINTERFACE_H

#include "../BBGE/Base.h"

struct lua_State;
struct LuaAlloc;

class Entity;
class CollideEntity;
class ScriptedEntity;

class Script
{
public:
	Script(lua_State *L, const std::string &file) : L(L), file(file) {}

	// function()
	bool call(const char *name);
	// function(number)
	bool call(const char *name, float param1);
	// function(pointer)
	bool call(const char *name, void *param1);
	// function(pointer, number)
	bool call(const char *name, void *param1, float param2);
	// function(pointer, pointer)
	bool call(const char *name, void *param1, void *param2);
	// function(pointer, number, number)
	bool call(const char *name, void *param1, float param2, float param3);
	// boolean = function(pointer, number, number)
	bool call(const char *name, void *param1, float param2, float param3, bool *ret1);
	// boolean = function(pointer, int, int, int)
	bool call(const char *name, void *param1, int param2, int param3, int param4, bool *ret1);
	// function(pointer, string, number)
	bool call(const char *name, void *param1, const char *param2, float param3);
	// function(pointer, string, pointer)
	bool call(const char *name, void *param1, const char *param2, void *param3);
	// function(pointer, pointer, int)
	bool call(const char *name, void *param1, void *param2, int param3);
	// function(pointer, pointer, pointer)
	bool call(const char *name, void *param1, void *param2, void *param3);
	// function(pointer, number, number, number)
	bool call(const char *name, void *param1, float param2, float param3, float param4);
	// function(pointer, pointer, pointer, pointer)
	bool call(const char *name, void *param1, void *param2, void *param3, void *param4);
	// boolean = function(pointer, pointer, pointer, number, number, number, number, pointer)
	bool call(const char *name, void *param1, void *param2, void *param3, float param4, float param5, float param6, float param7, void *param8, bool *ret1);
	// boolean = function(string)
	bool call(const char *name, const char *param, bool *ret);
	// boolean = function(pointer)
	bool call(const char *name, void *param1, bool *ret1);
	// boolean = function(pointer, bool)
	bool call(const char *name, void *param1, bool param2, bool *ret1);
	// boolean = function(pointer, pointer)
	bool call(const char *name, void *param1, void *param2, bool *ret1);
	// string = function(string)
	bool call(const char *name, const char *param, std::string *ret);
	// string = function(string, string, string)
	bool call(const char *name, const char *param1, const char *param2, const char *param3, std::string *ret);
	// function(pointer, ...) - anything that is already on the stack is forwarded. Results are left on the stack.
	// Returns how many values the called function returned, or -1 in case of error.
	int callVariadic(const char *name, lua_State *L, int nparams, void *param);
	// Pushes the entity's "v" table on top of the passed Lua stack. Returns number of things pushed (0 or 1)
	int pushLocalVars(lua_State *Ltarget);

	lua_State *getLuaState() {return L;}
	const std::string &getFile() {return file;}
	const std::string &getLastError() {return lastError;}

protected:
	// Low-level helpers.
	void lookupFunc(const char *name);
	bool doCall(int nparams, int nrets = 0);

	lua_State *L;
	std::string file;
	std::string lastError;
};

class ScriptInterface
{
public:
	ScriptInterface();
	void init();
	void reset();
	void collectGarbage();
	int gcGetStats();
	void shutdown();

	Script *openScript(const std::string &file, bool ignoremissing = false);
	void closeScript(Script *script);

	bool runScript(const std::string &file, const std::string &func, bool ignoremissing = false);
	bool runScriptNum(const std::string &file, const std::string &func, int num);

	static std::string MakeScriptFileName(const std::string& name, const char *subdir);

protected:
	lua_State *createLuaVM();
	lua_State *createLuaThread(const std::string &file);
	int destroyLuaThread(const std::string &file, lua_State *thread);
	static void *the_alloc(void *ud, void *ptr, size_t osize, size_t nsize);

	lua_State *baseState;
	LuaAlloc *_LA;
};

#endif
