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
#include "ScriptInterface.h"
#include "../BBGE/ScriptObject.h"
extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "ScriptedEntity.h"
#include "Shot.h"
#include "Entity.h"
#include "Web.h"
#include "GridRender.h"
#include "AfterEffect.h"

#include "../BBGE/MathFunctions.h"

#if defined(AQUARIA_FULL) || defined(AQUARIA_DEMO)

// Define this to 1 to check types of pointers passed to functions,
// and warn if a type mismatch is detected. In this case,
// the pointer is treated as NULL, to avoid crashing or undefined behavior.
// Note: There are a few functions that depend on this (isObject and related).
//       They will still work as expected when this is disabled.
#define CHECK_POINTER_TYPES 1

// If true, send all sort of script errors to errorLog instead of debugLog.
// On win32/OSX, this pops up message boxes which help to locate errors easily,
// but can be annoying for regular gameplay.
const bool loudScriptErrors = false;

// Set this to true to complain whenever a script tries to
// get or set a global variable.
const bool complainOnGlobalVar = false;

// Set this to true to complain whenever a script tries to get an undefined
// thread-local variable.
const bool complainOnUndefLocal = false;

#else

// Use maximal safety for developer builds.
#define CHECK_POINTER_TYPES 1
const bool loudScriptErrors = true;
const bool complainOnGlobalVar = true;
const bool complainOnUndefLocal = true;

#endif

// List of all interface functions called by C++ code, terminated by NULL.
static const char * const interfaceFunctions[] = {
	"action",
	"activate",
	"animationKey",
	"castSong",
	"damage",
	"deathNotify",
	"dieEaten",
	"dieNormal",
	"enterState",
	"entityDied",
	"exitState",
	"exitTimer",
	"hitEntity",
	"hitSurface",
	"init",
	"lightFlare",
	"msg",
	"postInit",
	"preUpdate",
	"shiftWorlds",
	"shotHitEntity",
	"song",
	"songNote",
	"songNoteDone",
	"sporesDropped",
	"update",
	"useTreasure",
	NULL
};

//============================================================================================
// R U L E S  F O R  W R I T I N G  S C R I P T S
//============================================================================================

//
// All scripts in Aquaria run as separate threads in the same Lua state.
// This means that scripts must follow certain rules to avoid interfering
// with each other:
//
// -- DON'T use global variables (or functions).  Use file-scope or
//    instance locals instead.
//
//    Since every script runs in the same Lua state and thus shares the
//    same global environment, global variables set in one script affect
//    every other script.  Something as innocuous-looking as "hits = 10"
//    would set a "hits" variable in _every_ script -- overwriting any
//    value that another script might have already set!
//
//    For constant values and functions (which are in effect constant
//    values), you can use Lua's "local" keyword to declare the value as
//    local to the script file which declares it.  Any functions defined
//    later in the file will then be able to access those local values as
//    function upvalues, thus avoiding touching the global environment.
//    (However, remember to define the constants or functions _before_
//    you use them!)
//
//    For variables, a file-scope local won't work, because a script's
//    functions are shared across all instances of that script -- for
//    example, every active jellyfish entity calls the same "update"
//    function.  If you used file-scope locals, then you'd have no way to
//    separate one instance's data from another.  Instead, the Aquaria
//    script engine provides a Lua table specific to each script instance,
//    into which instance-local variables can be stored.  This table is
//    loaded into the global variable "v" when any script function is
//    called from the game, so functions can store variables in this table
//    without worrying that another instance of the script will clobber
//    them.
//
//    The instance-local table is also available to code in the script
//    outside any functions, which is executed when the script is loaded.
//    In this case, the values in the table are used as defaults and
//    copied into the instance-local table of each new instance of the
//    script.
//
//    If you use any include files, be aware that file-scope locals in the
//    include files can only be used inside those files, since they would
//    be out of scope in the calling file.  To export constants or
//    functions to the calling file, you'll have to use instance locals
//    instead.  (As an exception, if you have constants which you know are
//    unique across all scripts, you can define them as globals.  Aquaria
//    itself defines a number of global constants for use in scripts --
//    see the "SCRIPT CONSTANTS" section toward the bottom of this file.)
//
// -- DO define instance functions in the global namespace.
//
//    As an exception to the rule above, interface functions such as
//    init() or update() _should_ be defined in the global namespace.
//    For example:
//
//        local function doUpdateStuff(dt)
//            -- Some update stuff.
//        end
//        function update(dt)
//            doUpdateStuff(dt)
//            -- Other update stuff.
//        end
//
//    The script engine will take care of ensuring that different scripts'
//    functions don't interfere with each other.
//
// -- DON'T call interface functions from within a script.
//
//    Interface functions, such as init() and update(), are treated
//    specially by the script engine, and attempting to call them from
//    other script functions will fail.  If you need to perform the same
//    processing from two or more different interface functions, define
//    a local function with the necessary code and call it from the
//    interface functions.
//
//    It _is_ possible, though not recommended, to have a local function
//    with the same name as an interface function.  For example, if you
//    write a script containing:
//
//        local function activate(me)
//            -- Do something.
//        end
//
//    then you can call activate() from other functions without problems.
//    The local function, activate() in this case, will naturally not be
//    visible to the script engine.  (This is discouraged because someone
//    reading the code may be confused at seeing what looks like an
//    interface function defined locally.)
//
// -- DON'T call any functions from the outermost (file) scope of an
//    instanced script file.
//
//    "Instanced" script files are those for which multiple instances may
//    be created, i.e. entity or node scripts.  For these, the script
//    itself is executed only once, when it is loaded; any statements
//    outside function definitions will be executed at this time, but not
//    when a new script instance is created.  For example, if you try to
//    call a function such as math.random() to set a different value for
//    each instance, you'll instead end up with the same value for every
//    instance.  In cases like this, the variable should be set in the
//    script's init() function, not at file scope.
//
//    Likewise, any functions which have side effects or otherwise modify
//    program state should not be called from file scope.  Call them from
//    init() instead.
//
// -- DON'T declare non-constant Lua tables at file scope in instanced
//    scripts.
//
//    One non-obvious result of the above restrictions is that tables
//    intended to be modified by the script cannot be declared at file
//    scope, even as instance variables.  The reason for this is that
//    table variables in Lua are actually pointers; the Lua statement
//    "v.table = {}" is functionally the same as "v.table = newtable()",
//    where the hypothetical newtable() function allocates and returns a
//    pointer to a table object, and thus falls under the restriction
//    that functions must not be called at file scope.  Table variables
//    in instanced scripts must therefore be initialized in the init()
//    function, even if you're only setting the variable to an empty
//    table.
//
// In summary:
//
// -- Never use global variables or functions, except interface functions.
// -- Constants and local functions should be defined with "local":
//        local MY_CONSTANT = 42
//        local function getMyConstant()  return MY_CONSTANT  end
// -- Variables should be stored in the "v" table:
//        function update(dt)  v.timer = v.timer + dt  end
// -- Variables (except table variables) can have default values set when
//    the script is loaded:
//        v.countdown = 5
//        function update(dt)  v.countdown = v.countdown - dt  end
// -- Non-constant tables must be initialized in init(), even if the
//    variable is always set to the same thing (such as an empty table).
// -- Never call interface functions from other functions.
// -- Always perform instance-specific setup in init(), not at file scope.
//
// ====================
// Compatibility notes:
// ====================
//
// Due to the use of an instance variable table (the "v" global), scripts
// written for this version of Aquaria will _not_ work with commercial
// releases (at least through version 1.1.3) of the game; likewise, the
// scripts from those commercial releases, and mods written to target
// those releases, will not work with this engine.
//
// The latter problem is unfortunately an unsolvable one, in any practical
// sense.  Since the original engine created a new Lua state for each
// script, scripts could create and modify global variables with impunity.
// The mere act of loading such a script could wreak havoc on the single
// Lua state used in the current engine, and attempting to work around
// this would require at least the implementation of a custom Lua parser
// to analyze and/or alter each script before it was passed to the Lua
// interpreter.
//
// However, the former problem -- of writing scripts for this version of
// the engine which also work on earlier versions -- can be solved with
// a few extra lines of code at the top of each script.  Since the new
// engine initializes the "v" global before each call to a script,
// including when the script is first loaded, scripts can check for the
// existence of this variable and assign an empty table to it if needed,
// such as with this line:
//
// if not v then v = {} end
//
// Additionally, the current engine provides built-in constants which
// were formerly loaded from external files.  To differentiate between
// this and other versions of the engine, the script interface exports a
// constant named AQUARIA_VERSION, generated directly from the program
// version (shown on the title screen) as:
//     major*10000 + minor*100 + revision
// For example, in version 1.1.3, AQUARIA_VERSION == 10103.  In earlier
// versions of the engine, the value of this constant will be nil, which
// can be used as a trigger to load the constant definition file from
// that version:
//
// if not AQUARIA_VERSION then dofile("scripts/entities/entityinclude.lua") end
//
// Note that scripts should _not_ rely on AQUARIA_VERSION for the v = {}
// assignment.  The code "if not AQUARIA_VERSION then v = {} end" would
// work correctly in a top-level script, but if executed from a script
// used as an include file, the table created in the include file would
// overwrite any existing table created by the file's caller.
//

//============================================================================================
// S C R I P T  C O M M A N D S
//============================================================================================

static void scriptError(const std::string& msg)
{
	if(loudScriptErrors)
		errorLog(msg);
	else
		debugLog(msg);
}

static inline void luaPushPointer(lua_State *L, void *ptr)
{
	// All the scripts do this:
	//    x = getFirstEntity()
	//    while x =~ 0 do x = getNextEntity() end
	// The problem is this is now a pointer ("light user data"), so in
	//  Lua, it's never equal to 0 (or nil!), even if it's NULL.
	// So we push an actual zero when we get a NULL to keep the existing
	//  scripts happy.  --ryan.
	if (ptr != NULL)
		lua_pushlightuserdata(L, ptr);
	else
		lua_pushnumber(L, 0);
}

static std::string luaFormatStackInfo(lua_State *L, int level = 1)
{
	lua_Debug ar;
	std::ostringstream os;
	if (lua_getstack(L, level, &ar) && lua_getinfo(L, "Sln", &ar))
	{
		os << ar.short_src << ":" << ar.currentline
			<< " ([" << ar.what << "] "  << ar.namewhat << " " << (ar.name ? ar.name : "(?)") << ")";
	}
	else
	{
		os << "???:0";
	}

	return os.str();
}

static void scriptDebug(lua_State *L, const std::string& msg)
{
	debugLog(luaFormatStackInfo(L) + ": " + msg);
}

static void scriptError(lua_State *L, const std::string& msg)
{
	lua_Debug dummy;
	std::ostringstream os;
	os << msg;
	for (int level = 0; lua_getstack(L, level, &dummy); ++level)
		os << '\n' << luaFormatStackInfo(L, level);

	scriptError(os.str());
}


#if CHECK_POINTER_TYPES
// Not intended to be called.
// Because wild typecasting expects X::_objtype to reside at the same relative
// memory location, be sure this is the case before running into undefined behavior later.
// - The C++ standard allows offsetof() only on POD-types. Oh well, it probably works anyways.
// If it does not compile for some reason, comment it out, hope for the best, and go ahead.
#if !(defined(__GNUC__) && __GNUC__ <= 2)
static void compile_time_assertions()
{
#define oo(cls) offsetof(cls, _objtype)
	compile_assert(oo(Path) == oo(RenderObject));
	compile_assert(oo(Path) == oo(Entity));
	compile_assert(oo(Path) == oo(Ingredient));
	compile_assert(oo(Path) == oo(CollideEntity));
	compile_assert(oo(Path) == oo(ScriptedEntity));
	compile_assert(oo(Path) == oo(Beam));
	compile_assert(oo(Path) == oo(Shot));
	compile_assert(oo(Path) == oo(Web));
	compile_assert(oo(Path) == oo(Bone));
	compile_assert(oo(Path) == oo(PauseQuad));
	compile_assert(oo(Path) == oo(Quad));
	compile_assert(oo(Path) == oo(Avatar));
	compile_assert(oo(Path) == oo(BaseText));
	compile_assert(oo(Path) == oo(PauseQuad));
	compile_assert(oo(Path) == oo(Shader));
#undef oo
}
#endif

template <typename T>
static void ensureType(lua_State *L, T *& ptr, ScriptObjectType ty)
{
	if (ptr)
	{
		ScriptObject *so = (ScriptObject*)(ptr);
		if (!so->isType(ty))
		{
			std::ostringstream os;
			os << "WARNING: script passed wrong pointer to function (expected type: "
				<< ScriptObject::getTypeString(ty) << "; got: "
				<< so->getTypeString() << ')';
			scriptError(L, os.str());

			ptr = NULL; // note that the pointer is passed by reference
		}
	}
}
#  define ENSURE_TYPE(ptr, ty) ensureType(L, (ptr), (ty))
#  define typecheckOnly(func) func
#else
#  define ENSURE_TYPE(ptr, ty)
#  define typecheckOnly(func)
#endif

static inline
RenderObject *robj(lua_State *L, int slot = 1)
{
	RenderObject *r = (RenderObject*)lua_touserdata(L, slot);
	ENSURE_TYPE(r, SCO_RENDEROBJECT);
	if (!r)
		scriptDebug(L, "RenderObject invalid pointer.");
	return r;
}

static inline
ScriptedEntity *scriptedEntity(lua_State *L, int slot = 1)
{
	ScriptedEntity *se = (ScriptedEntity*)lua_touserdata(L, slot);
	ENSURE_TYPE(se, SCO_SCRIPTED_ENTITY);
	if (!se)
		scriptDebug(L, "ScriptedEntity invalid pointer.");
	return se;
}

static inline
CollideEntity *collideEntity(lua_State *L, int slot = 1)
{
	CollideEntity *ce = (CollideEntity*)lua_touserdata(L, slot);
	ENSURE_TYPE(ce, SCO_COLLIDE_ENTITY);
	if (!ce)
		scriptDebug(L, "CollideEntity invalid pointer.");
	return ce ;
}

static inline
Beam *beam(lua_State *L, int slot = 1)
{
	Beam *b = (Beam*)lua_touserdata(L, slot);
	ENSURE_TYPE(b, SCO_BEAM);
	if (!b)
		scriptDebug(L, "Beam invalid pointer.");
	return b;
}

static inline
std::string getString(lua_State *L, int slot = 1)
{
	std::string sr;
	if (lua_isstring(L, slot))
	{
		sr = lua_tostring(L, slot);
	}
	return sr;
}

static inline
const char *getCString(lua_State *L, int slot = 1)
{
	return lua_isstring(L, slot) ? lua_tostring(L, slot) : NULL;
}

static inline
Shot *getShot(lua_State *L, int slot = 1)
{
	Shot *shot = (Shot*)lua_touserdata(L, slot);
	ENSURE_TYPE(shot, SCO_SHOT);
	if (!shot)
		scriptDebug(L, "Shot invalid pointer.");
	return shot;
}

static inline
Web *getWeb(lua_State *L, int slot = 1)
{
	Web *web = (Web*)lua_touserdata(L, slot);
	ENSURE_TYPE(web, SCO_WEB);
	if (!web)
		scriptDebug(L, "Web invalid pointer.");
	return web;
}

static inline
Ingredient *getIng(lua_State *L, int slot = 1)
{
	Ingredient *ing = (Ingredient*)lua_touserdata(L, slot);
	ENSURE_TYPE(ing, SCO_INGREDIENT);
	if (!ing)
		scriptDebug(L, "Ingredient invalid pointer.");
	return ing;
}

static inline
bool getBool(lua_State *L, int slot = 1)
{
	if (lua_isnumber(L, slot))
	{
		return bool(lua_tonumber(L, slot));
	}
	else if (lua_islightuserdata(L, slot))
	{
		return (lua_touserdata(L, slot) != NULL);
	}
	else if (lua_isboolean(L, slot))
	{
		return lua_toboolean(L, slot);
	}
	return false;
}

static inline
Entity *entity(lua_State *L, int slot = 1)
{
	Entity *ent = (Entity*)lua_touserdata(L, slot);
	ENSURE_TYPE(ent, SCO_ENTITY);
	if (!ent)
	{
		scriptDebug(L, "Entity Invalid Pointer");
	}
	return ent;
}

static inline
Vector getVector(lua_State *L, int slot = 1)
{
	Vector v(lua_tonumber(L, slot), lua_tonumber(L, slot+1));
	return v;
}


static inline
Bone *bone(lua_State *L, int slot = 1)
{
	Bone *b = (Bone*)lua_touserdata(L, slot);
	ENSURE_TYPE(b, SCO_BONE);
	if (!b)
	{
		scriptDebug(L, "Bone Invalid Pointer");
	}
	return b;
}

static inline
Path *pathFromName(lua_State *L, int slot = 1)
{
	std::string s = getString(L, slot);
	stringToLower(s);
	Path *p = dsq->game->getPathByName(s);
	if (!p)
	{
		debugLog("Could not find path [" + s + "]");
	}
	return p;
}

static inline
Path *path(lua_State *L, int slot = 1)
{
	Path *p = (Path*)lua_touserdata(L, slot);
	ENSURE_TYPE(p, SCO_PATH);
	return p;
}

static inline
Quad *getQuad(lua_State *L, int slot = 1)
{
	Quad *q = (Quad*)lua_touserdata(L, slot);
	ENSURE_TYPE(q, SCO_QUAD);
	if (!q)
		scriptDebug(L, "Invalid Quad");
	return q;
}

static inline
BaseText *getText(lua_State *L, int slot = 1)
{
	BaseText *q = (BaseText*)lua_touserdata(L, slot);
	ENSURE_TYPE(q, SCO_TEXT);
	if (!q)
		scriptDebug(L, "Invalid Text");
	return q;
}

static inline
Shader *getShader(lua_State *L, int slot = 1)
{
	Shader *q = (Shader*)lua_touserdata(L, slot);
	ENSURE_TYPE(q, SCO_SHADER);
	if (!q)
		scriptDebug(L, "Invalid Shader");
	return q;
}

static SkeletalSprite *getSkeletalSprite(Entity *e)
{
	return e ? &e->skeletalSprite : NULL;
}

static bool looksLikeGlobal(const char *s)
{
	for( ; *s; ++s)
		if( !((*s >= 'A' && *s <= 'Z') || *s == '_' || (*s >= '0' && *s <= '9')) ) // accept any uppercase, number, and _ char
			return false;
	return true;
}

//----------------------------------//

#define luaFunc(func)		static int l_##func(lua_State *L)
#define luaReturnBool(bool)	do {lua_pushboolean(L, (bool)); return 1;} while(0)
#define luaReturnInt(num)	do {lua_pushinteger(L, (num)); return 1;} while(0)
#define luaReturnNum(num)	do {lua_pushnumber(L, (num)); return 1;} while(0)
#define luaReturnPtr(ptr)	do {luaPushPointer(L, (ptr)); return 1;} while(0)
#define luaReturnStr(str)	do {lua_pushstring(L, (str)); return 1;} while(0)
#define luaReturnVec2(x,y)	do {lua_pushnumber(L, (x)); lua_pushnumber(L, (y)); return 2;} while(0)
#define luaReturnVec3(x,y,z)	do {lua_pushnumber(L, (x)); lua_pushnumber(L, (y)); lua_pushnumber(L, (z)); return 3;} while(0)
#define luaReturnNil()		return 0;

// Set the global "v" to the instance's local variable table.  Must be
// called when starting a script.
static void fixupLocalVars(lua_State *L)
{
	lua_getglobal(L, "_threadvars");
	lua_pushlightuserdata(L, L);
	lua_gettable(L, -2);
	lua_remove(L, -2);
	lua_setglobal(L, "v");
}

luaFunc(indexWarnGlobal)
{
	lua_pushvalue(L, -1);
	lua_rawget(L, -3);
	lua_remove(L, -3);

	if (lua_isnil(L, -1))
	{
		// Don't warn on "v" or known interface functions.
		lua_pushvalue(L, -2);
		const char *varname = lua_tostring(L, -1);
		bool doWarn = (strcmp(varname, "v") != 0);
		for (unsigned int i = 0; doWarn && interfaceFunctions[i] != NULL; i++)
		{
			doWarn = (strcmp(varname, interfaceFunctions[i]) != 0);
		}

		if (doWarn)
		{
			std::string s = "WARNING: script tried to get/call undefined global variable ";
			s += varname;
			scriptError(L, s);
		}

		lua_pop(L, 1);
	}

	lua_remove(L, -2);

	return 1;
}

luaFunc(newindexWarnGlobal)
{
	// Don't warn on "v" or known interface functions.
	lua_pushvalue(L, -2);
	const char *varname = lua_tostring(L, -1);
	bool doWarn = (strcmp(varname, "v") != 0) && !looksLikeGlobal(varname);
	for (unsigned int i = 0; doWarn && interfaceFunctions[i] != NULL; i++)
	{
		doWarn = (strcmp(varname, interfaceFunctions[i]) != 0);
	}

	if (doWarn)
	{
		std::ostringstream os;
		os << "WARNING: script set global "
		   << lua_typename(L, lua_type(L, -2))
		   << " " << varname;
		scriptError(L, os.str());
	}

	lua_pop(L, 1);

	// Do the set anyway.
	lua_rawset(L, -3);
	lua_pop(L, 1);
	return 0;
}

luaFunc(indexWarnInstance)
{
	lua_pushvalue(L, -1);
	lua_rawget(L, -3);
	lua_remove(L, -3);
	if (lua_isnil(L, -1))
	{
		std::ostringstream os;
		os << "WARNING: script tried to get/call undefined instance variable "
		   << getString(L, -2);
		scriptError(L, os.str());
	}
	lua_remove(L, -2);

	return 1;
}

luaFunc(panicHandler)
{
	errorLog(luaFormatStackInfo(L) + ": Lua PANIC: " + getString(L, -1));
	exit(1);
}

static bool findFile_helper(const char *rawname, std::string &fname)
{
	if (!rawname)
		return false;
	if (dsq->mod.isActive())
	{
		fname = dsq->mod.getPath();
		if(fname[fname.length() - 1] != '/')
			fname += '/';
		fname += rawname;
		fname = localisePath(fname, dsq->mod.getPath());
		fname = core->adjustFilenameCase(fname);
		if (exists(fname))
			return true;
	}
	fname = localisePath(rawname);
	fname = core->adjustFilenameCase(fname);
	return exists(fname);
}

static int loadFile_helper(lua_State *L, const char *fn)
{
#ifdef BBGE_BUILD_VFS
	VFILE *vf = vfs.GetFile(fn);
	if (!vf)
	{
		lua_pushfstring(L, "cannot open %s", fn);
		return LUA_ERRFILE;
	}
	else
	{
		int result = luaL_loadbuffer(L, (const char*)vf->getBuf(), vf->size(), fn);
		vf->dropBuf(true);
		return result;
	}
#else
	return luaL_loadfile(L, fn);
#endif
}

luaFunc(dofile_caseinsensitive)
{
	// This is Lua's dofile(), with some tweaks.  --ryan.
	std::string fname;
	findFile_helper(luaL_checkstring(L, 1), fname);

	int n = lua_gettop(L);
	if (loadFile_helper(L, fname.c_str()) != 0)
		lua_error(L);
	lua_call(L, 0, LUA_MULTRET);
	return lua_gettop(L) - n;
}

luaFunc(loadfile_caseinsensitive)
{
	// This is Lua's loadfile(), with some tweaks.  --FG.
	std::string fname;
	findFile_helper(luaL_checkstring(L, 1), fname);

	if (loadFile_helper(L, fname.c_str()) == 0)  /* OK? */
		return 1;
	else
	{
		lua_pushnil(L);
		lua_insert(L, -2);  /* put before error message */
		return 2;  /* return nil plus error message */
	}
}



// ----- RenderObject common functions -----

#define forwardCall(func) return l_##func(L);

#define MakeTypeCheckFunc(fname, ty) luaFunc(fname) \
	{ ScriptObject *r = (ScriptObject*)lua_touserdata(L, 1); luaReturnBool(r ? r->isType(ty) : false); }

MakeTypeCheckFunc(isNode, SCO_PATH);
MakeTypeCheckFunc(isObject, SCO_RENDEROBJECT);
MakeTypeCheckFunc(isEntity, SCO_ENTITY)
MakeTypeCheckFunc(isShot, SCO_SHOT)
MakeTypeCheckFunc(isWeb, SCO_WEB)
MakeTypeCheckFunc(isIng, SCO_INGREDIENT)
MakeTypeCheckFunc(isBeam, SCO_BEAM)
MakeTypeCheckFunc(isText, SCO_TEXT)

#undef MakeTypeCheckFunc

// special, because it would return true on almost everything that is RenderObject based.
// Instead, return true only for stuff created with createQuad()
luaFunc(isQuad)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isExactType(ScriptObjectType(SCO_RENDEROBJECT | SCO_QUAD)) : false);
}


luaFunc(obj_setPosition)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->position.stop();
		r->position.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)),
			lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNil();
}

luaFunc(obj_scale)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->scale.stop();
		r->scale.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), 0),
			lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNil();
}

luaFunc(obj_getScale)
{
	RenderObject *r = robj(L);
	Vector s;
	if (r)
		s = r->scale;
	luaReturnVec2(s.x, s.y);
}

luaFunc(obj_alpha)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->alpha.stop();
		r->alpha.interpolateTo(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	luaReturnNil();
}

luaFunc(obj_alphaMod)
{
	RenderObject *r = robj(L);
	if (r)
		r->alphaMod = lua_tonumber(L, 2);
	luaReturnNil()
}

luaFunc(obj_getAlpha)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->alpha.x : 0.0f);
}

luaFunc(obj_color)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->color.stop();
		r->color.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4)),
			lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8));
	}
	luaReturnNil();
}

luaFunc(obj_rotate)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->rotation.stop();
		r->rotation.interpolateTo(Vector(0,0,lua_tonumber(L, 2)),
			lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	}
	luaReturnNil();
}

luaFunc(obj_rotateOffset)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->rotationOffset.stop();
		r->rotationOffset.interpolateTo(Vector(0,0,lua_tonumber(L, 2)),
			lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	}
	luaReturnNil();
}

luaFunc(obj_getRotation)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->rotation.z : 0.0f);
}

luaFunc(obj_getRotationOffset)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->rotationOffset.z : 0.0f);
}

luaFunc(obj_offset)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->offset.stop();
		r->offset.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)),
			lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNil();
}

luaFunc(obj_internalOffset)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->internalOffset.stop();
		r->internalOffset.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)),
			lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNil();
}

luaFunc(obj_getInternalOffset)
{
	RenderObject *r = robj(L);
	Vector io;
	if (r)
		io = r->internalOffset;
	luaReturnVec2(io.x, io.y);
}

luaFunc(obj_getPosition)
{
	float x=0,y=0;
	RenderObject *r = robj(L);
	if (r)
	{
		x = r->position.x;
		y = r->position.y;
	}
	luaReturnVec2(x, y);
}


luaFunc(obj_getOffset)
{
	float x=0,y=0;
	RenderObject *r = robj(L);
	if (r)
	{
		x = r->offset.x;
		y = r->offset.y;
	}
	luaReturnVec2(x, y);
}
luaFunc(obj_x)
{
	RenderObject *r = robj(L);
	float x = 0;
	if (r)
		x = r->position.x;
	luaReturnNum(x);
}

luaFunc(obj_y)
{
	RenderObject *r = robj(L);
	float y = 0;
	if (r)
		y = r->position.y;
	luaReturnNum(y);
}

luaFunc(obj_setBlendType)
{
	RenderObject *r = robj(L);
	if (r)
		r->setBlendType(lua_tointeger(L, 2));
	luaReturnNil();
}

luaFunc(obj_setTexture)
{
	RenderObject *r = robj(L);
	if (r)
		r->setTexture(getString(L, 2));
	luaReturnNil();
}

luaFunc(obj_getTexture)
{
    RenderObject *r = robj(L);
    if (r && r->texture)
        luaReturnStr(r->texture->name.c_str());
    luaReturnStr("");
}

luaFunc(obj_delete)
{
	RenderObject *r = robj(L);
	if (r)
	{
		float time = lua_tonumber(L, 2);
		if (time == 0)
		{
			r->safeKill();
		}
		else
		{
			r->fadeAlphaWithLife = true;
			r->setLife(1);
			r->setDecayRate(1.0f/time);
		}
	}
	luaReturnNil();
}

luaFunc(obj_setLife)
{
	RenderObject *r = robj(L);
	if (r)
		r->setLife(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(obj_getLife)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->life : 0.0f);
}

luaFunc(obj_setDecayRate)
{
	RenderObject *r = robj(L);
	if (r)
		r->setDecayRate(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(obj_addDeathNotify)
{
	RenderObject *r = robj(L);
	RenderObject *which = robj(L, 2);
	if (r && which)
		r->addDeathNotify(which);
	luaReturnNil();
}

luaFunc(obj_addChild)
{
	RenderObject *r = robj(L);
	RenderObject *which = robj(L, 2);
	bool takeOwnership = getBool(L, 3);
	if (r && which)
	{
		if (takeOwnership)
		{
			// HACK: this is ugly, but necessary to prevent double-deletion
			// anyways, dangerous; addChild() may fail, causing a small memory leak (and an error message)
			dsq->getState(dsq->game->name)->removeRenderObjectFromList(which);
			which->setStateDataObject(NULL);
			core->removeRenderObject(which, Core::DO_NOT_DESTROY_RENDER_OBJECT);
			r->addChild(which, PM_POINTER);
		}
		else
			r->addChild(which, PM_STATIC);
	}
	luaReturnNil();
}

luaFunc(obj_setRenderBeforeParent)
{
	RenderObject *r = robj(L);
	if (r)
		r->renderBeforeParent = getBool(L, 2);
	luaReturnNil();
}

// Not so pretty: Because RenderObject has a `velocity' vector,
// and Entity has `vel' ADDITIONALLY, we need to use
// extra functions to manage RenderObject's velocities.
// Different names were chosen to allow avoid name clashing. -- FG

luaFunc(obj_setInternalVel)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->velocity.stop();
		r->velocity.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)),
			lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNil();
}

luaFunc(obj_setInternalVelLen)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->velocity.stop();
		r->velocity.setLength2D(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

luaFunc(obj_getInternalVelLen)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->velocity.getLength2D() : 0.0f);
}


luaFunc(obj_getInternalVel)
{
	Vector v;
	RenderObject *r = robj(L);
	if (r)
		v = r->velocity;
	luaReturnVec2(v.x, v.y);
}

luaFunc(obj_ivelx)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->velocity.x : 0.0f);
}

luaFunc(obj_ively)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->velocity.y : 0.0f);
}

luaFunc(obj_addInternalVel)
{
	RenderObject *r = robj(L);
	if (r)
		r->velocity += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

luaFunc(obj_isInternalVelIn)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->velocity.isLength2DIn(lua_tonumber(L, 2)) : false);
}

luaFunc(obj_getCollideRadius)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->collideRadius : 0);
}

luaFunc(obj_setCollideRadius)
{
	RenderObject *r = robj(L);
	if (r)
		r->collideRadius = lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(obj_getNormal)
{
	Vector v(0, 1);
	RenderObject *r = robj(L);
	if (r)
		v = r->getForward();
	luaReturnVec2(v.x, v.y);
}

luaFunc(obj_getVectorToObj)
{
	RenderObject *r1 = robj(L);
	RenderObject *r2 = robj(L, 2);
	if (r1 && r2)
	{
		Vector diff = r2->position - r1->position;
		luaReturnVec2(diff.x, diff.y);
	}
	else
	{
		luaReturnVec2(0, 0);
	}
}

luaFunc(obj_stopInterpolating)
{
	RenderObject *r = robj(L);
	if (r)
		r->position.stop();
	luaReturnNil();
}

luaFunc(obj_isInterpolating)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->position.isInterpolating() : false);
}

