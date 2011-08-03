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

#include "../BBGE/MathFunctions.h"

const bool throwLuaErrors = false;

// Set this to true to complain (via errorLog()) whenever a script tries to
// get or set a global variable.
const bool complainOnGlobalVar = false;
// Set this to true to complain whenever a script tries to get an undefined
// thread-local variable.
const bool complainOnUndefLocal = false;

// List of all interface functions called by C++ code, terminated by NULL.
static const char * const interfaceFunctions[] = {
	"action",
	"activate",
	"animationKey",
	"castSong",
	"damage",
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

static void luaErrorMsg(lua_State *L, const char *msg)
{
	debugLog(msg);

	if (throwLuaErrors)
	{
		lua_pushstring(L, msg);
		lua_error(L);
	}
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

static inline
ScriptedEntity *scriptedEntity(lua_State *L, int slot = 1)
{
	ScriptedEntity *se = (ScriptedEntity*)lua_touserdata(L, slot);
	if (!se)
		debugLog("ScriptedEntity invalid pointer.");
	return se;
}

static inline
CollideEntity *collideEntity(lua_State *L, int slot = 1)
{
	CollideEntity *ce = (CollideEntity*)lua_touserdata(L, slot);
	if (!ce)
		debugLog("CollideEntity invalid pointer.");
	return ce ;
}

static inline
RenderObject *object(lua_State *L, int slot = 1)
{
	//RenderObject *obj = dynamic_cast<RenderObject*>((RenderObject*)(int(lua_tonumber(L, slot))));
	RenderObject *obj = static_cast<RenderObject*>(lua_touserdata(L, slot));
	if (!obj)
		debugLog("RenderObject invalid pointer");
	return obj;
}

static inline
Beam *beam(lua_State *L, int slot = 1)
{
	Beam *b = (Beam*)lua_touserdata(L, slot);
	if (!b)
		debugLog("Beam invalid pointer.");
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
Shot *getShot(lua_State *L, int slot = 1)
{
	Shot *shot = (Shot*)lua_touserdata(L, slot);
	return shot;
}

static inline
Web *getWeb(lua_State *L, int slot = 1)
{
	Web *web = (Web*)lua_touserdata(L, slot);
	return web;
}

static inline
Ingredient *getIng(lua_State *L, int slot = 1)
{
	return (Ingredient*)lua_touserdata(L, slot);
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
	if (!ent)
	{
		luaErrorMsg(L, "Entity Invalid Pointer");
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
	if (!b)
	{
		luaErrorMsg(L, "Bone Invalid Pointer");
	}
	return b;
}

static inline
Path *pathFromName(lua_State *L, int slot = 1)
{
	std::string s = lua_tostring(L, slot);
	stringToLowerUserData(s);
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
	return p;
}

static RenderObject *entityToRenderObject(lua_State *L, int slot = 1)
{
	Entity *e = entity(L, slot);
	return dynamic_cast<RenderObject*>(e);
}

static RenderObject *boneToRenderObject(lua_State *L, int slot = 1)
{
	Bone *b = bone(L, slot);
	return dynamic_cast<RenderObject*>(b);
}

static PauseQuad *getPauseQuad(lua_State *L, int slot = 1)
{
	PauseQuad *q = (PauseQuad*)lua_touserdata(L, slot);
	if (q)
		return q;
	else
		errorLog("Invalid PauseQuad/Particle");
	return 0;
}

static SkeletalSprite *getSkeletalSprite(Entity *e)
{
	Avatar *a;
	ScriptedEntity *se;
	SkeletalSprite *skel = 0;
	if ((a = dynamic_cast<Avatar*>(e)) != 0)
	{
		//a->skeletalSprite.transitionAnimate(lua_tostring(L, 2), 0.15, lua_tointeger(L, 3));
		skel = &a->skeletalSprite;
	}
	else if ((se = dynamic_cast<ScriptedEntity*>(e)) != 0)
	{
		skel = &se->skeletalSprite;
	}
	return skel;
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
			lua_Debug ar;
			if (lua_getstack(L, 1, &ar))
			{
				lua_getinfo(L, "Sl", &ar);
			}
			else
			{
				snprintf(ar.short_src, sizeof(ar.short_src), "???");
				ar.currentline = 0;
			}

			lua_getinfo(L, "Sl", &ar);
			std::ostringstream os;
			os << "WARNING: " << ar.short_src << ":" << ar.currentline
			   << ": script tried to get/call undefined global variable "
			   << lua_tostring(L, -2);
			errorLog(os.str());
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
	bool doWarn = (strcmp(varname, "v") != 0);
	for (unsigned int i = 0; doWarn && interfaceFunctions[i] != NULL; i++)
	{
		doWarn = (strcmp(varname, interfaceFunctions[i]) != 0);
	}

	if (doWarn)
	{
		lua_Debug ar;
		if (lua_getstack(L, 1, &ar))
		{
			lua_getinfo(L, "Sl", &ar);
		}
		else
		{
			snprintf(ar.short_src, sizeof(ar.short_src), "???");
			ar.currentline = 0;
		}

		std::ostringstream os;
		os << "WARNING: " << ar.short_src << ":" << ar.currentline
		   << ": script set global "
		   << (lua_type(L, -2) == LUA_TFUNCTION ? "function" : "variable")
		   << " " << lua_tostring(L, -1);
		errorLog(os.str());
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
		lua_Debug ar;
		if (lua_getstack(L, 1, &ar))
		{
			lua_getinfo(L, "Sl", &ar);
		}
		else
		{
			snprintf(ar.short_src, sizeof(ar.short_src), "???");
			ar.currentline = 0;
		}
		std::ostringstream os;
		os << "WARNING: " << ar.short_src << ":" << ar.currentline
		   << ": script tried to get/call undefined instance variable "
		   << lua_tostring(L, -2);
		errorLog(os.str());
	}
	lua_remove(L, -2);

	return 1;
}

luaFunc(dofile_caseinsensitive)
{
	// This is Lua's dofile(), with some tweaks.  --ryan.
	std::string fname(core->adjustFilenameCase(luaL_checkstring(L, 1)));
	int n = lua_gettop(L);
	if (luaL_loadfile(L, fname.c_str()) != 0) lua_error(L);
	lua_call(L, 0, LUA_MULTRET);
	return lua_gettop(L) - n;
}

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
	luaReturnNum(0);
}

luaFunc(shakeCamera)
{
	dsq->shakeCamera(lua_tonumber(L,1), lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(changeForm)
{
	dsq->game->avatar->changeForm((FormType)lua_tointeger(L, 1));
	luaReturnNum(0);
}

luaFunc(getWaterLevel)
{
	luaReturnNum(dsq->game->getWaterLevel());
}

luaFunc(setPoison)
{
	dsq->continuity.setPoison(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(cureAllStatus)
{
	dsq->continuity.cureAllStatus();
	luaReturnNum(0);
}

luaFunc(setMusicToPlay)
{
	if (lua_isstring(L, 1))
		dsq->game->setMusicToPlay(lua_tostring(L, 1));
	luaReturnNum(0);
}

luaFunc(setActivePet)
{
	Entity *e = dsq->game->setActivePet(lua_tonumber(L, 1));

	luaReturnPtr(e);
}

luaFunc(setWaterLevel)
{
	dsq->game->waterLevel.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
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
	luaReturnNum(0);
}

luaFunc(hasLi)
{
	luaReturnBool(dsq->continuity.hasLi());
}

luaFunc(hasFormUpgrade)
{
	luaReturnBool(dsq->continuity.hasFormUpgrade((FormUpgradeType)lua_tointeger(L, 1)));
}

luaFunc(castSong)
{
	dsq->continuity.castSong(lua_tonumber(L, 1));
	luaReturnNum(0);
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

	luaReturnInt(0);
}

luaFunc(setStory)
{
	dsq->continuity.setStory(lua_tonumber(L, 1));
	luaReturnNum(0);
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
	luaReturnNum(r);
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
	luaReturnNum(pt);
}

luaFunc(web_getNumPoints)
{
	Web *web = getWeb(L);
	int num = 0;
	if (web)
	{
		num = web->getNumPoints();
	}
	luaReturnNum(num);
}

luaFunc(web_delete)
{
	Web *e = getWeb(L);
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
			e->setDecayRate(1.0/time);
		}
	}
	luaReturnInt(0);
}

luaFunc(shot_getPosition)
{
	float x=0,y=0;
	Shot *shot = getShot(L);
	if (shot)
	{
		x = shot->position.x;
		y = shot->position.y;
	}
	luaReturnVec2(x, y);
}

luaFunc(shot_setVel)
{
	Shot *shot = getShot(L);
	float vx = lua_tonumber(L, 2);
	float vy = lua_tonumber(L, 3);
	if (shot)
	{
		shot->velocity = Vector(vx, vy);
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_addIgnoreShotDamageType)
{
	Entity *e = entity(L);
	if (e)
	{
		e->addIgnoreShotDamageType((DamageType)lua_tointeger(L, 2));
	}
	luaReturnNum(0);
}

luaFunc(entity_warpLastPosition)
{
	Entity *e = entity(L);
	if (e)
	{
		e->warpLastPosition();
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_setRidingPosition)
{
	Entity *e = entity(L);
	if (e)
		e->setRidingPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	luaReturnNum(0);
}

luaFunc(entity_setRidingData)
{
	Entity *e = entity(L);
	if (e)
		e->setRidingData(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), getBool(L, 5));
	luaReturnNum(0);
}

luaFunc(entity_setBoneLock)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	Bone *b = bone(L, 3);
	bool ret = false;
	if (e)
	{
		BoneLock bl;
		bl.entity = e2;
		bl.bone = b;
		bl.on = true;
		bl.collisionMaskIndex = dsq->game->lastCollideMaskIndex;
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
	luaReturnNum(0);
}

luaFunc(entity_setSegsMaxDist)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->setMaxDist(lua_tonumber(L, 2));

	luaReturnNum(0);
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
	luaReturnInt(0);
}

luaFunc(user_save)
{
	dsq->user.save();
	luaReturnInt(0);
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
	luaReturnNum(0);
}

luaFunc(entity_setLookAtPoint)
{
	Entity *e = entity(L);
	if (e)
	{
		e->lookAtPoint = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnNum(0);
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


luaFunc(entity_setLife)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setLife(lua_tonumber(L, 2));
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(toggleCursor)
{
	dsq->toggleCursor(getBool(L, 1), lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(toggleBlackBars)
{
	dsq->toggleBlackBars(getBool(L, 1));
	luaReturnNum(0);
}

luaFunc(setBlackBarsColor)
{
	Vector c(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
	dsq->setBlackBarsColor(c);
	luaReturnNum(0);
}

luaFunc(toggleLiCombat)
{
	dsq->continuity.toggleLiCombat(getBool(L));
	luaReturnNum(0);
}

luaFunc(getNoteName)
{
	luaReturnStr(dsq->game->getNoteName(lua_tonumber(L, 1), getString(L, 2)).c_str());
}

luaFunc(getWorldType)
{
	luaReturnNum((int)dsq->continuity.getWorldType());
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
	luaReturnNum(0);
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
	luaReturnInt(0);
}

luaFunc(setActivation)
{
	dsq->game->activation = getBool(L, 1);
	luaReturnInt(0);
}

luaFunc(debugLog)
{
	const char *s = lua_tostring(L, 1);
	debugLog(s);
	luaReturnStr(s);
}

luaFunc(reconstructGrid)
{
	dsq->game->reconstructGrid(true);
	luaReturnNum(0);
}

luaFunc(reconstructEntityGrid)
{
	dsq->game->reconstructEntityGrid();
	luaReturnNum(0);
}

luaFunc(entity_setCanLeaveWater)
{
	Entity *e = entity(L);
	bool v = getBool(L, 2);
	if (e)
	{
		e->setCanLeaveWater(v);
	}
	luaReturnNum(0);
}

luaFunc(entity_setSegmentTexture)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		RenderObject *ro = e->getSegment(lua_tonumber(L, 2));
		if (ro)
		{
			ro->setTexture(lua_tostring(L, 3));
		}
	}
	luaReturnNum(0);
}

luaFunc(entity_findNearestEntityOfType)
{
	Entity *e = entity(L);
	Entity *nearest = 0;
	if (e)
	{
		int et = (EntityType)lua_tointeger(L, 2);
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
	std::string shotData = lua_tostring(L, 1);
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


	s = dsq->game->fireShot(shotData, e, t, pos, aim);

	luaReturnPtr(s);
}


luaFunc(entity_sound)
{
	Entity *e = entity(L);
	if (e)
	{
		e->sound(lua_tostring(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}
	luaReturnNum(0);
}


luaFunc(entity_soundFreq)
{
	Entity *e = entity(L);
	if (e)
	{
		e->soundFreq(lua_tostring(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}
	luaReturnNum(0);
}

luaFunc(entity_setSpiritFreeze)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setSpiritFreeze(getBool(L,2));
	}
	luaReturnNum(0);
}

luaFunc(entity_setFillGrid)
{
	Entity *e = entity(L);
	bool b = getBool(L,2);
	if (e)
	{
		e->fillGridFromQuad = b;
	}
	luaReturnNum(0);
}

luaFunc(entity_setTouchDamage)
{
	Entity *e = entity(L);
	if (e)
	{
		e->touchDamage = lua_tonumber(L, 2);
	}
	luaReturnNum(0);
}

luaFunc(entity_setCollideRadius)
{
	Entity *e = entity(L);
	if (e)
	{
		e->collideRadius = lua_tonumber(L, 2);
	}
	luaReturnNum(0);
}

luaFunc(entity_getNormal)
{
	float nx=0, ny=1;
	Entity *e = entity(L);
	if (e)
	{
		Vector v = e->getForward();
		nx = v.x;
		ny = v.y;
	}
	luaReturnVec2(nx, ny);
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
	Entity *e1 = entity(L);
	Entity *e2 = entity(L, 2);
	if (e1 && e2)
	{
		Vector diff = e2->position - e1->position;
		luaReturnVec2(diff.x, diff.y);
	}
	else
	{
		luaReturnVec2(0, 0);
	}
}

luaFunc(entity_getCollideRadius)
{
	Entity *e = entity(L);
	int ret = 0;
	if (e)
	{
		ret = e->collideRadius;
	}
	luaReturnNum(ret);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_stopPull)
{
	Entity *e = entity(L);
	if (e)
		e->stopPull();
	luaReturnNum(0);
}

luaFunc(entity_stopInterpolating)
{
	Entity *e = entity(L);
	if (e)
	{
		e->position.stop();
	}
	luaReturnNum(0);
}

luaFunc(entity_moveToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->moveToNode(p, lua_tointeger(L, 3), lua_tointeger(L, 4), 0);
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
}


luaFunc(avatar_setCanDie)
{
	dsq->game->avatar->canDie = getBool(L, 1);

	luaReturnNum(0);
}

luaFunc(avatar_toggleCape)
{
	dsq->game->avatar->toggleCape(getBool(L,1));

	luaReturnNum(0);
}

luaFunc(avatar_setBlockSinging)
{
	bool b = getBool(L);
	dsq->game->avatar->setBlockSinging(b);
	luaReturnNum(0);
}

luaFunc(avatar_fallOffWall)
{
	dsq->game->avatar->fallOffWall();
	luaReturnNum(0);
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
	dsq->enqueueJumpState(lua_tostring(L, 1), getBool(L, 2));
	luaReturnInt(0);
}

luaFunc(goToTitle)
{
	dsq->title();
	luaReturnNum(0);
}

luaFunc(getEnqueuedState)
{
	luaReturnStr(dsq->getEnqueuedJumpState().c_str());
}

luaFunc(learnSong)
{
	dsq->continuity.learnSong(lua_tointeger(L, 1));
	luaReturnNum(0);
}

luaFunc(unlearnSong)
{
	dsq->continuity.unlearnSong(lua_tointeger(L, 1));
	luaReturnNum(0);
}

luaFunc(showInGameMenu)
{
	dsq->game->showInGameMenu(getBool(L, 1), getBool(L, 2), (MenuPage)lua_tointeger(L, 3));
	luaReturnNum(0);
}

luaFunc(hideInGameMenu)
{
	dsq->game->hideInGameMenu();
	luaReturnNum(0);
}

luaFunc(showImage)
{
	dsq->game->showImage(getString(L));
	luaReturnNum(0);
}

luaFunc(hideImage)
{
	dsq->game->hideImage();
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_followPath)
{
	/*
	std::ostringstream os2;
	os2 << lua_tointeger(L, 1);
	errorLog(os2.str());
	std::ostringstream os;
	os << "Entity: " << scriptedEntity(L)->name << " moving on Path: " << lua_tostring(L, 2);
	debugLog(os.str());
	*/
	Entity *e = entity(L);
	if (e)
	{
		Path *p = path(L, 2);
		int speedType = lua_tonumber(L, 3);
		int dir = lua_tonumber(L, 4);

		e->followPath(p, speedType, dir);
	}
	luaReturnNum(0);
}

luaFunc(entity_enableMotionBlur)
{
	Entity *e = entity(L);
	if (e)
		e->enableMotionBlur(10, 2);
	luaReturnNum(0);
}

luaFunc(entity_disableMotionBlur)
{
	Entity *e = entity(L);
	if (e)
		e->disableMotionBlur();
	luaReturnNum(0);
}

luaFunc(entity_warpToPathStart)
{
	ScriptedEntity *e = scriptedEntity(L);
	std::string s;
	if (lua_isstring(L, 2))
		s = lua_tostring(L, 2);
	if (s.empty())
		e->warpToPathStart();
	else
	{
		//e->followPath(s, 0, 0);
		e->warpToPathStart();
		e->stopFollowingPath();
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(spawnParticleEffect)
{
	dsq->spawnParticleEffect(getString(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 5), lua_tonumber(L, 4));
	luaReturnNum(0);
}

luaFunc(bone_showFrame)
{
	Bone *b = bone(L);
	if (b)
		b->showFrame(lua_tointeger(L, 2));
	luaReturnNum(1);
}

luaFunc(bone_setRenderPass)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setRenderPass(lua_tonumber(L, 2));
	}
	luaReturnNum(0);
}

luaFunc(bone_setSegmentOffset)
{
	Bone *b = bone(L);
	if (b)
		b->segmentOffset = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNum(0);
}

luaFunc(bone_setSegmentProps)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setSegmentProps(lua_tonumber(L, 2), lua_tonumber(L, 3), getBool(L, 4));
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(bone_addSegment)
{
	Bone *b = bone(L);
	Bone *b2 = bone(L, 2);
	if (b && b2)
		b->addSegment(b2);
	luaReturnNum(0);
}

luaFunc(bone_setAnimated)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setAnimated(lua_tointeger(L, 2));
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(bone_setSegs)
{
	Bone *b = bone(L);
	if (b)
		b->setSegs(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tointeger(L, 9));
	luaReturnInt(0);
}

luaFunc(entity_setSegs)
{
	Entity *e = entity(L);
	if (e)
		e->setSegs(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tointeger(L, 9));
	luaReturnInt(0);
}

luaFunc(entity_resetTimer)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->resetTimer(lua_tonumber(L, 2));
	luaReturnInt(0);
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
	luaReturnInt(0);
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
	luaReturnInt(0);
}

luaFunc(entity_stopTimer)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->stopTimer();
	luaReturnInt(0);
}

luaFunc(entity_createEntity)
{
	Entity *e = entity(L);
	if (e)
		dsq->game->createEntity(dsq->getEntityTypeIndexByName(lua_tostring(L, 2)), 0, e->position, 0, false, "", ET_ENEMY, BT_NORMAL, 0, 0, true);
	luaReturnInt(0);
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
	if (lua_tonumber(L, 1) != 0)
		e = entity(L);

	if (dsq->game->avatar->pullTarget != 0)
		dsq->game->avatar->pullTarget->stopPull();

	dsq->game->avatar->pullTarget = e;

	if (e)
		e->startPull();

	luaReturnNum(0);
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
	luaReturnInt(0);
}

luaFunc(quitNestedMain)
{
	core->quitNestedMain();
	luaReturnInt(0);
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
	luaReturnInt(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_setColor)
{
	Entity *e = entity(L);
	if (e)
	{
		//e->color = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
		e->color.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5));
	}
	luaReturnInt(0);
}

luaFunc(bone_scale)
{
	Bone *e = bone(L);
	if (e)
	{
		//e->color = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
		e->scale.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnInt(0);
}

luaFunc(bone_setBlendType)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setBlendType(lua_tonumber(L, 2));
	}
	luaReturnInt(0);
}

luaFunc(bone_update)
{
	bone(L)->update(lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(bone_setColor)
{
	Bone *e = bone(L);
	if (e)
	{
		//e->color = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
		e->color.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5));
	}
	luaReturnInt(0);
}

luaFunc(bone_rotate)
{
	Bone *e = bone(L);
	if (e)
		e->rotation.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	luaReturnNum(0);
}

luaFunc(bone_rotateOffset)
{
	Bone *e = bone(L);
	if (e)
		e->rotationOffset.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	luaReturnNum(0);
}

luaFunc(bone_getRotation)
{
	Bone *e = bone(L);
	if (e)
		luaReturnNum(e->rotation.z);
	else
		luaReturnNum(0);
}


luaFunc(bone_setPosition)
{
	Bone *b = bone(L);
	if (b)
	{
		b->position.interpolateTo(Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	luaReturnInt(0);
}

luaFunc(bone_getWorldRotation)
{
	Bone *b = bone(L);
	float rot = 0;
	if (b)
	{
		//rot = b->getAbsoluteRotation().z;
		rot = b->getWorldRotation();
	}
	luaReturnNum(rot);
}

luaFunc(bone_getWorldPosition)
{
	Bone *b = bone(L);
	float x = 0, y = 0;
	if (b)
	{
		Vector v = b->getWorldPosition();
		x = v.x;
		y = v.y;
	}
	luaReturnVec2(x, y);
}

luaFunc(entity_setBlendType)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setBlendType(lua_tonumber(L, 2));
	}
	luaReturnInt(0);
}

luaFunc(entity_setEntityType)
{
	Entity *e = entity(L);
	if (e)
		e->setEntityType(EntityType(lua_tointeger(L, 2)));
	luaReturnInt(1);
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
	luaReturnNum(0);
}

luaFunc(cam_toNode)
{
	Path *p = path(L);
	if (p)
	{
		//dsq->game->cameraPanToNode(p, 500);
		dsq->game->setCameraFollow(&p->nodes[0].position);
	}
	luaReturnNum(0);
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
			//dsq->game->cameraPanToNode(p, 500);
			//dsq->game->setCameraFollow(&p->nodes[0].position);
		}
	}
	luaReturnNum(0);
}

luaFunc(cam_setPosition)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float time = lua_tonumber(L, 3);
	int loopType = lua_tointeger(L, 4);
	int pingPong = lua_tointeger(L, 5);
	int ease = lua_tointeger(L, 6);

	/*

	*/

	Vector p = dsq->game->getCameraPositionFor(Vector(x,y));

	dsq->game->cameraInterp.stop();
	dsq->game->cameraInterp.interpolateTo(p, time, loopType, pingPong, ease);

	if (time == 0)
	{
		dsq->game->cameraInterp = p;
	}

	dsq->cameraPos = p;
	luaReturnNum(0);
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

	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_startEmitter)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	if (se)
	{
		se->startEmitter(e);
	}
	luaReturnNum(0);
}

luaFunc(entity_stopEmitter)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	if (se)
	{
		se->stopEmitter(e);
	}
	luaReturnNum(0);
}

luaFunc(entity_initStrands)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		e->initStrands(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), Vector(lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8)));
	}
	luaReturnNum(0);
}

