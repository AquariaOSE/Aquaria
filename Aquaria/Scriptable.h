#ifndef AQUARIA_SCRIPTABLE_H
#define AQUARIA_SCRIPTABLE_H

class Script;
struct lua_State;

// Object that has a script attached
class Scriptable
{
public:
	Scriptable();

	Script *script; // NULL if no script is attached

	int pushLuaVars(lua_State *L);
	void closeScript();

	// Note! Before you attempt to move here some common functions like message() or anything that takes a 'this'-pointer:
	// ScriptInterface uses raw pointers everywhere. 'this' is always passed as a void*, so if we make any such method that pushes 'this'
	// a method of the Scriptable class, then that would pass an offset pointer.
	// Eg. Entity *e casted to ((void*)e) is not the same as ((void*)(Scriptable*)e)! And since ScriptInterface does (Entity*)lua_touserdata(L, slot),
	// this would break horribly since the necessary type infos to fix the pointer are not preserved.
	// A fix would be to dynamic_cast from a common base class, but right now that isn't worth the hassle.
};

#endif