luaFunc(obj_followCamera)
{
	RenderObject *r = robj(L);
	if (r)
		r->followCamera = lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(obj_update)
{
	RenderObject *r = robj(L);
	if (r)
		r->update(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(obj_getWorldPosition)
{
	RenderObject *b = robj(L);
	float x = 0, y = 0;
	if (b)
	{
		Vector v = b->getWorldPosition();
		x = v.x;
		y = v.y;
	}
	luaReturnVec2(x, y);
}

luaFunc(obj_getWorldRotation)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->getWorldRotation() : 0.0f);
}

luaFunc(obj_moveToFront)
{
	RenderObject *r = robj(L);
	if (r)
		r->moveToFront();
	luaReturnNil();
}

luaFunc(obj_moveToBack)
{
	RenderObject *r = robj(L);
	if (r)
		r->moveToBack();
	luaReturnNil();
}

luaFunc(obj_setLayer)
{
	RenderObject *r = robj(L);
	if (r)
		core->switchRenderObjectLayer(r, lua_tointeger(L, 2));
	luaReturnNil();
}

luaFunc(obj_getLayer)
{
	RenderObject *r = robj(L);
	luaReturnInt(r ? r->layer : 0);
}

luaFunc(obj_setRenderPass)
{
	RenderObject *r = robj(L);
	int pass = lua_tointeger(L, 2);
	if (r)
		r->setRenderPass(pass);
	luaReturnNil();
}

luaFunc(obj_fh)
{
	RenderObject *r = robj(L);
	if (r)
		r->flipHorizontal();
	luaReturnNil();
}

luaFunc(obj_fhTo)
{
	RenderObject *r = robj(L);
	bool b = getBool(L);
	if (r)
		r->fhTo(b);
	luaReturnNil();
}

luaFunc(obj_fv)
{
	RenderObject *r = robj(L);
	if (r)
		r->flipVertical();
	luaReturnNil();
}

luaFunc(obj_isfh)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isfh() : false);
}

luaFunc(obj_isfv)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isfv() : false);
}

luaFunc(obj_isfhr)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isfhr() : false);
}

luaFunc(obj_isfvr)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isfvr() : false);
}

luaFunc(obj_damageFlash)
{
	RenderObject *r = robj(L);
	int type = lua_tonumber(L, 2);
	if (r)
	{
		Vector toColor = Vector(1, 0.1, 0.1);
		if (type == 1)
			toColor = Vector(1, 1, 0.1);
		r->color = Vector(1,1,1);
		r->color.interpolateTo(toColor, 0.1, 5, 1);
	}
	luaReturnNil();
}

luaFunc(obj_setCull)
{
	RenderObject *r = robj(L);
	if (r)
		r->cull = getBool(L, 2);
	luaReturnNil();
}

luaFunc(obj_setCullRadius)
{
	RenderObject *r = robj(L);
	if (r)
		r->setOverrideCullRadius(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(obj_isScaling)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->scale.isInterpolating() : false);
}

luaFunc(obj_isRotating)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->rotation.isInterpolating() : false);
}

luaFunc(obj_setUpdateCull)
{
	RenderObject *r = robj(L);;
	if (r)
		r->updateCull = lua_tonumber(L, 2);
	luaReturnNil();
}


luaFunc(obj_setPositionX)
{
	RenderObject *r = robj(L);
	if (r)
		r->position.x = lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(obj_setPositionY)
{
	RenderObject *r = robj(L);
	if (r)
		r->position.y = lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(obj_enableMotionBlur)
{
	RenderObject *r = robj(L);
	if (r)
		r->enableMotionBlur(10, 2);
	luaReturnNil();
}

luaFunc(obj_disableMotionBlur)
{
	RenderObject *r = robj(L);
	if (r)
		r->disableMotionBlur();
	luaReturnNil();
}

luaFunc(obj_collideCircleVsLine)
{
	RenderObject *r = robj(L);
	float x1, y1, x2, y2, sz;
	x1 = lua_tonumber(L, 2);
	y1 = lua_tonumber(L, 3);
	x2 = lua_tonumber(L, 4);
	y2 = lua_tonumber(L, 5);
	sz = lua_tonumber(L, 6);
	bool v = false;
	if (r)
		v = dsq->game->collideCircleVsLine(r, Vector(x1, y1), Vector(x2, y2), sz);
	luaReturnBool(v);
}


luaFunc(obj_collideCircleVsLineAngle)
{
	RenderObject *r = robj(L);
	float angle = lua_tonumber(L, 2);
	float start=lua_tonumber(L, 3), end=lua_tonumber(L, 4), radius=lua_tonumber(L, 5);
	float x=lua_tonumber(L, 6);
	float y=lua_tonumber(L, 7);
	bool v = false;
	if (r)
		v = dsq->game->collideCircleVsLineAngle(r, angle, start, end, radius, Vector(x,y));
	luaReturnBool(v);
}


// ----- end RenderObject common functions -----

// ----- Quad common functions -----

luaFunc(quad_isVisible)
{
	Quad *q = getQuad(L);
	luaReturnBool(q ? q->renderQuad : false);
}

luaFunc(quad_setVisible)
{
	Quad *q = getQuad(L);
	if (q)
		q->renderQuad = getBool(L, 2);
	luaReturnNil();
}

luaFunc(quad_getWidth)
{
	Quad *q = getQuad(L);
	luaReturnNum(q ? q->width : 0.0f);
}

luaFunc(quad_getHeight)
{
	Quad *q = getQuad(L);
	luaReturnNum(q ? q->height : 0.0f);
}

luaFunc(quad_setWidth)
{
	Quad *q = getQuad(L);
	if (q)
		q->setWidth(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(quad_setHeight)
{
	Quad *q = getQuad(L);
	if (q)
		q->setHeight(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(quad_setSegs)
{
	Quad *b = getQuad(L);
	if (b)
		b->setSegs(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tointeger(L, 9));
	luaReturnNil();
}


// --- standard set/get functions for each type, wrapping RenderObject functions ---

#define MK_FUNC(base, getter, prefix, suffix)	\
	luaFunc(prefix##_##suffix) \
	{									\
		typecheckOnly(getter(L));		\
		forwardCall(base##_##suffix);		\
	}

#define MK_ALIAS(prefix, suffix, alias) // not yet used here. defined to avoid warnings

#define RO_FUNC(getter, prefix, suffix) MK_FUNC(obj,  getter, prefix, suffix)
#define Q_FUNC(getter, prefix, suffix)  MK_FUNC(quad, getter, prefix, suffix)

#define MAKE_ROBJ_FUNCS(getter, prefix) \
	RO_FUNC(getter, prefix,  setPosition	) \
	RO_FUNC(getter, prefix,  scale			) \
	RO_FUNC(getter, prefix,  getScale		) \
	RO_FUNC(getter, prefix,  isScaling		) \
	RO_FUNC(getter, prefix,  alpha			) \
	RO_FUNC(getter, prefix,  alphaMod		) \
	RO_FUNC(getter, prefix,  getAlpha		) \
	RO_FUNC(getter, prefix,  color			) \
	RO_FUNC(getter, prefix,  rotate			) \
	RO_FUNC(getter, prefix,  rotateOffset	) \
	RO_FUNC(getter, prefix,  getRotation	) \
	RO_FUNC(getter, prefix,  getRotationOffset) \
	RO_FUNC(getter, prefix,  isRotating		) \
	RO_FUNC(getter, prefix,  offset			) \
	RO_FUNC(getter, prefix,  getOffset		) \
	RO_FUNC(getter, prefix,  internalOffset	) \
	RO_FUNC(getter, prefix,  getInternalOffset) \
	RO_FUNC(getter, prefix,  getPosition	) \
	RO_FUNC(getter, prefix,  getTexture		) \
	RO_FUNC(getter, prefix,  x				) \
	RO_FUNC(getter, prefix,  y				) \
	RO_FUNC(getter, prefix,  setBlendType	) \
	RO_FUNC(getter, prefix,  setTexture		) \
	RO_FUNC(getter, prefix,  delete			) \
	RO_FUNC(getter, prefix,  getLife		) \
	RO_FUNC(getter, prefix,  setLife		) \
	RO_FUNC(getter, prefix,  setDecayRate	) \
	RO_FUNC(getter, prefix,  addDeathNotify	) \
	RO_FUNC(getter, prefix,  setInternalVel	) \
	RO_FUNC(getter, prefix,  setInternalVelLen) \
	RO_FUNC(getter, prefix,  getInternalVel	) \
	RO_FUNC(getter, prefix,  ivelx			) \
	RO_FUNC(getter, prefix,  ively			) \
	RO_FUNC(getter, prefix,  addInternalVel	) \
	RO_FUNC(getter, prefix,  isInternalVelIn) \
	RO_FUNC(getter, prefix,  getInternalVelLen) \
	RO_FUNC(getter, prefix,  getCollideRadius) \
	RO_FUNC(getter, prefix,  setCollideRadius) \
	RO_FUNC(getter, prefix,  getNormal		) \
	RO_FUNC(getter, prefix,  stopInterpolating)\
	RO_FUNC(getter, prefix,  isInterpolating)\
	RO_FUNC(getter, prefix,  followCamera	) \
	RO_FUNC(getter, prefix,  update			) \
	RO_FUNC(getter, prefix,  getWorldPosition) \
	RO_FUNC(getter, prefix,  getWorldRotation) \
	RO_FUNC(getter, prefix,  moveToFront	) \
	RO_FUNC(getter, prefix,  moveToBack		) \
	RO_FUNC(getter, prefix,  setLayer		) \
	RO_FUNC(getter, prefix,  getLayer		) \
	RO_FUNC(getter, prefix,  setRenderBeforeParent) \
	RO_FUNC(getter, prefix,  addChild		) \
	RO_FUNC(getter, prefix,  fh				) \
	RO_FUNC(getter, prefix,  fv				) \
	RO_FUNC(getter, prefix,  fhTo			) \
	RO_FUNC(getter, prefix,  isfh			) \
	RO_FUNC(getter, prefix,  isfv			) \
	RO_FUNC(getter, prefix,  isfhr			) \
	RO_FUNC(getter, prefix,  isfvr			) \
	RO_FUNC(getter, prefix,  damageFlash	) \
	RO_FUNC(getter, prefix,  setCull		) \
	RO_FUNC(getter, prefix,  setCullRadius	) \
	RO_FUNC(getter, prefix,  setUpdateCull	) \
	RO_FUNC(getter, prefix,  setRenderPass	) \
	RO_FUNC(getter, prefix,  setPositionX	) \
	RO_FUNC(getter, prefix,  setPositionY	) \
	RO_FUNC(getter, prefix,  enableMotionBlur	) \
	RO_FUNC(getter, prefix,  disableMotionBlur	) \
	RO_FUNC(getter, prefix,  collideCircleVsLine) \
	RO_FUNC(getter, prefix,  collideCircleVsLineAngle) \
	RO_FUNC(getter, prefix,  getVectorToObj	) \
	MK_ALIAS(prefix, fh, flipHorizontal	) \
	MK_ALIAS(prefix, fv, flipVertical	)

#define MAKE_QUAD_FUNCS(getter, prefix)	\
	MAKE_ROBJ_FUNCS(getter, prefix)		\
	Q_FUNC(getter, prefix,  setVisible		) \
	Q_FUNC(getter, prefix,  isVisible		) \
	Q_FUNC(getter, prefix,  setWidth		) \
	Q_FUNC(getter, prefix,  setHeight		) \
	Q_FUNC(getter, prefix,  getWidth		) \
	Q_FUNC(getter, prefix,  getHeight		) \
	Q_FUNC(getter, prefix,  setSegs			)

// This should reflect the internal class hierarchy,
// e.g. a Beam is a Quad, so it can use quad_* functions
#define EXPAND_FUNC_PROTOTYPES \
	MAKE_QUAD_FUNCS(entity,   entity) \
	MAKE_QUAD_FUNCS(bone,     bone	) \
	MAKE_QUAD_FUNCS(getShot,  shot	) \
	MAKE_QUAD_FUNCS(beam,     beam	) \
	MAKE_ROBJ_FUNCS(getQuad,  quad	) \
	MAKE_ROBJ_FUNCS(getWeb,   web	) \
	MAKE_ROBJ_FUNCS(getText,  text	)

// first time, create them. (There is a second use of this further down, with different MK_* macros)
EXPAND_FUNC_PROTOTYPES


luaFunc(randRange)
{
	int n1 = lua_tointeger(L, 1);
	int n2 = lua_tointeger(L, 2);
	int spread = n2-n1;

	int r = rand()%spread;
	r += n1;

	luaReturnNum(r);
}

luaFunc(upgradeHealth)
{
	dsq->continuity.upgradeHealth();
	luaReturnNil();
}

luaFunc(shakeCamera)
{
	dsq->shakeCamera(lua_tonumber(L,1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(changeForm)
{
	dsq->game->avatar->changeForm((FormType)lua_tointeger(L, 1));
	luaReturnNil();
}

luaFunc(getWaterLevel)
{
	luaReturnNum(dsq->game->getWaterLevel());
}

luaFunc(setPoison)
{
	dsq->continuity.setPoison(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(cureAllStatus)
{
	dsq->continuity.cureAllStatus();
	luaReturnNil();
}

luaFunc(setMusicToPlay)
{
	if (lua_isstring(L, 1))
		dsq->game->setMusicToPlay(getString(L, 1));
	luaReturnNil();
}

luaFunc(setActivePet)
{
	Entity *e = dsq->game->setActivePet(lua_tonumber(L, 1));

	luaReturnPtr(e);
}

luaFunc(setWaterLevel)
{
	dsq->game->waterLevel.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tointeger(L, 3), getBool(L, 4), getBool(L, 5));
	luaReturnNum(dsq->game->waterLevel.x);
}

luaFunc(getForm)
{
	luaReturnNum(dsq->continuity.form);
}

luaFunc(isForm)
{
	FormType form = FormType(lua_tointeger(L, 1));
	bool v = (form == dsq->continuity.form);
	luaReturnBool(v);
}

luaFunc(learnFormUpgrade)
{
	dsq->continuity.learnFormUpgrade((FormUpgradeType)lua_tointeger(L, 1));
	luaReturnNil();
}

luaFunc(hasLi)
{
	luaReturnBool(dsq->continuity.hasLi());
}

luaFunc(hasFormUpgrade)
{
	luaReturnBool(dsq->continuity.hasFormUpgrade((FormUpgradeType)lua_tointeger(L, 1)));
}

// This used to be castSong(), but that name is already taken by an interface function. -- FG
luaFunc(singSong)
{
	dsq->continuity.castSong(lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(isStory)
{
	luaReturnBool(dsq->continuity.isStory(lua_tonumber(L, 1)));
}

luaFunc(getNoteVector)
{
	int note = lua_tointeger(L, 1);
	float mag = lua_tonumber(L, 2);
	Vector v = dsq->getNoteVector(note, mag);
	luaReturnVec2(v.x, v.y);
}

luaFunc(getNoteColor)
{
	int note = lua_tointeger(L, 1);
	Vector v = dsq->getNoteColor(note);
	luaReturnVec3(v.x, v.y, v.z);
}

luaFunc(getRandNote)
{
	//int note = lua_tointeger(L, 1);

	luaReturnNum(dsq->getRandNote());
}

luaFunc(getStory)
{
	luaReturnNum(dsq->continuity.getStory());
}

luaFunc(foundLostMemory)
{
	int num = 0;
	if (dsq->continuity.getFlag(FLAG_SECRET01)) num++;
	if (dsq->continuity.getFlag(FLAG_SECRET02)) num++;
	if (dsq->continuity.getFlag(FLAG_SECRET03)) num++;

	int sbank = 800+(num-1);
	dsq->game->setControlHint(dsq->continuity.stringBank.get(sbank), 0, 0, 0, 4, "13/face");

	dsq->sound->playSfx("memory-found");

	luaReturnNil();
}

luaFunc(setStory)
{
	dsq->continuity.setStory(lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(confirm)
{
	std::string s1 = getString(L, 1);
	std::string s2 = getString(L, 2);
	bool b = dsq->confirm(s1, s2);
	luaReturnBool(b);
}

luaFunc(createWeb)
{
	Web *web = new Web();
	dsq->game->addRenderObject(web, LR_PARTICLES);
	luaReturnPtr(web);
}

// spore has base entity
luaFunc(createSpore)
{
	Vector pos(lua_tonumber(L, 1), lua_tonumber(L, 2));
	if (Spore::isPositionClear(pos))
	{
		Spore *spore = new Spore(pos);
		dsq->game->addRenderObject(spore, LR_ENTITIES);
		luaReturnPtr(spore);
	}
	else
		luaReturnPtr(NULL);
}

luaFunc(web_addPoint)
{
	Web *w = getWeb(L);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	int r = 0;
	if (w)
	{
		r = w->addPoint(Vector(x,y));
	}
	luaReturnInt(r);
}

luaFunc(web_setPoint)
{
	Web *w = getWeb(L);
	int pt = lua_tonumber(L, 2);
	float x = lua_tonumber(L, 3);
	float y = lua_tonumber(L, 4);
	if (w)
	{
		w->setPoint(pt, Vector(x, y));
	}
	luaReturnInt(pt);
}

luaFunc(web_getNumPoints)
{
	Web *web = getWeb(L);
	int num = 0;
	if (web)
	{
		num = web->getNumPoints();
	}
	luaReturnInt(num);
}

luaFunc(getFirstShot)
{
	luaReturnPtr(Shot::getFirstShot());
}

luaFunc(getNextShot)
{
	luaReturnPtr(Shot::getNextShot());
}

luaFunc(shot_getName)
{
	Shot *s = getShot(L);
	luaReturnStr(s ? s->getName() : "");
}

luaFunc(shot_setOut)
{
	Shot *shot = getShot(L);
	float outness = lua_tonumber(L, 2);
	if (shot && shot->firer)
	{
		Vector adjust = shot->velocity;
		adjust.setLength2D(outness);
		shot->position += adjust;
		/*
		std::ostringstream os;
		os << "out(" << adjust.x << ", " << adjust.y << ")";
		debugLog(os.str());
		*/
	}
	luaReturnNil();
}

luaFunc(shot_setAimVector)
{
	Shot *shot = getShot(L);
	float ax = lua_tonumber(L, 2);
	float ay = lua_tonumber(L, 3);
	if (shot)
	{
		shot->setAimVector(Vector(ax, ay));
	}
	luaReturnNil();
}

luaFunc(shot_getEffectTime)
{
	Shot *shot = getShot(L);
	luaReturnNum((shot && shot->shotData) ? shot->shotData->effectTime : 0.0f);
}

luaFunc(shot_isIgnoreShield)
{
	Shot *shot = getShot(L);
	luaReturnBool((shot && shot->shotData) ? shot->shotData->ignoreShield : false);
}

luaFunc(shot_getFirer)
{
	Shot *shot = getShot(L);
	luaReturnPtr(shot ? shot->firer : NULL);
}

luaFunc(shot_setFirer)
{
	Shot *shot = getShot(L);
	if(shot)
	{
		Entity *e = lua_isuserdata(L, 2) ? entity(L, 2) : NULL;
		shot->firer = e;
	}

	luaReturnNil();
}

luaFunc(shot_setTarget)
{
	Shot *shot = getShot(L);
	if(shot)
	{
		Entity *e = lua_isuserdata(L, 2) ? entity(L, 2) : NULL;
		shot->setTarget(e);
	}
	luaReturnNil();
}

luaFunc(shot_getTarget)
{
	Shot *shot = getShot(L);
	luaReturnPtr(shot ? shot->target : NULL);
}

luaFunc(shot_setExtraDamage)
{
	Shot *shot = getShot(L);
	if(shot)
		shot->extraDamage = lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(shot_getExtraDamage)
{
	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->extraDamage : 0.0f);
}

luaFunc(shot_getDamage)
{
	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->getDamage() : 0.0f);
}

luaFunc(shot_getDamageType)
{
	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->getDamageType() : DT_NONE);
}

luaFunc(entity_setVel)
{
	Entity *e = entity(L);
	if (e)
		e->vel = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

luaFunc(entity_setVelLen)
{
	Entity *e = entity(L);
	if (e)
		e->vel.setLength2D(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(entity_getVelLen)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->vel.getLength2D() : 0.0f);
}


luaFunc(entity_getVel)
{
	Vector v;
	Entity *e = entity(L);
	if (e)
		v = e->vel;
	luaReturnVec2(v.x, v.y);
}

luaFunc(entity_velx)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->vel.x : 0.0f);
}

luaFunc(entity_vely)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->vel.y : 0.0f);
}

luaFunc(entity_addVel)
{
	Entity *e = entity(L);
	if (e)
		e->vel += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

luaFunc(entity_addRandomVel)
{
	Entity *e = entity(L);
	float len = lua_tonumber(L, 2);
	Vector add;
	if (e && len)
	{
		int angle = int(rand()%360);
		float a = MathFunctions::toRadians(angle);
		add.x = sinf(a);
		add.y = cosf(a);
		add.setLength2D(len);
		e->vel += add;
	}
	luaReturnVec2(add.x, add.y);
}

luaFunc(entity_isVelIn)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->vel.isLength2DIn(lua_tonumber(L, 2)) : false);

}

luaFunc(entity_clearVel)
{
	Entity *e = entity(L);
	if (e)
		e->vel = Vector(0,0,0);
	luaReturnNil();
}
// end extra Entity::vel functions

luaFunc(entity_addIgnoreShotDamageType)
{
	Entity *e = entity(L);
	if (e)
	{
		e->addIgnoreShotDamageType((DamageType)lua_tointeger(L, 2));
	}
	luaReturnNil();
}

luaFunc(entity_warpLastPosition)
{
	Entity *e = entity(L);
	if (e)
	{
		e->warpLastPosition();
	}
	luaReturnNil();
}


luaFunc(entity_velTowards)
{
	Entity *e = entity(L);
	int x = lua_tonumber(L, 2);
	int y = lua_tonumber(L, 3);
	int velLen = lua_tonumber(L, 4);
	int range = lua_tonumber(L, 5);
	if (e)
	{
		Vector pos(x,y);
		if (range==0 || ((pos - e->position).getLength2D() < range))
		{
			Vector add = pos - e->position;
			add.setLength2D(velLen);
			e->vel2 += add;
		}
	}
	luaReturnNil();
}

luaFunc(entity_getBoneLockEntity)
{
	Entity *e = entity(L);
	Entity *ent = NULL;
	if (e)
	{
		BoneLock *b = e->getBoneLock();
		ent = b->entity;
		//ent = e->boneLock.entity;
	}
	luaReturnPtr(ent);
}

luaFunc(entity_ensureLimit)
{
	Entity *e = entity(L);
	dsq->game->ensureLimit(e, lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

luaFunc(entity_setRidingPosition)
{
	Entity *e = entity(L);
	if (e)
		e->setRidingPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	luaReturnNil();
}

luaFunc(entity_setRidingData)
{
	Entity *e = entity(L);
	if (e)
		e->setRidingData(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), getBool(L, 5));
	luaReturnNil();
}

luaFunc(entity_setBoneLock)
{
	Entity *e = entity(L);
	bool ret = false;
	if (e)
	{
		BoneLock bl;
		if (lua_isuserdata(L, 2))
		{
			Entity *e2 = entity(L, 2);
			Bone *b = 0;
			if (lua_isuserdata(L, 3))
				b = bone(L, 3);
			
			bl.entity = e2;
			bl.bone = b;
			bl.on = true;
			bl.collisionMaskIndex = dsq->game->lastCollideMaskIndex;
		}
		ret = e->setBoneLock(bl);
	}
	luaReturnBool(ret);
}

luaFunc(entity_setIngredient)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setIngredientData(getString(L,2));
	}
	luaReturnNil();
}

luaFunc(entity_setSegsMaxDist)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->setMaxDist(lua_tonumber(L, 2));

	luaReturnNil();
}

luaFunc(entity_setBounceType)
{
	Entity *e = entity(L);
	int v = lua_tointeger(L, 2);
	if (e)
	{
		e->setBounceType((BounceType)v);
	}
	luaReturnNum(v);
}


luaFunc(user_set_demo_intro)
{
#ifndef AQUARIA_DEMO
	dsq->user.demo.intro = lua_tonumber(L, 1);
#endif
	luaReturnNil();
}

luaFunc(user_save)
{
	dsq->user.save();
	luaReturnNil();
}

luaFunc(entity_setAutoSkeletalUpdate)
{
	ScriptedEntity *e = scriptedEntity(L);
	bool v = getBool(L, 2);
	if (e)
		e->setAutoSkeletalUpdate(v);
	luaReturnBool(v);
}

luaFunc(entity_getBounceType)
{
	Entity *e = entity(L);
	BounceType bt=BOUNCE_SIMPLE;
	if (e)
	{
		bt = (BounceType)e->getBounceType();
	}
	luaReturnInt((int)bt);
}

luaFunc(entity_setDieTimer)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDieTimer(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

luaFunc(entity_setLookAtPoint)
{
	Entity *e = entity(L);
	if (e)
	{
		e->lookAtPoint = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnNil();
}

luaFunc(entity_getLookAtPoint)
{
	Entity *e = entity(L);
	Vector pos;
	if (e)
	{
		pos = e->getLookAtPoint();
	}
	luaReturnVec2(pos.x, pos.y);
}

luaFunc(entity_setRiding)
{
	Entity *e = entity(L);
	Entity *e2 = 0;
	if (lua_touserdata(L, 2) != NULL)
		e2 = entity(L, 2);
	if (e)
	{
		e->setRiding(e2);
	}
	luaReturnNil();
}

luaFunc(entity_getHealthPerc)
{
	Entity *e = entity(L);
	float p = 0;
	if (e)
	{
		p = e->getHealthPerc();
	}
	luaReturnNum(p);
}

luaFunc(entity_getRiding)
{
	Entity *e = entity(L);
	Entity *ret = 0;
	if (e)
		ret = e->getRiding();
	luaReturnPtr(ret);
}

luaFunc(entity_setTargetPriority)
{
	Entity *e = entity(L);
	if (e)
	{
		e->targetPriority = lua_tonumber(L, 2);
	}
	luaReturnNil();
}

luaFunc(isQuitFlag)
{
	luaReturnBool(dsq->isQuitFlag());
}

luaFunc(isDeveloperKeys)
{
	luaReturnBool(dsq->isDeveloperKeys());
}

luaFunc(isDemo)
{
#ifdef AQUARIA_DEMO
	luaReturnBool(true);
#else
	luaReturnBool(false);
#endif
}

luaFunc(isWithin)
{
	Vector v1 = getVector(L, 1);
	Vector v2 = getVector(L, 3);
	int dist = lua_tonumber(L, 5);
	/*
	std::ostringstream os;
	os << "v1(" << v1.x << ", " << v1.y << ") v2(" << v2.x << ", " << v2.y << ")";
	debugLog(os.str());
	*/
	Vector d = v2-v1;
	bool v = false;
	if (d.isLength2DIn(dist))
	{
		v = true;
	}
	luaReturnBool(v);
}

luaFunc(toggleDamageSprite)
{
	dsq->game->toggleDamageSprite(getBool(L));
	luaReturnNil();
}

luaFunc(toggleCursor)
{
	dsq->toggleCursor(getBool(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(toggleBlackBars)
{
	dsq->toggleBlackBars(getBool(L, 1));
	luaReturnNil();
}

luaFunc(setBlackBarsColor)
{
	Vector c(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
	dsq->setBlackBarsColor(c);
	luaReturnNil();
}

luaFunc(toggleLiCombat)
{
	dsq->continuity.toggleLiCombat(getBool(L));
	luaReturnNil();
}

luaFunc(getNoteName)
{
	luaReturnStr(dsq->game->getNoteName(lua_tonumber(L, 1), getString(L, 2)).c_str());
}

luaFunc(getWorldType)
{
	luaReturnNum((int)dsq->continuity.getWorldType());
}

luaFunc(setWorldType)
{
	WorldType wt = (WorldType)lua_tointeger(L, 1);
	bool trans = getBool(L, 2);
	dsq->continuity.applyWorldEffects(wt, trans, 1); // last arg is not used
	luaReturnNil();
}

luaFunc(isWorldPaused)
{
	luaReturnBool(dsq->game->isWorldPaused());
}

luaFunc(setWorldPaused)
{
	dsq->game->setWorldPaused(getBool(L, 1));
	luaReturnNil();
}

luaFunc(getNearestNodeByType)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	int type = lua_tonumber(L, 3);

	luaReturnPtr(dsq->game->getNearestPath(Vector(x,y), (PathType)type));
}

luaFunc(fadeOutMusic)
{
	dsq->sound->fadeMusic(SFT_OUT, lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(getNode)
{
	luaReturnPtr(pathFromName(L));
}

luaFunc(getNodeToActivate)
{
	luaReturnPtr(dsq->game->avatar->pathToActivate);
}

luaFunc(setNodeToActivate)
{
	dsq->game->avatar->pathToActivate = path(L, 1);
	luaReturnNil();
}

luaFunc(setActivation)
{
	dsq->game->activation = getBool(L, 1);
	luaReturnNil();
}

luaFunc(debugLog)
{
	const char *s = lua_tostring(L, 1);
	if(s)
		debugLog(s);
	luaReturnStr(s);
}

luaFunc(errorLog)
{
	const char *s = lua_tostring(L, 1);
	if(s)
		errorLog(s);
	luaReturnStr(s);
}

luaFunc(reconstructGrid)
{
	dsq->game->reconstructGrid(true);
	luaReturnNil();
}

luaFunc(reconstructEntityGrid)
{
	dsq->game->reconstructEntityGrid();
	luaReturnNil();
}

luaFunc(entity_setCanLeaveWater)
{
	Entity *e = entity(L);
	bool v = getBool(L, 2);
	if (e)
	{
		e->setCanLeaveWater(v);
	}
	luaReturnNil();
}

luaFunc(entity_setSegmentTexture)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		RenderObject *ro = e->getSegment(lua_tonumber(L, 2));
		if (ro)
		{
			ro->setTexture(getString(L, 3));
		}
	}
	luaReturnNil();
}

luaFunc(entity_findNearestEntityOfType)
{
	Entity *e = entity(L);
	Entity *nearest = 0;
	if (e)
	{
		EntityType et = (EntityType)lua_tointeger(L, 2);
		int maxRange = lua_tointeger(L, 3);
		float smallestDist = HUGE_VALF;
		Entity *closest = 0;
		FOR_ENTITIES(i)
		{
			Entity *ee = *i;
			if (ee != e)
			{
				float dist = (ee->position - e->position).getSquaredLength2D();
				if (ee->health > 0 && !ee->isEntityDead() && ee->getEntityType() == et && dist < smallestDist)
				{
					smallestDist = dist;
					closest = ee;
				}
			}
		}
		if (maxRange == 0 || smallestDist <= sqr(maxRange))
		{
			nearest = closest;
		}
	}
	luaReturnPtr(nearest);
}

luaFunc(createShot)
{
	Entity *e = entity(L,2);
	Entity *t = 0;
	if (lua_touserdata(L, 3) != NULL)
		t = entity(L,3);
	Shot *s = 0;
	Vector pos, aim;
	pos.x = lua_tonumber(L, 4);
	pos.y = lua_tonumber(L, 5);
	aim.x = lua_tonumber(L, 6);
	aim.y = lua_tonumber(L, 7);


	s = dsq->game->fireShot(getString(L, 1), e, t, pos, aim);

	luaReturnPtr(s);
}

// deprecated, use entity_playSfx
luaFunc(entity_sound)
{
	Entity *e = entity(L);
	if (e && !dsq->isSkippingCutscene())
	{
		float freq = lua_tonumber(L, 3);
		// HACK: most old scripts still use a freq value of ~1000 as normal pitch.
		// Pitch values this high produce a barely audible click only,
		// so a cheap hack like this fixes it without changing older scripts. -- FG
		if (freq >= 100)
			freq *= 0.001f;
		e->sound(getString(L, 2), freq, lua_tonumber(L, 4));
	}
	luaReturnNil();
}

luaFunc(entity_playSfx)
{
	Entity *e = entity(L);
	void *h = NULL;
	if (e && !dsq->isSkippingCutscene())
	{
		PlaySfx sfx = dsq->calcPositionalSfx(e->position, lua_tonumber(L, 7));
		sfx.name = getString(L, 2);
		sfx.freq = lua_tonumber(L, 3);
		float vol = lua_tonumber(L, 4);
		sfx.loops = lua_tonumber(L, 5);
		float fadeOut = lua_tonumber(L, 6);
		if(vol > 0)
			sfx.vol *= vol;

		// FIXME: See comment in DSQ::playPositionalSfx() -- FG
		if (sfx.vol <= 0)
			luaReturnPtr(NULL);

		h = core->sound->playSfx(sfx);
		if (fadeOut != 0)
		{
			sound->fadeSfx(h, SFT_OUT, fadeOut);
		}
	}
	luaReturnPtr(h);
}

luaFunc(entity_setSpiritFreeze)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setSpiritFreeze(getBool(L,2));
	}
	luaReturnNil();
}

luaFunc(node_setSpiritFreeze)
{
	Path *e = path(L);
	if (e)
		e->spiritFreeze = getBool(L,2);
	luaReturnNil();
}

luaFunc(entity_setFillGrid)
{
	Entity *e = entity(L);
	bool b = getBool(L,2);
	if (e)
	{
		e->fillGridFromQuad = b;
	}
	luaReturnNil();
}

luaFunc(entity_isFillGrid)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->fillGridFromQuad : false);
}

luaFunc(entity_getAimVector)
{
	Entity *e = entity(L);
	Vector aim;
	float adjust = lua_tonumber(L, 2);
	float len = lua_tonumber(L, 3);
	bool flip = getBool(L, 4);
	if (e)
	{
		float a = e->rotation.z;
		if (!flip)
			a += adjust;
		else
		{
			if (e->isfh())
			{
				a -= adjust;
			}
			else
			{
				a += adjust;
			}
		}
		a = MathFunctions::toRadians(a);
		aim = Vector(sinf(a)*len, cosf(a)*len);
	}
	luaReturnVec2(aim.x, aim.y);
}

luaFunc(entity_getVectorToEntity)
{
	typecheckOnly(entity(L));
	typecheckOnly(entity(L, 2));
	forwardCall(obj_getVectorToObj);
}

luaFunc(entity_setDropChance)
{
	Entity *e = entity(L);
	if (e)
	{
		e->dropChance = lua_tonumber(L, 2);
		int amount = lua_tonumber(L, 3);
		ScriptedEntity *se = dynamic_cast<ScriptedEntity*>(e);
		if (se && amount)
		{
			se->manaBallAmount = amount;
		}
	}
	luaReturnNil();
}

luaFunc(entity_warpToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->position.stopPath();
		e->position = p->nodes[0].position;
		e->rotateToVec(Vector(0,-1), 0.1);
	}
	luaReturnNil();
}

luaFunc(entity_stopPull)
{
	Entity *e = entity(L);
	if (e)
		e->stopPull();
	luaReturnNil();
}

luaFunc(entity_moveToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->moveToNode(p, lua_tointeger(L, 3), lua_tointeger(L, 4), 0);
	}
	luaReturnNil();
}

luaFunc(entity_swimToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->moveToNode(p, lua_tointeger(L, 3), lua_tointeger(L, 4), 1);
		/*
		ScriptedEntity *se = dynamic_cast<ScriptedEntity*>(e);
		se->swimPath = true;
		*/
	}
	luaReturnNil();
}

luaFunc(entity_swimToPosition)
{
	Entity *e = entity(L);
	//Path *p = path(L, 2);
	Path p;
	PathNode n;
	n.position = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	p.nodes.push_back(n);
	if (e)
	{
		e->moveToNode(&p, lua_tointeger(L, 4), lua_tointeger(L, 5), 1);
		/*
		ScriptedEntity *se = dynamic_cast<ScriptedEntity*>(e);
		se->swimPath = true;
		*/
	}
	luaReturnNil();
}


luaFunc(avatar_setCanDie)
{
	dsq->game->avatar->canDie = getBool(L, 1);

	luaReturnNil();
}

// not naming this avatar_* because it rather belongs into the UI category...
luaFunc(setCanActivate)
{
	dsq->game->avatar->setCanActivateStuff(getBool(L, 1));
	luaReturnNil();
}

luaFunc(setSeeMapMode)
{
	dsq->game->avatar->setSeeMapMode((SeeMapMode)lua_tointeger(L, 1));
	luaReturnNil();
}

luaFunc(avatar_setCanBurst)
{
	dsq->game->avatar->setCanBurst(getBool(L, 1));
	luaReturnNil();
}

luaFunc(avatar_canBurst)
{
	luaReturnBool(dsq->game->avatar->canBurst());
}

luaFunc(avatar_setCanLockToWall)
{
	dsq->game->avatar->setCanBurst(getBool(L, 1));
	luaReturnNil();
}

luaFunc(avatar_canLockToWall)
{
	luaReturnBool(dsq->game->avatar->canBurst());
}

luaFunc(avatar_setCanSwimAgainstCurrents)
{
	dsq->game->avatar->setCanSwimAgainstCurrents(getBool(L, 1));
	luaReturnNil();
}

luaFunc(avatar_canSwimAgainstCurrents)
{
	luaReturnBool(dsq->game->avatar->canSwimAgainstCurrents());
}

luaFunc(avatar_setCanCollideWithShots)
{
	dsq->game->avatar->setCollideWithShots(getBool(L, 1));
	luaReturnNil();
}

luaFunc(avatar_canCollideWithShots)
{
	luaReturnBool(dsq->game->avatar->canCollideWithShots());
}

luaFunc(avatar_setCollisionAvoidanceData)
{
	dsq->game->avatar->setCollisionAvoidanceData(lua_tointeger(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(avatar_toggleCape)
{
	dsq->game->avatar->toggleCape(getBool(L,1));

	luaReturnNil();
}

luaFunc(avatar_setBlockSinging)
{
	bool b = getBool(L);
	dsq->game->avatar->setBlockSinging(b);
	luaReturnNil();
}

luaFunc(avatar_isBlockSinging)
{
	luaReturnBool(dsq->game->avatar->isBlockSinging());
}

luaFunc(avatar_fallOffWall)
{
	dsq->game->avatar->fallOffWall();
	luaReturnNil();
}

luaFunc(avatar_isBursting)
{
	luaReturnBool(dsq->game->avatar->bursting);
}

luaFunc(avatar_isLockable)
{
	luaReturnBool(dsq->game->avatar->isLockable());
}

luaFunc(avatar_isRolling)
{
	luaReturnBool(dsq->game->avatar->isRolling());
}

luaFunc(avatar_isOnWall)
{
	bool v = dsq->game->avatar->state.lockedToWall;
	luaReturnBool(v);
}

luaFunc(avatar_isShieldActive)
{
	bool v = (dsq->game->avatar->activeAura == AURA_SHIELD);
	luaReturnBool(v);
}

luaFunc(avatar_setShieldActive)
{
	bool on = getBool(L, 1);
	if (on)
		dsq->game->avatar->activateAura(AURA_SHIELD);
	else
		dsq->game->avatar->stopAura();
	luaReturnNil();
}

luaFunc(avatar_getStillTimer)
{
	luaReturnNum(dsq->game->avatar->stillTimer.getValue());
}

luaFunc(avatar_getRollDirection)
{
	int v = 0;
	if (dsq->game->avatar->isRolling())
		v = dsq->game->avatar->rollDir;
	luaReturnNum(v);
}

luaFunc(avatar_getSpellCharge)
{
	luaReturnNum(dsq->game->avatar->state.spellCharge);
}

luaFunc(jumpState)
{
	dsq->enqueueJumpState(getString(L, 1), getBool(L, 2));
	luaReturnNil();
}

luaFunc(goToTitle)
{
	dsq->title();
	luaReturnNil();
}

luaFunc(getEnqueuedState)
{
	luaReturnStr(dsq->getEnqueuedJumpState().c_str());
}

luaFunc(learnSong)
{
	dsq->continuity.learnSong(lua_tointeger(L, 1));
	luaReturnNil();
}

luaFunc(unlearnSong)
{
	dsq->continuity.unlearnSong(lua_tointeger(L, 1));
	luaReturnNil();
}

luaFunc(showInGameMenu)
{
	dsq->game->showInGameMenu(getBool(L, 1), getBool(L, 2), (MenuPage)lua_tointeger(L, 3));
	luaReturnNil();
}

luaFunc(hideInGameMenu)
{
	dsq->game->hideInGameMenu();
	luaReturnNil();
}

luaFunc(showImage)
{
	dsq->game->showImage(getString(L));
	luaReturnNil();
}

luaFunc(hideImage)
{
	dsq->game->hideImage();
	luaReturnNil();
}

luaFunc(hasSong)
{
	bool b = dsq->continuity.hasSong(lua_tointeger(L, 1));
	luaReturnBool(b);
}

luaFunc(loadSound)
{
	void *handle = core->sound->loadLocalSound(getString(L, 1));
	luaReturnPtr(handle);
}

luaFunc(loadMap)
{
	std::string s = getString(L, 1);
	std::string n = getString(L, 2);

	if (!s.empty())
	{
		if (!n.empty())
		{
			if (dsq->game->avatar)
				dsq->game->avatar->disableInput();
			dsq->game->warpToSceneNode(s, n);
		}
		else
		{
			if (dsq->game->avatar)
				dsq->game->avatar->disableInput();
			dsq->game->transitionToScene(s);
		}
	}
	luaReturnNil();
}

luaFunc(entity_followPath)
{
	Entity *e = entity(L);
	if (e)
	{
		Path *p = path(L, 2);
		int speedType = lua_tonumber(L, 3);
		int dir = lua_tonumber(L, 4);

		e->followPath(p, speedType, dir);
	}
	luaReturnNil();
}

luaFunc(getIngredientGfx)
{
	luaReturnStr(dsq->continuity.getIngredientGfx(getString(L, 1)).c_str());
}

luaFunc(spawnIngredient)
{
	int times = lua_tonumber(L, 4);
	if (times == 0) times = 1;
	bool out = getBool(L, 5);
	Entity *e = dsq->game->spawnIngredient(getString(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), times, out);

	luaReturnPtr(e);
}

luaFunc(getNearestIngredient)
{
	Ingredient *i = dsq->game->getNearestIngredient(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)), lua_tonumber(L, 3));
	luaReturnPtr(i);
}

luaFunc(spawnAllIngredients)
{
	dsq->spawnAllIngredients(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)));
	luaReturnNil();
}

luaFunc(spawnParticleEffect)
{
	float t = lua_tonumber(L, 4);
	// having t and rot reversed compared to the DSQ function is intentional
	float rot = lua_tonumber(L, 5);
	int layer = lua_tonumber(L, 6);
	if (!layer)
		layer = LR_PARTICLES;
	float follow = lua_tonumber(L, 7);
	ParticleEffect *pe = dsq->spawnParticleEffect(getString(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)),
		rot, t, layer, follow);
	luaReturnPtr(pe);
}

luaFunc(bone_showFrame)
{
	Bone *b = bone(L);
	if (b)
		b->showFrame(lua_tointeger(L, 2));
	luaReturnNil();
}

luaFunc(bone_setSegmentOffset)
{
	Bone *b = bone(L);
	if (b)
		b->segmentOffset = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

luaFunc(bone_setSegmentProps)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setSegmentProps(lua_tonumber(L, 2), lua_tonumber(L, 3), getBool(L, 4));
	}
	luaReturnNil();
}

luaFunc(bone_setSegmentChainHead)
{
	Bone *b = bone(L);
	if (b)
	{
		if (getBool(L, 2))
			b->segmentChain = 1;
		else
			b->segmentChain = 0;
	}
	luaReturnNil();
}

luaFunc(bone_addSegment)
{
	Bone *b = bone(L);
	Bone *b2 = bone(L, 2);
	if (b && b2)
		b->addSegment(b2);
	luaReturnNil();
}

luaFunc(bone_setAnimated)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setAnimated(lua_tointeger(L, 2));
	}
	luaReturnNil();
}

luaFunc(bone_lookAtEntity)
{
	Bone *b = bone(L);
	Entity *e = entity(L, 2);
	if (b && e)
	{
		Vector pos = e->position;
		if (e->getEntityType() == ET_AVATAR)
		{
			pos = e->skeletalSprite.getBoneByIdx(1)->getWorldPosition();
		}
		b->lookAt(pos, lua_tonumber(L, 3), lua_tonumber(L, 4),lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	luaReturnNil();
}

luaFunc(bone_lookAtPosition)
{
	Bone *b = bone(L);
	if (b)
	{
		b->lookAt(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNil();
}

luaFunc(entity_resetTimer)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->resetTimer(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(entity_stopFollowingPath)
{
	Entity *e = entity(L);
	if (e)
	{
		if (e->isFollowingPath())
		{
			e->stopFollowingPath();
		}
	}
	luaReturnNil();
}

luaFunc(entity_slowToStopPath)
{
	Entity *e = entity(L);
	if (e)
	{
		if (e->isFollowingPath())
		{
			debugLog("calling slow to stop path");
			e->slowToStopPath(lua_tonumber(L, 2));
		}
		else
		{
			debugLog("wasn't following path");
		}
	}
	luaReturnNil();
}

luaFunc(entity_stopTimer)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->stopTimer();
	luaReturnNil();
}

luaFunc(entity_createEntity)
{
	Entity *e = entity(L);
	Entity *ret = NULL;
	if (e)
		ret = dsq->game->createEntity(dsq->getEntityTypeIndexByName(getString(L, 2)), 0, e->position, 0, false, "", ET_ENEMY, true);
	luaReturnPtr(ret);
}

luaFunc(entity_checkSplash)
{
	Entity *e = entity(L);
	bool ret = false;
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	if (e)
		ret = e->checkSplash(Vector(x,y));
	luaReturnBool(ret);
}


luaFunc(entity_isUnderWater)
{
	Entity *e = entity(L);
	bool b = false;
	if (e)
	{
		b = e->isUnderWater();
	}
	luaReturnBool(b);
}

luaFunc(entity_isBeingPulled)
{
	Entity *e = entity(L);
	bool v= false;
	if (e)
		v = (dsq->game->avatar->pullTarget == e);
	luaReturnBool(v);
}

luaFunc(avatar_setPullTarget)
{
	Entity *e = 0;
	if (lua_isuserdata(L, 1))
		e = entity(L, 1);

	if (dsq->game->avatar->pullTarget != 0)
		dsq->game->avatar->pullTarget->stopPull();

	dsq->game->avatar->pullTarget = e;

	if (e)
		e->startPull();

	luaReturnNil();
}

luaFunc(entity_isDead)
{
	Entity *e = entity(L);
	bool v= false;
	if (e)
	{
		v = e->isEntityDead();
	}
	luaReturnBool(v);
}


luaFunc(getLastCollidePosition)
{
	luaReturnVec2(dsq->game->lastCollidePosition.x, dsq->game->lastCollidePosition.y);
}

luaFunc(getLastCollideTileType)
{
	luaReturnInt(dsq->game->lastCollideTileType);
}

luaFunc(collideCircleWithGrid)
{
	bool c = dsq->game->collideCircleWithGrid(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)), lua_tonumber(L, 3));
	luaReturnBool(c);
}


luaFunc(entity_isNearGround)
{
	Entity *e = entity(L);
	int sampleArea = 0;
	bool value = false;
	if (e)
	{
		if (lua_isnumber(L, 2))
			sampleArea = int(lua_tonumber(L, 2));
		Vector v = dsq->game->getWallNormal(e->position, sampleArea);
		if (!v.isZero())
		{
			//if (v.y < -0.5f && fabsf(v.x) < 0.4f)
			if (v.y < 0 && fabsf(v.x) < 0.6f)
			{
				value = true;
			}
		}
		/*
		Vector v = e->position + Vector(0,e->collideRadius + TILE_SIZE/2);
		std::ostringstream os;
		os << "checking (" << v.x << ", " << v.y << ")";
		debugLog(os.str());
		TileVector t(v);
		TileVector c;
		for (int i = -5; i < 5; i++)
		{
			c.x = t.x+i;
			c.y = t.y;
			if (dsq->game->isObstructed(t))
			{
				value = true;
			}
		}
		*/
	}
	luaReturnBool(value);
}

luaFunc(entity_isHit)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
		v = e->isHit();
	luaReturnBool(v);
}

luaFunc(entity_waitForPath)
{
	Entity *e = entity(L);
	while (e && e->isFollowingPath())
	{
		core->main(FRAME_TIME);
	}
	luaReturnNil();
}

luaFunc(quitNestedMain)
{
	core->quitNestedMain();
	luaReturnNil();
}

luaFunc(isNestedMain)
{
	luaReturnBool(core->isNested());
}

luaFunc(entity_watchForPath)
{
	dsq->game->avatar->disableInput();

	Entity *e = entity(L);
	while (e && e->isFollowingPath())
	{
		core->main(FRAME_TIME);
	}

	dsq->game->avatar->enableInput();
	luaReturnNil();
}


luaFunc(watchForVoice)
{
	int quit = lua_tointeger(L, 1);
	while (dsq->sound->isPlayingVoice())
	{
		dsq->watch(FRAME_TIME, quit);
		if (quit && dsq->isQuitFlag())
		{
			dsq->sound->stopVoice();
			break;
		}
	}
	luaReturnNil();
}


luaFunc(entity_isSlowingToStopPath)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->isSlowingToStopPath();
	}
	luaReturnBool(v);
}