luaFunc(entity_initSkeletal)
{
	ScriptedEntity *e = scriptedEntity(L);
	e->renderQuad = false;
	e->setWidthHeight(128, 128);
	e->skeletalSprite.loadSkeletal(lua_tostring(L, 2));
	if (lua_isstring(L, 3))
	{
		std::string s = lua_tostring(L, 3);
		if (!s.empty())
		{
			e->skeletalSprite.loadSkin(s);
		}
	}
	luaReturnNum(0);
}


luaFunc(entity_idle)
{
	Entity *e = entity(L);
	if (e)
	{
		e->idle();
	}
	luaReturnNum(0);
}

luaFunc(entity_stopAllAnimations)
{
	Entity *e = entity(L);
	if (e)
		e->skeletalSprite.stopAllAnimations();
	luaReturnNum(0);
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

luaFunc(entity_animate)
{
	SkeletalSprite *skel = getSkeletalSprite(entity(L));

	// 0.15
	// 0.2
	float transition = lua_tonumber(L, 5);
	if (transition == -1)
		transition = 0;
	else if (transition == 0)
		transition = 0.2;
	float ret = skel->transitionAnimate(lua_tostring(L, 2), transition, lua_tointeger(L, 3), lua_tointeger(L, 4));

	luaReturnNum(ret);
}

luaFunc(entity_moveToFront)
{
	Entity *e = entity(L);
	e->moveToFront();

	luaReturnNum(0);
}

luaFunc(entity_moveToBack)
{
	Entity *e = entity(L);
	e->moveToBack();

	luaReturnNum(0);
}

luaFunc(entity_move)
{
	Entity *e = entity(L);
	bool ease = lua_tointeger(L, 5);
	Vector p(lua_tointeger(L, 2), lua_tointeger(L, 3));
	if (lua_tointeger(L, 6))
	{
		p = e->position + p;
	}
	if (!ease)
	{
		e->position.interpolateTo(p, lua_tonumber(L, 4));
	}
	else
	{
		e->position.interpolateTo(p, lua_tonumber(L, 4), 0, 0, 1);
	}
	luaReturnInt(0);
}

luaFunc(spawnManaBall)
{
	Vector p;
	p.x = lua_tonumber(L, 1);
	p.y = lua_tonumber(L, 2);
	int amount = lua_tonumber(L, 3);
	dsq->game->spawnManaBall(p, amount);
	luaReturnInt(0);
}

luaFunc(spawnAroundEntity)
{
	Entity *e = entity(L);
	int num = lua_tonumber(L, 2);
	int radius = lua_tonumber(L, 3);
	std::string entType = lua_tostring(L, 4);
	std::string name = lua_tostring(L, 5);
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
	luaReturnNum(0);
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

luaFunc(beam_setPosition)
{
	Beam *b = beam(L);
	if (b)
	{
		b->position = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
		b->trace();
	}
	luaReturnNum(0);
}

luaFunc(beam_setDamage)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setDamage(lua_tonumber(L, 2));
	}
	luaReturnNum(0);
}

luaFunc(beam_setBeamWidth)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setBeamWidth(lua_tonumber(L, 2));
	}
	luaReturnNum(0);
}

luaFunc(beam_setTexture)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setTexture(getString(L, 2));
	}
	luaReturnNum(0);
}

luaFunc(beam_setAngle)
{
	Beam *b = beam(L);
	if (b)
	{
		b->angle = lua_tonumber(L, 2);
		b->trace();
	}
	luaReturnNum(0);
}

luaFunc(beam_delete)
{
	Beam *b = beam(L);
	if (b)
	{
		b->safeKill();
	}
	luaReturnNum(0);
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
	std::string type = lua_tostring(L, 1);
	std::string name;
	if (lua_isstring(L, 2))
		name = lua_tostring(L, 2);
	int x = lua_tointeger(L, 3);
	int y = lua_tointeger(L, 4);

	Entity *e = 0;
	e = dsq->game->createEntity(type, 0, Vector(x, y), 0, false, name, ET_ENEMY, BT_NORMAL, 0, 0, true);

	/*
	int idx = dsq->game->getIdxForEntityType(type);
	Entity *e = 0;
	if (idx == -1)
	{
		errorLog("Unknown entity type [" + type + "]");
	}
	else
	{
		e = dsq->game->createEntity(idx, 0, Vector(x,y), 0, false, name, ET_ENEMY, BT_NORMAL, 0, 0, true);
	}
	*/
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
	luaReturnNum(0);
}

luaFunc(pause)
{
	dsq->game->togglePause(1);
	luaReturnNum(0);
}

luaFunc(unpause)
{
	dsq->game->togglePause(0);
	luaReturnNum(0);
}

luaFunc(clearControlHint)
{
	dsq->game->clearControlHint();
	luaReturnNum(0);
}

luaFunc(setSceneColor)
{
	dsq->game->sceneColor3.interpolateTo(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	luaReturnNum(0);
}

luaFunc(setCameraLerpDelay)
{
	dsq->game->cameraLerpDelay = lua_tonumber(L, 1);
	luaReturnNum(0);
}

luaFunc(setControlHint)
{
	std::string str = lua_tostring(L, 1);
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
	luaReturnNum(0);
}

luaFunc(setCanChangeForm)
{
	dsq->game->avatar->canChangeForm = getBool(L);
	luaReturnNum(0);
}

luaFunc(setInvincibleOnNested)
{
	dsq->game->invincibleOnNested = getBool(L);
	luaReturnNum(0);
}

luaFunc(setCanWarp)
{
	dsq->game->avatar->canWarp = getBool(L);
	luaReturnNum(0);
}

luaFunc(entity_generateCollisionMask)
{
	Entity *e = entity(L);
	float num = lua_tonumber(L, 2);
	if (e)
	{
		e->generateCollisionMask(num);
	}
	luaReturnNum(0);
}

luaFunc(entity_damage)
{
	Entity *e = entity(L);
	if (e)
	{
		DamageData d;
		//d.attacker = e;
		d.attacker = entity(L, 2);
		d.damage = lua_tonumber(L, 3);
		d.damageType = (DamageType)lua_tointeger(L, 4);

		e->damage(d);
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_setRenderPass)
{
	Entity *e = entity(L);
	int pass = lua_tonumber(L, 2);
	if (e)
		e->setOverrideRenderPass(pass);
	luaReturnNum(0);
}

// intended to be used for setting max health and refilling it all
luaFunc(entity_setHealth)
{
	Entity *e = entity(L, 1);
	int h = lua_tonumber(L, 2);
	if (e)
	{
		e->health = e->maxHealth = h;
	}
	luaReturnNum(0);
}

// intended to be used for setting max health and refilling it all
luaFunc(entity_changeHealth)
{
	Entity *e = entity(L, 1);
	int h = lua_tonumber(L, 2);
	if (e)
	{
		e->health += h;
	}
	luaReturnNum(0);
}

luaFunc(entity_heal)
{
	Entity *e = entity(L);
	if (e)
		e->heal(lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(entity_revive)
{
	Entity *e = entity(L);
	if (e)
		e->revive(lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(screenFadeCapture)
{
	dsq->screenTransition->capture();
	luaReturnNum(0);
}


luaFunc(screenFadeTransition)
{
	dsq->screenTransition->transition(lua_tonumber(L, 1));
	luaReturnNum(0);
}

luaFunc(screenFadeGo)
{
	dsq->screenTransition->go(lua_tonumber(L, 1));
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(setTimerText)
{
	dsq->game->setTimerText(lua_tonumber(L, 1));
	luaReturnNum(0);
}

luaFunc(getWallNormal)
{
	float x,y;
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	int range = lua_tonumber(L, 3);
	if (range == 0)
	{
		range = 5;
	}

	Vector n = dsq->game->getWallNormal(Vector(x, y), range);

	luaReturnVec2(n.x, n.y);
}

luaFunc(incrFlag)
{
	std::string f = lua_tostring(L, 1);
	int v = 1;
	if (lua_isnumber(L, 2))
		v = lua_tointeger(L, 2);
	dsq->continuity.setFlag(f, dsq->continuity.getFlag(f)+v);
	luaReturnNum(0);
}

luaFunc(decrFlag)
{
	std::string f = lua_tostring(L, 1);
	int v = 1;
	if (lua_isnumber(L, 2))
		v = lua_tointeger(L, 2);
	dsq->continuity.setFlag(f, dsq->continuity.getFlag(f)-v);
	luaReturnNum(0);
}

luaFunc(setFlag)
{
	/*
	if (lua_isstring(L, 1))
		dsq->continuity.setFlag(lua_tostring(L, 1), lua_tonumber(L, 2));
	else
	*/
	dsq->continuity.setFlag(lua_tointeger(L, 1), lua_tointeger(L, 2));
	luaReturnNum(0);
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

luaFunc(entity_x)
{
	Entity *e = entity(L);
	float v = 0;
	if (e)
	{
		v = e->position.x;
	}
	luaReturnNum(v);
}

luaFunc(entity_y)
{
	Entity *e = entity(L);
	float v = 0;
	if (e)
	{
		v = e->position.y;
	}
	luaReturnNum(v);
}

luaFunc(node_setActive)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->active = v;
	}
	luaReturnNum(0);
}

luaFunc(node_setCursorActivation)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->cursorActivation = v;
	}
	luaReturnNum(0);
}

luaFunc(node_setCatchActions)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->catchActions = v;
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}


luaFunc(registerSporeDrop)
{
	float x, y;
	int t=0;
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	t = lua_tointeger(L, 3);

	dsq->game->registerSporeDrop(Vector(x,y), t);

	luaReturnNum(0);
}

luaFunc(setStringFlag)
{
	std::string n = getString(L, 1);
	std::string v = getString(L, 2);
	dsq->continuity.setStringFlag(n, v);
	luaReturnInt(0);
}

luaFunc(centerText)
{
	dsq->centerText(getString(L, 1));
	luaReturnNum(0);
}

luaFunc(msg)
{
	std::string s(lua_tostring(L, 1));
	dsq->screenMessage(s);
	luaReturnNum(0);
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
	luaReturnInt(0);
}

luaFunc(entity_handleShotCollisionsSkeletal)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->game->handleShotCollisionsSkeletal(e);
	}
	luaReturnInt(0);
}

luaFunc(entity_handleShotCollisionsHair)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->game->handleShotCollisionsHair(e, lua_tonumber(L, 2));
	}
	luaReturnInt(0);
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