luaFunc(entity_resumePath)
{
	Entity *e = entity(L);
	if (e)
	{
		e->position.resumePath();
	}
	luaReturnNil();
}

luaFunc(entity_isAnimating)
{
	Entity *e = entity(L);
	bool v= false;
	if (e)
	{
		v = e->skeletalSprite.isAnimating(lua_tonumber(L, 2));
	}
	luaReturnBool(v);
}


luaFunc(entity_getAnimationName)
{
	Entity *e = entity(L);
	const char *ret = "";
	int layer = lua_tonumber(L, 2);
	if (e)
	{
		if (Animation *anim = e->skeletalSprite.getCurrentAnimation(layer))
		{
			ret = anim->name.c_str();
		}
	}
	luaReturnStr(ret);
}

luaFunc(entity_getAnimationLength)
{
	Entity *e = entity(L);
	float ret=0;
	int layer = lua_tonumber(L, 2);
	if (e)
	{
		if (Animation *anim = e->skeletalSprite.getCurrentAnimation(layer))
		{
			ret = anim->getAnimationLength();
		}
	}
	luaReturnNum(ret);
}

luaFunc(entity_isFollowingPath)
{
	Entity *e = entity(L);
	if (e)
		luaReturnBool(e->isFollowingPath());
	else
		luaReturnBool(false);
}

luaFunc(entity_toggleBone)
{
	Entity *e = entity(L);
	Bone *b = bone(L, 2);
	if (e && b)
	{
		e->skeletalSprite.toggleBone(e->skeletalSprite.getBoneIdx(b), lua_tonumber(L, 3));
	}
	luaReturnNil();
}

luaFunc(entity_setEntityType)
{
	Entity *e = entity(L);
	if (e)
		e->setEntityType(EntityType(lua_tointeger(L, 2)));
	luaReturnNil();
}

luaFunc(entity_getEntityType)
{
	Entity *e = entity(L);
	if (e)
		luaReturnInt(int(e->getEntityType()));
	else
		luaReturnInt(0);
}

luaFunc(cam_snap)
{
	dsq->game->snapCam();
	luaReturnNil();
}

luaFunc(cam_toNode)
{
	Path *p = path(L);
	if (p)
	{
		dsq->game->setCameraFollow(&p->nodes[0].position);
	}
	luaReturnNil();
}

luaFunc(cam_toEntity)
{
	if (lua_touserdata(L, 1) == NULL)
	{
		Vector *pos = 0;
		dsq->game->setCameraFollow(pos);
	}
	else
	{
		Entity *e = entity(L);
		if (e)
		{
			dsq->game->setCameraFollowEntity(e);
		}
	}
	luaReturnNil();
}

luaFunc(cam_setPosition)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float time = lua_tonumber(L, 3);
	int loopType = lua_tointeger(L, 4);
	int pingPong = lua_tointeger(L, 5);
	int ease = lua_tointeger(L, 6);

	Vector p = dsq->game->getCameraPositionFor(Vector(x,y));

	dsq->game->cameraInterp.stop();
	dsq->game->cameraInterp.interpolateTo(p, time, loopType, pingPong, ease);

	dsq->cameraPos = p;
	luaReturnNil();
}


luaFunc(entity_spawnParticlesFromCollisionMask)
{
	Entity *e = entity(L);
	if (e)
	{
		int intv = lua_tonumber(L, 3);
		if (intv <= 0)
			intv = 1;
		e->spawnParticlesFromCollisionMask(getString(L, 2), intv);
	}

	luaReturnNil();
}

luaFunc(entity_initEmitter)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	std::string pfile = getString(L, 3);
	if (se)
	{
		se->initEmitter(e, pfile);
	}
	luaReturnNil();
}

luaFunc(entity_startEmitter)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	if (se)
	{
		se->startEmitter(e);
	}
	luaReturnNil();
}

luaFunc(entity_stopEmitter)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	if (se)
	{
		se->stopEmitter(e);
	}
	luaReturnNil();
}

luaFunc(entity_initStrands)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		e->initStrands(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), Vector(lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8)));
	}
	luaReturnNil();
}

luaFunc(entity_initSkeletal)
{
	Entity *e = entity(L);
	if (e)
	{
		e->renderQuad = false;
		e->setWidthHeight(128, 128);
		e->skeletalSprite.loadSkeletal(getString(L, 2));
		const char *s = lua_tostring(L, 3);
		if (s && *s)
			e->skeletalSprite.loadSkin(s);
	}
	luaReturnNil();
}

luaFunc(entity_loadSkin)
{
	Entity *e = entity(L);
	if (e && e->skeletalSprite.isLoaded())
	{
		const char *s = lua_tostring(L, 2);
		if (s && *s)
			e->skeletalSprite.loadSkin(s);
	}
	luaReturnNil();
}

luaFunc(entity_getSkeletalName)
{
	Entity *e = entity(L);
	const char *s = "";
	if (e && e->skeletalSprite.isLoaded())
		s = e->skeletalSprite.filenameLoaded.c_str();
	luaReturnStr(s);
}

luaFunc(entity_idle)
{
	Entity *e = entity(L);
	if (e)
	{
		e->idle();
	}
	luaReturnNil();
}

luaFunc(entity_stopAllAnimations)
{
	Entity *e = entity(L);
	if (e)
		e->skeletalSprite.stopAllAnimations();
	luaReturnNil();
}