luaFunc(entity_collideCircleVsLine)
{
	Entity *e = entity(L);
	int x1, y1, x2, y2, sz;
	x1 = lua_tonumber(L, 2);
	y1 = lua_tonumber(L, 3);
	x2 = lua_tonumber(L, 4);
	y2 = lua_tonumber(L, 5);
	sz = lua_tonumber(L, 6);
	bool v = false;
	if (e)
		v = dsq->game->collideCircleVsLine(e, Vector(x1, y1), Vector(x2, y2), sz);
	luaReturnBool(v);
}


luaFunc(entity_collideCircleVsLineAngle)
{
	Entity *e = entity(L);
	float angle = lua_tonumber(L, 2);
	int start=lua_tonumber(L, 3), end=lua_tonumber(L, 4), radius=lua_tonumber(L, 5);
	int x=lua_tonumber(L, 6);
	int y=lua_tonumber(L, 7);
	bool v = false;
	if (e)
		v = dsq->game->collideCircleVsLineAngle(e, angle, start, end, radius, Vector(x,y));
	luaReturnBool(v);
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
		col = dsq->game->collideHairVsCircle(e, num, e2->position, e2->collideRadius, perc);
	}
	luaReturnBool(col);
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
	luaReturnInt(0);
}

luaFunc(entity_debugText)
{
	Entity *e = entity(L);
	if (e)
	{
		BitmapText *f = new BitmapText(&dsq->smallFont);
		f->setText(lua_tostring(L, 2));
		f->position = e->position;
		core->getTopStateData()->addRenderObject(f, LR_DEBUG_TEXT);
		f->setLife(5);
		f->setDecayRate(1);
		f->fadeAlphaWithLife=1;
	}
	luaReturnInt(0);
}

luaFunc(entity_getHealth)
{
	Entity *e = entity(L);
	if (e)
		luaReturnNum(e->health);
	else
		luaReturnNum(0);
}

luaFunc(entity_initSegments)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->initSegments(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tostring(L, 5), lua_tostring(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8), lua_tonumber(L, 9), lua_tointeger(L, 10));

	luaReturnNum(0);
}

luaFunc(entity_warpSegments)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->warpSegments();

	luaReturnNum(0);
}

//entity_incrTargetLeaches
luaFunc(entity_incrTargetLeaches)
{
	Entity *e = entity(L);
	if (e && e->getTargetEntity())
		e->getTargetEntity()->leaches++;
	luaReturnNum(0);
}

luaFunc(entity_decrTargetLeaches)
{
	Entity *e = entity(L);
	if (e && e->getTargetEntity())
		e->getTargetEntity()->leaches--;
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
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_update)
{
	entity(L)->update(lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(entity_updateSkeletal)
{
	entity(L)->skeletalSprite.update(lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(entity_msg)
{
	Entity *e = entity(L);
	if (e)
	{
		if (lua_isuserdata(L, 3))
			e->message(getString(L, 2), lua_touserdata(L, 3));
		else
			e->message(getString(L, 2), lua_tonumber(L, 3));
	}
	luaReturnNum(0);
}

luaFunc(entity_updateCurrents)
{
	luaReturnBool(entity(L)->updateCurrents(lua_tonumber(L, 2)));
}

luaFunc(entity_updateLocalWarpAreas)
{
	luaReturnBool(entity(L)->updateLocalWarpAreas(getBool(L, 2)));
}

luaFunc(entity_updateMovement)
{
	scriptedEntity(L)->updateMovement(lua_tonumber(L, 2));
	luaReturnNum(0);
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
	luaReturnInt(0);
}

luaFunc(entity_getRotation)
{
	Entity *e = entity(L);
	float v = 0;
	if (e)
	{
		v = e->rotation.z;
	}
	luaReturnNum(v);
}

luaFunc(flingMonkey)
{
	Entity *e = entity(L);

	dsq->continuity.flingMonkey(e);

	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(setNaijaHeadTexture)
{
	Avatar *a = dsq->game->avatar;
	if (a)
	{
		a->setHeadTexture(lua_tostring(L, 1));
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_flipToEntity)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	if (e && e2)
	{
		e->flipToTarget(e2->position);
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_flipToVel)
{
	Entity *e = entity(L);
	if (e)
	{
		e->flipToVel();
	}
	luaReturnNum(0);
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

luaFunc(entity_isFlippedHorizontal)
{
	Entity *e = entity(L);
	bool v=false;
	if (e)
	{
		v = e->isfh();
	}
	luaReturnBool(v);
}

luaFunc(entity_isFlippedVertical)
{
	Entity *e = entity(L);
	bool v=false;
	if (e)
	{
		v = e->isfv();
	}
	luaReturnBool(v);
}

luaFunc(entity_flipHorizontal)
{
	Entity *e = entity(L);
	if (e)
		e->flipHorizontal();
	luaReturnNum(0);
}

luaFunc(entity_fhTo)
{
	Entity *e = entity(L);
	bool b = getBool(L);
	if (e)
		e->fhTo(b);
	luaReturnNum(0);
}

luaFunc(entity_flipVertical)
{
	Entity *e = entity(L);
	if (e)
		e->flipVertical();
	luaReturnNum(0);
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

	luaReturnPtr(q);
}

luaFunc(quad_scale)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
		q->scale.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	luaReturnInt(0);
}

luaFunc(quad_rotate)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
		q->rotation.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4));
	luaReturnNum(0);
}

luaFunc(quad_color)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
	{
		//e->color = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
		q->color.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8));
	}
	luaReturnInt(0);
}

luaFunc(quad_alpha)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
		q->alpha.interpolateTo(Vector(lua_tonumber(L, 2),0,0), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	luaReturnInt(0);
}

luaFunc(quad_alphaMod)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
		q->alphaMod = lua_tonumber(L, 2);
	luaReturnInt(0);
}

luaFunc(quad_getAlpha)
{
	PauseQuad *q = getPauseQuad(L);
	float v = 0;
	if (q)
		v = q->alpha.x;
	luaReturnNum(v);
}


luaFunc(quad_delete)
{
	PauseQuad *q = getPauseQuad(L);
	float t = lua_tonumber(L, 2);
	if (q)
	{
		if (t == 0)
			q->safeKill();
		else
		{
			q->setLife(1);
			q->setDecayRate(1.0f/t);
			q->fadeAlphaWithLife = 1;
		}
	}
	luaReturnNum(0);
}


luaFunc(quad_setBlendType)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
	{
		q->setBlendType(lua_tonumber(L, 2));
	}
	luaReturnInt(0);
}

luaFunc(quad_setPosition)
{
	PauseQuad *q = getPauseQuad(L);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	if (q)
		q->position.interpolateTo(Vector(x,y), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(setupBasicEntity)
{
	ScriptedEntity *se = scriptedEntity(L);
	//-- texture, health, manaballamount, exp, money, collideRadius, initState
	if (se)
		se->setupBasicEntity(lua_tostring(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8), lua_tointeger(L, 9), lua_tointeger(L, 10), lua_tointeger(L, 11), lua_tointeger(L, 12), lua_tointeger(L, 13), lua_tointeger(L, 14));

	luaReturnNum(0);
}

luaFunc(entity_setBeautyFlip)
{
	Entity *e = entity(L);
	if (e)
	{
		e->beautyFlip = getBool(L, 2);
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_setDeathSound)
{
	Entity *e = entity(L);
	if (e)
	{
		e->deathSound = lua_tostring(L, 2);
	}
	luaReturnNum(0);
}

luaFunc(entity_setDeathParticleEffect)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->deathParticleEffect = getString(L, 2);
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	luaReturnNum(0);
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
	{
		e->moveTowardsAngle(lua_tointeger(L, 2), lua_tonumber(L, 3), lua_tointeger(L, 4));
	}
	luaReturnInt(0);
}

luaFunc(entity_moveAroundAngle)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowardsAngle(lua_tointeger(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}
	luaReturnInt(0);
}

luaFunc(entity_moveTowards)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowards(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5));
	}
	luaReturnInt(0);
}

luaFunc(entity_moveAround)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveAround(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	luaReturnInt(0);
}

luaFunc(entity_addVel)
{
	Entity *e = entity(L);
	if (e)
	{
		e->vel += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnInt(0);
}

luaFunc(entity_addVel2)
{
	Entity *e = entity(L);
	if (e)
	{
		e->vel2 += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnInt(0);
}

luaFunc(entity_addRandomVel)
{
	Entity *e = entity(L);
	int len = lua_tonumber(L, 2);
	if (e && len)
	{
		int angle = int(rand()%360);
		float a = MathFunctions::toRadians(angle);
		Vector add(sinf(a), cosf(a));
		add.setLength2D(len);
		e->vel += add;
		//e->vel += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnInt(0);
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

luaFunc(entity_isVelIn)
{
	Entity *e = entity(L);
	bool b = false;
	if (e)
	{
		b = e->vel.isLength2DIn(lua_tonumber(L, 2));
	}
	luaReturnBool(b);
}

luaFunc(entity_getVelLen)
{
	Entity *e = entity(L);
	int l = 0;
	if (e)
		l = e->vel.getLength2D();
	luaReturnNum(l);
}

luaFunc(entity_velx)
{
	Entity *e = entity(L);
	float velx = 0;
	if (e)
	{
		velx = e->vel.x;
	}
	luaReturnNum(velx);
}

luaFunc(entity_vely)
{
	Entity *e = entity(L);
	float vely = 0;
	if (e)
	{
		vely = e->vel.y;
	}
	luaReturnNum(vely);
}

luaFunc(entity_clearVel)
{
	Entity *e = entity(L);
	if (e)
		e->vel = Vector(0,0,0);
	luaReturnNum(0);
}

luaFunc(entity_clearVel2)
{
	Entity *e = entity(L);
	if (e)
		e->vel2 = Vector(0,0,0);
	luaReturnNum(0);
}

luaFunc(getScreenCenter)
{
	luaReturnVec2(core->screenCenter.x, core->screenCenter.y);
}

luaFunc(entity_rotate)
{
	Entity *e = entity(L);
	if (e)
		e->rotation.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	luaReturnNum(0);
}

luaFunc(entity_rotateOffset)
{
	Entity *e = entity(L);
	if (e)
		e->rotationOffset.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_setBounce)
{
	CollideEntity *e = collideEntity(L);
	if (e)
		e->bounceAmount = e->bounceEntityAmount = lua_tonumber(L, 2);
	luaReturnInt(0);
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
	luaReturnInt(0);
}

luaFunc(entity_setPosition)
{
	Entity *e = entity(L);
	float t = 0;
	if (e)
	{
		t = e->position.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNum(t);
}

luaFunc(entity_setInternalOffset)
{
	Entity *e = entity(L);
	float t = 0;
	if (e)
	{
		t = e->internalOffset.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNum(t);
}

luaFunc(entity_setTexture)
{
	Entity *e = entity(L);
	if (e)
	{
		std::string s = lua_tostring(L, 2);
		e->setTexture(s);
	}
	luaReturnNum(0);
}

luaFunc(entity_setMaxSpeed)
{
	Entity *e = entity(L);
	if (e)
		e->setMaxSpeed(lua_tointeger(L, 2));

	luaReturnNum(0);
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

	luaReturnNum(0);
}

// note: this is a weaker setState than perform
// this is so that things can override it
// for example getting PUSH-ed (Force) or FROZEN (bubbled)
luaFunc(entity_setState)
{
	Entity *me = entity(L);
	if (me)
	{
		/*
		if (!me->isDead())
		{
		*/
		int state = lua_tointeger(L, 2);
		float time = lua_tonumber(L, 3);
		if (time == 0)
			time = -1;
		bool force = getBool(L, 4);
		//if (me->getEnqueuedState() == StateMachine::STATE_NONE)
		me->setState(state, time, force);
		//}
	}
	luaReturnNum(0);
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
	Bone *b = 0;
	if (e)
	{
		b = e->skeletalSprite.getBoneByName(lua_tostring(L, 2));
	}
	/*
	if (e)
	{
		int n = 0;
		if (lua_isnumber(L, 2))
		{
			n = lua_tonumber(L, 2);
			b = e->skeletalSprite.getBoneByIdx(n);
		}
	}
	*/
	luaReturnPtr(b);
}

// ditch entity::sound and use this code instead...
// replace entity sound with this code

luaFunc(entity_playSfx)
{
	Entity *e= entity(L);
	if (e)
	{
		std::string sfx = lua_tostring(L, 2);


		/*
		int f = rand()%200-100;
		f += 1000;
		*/
		dsq->playPositionalSfx(sfx, e->position);
		/*
		Vector diff = e->position - dsq->game->avatar->position;
		if (diff.isLength2DIn(800))
		{
			int dist = diff.getLength2D();
			int vol = 255 - int((dist*255.0f) / 1500.0f);
			int pan = (diff.x*100)/800.0f;
			if (pan < -100)
				pan = -100;
			if (pan > 100)
				pan = 100;

			std::ostringstream os;
			os << "vol: " << vol << " pan: " << pan;
			debugLog(os.str());


			sound->playSfx(sfx, vol, pan, 1000+f);
		}
		*/

		//sound->playSfx(sfx);
	}
	luaReturnNum(0);
}

luaFunc(bone_getPosition)
{
	Bone *b = bone(L);
	int x=0,y=0;
	if (b)
	{
		Vector pos = b->getWorldPosition();
		x = pos.x;
		y = pos.y;
	}
	luaReturnVec2(x, y);
}

luaFunc(bone_getScale)
{
	Bone *b = bone(L);
	float x=0,y=0;
	if (b)
	{
		Vector sc = b->scale;
		x = sc.x;
		y = sc.y;
	}
	luaReturnVec2(x, y);
}


luaFunc(bone_getNormal)
{
	Bone *b = bone(L);
	if (b)
	{
		Vector n = b->getForward();
		luaReturnVec2(n.x, n.y);
	}
	else
	{
		luaReturnVec2(0, 0);
	}
}

luaFunc(bone_damageFlash)
{
	Bone *b = bone(L);
	int type = lua_tonumber(L, 2);
	if (b)
	{
		Vector toColor = Vector(1, 0.1, 0.1);
		if (type == 1)
		{
			toColor = Vector(1, 1, 0.1);
		}
		b->color = Vector(1,1,1);
		b->color.interpolateTo(toColor, 0.1, 5, 1);
	}
	luaReturnNum(0);
}

luaFunc(bone_isVisible)
{
	bool ret = false;
	Bone *b = bone(L);
	if (b)
		ret = b->renderQuad;
	luaReturnBool(ret);
}

luaFunc(bone_setVisible)
{
	Bone *b = bone(L);
	if (b)
		b->renderQuad = getBool(L, 2);
	luaReturnNum(0);
}

luaFunc(bone_setTexture)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setTexture(lua_tostring(L, 2));
	}
	luaReturnNum(0);
}

luaFunc(bone_setTouchDamage)
{
	Bone *b = bone(L);
	if (b)
	{
		b->touchDamage = lua_tonumber(L, 2);
	}
	luaReturnNum(0);
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
	if (b)
	{
		v = b->name == lua_tostring(L, 2);
	}
	luaReturnBool(v);
}

luaFunc(overrideZoom)
{
	dsq->game->overrideZoom(lua_tonumber(L, 1), lua_tonumber(L, 2));

	luaReturnNum(0);
}

luaFunc(disableOverrideZoom)
{
	dsq->game->toggleOverrideZoom(false);
	luaReturnNum(0);
}

// dt, range, mod
luaFunc(entity_doSpellAvoidance)
{
	Entity *e = entity(L);
	if (e)
		e->doSpellAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4));
	luaReturnNum(0);
}

// dt, range, mod, ignore
luaFunc(entity_doEntityAvoidance)
{
	Entity *e = entity(L);
	if (e)
		e->doEntityAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), e->getTargetEntity());
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(setOverrideVoiceFader)
{
	dsq->sound->setOverrideVoiceFader(lua_tonumber(L, 1));
	luaReturnNum(0);
}

luaFunc(setGameSpeed)
{
	dsq->gameSpeed.stop();
	dsq->gameSpeed.stopPath();
	dsq->gameSpeed.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	luaReturnNum(0);
}

luaFunc(sendEntityMessage)
{
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	if (e)
		e->onMessage(lua_tostring(L, 2));

	luaReturnNum(0);
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

	luaReturnNum(0);
}

luaFunc(entity_setDeathScene)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDeathScene(getBool(L, 2));
	}
	luaReturnNum(0);
}

luaFunc(entity_setCollideWithAvatar)
{
	Entity *e = entity(L);
	if (e)
	{
		e->collideWithAvatar = getBool(L, 2);
	}
	luaReturnNum(0);
}

luaFunc(entity_setCurrentTarget)
{
	Entity *e = entity(L);
	if (e)
		e->currentEntityTarget = lua_tointeger(L, 2);
	luaReturnInt(0);
}

luaFunc(setMiniMapHint)
{
	std::istringstream is(lua_tostring(L, 1));
	is >> dsq->game->miniMapHint.scene >> dsq->game->miniMapHint.warpAreaType;
	dsq->game->updateMiniMapHintPosition();

	luaReturnNum(0);
}

luaFunc(entityFollowEntity)
{
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	if (e)
	{
		e->followEntity = dsq->getEntityByName(lua_tostring(L, 2));
	}

	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(toggleInput)
{
	int v = lua_tointeger(L, 1);
	if (v)
		dsq->game->avatar->enableInput();
	else
		dsq->game->avatar->disableInput();
	luaReturnNum(0);
}

luaFunc(bone_offset)
{
	Bone *b = bone(L);
	if (b)
		b->offset.interpolateTo(Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	luaReturnNum(0);
}

luaFunc(entity_offset)
{
	Entity *e = entity(L);
	if (e)
		e->offset.interpolateTo(Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	luaReturnNum(0);
}

luaFunc(warpAvatar)
{
	dsq->game->positionToAvatar = Vector(lua_tointeger(L, 2),lua_tointeger(L, 3));
	dsq->game->transitionToScene(lua_tostring(L, 1));

	luaReturnNum(0);
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

	luaReturnNum(0);
}

luaFunc(registerSporeChildData)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->continuity.registerSporeChildData(e);
	}
	luaReturnNum(0);
}

luaFunc(entity_setDamageTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDamageTarget((DamageType)lua_tointeger(L, 2), getBool(L, 3));
	}
	luaReturnNum(0);
}

luaFunc(entity_setAllDamageTargets)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setAllDamageTargets(getBool(L, 2));
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_clearTargetPoints)
{
	Entity *e = entity(L);
	if (e)
		e->clearTargetPoints();
	luaReturnNum(0);
}

luaFunc(entity_addTargetPoint)
{
	Entity *e = entity(L);
	if (e)
		e->addTargetPoint(Vector(lua_tonumber(L,2), lua_tonumber(L, 3)));
	luaReturnNum(0);
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
	dsq->playVisualEffect(lua_tonumber(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	luaReturnNum(0);
}

luaFunc(playNoEffect)
{
	dsq->playNoEffect();
	luaReturnNum(0);
}

luaFunc(emote)
{
	int emote = lua_tonumber(L, 1);
	dsq->emote.playSfx(emote);
	luaReturnNum(0);
}

luaFunc(playSfx)
{
	int freq = lua_tonumber(L, 2);
	float vol = lua_tonumber(L, 3);
	int loops = lua_tointeger(L, 4);
	if (vol == 0)
		vol = 1;

	PlaySfx sfx;
	sfx.name = getString(L, 1);
	sfx.vol = vol;
	sfx.freq = freq;
	sfx.loops = loops;

	void *handle = NULL;
	
	if (!dsq->isSkippingCutscene())
		handle = core->sound->playSfx(sfx);
	
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
	luaReturnNum(0);
}

luaFunc(stopMusic)
{
	dsq->sound->stopMusic();
	luaReturnNum(0);
}

luaFunc(playMusic)
{
	float crossfadeTime = 0.8;
	dsq->sound->playMusic(std::string(lua_tostring(L, 1)), SLT_LOOP, SFT_CROSS, crossfadeTime);
	luaReturnNum(0);
}

luaFunc(playMusicStraight)
{
	dsq->sound->setMusicFader(1,0);
	dsq->sound->playMusic(getString(L, 1), SLT_LOOP, SFT_IN, 0.5); //SFT_IN, 0.1);//, SFT_IN, 0.2);
	luaReturnNum(0);
}

luaFunc(playMusicOnce)
{
	float crossfadeTime = 0.8;
	dsq->sound->playMusic(std::string(lua_tostring(L, 1)), SLT_NONE, SFT_CROSS, crossfadeTime);
	luaReturnNum(0);
}

luaFunc(addInfluence)
{
	ParticleInfluence pinf;
	pinf.pos.x = lua_tonumber(L, 1);
	pinf.pos.y = lua_tonumber(L, 2);
	pinf.size = lua_tonumber(L, 3);
	pinf.spd = lua_tonumber(L, 4);
	dsq->particleManager->addInfluence(pinf);
	luaReturnNum(0);
}

luaFunc(updateMusic)
{
	dsq->game->updateMusic();
	luaReturnNum(0);
}

luaFunc(entity_grabTarget)
{
	Entity *e = entity(L);
	e->attachEntity(e->getTargetEntity(), Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)));
	luaReturnNum(0);
}

luaFunc(entity_clampToHit)
{
	Entity *e = entity(L);
	if (e)
		e->clampToHit();
	luaReturnNum(0);
}

luaFunc(entity_clampToSurface)
{
	Entity *e = entity(L);
	bool ret = e->clampToSurface(lua_tonumber(L, 2));

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

	luaReturnNum(0);
}