luaFunc(entity_setAnimLayerTimeMult)
{
	Entity *e = entity(L);
	int layer = 0;
	float t = 0;
	if (e)
	{
		layer = lua_tointeger(L, 2);
		t = lua_tonumber(L, 3);
		AnimationLayer *l = e->skeletalSprite.getAnimationLayer(layer);
		if (l)
		{
			l->timeMultiplier.interpolateTo(t, lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
		}
	}
	luaReturnNum(t);
}

luaFunc(entity_getAnimLayerTimeMult)
{
	Entity *e = entity(L);
	float t = 0;
	if (e)
	{
		AnimationLayer *l = e->skeletalSprite.getAnimationLayer(lua_tointeger(L, 2));
		if (l)
		{
			t = l->timeMultiplier.x;
		}
	}
	luaReturnNum(t);
}

luaFunc(entity_animate)
{
	SkeletalSprite *skel = getSkeletalSprite(entity(L));
	float ret = 0;
	if (skel)
	{
		float transition = lua_tonumber(L, 5);
		if (transition == -1)
			transition = 0;
		else if (transition == 0)
			transition = 0.2;
		ret = skel->transitionAnimate(getString(L, 2), transition, lua_tointeger(L, 3), lua_tointeger(L, 4));
	}
	luaReturnNum(ret);
}

luaFunc(entity_stopAnimation)
{
	SkeletalSprite *skel = getSkeletalSprite(entity(L));
	if (skel)
	{
		AnimationLayer *animlayer = skel->getAnimationLayer(lua_tointeger(L, 2));
		if (animlayer)
			animlayer->stopAnimation();
	}
	luaReturnNil();
}

luaFunc(entity_getAnimationLoop)
{
	int loop = 0;
	SkeletalSprite *skel = getSkeletalSprite(entity(L));
	if (skel)
	{
		AnimationLayer *animlayer = skel->getAnimationLayer(lua_tointeger(L, 2));
		if (animlayer)
			loop = animlayer->loop ? animlayer->loop : animlayer->enqueuedAnimationLoop;
	}
	luaReturnInt(loop);
}

// entity, x, y, time, ease, relative
luaFunc(entity_move)
{
	Entity *e = entity(L);
	bool ease = lua_tointeger(L, 5);
	Vector p(lua_tonumber(L, 2), lua_tonumber(L, 3));
	if (getBool(L, 6))
		p = e->position + p;

	e->position.interpolateTo(p, lua_tonumber(L, 4), 0, 0, getBool(L, 5));
	luaReturnNil();
}

luaFunc(spawnManaBall)
{
	Vector p;
	p.x = lua_tonumber(L, 1);
	p.y = lua_tonumber(L, 2);
	int amount = lua_tonumber(L, 3);
	dsq->game->spawnManaBall(p, amount);
	luaReturnNil();
}

luaFunc(spawnAroundEntity)
{
	Entity *e = entity(L);
	int num = lua_tonumber(L, 2);
	int radius = lua_tonumber(L, 3);
	std::string entType = getString(L, 4);
	std::string name = getString(L, 5);
	int idx = dsq->game->getIdxForEntityType(entType);
	if (e)
	{
		Vector pos = e->position;
		for (int i = 0; i < num; i++)
		{
			float angle = i*((2*PI)/float(num));

			e = dsq->game->createEntity(idx, 0, pos + Vector(sinf(angle)*radius, cosf(angle)*radius), 0, false, name);
		}
	}
	luaReturnNil();
}

luaFunc(createBeam)
{
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	float a = lua_tonumber(L, 3);
	int l = lua_tointeger(L, 4);
	Beam *b = new Beam(Vector(x,y), a);
	if (l == 1)
		dsq->game->addRenderObject(b, LR_PARTICLES);
	else
		dsq->game->addRenderObject(b, LR_ENTITIES_MINUS2);
	luaReturnPtr(b);
}

luaFunc(beam_setDamage)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setDamage(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

luaFunc(beam_setDamageType)
{
	Beam *b = beam(L);
	if (b)
	{
		b->damageData.damageType = (DamageType)lua_tointeger(L, 2);
	}
	luaReturnNil();
}

luaFunc(beam_setBeamWidth)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setBeamWidth(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

luaFunc(beam_setAngle)
{
	Beam *b = beam(L);
	if (b)
	{
		b->angle = lua_tonumber(L, 2);
		b->trace();
	}
	luaReturnNil();
}

luaFunc(beam_setFirer)
{
	Beam *b = beam(L);
	if (b)
		b->setFirer(entity(L));
	luaReturnNil();
}

// Note the additional trace() call
luaFunc(beam_setPosition_override)
{
	Beam *b = beam(L);
	if (b)
	{
		b->position.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)),
			lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
		b->trace();
	}
	luaReturnNil();
}

luaFunc(getStringBank)
{
	luaReturnStr(dsq->continuity.stringBank.get(lua_tointeger(L, 1)).c_str());
}

luaFunc(isPlat)
{
	int plat = lua_tointeger(L, 1);
	bool v = false;
#ifdef BBGE_BUILD_WINDOWS
	v = (plat == 0);
#elif BBGE_BUILD_MACOSX
	v = (plat == 1);
#elif BBGE_BUILD_UNIX
	v = (plat == 2);
#endif
	luaReturnBool(v);
}

luaFunc(createEntity)
{
	std::string type = getString(L, 1);
	std::string name;
	if (lua_isstring(L, 2))
		name = lua_tostring(L, 2);
	int x = lua_tointeger(L, 3);
	int y = lua_tointeger(L, 4);

	Entity *e = 0;
	e = dsq->game->createEntity(type, 0, Vector(x, y), 0, false, name, ET_ENEMY, true);

	luaReturnPtr(e);
}

luaFunc(savePoint)
{
	Path *p = path(L);
	Vector position;
	if (p)
	{
		//dsq->game->avatar->moveToNode(p, 0, 0, 1);
		position = p->nodes[0].position;
	}

	dsq->doSavePoint(position);
	luaReturnNil();
}

luaFunc(pause)
{
	dsq->game->togglePause(1);
	luaReturnNil();
}

luaFunc(unpause)
{
	dsq->game->togglePause(0);
	luaReturnNil();
}

luaFunc(isPaused)
{
	luaReturnBool(dsq->game->isPaused());
}

luaFunc(isInGameMenu)
{
	luaReturnBool(dsq->game->isInGameMenu());
}

luaFunc(isInEditor)
{
	luaReturnBool(dsq->game->isSceneEditorActive());
}

luaFunc(clearControlHint)
{
	dsq->game->clearControlHint();
	luaReturnNil();
}

luaFunc(setSceneColor)
{
	dsq->game->sceneColor3.interpolateTo(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	luaReturnNil();
}

luaFunc(getSceneColor)
{
	const Vector& c = dsq->game->sceneColor3;
	luaReturnVec3(c.x, c.y, c.z);
}

luaFunc(setCameraLerpDelay)
{
	dsq->game->cameraLerpDelay = lua_tonumber(L, 1);
	luaReturnNil();
}

luaFunc(setControlHint)
{
	std::string str = getString(L, 1);
	bool left = getBool(L, 2);
	bool right = getBool(L, 3);
	bool middle = getBool(L, 4);
	float t = lua_tonumber(L, 5);
	std::string s;
	if (lua_isstring(L, 6))
		s = lua_tostring(L, 6);
	int songType = lua_tointeger(L, 7);
	float scale = lua_tonumber(L, 8);
	if (scale == 0)
		scale = 1;

	dsq->game->setControlHint(str, left, right, middle, t, s, false, songType, scale);
	luaReturnNil();
}

luaFunc(setCanChangeForm)
{
	dsq->game->avatar->canChangeForm = getBool(L);
	luaReturnNil();
}

luaFunc(setInvincibleOnNested)
{
	dsq->game->invincibleOnNested = getBool(L);
	luaReturnNil();
}

luaFunc(setCanWarp)
{
	dsq->game->avatar->canWarp = getBool(L);
	luaReturnNil();
}

luaFunc(entity_generateCollisionMask)
{
	Entity *e = entity(L);
	float num = lua_tonumber(L, 2);
	if (e)
	{
		e->generateCollisionMask(num);
	}
	luaReturnNil();
}

luaFunc(entity_damage)
{
	Entity *e = entity(L);
	bool didDamage = false;
	if (e)
	{
		DamageData d;
		//d.attacker = e;
		d.attacker = lua_isuserdata(L, 2) ? entity(L, 2) : NULL;
		d.damage = lua_tonumber(L, 3);
		d.damageType = (DamageType)lua_tointeger(L, 4);
		didDamage = e->damage(d);
	}
	luaReturnBool(didDamage);
}

// must be called in init
luaFunc(entity_setEntityLayer)
{
	ScriptedEntity *e = scriptedEntity(L);
	int l = lua_tonumber(L, 2);
	if (e)
	{
		e->setEntityLayer(l);
	}
	luaReturnNil();
}

// Note that this overrides the generic obj_setRenderPass function for entities.
// (It's registered as "entity_setRenderPass" to Lua)
luaFunc(entity_setRenderPass_override)
{
	Entity *e = entity(L);
	int pass = lua_tointeger(L, 2);
	if (e)
		e->setOverrideRenderPass(pass);
	luaReturnNil();
}

// intended to be used for setting max health and refilling it all
luaFunc(entity_setHealth)
{
	Entity *e = entity(L, 1);
	if (e)
		e->health = e->maxHealth = lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(entity_changeHealth)
{
	Entity *e = entity(L, 1);
	if (e)
		e->health += lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(entity_heal)
{
	Entity *e = entity(L);
	if (e)
		e->heal(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(entity_revive)
{
	Entity *e = entity(L);
	if (e)
		e->revive(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(screenFadeCapture)
{
	dsq->screenTransition->capture();
	luaReturnNil();
}


luaFunc(screenFadeTransition)
{
	dsq->screenTransition->transition(lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(screenFadeGo)
{
	dsq->screenTransition->go(lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(isEscapeKey)
{
	bool isDown = dsq->game->isActing(ACTION_ESC);
	luaReturnBool(isDown);
}

luaFunc(isLeftMouse)
{
	bool isDown = core->mouse.buttons.left || (dsq->game->avatar && dsq->game->avatar->pollAction(ACTION_PRIMARY));
	luaReturnBool(isDown);
}

luaFunc(isRightMouse)
{
	bool isDown = core->mouse.buttons.right || (dsq->game->avatar && dsq->game->avatar->pollAction(ACTION_SECONDARY));
	luaReturnBool(isDown);
}

luaFunc(setTimerTextAlpha)
{
	dsq->game->setTimerTextAlpha(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(setTimerText)
{
	dsq->game->setTimerText(lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(getWallNormal)
{
	float x,y;
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	int range = lua_tointeger(L, 3);
	if (range == 0)
	{
		range = 5;
	}

	Vector n = dsq->game->getWallNormal(Vector(x, y), range);

	luaReturnVec2(n.x, n.y);
}

luaFunc(incrFlag)
{
	std::string f = getString(L, 1);
	int v = 1;
	if (lua_isnumber(L, 2))
		v = lua_tointeger(L, 2);
	dsq->continuity.setFlag(f, dsq->continuity.getFlag(f)+v);
	luaReturnNil();
}

luaFunc(decrFlag)
{
	std::string f = getString(L, 1);
	int v = 1;
	if (lua_isnumber(L, 2))
		v = lua_tointeger(L, 2);
	dsq->continuity.setFlag(f, dsq->continuity.getFlag(f)-v);
	luaReturnNil();
}

luaFunc(setFlag)
{
	/*
	if (lua_isstring(L, 1))
		dsq->continuity.setFlag(lua_tostring(L, 1), lua_tonumber(L, 2));
	else
	*/
	dsq->continuity.setFlag(lua_tointeger(L, 1), lua_tointeger(L, 2));
	luaReturnNil();
}

luaFunc(getFlag)
{
	int v = 0;
	/*
	if (lua_isstring(L, 1))
		v = dsq->continuity.getFlag(lua_tostring(L, 1));
	else if (lua_isnumber(L, 1))
	*/
	v = dsq->continuity.getFlag(lua_tointeger(L, 1));

	luaReturnNum(v);
}

luaFunc(getStringFlag)
{
	luaReturnStr(dsq->continuity.getStringFlag(getString(L, 1)).c_str());
}

luaFunc(node_setActive)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->active = v;
	}
	luaReturnNil();
}

luaFunc(node_setCursorActivation)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->cursorActivation = v;
	}
	luaReturnNil();
}

luaFunc(node_setCatchActions)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->catchActions = v;
	}
	luaReturnNil();
}

luaFunc(node_isEntityInRange)
{
	Path *p = path(L);
	Entity *e = entity(L,2);
	int range = lua_tonumber(L, 3);
	bool v = false;
	if (p && e)
	{
		if ((p->nodes[0].position - e->position).isLength2DIn(range))
		{
			v = true;
		}
	}
	luaReturnBool(v);
}

luaFunc(node_isEntityPast)
{
	Path *p = path(L);
	bool past = false;
	if (p && !p->nodes.empty())
	{
		PathNode *n = &p->nodes[0];
		Entity *e = entity(L, 2);
		if (e)
		{
			bool checkY = lua_tonumber(L, 3);
			int dir = lua_tonumber(L, 4);
			int range = lua_tonumber(L, 5);
			if (!checkY)
			{
				if (e->position.x > n->position.x-range && e->position.x < n->position.x+range)
				{
					if (!dir)
					{
						if (e->position.y < n->position.y)
							past = true;
					}
					else
					{
						if (e->position.y > n->position.y)
							past = true;
					}
				}
			}
			else
			{
				if (e->position.y > n->position.y-range && e->position.y < n->position.y+range)
				{
					if (!dir)
					{
						if (e->position.x < n->position.x)
							past = true;
					}
					else
					{
						if (e->position.x > n->position.x)
							past = true;
					}
				}
			}
		}
	}
	luaReturnBool(past);
}

luaFunc(node_x)
{
	Path *p = path(L);
	float v = 0;
	if (p)
	{
		v = p->nodes[0].position.x;
	}
	luaReturnNum(v);
}

luaFunc(node_y)
{
	Path *p = path(L);
	float v = 0;
	if (p)
	{
		v = p->nodes[0].position.y;
	}
	luaReturnNum(v);
}

luaFunc(entity_isName)
{
	Entity *e = entity(L);
	std::string s = getString(L, 2);
	bool ret = false;
	if (e)
	{
		ret = (nocasecmp(s,e->name)==0);
	}
	luaReturnBool(ret);
}


luaFunc(entity_getName)
{
	Entity *e = entity(L);
	const char *s = "";
	if (e)
	{
		s = e->name.c_str();
	}
	luaReturnStr(s);
}

luaFunc(node_getContent)
{
	Path *p = path(L);
	const char *s = "";
	if (p)
	{
		s = p->content.c_str();
	}
	luaReturnStr(s);
}

luaFunc(node_getAmount)
{
	Path *p = path(L);
	float a = 0;
	if (p)
	{
		a = p->amount;
	}
	luaReturnNum(a);
}

luaFunc(node_getSize)
{
	Path *p = path(L);
	int w=0,h=0;
	if (p)
	{
		w = p->rect.getWidth();
		h = p->rect.getHeight();
	}
	luaReturnVec2(w, h);
}

luaFunc(node_getName)
{
	Path *p = path(L);
	const char *s = "";
	if (p)
	{
		s = p->name.c_str();
	}
	luaReturnStr(s);
}

luaFunc(node_getLabel)
{
	Path *p = path(L);
	const char *s = "";
	if (p)
	{
		s = p->label.c_str();
	}
	luaReturnStr(s);
}

luaFunc(node_getPathPosition)
{
	Path *p = path(L);
	int idx = lua_tonumber(L, 2);
	float x=0,y=0;
	if (p)
	{
		PathNode *node = p->getPathNode(idx);
		if (node)
		{
			x = node->position.x;
			y = node->position.y;
		}
	}
	luaReturnVec2(x, y);
}

luaFunc(node_getPosition)
{
	Path *p = path(L);
	float x=0,y=0;
	if (p)
	{
		PathNode *node = &p->nodes[0];
		x = node->position.x;
		y = node->position.y;
	}
	luaReturnVec2(x, y);
}

luaFunc(node_setPosition)
{
	Path *p = path(L);
	float x=0,y=0;
	if (p)
	{
		x = lua_tonumber(L, 2);
		y = lua_tonumber(L, 3);
		PathNode *node = &p->nodes[0];
		node->position = Vector(x, y);
	}
	luaReturnNil();
}


luaFunc(registerSporeDrop)
{
	float x, y;
	int t=0;
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	t = lua_tointeger(L, 3);

	dsq->game->registerSporeDrop(Vector(x,y), t);

	luaReturnNil();
}

luaFunc(setStringFlag)
{
	std::string n = getString(L, 1);
	std::string v = getString(L, 2);
	dsq->continuity.setStringFlag(n, v);
	luaReturnNil();
}

luaFunc(centerText)
{
	dsq->centerText(getString(L, 1));
	luaReturnNil();
}

luaFunc(msg)
{
	dsq->screenMessage(getString(L, 1));
	luaReturnNil();
}

luaFunc(chance)
{
	int r = rand()%100;
	int c = lua_tointeger(L, 1);
	if (c == 0)
		luaReturnBool(false);
	else
	{
		if (r <= c || c==100)
			luaReturnBool(true);
		else
			luaReturnBool(false);
	}
}

luaFunc(entity_handleShotCollisions)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->game->handleShotCollisions(e);
	}
	luaReturnNil();
}

luaFunc(entity_handleShotCollisionsSkeletal)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->game->handleShotCollisionsSkeletal(e);
	}
	luaReturnNil();
}

luaFunc(entity_handleShotCollisionsHair)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->game->handleShotCollisionsHair(e, lua_tonumber(L, 2));
	}
	luaReturnNil();
}

luaFunc(entity_collideSkeletalVsCircle)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L,2);
	Bone *b = 0;
	if (e && e2)
	{
		b = dsq->game->collideSkeletalVsCircle(e,e2);
	}
	luaReturnPtr(b);
}

luaFunc(entity_collideSkeletalVsLine)
{
	Entity *e = entity(L);
	int x1, y1, x2, y2, sz;
	x1 = lua_tonumber(L, 2);
	y1 = lua_tonumber(L, 3);
	x2 = lua_tonumber(L, 4);
	y2 = lua_tonumber(L, 5);
	sz = lua_tonumber(L, 6);
	Bone *b = 0;
	if (e)
	{
		b = dsq->game->collideSkeletalVsLine(e, Vector(x1, y1), Vector(x2, y2), sz);
	}
	luaReturnPtr(b);
}

luaFunc(entity_collideHairVsCircle)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	bool col=false;
	if (e && e2)
	{
		int num = lua_tonumber(L, 3);
		// perc: percent of hairWidth to use as collide radius
		float perc = lua_tonumber(L, 4);
		int colSegment;
		col = dsq->game->collideHairVsCircle(e, num, e2->position, e2->collideRadius, perc, &colSegment);
		if(col)
		{
			lua_pushboolean(L, true);
			lua_pushinteger(L, colSegment);
			return 2;
		}
	}
	luaReturnBool(false);
}

luaFunc(entity_collideSkeletalVsCircleForListByName)
{
	Entity *e = entity(L);
	std::string name;
	if (lua_isstring(L, 2))
		name = lua_tostring(L, 2);
	if (e && !name.empty())
	{
		FOR_ENTITIES(i)
		{
			Entity *e2 = *i;
			if (e2->life == 1 && e2->name == name)
			{
				Bone *b = dsq->game->collideSkeletalVsCircle(e, e2);
				if (b)
				{
					DamageData d;
					d.attacker = e2;
					d.bone = b;
					e->damage(d);
				}
			}
		}
	}
	luaReturnNil();
}

luaFunc(entity_debugText)
{
	Entity *e = entity(L);
	const char *txt = lua_tostring(L, 2);
	if (e && txt)
	{
		BitmapText *f = new BitmapText(&dsq->smallFont);
		f->setText(txt);
		f->position = e->position;
		core->getTopStateData()->addRenderObject(f, LR_DEBUG_TEXT);
		f->setLife(5);
		f->setDecayRate(1);
		f->fadeAlphaWithLife=1;
	}
	luaReturnNil();
}

luaFunc(entity_getHealth)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->health : 0);
}

luaFunc(entity_getMaxHealth)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->maxHealth : 0);
}

luaFunc(entity_initSegments)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->initSegments(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), getString(L, 5), getString(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8), lua_tonumber(L, 9), lua_tointeger(L, 10));

	luaReturnNil();
}

luaFunc(entity_warpSegments)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->warpSegments();

	luaReturnNum(0);
}

luaFunc(avatar_incrLeaches)
{
	dsq->game->avatar->leaches++;
	luaReturnNum(0);
}

luaFunc(avatar_decrLeaches)
{
	// Not checking for underflow here because this allows some neat tricks.
	dsq->game->avatar->leaches--;
	luaReturnNum(0);
}

luaFunc(entity_rotateToVel)
{
	Entity *e = entity(L);
	if (e)
	{
		if (!e->vel.isZero())
		{
			e->rotateToVec(e->vel, lua_tonumber(L, 2), lua_tointeger(L, 3));
		}
	}
	luaReturnNil();
}

luaFunc(entity_rotateToEntity)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);

	if (e && e2)
	{
		Vector vec = e2->position - e->position;
		if (!vec.isZero())
		{
			e->rotateToVec(vec, lua_tonumber(L, 3), lua_tointeger(L, 4));
		}
	}
	luaReturnNil();
}

luaFunc(entity_rotateToVec)
{
	Entity *e = entity(L);
	Vector vec(lua_tonumber(L, 2), lua_tonumber(L, 3));
	if (e)
	{
		if (!vec.isZero())
		{
			e->rotateToVec(vec, lua_tonumber(L, 4), lua_tointeger(L, 5));
		}
	}
	luaReturnNil();
}

luaFunc(entity_updateSkeletal)
{
	Entity *e = entity(L);
	if (e)
	{
		bool oldIgnore = e->skeletalSprite.ignoreUpdate;
		e->skeletalSprite.ignoreUpdate = false;
		e->skeletalSprite.update(lua_tonumber(L, 2));
		e->skeletalSprite.ignoreUpdate = oldIgnore;
	}
	luaReturnNil();
}

luaFunc(entity_msg)
{
	Entity *e = entity(L);
	if (e)
	{
		// pass everything on the stack except the entity pointer
		int res = e->messageVariadic(L, lua_gettop(L) - 1);
		if (res >= 0)
			return res;
	}
	luaReturnNil();
}

luaFunc(entity_updateCurrents)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->updateCurrents(lua_tonumber(L, 2)) : false);
}

luaFunc(entity_updateLocalWarpAreas)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->updateLocalWarpAreas(getBool(L, 2)) : false);
}

luaFunc(entity_updateMovement)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
		e->updateMovement(lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(entity_applySurfaceNormalForce)
{
	Entity *e = entity(L);
	if (e)
	{
		Vector v;
		if (!e->ridingOnEntity)
		{
			v = dsq->game->getWallNormal(e->position, 8);
		}
		else
		{
			v = e->position - e->ridingOnEntity->position;
			e->ridingOnEntity = 0;
		}
		v.setLength2D(lua_tointeger(L, 2));
		e->vel += v;
	}
	luaReturnNil();
}

luaFunc(flingMonkey)
{
	Entity *e = entity(L);

	dsq->continuity.flingMonkey(e);

	luaReturnNil();
}

luaFunc(entity_getDistanceToTarget)
{
	Entity *e = entity(L);
	float dist = 0;
	if (e)
	{
		Entity *t = e->getTargetEntity();
		if (t)
		{
			dist = (t->position - e->position).getLength2D();
		}
	}
	luaReturnNum(dist);
}

luaFunc(entity_getDistanceToPoint)
{
	Entity *e = entity(L);
	float dist = 0;
	if (e)
	{
		Vector p(lua_tonumber(L, 2), lua_tonumber(L, 3));
		dist = (p - e->position).getLength2D();
	}
	luaReturnNum(dist);
}

luaFunc(entity_watchEntity)
{
	Entity *e = entity(L);
	Entity *e2 = 0;
	if (lua_touserdata(L, 2) != NULL)
		e2 = entity(L, 2);

	if (e)
	{
		e->watchEntity(e2);
	}
	luaReturnNil();
}

luaFunc(setNaijaHeadTexture)
{
	Avatar *a = dsq->game->avatar;
	if (a)
	{
		a->setHeadTexture(getString(L, 1), lua_tonumber(L, 2));
	}
	luaReturnNil();
}

luaFunc(entity_flipToSame)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	if (e && e2)
	{
		if ((e->isfh() && !e2->isfh())
			|| (!e->isfh() && e2->isfh()))
		{
			e->flipHorizontal();
		}
	}
	luaReturnNil();
}

luaFunc(entity_flipToEntity)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	if (e && e2)
	{
		e->flipToTarget(e2->position);
	}
	luaReturnNil();
}

luaFunc(entity_flipToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	PathNode *n = &p->nodes[0];
	if (e && n)
	{
		e->flipToTarget(n->position);
	}
	luaReturnNil();
}

luaFunc(entity_flipToVel)
{
	Entity *e = entity(L);
	if (e)
	{
		e->flipToVel();
	}
	luaReturnNil();
}

luaFunc(node_isEntityIn)
{
	Path *p = path(L,1);
	Entity *e = entity(L,2);

	bool v = false;
	if (e && p)
	{
		if (!p->nodes.empty())
		{
			v = p->isCoordinateInside(e->position);
			//(e->position - p->nodes[0].position);
		}
	}
	luaReturnBool(v);
}

luaFunc(node_isPositionIn)
{
	Path *p = path(L,1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);

	bool v = false;
	if (p)
	{
		if (!p->nodes.empty())
		{
			v = p->rect.isCoordinateInside(Vector(x,y) - p->nodes[0].position);
		}
	}
	luaReturnBool(v);
}

luaFunc(entity_isInDarkness)
{
	Entity *e = entity(L);
	bool d = false;
	if (e)
	{
		d = e->isInDarkness();
	}
	luaReturnBool(d);
}

luaFunc(entity_isInRect)
{
	Entity *e = entity(L);
	bool v= false;
	int x1, y1, x2, y2;
	x1 = lua_tonumber(L, 2);
	y1 = lua_tonumber(L, 3);
	x2 = lua_tonumber(L, 4);
	y2 = lua_tonumber(L, 5);
	if (e)
	{
		if (e->position.x > x1 && e->position.x < x2)
		{
			if (e->position.y > y1 && e->position.y < y2)
			{
				v = true;
			}
		}
	}
	luaReturnBool(v);
}

luaFunc(createQuad)
{
	PauseQuad *q = new PauseQuad();
	q->setTexture(getString(L, 1));
	int layer = lua_tonumber(L, 2);
	if (layer == 13)
		layer = 13;
	else
		layer = (LR_PARTICLES+1) - LR_ELEMENTS1;
	dsq->game->addRenderObject(q, LR_ELEMENTS1+(layer-1));
	q->moveToFront();

	luaReturnPtr(q);
}

luaFunc(quad_setPauseLevel)
{
	Quad *q = getQuad(L);
	ENSURE_TYPE(q, SCO_PAUSEQUAD);
	if (q)
		((PauseQuad*)q)->pauseLevel = lua_tointeger(L, 2);
	luaReturnNil();
}

luaFunc(setupEntity)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		std::string tex;
		if (lua_isstring(L, 2))
		{
			tex = lua_tostring(L, 2);
		}
		se->setupEntity(tex, lua_tonumber(L, 3));
	}
	luaReturnNil();
}

luaFunc(setupBasicEntity)
{
	ScriptedEntity *se = scriptedEntity(L);
	//-- texture, health, manaballamount, exp, money, collideRadius, initState
	if (se)
		se->setupBasicEntity(getString(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8), lua_tointeger(L, 9), lua_tointeger(L, 10), lua_tointeger(L, 11), lua_tointeger(L, 12), lua_tointeger(L, 13), lua_tointeger(L, 14));

	luaReturnNil();
}

luaFunc(entity_setBeautyFlip)
{
	Entity *e = entity(L);
	if (e)
	{
		e->beautyFlip = getBool(L, 2);
	}
	luaReturnNil();
}

luaFunc(setInvincible)
{
	dsq->game->invinciblity = getBool(L, 1);

	luaReturnBool(dsq->game->invinciblity);
}

luaFunc(entity_setInvincible)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setInvincible(getBool(L, 2));

	}
	luaReturnNil();
}

luaFunc(entity_setDeathSound)
{
	Entity *e = entity(L);
	if (e)
	{
		e->deathSound = getString(L, 2);
	}
	luaReturnNil();
}

luaFunc(entity_setDeathParticleEffect)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->deathParticleEffect = getString(L, 2);
	}
	luaReturnNil();
}

luaFunc(entity_setNaijaReaction)
{
	Entity *e = entity(L);
	std::string s;
	if (lua_isstring(L, 2))
		s = lua_tostring(L, 2);
	if (e)
	{
		e->naijaReaction = s;
	}
	luaReturnNil();
}

luaFunc(entity_setName)
{
	Entity *e = entity(L);
	std::string s;
	if (lua_isstring(L, 2))
		s = lua_tostring(L, 2);
	if (e)
	{
		debugLog("setting entity name to: " + s);
		e->setName(s);
	}
	luaReturnNil();
}

luaFunc(entity_pathBurst)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
		v = e->pathBurst(lua_tointeger(L, 2));
	luaReturnBool(v);
}

luaFunc(entity_moveTowardsAngle)
{
	Entity *e = entity(L);
	if (e)
		e->moveTowardsAngle(lua_tointeger(L, 2), lua_tonumber(L, 3), lua_tointeger(L, 4));
	luaReturnNil();
}

luaFunc(entity_moveAroundAngle)
{
	Entity *e = entity(L);
	if (e)
		e->moveTowardsAngle(lua_tointeger(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	luaReturnNil();
}

luaFunc(entity_moveTowards)
{
	Entity *e = entity(L);
	if (e)
		e->moveTowards(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5));
	luaReturnNil();
}

luaFunc(entity_moveAround)
{
	Entity *e = entity(L);
	if (e)
		e->moveAround(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	luaReturnNil();
}

luaFunc(entity_addVel2)
{
	Entity *e = entity(L);
	if (e)
		e->vel2 += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

luaFunc(entity_setVel2)
{
	Entity *e = entity(L);
	if (e)
	{
		e->vel2 = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnNil();
}

luaFunc(entity_getVel2Len)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->vel2.getLength2D() : 0.0f);
}

luaFunc(entity_setVel2Len)
{
	Entity *e = entity(L);
	if(e)
		e->vel2.setLength2D(lua_tonumber(L, 2));
	luaReturnNil();
};

luaFunc(entity_getVel2)
{
	Entity *e = entity(L);
	Vector vel2;
	if(e)
		vel2 = e->vel2;
	luaReturnVec2(vel2.x, vel2.y);
}

luaFunc(entity_isValidTarget)
{
	Entity *e = entity(L);
	Entity *e2 = 0;
	if (lua_tonumber(L, 2)!=0)
		e2 = entity(L);
	bool b = false;
	if (e)
		b = dsq->game->isValidTarget(e, e2);
	luaReturnBool(b);
}
luaFunc(entity_clearVel2)
{
	Entity *e = entity(L);
	if (e)
		e->vel2 = Vector(0,0,0);
	luaReturnNil();
}

luaFunc(getScreenCenter)
{
	luaReturnVec2(core->screenCenter.x, core->screenCenter.y);
}

luaFunc(entity_isState)
{
	Entity *e = entity(L);
	bool v=false;
	if (e)
	{
		v = (e->getState() == lua_tointeger(L, 2));
	}
	luaReturnBool(v);
}

luaFunc(entity_getState)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getState();
	luaReturnNum(state);
}

luaFunc(entity_getEnqueuedState)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getEnqueuedState();
	luaReturnNum(state);
}

luaFunc(entity_getPrevState)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getPrevState();
	luaReturnNum(state);
}

luaFunc(entity_setTarget)
{
	Entity *e = entity(L);
	Entity *t = 0;
	if (lua_touserdata(L, 2) != NULL)
	{
		t = entity(L, 2);
	}
	if (e)
	{
		e->setTargetEntity(t);
	}
	luaReturnNil();
}

luaFunc(entity_setBounce)
{
	CollideEntity *e = collideEntity(L);
	if (e)
		e->bounceAmount = e->bounceEntityAmount = lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(avatar_isSinging)
{
	bool b = dsq->game->avatar->isSinging();
	luaReturnBool(b);
}

luaFunc(avatar_isTouchHit)
{
	//avatar_canBeTouchHit
	bool b = !(dsq->game->avatar->bursting && dsq->continuity.form == FORM_BEAST);
	luaReturnBool(b);
}

luaFunc(avatar_clampPosition)
{
	dsq->game->avatar->clampPosition();
	luaReturnNil();
}

luaFunc(entity_setMaxSpeed)
{
	Entity *e = entity(L);
	if (e)
		e->setMaxSpeed(lua_tointeger(L, 2));

	luaReturnNil();
}

luaFunc(entity_getMaxSpeed)
{
	Entity *e = entity(L);
	int v = 0;
	if (e)
		v = e->getMaxSpeed();

	luaReturnNum(v);
}

luaFunc(entity_setMaxSpeedLerp)
{
	Entity *e = entity(L);
	if (e)
		e->maxSpeedLerp.interpolateTo(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));

	luaReturnNil();
}

luaFunc(entity_getMaxSpeedLerp)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->maxSpeedLerp.x : 0.0f);
}

// note: this is a weaker setState than perform
// this is so that things can override it
// for example getting PUSH-ed (Force) or FROZEN (bubbled)
luaFunc(entity_setState)
{
	Entity *me = entity(L);
	if (me)
	{
		int state = lua_tointeger(L, 2);
		float time = lua_tonumber(L, 3);
		if (time == 0)
			time = -1;
		bool force = getBool(L, 4);
		me->setState(state, time, force);
	}
	luaReturnNil();
}

luaFunc(entity_getBoneByIdx)
{
	Entity *e = entity(L);
	Bone *b = 0;
	if (e)
	{
		int n = 0;
		if (lua_isnumber(L, 2))
		{
			n = lua_tonumber(L, 2);
			b = e->skeletalSprite.getBoneByIdx(n);
		}
	}
	luaReturnPtr(b);
}

luaFunc(entity_getBoneByName)
{
	Entity *e = entity(L);
	luaReturnPtr(e ? e->skeletalSprite.getBoneByName(getString(L, 2)) : NULL);
}

luaFunc(bone_getIndex)
{
	Bone *b = bone(L);
	int idx = -1;
	if (b)
		idx = b->boneIdx;
	luaReturnNum(idx);
}

luaFunc(bone_getName)
{
	const char *n = "";
	Bone *b = bone(L);
	if (b)
	{
		n = b->name.c_str();
	}
	luaReturnStr(n);
}

luaFunc(bone_isName)
{
	Bone *b = bone(L);
	bool v = false;
	const char *s = lua_tostring(L, 2);
	if (b && s)
	{
		v = b->name == s;
	}
	luaReturnBool(v);
}

luaFunc(overrideZoom)
{
	dsq->game->overrideZoom(lua_tonumber(L, 1), lua_tonumber(L, 2));

	luaReturnNil();
}

luaFunc(disableOverrideZoom)
{
	dsq->game->toggleOverrideZoom(false);
	luaReturnNil();
}

// dt, range, mod
luaFunc(entity_doSpellAvoidance)
{
	Entity *e = entity(L);
	if (e)
		e->doSpellAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4));
	luaReturnNil();
}

// dt, range, mod, ignore
luaFunc(entity_doEntityAvoidance)
{
	Entity *e = entity(L);
	if (e)
		e->doEntityAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), e->getTargetEntity());
	luaReturnNil();
}

// doCollisionAvoidance(me, dt, search, mod)
luaFunc(entity_doCollisionAvoidance)
{
	Entity *e = entity(L);
	bool ret = false;

	int useVel2 = lua_tonumber(L, 6);
	bool onlyVP = getBool(L, 7);

	if (e)
	{
		if (useVel2)
			ret = e->doCollisionAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), &e->vel2, lua_tonumber(L, 5), onlyVP);
		else
			ret = e->doCollisionAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), 0, lua_tonumber(L, 5));
	}
	luaReturnBool(ret);
}

luaFunc(setOverrideMusic)
{
	dsq->game->overrideMusic = getString(L, 1);
	luaReturnNil();
}

luaFunc(setOverrideVoiceFader)
{
	dsq->sound->setOverrideVoiceFader(lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(setGameSpeed)
{
	dsq->gameSpeed.stop();
	dsq->gameSpeed.stopPath();
	dsq->gameSpeed.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	luaReturnNil();
}

luaFunc(bedEffects)
{
	dsq->overlay->alpha.interpolateTo(1, 2);
	dsq->sound->fadeMusic(SFT_OUT, 1);
	core->main(1);
	// music goes here
	dsq->sound->fadeMusic(SFT_CROSS, 1);
	dsq->sound->playMusic("Sleep");
	core->main(6);
	Vector bedPosition(lua_tointeger(L, 1), lua_tointeger(L, 2));
	if (bedPosition.x == 0 && bedPosition.y == 0)
	{
		bedPosition = dsq->game->avatar->position;
	}
	dsq->game->positionToAvatar = bedPosition;
	dsq->game->transitionToScene(dsq->game->sceneName);

	luaReturnNil();
}

luaFunc(entity_setDeathScene)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDeathScene(getBool(L, 2));
	}
	luaReturnNil();
}

luaFunc(entity_isDeathScene)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->isDeathScene() : false);
}

luaFunc(entity_setCurrentTarget)
{
	Entity *e = entity(L);
	if (e)
		e->currentEntityTarget = lua_tointeger(L, 2);
	luaReturnNil();
}

luaFunc(setMiniMapHint)
{
	std::istringstream is(getString(L, 1));
	is >> dsq->game->miniMapHint.scene >> dsq->game->miniMapHint.warpAreaType;
	dsq->game->updateMiniMapHintPosition();

	luaReturnNil();
}

luaFunc(entityFollowEntity)
{
	Entity *e = dsq->getEntityByName(getString(L, 1));
	if (e)
	{
		e->followEntity = dsq->getEntityByName(getString(L, 2));
	}

	luaReturnNil();
}

luaFunc(entity_isFollowingEntity)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
		v = e->followEntity != 0;
	luaReturnBool(v);
}

luaFunc(entity_followEntity)
{
	Entity *e1 = entity(L);
	Entity *e2 = 0;
	if (lua_touserdata(L, 2) != NULL)
	{
		e2 = entity(L, 2);
	}
	if (e1)
	{
		e1->followEntity = e2;
		e1->followPos = lua_tointeger(L, 3);
	}
	luaReturnNil();
}

luaFunc(toggleInput)
{
	int v = lua_tointeger(L, 1);
	if (v)
		dsq->game->avatar->enableInput();
	else
		dsq->game->avatar->disableInput();
	luaReturnNil();
}

luaFunc(warpAvatar)
{
	dsq->game->positionToAvatar = Vector(lua_tointeger(L, 2),lua_tointeger(L, 3));
	dsq->game->transitionToScene(getString(L, 1));

	luaReturnNil();
}