luaFunc(entity_adjustPositionBySurfaceNormal)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (!e->ridingOnEntity)
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
	luaReturnInt(0);
}

// HACK: move functionality inside entity class
luaFunc(entity_moveAlongSurface)
{
	ScriptedEntity *e = scriptedEntity(L);

	if (e->isv(EV_CLAMPING,0))
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

	luaReturnNum(0);
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

	luaReturnNum(0);
}

luaFunc(entity_releaseTarget)
{
	Entity *e = entity(L);
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
	luaReturnNum(e->getv(ev));
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
	EV ev = (EV)lua_tointeger(L, 2);
	luaReturnNum(e->getvf(ev));
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

luaFunc(entity_setClampOnSwitchDir)
{
	debugLog("_setClampOnSwitchDir is old");
	luaReturnNum(0);
}

luaFunc(entity_setWidth)
{
	Entity *e = entity(L);
	if (e)
		e->setWidth(lua_tonumber(L, 2));
		//e->width.interpolateTo(Vector(lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	luaReturnNum(0);
}

luaFunc(entity_setHeight)
{
	Entity *e = entity(L);
	if (e)
		e->setHeight(lua_tonumber(L, 2));
		//e->height.interpolateTo(Vector(lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	luaReturnNum(0);
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
	luaReturnNum(0);
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

	luaReturnNum(0);
}

luaFunc(watch)
{
	float t = lua_tonumber(L, 1);
	int quit = lua_tointeger(L, 2);
	dsq->watch(t, quit);
	luaReturnNum(0);
}

luaFunc(wait)
{
	core->main(lua_tonumber(L, 1));
	luaReturnNum(0);
}

luaFunc(warpNaijaToEntity)
{
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	if (e)
	{
		dsq->overlay->alpha.interpolateTo(1, 1);
		core->main(1);

		Vector offset(lua_tointeger(L, 2), lua_tointeger(L, 3));
		dsq->game->avatar->position = e->position + offset;

		dsq->overlay->alpha.interpolateTo(0, 1);
		core->main(1);
	}
	luaReturnNum(0);
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
		int range = lua_tonumber(L, 4);
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
	luaReturnNum(0);
}

luaFunc(entity_delete)
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


luaFunc(entity_setCull)
{
	Entity *e = entity(L);
	bool v = getBool(L, 2);
	if (e)
	{
		e->cull = v;
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_setActivation)
{
	ScriptedEntity *e = scriptedEntity(L);
	//if (!e) return;
	int type = lua_tonumber(L, 2);
	// cursor radius
	int activationRadius = lua_tonumber(L, 3);
	int range = lua_tonumber(L, 4);
	e->activationType = (Entity::ActivationType)type;
	e->activationRange = range;
	e->activationRadius = activationRadius;

	luaReturnNum(0);
}

luaFunc(entity_setCullRadius)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setOverrideCullRadius(lua_tonumber(L, 2));
	}
	luaReturnNum(0);
}

luaFunc(entity_setActivationType)
{
	Entity *e = entity(L);
	int type = lua_tonumber(L, 2);
	e->activationType = (Entity::ActivationType)type;

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
	luaReturnInt(0);
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

	luaReturnNum(0);
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

	luaReturnNum(0);
}

luaFunc(entity_setVelLen)
{
	Entity *e = entity(L);
	int len = lua_tonumber(L, 2);
	if (e)
	{
		e->vel.setLength2D(len);
	}
	luaReturnNum(0);
}

// entity dt speed dir
luaFunc(entity_moveAroundTarget)
{
	Entity *e = entity(L);
	if (e)
		e->moveAroundTarget(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), e->currentEntityTarget);
	// do stuff
	luaReturnNum(0);
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
	luaReturnNum(0);
}

/*
entity_initPart(partName, partTexture, partPosition, partFlipH, partFlipV,
partOffsetInterpolateTo, partOffsetInterpolateTime

*/

luaFunc(entity_partWidthHeight)
{
	ScriptedEntity *e = scriptedEntity(L);
	Quad *r = (Quad*)e->partMap[lua_tostring(L, 2)];
	if (r)
	{
		int w = lua_tointeger(L, 3);
		int h = lua_tointeger(L, 4);
		r->setWidthHeight(w, h);
	}
	luaReturnInt(0);
}

luaFunc(entity_partSetSegs)
{
	ScriptedEntity *e = scriptedEntity(L);
	Quad *r = (Quad*)e->partMap[lua_tostring(L, 2)];
	if (r)
	{
		r->setSegs(lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tonumber(L, 9), lua_tointeger(L, 10));
	}
	luaReturnInt(0);
}

luaFunc(getEntityInGroup)
{
	int gid = lua_tonumber(L, 1);
	int iter = lua_tonumber(L, 2);
	luaReturnPtr(dsq->game->getEntityInGroup(gid, iter));
}

luaFunc(entity_getGroupID)
{
	Entity *e = entity(L);
	int id = 0;
	if(e)
	{
		id = e->getGroupID();
	}
	luaReturnNum(id);
}

luaFunc(entity_getID)
{
	Entity *e = entity(L);
	int id = 0;
	if (e)
	{
		id = e->getID();
		std::ostringstream os;
		os << "id: " << id;
		debugLog(os.str());
	}
	luaReturnNum(id);
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
	p->setEffectOn(getBool(L, 2));
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(node_setElementsInLayerActive)
{
	Path *p = path(L);
	int l = lua_tonumber(L, 2);
	bool v = getBool(L, 3);
	for (Element *e = dsq->getFirstElementOnLayer(l); e; e = e->bgLayerNext)
	{
		if (e && p->isCoordinateInside(e->position))
		{
			debugLog("setting an element to the value");
			e->setElementActive(v);
		}
	}
	luaReturnNum(0);
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

	/*
	std::ostringstream os;
	os << "counted: " << c << " entities with name: " << name;
	if (p)
		os << " in node area: " << p->name;
	debugLog(os.str());
	*/

	luaReturnNum(c);
}

luaFunc(node_getNearestEntity)
{
	//Entity *me = entity(L);
	Path *p = path(L);
	Entity *closest=0;

	if (p && !p->nodes.empty())
	{

		Vector pos = p->nodes[0].position;
		std::string name;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);

		float smallestDist = HUGE_VALF;
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e->isPresent() && e->isNormalLayer())
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
	//Entity *me = entity(L);
	Path *p = path(L);
	Path *closest = 0;
	if (p && !p->nodes.empty())
	{
		std::string name;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);
		closest = dsq->game->getNearestPath(p->nodes[0].position, name);
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
	std::string name;
	if (lua_isstring(L, 2))
		name = lua_tostring(L, 2);
	Path *ignore = path(L, 3);

	Path *closest = dsq->game->getNearestPath(me->position, name, ignore);
	luaReturnPtr(closest);
}

luaFunc(ing_hasIET)
{
	Ingredient *i = getIng(L, 1);
	bool has = i->hasIET((IngredientEffectType)lua_tointeger(L, 2));
	luaReturnBool(has);
}

luaFunc(entity_getNearestEntity)
{
	Entity *me = entity(L);
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

	int range = lua_tointeger(L, 3);
	int type = lua_tointeger(L, 4);
	int damageTarget = lua_tointeger(L, 5);
	Entity *closest = 0;
	float smallestDist = range ? sqr(range) : HUGE_VALF;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e != me && e->isPresent() && e->isNormalLayer())
		{
			if (!name || ((nocasecmp(e->name, name)==0) == nameCheck))
			{
				if (type == 0 || e->getEntityType() == type)
				{
					if (damageTarget == 0 || e->isDamageTarget((DamageType)damageTarget))
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
	luaReturnPtr(NULL);
}

luaFunc(setCutscene)
{
	dsq->setCutscene(getBool(L, 1), getBool(L, 2));
	luaReturnPtr(NULL);
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

int _alpha(lua_State *L, RenderObject *r)
{
	if (r)
	{
		r->alpha.stop();
		r->alpha.interpolateTo(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	luaReturnNum(0);
}

luaFunc(bone_alpha)
{
	return _alpha(L, boneToRenderObject(L));
}

luaFunc(entity_alpha)
{
	return _alpha(L, entityToRenderObject(L));
}

luaFunc(entity_partAlpha)
{
	ScriptedEntity *e = scriptedEntity(L);
	RenderObject *r = e->partMap[lua_tostring(L, 2)];
	if (r)
	{

		float start = lua_tonumber(L, 3);
		if (start != -1)
			r->alpha = start;
		r->alpha.interpolateTo(lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8));
	}

	luaReturnNum(0);
}

luaFunc(entity_partBlendType)
{
	ScriptedEntity *e = scriptedEntity(L);
	e->partMap[lua_tostring(L, 2)]->setBlendType(lua_tointeger(L, 3));
	luaReturnInt(0);
}

luaFunc(entity_partRotate)
{
	ScriptedEntity *e = scriptedEntity(L);
	RenderObject *r = e->partMap[lua_tostring(L, 2)];
	if (r)
	{
		r->rotation.interpolateTo(Vector(0,0,lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7));
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
	luaReturnNum(0);
}

luaFunc(entity_scale)
{
	Entity *e = entity(L);
	float time = lua_tonumber(L, 4);
	//e->scale = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	e->scale.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), 0), time, lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	luaReturnNum(0);
}

luaFunc(entity_switchLayer)
{
	Entity *e = entity(L);
	int lcode = lua_tonumber(L, 2);
	int toLayer = LR_ENTITIES;

	toLayer = dsq->getEntityLayerToLayer(lcode);

	if (e->getEntityType() == ET_AVATAR)
		toLayer = LR_ENTITIES;

	core->switchRenderObjectLayer(e, toLayer);
	luaReturnNum(0);
}

luaFunc(entity_isScaling)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->scale.isInterpolating();
	}
	luaReturnBool(v);
}

luaFunc(entity_getScale)
{
	Entity *e = entity(L);
	if (e)
		luaReturnVec2(e->scale.x, e->scale.y);
	else
		luaReturnVec2(0, 0);
}

// entity numSegments segmentLength width texture
luaFunc(entity_initHair)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->initHair(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tostring(L, 5));
	}
	luaReturnNum(0);
}

luaFunc(entity_getHairPosition)
{
	ScriptedEntity *se = scriptedEntity(L);
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

luaFunc(entity_setUpdateCull)
{
	Entity *e = entity(L);
	if (e)
	{
		e->updateCull = lua_tonumber(L, 2);
	}
	luaReturnNum(0);
}

// entity x y z
luaFunc(entity_setHairHeadPosition)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->setHairHeadPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	}
	luaReturnNum(0);
}

luaFunc(entity_updateHair)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->updateHair(lua_tonumber(L, 2));
	}
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(entity_initPart)
{
	std::string partName(lua_tostring(L, 2));
	std::string partTex(lua_tostring(L, 3));
	Vector partPosition(lua_tointeger(L, 4), lua_tointeger(L, 5));
	int renderAfter = lua_tointeger(L, 6);
	bool partFlipH = lua_tointeger(L, 7);
	bool partFlipV = lua_tointeger(L,8);
	Vector offsetInterpolateTo(lua_tointeger(L, 9), lua_tointeger(L, 10));
	float offsetInterpolateTime = lua_tonumber(L, 11);


	ScriptedEntity *e = scriptedEntity(L);

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

	luaReturnNum(0);
}