luaFunc(warpNaijaToSceneNode)
{
	std::string scene = getString(L, 1);
	std::string node = getString(L, 2);
	std::string flip = getString(L, 3);
	if (!scene.empty() && !node.empty())
	{
		dsq->game->toNode = node;
		stringToLower(flip);
		if (flip == "l")
			dsq->game->toFlip = 0;
		if (flip == "r")
			dsq->game->toFlip = 1;
		dsq->game->transitionToScene(scene);
	}

	luaReturnNil();
}

luaFunc(entity_setDamageTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDamageTarget((DamageType)lua_tointeger(L, 2), getBool(L, 3));
	}
	luaReturnNil();
}

luaFunc(entity_setAllDamageTargets)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setAllDamageTargets(getBool(L, 2));
	}
	luaReturnNil();
}

luaFunc(entity_isDamageTarget)
{
	Entity *e = entity(L);
	bool v=false;
	if (e)
	{
		v = e->isDamageTarget((DamageType)lua_tointeger(L, 2));
	}
	luaReturnBool(v);
}

luaFunc(entity_setTargetRange)
{
	Entity *e = entity(L);
	if (e)
	{
		e->targetRange = lua_tonumber(L, 2);
	}
	luaReturnNil();
}

luaFunc(entity_clearTargetPoints)
{
	Entity *e = entity(L);
	if (e)
		e->clearTargetPoints();
	luaReturnNil();
}

luaFunc(entity_addTargetPoint)
{
	Entity *e = entity(L);
	if (e)
		e->addTargetPoint(Vector(lua_tonumber(L,2), lua_tonumber(L, 3)));
	luaReturnNil();
}

luaFunc(entity_getTargetPoint)
{
	Entity *e = entity(L);
	int idx = lua_tointeger(L, 2);
	Vector v;
	if (e)
	{
		v = e->getTargetPoint(idx);
	}
	luaReturnVec2(v.x, v.y);
}

luaFunc(entity_getRandomTargetPoint)
{
	Entity *e = entity(L);
	Vector v;
	int idx = 0;
	if (e)
	{
		idx = e->getRandomTargetPoint();
	}
	luaReturnNum(idx);
}

luaFunc(playVisualEffect)
{
	Entity *target = NULL;
	if(lua_isuserdata(L, 4))
		target = entity(L, 4);
	dsq->playVisualEffect(lua_tonumber(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), target);
	luaReturnNil();
}

luaFunc(playNoEffect)
{
	dsq->playNoEffect();
	luaReturnNil();
}

luaFunc(createShockEffect)
{
	if (core->afterEffectManager)
	{
		core->afterEffectManager->addEffect(new ShockEffect(
			Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)), // position
			Vector(lua_tonumber(L, 3), lua_tonumber(L, 4)), // original position
			lua_tonumber(L, 5),   // amplitude
			lua_tonumber(L, 6),   // amplitude decay
			lua_tonumber(L, 7),   // frequency
			lua_tonumber(L, 8),   // wave length
			lua_tonumber(L, 9))); // time multiplier
	}
	luaReturnNil();
}

luaFunc(emote)
{
	int emote = lua_tonumber(L, 1);
	dsq->emote.playSfx(emote);
	luaReturnNil();
}

luaFunc(playSfx)
{
	float freq = lua_tonumber(L, 2);
	float vol = lua_tonumber(L, 3);
	int loops = lua_tointeger(L, 4);
	if (vol <= 0)
		vol = 1;

	PlaySfx sfx;

	if (lua_isnumber(L, 5) && lua_isnumber(L, 6))
	{
		const Vector pos(lua_tonumber(L, 5), lua_tonumber(L, 6));
		sfx = dsq->calcPositionalSfx(pos, lua_tonumber(L, 7));
		sfx.vol *= vol;
	}
	else
	{
		sfx.vol = vol;
	}

	// FIXME: See comment in DSQ::playPositionalSfx() -- FG
	if (sfx.vol <= 0)
		luaReturnPtr(NULL);

	sfx.name = getString(L, 1);
	sfx.freq = freq;
	sfx.loops = loops;

	void *handle = NULL;

	if (!dsq->isSkippingCutscene())
	{
		handle = core->sound->playSfx(sfx);
	}

	luaReturnPtr(handle);
}

luaFunc(fadeSfx)
{
	void *header = lua_touserdata(L, 1);
	float ft = lua_tonumber(L, 2);

	core->sound->fadeSfx(header, SFT_OUT, ft);

	luaReturnPtr(header);
}

luaFunc(resetTimer)
{
	dsq->resetTimer();
	luaReturnNil();
}

luaFunc(stopMusic)
{
	dsq->sound->stopMusic();
	luaReturnNil();
}

luaFunc(playMusic)
{
	float crossfadeTime = 0.8;
	dsq->sound->playMusic(getString(L, 1), SLT_LOOP, SFT_CROSS, crossfadeTime);
	luaReturnNil();
}

luaFunc(playMusicStraight)
{
	dsq->sound->setMusicFader(1,0);
	dsq->sound->playMusic(getString(L, 1), SLT_LOOP, SFT_IN, 0.5); //SFT_IN, 0.1);//, SFT_IN, 0.2);
	luaReturnNil();
}

luaFunc(playMusicOnce)
{
	float crossfadeTime = 0.8;
	dsq->sound->playMusic(getString(L, 1), SLT_NONE, SFT_CROSS, crossfadeTime);
	luaReturnNil();
}

luaFunc(addInfluence)
{
	ParticleInfluence pinf;
	pinf.pos.x = lua_tonumber(L, 1);
	pinf.pos.y = lua_tonumber(L, 2);
	pinf.size = lua_tonumber(L, 3);
	pinf.spd = lua_tonumber(L, 4);
	dsq->particleManager->addInfluence(pinf);
	luaReturnNil();
}

luaFunc(updateMusic)
{
	dsq->game->updateMusic();
	luaReturnNil();
}

luaFunc(entity_grabTarget)
{
	Entity *e = entity(L);
	if (e)
		e->attachEntity(e->getTargetEntity(), Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)));
	luaReturnNum(0);
}

luaFunc(entity_clampToHit)
{
	Entity *e = entity(L);
	if (e)
		e->clampToHit();
	luaReturnNil();
}

luaFunc(entity_clampToSurface)
{
	Entity *e = entity(L);
	bool ret = e && e->clampToSurface(lua_tonumber(L, 2));

	luaReturnBool(ret);
}

luaFunc(entity_checkSurface)
{
	Entity *e = entity(L);
	bool c = false;

	if (e)
	{
		c = e->checkSurface(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}

	luaReturnBool(c);
}

luaFunc(entity_switchSurfaceDirection)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (!e)
		luaReturnNum(0);

	int n = -1;
	if (lua_isnumber(L, 2))
	{
		n = lua_tonumber(L, 2);
	}

	if (e->isv(EV_SWITCHCLAMP, 1))
	{
		Vector oldPos = e->position;
		if (e->isNearObstruction(0))
		{
			Vector n = dsq->game->getWallNormal(e->position);
			if (!n.isZero())
			{
				do
				{
					e->position += n * 2;
				}
				while(e->isNearObstruction(0));
			}
		}
		Vector usePos = e->position;
		e->position = oldPos;
		e->clampToSurface(0, usePos);
	}

	if (n == -1)
	{
		if (e->surfaceMoveDir)
			e->surfaceMoveDir = 0;
		else
			e->surfaceMoveDir = 1;
	}
	else
	{
		e->surfaceMoveDir = n;
	}

	luaReturnNil();
}

luaFunc(entity_adjustPositionBySurfaceNormal)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e && !e->ridingOnEntity)
	{
		Vector v = dsq->game->getWallNormal(e->position);
		if (v.x != 0 || v.y != 0)
		{
			v.setLength2D(lua_tonumber(L, 2));
			e->position += v;
		}
		e->setv(EV_CRAWLING, 0);
		//e->setCrawling(false);
	}
	luaReturnNil();
}

// HACK: move functionality inside entity class
luaFunc(entity_moveAlongSurface)
{
	ScriptedEntity *e = scriptedEntity(L);

	if (e && e->isv(EV_CLAMPING,0))
	{
		e->lastPosition = e->position;

		//if (!e->position.isInterpolating())
		{


			/*
			if (dsq->game->isObstructed(TileVector(e->position)))
			{
				e->moveOutOfWall();
			}
			*/

			Vector v;
			if (e->ridingOnEntity)
			{
				v = (e->position - e->ridingOnEntity->position);
				v.normalize2D();
			}
			else
				v = dsq->game->getWallNormal(e->position);
			//int outFromWall = lua_tonumber(L, 5);
			int outFromWall = e->getv(EV_WALLOUT);
			bool invisibleIn = e->isSittingOnInvisibleIn();

			/*
			if (invisibleIn)
				debugLog("Found invisibleIn");
			else
				debugLog("NOT FOUND");
			*/

			/*
			for (int x = -2; x < 2; x++)
			{
				for (int y = -2; y< 2; y++)
				{
					if (dsq->game->getGrid(TileVector(x,y))== OT_INVISIBLEIN)
					{
						debugLog("found invisible in");
						invisibleIn = true;
						break;
					}
				}
			}
			*/
			if (invisibleIn)
				outFromWall -= TILE_SIZE;
			float t = 0.1;
			e->offset.interpolateTo(v*outFromWall, t);
			/*
			if (outFromWall)
			{
				//e->lastWallOffset = dsq->game->getWallNormal(e->position)*outFromWall;
				//e->offset.interpolateTo(dsq->game->getWallNormal(e->position)*outFromWall, time*2);
				//e->offset = v*outFromWall;

				//float t = 0;
				e->offset.interpolateTo(v*outFromWall, t);
				//pos += e->lastWallOffset;
			}
			else
			{
				e->offset.interpolateTo(Vector(0,0), t);
				//e->offset.interpolateTo(Vector(0,0), time*2);
				//e->lastWallOffset = Vector(0,0);g
			}
			*/
			// HACK: make this an optional parameter?
			//e->rotateToVec(v, 0.1);
			float dt = lua_tonumber(L, 2);
			int speed = lua_tonumber(L, 3);
			//int climbHeight = lua_tonumber(L, 4);
			Vector mov;
			if (e->surfaceMoveDir==1)
				mov = Vector(v.y, -v.x);
			else
				mov = Vector(-v.y, v.x);
			e->position += mov * speed * dt;

			if (e->ridingOnEntity)
				e->ridingOnEntityOffset = e->position - e->ridingOnEntity->position;

			e->vel = 0;

			/*
			float adjustbit = float(speed)/float(TILE_SIZE);
			if (e->isNearObstruction(0))
			{
				Vector n = dsq->game->getWallNormal(e->position);
				if (!n.isZero())
				{
					Vector sp = e->position;
					e->position += n * adjustbit * dt;
				}
			}
			if (!e->isNearObstruction(1))
			{
				Vector n = dsq->game->getWallNormal(e->position);
				if (!n.isZero())
				{
					Vector sp = e->position;
					e->position -= n * adjustbit * dt;
				}
			}
			*/
				/*
				Vector sp = e->position;
				e->clampToSurface();
				*/
				/*
				e->position = sp;
				e->internalOffset.interpolateTo(e->position-sp, 0.2);
				*/
				/*
				e->position = e->lastPosition;
				e->position.interpolateTo(to*0.5f + e->position*0.5f, 0.5);
				*/
					/*
					Vector to = e->position;
					e->position = e->lastPosition;
					e->position.interpolateTo(to, 0.5);
					*/
					/*
					e->position = sp;
					e->internalOffset.interpolateTo(e->position - sp, 0.2);
					*/
					//e->clampToSurface(0.1);
		}
	}

	luaReturnNil();
}

luaFunc(entity_rotateToSurfaceNormal)
{
	//ScriptedEntity *e = scriptedEntity(L);
	Entity *e = entity(L);
	float t = lua_tonumber(L, 2);
	int n = lua_tonumber(L, 3);
	int rot = lua_tonumber(L, 4);
	if (e)
	{
		e->rotateToSurfaceNormal(t, n, rot);
	}
	//Entity *e = entity(L);

	luaReturnNil();
}

luaFunc(entity_releaseTarget)
{
	Entity *e = entity(L);
	if (e)
		e->detachEntity(e->getTargetEntity());
	luaReturnNum(0);
}

luaFunc(esetv)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	int n = lua_tointeger(L, 3);
	if (e)
		e->setv(ev, n);
	luaReturnNum(n);
}

luaFunc(egetv)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	int v = 0;
	if (e)
		v = e->getv(ev);
	luaReturnNum(v);
}

luaFunc(esetvf)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	float n = lua_tonumber(L, 3);
	if (e)
		e->setvf(ev, n);
	luaReturnNum(n);
}

luaFunc(egetvf)
{
	Entity *e = entity(L);
	float vf = 0;
	if (e)
	{
		EV ev = (EV)lua_tointeger(L, 2);
		vf = e->getvf(ev);
	}
	luaReturnNum(vf);
}

luaFunc(eisv)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	int n = lua_tonumber(L, 3);
	bool b = 0;
	if (e)
		b = e->isv(ev, n);
	luaReturnBool(b);
}

luaFunc(vector_normalize)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.normalize2D();
	luaReturnVec2(v.x, v.y);
}

luaFunc(vector_getLength)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	float len = v.getLength2D();
	luaReturnNum(len);
}

luaFunc(vector_setLength)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.setLength2D(lua_tonumber(L, 3));
	luaReturnVec2(v.x, v.y);
}

luaFunc(vector_dot)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float x2 = lua_tonumber(L, 3);
	float y2 = lua_tonumber(L, 4);
	Vector v(x,y);
	Vector v2(x2,y2);
	luaReturnNum(v.dot2D(v2));
}

luaFunc(vector_cap)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.capLength2D(lua_tonumber(L, 3));
	luaReturnVec2(v.x, v.y);
}

luaFunc(vector_isLength2DIn)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	bool ret = v.isLength2DIn(lua_tonumber(L, 3));
	luaReturnBool(ret);
}

luaFunc(entity_push)
{
	Entity *e = entity(L);
	if (e)
	{
		e->push(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	luaReturnNil();
}

luaFunc(entity_pushTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		Entity *target = e->getTargetEntity();
		if (target)
		{
			Vector diff = target->position - e->position;
			diff.setLength2D(lua_tointeger(L, 2));
			target->vel += diff;
		}
	}

	luaReturnNil();
}

luaFunc(watch)
{
	float t = lua_tonumber(L, 1);
	int quit = lua_tointeger(L, 2);
	dsq->watch(t, quit);
	luaReturnNil();
}

luaFunc(wait)
{
	core->main(lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(warpNaijaToEntity)
{
	Entity *e = dsq->getEntityByName(getString(L, 1));
	if (e)
	{
		dsq->overlay->alpha.interpolateTo(1, 1);
		core->main(1);

		Vector offset(lua_tointeger(L, 2), lua_tointeger(L, 3));
		dsq->game->avatar->position = e->position + offset;

		dsq->overlay->alpha.interpolateTo(0, 1);
		core->main(1);
	}
	luaReturnNil();
}

luaFunc(getTimer)
{
	float n = lua_tonumber(L, 1);
	if (n == 0)
		n = 1;
	luaReturnNum(dsq->game->getTimer(n));
}

luaFunc(getHalfTimer)
{
	float n = lua_tonumber(L, 1);
	if (n == 0)
		n = 1;
	luaReturnNum(dsq->game->getHalfTimer(n));
}

luaFunc(isNested)
{
	luaReturnBool(core->isNested());
}

luaFunc(getNumberOfEntitiesNamed)
{
	std::string s = getString(L);
	int c = dsq->game->getNumberOfEntitiesNamed(s);
	luaReturnNum(c);
}

luaFunc(entity_pullEntities)
{
	Entity *e = entity(L);
	if (e)
	{
		Vector pos(lua_tonumber(L, 2), lua_tonumber(L, 3));
		float range = lua_tonumber(L, 4);
		float len = lua_tonumber(L, 5);
		float dt = lua_tonumber(L, 6);
		FOR_ENTITIES(i)
		{
			Entity *ent = *i;
			if (ent != e && (e->getEntityType() == ET_ENEMY || e->getEntityType() == ET_AVATAR) && e->isUnderWater())
			{
				Vector diff = ent->position - pos;
				if (diff.isLength2DIn(range))
				{
					Vector pull = pos - ent->position;
					pull.setLength2D(float(len) * dt);
					ent->vel2 += pull;
					/*
					std::ostringstream os;
					os << "ent: " << ent->name << " + (" << pull.x << ", " << pull.y << ")";
					debugLog(os.str());
					*/
				}
			}
		}
	}
	luaReturnNil();
}

// Note that this overrides the generic obj_delete function for entities.
// (It's registered as "entity_delete" to Lua)
// There is at least one known case where this is necessary:
// Avatar::pullTarget does a life check to drop the pointer;
// If it's instantly deleted, this will cause a crash.
luaFunc(entity_delete_override)
{
	Entity *e = entity(L);
	if (e)
	{
		float time = lua_tonumber(L, 2);
		if (time == 0)
		{
			e->alpha = 0;
			e->setLife(0);
			e->setDecayRate(1);
		}
		else
		{
			e->fadeAlphaWithLife = true;
			e->setLife(1);
			e->setDecayRate(1.0f/time);
		}
	}
	luaReturnInt(0);
}

luaFunc(entity_isRidingOnEntity)
{
	Entity *e = entity(L);
	if (e)
		luaReturnPtr(e->ridingOnEntity);
	else
		luaReturnPtr(NULL);
}

//entity_setProperty(me, EP_SOLID, true)
luaFunc(entity_isProperty)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->isEntityProperty((EntityProperty)int(lua_tonumber(L, 2)));
		//e->setEntityProperty((EntityProperty)lua_tointeger(L, 2), getBool(L, 3));
	}
	luaReturnBool(v);
}

//entity_setProperty(me, EP_SOLID, true)
luaFunc(entity_setProperty)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setEntityProperty((EntityProperty)lua_tointeger(L, 2), getBool(L, 3));
	}
	luaReturnNil();
}

luaFunc(entity_setActivation)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		int type = lua_tonumber(L, 2);
		// cursor radius
		int activationRadius = lua_tonumber(L, 3);
		int range = lua_tonumber(L, 4);
		e->activationType = (Entity::ActivationType)type;
		e->activationRange = range;
		e->activationRadius = activationRadius;
	}

	luaReturnNum(0);
}

luaFunc(entity_setActivationType)
{
	Entity *e = entity(L);
	if (e)
		e->activationType = (Entity::ActivationType)lua_tointeger(L, 2);

	luaReturnInt(0);
}

luaFunc(entity_hasTarget)
{
	Entity *e = entity(L);
	if (e)
		luaReturnBool(e->hasTarget(e->currentEntityTarget));
	else
		luaReturnBool(false);
}

luaFunc(entity_hurtTarget)
{
	Entity *e = entity(L);
	if (e && e->getTargetEntity())
	{
		DamageData d;
		d.attacker = e;
		d.damage = lua_tointeger(L, 2);
		e->getTargetEntity(e->currentEntityTarget)->damage(d);
	}
	/*
	if (e && e->getTargetEntity())
		e->getTargetEntity(e->currentEntityTarget)->damage(lua_tointeger(L, 2), 0, e);
	*/
	luaReturnNil();
}

// radius dmg speed pushtime
luaFunc(entity_touchAvatarDamage)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->touchAvatarDamage(lua_tonumber(L, 2), lua_tonumber(L, 3), Vector(-1,-1,-1), lua_tonumber(L, 4), lua_tonumber(L, 5), Vector(lua_tonumber(L, 6), lua_tonumber(L, 7)));
	}
	luaReturnBool(v);
}

luaFunc(entity_getDistanceToEntity)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	float d = 0;
	if (e && e2)
	{
		Vector diff = e->position - e2->position;
		d = diff.getLength2D();
	}
	luaReturnNum(d);
}

luaFunc(entity_isEntityInside)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->isEntityInside() : false);
}

// entity_istargetInRange
luaFunc(entity_isTargetInRange)
{
	Entity *e = entity(L);
	if (e)
		luaReturnBool(e->isTargetInRange(lua_tointeger(L, 2), e->currentEntityTarget));
	else
		luaReturnBool(false);
}

luaFunc(randAngle360)
{
	luaReturnNum(randAngle360());
}

luaFunc(randVector)
{
	float num = lua_tonumber(L, 1);
	if (num == 0)
		num = 1;
	Vector v = randVector(num);
	luaReturnVec2(v.x, v.y);
}

luaFunc(getNaija)
{
	luaReturnPtr(dsq->game->avatar);
}

luaFunc(getLi)
{
	luaReturnPtr(dsq->game->li);
}

luaFunc(setLi)
{
	dsq->game->li = entity(L);

	luaReturnNil();
}

luaFunc(entity_isPositionInRange)
{
	Entity *e = entity(L);
	bool v = false;
	int x, y;
	x = lua_tonumber(L, 2);
	y = lua_tonumber(L, 3);
	if (e)
	{
		if ((e->position - Vector(x,y)).isLength2DIn(lua_tonumber(L, 4)))
		{
			v = true;
		}
	}
	luaReturnBool(v);
}

luaFunc(entity_isEntityInRange)
{
	Entity *e1 = entity(L);
	Entity *e2 = entity(L, 2);
	bool v= false;
	if (e1 && e2)
	{
		//v = ((e2->position - e1->position).getSquaredLength2D() < sqr(lua_tonumber(L, 3)));
		v = (e2->position - e1->position).isLength2DIn(lua_tonumber(L, 3));
	}
	luaReturnBool(v);
}

//entity_moveTowardsTarget(spd, dt)
luaFunc(entity_moveTowardsTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowardsTarget(lua_tonumber(L, 2), lua_tonumber(L, 3), e->currentEntityTarget);
	}

	luaReturnNil();
}

// entity dt speed dir
luaFunc(entity_moveAroundTarget)
{
	Entity *e = entity(L);
	if (e)
		e->moveAroundTarget(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), e->currentEntityTarget);
	// do stuff
	luaReturnNil();
}

luaFunc(entity_rotateToTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		Entity *t = e->getTargetEntity(e->currentEntityTarget);
		if (t)
		{
			Vector v = t->position - e->position;
			e->rotateToVec(v, lua_tonumber(L, 2), lua_tointeger(L, 3));
		}
	}
	luaReturnNil();
}

luaFunc(entity_partWidthHeight)
{
	ScriptedEntity *e = scriptedEntity(L);
	Quad *r = (Quad*)e->partMap[getString(L, 2)];
	if (r)
	{
		int w = lua_tointeger(L, 3);
		int h = lua_tointeger(L, 4);
		r->setWidthHeight(w, h);
	}
	luaReturnNil();
}

luaFunc(entity_partSetSegs)
{
	ScriptedEntity *e = scriptedEntity(L);
	Quad *r = (Quad*)e->partMap[getString(L, 2)];
	if (r)
	{
		r->setSegs(lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tonumber(L, 9), lua_tointeger(L, 10));
	}
	luaReturnNil();
}

luaFunc(entity_getID)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->getID() : 0);
}

luaFunc(getEntityByID)
{
	debugLog("Calling getEntityByID");
	int v = lua_tointeger(L, 1);
	Entity *found = 0;
	if (v)
	{
		std::ostringstream os;
		os << "searching for entity with id: " << v;
		debugLog(os.str());
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e->getID() == v)
			{
				found = e;
				break;
			}
		}
		if (!found)
		{
			std::ostringstream os;
			os << "entity with id: " << v << " not found!";
			debugLog(os.str());
		}
		else
		{
			std::ostringstream os;
			os << "Found: " << found->name;
			debugLog(os.str());
		}
	}
	else
	{
		debugLog("entity ID was 0");
	}
	luaReturnPtr(found);
}

luaFunc(node_setEffectOn)
{
	Path *p = path(L, 1);
	if (p)
		p->setEffectOn(getBool(L, 2));
	luaReturnNil();
}

luaFunc(node_isEffectOn)
{
	Path *p = path(L, 1);
	luaReturnBool(p ? p->effectOn : false);
}

luaFunc(node_activate)
{
	Path *p = path(L);
	Entity *e = 0;
	if (lua_touserdata(L, 2) != NULL)
		e = entity(L, 2);
	if (p)
	{
		p->activate(e);
	}
	luaReturnNil();
}

luaFunc(node_setElementsInLayerActive)
{
	Path *p = path(L);
	if (p)
	{
		int l = lua_tonumber(L, 2);
		bool v = getBool(L, 3);
		for (Element *e = dsq->getFirstElementOnLayer(l); e; e = e->bgLayerNext)
		{
			if (e && p->isCoordinateInside(e->position))
			{
				e->setElementActive(v);
			}
		}
	}
	luaReturnNil();
}

luaFunc(node_getNumEntitiesIn)
{
	Path *p = path(L);
	std::string name;
	if (lua_isstring(L, 2))
	{
		name = lua_tostring(L, 2);
	}
	int c = 0;
	if (p && !p->nodes.empty())
	{
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (name.empty() || (nocasecmp(e->name, name)==0))
			{
				if (p->isCoordinateInside(e->position))
				{
					c++;
				}
			}
		}
	}
	luaReturnNum(c);
}

luaFunc(node_getNearestEntity)
{
	Path *p = path(L);
	Entity *closest=0;

	if (p && !p->nodes.empty())
	{

		Vector pos = p->nodes[0].position;
		std::string name;
		Entity *ignore = 0;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);
		if (lua_isuserdata(L, 3))
			ignore = entity(L, 3);

		float smallestDist = HUGE_VALF;
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e != ignore && e->isPresent() && e->isNormalLayer())
			{
				if (name.empty() || (nocasecmp(e->name, name)==0))
				{
					float dist = (pos - e->position).getSquaredLength2D();
					if (dist < smallestDist)
					{
						smallestDist = dist;
						closest = e;
					}
				}
			}
		}
	}
	luaReturnPtr(closest);
}

luaFunc(node_getNearestNode)
{
	Path *p = path(L);
	Path *closest = 0;
	if (p && !p->nodes.empty())
	{
		std::string name;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);
		Path *ignore = path(L, 3);
		closest = dsq->game->getNearestPath(p->nodes[0].position, name, ignore);
	}
	luaReturnPtr(closest);
}

luaFunc(entity_getNearestBoneToPosition)
{
	Entity *me = entity(L);
	Vector p(lua_tonumber(L, 2), lua_tonumber(L, 3));
	float smallestDist = HUGE_VALF;
	Bone *closest = 0;
	if (me)
	{
		for (int i = 0; i < me->skeletalSprite.bones.size(); i++)
		{
			Bone *b = me->skeletalSprite.bones[i];
			float dist = (b->getWorldPosition() - p).getSquaredLength2D();
			if (dist < smallestDist)
			{
				smallestDist = dist;
				closest = b;
			}
		}
	}
	luaReturnPtr(closest);
}

luaFunc(entity_getNearestNode)
{
	Entity *me = entity(L);
	Path *closest = 0;
	if (me)
	{
		std::string name;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);
		Path *ignore = path(L, 3);
		closest = dsq->game->getNearestPath(me->position, name, ignore);
	}
	luaReturnPtr(closest);
}

luaFunc(ing_hasIET)
{
	Ingredient *i = getIng(L, 1);
	bool has = i && i->hasIET((IngredientEffectType)lua_tointeger(L, 2));
	luaReturnBool(has);
}

luaFunc(entity_getNearestEntity)
{
	Entity *me = entity(L);
	if (!me)
		luaReturnPtr(0);

	const char *name = 0;
	if (lua_isstring(L, 2))
	{
		name = lua_tostring(L, 2);
		if (!*name)
			name = NULL;
	}

	bool nameCheck = true;
	if (name && (name[0] == '!' || name[0] == '~'))
	{
		name++;
		nameCheck = false;
	}

	float range = lua_tonumber(L, 3);
	EntityType type = ET_NOTYPE;
	if (lua_isnumber(L, 4))
		type = (EntityType)lua_tointeger(L, 4);

	DamageType damageTarget = DT_NONE;
	if (lua_isnumber(L, 5))
		damageTarget = (DamageType)lua_tointeger(L, 5);
	Entity *closest = 0;
	Entity *ignore = 0;
	if (lua_isuserdata(L, 6))
		ignore = entity(L, 6);

	float smallestDist = range ? sqr(range) : HUGE_VALF;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e != me && e != ignore && e->isPresent() && e->isNormalLayer())
		{
			if (type == ET_NOTYPE || e->getEntityType() == type)
			{
				if (damageTarget == DT_NONE || e->isDamageTarget((DamageType)damageTarget))
				{
					if (!name || ((nocasecmp(e->name, name)==0) == nameCheck))
					{
						float dist = (me->position - e->position).getSquaredLength2D();
						if (dist < smallestDist)
						{
							smallestDist = dist;
							closest = e;
						}
					}
				}
			}
		}
	}
	luaReturnPtr(closest);
}

luaFunc(getNearestEntity)
{
	Vector p(lua_tonumber(L, 1), lua_tonumber(L, 2));
	int radius = lua_tointeger(L, 3);
	Entity *ignore = lua_isuserdata(L, 4) ? entity(L, 4) : NULL;
	EntityType et = lua_isnumber(L, 5) ? (EntityType)lua_tointeger(L, 5) : ET_NOTYPE;
	DamageType dt = lua_isnumber(L, 6) ? (DamageType)lua_tointeger(L, 6) : DT_NONE;
	int lrStart = lua_isnumber(L, 7) ? lua_tointeger(L, 7) : -1;
	int lrEnd = lua_isnumber(L, 8) ? lua_tointeger(L, 8) : -1;

	Entity *target = dsq->game->getNearestEntity(p, radius, ignore, ET_ENEMY, dt, lrStart, lrEnd);
	luaReturnPtr(target);
}

luaFunc(findWall)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	int dirx = lua_tonumber(L, 3);
	int diry = lua_tonumber(L, 4);
	if (dirx == 0 && diry == 0){ debugLog("dirx && diry are zero!"); luaReturnNum(0); }

	TileVector t(Vector(x, y));
	while (!dsq->game->isObstructed(t))
	{
		t.x += dirx;
		t.y += diry;
	}
	Vector v = t.worldVector();
	int wall = 0;
	if (diry != 0)		wall = v.y;
	if (dirx != 0)		wall = v.x;
	luaReturnNum(wall);
}

luaFunc(toggleVersionLabel)
{
	bool on = getBool(L, 1);

	dsq->toggleVersionLabel(on);

	luaReturnBool(on);
}

luaFunc(setVersionLabelText)
{
	dsq->setVersionLabelText();
	luaReturnNil();
}

luaFunc(setCutscene)
{
	dsq->setCutscene(getBool(L, 1), getBool(L, 2));
	luaReturnNil();
}

luaFunc(isInCutscene)
{
	luaReturnBool(dsq->isInCutscene());
}

luaFunc(toggleSteam)
{
	bool on = getBool(L, 1);
	for (Path *p = dsq->game->getFirstPathOfType(PATH_STEAM); p; p = p->nextOfType)
	{
		p->setEffectOn(on);
	}
	luaReturnBool(on);
}

luaFunc(getFirstEntity)
{
	luaReturnPtr(dsq->getFirstEntity());
}

luaFunc(getNextEntity)
{
	luaReturnPtr(dsq->getNextEntity());
}

luaFunc(getEntity)
{
	Entity *ent = 0;
	// THIS WAS IMPORTANT: this was for getting entity by NUMBER IN LIST used for looping through all entities in script
	if (lua_isnumber(L, 1))
	{
		//HACK: FIX:
		// this has been disabled due to switching to list based entities
	}
	else if (lua_isstring(L, 1))
	{
		std::string s = lua_tostring(L, 1);
		ent = dsq->getEntityByName(s);
	}
	luaReturnPtr(ent);
}

luaFunc(entity_partAlpha)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		RenderObject *r = e->partMap[getString(L, 2)];
		if (r)
		{
			float start = lua_tonumber(L, 3);
			if (start != -1)
				r->alpha = start;
			r->alpha.interpolateTo(lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8));
		}
	}

	luaReturnNum(0);
}

luaFunc(entity_partBlendType)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
		e->partMap[getString(L, 2)]->setBlendType(lua_tointeger(L, 3));
	luaReturnInt(0);
}

luaFunc(entity_partRotate)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		RenderObject *r = e->partMap[getString(L, 2)];
		if (r)
		{
			r->rotation.interpolateTo(Vector(0,0,lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7));
		}
	}

	luaReturnNum(0);
}

luaFunc(entity_getStateTime)
{
	Entity *e = entity(L);
	float t = 0;
	if (e)
		t = e->getStateTime();
	luaReturnNum(t);
}

luaFunc(entity_setStateTime)
{
	Entity *e = entity(L);
	float t = lua_tonumber(L, 2);
	if (e)
		e->setStateTime(t);
	luaReturnNum(e->getStateTime());
}

luaFunc(entity_offsetUpdate)
{
	Entity *e = entity(L);
	if (e)
	{
		int uc = e->updateCull;
		e->updateCull = -1;
		float t = float(rand()%10000)/1000.0f;
		e->update(t);
		e->updateCull = uc;
	}
	luaReturnNil();
}

// not to be confused with obj_setLayer or entity_setEntityLayer!!
luaFunc(entity_switchLayer)
{
	Entity *e = entity(L);
	if (e)
	{
		int lcode = lua_tonumber(L, 2);
		int toLayer = LR_ENTITIES;

		toLayer = dsq->getEntityLayerToLayer(lcode);

		if (e->getEntityType() == ET_AVATAR)
			toLayer = LR_ENTITIES;

		core->switchRenderObjectLayer(e, toLayer);
	}
	luaReturnNum(0);
}

// entity numSegments segmentLength width texture
luaFunc(entity_initHair)
{
	Entity *se = entity(L);
	if (se)
	{
		se->initHair(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), getString(L, 5));
	}
	luaReturnNil();
}

luaFunc(entity_getHair)
{
	Entity *e = entity(L);
	luaReturnPtr(e ? e->hair : NULL);
}

luaFunc(entity_clearHair)
{
	Entity *e = entity(L);
	if (e)
	{
		e->hair->safeKill();
		e->hair = 0;
	}
	luaReturnNil();
}

luaFunc(entity_getHairPosition)
{
	Entity *se = entity(L);
	float x=0;
	float y=0;
	int idx = lua_tonumber(L, 2);
	if (se && se->hair)
	{
		HairNode *h = se->hair->getHairNode(idx);
		if (h)
		{
			x = h->position.x;
			y = h->position.y;
		}
	}
	luaReturnVec2(x, y);
}

// entity x y z
luaFunc(entity_setHairHeadPosition)
{
	Entity *se = entity(L);
	if (se)
	{
		se->setHairHeadPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	}
	luaReturnNil();
}

luaFunc(entity_updateHair)
{
	Entity *se = entity(L);
	if (se)
	{
		se->updateHair(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

// entity x y dt
luaFunc(entity_exertHairForce)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		if (se->hair)
			se->hair->exertForce(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5));
	}
	luaReturnNil();
}

luaFunc(entity_initPart)
{
	std::string partName(getString(L, 2));
	std::string partTex(getString(L, 3));
	Vector partPosition(lua_tointeger(L, 4), lua_tointeger(L, 5));
	int renderAfter = lua_tointeger(L, 6);
	bool partFlipH = lua_tointeger(L, 7);
	bool partFlipV = lua_tointeger(L,8);
	Vector offsetInterpolateTo(lua_tointeger(L, 9), lua_tointeger(L, 10));
	float offsetInterpolateTime = lua_tonumber(L, 11);


	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		Quad *q = new Quad;
		q->setTexture(partTex);
		q->renderBeforeParent = !renderAfter;


		q->position = partPosition;
		if (offsetInterpolateTo.x != 0 || offsetInterpolateTo.y != 0)
			q->offset.interpolateTo(offsetInterpolateTo, offsetInterpolateTime, -1, 1, 1);
		if (partFlipH)
			q->flipHorizontal();
		if (partFlipV)
			q->flipVertical();

		e->addChild(q, PM_POINTER);
		e->registerNewPart(q, partName);
	}

	luaReturnNum(0);
}

luaFunc(entity_findTarget)
{
	Entity *e = entity(L);
	if (e)
		e->findTarget(lua_tointeger(L, 2), lua_tointeger(L, 3), e->currentEntityTarget);

	luaReturnNil();
}

luaFunc(entity_doFriction)
{
	Entity *e = entity(L);
	if (e)
	{
		e->doFriction(lua_tonumber(L, 2), lua_tointeger(L, 3));
	}
	luaReturnNil();
}

luaFunc(entity_doGlint)
{
	Entity *e = entity(L);
	if (e)
		e->doGlint(e->position, Vector(2,2), getString(L,2), (RenderObject::BlendTypes)lua_tointeger(L, 3));
	luaReturnNil();
}

luaFunc(entity_getTarget)
{
	Entity *e = entity(L);
	Entity *retEnt = NULL;
	if (e)
	{
		retEnt = e->getTargetEntity(lua_tonumber(L, 2));
		//e->activate();
	}
	luaReturnPtr(retEnt);
}

luaFunc(entity_getTargetPositionX)
{
	Entity *e = entity(L);
	luaReturnInt(e ? e->getTargetEntity()->position.x : 0);
}

luaFunc(entity_getTargetPositionY)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->getTargetEntity()->position.y : 0);
}

luaFunc(entity_isNearObstruction)
{
	Entity *e = entity(L);
	int sz = lua_tonumber(L, 2);
	int type = lua_tointeger(L, 3);
	bool v = false;
	if (e)
	{
		v = e->isNearObstruction(sz, type);
	}
	luaReturnBool(v);
}

luaFunc(entity_isInvincible)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->isInvincible();
	}
	luaReturnBool(v);
}

luaFunc(entity_setEatType)
{
	Entity *e = entity(L);
	int et = lua_tointeger(L, 2);
	if (e)
		e->setEatType((EatType)et, getString(L, 3));
	luaReturnInt(et);
}

luaFunc(getMapName)
{
	luaReturnStr(dsq->game->sceneName.c_str());
}

luaFunc(isMapName)
{
	std::string s1 = dsq->game->sceneName;
	std::string s2 = getString(L, 1);
	stringToUpper(s1);
	stringToUpper(s2);
	bool ret = (s1 == s2);

	luaReturnBool(ret);
}

luaFunc(mapNameContains)
{
	std::string s = dsq->game->sceneName;
	stringToLower(s);
	bool b = (s.find(getString(L, 1)) != std::string::npos);
	luaReturnBool(b);
}

luaFunc(entity_fireGas)
{
	Entity *e = entity(L);
	if (e)
	{
		int radius = lua_tointeger(L, 2);
		float life = lua_tonumber(L, 3);
		float damage = lua_tonumber(L, 4);
		std::string gfx = getString(L, 5);
		float colorx = lua_tonumber(L, 6);
		float colory = lua_tonumber(L, 7);
		float colorz = lua_tonumber(L, 8);
		float offx = lua_tonumber(L, 9);
		float offy = lua_tonumber(L, 10);
		float poisonTime = lua_tonumber(L, 11);

		GasCloud *c = new GasCloud(e, e->position + Vector(offx, offy), gfx, Vector(colorx, colory, colorz), radius, life, damage, false, poisonTime);
		core->getTopStateData()->addRenderObject(c, LR_PARTICLES);
	}
	luaReturnNil();
}

luaFunc(isInputEnabled)
{
	luaReturnBool(dsq->game->avatar->isInputEnabled());
}

luaFunc(enableInput)
{
	dsq->game->avatar->enableInput();
	luaReturnNil();
}

luaFunc(disableInput)
{
	dsq->game->avatar->disableInput();
	luaReturnNil();
}

luaFunc(getInputMode)
{
	luaReturnInt(dsq->inputMode);
}

luaFunc(quit)
{
#ifdef AQUARIA_DEMO
	dsq->nag(NAG_QUIT);
#else
	dsq->quit();
#endif

	luaReturnNil();
}

luaFunc(doModSelect)
{
	dsq->doModSelect();
	luaReturnNil();
}

luaFunc(doLoadMenu)
{
	dsq->doLoadMenu();
	luaReturnNil();
}

luaFunc(resetContinuity)
{
	dsq->continuity.reset();
	luaReturnNil();
}

luaFunc(toWindowFromWorld)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	x = x - core->screenCenter.x;
	y = y - core->screenCenter.y;
	x *= core->globalScale.x;
	y *= core->globalScale.x;
	x = 400+x;
	y = 300+y;
	luaReturnVec2(x, y);
}

luaFunc(setMousePos)
{
	core->setMousePosition(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)));
	luaReturnNil();
}

luaFunc(getMousePos)
{
	luaReturnVec2(core->mouse.position.x, core->mouse.position.y);
}

luaFunc(getMouseWorldPos)
{
	Vector v = dsq->getGameCursorPosition();
	luaReturnVec2(v.x, v.y);
}

luaFunc(fade)
{
	dsq->overlay->color.interpolateTo(Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5)), lua_tonumber(L, 6));
	dsq->overlay->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(fade2)
{
	dsq->overlay2->color.interpolateTo(Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5)), lua_tonumber(L, 6));
	dsq->overlay2->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(fade3)
{
	dsq->overlay3->color.interpolateTo(Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5)), lua_tonumber(L, 6));
	dsq->overlay3->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(vision)
{
	dsq->vision(getString(L, 1), lua_tonumber(L, 2), getBool(L, 3));
	luaReturnNil();
}

luaFunc(musicVolume)
{
	dsq->sound->setMusicFader(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

luaFunc(voice)
{
	float vmod = lua_tonumber(L, 2);
	if (vmod == 0)
		vmod = -1;
	else if (vmod == -1)
		vmod = 0;
	dsq->voice(getString(L, 1), vmod);
	luaReturnNil();
}

luaFunc(voiceOnce)
{
	dsq->voiceOnce(getString(L, 1));
	luaReturnNil();
}

luaFunc(voiceInterupt)
{
	dsq->voiceInterupt(getString(L, 1));
	luaReturnNil();
}

luaFunc(stopVoice)
{
	dsq->stopVoice();
	luaReturnNil();
}

luaFunc(stopAllSfx)
{
	dsq->sound->stopAllSfx();
	luaReturnNil();
}

luaFunc(stopAllVoice)
{
	dsq->sound->stopAllVoice();
	luaReturnNil();
}

luaFunc(fadeIn)
{
	dsq->overlay->alpha.interpolateTo(0, lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(fadeOut)
{
	dsq->overlay->color = 0;
	dsq->overlay->alpha.interpolateTo(1, lua_tonumber(L, 1));
	luaReturnNil();
}

luaFunc(entity_setWeight)
{
	CollideEntity *e = collideEntity(L);
	if (e)
		e->weight = lua_tonumber(L, 2);
	luaReturnNil();
}

luaFunc(pickupGem)
{
	dsq->continuity.pickupGem(getString(L), !getBool(L, 2));
	luaReturnNil();
}

luaFunc(beaconEffect)
{
	int index = lua_tointeger(L, 1);

	BeaconData *b = dsq->continuity.getBeaconByIndex(index);

	float p1 = 0.7f;
	float p2 = 1.0f - p1;

	dsq->clickRingEffect(dsq->game->miniMapRender->getWorldPosition(), 0, (b->color*p1) + Vector(p2, p2, p2), 1);
	dsq->clickRingEffect(dsq->game->miniMapRender->getWorldPosition(), 1, (b->color*p1) + Vector(p2, p2, p2), 1);

	dsq->sound->playSfx("ping");

	luaReturnNil();
}

luaFunc(setBeacon)
{
	int index = lua_tointeger(L, 1);

	bool v = getBool(L, 2);

	Vector pos;
	pos.x = lua_tonumber(L, 3);
	pos.y = lua_tonumber(L, 4);

	Vector color;
	color.x = lua_tonumber(L, 5);
	color.y = lua_tonumber(L, 6);
	color.z = lua_tonumber(L, 7);

	dsq->continuity.setBeacon(index, v, pos, color);

	luaReturnNil();
}

luaFunc(getBeacon)
{
	int index = lua_tointeger(L, 1);
	bool v = false;

	if (dsq->continuity.getBeaconByIndex(index))
	{
		v = true;
	}

	luaReturnBool(v);
}

luaFunc(getCostume)
{
	luaReturnStr(dsq->continuity.costume.c_str());
}

luaFunc(setCostume)
{
	dsq->continuity.setCostume(getString(L));
	luaReturnNil();
}

luaFunc(setElementLayerVisible)
{
	int l = lua_tonumber(L, 1);
	bool v = getBool(L, 2);
	dsq->game->setElementLayerVisible(l, v);
	luaReturnNil();
}

luaFunc(isElementLayerVisible)
{
	luaReturnBool(dsq->game->isElementLayerVisible(lua_tonumber(L, 1)));
}

luaFunc(isStreamingVoice)
{
	bool v = dsq->sound->isPlayingVoice();
	luaReturnBool(v);
}

luaFunc(isObstructed)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	luaReturnBool(dsq->game->isObstructed(TileVector(Vector(x,y))));
}

luaFunc(getObstruction)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	luaReturnInt(dsq->game->getGrid(TileVector(Vector(x,y))));
}

luaFunc(isObstructedBlock)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	int span = lua_tonumber(L, 3);
	TileVector t(Vector(x,y));

	bool obs = false;
	for (int xx = t.x-span; xx < t.x+span; xx++)
	{
		for (int yy = t.y-span; yy < t.y+span; yy++)
		{
			if (dsq->game->isObstructed(TileVector(xx, yy)))
			{
				obs = true;
				break;
			}
		}
	}
	luaReturnBool(obs);
}

luaFunc(node_getFlag)
{
	Path *p = path(L);
	int v = 0;
	if (p)
	{
		v = dsq->continuity.getPathFlag(p);
	}
	luaReturnNum(v);
}

luaFunc(node_isFlag)
{
	Path *p = path(L);
	int c = lua_tonumber(L, 2);
	bool ret = false;
	if (p)
	{
		ret = (c == dsq->continuity.getPathFlag(p));
	}
	luaReturnBool(ret);
}

luaFunc(node_setFlag)
{
	Path *p = path(L);
	int v = lua_tonumber(L, 2);
	if (p)
	{
		dsq->continuity.setPathFlag(p, v);
	}
	luaReturnNum(v);
}

luaFunc(entity_isFlag)
{
	Entity *e = entity(L);
	int v = lua_tonumber(L, 2);
	bool b = false;
	if (e)
	{
		b = (dsq->continuity.getEntityFlag(dsq->game->sceneName, e->getID())==v);
	}
	luaReturnBool(b);
}

luaFunc(entity_setFlag)
{
	Entity *e = entity(L);
	int v = lua_tonumber(L, 2);
	if (e)
	{
		dsq->continuity.setEntityFlag(dsq->game->sceneName, e->getID(), v);
	}
	luaReturnNil();
}

luaFunc(entity_setPoison)
{
	Entity *e = entity(L);
	if(e)
		e->setPoison(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

luaFunc(entity_getPoison)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->getPoison() : 0.0f);
}

luaFunc(entity_getFlag)
{
	Entity *e = entity(L);
	int ret = 0;
	if (e)
	{
		ret = dsq->continuity.getEntityFlag(dsq->game->sceneName, e->getID());
	}
	luaReturnNum(ret);
}

luaFunc(isFlag)
{
	int v = 0;
	/*
	if (lua_isstring(L, 1))
		v = dsq->continuity.getFlag(lua_tostring(L, 1));
	else if (lua_isnumber(L, 1))
	*/
	bool f = false;
	if (lua_isnumber(L, 1))
	{
		v = dsq->continuity.getFlag(lua_tointeger(L, 1));
		f = (v == lua_tointeger(L, 2));
	}
	else
	{
		v = dsq->continuity.getFlag(getString(L, 1));
		f = (v == lua_tointeger(L, 2));
	}
	/*
	int f = 0;
	dsq->continuity.getFlag(lua_tostring(L, 1));

	*/
	luaReturnBool(f);
}

luaFunc(avatar_updatePosition)
{
	dsq->game->avatar->updatePosition();
	luaReturnNil();
}

luaFunc(avatar_toggleMovement)
{
	dsq->game->avatar->toggleMovement((bool)lua_tointeger(L, 1));
	luaReturnNil();
}

luaFunc(clearShots)
{
	Shot::killAllShots();
	luaReturnNil();
}

luaFunc(clearHelp)
{
	float t = 0.4;

	RenderObjectLayer *rl = &core->renderObjectLayers[LR_HELP];
	RenderObject *ro = rl->getFirst();
	while (ro)
	{
		ro->setLife(t);
		ro->setDecayRate(1);
		ro->alpha.stopPath();
		ro->alpha.interpolateTo(0,t-0.01f);

		ro = rl->getNext();
	}

	luaReturnNil();
}

luaFunc(setLiPower)
{
	float m = lua_tonumber(L, 1);
	float t = lua_tonumber(L, 2);
	dsq->continuity.setLiPower(m, t);
	luaReturnNil();
}

luaFunc(getLiPower)
{
	luaReturnNum(dsq->continuity.liPower);
}

luaFunc(getPetPower)
{
	luaReturnNum(dsq->continuity.petPower);
}

luaFunc(appendUserDataPath)
{
	std::string path = getString(L, 1);

	if (!dsq->getUserDataFolder().empty())
		path = dsq->getUserDataFolder() + "/" + path;

	luaReturnStr(path.c_str());
}

luaFunc(getScreenVirtualOff)
{
	luaReturnVec2(core->getVirtualOffX(), core->getVirtualOffY());
}

luaFunc(getScreenSize)
{
	luaReturnVec2(core->width, core->height);
}

luaFunc(createDebugText)
{
	DebugFont *txt = new DebugFont(lua_tointeger(L, 2), getString(L, 1));
	txt->position = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4));
	dsq->game->addRenderObject(txt, LR_DEBUG_TEXT);
	luaReturnPtr(txt);
}

luaFunc(createBitmapText)
{
	BmpFont *font = &dsq->smallFont; // make this configurable?
	BitmapText *txt = new BitmapText(font);
	txt->setText(getString(L, 1));
	txt->setFontSize(lua_tointeger(L, 2));
	txt->position = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4));
	dsq->game->addRenderObject(txt, LR_HUD);
	luaReturnPtr(txt);
}

luaFunc(text_setText)
{
	BaseText *txt = getText(L);
	if (txt)
		txt->setText(getString(L, 2));
	luaReturnNil();
}

luaFunc(text_setFontSize)
{
	BaseText *txt = getText(L);
	if (txt)
		txt->setFontSize(lua_tointeger(L, 2));
	luaReturnNil();
}

luaFunc(text_setWidth)
{
	BaseText *txt = getText(L);
	if (txt)
		txt->setWidth(lua_tointeger(L, 2));
	luaReturnNil();
}

luaFunc(loadShader)
{
	const char *vertRaw = getCString(L, 1);
	const char *fragRaw = getCString(L, 2);
	std::string vert, frag;
	if(vertRaw)
		findFile_helper(vertRaw, vert);
	if(fragRaw)
		findFile_helper(fragRaw, frag);
	Shader *sh = new Shader();
	sh->load(vert, frag);
	if(!sh->isLoaded())
	{
		delete sh;
		sh = NULL;
	}
	luaReturnPtr(sh);
}

luaFunc(createShader)
{
	Shader *sh = new Shader();
	sh->loadSrc(getCString(L, 1), getCString(L, 2));
	if(!sh->isLoaded())
	{
		delete sh;
		sh = NULL;
	}
	luaReturnPtr(sh);
}

luaFunc(shader_setAsAfterEffect)
{
	core->afterEffectManager->scriptShader = lua_isuserdata(L, 1) ? getShader(L, 1) : NULL;
	luaReturnNil();
}