luaFunc(entity_findTarget)
{
	Entity *e = entity(L);
	if (e)
		e->findTarget(lua_tointeger(L, 2), lua_tointeger(L, 3), e->currentEntityTarget);

	luaReturnNum(0);
}

luaFunc(entity_doFriction)
{
	Entity *e = entity(L);
	if (e)
	{
		e->doFriction(lua_tonumber(L, 2), lua_tointeger(L, 3));
	}
	luaReturnNum(0);
}

luaFunc(entity_doGlint)
{
	Entity *e = entity(L);
	if (e)
		e->doGlint(e->position, Vector(2,2), getString(L,2), (RenderObject::BlendTypes)lua_tointeger(L, 3));
	luaReturnNum(0);
}

luaFunc(entity_getPosition)
{
	Entity *e = entity(L);
	float x=0,y=0;
	if (e)
	{
		x = e->position.x;
		y = e->position.y;
	}
	luaReturnVec2(x, y);
}

luaFunc(entity_getOffset)
{
	Entity *e = entity(L);
	float x=0,y=0;
	if (e)
	{
		x = e->offset.x;
		y = e->offset.y;
	}
	luaReturnVec2(x, y);
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
	luaReturnInt(int(entity(L)->getTargetEntity()->position.x));
}

luaFunc(entity_getTargetPositionY)
{
	luaReturnInt(int(entity(L)->getTargetEntity()->position.y));
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

luaFunc(entity_isInterpolating)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->position.isInterpolating();
	}
	luaReturnBool(v);
}

luaFunc(entity_isRotating)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->rotation.isInterpolating();
	}
	luaReturnBool(v);
}


luaFunc(entity_interpolateTo)
{
	Entity *e = entity(L);
	int x = lua_tonumber(L, 2);
	int y = lua_tonumber(L, 3);
	float t = lua_tonumber(L, 4);
	if (e)
	{
		e->position.interpolateTo(Vector(x, y), t);
	}
	luaReturnNum(0);
}

luaFunc(entity_setEatType)
{
	Entity *e = entity(L);
	int et = lua_tointeger(L, 2);
	if (e)
		e->setEatType((EatType)et, getString(L, 3));
	luaReturnInt(et);
}

luaFunc(entity_setPositionX)
{
	Entity *e = entity(L);
	if (e)
		e->position.x = lua_tointeger(L, 2);
	luaReturnInt(0);
}

luaFunc(entity_setPositionY)
{
	Entity *e = entity(L);
	if (e)
		e->position.y = lua_tointeger(L, 2);
	luaReturnInt(0);
}

luaFunc(entity_rotateTo)
{
	Entity *e = entity(L);
	if (e)
		e->rotation.interpolateTo(Vector(0,0,lua_tointeger(L, 2)), lua_tonumber(L, 3));
	luaReturnInt(0);
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
		std::string gfx = lua_tostring(L, 5);
		float colorx = lua_tonumber(L, 6);
		float colory = lua_tonumber(L, 7);
		float colorz = lua_tonumber(L, 8);
		float offx = lua_tonumber(L, 9);
		float offy = lua_tonumber(L, 10);
		float poisonTime = lua_tonumber(L, 11);

		GasCloud *c = new GasCloud(e, e->position + Vector(offx, offy), gfx, Vector(colorx, colory, colorz), radius, life, damage, false, poisonTime);
		core->getTopStateData()->addRenderObject(c, LR_PARTICLES);
	}
	luaReturnInt(0);
}

luaFunc(isInputEnabled)
{
	luaReturnBool(dsq->game->avatar->isInputEnabled());
}

luaFunc(enableInput)
{
	dsq->game->avatar->enableInput();
	luaReturnInt(0);
}

luaFunc(disableInput)
{
	dsq->game->avatar->disableInput();
	luaReturnInt(0);
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

	luaReturnInt(0);
}

luaFunc(doModSelect)
{
	dsq->doModSelect();
	luaReturnInt(0);
}

luaFunc(doLoadMenu)
{
	dsq->doLoadMenu();
	luaReturnInt(0);
}

luaFunc(resetContinuity)
{
	dsq->continuity.reset();
	luaReturnInt(0);
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
	luaReturnNum(0);
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
	dsq->overlay->color = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	dsq->overlay->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnInt(0);
}

luaFunc(fade2)
{
	dsq->overlay2->color = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	dsq->overlay2->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnInt(0);
}

luaFunc(fade3)
{
	dsq->overlay3->color = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	dsq->overlay3->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnInt(0);
}

luaFunc(vision)
{
	dsq->vision(lua_tostring(L, 1), lua_tonumber(L, 2), getBool(L, 3));
	luaReturnNum(0);
}