luaFunc(shader_setInt)
{
	Shader *sh = getShader(L, 1);
	const char *name = getCString(L, 2);
	if(sh && name)
		sh->setInt(name, lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	luaReturnNil();
}

luaFunc(shader_setFloat)
{
	Shader *sh = getShader(L, 1);
	const char *name = getCString(L, 2);
	if(sh && name)
		sh->setFloat(name, lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	luaReturnNil();
}

luaFunc(shader_delete)
{
	Shader *sh = getShader(L);
	delete sh;
	if(core->afterEffectManager->scriptShader == sh)
		core->afterEffectManager->scriptShader = NULL;
	luaReturnNil();
}


//--------------------------------------------------------------------------------------------

#define luaRegister(func)	{#func, l_##func}

static const struct {
	const char *name;
	lua_CFunction func;
} luaFunctionTable[] = {

	// override Lua's standard dofile() and loadfile(), so we can handle filename case issues.
	{"dofile", l_dofile_caseinsensitive},
	{"loadfile", l_loadfile_caseinsensitive},

	luaRegister(shakeCamera),
	luaRegister(upgradeHealth),

	luaRegister(cureAllStatus),
	luaRegister(setPoison),
	luaRegister(setMusicToPlay),
	luaRegister(confirm),

	luaRegister(randRange),

	luaRegister(flingMonkey),


	luaRegister(setLiPower),
	luaRegister(getLiPower),
	luaRegister(getPetPower),
	luaRegister(getTimer),
	luaRegister(getHalfTimer),
	luaRegister(setCostume),
	luaRegister(getCostume),
	luaRegister(getNoteName),

	luaRegister(getWorldType),
	luaRegister(setWorldType),
	luaRegister(setWorldPaused),
	luaRegister(isWorldPaused),

	luaRegister(getWaterLevel),
	luaRegister(setWaterLevel),

	luaRegister(createQuad),
	luaRegister(quad_setPauseLevel),

	luaRegister(setupEntity),
	luaRegister(setActivePet),

	luaRegister(reconstructGrid),
	luaRegister(reconstructEntityGrid),

	luaRegister(ing_hasIET),

	luaRegister(esetv),
	luaRegister(esetvf),
	luaRegister(egetv),
	luaRegister(egetvf),
	luaRegister(eisv),

	luaRegister(entity_addIgnoreShotDamageType),
	luaRegister(entity_ensureLimit),
	luaRegister(entity_getBoneLockEntity),

	luaRegister(entity_setRidingPosition),
	luaRegister(entity_setRidingData),
	luaRegister(entity_setBoneLock),
	luaRegister(entity_setIngredient),
	luaRegister(entity_setDeathScene),
	luaRegister(entity_isDeathScene),

	luaRegister(entity_setBeautyFlip),
	luaRegister(entity_setInvincible),

	luaRegister(setInvincible),

	luaRegister(entity_setLookAtPoint),
	luaRegister(entity_getLookAtPoint),

	luaRegister(entity_setDieTimer),
	luaRegister(entity_setAutoSkeletalUpdate),
	luaRegister(entity_updateSkeletal),
	luaRegister(entity_setBounceType),

	luaRegister(entity_getHealthPerc),
	luaRegister(entity_getBounceType),
	luaRegister(entity_setRiding),
	luaRegister(entity_getRiding),

	luaRegister(entity_setNaijaReaction),

	luaRegister(entity_setEatType),

	luaRegister(entity_setSpiritFreeze),
	luaRegister(node_setSpiritFreeze),

	luaRegister(entity_setCanLeaveWater),

	luaRegister(entity_pullEntities),

	luaRegister(entity_setEntityLayer),

	luaRegister(entity_clearTargetPoints),
	luaRegister(entity_addTargetPoint),


	luaRegister(entity_setCullRadius),
	luaRegister(entity_setUpdateCull),

	luaRegister(entity_switchLayer),

	luaRegister(entity_debugText),


	luaRegister(avatar_setCanDie),
	luaRegister(setCanActivate),
	luaRegister(setSeeMapMode),
	luaRegister(avatar_toggleCape),
	luaRegister(avatar_setPullTarget),

	luaRegister(avatar_setCanLockToWall),
	luaRegister(avatar_canLockToWall),
	luaRegister(avatar_setCanBurst),
	luaRegister(avatar_canBurst),
	luaRegister(avatar_setCanSwimAgainstCurrents),
	luaRegister(avatar_canSwimAgainstCurrents),
	luaRegister(avatar_setCanCollideWithShots),
	luaRegister(avatar_canCollideWithShots),
	luaRegister(avatar_setCollisionAvoidanceData),

	luaRegister(avatar_clampPosition),
	luaRegister(avatar_updatePosition),

	luaRegister(pause),
	luaRegister(unpause),
	luaRegister(isPaused),
	luaRegister(isInGameMenu),
	luaRegister(isInEditor),


	luaRegister(vector_normalize),
	luaRegister(vector_setLength),
	luaRegister(vector_getLength),

	luaRegister(vector_dot),

	luaRegister(vector_isLength2DIn),
	luaRegister(vector_cap),


	luaRegister(entity_setDeathParticleEffect),
	luaRegister(entity_setDeathSound),

	luaRegister(entity_setDamageTarget),
	luaRegister(entity_setAllDamageTargets),

	luaRegister(entity_isDamageTarget),
	luaRegister(entity_isValidTarget),

	luaRegister(entity_isUnderWater),
	luaRegister(entity_checkSplash),

	luaRegister(entity_getRandomTargetPoint),
	luaRegister(entity_getTargetPoint),

	luaRegister(entity_setTargetRange),

	luaRegister(bone_addSegment),

	luaRegister(bone_setSegmentOffset),
	luaRegister(bone_setSegmentProps),
	luaRegister(bone_setSegmentChainHead),
	luaRegister(bone_setAnimated),
	luaRegister(bone_showFrame),

	luaRegister(bone_lookAtEntity),
	luaRegister(bone_lookAtPosition),

	luaRegister(entity_partSetSegs),

	luaRegister(entity_adjustPositionBySurfaceNormal),
	luaRegister(entity_applySurfaceNormalForce),

	luaRegister(createBeam),
	luaRegister(beam_setAngle),
	luaRegister(beam_setDamage),
	luaRegister(beam_setBeamWidth),
	luaRegister(beam_setFirer),
	luaRegister(beam_setDamageType),

	luaRegister(getStringBank),

	luaRegister(isPlat),


	luaRegister(createEntity),
	luaRegister(entity_setWeight),

	luaRegister(entity_setActivationType),

	luaRegister(isQuitFlag),
	luaRegister(isDeveloperKeys),
	luaRegister(isDemo),

	luaRegister(isInputEnabled),
	luaRegister(disableInput),

	luaRegister(getInputMode),

	luaRegister(setMousePos),
	luaRegister(getMousePos),
	luaRegister(getMouseWorldPos),

	luaRegister(resetContinuity),

	luaRegister(quit),
	luaRegister(doModSelect),
	luaRegister(doLoadMenu),


	luaRegister(enableInput),
	luaRegister(fade),
	luaRegister(fade2),
	luaRegister(fade3),

	luaRegister(getMapName),
	luaRegister(isMapName),
	luaRegister(mapNameContains),

	luaRegister(entity_getAimVector),

	luaRegister(entity_getVectorToEntity),

	luaRegister(entity_getDistanceToTarget),
	luaRegister(entity_getDistanceToPoint),
	luaRegister(entity_move),

	luaRegister(entity_getID),

	luaRegister(getEntityByID),

	luaRegister(entity_setBounce),
	luaRegister(entity_setActivation),
	luaRegister(entity_rotateToEntity),

	luaRegister(entity_fireGas),
	luaRegister(entity_rotateToTarget),

	luaRegister(entity_switchSurfaceDirection),

	luaRegister(entity_moveAlongSurface),
	luaRegister(entity_rotateToSurfaceNormal),
	luaRegister(entity_clampToSurface),
	luaRegister(entity_checkSurface),
	luaRegister(entity_clampToHit),

	luaRegister(entity_grabTarget),
	luaRegister(entity_releaseTarget),

	luaRegister(entity_getStateTime),
	luaRegister(entity_setStateTime),

	luaRegister(entity_doFriction),

	luaRegister(entity_partWidthHeight),
	luaRegister(entity_partBlendType),
	luaRegister(entity_partRotate),
	luaRegister(entity_partAlpha),

	luaRegister(entity_getHealth),
	luaRegister(entity_getMaxHealth),
	luaRegister(entity_pushTarget),
	luaRegister(entity_msg),
	luaRegister(entity_updateMovement),
	luaRegister(entity_updateCurrents),
	luaRegister(entity_updateLocalWarpAreas),

	luaRegister(entity_getTargetPositionX),
	luaRegister(entity_getTargetPositionY),

	luaRegister(avatar_incrLeaches),
	luaRegister(avatar_decrLeaches),
	luaRegister(entity_rotateToVel),
	luaRegister(entity_rotateToVec),

	luaRegister(entity_setSegsMaxDist),

	luaRegister(entity_offsetUpdate),

	luaRegister(entity_createEntity),
	luaRegister(entity_resetTimer),
	luaRegister(entity_stopTimer),
	luaRegister(entity_stopPull),
	luaRegister(entity_setTargetPriority),

	luaRegister(entity_setEntityType),
	luaRegister(entity_getEntityType),

	luaRegister(entity_setSegmentTexture),

	luaRegister(entity_spawnParticlesFromCollisionMask),
	luaRegister(entity_initEmitter),
	luaRegister(entity_startEmitter),
	luaRegister(entity_stopEmitter),

	luaRegister(entity_initPart),
	luaRegister(entity_initSegments),
	luaRegister(entity_warpSegments),
	luaRegister(entity_initSkeletal),
	luaRegister(entity_loadSkin),
	luaRegister(entity_getSkeletalName),
	luaRegister(entity_initStrands),

	luaRegister(entity_hurtTarget),
	luaRegister(entity_doSpellAvoidance),
	luaRegister(entity_doEntityAvoidance),
	luaRegister(entity_rotate),
	luaRegister(entity_doGlint),
	luaRegister(entity_findTarget),
	luaRegister(entity_hasTarget),
	luaRegister(entity_isInRect),
	luaRegister(entity_isInDarkness),

	luaRegister(entity_isRidingOnEntity),

	luaRegister(entity_isBeingPulled),

	luaRegister(entity_isNearObstruction),
	luaRegister(entity_isDead),

	luaRegister(entity_isTargetInRange),
	luaRegister(entity_getDistanceToEntity),
	luaRegister(entity_isEntityInside),

	luaRegister(entity_isInvincible),

	luaRegister(entity_isNearGround),

	luaRegister(entity_moveTowardsTarget),
	luaRegister(entity_moveAroundTarget),

	luaRegister(entity_moveTowardsAngle),
	luaRegister(entity_moveAroundAngle),
	luaRegister(entity_moveTowards),
	luaRegister(entity_moveAround),

	luaRegister(entity_setMaxSpeed),
	luaRegister(entity_getMaxSpeed),
	luaRegister(entity_setMaxSpeedLerp),
	luaRegister(entity_getMaxSpeedLerp),
	luaRegister(entity_setState),
	luaRegister(entity_getState),
	luaRegister(entity_getEnqueuedState),

	luaRegister(entity_getPrevState),
	luaRegister(entity_doCollisionAvoidance),
	luaRegister(entity_animate),
	luaRegister(entity_setAnimLayerTimeMult),
	luaRegister(entity_getAnimLayerTimeMult),
	luaRegister(entity_stopAnimation),
	luaRegister(entity_getAnimationLoop),

	luaRegister(entity_setCurrentTarget),
	luaRegister(entity_stopInterpolating),

	luaRegister(entity_followPath),
	luaRegister(entity_isFollowingPath),
	luaRegister(entity_followEntity),
	luaRegister(entity_sound),
	luaRegister(entity_playSfx),

	luaRegister(registerSporeDrop),

	luaRegister(getIngredientGfx),

	luaRegister(spawnIngredient),
	luaRegister(spawnAllIngredients),
	luaRegister(spawnParticleEffect),
	luaRegister(spawnManaBall),

	luaRegister(isEscapeKey),

	luaRegister(resetTimer),

	luaRegister(addInfluence),
	luaRegister(setupBasicEntity),
	luaRegister(playMusic),
	luaRegister(playMusicStraight),
	luaRegister(stopMusic),

	luaRegister(user_set_demo_intro),
	luaRegister(user_save),

	luaRegister(playMusicOnce),

	luaRegister(playSfx),
	luaRegister(fadeSfx),

	luaRegister(emote),

	luaRegister(playVisualEffect),
	luaRegister(playNoEffect),
	luaRegister(createShockEffect),

	luaRegister(setOverrideMusic),

	luaRegister(setOverrideVoiceFader),
	luaRegister(setGameSpeed),
	luaRegister(warpAvatar),
	luaRegister(warpNaijaToSceneNode),

	luaRegister(toWindowFromWorld),

	luaRegister(toggleDamageSprite),

	luaRegister(toggleLiCombat),

	luaRegister(toggleCursor),
	luaRegister(toggleBlackBars),
	luaRegister(setBlackBarsColor),

	luaRegister(entityFollowEntity),

	luaRegister(setMiniMapHint),
	luaRegister(bedEffects),

	luaRegister(warpNaijaToEntity),

	luaRegister(setNaijaHeadTexture),

	luaRegister(incrFlag),
	luaRegister(decrFlag),
	luaRegister(setFlag),
	luaRegister(getFlag),
	luaRegister(setStringFlag),
	luaRegister(getStringFlag),
	luaRegister(learnSong),
	luaRegister(unlearnSong),
	luaRegister(hasSong),
	luaRegister(hasLi),

	luaRegister(setCanWarp),
	luaRegister(setCanChangeForm),
	luaRegister(setInvincibleOnNested),

	luaRegister(setControlHint),
	luaRegister(setCameraLerpDelay),
	luaRegister(screenFadeGo),
	luaRegister(screenFadeTransition),
	luaRegister(screenFadeCapture),

	luaRegister(clearControlHint),


	luaRegister(savePoint),
	luaRegister(wait),
	luaRegister(watch),

	luaRegister(quitNestedMain),
	luaRegister(isNestedMain),


	luaRegister(msg),
	luaRegister(centerText),
	luaRegister(watchForVoice),

	luaRegister(setElementLayerVisible),
	luaRegister(isElementLayerVisible),

	luaRegister(isWithin),



	luaRegister(pickupGem),
	luaRegister(setBeacon),
	luaRegister(getBeacon),
	luaRegister(beaconEffect),

	luaRegister(chance),

	luaRegister(goToTitle),
	luaRegister(jumpState),
	luaRegister(getEnqueuedState),


	luaRegister(fadeIn),
	luaRegister(fadeOut),

	luaRegister(vision),

	luaRegister(musicVolume),

	luaRegister(voice),
	luaRegister(voiceOnce),
	luaRegister(voiceInterupt),


	luaRegister(stopVoice),
	luaRegister(stopAllVoice),
	luaRegister(stopAllSfx),



	luaRegister(fadeOutMusic),


	luaRegister(isStreamingVoice),

	luaRegister(changeForm),
	luaRegister(getForm),
	luaRegister(isForm),
	luaRegister(learnFormUpgrade),
	luaRegister(hasFormUpgrade),


	luaRegister(singSong),
	luaRegister(isObstructed),
	luaRegister(isObstructedBlock),
	luaRegister(getObstruction),

	luaRegister(isFlag),

	luaRegister(entity_isFlag),
	luaRegister(entity_setFlag),
	luaRegister(entity_getFlag),

	luaRegister(node_isFlag),
	luaRegister(node_setFlag),
	luaRegister(node_getFlag),

	luaRegister(avatar_getStillTimer),
	luaRegister(avatar_getSpellCharge),

	luaRegister(avatar_isSinging),
	luaRegister(avatar_isTouchHit),
	luaRegister(avatar_isBursting),
	luaRegister(avatar_isLockable),
	luaRegister(avatar_isRolling),
	luaRegister(avatar_isOnWall),
	luaRegister(avatar_isShieldActive),
	luaRegister(avatar_setShieldActive),
	luaRegister(avatar_getRollDirection),

	luaRegister(avatar_fallOffWall),
	luaRegister(avatar_setBlockSinging),
	luaRegister(avatar_isBlockSinging),


	luaRegister(avatar_toggleMovement),


	luaRegister(showInGameMenu),
	luaRegister(hideInGameMenu),


	luaRegister(showImage),
	luaRegister(hideImage),
	luaRegister(clearHelp),
	luaRegister(clearShots),



	luaRegister(getEntity),
	luaRegister(getFirstEntity),
	luaRegister(getNextEntity),

	luaRegister(setStory),
	luaRegister(getStory),
	luaRegister(getNoteColor),
	luaRegister(getNoteVector),
	luaRegister(getRandNote),

	luaRegister(foundLostMemory),

	luaRegister(isStory),

	luaRegister(entity_damage),
	luaRegister(entity_heal),

	luaRegister(getNearestIngredient),

	luaRegister(getNearestNodeByType),

	luaRegister(getNode),
	luaRegister(getNodeToActivate),
	luaRegister(setNodeToActivate),
	luaRegister(setActivation),

	luaRegister(entity_warpToNode),
	luaRegister(entity_moveToNode),

	luaRegister(cam_toNode),
	luaRegister(cam_snap),
	luaRegister(cam_toEntity),
	luaRegister(cam_setPosition),


	luaRegister(entity_flipToEntity),
	luaRegister(entity_flipToSame),

	luaRegister(entity_flipToNode),
	luaRegister(entity_flipToVel),

	luaRegister(entity_swimToNode),
	luaRegister(entity_swimToPosition),


	luaRegister(createShot),

	luaRegister(entity_isHit),



	luaRegister(createWeb),
	luaRegister(web_addPoint),
	luaRegister(web_setPoint),
	luaRegister(web_getNumPoints),

	luaRegister(createSpore),

	luaRegister(getFirstShot),
	luaRegister(getNextShot),
	luaRegister(shot_setAimVector),
	luaRegister(shot_setOut),
	luaRegister(shot_getEffectTime),
	luaRegister(shot_isIgnoreShield),
	luaRegister(shot_setFirer),
	luaRegister(shot_getFirer),
	luaRegister(shot_setTarget),
	luaRegister(shot_getTarget),
	luaRegister(shot_setExtraDamage),
	luaRegister(shot_getExtraDamage),
	luaRegister(shot_getDamage),
	luaRegister(shot_getDamageType),
	luaRegister(shot_getName),
	luaRegister(entity_pathBurst),
	luaRegister(entity_handleShotCollisions),
	luaRegister(entity_handleShotCollisionsSkeletal),
	luaRegister(entity_handleShotCollisionsHair),
	luaRegister(entity_collideSkeletalVsCircle),
	luaRegister(entity_collideSkeletalVsLine),
	luaRegister(entity_collideSkeletalVsCircleForListByName),
	luaRegister(entity_collideCircleVsLine),
	luaRegister(entity_collideCircleVsLineAngle),

	luaRegister(entity_collideHairVsCircle),

	luaRegister(entity_setDropChance),

	luaRegister(entity_waitForPath),
	luaRegister(entity_watchForPath),

	luaRegister(entity_revive),

	luaRegister(entity_getTarget),
	luaRegister(entity_isState),

	luaRegister(entity_setProperty),
	luaRegister(entity_isProperty),


	luaRegister(entity_initHair),
	luaRegister(entity_getHairPosition),
	luaRegister(entity_getHair),
	luaRegister(entity_clearHair),

	luaRegister(entity_setHairHeadPosition),
	luaRegister(entity_updateHair),
	luaRegister(entity_exertHairForce),

	luaRegister(entity_setName),

	luaRegister(getNumberOfEntitiesNamed),

	luaRegister(isNested),

	luaRegister(entity_idle),
	luaRegister(entity_stopAllAnimations),

	luaRegister(entity_getBoneByIdx),
	luaRegister(entity_getBoneByName),



	luaRegister(toggleInput),

	luaRegister(entity_setTarget),

	luaRegister(getScreenCenter),



	luaRegister(debugLog),
	luaRegister(errorLog),
	luaRegister(loadMap),

	luaRegister(loadSound),

	luaRegister(node_activate),
	luaRegister(node_getName),
	luaRegister(node_getLabel),
	luaRegister(node_getPathPosition),
	luaRegister(node_getPosition),
	luaRegister(node_setPosition),
	luaRegister(node_getContent),
	luaRegister(node_getAmount),
	luaRegister(node_getSize),
	luaRegister(node_setEffectOn),
	luaRegister(node_isEffectOn),

	luaRegister(toggleSteam),
	luaRegister(toggleVersionLabel),
	luaRegister(setVersionLabelText),

	luaRegister(appendUserDataPath),

	luaRegister(setCutscene),
	luaRegister(isInCutscene),



	luaRegister(node_getNumEntitiesIn),


	luaRegister(entity_getName),
	luaRegister(entity_isName),


	luaRegister(node_setCursorActivation),
	luaRegister(node_setCatchActions),

	luaRegister(node_setElementsInLayerActive),


	luaRegister(entity_setHealth),
	luaRegister(entity_changeHealth),

	luaRegister(node_setActive),


	luaRegister(setSceneColor),
	luaRegister(getSceneColor),

	luaRegister(entity_watchEntity),

	luaRegister(entity_setCollideRadius),
	luaRegister(entity_getCollideRadius),

	luaRegister(entity_isEntityInRange),
	luaRegister(entity_isPositionInRange),

	luaRegister(entity_stopFollowingPath),
	luaRegister(entity_slowToStopPath),
	luaRegister(entity_isSlowingToStopPath),

	luaRegister(entity_findNearestEntityOfType),
	luaRegister(entity_isFollowingEntity),
	luaRegister(entity_resumePath),

	luaRegister(entity_generateCollisionMask),

	luaRegister(entity_isAnimating),
	luaRegister(entity_getAnimationName),
	luaRegister(entity_getAnimationLength),

	luaRegister(entity_setCull),

	luaRegister(entity_setTexture),
	luaRegister(entity_setFillGrid),
	luaRegister(entity_isFillGrid),

	luaRegister(entity_push),

	luaRegister(entity_alpha),

	luaRegister(findWall),


	luaRegister(overrideZoom),
	luaRegister(disableOverrideZoom),



	luaRegister(spawnAroundEntity),

	luaRegister(entity_toggleBone),

	luaRegister(bone_getName),
	luaRegister(bone_isName),
	luaRegister(bone_getIndex),
	luaRegister(node_x),
	luaRegister(node_y),
	luaRegister(node_isEntityPast),
	luaRegister(node_isEntityInRange),
	luaRegister(node_isPositionIn),



	luaRegister(entity_warpLastPosition),
	luaRegister(entity_x),
	luaRegister(entity_y),
	luaRegister(entity_setVel),
	luaRegister(entity_setVelLen),
	luaRegister(entity_getVelLen),
	luaRegister(entity_getVel),
	luaRegister(entity_velx),
	luaRegister(entity_vely),
	luaRegister(entity_addVel),
	luaRegister(entity_addRandomVel),
	luaRegister(entity_isVelIn),
	luaRegister(entity_clearVel),
	luaRegister(entity_velTowards),

	luaRegister(entity_setVel2),
	luaRegister(entity_setVel2Len),
	luaRegister(entity_getVel2Len),
	luaRegister(entity_addVel2),
	luaRegister(entity_getVel2),
	luaRegister(entity_clearVel2),



	luaRegister(updateMusic),

	luaRegister(entity_touchAvatarDamage),
	luaRegister(getNaija),
	luaRegister(getLi),
	luaRegister(setLi),

	luaRegister(randAngle360),
	luaRegister(randVector),

	luaRegister(entity_getNearestEntity),
	luaRegister(entity_getNearestBoneToPosition),
	luaRegister(getNearestEntity),

	luaRegister(entity_getNearestNode),
	luaRegister(entity_setPoison),
	luaRegister(entity_getPoison),

	luaRegister(node_getNearestEntity),
	luaRegister(node_getNearestNode),


	luaRegister(node_isEntityIn),



	luaRegister(isLeftMouse),
	luaRegister(isRightMouse),


	luaRegister(setTimerTextAlpha),
	luaRegister(setTimerText),


	luaRegister(getWallNormal),
	luaRegister(getLastCollidePosition),
	luaRegister(getLastCollideTileType),
	luaRegister(collideCircleWithGrid),

	luaRegister(getScreenVirtualOff),
	luaRegister(getScreenSize),

	luaRegister(createDebugText),
	luaRegister(createBitmapText),
	luaRegister(text_setText),
	luaRegister(text_setFontSize),
	luaRegister(text_setWidth),

	luaRegister(loadShader),
	luaRegister(createShader),
	luaRegister(shader_setAsAfterEffect),
	luaRegister(shader_setFloat),
	luaRegister(shader_setInt),
	luaRegister(shader_delete),

	luaRegister(isQuad),
	luaRegister(isNode),
	luaRegister(isObject),
	luaRegister(isEntity),
	luaRegister(isShot),
	luaRegister(isWeb),
	luaRegister(isIng),
	luaRegister(isBeam),
	luaRegister(isText),


#undef MK_FUNC
#undef MK_ALIAS
#define MK_FUNC(base, getter, prefix, suffix) luaRegister(prefix##_##suffix),
#define MK_STR(s) #s
#define MK_ALIAS(prefix, suffix, alias) {MK_STR(prefix)"_"MK_STR(alias), l_##prefix##_##suffix},

	EXPAND_FUNC_PROTOTYPES

	// obj_* are not in the define above
	MAKE_ROBJ_FUNCS(_, obj)
	// same for quad_* base functions
	MAKE_QUAD_FUNCS(_, quad)

	// -- overrides / special cases--

	{"bone_getPosition", l_bone_getWorldPosition},
	{ "entity_delete", l_entity_delete_override },
	{ "entity_setRenderPass", l_entity_setRenderPass_override },
	{ "beam_setPosition", l_beam_setPosition_override },

	// -- deprecated/compatibility related functions below here --

	{"entity_incrTargetLeaches", l_avatar_incrLeaches},
	{"entity_decrTargetLeaches", l_avatar_decrLeaches},
	{"entity_soundFreq", l_entity_sound},
	{"entity_interpolateTo", l_entity_setPosition},
	{"entity_isFlippedHorizontal", l_entity_isfh},
	{"entity_isFlippedVertical", l_entity_isfv},
	{"entity_rotateTo", l_entity_rotate},
	{"entity_setColor", l_entity_color},
	{"entity_setInternalOffset", l_entity_internalOffset},

	{"bone_setColor", l_bone_color},

};

//============================================================================================
// S C R I P T  C O N S T A N T S
//============================================================================================

#define luaConstant(name)					{#name, name}
#define luaConstantFromClass(name,class)	{#name, class::name}

static const struct {
	const char *name;
	lua_Number value;
} luaConstantTable[] = {

	{"AQUARIA_VERSION", VERSION_MAJOR*10000 + VERSION_MINOR*100 + VERSION_REVISION},

	// emotes
	luaConstant(EMOTE_NAIJAEVILLAUGH),
	luaConstant(EMOTE_NAIJAGIGGLE),
	luaConstant(EMOTE_NAIJALAUGH),
	luaConstant(EMOTE_NAIJASADSIGH),
	luaConstant(EMOTE_NAIJASIGH),
	luaConstant(EMOTE_NAIJAWOW),
	luaConstant(EMOTE_NAIJAUGH),
	luaConstant(EMOTE_NAIJALOW),
	luaConstant(EMOTE_NAIJALI),
	{"EMOTE_NAIJAEW",			9},  // FIXME: unused

	// Li expressions
	{"EXPRESSION_NORMAL",		0},
	{"EXPRESSION_ANGRY",		1},
	{"EXPRESSION_HAPPY",		2},
	{"EXPRESSION_HURT",			3},
	{"EXPRESSION_LAUGH",		4},
	{"EXPRESSION_SURPRISE",		5},

	luaConstantFromClass(OVERRIDE_NONE,	RenderObject),

	//actions
	luaConstant(ACTION_MENULEFT),
	luaConstant(ACTION_MENURIGHT),
	luaConstant(ACTION_MENUUP),
	luaConstant(ACTION_MENUDOWN),

	{"WATCH_QUIT",				1},

	{"BEACON_HOMECAVE",			1},
	{"BEACON_ENERGYTEMPLE",		2},
	{"BEACON_MITHALAS",			3},
	{"BEACON_FOREST",			4},
	{"BEACON_LI",				5},
	{"BEACON_SUNTEMPLE",		6},
	{"BEACON_SONGCAVE",			7},

	{"PLAT_WIN",				0},
	{"PLAT_MAC",				1},
	{"PLAT_LNX",				2},

	// ingredient effect types
	luaConstant(IET_NONE),
	luaConstant(IET_HP),
	luaConstant(IET_DEFENSE),
	luaConstant(IET_SPEED),
	luaConstant(IET_RANDOM),
	luaConstant(IET_MAXHP),
	luaConstant(IET_INVINCIBLE),
	luaConstant(IET_TRIP),
	luaConstant(IET_REGEN),
	luaConstant(IET_LI),
	luaConstant(IET_FISHPOISON),
	luaConstant(IET_BITE),
	luaConstant(IET_EAT),
	luaConstant(IET_LIGHT),
	luaConstant(IET_YUM),
	luaConstant(IET_PETPOWER),
	luaConstant(IET_WEB),
	luaConstant(IET_ENERGY),
	luaConstant(IET_POISON),
	luaConstant(IET_BLIND),
	luaConstant(IET_ALLSTATUS),
	luaConstant(IET_MAX),

	// menu pages
	luaConstant(MENUPAGE_NONE),
	luaConstant(MENUPAGE_SONGS),
	luaConstant(MENUPAGE_FOOD),
	luaConstant(MENUPAGE_TREASURES),
	luaConstant(MENUPAGE_PETS),

	// Entity States
	luaConstantFromClass(STATE_DEAD,		Entity),
	luaConstantFromClass(STATE_IDLE,		Entity),
	luaConstantFromClass(STATE_PUSH,		Entity),
	luaConstantFromClass(STATE_PUSHDELAY,	Entity),
	luaConstantFromClass(STATE_PLANTED,		Entity),
	luaConstantFromClass(STATE_PULLED,		Entity),
	luaConstantFromClass(STATE_FOLLOWNAIJA,	Entity),
	luaConstantFromClass(STATE_DEATHSCENE,	Entity),
	luaConstantFromClass(STATE_ATTACK,		Entity),
	luaConstantFromClass(STATE_CHARGE0,		Entity),
	luaConstantFromClass(STATE_CHARGE1,		Entity),
	luaConstantFromClass(STATE_CHARGE2,		Entity),
	luaConstantFromClass(STATE_CHARGE3,		Entity),
	luaConstantFromClass(STATE_WAIT,		Entity),
	luaConstantFromClass(STATE_HUG,			Entity),
	luaConstantFromClass(STATE_EATING,		Entity),
	luaConstantFromClass(STATE_FOLLOW,		Entity),
	luaConstantFromClass(STATE_TITLE,		Entity),
	// Remainder are script-specific, not used by C++ code
	{"STATE_HATCH",			25},
	{"STATE_CARRIED",		26},

	{"STATE_HOSTILE",		100},

	{"STATE_CLOSE",			200},
	{"STATE_OPEN",			201},
	{"STATE_CLOSED",		202},
	{"STATE_OPENED",		203},
	{"STATE_CHARGED",		300},
	{"STATE_INHOLDER",		301},
	{"STATE_DISABLED",		302},
	{"STATE_FLICKER",		303},
	{"STATE_ACTIVE",		304},
	{"STATE_USED",			305},
	{"STATE_BLOATED",		306},
	{"STATE_DELAY",			307},
	{"STATE_DONE",			309},
	{"STATE_RAGE",			310},
	{"STATE_CALM",			311},
	{"STATE_DESCEND",		312},
	{"STATE_SING",			313},
	{"STATE_TRANSFORM",		314},
	{"STATE_GROW",			315},
	{"STATE_MATING",		316},
	{"STATE_SHRINK",		317},
	{"STATE_MOVE",			319},
	{"STATE_TRANSITION",	320},
	{"STATE_TRANSITION2",	321},
	{"STATE_TRAPPEDINCREATOR",	322},
	{"STATE_GRAB",			323},
	{"STATE_FIGURE",		324},
	{"STATE_CUTSCENE",		325},
	{"STATE_WAITFORCUTSCENE",	326},
	{"STATE_FIRE",			327},
	{"STATE_FIRING",		328},
	{"STATE_PREP",			329},
	{"STATE_INTRO",			330},
	{"STATE_PUPPET",		331},

	{"STATE_COLLECT",				400},
	{"STATE_COLLECTED",				401},
	{"STATE_COLLECTEDINHOUSE",		402},


	//{"STATE_ATTACK"},		500},
	{"STATE_STEP",			501},
	{"STATE_AWAKEN",		502},

	{"STATE_WEAK",			600},
	{"STATE_BREAK",			601},
	{"STATE_BROKEN",		602},

	{"STATE_PULSE",			700},
	{"STATE_ON",			701},
	{"STATE_OFF",			702},
	{"STATE_SEED",			703},
	{"STATE_PLANTED",		704},
	{"STATE_SK_RED",		705},
	{"STATE_SK_GREEN",		706},
	{"STATE_SK_BLUE",		707},
	{"STATE_SK_YELLOW",		708},
	{"STATE_WAITFORKISS",	710},
	{"STATE_KISS",			711},
	{"STATE_START",			712},
	{"STATE_RACE",			714},
	{"STATE_RESTART",		715},
	{"STATE_APPEAR",		716},

	{"STATE_MOVETOWEED",	2000},
	{"STATE_PULLWEED",		2001},
	{"STATE_DONEWEED",		2002},

	{"ORIENT_NONE",			-1},
	{"ORIENT_LEFT",			0},
	{"ORIENT_RIGHT",		1},
	{"ORIENT_UP",			2},
	{"ORIENT_DOWN",			3},
	{"ORIENT_HORIZONTAL",	4},
	{"ORIENT_VERTICAL",		5},

	// for entity_isNearObstruction
	luaConstant(OBSCHECK_RANGE),
	luaConstant(OBSCHECK_4DIR),
	luaConstant(OBSCHECK_DOWN),
	luaConstant(OBSCHECK_8DIR),

	luaConstant(EV_WALLOUT),
	luaConstant(EV_WALLTRANS),
	luaConstant(EV_CLAMPING),
	luaConstant(EV_SWITCHCLAMP),
	luaConstant(EV_CLAMPTRANSF),
	luaConstant(EV_MOVEMENT),
	luaConstant(EV_COLLIDE),
	luaConstant(EV_TOUCHDMG),
	luaConstant(EV_FRICTION),
	luaConstant(EV_LOOKAT),
	luaConstant(EV_CRAWLING),
	luaConstant(EV_ENTITYDIED),
	luaConstant(EV_TYPEID),
	luaConstant(EV_COLLIDELEVEL),
	luaConstant(EV_BONELOCKED),
	luaConstant(EV_FLIPTOPATH),
	luaConstant(EV_NOINPUTNOVEL),
	luaConstant(EV_VINEPUSH),
	luaConstant(EV_BEASTBURST),
	luaConstant(EV_MINIMAP),
	luaConstant(EV_SOULSCREAMRADIUS),
	luaConstant(EV_WEBSLOW),
	luaConstant(EV_NOAVOID),
	luaConstant(EV_MAX),

	{"EVT_NONE",				0},
	{"EVT_THERMALVENT",			1},
	{"EVT_GLOBEJELLY",			2},
	{"EVT_CELLWHITE",			3},
	{"EVT_CELLRED",				4},
	{"EVT_PET",					5},
	{"EVT_DARKLISHOT",			6},
	{"EVT_ROCK",				7},
	{"EVT_FORESTGODVINE",		8},
	{"EVT_CONTAINER",			9},
	{"EVT_PISTOLSHRIMP",		10},
	{"EVT_GATEWAYMUTANT",		11},


	// PATH/node types
	luaConstant(PATH_NONE),
	luaConstant(PATH_CURRENT),
	luaConstant(PATH_STEAM),
	luaConstant(PATH_LI),
	luaConstant(PATH_SAVEPOINT),
	luaConstant(PATH_WARP),
	luaConstant(PATH_SPIRITPORTAL),
	luaConstant(PATH_BGSFXLOOP),
	luaConstant(PATH_RADARHIDE),
	luaConstant(PATH_COOK),
	luaConstant(PATH_WATERBUBBLE),
	luaConstant(PATH_GEM),
	luaConstant(PATH_SETING),
	luaConstant(PATH_SETENT),

	// Entity Types
	luaConstant(ET_AVATAR),
	luaConstant(ET_ENEMY),
	luaConstant(ET_PET),
	luaConstant(ET_FLOCK),
	luaConstant(ET_NEUTRAL),
	luaConstant(ET_INGREDIENT),

	luaConstant(EP_SOLID),
	luaConstant(EP_MOVABLE),
	luaConstant(EP_BATTERY),
	luaConstant(EP_BLOCKER),

	// ACTIVATION TYPES
	{"AT_NONE",				-1},
	{"AT_NORMAL",			0},
	{"AT_CLICK",			0},
	{"AT_RANGE",			1},

	luaConstant(WT_NORMAL),
	luaConstant(WT_SPIRIT),

	{"SPEED_NORMAL",		0},
	{"SPEED_SLOW",			1},
	{"SPEED_FAST",			2},
	{"SPEED_VERYFAST",		3},
	{"SPEED_MODSLOW",		4},
	{"SPEED_VERYSLOW",		5},
	{"SPEED_FAST2",			6},
	{"SPEED_LITOCAVE",		7},

	luaConstant(BOUNCE_NONE),
	luaConstant(BOUNCE_SIMPLE),
	luaConstant(BOUNCE_REAL),

	{"LOOP_INFINITE",		-1},
	{"LOOP_INF",			-1},

	{"LAYER_BODY",			0},
	{"LAYER_UPPERBODY",		1},
	{"LAYER_HEAD",			2},
	{"LAYER_OVERRIDE",		3},

	luaConstant(SONG_NONE),
	luaConstant(SONG_HEAL),
	luaConstant(SONG_ENERGYFORM),
	luaConstant(SONG_SONGDOOR1),
	luaConstant(SONG_SPIRITFORM),
	luaConstant(SONG_BIND),
	{"SONG_PULL",				SONG_BIND},
	luaConstant(SONG_NATUREFORM),
	luaConstant(SONG_BEASTFORM),
	luaConstant(SONG_SHIELDAURA),
	{"SONG_SHIELD",				SONG_SHIELDAURA},
	luaConstant(SONG_SONGDOOR2),
	luaConstant(SONG_DUALFORM),
	luaConstant(SONG_FISHFORM),
	luaConstant(SONG_SUNFORM),
	{"SONG_LIGHTFORM",			SONG_SUNFORM},
	luaConstant(SONG_LI),
	luaConstant(SONG_TIME),
	luaConstant(SONG_LANCE),
	luaConstant(SONG_MAP),
	luaConstant(SONG_ANIMA),
	luaConstant(SONG_MAX),

	luaConstantFromClass(BLEND_DEFAULT,	RenderObject),
	luaConstantFromClass(BLEND_ADD,		RenderObject),
	{"BLEND_ADDITIVE",					RenderObject::BLEND_ADD},
	luaConstantFromClass(BLEND_SUB,		RenderObject),
	luaConstantFromClass(BLEND_MULT,	RenderObject),

	{"ENDING_NAIJACAVE",				10},
	{"ENDING_NAIJACAVEDONE",			11},
	{"ENDING_SECRETCAVE",				12},
	{"ENDING_MAINAREA",					13},
	{"ENDING_DONE",						14},


	{"FLAG_SONGCAVECRYSTAL",			20},
	{"FLAG_TEIRA",						50},
	{"FLAG_SHARAN",						51},
	{"FLAG_DRASK",						52},
	{"FLAG_VEDHA",						53},

	{"FLAG_ENERGYTEMPLE01DOOR",			100},
	{"FLAG_ENERGYDOOR02",				101},
	{"FLAG_ENERGYSLOT01",				102},
	{"FLAG_ENERGYSLOT02",				103},
	{"FLAG_ENERGYSLOT_MAINAREA",		104},
	{"FLAG_MAINAREA_ENERGYTEMPLE_ROCK",	105},
	{"FLAG_ENERGYSLOT_FIRST",			106},
	{"FLAG_ENERGYDOOR03",				107},
	{"FLAG_ENERGYGODENCOUNTER",			108},
	{"FLAG_ENERGYBOSSDEAD",				109},
	{"FLAG_MAINAREA_ETENTER2",			110},
	{"FLAG_SUNTEMPLE_WATERLEVEL",		111},
	{"FLAG_SUNTEMPLE_LIGHTCRYSTAL",		112},
	{"FLAG_SUNKENCITY_PUZZLE",			113},
	{"FLAG_SUNKENCITY_BOSS",			114},
	{"FLAG_MITHALAS_THRONEROOM",		115},
	{"FLAG_BOSS_MITHALA",				116},
	{"FLAG_BOSS_FOREST",				117},
	{"FLAG_FISHCAVE",					118},
	{"FLAG_VISION_VEIL",				119},
	{"FLAG_MITHALAS_PRIESTS",			120},
	{"FLAG_FIRSTTRANSTURTLE",			121},
	{"FLAG_13PROGRESSION",				122},
	{"FLAG_FINAL",						123},
	{"FLAG_SPIRIT_ERULIAN",				124},
	{"FLAG_SPIRIT_KROTITE",				125},
	{"FLAG_SPIRIT_DRASK",				126},
	{"FLAG_SPIRIT_DRUNIAD",				127},
	{"FLAG_BOSS_SUNWORM",				128},
	{"FLAG_WHALELAMPPUZZLE",			129},

	{"FLAG_TRANSTURTLE_VEIL01",			130},
	{"FLAG_TRANSTURTLE_OPENWATER06",	131},
	{"FLAG_TRANSTURTLE_FOREST04",		132},
	{"FLAG_TRANSTURTLE_OPENWATER03",	133},
	{"FLAG_TRANSTURTLE_FOREST05",		134},
	{"FLAG_TRANSTURTLE_MAINAREA",		135},
	{"FLAG_TRANSTURTLE_SEAHORSE",		136},
	{"FLAG_TRANSTURTLE_VEIL02",			137},
	{"FLAG_TRANSTURTLE_ABYSS03",		138},
	{"FLAG_TRANSTURTLE_FINALBOSS",		139},

	{"FLAG_NAIJA_SWIM",					200},
	{"FLAG_NAIJA_MINIMAP",				201},
	{"FLAG_NAIJA_SPEEDBOOST",			202},
	{"FLAG_NAIJA_MEMORYCRYSTAL",		203},
	{"FLAG_NAIJA_SINGING",				204},
	{"FLAG_NAIJA_LEAVESVEDHA",			205},
	{"FLAG_NAIJA_SONGDOOR",				206},
	{"FLAG_NAIJA_ENTERVEDHACAVE",		207},
	{"FLAG_NAIJA_INTERACT",				208},
	{"FLAG_NAIJA_ENTERSONGCAVE",		209},
	{"FLAG_NAIJA_ENERGYFORMSHOT",		210},
	{"FLAG_NAIJA_ENERGYFORMCHARGE",		211},
	{"FLAG_NAIJA_RETURNTONORMALFORM",	212},
	{"FLAG_NAIJA_ENERGYBARRIER",		213},
	{"FLAG_NAIJA_SOLIDENERGYBARRIER",	214},
	{"FLAG_NAIJA_ENTERENERGYTEMPLE",	215},
	{"FLAG_NAIJA_OPENWATERS",			216},
	{"FLAG_NAIJA_SINGING",				217},
	{"FLAG_NAIJA_INGAMEMENU",			218},
	{"FLAG_NAIJA_SINGINGHINT",			219},
	{"FLAG_NAIJA_LOOK",					220},
	{"FLAG_HINT_MINIMAP",				221},
	{"FLAG_HINT_HEALTHPLANT",			222},
	{"FLAG_HINT_SLEEP",					223},
	{"FLAG_HINT_COLLECTIBLE",			224},
	{"FLAG_HINT_IGFDEMO",				225},
	{"FLAG_HINT_BEASTFORM1",			226},
	{"FLAG_HINT_BEASTFORM2",			227},
	{"FLAG_HINT_LISONG",				228},
	{"FLAG_HINT_ENERGYTARGET",			229},
	{"FLAG_HINT_NATUREFORMABILITY",		230},
	{"FLAG_HINT_LICOMBAT",				231},
	{"FLAG_HINT_COOKING",				232},
	{"FLAG_NAIJA_FIRSTVINE",			233},
	luaConstant(FLAG_SECRET01),
	luaConstant(FLAG_SECRET02),
	luaConstant(FLAG_SECRET03),
	{"FLAG_DEEPWHALE",					237},
	{"FLAG_OMPO",						238},
	{"FLAG_HINT_SINGBULB",				239},
	{"FLAG_ENDING",						240},
	{"FLAG_NAIJA_BINDSHELL",			241},
	{"FLAG_NAIJA_BINDROCK",				242},
	{"FLAG_HINT_ROLLGEAR",				243},
	{"FLAG_FIRSTHEALTHUPGRADE",			244},
	{"FLAG_MAINAREA_TRANSTURTLE_ROCK",	245},
	{"FLAG_SKIPSECRETCHECK",			246},
	{"FLAG_SEAHORSEBESTTIME",			247},
	{"FLAG_SEAHORSETIMETOBEAT",			248},
	{"FLAG_HINT_BINDMERMEN",			249},


	{"FLAG_CREATORVOICE",				250},

	{"FLAG_HINT_DUALFORMCHANGE",		251},
	{"FLAG_HINT_DUALFORMCHARGE",		252},
	{"FLAG_HINT_HEALTHUPGRADE",			253},

	{"FLAG_VISION_ENERGYTEMPLE",		300},

	luaConstant(FLAG_COLLECTIBLE_START),
	{"FLAG_COLLECTIBLE_SONGCAVE",			500},
	{"FLAG_COLLECTIBLE_ENERGYTEMPLE",		501},
	{"FLAG_COLLECTIBLE_ENERGYSTATUE",		502},
	{"FLAG_COLLECTIBLE_ENERGYBOSS",			503},
	{"FLAG_COLLECTIBLE_NAIJACAVE",			504},
	{"FLAG_COLLECTIBLE_CRABCOSTUME",		505},
	{"FLAG_COLLECTIBLE_JELLYPLANT",			506},
	{"FLAG_COLLECTIBLE_MITHALASPOT",		507},
	{"FLAG_COLLECTIBLE_SEAHORSECOSTUME",	508},
	{"FLAG_COLLECTIBLE_CHEST",				509},
	{"FLAG_COLLECTIBLE_BANNER",				510},
	{"FLAG_COLLECTIBLE_MITHALADOLL",		511},
	{"FLAG_COLLECTIBLE_WALKERBABY",			512},
	{"FLAG_COLLECTIBLE_SEEDBAG",			513},
	{"FLAG_COLLECTIBLE_ARNASSISTATUE",		514},
	{"FLAG_COLLECTIBLE_GEAR",				515},
	{"FLAG_COLLECTIBLE_SUNKEY",				516},
	{"FLAG_COLLECTIBLE_URCHINCOSTUME",		517},
	{"FLAG_COLLECTIBLE_TEENCOSTUME",		518},
	{"FLAG_COLLECTIBLE_MUTANTCOSTUME",		519},
	{"FLAG_COLLECTIBLE_JELLYCOSTUME",		520},
	{"FLAG_COLLECTIBLE_MITHALANCOSTUME",	521},
	{"FLAG_COLLECTIBLE_ANEMONESEED",		522},
	{"FLAG_COLLECTIBLE_BIOSEED",			523},
	{"FLAG_COLLECTIBLE_TURTLEEGG",			524},
	{"FLAG_COLLECTIBLE_SKULL",				525},
	{"FLAG_COLLECTIBLE_TRIDENTHEAD",		526},
	{"FLAG_COLLECTIBLE_SPORESEED",			527},
	{"FLAG_COLLECTIBLE_UPSIDEDOWNSEED",		528},
	{"FLAG_COLLECTIBLE_STONEHEAD",			529},
	{"FLAG_COLLECTIBLE_STARFISH",			530},
	{"FLAG_COLLECTIBLE_BLACKPEARL",			531},
	luaConstant(FLAG_COLLECTIBLE_END),

	luaConstant(FLAG_PET_ACTIVE),
	luaConstant(FLAG_PET_NAMESTART),
	{"FLAG_PET_NAUTILUS",					601},
	{"FLAG_PET_DUMBO",						602},
	{"FLAG_PET_BLASTER",					603},
	{"FLAG_PET_PIRANHA",					604},

	luaConstant(FLAG_UPGRADE_WOK),
	// does the player have access to 3 slots all the time?

	{"FLAG_COLLECTIBLE_NAUTILUSPRIME",	630},
	{"FLAG_COLLECTIBLE_DUMBOEGG",		631},
	{"FLAG_COLLECTIBLE_BLASTEREGG",		632},
	{"FLAG_COLLECTIBLE_PIRANHAEGG",		633},

	{"FLAG_ENTER_HOMEWATERS",			650},
	{"FLAG_ENTER_SONGCAVE",				651},
	{"FLAG_ENTER_ENERGYTEMPLE",			652},
	{"FLAG_ENTER_OPENWATERS",			653},
	{"FLAG_ENTER_HOMECAVE",				654},
	{"FLAG_ENTER_FOREST",				655},
	{"FLAG_ENTER_VEIL",					656},
	{"FLAG_ENTER_MITHALAS",				657},
	{"FLAG_ENTER_MERMOGCAVE",			658},
	{"FLAG_ENTER_MITHALAS",				659},
	{"FLAG_ENTER_SUNTEMPLE",			660},
	{"FLAG_ENTER_ABYSS",				661},
	{"FLAG_ENTER_SUNKENCITY",			662},
	{"FLAG_ENTER_FORESTSPRITECAVE",		663},
	{"FLAG_ENTER_FISHCAVE",				664},
	{"FLAG_ENTER_MITHALASCATHEDRAL",	665},
	{"FLAG_ENTER_TURTLECAVE",			666},
	{"FLAG_ENTER_FROZENVEIL",			667},
	{"FLAG_ENTER_ICECAVE",				668},
	{"FLAG_ENTER_SEAHORSE",				669},


	{"FLAG_MINIBOSS_START",				700},
	{"FLAG_MINIBOSS_NAUTILUSPRIME",		700},
	{"FLAG_MINIBOSS_KINGJELLY",			701},
	{"FLAG_MINIBOSS_MERGOG",			702},
	{"FLAG_MINIBOSS_CRAB",				703},
	{"FLAG_MINIBOSS_OCTOMUN",			704},
	{"FLAG_MINIBOSS_MANTISSHRIMP",		705},
	{"FLAG_MINIBOSS_PRIESTS",			706},
	{"FLAG_MINIBOSS_END",				720},

	{"FLAG_MAMATURTLE_RESCUE1",			750},
	{"FLAG_MAMATURTLE_RESCUE2",			751},
	{"FLAG_MAMATURTLE_RESCUE3",			752},

	{"FLAG_SONGDOOR1",					800},
	luaConstant(FLAG_SEALOAFANNOYANCE),

	{"FLAG_SEAL_KING",					900},
	{"FLAG_SEAL_QUEEN",					901},
	{"FLAG_SEAL_PRINCE",				902},

	{"FLAG_HEALTHUPGRADES",				950},
	{"FLAG_HEALTHUPGRADES_END",			960},

	luaConstant(FLAG_LI),
	luaConstant(FLAG_LICOMBAT),



	luaConstant(MAX_FLAGS),

	{"ALPHA_NEARZERO",					0.001},

	{"SUNKENCITY_START",				0},
	{"SUNKENCITY_CLIMBDOWN",			1},
	{"SUNKENCITY_RUNAWAY",				2},
	{"SUNKENCITY_INHOLE",				3},
	{"SUNKENCITY_GF",					4},
	{"SUNKENCITY_BULLIES",				5},
	{"SUNKENCITY_ANIMA",				6},
	{"SUNKENCITY_BOSSWAIT",				7},
	{"SUNKENCITY_CLAY1",				8},
	{"SUNKENCITY_CLAY2",				9},
	{"SUNKENCITY_CLAY3",				10},
	{"SUNKENCITY_CLAY4",				11},
	{"SUNKENCITY_CLAY5",				12},
	{"SUNKENCITY_CLAY6",				13},
	{"SUNKENCITY_CLAYDONE",				14},
	{"SUNKENCITY_BOSSFIGHT",			15},
	{"SUNKENCITY_BOSSDONE",				16},
	{"SUNKENCITY_FINALTONGUE",			17},

	{"FINAL_START",						0},
	{"FINAL_SOMETHING",					1},
	{"FINAL_FREEDLI",					2},

	luaConstantFromClass(ANIM_NONE,		Bone),
	luaConstantFromClass(ANIM_POS,		Bone),
	luaConstantFromClass(ANIM_ROT,		Bone),
	luaConstantFromClass(ANIM_ALL,		Bone),

	luaConstant(FORM_NORMAL),
	luaConstant(FORM_ENERGY),
	luaConstant(FORM_BEAST),
	luaConstant(FORM_NATURE),
	luaConstant(FORM_SPIRIT),
	luaConstant(FORM_DUAL),
	luaConstant(FORM_FISH),
	luaConstant(FORM_SUN),
	{"FORM_LIGHT",			FORM_SUN},
	luaConstant(FORM_MAX),

	luaConstant(VFX_SHOCK),
	luaConstant(VFX_RIPPLE),

	luaConstant(EAT_NONE),
	luaConstant(EAT_DEFAULT),
	luaConstant(EAT_FILE),
	luaConstant(EAT_MAX),

	luaConstant(DT_NONE),
	luaConstant(DT_ENEMY),
	luaConstant(DT_ENEMY_ENERGYBLAST),
	luaConstant(DT_ENEMY_SHOCK),
	luaConstant(DT_ENEMY_BITE),
	luaConstant(DT_ENEMY_TRAP),
	luaConstant(DT_ENEMY_WEB),
	luaConstant(DT_ENEMY_BEAM),
	luaConstant(DT_ENEMY_GAS),
	luaConstant(DT_ENEMY_INK),
	luaConstant(DT_ENEMY_POISON),
	luaConstant(DT_ENEMY_ACTIVEPOISON),
	luaConstant(DT_ENEMY_CREATOR),
	luaConstant(DT_ENEMY_MANTISBOMB),
	luaConstant(DT_ENEMY_MAX),
	{"DT_ENEMY_END",			DT_ENEMY_MAX},

	luaConstant(DT_AVATAR),
	luaConstant(DT_AVATAR_ENERGYBLAST),
	luaConstant(DT_AVATAR_SHOCK),
	luaConstant(DT_AVATAR_BITE),
	luaConstant(DT_AVATAR_VOMIT),
	luaConstant(DT_AVATAR_ACID),
	luaConstant(DT_AVATAR_SPORECHILD),
	luaConstant(DT_AVATAR_LIZAP),
	luaConstant(DT_AVATAR_NATURE),
	luaConstant(DT_AVATAR_ENERGYROLL),
	luaConstant(DT_AVATAR_VINE),
	luaConstant(DT_AVATAR_EAT),
	luaConstant(DT_AVATAR_EAT_BASICSHOT),
	luaConstant(DT_AVATAR_EAT_MAX),
	luaConstant(DT_AVATAR_LANCEATTACH),
	luaConstant(DT_AVATAR_LANCE),
	luaConstant(DT_AVATAR_CREATORSHOT),
	luaConstant(DT_AVATAR_DUALFORMLI),
	luaConstant(DT_AVATAR_DUALFORMNAIJA),
	luaConstant(DT_AVATAR_BUBBLE),
	luaConstant(DT_AVATAR_SEED),
	luaConstant(DT_AVATAR_PET),
	luaConstant(DT_AVATAR_PETNAUTILUS),
	luaConstant(DT_AVATAR_PETBITE),
	luaConstant(DT_AVATAR_MAX),
	{"DT_AVATAR_END",			DT_AVATAR_MAX},

	luaConstant(DT_TOUCH),
	luaConstant(DT_CRUSH),
	luaConstant(DT_SPIKES),
	luaConstant(DT_STEAM),
	luaConstant(DT_WALLHURT),


	luaConstant(FRAME_TIME),

	luaConstant(FORMUPGRADE_ENERGY1),
	luaConstant(FORMUPGRADE_ENERGY2),
	luaConstant(FORMUPGRADE_BEAST),

	luaConstant(TILE_SIZE),

	luaConstant(INPUT_MOUSE),
	luaConstant(INPUT_JOYSTICK),
	luaConstant(INPUT_KEYBOARD),

	luaConstant(ANIMLAYER_FLOURISH),
	luaConstant(ANIMLAYER_OVERRIDE),
	luaConstant(ANIMLAYER_ARMOVERRIDE),
	luaConstant(ANIMLAYER_UPPERBODYIDLE),
	luaConstant(ANIMLAYER_HEADOVERRIDE),

	luaConstant(OT_EMPTY),
	luaConstant(OT_BLACK),
	luaConstant(OT_BLACKINVIS),
	luaConstant(OT_INVISIBLE),
	luaConstant(OT_INVISIBLEIN),
	luaConstant(OT_HURT),
	luaConstant(OT_INVISIBLEENT),

	luaConstant(SEE_MAP_NEVER),
	luaConstant(SEE_MAP_DEFAULT),
	luaConstant(SEE_MAP_ALWAYS),
};

//============================================================================================
// F U N C T I O N S
//============================================================================================

ScriptInterface::ScriptInterface()
: baseState(NULL)
{
}

void ScriptInterface::init()
{
	if (!baseState)
		baseState = createLuaVM();
}

void ScriptInterface::reset()
{
	shutdown();
	init();
}

lua_State *ScriptInterface::createLuaVM()
{
	lua_State *state = lua_open();	/* opens Lua */
	luaopen_base(state);			/* opens the basic library */
	luaopen_table(state);			/* opens the table library */
	luaopen_string(state);			/* opens the string lib. */
	luaopen_math(state);			/* opens the math lib. */
	luaopen_debug(state);

	// Set up various tables for state management:

	// -- Interface function tables for each script file.
	lua_newtable(state);
	lua_setglobal(state, "_scriptfuncs");

	// -- Number of users (active threads) for each script file.
	lua_newtable(state);
	lua_setglobal(state, "_scriptusers");

	// -- Initial instance-local tables for each script file.
	lua_newtable(state);
	lua_setglobal(state, "_scriptvars");

	// -- Instance-local variable tables for each thread.
	lua_newtable(state);
	lua_setglobal(state, "_threadvars");

	// -- Active threads (so they aren't garbage-collected).
	lua_newtable(state);
	lua_setglobal(state, "_threadtable");

	// Register all custom functions and constants.
	for (unsigned int i = 0; i < sizeof(luaFunctionTable)/sizeof(*luaFunctionTable); i++)
	{
		lua_register(state, luaFunctionTable[i].name, luaFunctionTable[i].func);
	}
	for (unsigned int i = 0; i < sizeof(luaConstantTable)/sizeof(*luaConstantTable); i++)
	{
		lua_pushnumber(state, luaConstantTable[i].value);
		lua_setglobal(state, luaConstantTable[i].name);
	}

	// Add hooks to monitor global get/set operations if requested.
	if (complainOnGlobalVar)
	{
		if (!lua_getmetatable(state, LUA_GLOBALSINDEX))
			lua_newtable(state);
		lua_pushcfunction(state, l_indexWarnGlobal);
		lua_setfield(state, -2, "__index");
		lua_pushcfunction(state, l_newindexWarnGlobal);
		lua_setfield(state, -2, "__newindex");
		lua_setmetatable(state, LUA_GLOBALSINDEX);
	}

	// In case a script errors outside of any protected environment, report and exit.
	lua_atpanic(state, l_panicHandler);

	// All done, return the new state.
	return state;
}

void ScriptInterface::destroyLuaVM(lua_State *state)
{
	if (state)
		lua_close(state);
}

// Initial value for the instance-local table should be on the stack of
// the base Lua state; it will be popped when this function returns.
lua_State *ScriptInterface::createLuaThread(const std::string &file)
{
	lua_State *thread = lua_newthread(baseState);
	if (!thread)
	{
		lua_pop(baseState, 1);
		return NULL;
	}

	// Save the thread object in a Lua table to prevent it from being
	// garbage-collected.
	lua_getglobal(baseState, "_threadtable");
	lua_pushlightuserdata(baseState, thread);
	lua_pushvalue(baseState, -3);  // -3 = thread
	lua_rawset(baseState, -3);     // -3 = _threadtable
	lua_pop(baseState, 2);

	// Set up the instance-local variable table for this thread, copying
	// the contents of the initial-value table.
	lua_newtable(baseState);
	lua_pushnil(baseState);
	while (lua_next(baseState, -3))
	{
		// We need to save a copy of the key for the next iteration.
		lua_pushvalue(baseState, -2);
		lua_insert(baseState, -2);
		lua_settable(baseState, -4);
	}
	lua_remove(baseState, -2);  // We no longer need the original table.
	if (complainOnUndefLocal)
	{
		if (!lua_getmetatable(baseState, -1))
			lua_newtable(baseState);
		lua_pushcfunction(baseState, l_indexWarnInstance);
		lua_setfield(baseState, -2, "__index");
		lua_setmetatable(baseState, -2);
	}
	lua_getglobal(baseState, "_threadvars");
	lua_pushlightuserdata(baseState, thread);
	lua_pushvalue(baseState, -3);  // -3 = instance-local table
	lua_rawset(baseState, -3);     // -3 = _threadvars
	lua_pop(baseState, 2);

	// Update the usage count for this script.
	lua_getglobal(baseState, "_scriptusers");
	lua_getfield(baseState, -1, file.c_str());
	const int users = lua_tointeger(baseState, -1) + 1;
	lua_pop(baseState, 1);
	lua_pushinteger(baseState, users);
	lua_setfield(baseState, -2, file.c_str());
	lua_pop(baseState, 1);

	return thread;
}

// Returns the number of remaining users of this script.
int ScriptInterface::destroyLuaThread(const std::string &file, lua_State *thread)
{
	// Threads are not explicitly closed; instead, we delete the thread
	// resources from the state-global tables, thus allowing them to be
	// garbage-collected.  collectGarbage() can be called at a convenient
	// time to forcibly free all dead thread resources.

	lua_getglobal(baseState, "_threadtable");
	lua_pushlightuserdata(baseState, thread);
	lua_pushnil(baseState);
	lua_rawset(baseState, -3);
	lua_pop(baseState, 1);

	lua_getglobal(baseState, "_threadvars");
	lua_pushlightuserdata(baseState, thread);
	lua_pushnil(baseState);
	lua_rawset(baseState, -3);
	lua_pop(baseState, 1);

	lua_getglobal(baseState, "_scriptusers");
	lua_getfield(baseState, -1, file.c_str());
	const int users = lua_tointeger(baseState, -1) - 1;
	lua_pop(baseState, 1);
	if (users > 0)
		lua_pushinteger(baseState, users);
	else
		lua_pushnil(baseState);
	lua_setfield(baseState, -2, file.c_str());
	lua_pop(baseState, 1);

	return users;
}

void ScriptInterface::collectGarbage()
{
	lua_gc(baseState, LUA_GCCOLLECT, 0);
}

int ScriptInterface::gcGetStats()
{
	return lua_gc(baseState, LUA_GCCOUNT, 0);
}

void ScriptInterface::shutdown()
{
	destroyLuaVM(baseState);
	baseState = NULL;
}

Script *ScriptInterface::openScript(const std::string &file, bool ignoremissing /* = false */)
{
	std::string realFile = localisePathInternalModpath(file);
	realFile = core->adjustFilenameCase(realFile);
	bool loadedScript = false;

	lua_getglobal(baseState, "_scriptvars");
	lua_getfield(baseState, -1, realFile.c_str());
	lua_remove(baseState, -2);
	if (!lua_istable(baseState, -1))
	{
		// We must not have loaded the script yet, so load it.
		loadedScript = true;

		// Clear out the (presumably nil) getfield() result from the stack.
		lua_pop(baseState, 1);

		// Create a new variable table for the initial run of the script.
		// This will become the initial instance-local variable table for
		// all instances of this script.
		lua_newtable(baseState);
		if (complainOnUndefLocal)
		{
			if (!lua_getmetatable(baseState, -1))
				lua_newtable(baseState);
			lua_pushcfunction(baseState, l_indexWarnInstance);
			lua_setfield(baseState, -2, "__index");
			lua_setmetatable(baseState, -2);
		}

		// Save the current value of the "v" global, so we can restore it
		// after we run the script.  (We do this here and in Script::call()
		// so that nested Lua calls don't disrupt the caller's instance
		// variable table.)
		lua_getglobal(baseState, "v");

		// Load the file itself.  This leaves the Lua chunk on the stack.
		int result = loadFile_helper(baseState, realFile.c_str());
		if (result != 0)
		{
			if(result != LUA_ERRFILE || (result == LUA_ERRFILE && !ignoremissing))
				scriptError("Error loading script [" + realFile + "]: " + lua_tostring(baseState, -1));
			lua_pop(baseState, 2);
			return NULL;
		}

		// Do the initial run of the script, popping the Lua chunk.
		lua_getglobal(baseState, "_threadvars");
		lua_pushlightuserdata(baseState, baseState);
		lua_pushvalue(baseState, -5);
		lua_settable(baseState, -3);
		lua_pop(baseState, 1);
		fixupLocalVars(baseState);
		result = lua_pcall(baseState, 0, 0, 0);
		lua_getglobal(baseState, "_threadvars");
		lua_pushlightuserdata(baseState, baseState);
		lua_pushnil(baseState);
		lua_settable(baseState, -3);
		lua_pop(baseState, 1);
		if (result != 0)
		{
			scriptError("Error doing initial run of script [" + realFile + "]: " + lua_tostring(baseState, -1));
			lua_pop(baseState, 2);
			return NULL;
		}

		// Restore the old value of the "v" global.
		lua_setglobal(baseState, "v");

		// Store the instance-local table in the _scriptvars table.
		lua_getglobal(baseState, "_scriptvars");
		lua_pushvalue(baseState, -2);
		lua_setfield(baseState, -2, realFile.c_str());
		lua_pop(baseState, 1);

		// Generate an interface function table for the script, and
		// clear out the functions from the global environment.
		lua_getglobal(baseState, "_scriptfuncs");
		lua_newtable(baseState);
		for (unsigned int i = 0; interfaceFunctions[i] != NULL; i++)
		{
			const char *funcName = interfaceFunctions[i];
			lua_getglobal(baseState, funcName);
			if (!lua_isnil(baseState, -1))
			{
				lua_setfield(baseState, -2, funcName);
				lua_pushnil(baseState);
				lua_setglobal(baseState, funcName);
			}
			else
			{
				lua_pop(baseState, 1);
			}
		}
		lua_setfield(baseState, -2, realFile.c_str());
		lua_pop(baseState, 1);

		// Leave the instance-local table on the stack for createLuaThread().
	}

	lua_State *thread = createLuaThread(realFile.c_str());
	if (!thread)
	{
		scriptError("Unable to create new thread for script [" + realFile + "]");
		if (loadedScript)
		{
			lua_getglobal(baseState, "_scriptfuncs");
			lua_pushnil(baseState);
			lua_setfield(baseState, -2, realFile.c_str());
			lua_pop(baseState, 1);
			lua_getglobal(baseState, "_scriptvars");
			lua_pushnil(baseState);
			lua_setfield(baseState, -2, realFile.c_str());
			lua_pop(baseState, 1);
		}
		return NULL;
	}

	return new Script(thread, realFile);
}

void ScriptInterface::closeScript(Script *script)
{
	const char *file = script->getFile().c_str();
	int users = destroyLuaThread(file, script->getLuaState());

	// If this was the last instance of this script, unload the script itself.
	if (users <= 0)
	{
		lua_getglobal(baseState, "_scriptfuncs");
		lua_pushnil(baseState);
		lua_setfield(baseState, -2, file);
		lua_pop(baseState, 1);
		lua_getglobal(baseState, "_scriptvars");
		lua_pushnil(baseState);
		lua_setfield(baseState, -2, file);
		lua_pop(baseState, 1);
	}

	delete script;
}

bool ScriptInterface::runScriptNum(const std::string &file, const std::string &func, int num)
{
	std::string realFile = file;
	if (file.find('/')==std::string::npos)
		realFile = "scripts/" + file + ".lua";
	Script *script = openScript(realFile);
	if (!script)
		return false;

	if (!script->call(func.c_str(), num))
	{
		debugLog(script->getLastError());
		debugLog("(error calling func: " + func + " in script: " + file + ")");
		closeScript(script);
		return false;
	}

	closeScript(script);
	return true;
}

bool ScriptInterface::runScript(const std::string &file, const std::string &func, bool ignoremissing /* = false */)
{
	std::string realFile = file;
	if (file.find('/')==std::string::npos)
		realFile = "scripts/" + file + ".lua";
	Script *script = openScript(realFile, ignoremissing);
	if (!script)
		return false;

	if (!func.empty() && !script->call(func.c_str()))
	{
		debugLog(script->getLastError());
		debugLog("(error calling func: " + func + " in script: " + file + ")");
		closeScript(script);
		return false;
	}

	closeScript(script);
	return true;
}

//-------------------------------------------------------------------------

void Script::lookupFunc(const char *name)
{
	lua_getglobal(L, "_scriptfuncs");  // [_scriptfuncs]
	lua_getfield(L, -1, file.c_str()); // [_scriptfuncs, tab]
	lua_remove(L, -2);                 // [tab]
	lua_getfield(L, -1, name);         // [tab, f]
	lua_remove(L, -2);                 // [f]
}

bool Script::doCall(int nparams, int nrets)
{
	// Push the current value of the "v" global onto the Lua stack,
	// so we can restore the current script's instance variable table
	// before returning.
	lua_getglobal(L, "v");                      // [f, ..., v]

	lua_insert(L, -(nparams+2));                // [v, f, ...]
	fixupLocalVars(L);

	int vpos = lua_gettop(L) - (nparams+1);

	bool result;
	if (lua_pcall(L, nparams, nrets, 0) == 0)   // [v, ...]
	{
		result = true;
	}
	else
	{
		lastError = lua_tostring(L, -1);
		lastError += " [";
		lastError += luaFormatStackInfo(L);
		lastError += "]";
		lua_pop(L, 1);
		result = false;
	}

	if (nrets != 0)
	{
		lua_pushvalue(L, vpos);                // [v, ..., v]
		lua_remove(L, vpos);                   // [..., v]
	}

	lua_setglobal(L, "v");                     // [...]

	return result;
}

bool Script::call(const char *name)
{
	lookupFunc(name);
	return doCall(0);
}

bool Script::call(const char *name, float param1)
{
	lookupFunc(name);
	lua_pushnumber(L, param1);
	return doCall(1);
}

bool Script::call(const char *name, void *param1)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	return doCall(1);
}

bool Script::call(const char *name, void *param1, float param2)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	lua_pushnumber(L, param2);
	return doCall(2);
}