luaFunc(musicVolume)
{
	dsq->sound->setMusicFader(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNum(0);
}

luaFunc(voice)
{
	float vmod = lua_tonumber(L, 2);
	if (vmod == 0)
		vmod = -1;
	else if (vmod == -1)
		vmod = 0;
	dsq->voice(lua_tostring(L, 1), vmod);
	luaReturnInt(0);
}

luaFunc(voiceOnce)
{
	dsq->voiceOnce(lua_tostring(L, 1));
	luaReturnInt(0);
}

luaFunc(voiceInterupt)
{
	dsq->voiceInterupt(lua_tostring(L, 1));
	luaReturnInt(0);
}

luaFunc(stopVoice)
{
	dsq->stopVoice();
	luaReturnNum(0);
}

luaFunc(stopAllSfx)
{
	dsq->sound->stopAllSfx();
	luaReturnNum(0);
}

luaFunc(stopAllVoice)
{
	dsq->sound->stopAllVoice();
	luaReturnNum(0);
}

luaFunc(fadeIn)
{
	dsq->overlay->alpha.interpolateTo(0, lua_tonumber(L, 1));
	luaReturnInt(0);
}

luaFunc(fadeOut)
{
	dsq->overlay->color = 0;
	dsq->overlay->alpha.interpolateTo(1, lua_tonumber(L, 1));
	luaReturnInt(0);
}

luaFunc(entity_setWeight)
{
	CollideEntity *e = collideEntity(L);
	if (e)
		e->weight = lua_tointeger(L, 2);
	luaReturnInt(0);
}

luaFunc(pickupGem)
{
	dsq->continuity.pickupGem(getString(L), !getBool(L, 2));
	luaReturnNum(0);
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

	luaReturnNum(0);
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

	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(setElementLayerVisible)
{
	int l = lua_tonumber(L, 1);
	bool v = getBool(L, 2);
	dsq->game->setElementLayerVisible(l, v);
	luaReturnNum(0);
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

luaFunc(entity_getAlpha)
{
	Entity *e = entity(L);
	float a = 0;
	if (e)
	{
		a = e->alpha.x;
	}
	luaReturnNum(a);
}

luaFunc(isObstructed)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	luaReturnBool(dsq->game->isObstructed(TileVector(Vector(x,y))));
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
	luaReturnNum(0);
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
	luaReturnNum(0);
}

luaFunc(avatar_toggleMovement)
{
	dsq->game->avatar->toggleMovement((bool)lua_tointeger(L, 1));
	luaReturnNum(0);
}

luaFunc(clearShots)
{
	Shot::killAllShots();
	luaReturnNum(0);
}

luaFunc(clearHelp)
{
	float t = 0.4;

	/*
	RenderObjects *r = &core->renderObjectLayers[LR_HELP].renderObjects;


	for (RenderObjects::iterator i = r->begin(); i != r->end(); i++)
	{
		RenderObject *ro = (*i);
	*/

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

	luaReturnNum(0);
}

luaFunc(setLiPower)
{
	float m = lua_tonumber(L, 1);
	float t = lua_tonumber(L, 2);
	dsq->continuity.setLiPower(m, t); 
	luaReturnNum(0);
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

//--------------------------------------------------------------------------------------------

#define luaRegister(func)	{#func, l_##func}

static const struct {
	const char *name;
	lua_CFunction func;
} luaFunctionTable[] = {

	// override Lua's standard dofile(), so we can handle filename case issues.
	{"dofile", l_dofile_caseinsensitive},

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


	luaRegister(getWaterLevel),
	luaRegister(setWaterLevel),


	luaRegister(getEntityInGroup),

	luaRegister(createQuad),
	luaRegister(quad_delete),
	luaRegister(quad_scale),
	luaRegister(quad_rotate),

	luaRegister(quad_color),
	luaRegister(quad_alpha),
	luaRegister(quad_alphaMod),
	luaRegister(quad_getAlpha),

	luaRegister(quad_setPosition),
	luaRegister(quad_setBlendType),


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


	luaRegister(entity_setClampOnSwitchDir),

	luaRegister(entity_setBeautyFlip),
	luaRegister(entity_setInvincible),

	luaRegister(setInvincible),





	luaRegister(entity_setLife),
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

	luaRegister(entity_setCanLeaveWater),

	luaRegister(entity_pullEntities),

	luaRegister(entity_setEntityLayer),
	luaRegister(entity_setRenderPass),

	luaRegister(entity_clearTargetPoints),
	luaRegister(entity_addTargetPoint),


	luaRegister(entity_setCullRadius),
	luaRegister(entity_setUpdateCull),

	luaRegister(entity_switchLayer),

	luaRegister(entity_debugText),


	luaRegister(avatar_setCanDie),
	luaRegister(avatar_toggleCape),
	luaRegister(avatar_setPullTarget),


	luaRegister(avatar_clampPosition),
	luaRegister(avatar_updatePosition),

	luaRegister(pause),
	luaRegister(unpause),


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
	luaRegister(entity_isVelIn),
	luaRegister(entity_isValidTarget),


	luaRegister(entity_isUnderWater),
	luaRegister(entity_checkSplash),




	luaRegister(entity_getRandomTargetPoint),
	luaRegister(entity_getTargetPoint),


	luaRegister(entity_setTargetRange),


	luaRegister(entity_setCollideWithAvatar),


	luaRegister(bone_setRenderPass),
	luaRegister(bone_setVisible),
	luaRegister(bone_isVisible),

	luaRegister(bone_addSegment),
	luaRegister(entity_setSegs),
	luaRegister(bone_setSegs),
	luaRegister(bone_update),


	luaRegister(bone_setSegmentOffset),
	luaRegister(bone_setSegmentProps),
	luaRegister(bone_setSegmentChainHead),
	luaRegister(bone_setAnimated),
	luaRegister(bone_showFrame),

	luaRegister(bone_lookAtEntity),

	luaRegister(bone_setTexture),

	luaRegister(bone_scale),
	luaRegister(bone_setBlendType),


	luaRegister(entity_partSetSegs),


	luaRegister(entity_adjustPositionBySurfaceNormal),
	luaRegister(entity_applySurfaceNormalForce),

	luaRegister(createBeam),
	luaRegister(beam_setAngle),
	luaRegister(beam_setPosition),
	luaRegister(beam_setTexture),
	luaRegister(beam_setDamage),
	luaRegister(beam_setBeamWidth),


	luaRegister(beam_delete),

	luaRegister(getStringBank),

	luaRegister(isPlat),


	luaRegister(createEntity),
	luaRegister(entity_setWeight),
	luaRegister(entity_setBlendType),

	luaRegister(entity_setActivationType),
	luaRegister(entity_setColor),
	{"entity_color", l_entity_setColor},
	luaRegister(entity_playSfx),

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

	luaRegister(entity_getNormal),

	luaRegister(entity_getAlpha),
	luaRegister(entity_getAimVector),

	luaRegister(entity_getVectorToEntity),

	luaRegister(entity_getVelLen),

	luaRegister(entity_getDistanceToTarget),
	luaRegister(entity_delete),
	luaRegister(entity_move),


	luaRegister(entity_moveToFront),
	luaRegister(entity_moveToBack),




	luaRegister(entity_getID),
	luaRegister(entity_getGroupID),

	luaRegister(getEntityByID),

	luaRegister(entity_setBounce),
	luaRegister(entity_setPosition),
	luaRegister(entity_setInternalOffset),
	luaRegister(entity_setActivation),
	luaRegister(entity_rotateToEntity),
	luaRegister(entity_rotateTo),
	luaRegister(entity_rotateOffset),

	luaRegister(entity_fireGas),
	luaRegister(entity_rotateToTarget),

	luaRegister(entity_switchSurfaceDirection),

	luaRegister(entity_offset),
	luaRegister(entity_moveAlongSurface),
	luaRegister(entity_rotateToSurfaceNormal),
	luaRegister(entity_clampToSurface),
	luaRegister(entity_checkSurface),
	luaRegister(entity_clampToHit),


	luaRegister(entity_grabTarget),
	luaRegister(entity_releaseTarget),

	luaRegister(entity_getStateTime),
	luaRegister(entity_setStateTime),

	luaRegister(entity_scale),
	luaRegister(entity_getScale),

	luaRegister(entity_doFriction),

	luaRegister(entity_partWidthHeight),
	luaRegister(entity_partBlendType),
	luaRegister(entity_partRotate),
	luaRegister(entity_partAlpha),

	luaRegister(entity_getHealth),
	luaRegister(entity_pushTarget),
	luaRegister(entity_flipHorizontal),
	luaRegister(entity_flipVertical),
	{"entity_fh", l_entity_flipHorizontal},
	luaRegister(entity_fhTo),
	luaRegister(entity_update),
	luaRegister(entity_msg),
	luaRegister(entity_updateMovement),
	luaRegister(entity_updateCurrents),
	luaRegister(entity_updateLocalWarpAreas),

	luaRegister(entity_setPositionX),
	luaRegister(entity_setPositionY),
	luaRegister(entity_getPosition),
	luaRegister(entity_getOffset),

	luaRegister(entity_getTargetPositionX),
	luaRegister(entity_getTargetPositionY),

	luaRegister(entity_incrTargetLeaches),
	luaRegister(entity_decrTargetLeaches),
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
	luaRegister(entity_isScaling),

	luaRegister(entity_isRidingOnEntity),

	luaRegister(entity_isBeingPulled),

	luaRegister(entity_isNearObstruction),
	luaRegister(entity_isDead),



	luaRegister(entity_isTargetInRange),
	luaRegister(entity_getDistanceToEntity),

	luaRegister(entity_isInvincible),

	luaRegister(entity_isNearGround),

	luaRegister(entity_moveTowardsTarget),
	luaRegister(entity_moveAroundTarget),

	luaRegister(entity_moveTowardsAngle),
	luaRegister(entity_moveAroundAngle),
	luaRegister(entity_moveTowards),
	luaRegister(entity_moveAround),

	luaRegister(entity_setVelLen),

	luaRegister(entity_setMaxSpeed),
	luaRegister(entity_getMaxSpeed),
	luaRegister(entity_setMaxSpeedLerp),
	luaRegister(entity_setState),
	luaRegister(entity_getState),
	luaRegister(entity_getEnqueuedState),

	luaRegister(entity_getPrevState),
	luaRegister(entity_doCollisionAvoidance),
	luaRegister(entity_animate),
	luaRegister(entity_setAnimLayerTimeMult),

	luaRegister(entity_setCurrentTarget),
	luaRegister(entity_warpToPathStart),
	luaRegister(entity_stopInterpolating),

	luaRegister(entity_followPath),
	luaRegister(entity_isFollowingPath),
	luaRegister(entity_followEntity),
	luaRegister(entity_sound),
	luaRegister(entity_soundFreq),


	luaRegister(entity_enableMotionBlur),
	luaRegister(entity_disableMotionBlur),


	luaRegister(registerSporeChildData),
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


	luaRegister(setOverrideMusic),

	luaRegister(setOverrideVoiceFader),
	luaRegister(setGameSpeed),
	luaRegister(sendEntityMessage),
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


	luaRegister(castSong),
	luaRegister(isObstructed),
	luaRegister(isObstructedBlock),

	luaRegister(isFlag),

	luaRegister(entity_isFlag),
	luaRegister(entity_setFlag),

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
	luaRegister(avatar_getRollDirection),

	luaRegister(avatar_fallOffWall),
	luaRegister(avatar_setBlockSinging),


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
	luaRegister(web_delete),

	luaRegister(createSpore),



	luaRegister(shot_getPosition),
	luaRegister(shot_setAimVector),
	luaRegister(shot_setOut),
	luaRegister(shot_setVel),
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

	luaRegister(entity_addVel),
	luaRegister(entity_addVel2),
	luaRegister(entity_addRandomVel),

	luaRegister(entity_clearVel),
	luaRegister(entity_clearVel2),


	luaRegister(entity_revive),

	luaRegister(entity_getTarget),
	luaRegister(entity_isState),

	luaRegister(entity_setProperty),
	luaRegister(entity_isProperty),


	luaRegister(entity_initHair),
	luaRegister(entity_getHairPosition),

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
	luaRegister(loadMap),

	luaRegister(loadSound),

	luaRegister(node_activate),
	luaRegister(node_getName),
	luaRegister(node_getPathPosition),
	luaRegister(node_getPosition),
	luaRegister(node_setPosition),
	luaRegister(node_getContent),
	luaRegister(node_getAmount),
	luaRegister(node_getSize),
	luaRegister(node_setEffectOn),

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


	luaRegister(entity_watchEntity),

	luaRegister(entity_setCollideRadius),
	luaRegister(entity_getCollideRadius),
	luaRegister(entity_setTouchDamage),

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

	luaRegister(entity_interpolateTo),
	luaRegister(entity_isInterpolating),
	luaRegister(entity_isRotating),


	luaRegister(entity_isFlippedHorizontal),
	{"entity_isfh", l_entity_isFlippedHorizontal},
	luaRegister(entity_isFlippedVertical),

	luaRegister(entity_setWidth),
	luaRegister(entity_setHeight),
	luaRegister(entity_push),

	luaRegister(entity_alpha),

	luaRegister(findWall),


	luaRegister(overrideZoom),
	luaRegister(disableOverrideZoom),



	luaRegister(spawnAroundEntity),

	luaRegister(entity_toggleBone),

	luaRegister(bone_damageFlash),
	luaRegister(bone_setColor),
	luaRegister(bone_setPosition),
	luaRegister(bone_rotate),
	luaRegister(bone_rotateOffset),
	luaRegister(bone_getRotation),
	luaRegister(bone_offset),

	luaRegister(bone_alpha),

	luaRegister(bone_setTouchDamage),
	luaRegister(bone_getNormal),
	luaRegister(bone_getPosition),
	luaRegister(bone_getScale),
	luaRegister(bone_getWorldPosition),
	luaRegister(bone_getWorldRotation),



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
	luaRegister(entity_velx),
	luaRegister(entity_vely),
	luaRegister(entity_velTowards),



	luaRegister(updateMusic),

	luaRegister(entity_touchAvatarDamage),
	luaRegister(getNaija),
	luaRegister(getLi),
	luaRegister(setLi),

	luaRegister(randAngle360),
	luaRegister(randVector),

	luaRegister(entity_getNearestEntity),
	luaRegister(entity_getNearestBoneToPosition),

	luaRegister(entity_getNearestNode),

	luaRegister(node_getNearestEntity),
	luaRegister(node_getNearestNode),


	luaRegister(entity_getRotation),

	luaRegister(node_isEntityIn),



	luaRegister(isLeftMouse),
	luaRegister(isRightMouse),


	luaRegister(setTimerTextAlpha),
	luaRegister(setTimerText),


	luaRegister(getWallNormal),
	luaRegister(getLastCollidePosition),
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

	// Entity Behaviors
	luaConstant(BT_NORMAL),
	luaConstant(BT_MOTHER),
	luaConstant(BT_ACTIVEPET),

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


	luaConstant(FRAME_TIME),

	luaConstant(FORMUPGRADE_ENERGY1),
	luaConstant(FORMUPGRADE_ENERGY2),
	luaConstant(FORMUPGRADE_BEAST),

	luaConstant(TILE_SIZE),

	luaConstant(INPUT_MOUSE),
	luaConstant(INPUT_JOYSTICK),
	luaConstant(INPUT_KEYBOARD),
};

//============================================================================================
// F U N C T I O N S
//============================================================================================

void ScriptInterface::init()
{
	baseState = createLuaVM();
}

lua_State *ScriptInterface::createLuaVM()
{
	lua_State *state = lua_open();	/* opens Lua */
	luaopen_base(state);			/* opens the basic library */
	luaopen_table(state);			/* opens the table library */
	luaopen_string(state);			/* opens the string lib. */
	luaopen_math(state);			/* opens the math lib. */

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

void ScriptInterface::shutdown()
{
}

Script *ScriptInterface::openScript(const std::string &file)
{
	std::string realFile = core->adjustFilenameCase(file);
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
		int result = luaL_loadfile(baseState, realFile.c_str());
		if (result != 0)
		{
			debugLog("Error loading script [" + realFile + "]: " + lua_tostring(baseState, -1));
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
			debugLog("Error doing initial run of script [" + realFile + "]: " + lua_tostring(baseState, -1));
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
		debugLog("Unable to create new thread for script [" + realFile + "]");
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

bool ScriptInterface::runScript(const std::string &file, const std::string &func)
{
	std::string realFile = file;
	if (file.find('/')==std::string::npos)
		realFile = "scripts/" + file + ".lua";
	Script *script = openScript(realFile);
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
	lua_getglobal(L, "_scriptfuncs");
	lua_getfield(L, -1, file.c_str());
	lua_remove(L, -2);
	lua_getfield(L, -1, name);
	lua_remove(L, -2);
}

bool Script::doCall(int nparams, int nrets)
{
	// Push the current value of the "v" global onto the Lua stack,
	// so we can restore the current script's instance variable table
	// before returning.
	lua_getglobal(L, "v");
	lua_insert(L, -(nparams+2));
	fixupLocalVars(L);

	bool result;
	if (lua_pcall(L, nparams, nrets, 0) == 0)
	{
		result = true;
	}
	else
	{
		lastError = lua_tostring(L, -1);
		lua_pop(L, 1);
		result = false;
	}

	if (nrets > 0)
	{
		lua_pushvalue(L, -(nrets+1));
		lua_remove(L, -(nrets+2));
	}
	lua_setglobal(L, "v");

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