bool Script::call(const char *name, void *param1, void *param2)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	luaPushPointer(L, param2);
	return doCall(2);
}

bool Script::call(const char *name, void *param1, float param2, float param3)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	lua_pushnumber(L, param2);
	lua_pushnumber(L, param3);
	return doCall(3);
}

bool Script::call(const char *name, void *param1, float param2, float param3, bool *ret1)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	lua_pushnumber(L, param2);
	lua_pushnumber(L, param3);
	if (!doCall(3, 1))
		return false;
	*ret1 = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return true;
}

bool Script::call(const char *name, void *param1, const char *param2, float param3)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	lua_pushstring(L, param2);
	lua_pushnumber(L, param3);
	return doCall(3);
}

bool Script::call(const char *name, void *param1, const char *param2, void *param3)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	lua_pushstring(L, param2);
	luaPushPointer(L, param3);
	return doCall(3);
}

bool Script::call(const char *name, void *param1, void *param2, void *param3)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	luaPushPointer(L, param2);
	luaPushPointer(L, param3);
	return doCall(3);
}

bool Script::call(const char *name, void *param1, float param2, float param3, float param4)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	lua_pushnumber(L, param2);
	lua_pushnumber(L, param3);
	lua_pushnumber(L, param4);
	return doCall(4);
}

bool Script::call(const char *name, void *param1, void *param2, void *param3, void *param4)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	luaPushPointer(L, param2);
	luaPushPointer(L, param3);
	luaPushPointer(L, param4);
	return doCall(4);
}

bool Script::call(const char *name, void *param1, void *param2, void *param3, float param4, float param5, float param6, float param7, void *param8, bool *ret1)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	luaPushPointer(L, param2);
	luaPushPointer(L, param3);
	lua_pushnumber(L, param4);
	lua_pushnumber(L, param5);
	lua_pushnumber(L, param6);
	lua_pushnumber(L, param7);
	luaPushPointer(L, param8);
	if (!doCall(8, 1))
		return false;
	*ret1 = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return true;
}

int Script::callVariadic(const char *name, lua_State *fromL, int nparams, void *param)
{
	int oldtop = lua_gettop(L);

	lookupFunc(name);
	luaPushPointer(L, param);

	// If both stacks are the same, we already pushed 2 more entries to the stack.
	int pos = (L == fromL) ? -(nparams+2) : -nparams;
	for (int i = 0; i < nparams; ++i)
		lua_pushvalue(fromL, pos);

	// Move them to the other stack. Ignored if L == fromL.
	lua_xmove(fromL, L, nparams);

	// Do the call
	if (!doCall(nparams + 1, LUA_MULTRET))
		return -1;

	nparams = lua_gettop(L) - oldtop;
	lua_xmove(L, fromL, nparams);

	return nparams;
}
