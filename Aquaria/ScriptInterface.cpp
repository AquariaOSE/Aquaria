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

// Internal Lua is built as C++ code, so we need to include the headers properly
#ifndef AQUARIA_INTERNAL_LUA
extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#ifndef AQUARIA_INTERNAL_LUA
} // end extern "C"
#endif

#include "luaalloc.h"

#include <SDL.h>
#include "ScriptInterface.h"
#include "ScriptObject.h"

#include "ReadXML.h"

#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "ScriptedEntity.h"
#include "Shot.h"
#include "Entity.h"
#include "Web.h"
#include "GridRender.h"
#include "AfterEffect.h"
#include "PathFinding.h"
#include <algorithm>
#include "Gradient.h"
#include "InGameMenu.h"
#include "GasCloud.h"
#include "Ingredient.h"
#include "Beam.h"
#include "Hair.h"
#include "Spore.h"
#include "Shader.h"
#include "ActionMapper.h"
#include "QuadGrid.h"
#include "WorldMapRender.h"
#include "SplineQuad.h"


#include "MathFunctions.h"

#undef quad // avoid conflict with quad precision types

// Define this to 1 to check types of pointers passed to functions,
// and warn if a type mismatch is detected. In this case,
// the pointer is treated as NULL, to avoid crashing or undefined behavior.
// Note: There are a few functions that depend on this (isObject and related).
//       They will still work as expected when this is disabled.
#define CHECK_POINTER_TYPES 1

// If true, send all sort of script errors to errorLog instead of debugLog.
// On win32/OSX, this pops up message boxes which help to locate errors easily,
// but can be annoying for regular gameplay.
bool loudScriptErrors = false;

// Set this to true to complain whenever a script tries to
// get or set a global variable.
bool complainOnGlobalVar = false;

// Set this to true to complain whenever a script tries to get an undefined
// thread-local variable.
bool complainOnUndefLocal = false;

// Set to true to make 'os' and 'io' Lua tables accessible
bool allowUnsafeFunctions = false;


// List of all interface functions called by C++ code, terminated by NULL.
static const char * const interfaceFunctions[] = {
	"action",
	"activate",
	"animationKey",
	"castSong",
	"canShotHit",
	"cookFailure",
	"damage",
	"deathNotify",
	"dieEaten",
	"dieNormal",
	"enterState",
	"entityDied",
	"exitState",
	"exitTimer",
	"getIngredientEffectString",
	"hitEntity",
	"hitSurface",
	"init",
	"lightFlare",
	"msg",
	"postInit",
	"shiftWorlds",
	"shotHitEntity",
	"song",
	"songNote",
	"songNoteDone",
	"sporesDropped",
	"targetDied",
	"update",
	"useIngredient",
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

static inline void luaPushPointerNonNull(lua_State *L, void *ptr)
{
	assert(ptr);
	lua_pushlightuserdata(L, ptr);
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
	compile_assert(oo(Path) == oo(ParticleEffect));
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
template <typename T>
inline static void ensureTypeNoError(lua_State *L, T *& ptr, ScriptObjectType ty)
{
	if (ptr)
	{
		ScriptObject *so = (ScriptObject*)(ptr);
		if (!so->isType(ty))
			ptr = NULL;
	}
}
#  define ENSURE_TYPE(ptr, ty) ensureType(L, (ptr), (ty))
#  define ENSURE_TYPE_NO_ERROR(ptr, ty) ensureTypeNoError(L, (ptr), (ty))
#  define typecheckOnly(func) func
#else
#  define ENSURE_TYPE(ptr, ty)
#  define ENSURE_TYPE_NO_ERROR(ptr, ty)
#  define typecheckOnly(func)
#endif

// Slower than just lua_touserdata() and a cast, so this is only used for isEntity(), isNode() ... functions to be extra safe
static inline ScriptObject *getScriptObjectSlow(lua_State *L, int slot = 1)
{
	int tt = lua_type(L, slot);
	return (ScriptObject*)(tt == LUA_TLIGHTUSERDATA ? lua_touserdata(L, slot) : NULL);
}

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
ScriptedEntity *scriptedEntityOpt(lua_State *L, int slot = 1)
{
	ScriptedEntity *se = (ScriptedEntity*)lua_touserdata(L, slot);
	ENSURE_TYPE_NO_ERROR(se, SCO_SCRIPTED_ENTITY); // Dont't error if not ScriptedEntity...
	if (se)
		return se;

	ENSURE_TYPE(se, SCO_ENTITY); // ... but error if not even Entity
	return NULL; // still return NULL
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
Shot *getShotOpt(lua_State *L, int slot = 1)
{
	Shot *shot = (Shot*)lua_touserdata(L, slot);
	if(shot)
	{
		ENSURE_TYPE(shot, SCO_SHOT);
	}
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
int getBoolOrInt(lua_State *L, int slot = 1)
{
	if (lua_isnumber(L, slot))
	{
		return lua_tointeger(L, slot);
	}
	else if (lua_islightuserdata(L, slot))
	{
		return (lua_touserdata(L, slot) != NULL);
	}
	else if (lua_isboolean(L, slot))
	{
		return !!lua_toboolean(L, slot);
	}
	return 0;
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
Entity *entityOpt(lua_State *L, int slot = 1)
{
	Entity *ent = (Entity*)lua_touserdata(L, slot);
	if(ent)
	{
		ENSURE_TYPE(ent, SCO_ENTITY);
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
Hair *hair(lua_State *L, int slot = 1)
{
	Hair *h = (Hair*)lua_touserdata(L, slot);
	ENSURE_TYPE(h, SCO_HAIR);
	if (!h)
	{
		scriptDebug(L, "Hair Invalid Pointer");
	}
	return h;
}

static inline
SplineQuad *splineq(lua_State *L, int slot = 1)
{
	SplineQuad *h = (SplineQuad*)lua_touserdata(L, slot);
	ENSURE_TYPE(h, SCO_SPLINEQUAD);
	if (!h)
	{
		scriptDebug(L, "SplineQuad Invalid Pointer");
	}
	return h;
}

static inline
BlendType getBlendType(lua_State *L, int slot = 1)
{
	int bt = lua_tointeger(L, slot);
	return (BlendType)((bt >= BLEND_DEFAULT && bt < _BLEND_MAXSIZE) ? bt : BLEND_DISABLED);
}

static inline
Path *pathFromName(lua_State *L, int slot = 1)
{
	std::string s = getString(L, slot);
	stringToLower(s);
	Path *p = game->getPathByName(s);
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
	if (!p)
		scriptDebug(L, "Invalid Node/Path");
	return p;
}

static inline
Path *pathOpt(lua_State *L, int slot = 1)
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
CollideQuad *getCollideQuad(lua_State *L, int slot = 1)
{
	CollideQuad *q = (CollideQuad*)lua_touserdata(L, slot);
	ENSURE_TYPE(q, SCO_COLLIDE_QUAD);
	if (!q)
		scriptDebug(L, "Invalid CollideQuad");
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

static SkeletalSprite *getSkeletalSprite(Entity *e)
{
	return e ? &e->skeletalSprite : NULL;
}

static inline
ParticleEffect *getParticle(lua_State *L, int slot = 1)
{
	ParticleEffect *q = (ParticleEffect*)lua_touserdata(L, slot);
	ENSURE_TYPE(q, SCO_PARTICLE_EFFECT);
	if (!q)
		scriptDebug(L, "Invalid Particle Effect");
	return q;
}

static inline
int getShaderHandle(lua_State *L, int slot = 1)
{
	return lua_tointeger(L, slot);
}

static inline
Shader *getShaderPtr(lua_State *L, int slot = 1)
{
	AfterEffectManager *ae = core->afterEffectManager;
	return ae ? ae->getShaderPtr(getShaderHandle(L, slot)) : NULL;
}

static inline
QuadGrid *getQuadGrid(lua_State *L, int slot = 1)
{
	QuadGrid *q = (QuadGrid*)lua_touserdata(L, slot);
	ENSURE_TYPE(q, SCO_QUAD_GRID);
	if (!q)
		scriptDebug(L, "Invalid QuadGrid");
	return q;
}

static bool looksLikeGlobal(const char *s)
{
	for( ; *s; ++s)
		if( !((*s >= 'A' && *s <= 'Z') || *s == '_' || (*s >= '0' && *s <= '9')) ) // accept any uppercase, number, and _ char
			return false;
	return true;
}

inline float interpolateVec1(lua_State *L, InterpolatedVector& vec, int argOffs)
{
	return vec.interpolateTo(
		lua_tonumber (L, argOffs  ),  // value
		lua_tonumber (L, argOffs+1),  // time
		lua_tointeger(L, argOffs+2),  // loopType
		getBool      (L, argOffs+3),  // pingPong
		getBool      (L, argOffs+4)); // ease
}

inline float interpolateVec1z(lua_State *L, InterpolatedVector& vec, int argOffs)
{
	return vec.interpolateTo(
		Vector(0.0f, 0.0f, lua_tonumber (L, argOffs  )),  // value, last component
		lua_tonumber (L, argOffs+1),  // time
		lua_tointeger(L, argOffs+2),  // loopType
		getBool      (L, argOffs+3),  // pingPong
		getBool      (L, argOffs+4)); // ease
}

inline float interpolateVec2(lua_State *L, InterpolatedVector& vec, int argOffs)
{
	return vec.interpolateTo(
		Vector(lua_tonumber (L, argOffs  ),  // x value
		       lua_tonumber (L, argOffs+1)), // y value
		lua_tonumber (L, argOffs+2),         // time
		lua_tointeger(L, argOffs+3),         // loopType
		getBool(L, argOffs+4),               // pingPong
		getBool(L, argOffs+5));              // ease
}

inline float interpolateVec3(lua_State *L, InterpolatedVector& vec, int argOffs)
{
	return vec.interpolateTo(
		Vector(lua_tonumber (L, argOffs  ),  // x value
		       lua_tonumber (L, argOffs+1),  // y value
		       lua_tonumber (L, argOffs+2)), // z value
		lua_tonumber (L, argOffs+3),         // time
		lua_tointeger(L, argOffs+4),         // loopType
		getBool(L, argOffs+5),               // pingPong
		getBool(L, argOffs+6));              // ease
}

static void safePath(lua_State *L, const std::string& path)
{
	// invalid characters for file/path names.
	// Filter out \ as well, it'd work on on win32 only, so it's not supposed to be used.
	size_t invchr = path.find_first_of("\\\":*?<>|");
	if(invchr != std::string::npos)
	{
		lua_pushfstring(L, "Invalid char in path/file name: %c", path[invchr]);
		lua_error(L);
	}
	// Block attempts to access files outside of the "safe area"
	if(path.length())
	{
		if(path[0] == '/')
		{
			if(!(dsq->mod.isActive() && path.substr(0, dsq->mod.getBaseModPath().length()) == dsq->mod.getBaseModPath()))
			{
				lua_pushliteral(L, "Absolute paths are not allowed");
				lua_error(L);
			}
		}
		if(path.find("../") != std::string::npos)
		{
			lua_pushliteral(L, "Accessing parent is not allowed");
			lua_error(L);
		}
	}
}


//----------------------------------//

#define luaFunc(func)		static int l_##func(lua_State *L)
#define luaReturnBool(b)	do {lua_pushboolean(L, (b)); return 1;} while(0)
#define luaReturnInt(num)	do {lua_pushinteger(L, (num)); return 1;} while(0)
#define luaReturnNum(num)	do {lua_pushnumber(L, (num)); return 1;} while(0)
#define luaReturnPtr(ptr)	do {luaPushPointer(L, (ptr)); return 1;} while(0)
#define luaReturnStr(str)	do {lua_pushstring(L, (str)); return 1;} while(0)
#define luaReturnVec2(x,y)	do {lua_pushnumber(L, (x)); lua_pushnumber(L, (y)); return 2;} while(0)
#define luaReturnVec3(x,y,z)	do {lua_pushnumber(L, (x)); lua_pushnumber(L, (y)); lua_pushnumber(L, (z)); return 3;} while(0)
#define luaReturnVec4(x,y,z,w)	do {lua_pushnumber(L, (x)); lua_pushnumber(L, (y)); lua_pushnumber(L, (z)); lua_pushnumber(L, (w)); return 4;} while(0)
#define luaReturnNil()		return 0;

static void pushLocalVarTab(lua_State *L, lua_State *Lv)
{
	lua_getglobal(L, "_threadvars");
	// [_thv]
	lua_pushlightuserdata(L, Lv);
	// [_thv][L]
	lua_gettable(L, -2);
	// [_thv][v]
	lua_remove(L, -2);
	// [v]
}

// Set the global "v" to the instance's local variable table.  Must be
// called when starting a script.
static void fixupLocalVars(lua_State *L)
{
	pushLocalVarTab(L, L);
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
	std::string err = luaFormatStackInfo(L) + ": Lua PANIC: " + getString(L, -1);
	exit_error(err);
	return 0;
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
		fname = adjustFilenameCase(fname);
		if (exists(fname))
			return true;
	}
	fname = localisePath(rawname);
	fname = adjustFilenameCase(fname);
	return exists(fname);
}

static int loadFile_helper(lua_State *L, const char *fn)
{
#ifdef BBGE_BUILD_VFS
	size_t size = 0;
	const char *data = readFile(fn, &size);
	if (!data)
	{
		lua_pushfstring(L, "cannot open %s", fn);
		return LUA_ERRFILE;
	}
	else
	{
		int result = luaL_loadbuffer(L, data, size, fn);
		delete [] data;
		return result;
	}
#else
	return luaL_loadfile(L, fn);
#endif
}

/// dofile(string file) -> Any...
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

/// loadfile(string file) -> function?, string?
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

#if SDL_VERSION_ATLEAST(2,0,0)
#define LUAAPI_HAS_CLIPBOARD
luaFunc(os_setclipboard)
{
	const char *s = getCString(L);
	SDL_SetClipboardText(s ? s : "");
	luaReturnNil();
}

luaFunc(os_getclipboard)
{
	char *s = SDL_GetClipboardText();
	lua_pushstring(L, s ? s : "");
	if(s)
		SDL_free(s);
	return 1;
}
#endif

/// fileExists(string path) -> bool!, string
/// Returns whether the file exists and the resolved path (mod dir first, then game dir). Errors on unsafe paths.
luaFunc(fileExists)
{
	const std::string s = getString(L);
	safePath(L, s);
	std::string res;
	bool there = findFile_helper(s.c_str(), res);
	lua_pushboolean(L, there);
	lua_pushstring(L, res.c_str());
	return 2;
}

/// getModName() -> string!
/// "" when no mod is active.
luaFunc(getModName)
{
	luaReturnStr(dsq->mod.isActive() ? dsq->mod.getName().c_str() : "");
}

/// getModPath() -> string!
/// With trailing '/'; "" when no mod is active.
luaFunc(getModPath)
{
	std::string path;
	if (dsq->mod.isActive())
	{
		path = dsq->mod.getPath();
		if(path[path.length() - 1] != '/')
			path += '/';
	}
	luaReturnStr(path.c_str());
}

/// getInterfaceFunctionNames() -> [string]!
luaFunc(getInterfaceFunctionNames)
{
	lua_newtable(L);
	for(unsigned i = 0; interfaceFunctions[i]; ++i)
	{
		lua_pushstring(L, interfaceFunctions[i]);
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}



// ----- RenderObject common functions -----

#define forwardCall(func) l_##func(L)

#define MakeTypeCheckFunc(fname, ty) luaFunc(fname) \
	{ ScriptObject *r = getScriptObjectSlow(L); luaReturnBool(r && r->isType(ty)); }

/// isNode(Any x) -> bool!
/// isObject(Any x) -> bool!
/// isEntity(Any x) -> bool!
/// isScriptedEntity(Any x) -> bool!
/// isQuad(Any x) -> bool!
/// isBone(Any x) -> bool!
/// isShot(Any x) -> bool!
/// isWeb(Any x) -> bool!
/// isIng(Any x) -> bool!
/// isBeam(Any x) -> bool!
/// isText(Any x) -> bool!
/// isShader(Any x) -> bool!
/// isParticleEffect(Any x) -> bool!
/// isQuadGrid(Any x) -> bool!
MakeTypeCheckFunc(isNode, SCO_PATH)
MakeTypeCheckFunc(isObject, SCO_RENDEROBJECT)
MakeTypeCheckFunc(isEntity, SCO_ENTITY)
MakeTypeCheckFunc(isScriptedEntity, SCO_SCRIPTED_ENTITY)
MakeTypeCheckFunc(isBone, SCO_BONE)
MakeTypeCheckFunc(isShot, SCO_SHOT)
MakeTypeCheckFunc(isWeb, SCO_WEB)
MakeTypeCheckFunc(isIng, SCO_INGREDIENT)
MakeTypeCheckFunc(isBeam, SCO_BEAM)
MakeTypeCheckFunc(isText, SCO_TEXT)
//MakeTypeCheckFunc(isShader, SCO_SHADER)
MakeTypeCheckFunc(isParticleEffect, SCO_PARTICLE_EFFECT)
MakeTypeCheckFunc(isQuadGrid, SCO_QUAD_GRID)

#undef MakeTypeCheckFunc

// special, because it would return true on almost everything that is RenderObject based.
// Instead, return true only for quads created with createQuad()
luaFunc(isQuad)
{
	ScriptObject *r = getScriptObjectSlow(L);

	luaReturnBool(r && r->isExactType(ScriptObjectType(SCO_RENDEROBJECT | SCO_QUAD)));
}

luaFunc(isShader)
{
	luaReturnBool(!!getShaderPtr(L));
}


/// obj_setPosition(RenderObject o, float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_setPosition)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->position.stop();
		interpolateVec2(L, r->position, 2);
	}
	luaReturnNil();
}

/// obj_scale(RenderObject o, float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_scale)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->scale.stop();
		interpolateVec2(L, r->scale, 2);
	}
	luaReturnNil();
}

/// obj_getScale(RenderObject o) -> float!, float
luaFunc(obj_getScale)
{
	RenderObject *r = robj(L);
	Vector s;
	if (r)
		s = r->scale;
	luaReturnVec2(s.x, s.y);
}

/// obj_alpha(RenderObject o, float alpha, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_alpha)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->alpha.stop();
		interpolateVec1(L, r->alpha, 2);
	}
	luaReturnNil();
}

/// obj_alphaMod(RenderObject o, float mod)
luaFunc(obj_alphaMod)
{
	RenderObject *r = robj(L);
	if (r)
		r->alphaMod = lua_tonumber(L, 2);
	luaReturnNil()
}

/// obj_getAlphaMod(RenderObject o) -> float!
luaFunc(obj_getAlphaMod)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->alphaMod : 0.0f);
}

/// obj_getAlpha(RenderObject o) -> float!
luaFunc(obj_getAlpha)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->alpha.x : 0.0f);
}

/// obj_color(RenderObject o, float r, float g, float b, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_color)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->color.stop();
		interpolateVec3(L, r->color, 2);
	}
	luaReturnNil();
}

/// obj_getColor(RenderObject o) -> float!, float, float
luaFunc(obj_getColor)
{
	RenderObject *r = robj(L);
	Vector c;
	if(r)
		c = r->color;
	luaReturnVec3(c.x, c.y, c.z);
}


/// obj_rotate(RenderObject o, float angle, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_rotate)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->rotation.stop();
		interpolateVec1z(L, r->rotation, 2);
	}
	luaReturnNil();
}

/// obj_rotateOffset(RenderObject o, float angle, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_rotateOffset)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->rotationOffset.stop();
		interpolateVec1z(L, r->rotationOffset, 2);
	}
	luaReturnNil();
}

/// obj_getRotation(RenderObject o) -> float!
luaFunc(obj_getRotation)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->rotation.z : 0.0f);
}

/// obj_getRotationOffset(RenderObject o) -> float!
luaFunc(obj_getRotationOffset)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->rotationOffset.z : 0.0f);
}

/// obj_offset(RenderObject o, float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_offset)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->offset.stop();
		interpolateVec2(L, r->offset, 2);
	}
	luaReturnNil();
}

/// obj_internalOffset(RenderObject o, float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_internalOffset)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->internalOffset.stop();
		interpolateVec2(L, r->internalOffset, 2);
	}
	luaReturnNil();
}

/// obj_getInternalOffset(RenderObject o) -> float!, float
luaFunc(obj_getInternalOffset)
{
	RenderObject *r = robj(L);
	Vector io;
	if (r)
		io = r->internalOffset;
	luaReturnVec2(io.x, io.y);
}

/// obj_getPosition(RenderObject o) -> float!, float
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


/// obj_getOffset(RenderObject o) -> float!, float
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
/// obj_x(RenderObject o) -> float!
luaFunc(obj_x)
{
	RenderObject *r = robj(L);
	float x = 0;
	if (r)
		x = r->position.x;
	luaReturnNum(x);
}

/// obj_y(RenderObject o) -> float!
luaFunc(obj_y)
{
	RenderObject *r = robj(L);
	float y = 0;
	if (r)
		y = r->position.y;
	luaReturnNum(y);
}

/// obj_setBlendType(RenderObject o, BlendType blend)
luaFunc(obj_setBlendType)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->setBlendType(getBlendType(L, 2));
	}
	luaReturnNil();
}

/// obj_getBlendType(RenderObject o) -> BlendType!
luaFunc(obj_getBlendType)
{
	RenderObject *r = robj(L);
	luaReturnInt(r ? r->getBlendType() : 0);
}

/// obj_setTexture(RenderObject o, string tex) -> bool
/// Returns whether the texture was found.
luaFunc(obj_setTexture)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->setTexture(getString(L, 2)) : false);
}

/// obj_getTexture(RenderObject o) -> string!
/// "" when there's no texture.
luaFunc(obj_getTexture)
{
	RenderObject *r = robj(L);
	if (r && r->texture)
		luaReturnStr(r->texture->name.c_str());
	luaReturnStr("");
}

/// obj_delete(RenderObject o, float time = 0)
/// time > 0 fades out over that many seconds; 0 kills immediately.
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

/// obj_setLife(RenderObject o, float life)
luaFunc(obj_setLife)
{
	RenderObject *r = robj(L);
	if (r)
		r->setLife(lua_tonumber(L, 2));
	luaReturnNil();
}

/// obj_getLife(RenderObject o) -> float!
luaFunc(obj_getLife)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->life : 0.0f);
}

/// obj_setDecayRate(RenderObject o, float rate)
luaFunc(obj_setDecayRate)
{
	RenderObject *r = robj(L);
	if (r)
		r->setDecayRate(lua_tonumber(L, 2));
	luaReturnNil();
}

/// obj_addDeathNotify(RenderObject o, RenderObject notify); distinct(o, notify)
luaFunc(obj_addDeathNotify)
{
	RenderObject *r = robj(L);
	RenderObject *which = robj(L, 2);
	if (r && which)
		r->addDeathNotify(which);
	luaReturnNil();
}

/// obj_addChild(RenderObject o, RenderObject child, boolx takeOwnership = false, boolx front = false); distinct(o, child)
luaFunc(obj_addChild)
{
	RenderObject *r = robj(L);
	RenderObject *which = robj(L, 2);
	bool takeOwnership = getBool(L, 3);
	bool front = getBool(L, 4);
	if (r && which)
	{
		if (takeOwnership)
		{
			// HACK: this is ugly, but necessary to prevent double-deletion
			// anyways, dangerous; addChild() may fail, causing a small memory leak (and an error message)
			core->removeRenderObject(which, Core::DO_NOT_DESTROY_RENDER_OBJECT);
			r->addChild(which, PM_POINTER, RBP_NONE, front ? CHILD_FRONT : CHILD_BACK);
		}
		else
			r->addChild(which, PM_STATIC, RBP_NONE, front ? CHILD_FRONT : CHILD_BACK);
	}
	luaReturnNil();
}

/// obj_getChild(RenderObject o, int idx) -> RenderObject|NoObj!
/// idx is 0-based.
luaFunc(obj_getChild)
{
	RenderObject *r = robj(L);
	size_t idx = lua_tointeger(L, 2);
	luaReturnPtr(r && idx < r->children.size() ? r->children[idx] : NULL);
}

/// obj_removeChild(RenderObject o, RenderObject child); distinct(o, child)
luaFunc(obj_removeChild)
{
	RenderObject *r = robj(L);
	RenderObject *which = robj(L, 2);
	if(r && which)
		r->removeChild(which);
	luaReturnNil();
}

/// obj_removeChildIdx(RenderObject o, int idx)
/// idx is 0-based.
luaFunc(obj_removeChildIdx)
{
	RenderObject *r = robj(L);
	size_t idx = lua_tointeger(L, 2);
	if(r && idx < r->children.size())
		r->removeChild(r->children[idx]);
	luaReturnNil();
}

/// obj_removeAllChildren(RenderObject o, boolx kill = false)
luaFunc(obj_removeAllChildren)
{
	RenderObject *r = robj(L);
	bool del = getBool(L, 2);
	if(r)
	{
		if(del)
			for(RenderObject::Children::iterator it = r->children.begin(); it != r->children.end(); ++it)
				(*it)->safeKill();
		else
		{
			// When not deleting, reparent the child objects to the current game state as toplevel objects
			// FIXME: Untested whether this works as intended
			for(RenderObject::Children::iterator it = r->children.begin(); it != r->children.end(); ++it)
			{
				RenderObject *ch = *it;
				Vector world = ch->getWorldPositionAndRotation();
				ch->position.x = world.x;
				ch->position.y = world.y;
				ch->rotation.z = world.z;
				ch->clearParent();
				core->getTopStateObject()->addRenderObject(ch, r->getTopLayer()); // Use layer of parent
			}
		}

		r->children.clear();
	}
	luaReturnNil();
}

/// obj_getNumChildren(RenderObject o) -> int!
luaFunc(obj_getNumChildren)
{
	RenderObject *r = robj(L);
	luaReturnInt(r ? (int)r->children.size() : 0);
}

/// obj_setRenderBeforeParent(RenderObject o, boolx on)
luaFunc(obj_setRenderBeforeParent)
{
	RenderObject *r = robj(L);
	if (r)
		r->renderBeforeParent = getBool(L, 2);
	luaReturnNil();
}

/// obj_isRenderBeforeParent(RenderObject o) -> bool!
luaFunc(obj_isRenderBeforeParent)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->renderBeforeParent : false);
}

// Not so pretty: Because RenderObject has a `velocity' vector,
// and Entity has `vel' ADDITIONALLY, we need to use
// extra functions to manage RenderObject's velocities.
// Different names were chosen to allow avoid name clashing. -- FG

/// obj_setInternalVel(RenderObject o, float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_setInternalVel)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->velocity.stop();
		interpolateVec2(L, r->velocity, 2);
	}
	luaReturnNil();
}

/// obj_setInternalVelLen(RenderObject o, float len)
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

/// obj_getInternalVelLen(RenderObject o) -> float!
luaFunc(obj_getInternalVelLen)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->velocity.getLength2D() : 0.0f);
}


/// obj_getInternalVel(RenderObject o) -> float!, float
luaFunc(obj_getInternalVel)
{
	Vector v;
	RenderObject *r = robj(L);
	if (r)
		v = r->velocity;
	luaReturnVec2(v.x, v.y);
}

/// obj_ivelx(RenderObject o) -> float!
luaFunc(obj_ivelx)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->velocity.x : 0.0f);
}

/// obj_ively(RenderObject o) -> float!
luaFunc(obj_ively)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->velocity.y : 0.0f);
}

/// obj_addInternalVel(RenderObject o, float x, float y)
luaFunc(obj_addInternalVel)
{
	RenderObject *r = robj(L);
	if (r)
		r->velocity += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

/// obj_isInternalVelIn(RenderObject o, float len) -> bool!
luaFunc(obj_isInternalVelIn)
{
	RenderObject *r = robj(L);
	luaReturnBool(r && r->velocity.isLength2DIn(lua_tonumber(L, 2)));
}

/// obj_setGravity(RenderObject o, float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(obj_setGravity)
{
	RenderObject *r = robj(L);
	if (r)
	{
		r->gravity.stop();
		interpolateVec2(L, r->gravity, 2);
	}
	luaReturnNil();
}

/// obj_getGravity(RenderObject o) -> float!, float
luaFunc(obj_getGravity)
{
	Vector v;
	RenderObject *r = robj(L);
	if (r)
		v = r->gravity;
	luaReturnVec2(v.x, v.y);
}

/// obj_getNormal(RenderObject o) -> float!, float
/// World-space forward direction (normalized); (0, 1) for an invalid object.
luaFunc(obj_getNormal)
{
	Vector v(0, 1);
	RenderObject *r = robj(L);
	if (r)
		v = r->getForward();
	luaReturnVec2(v.x, v.y);
}

/// obj_getVectorToObj(RenderObject o, RenderObject other) -> float!, float
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

// LUAWTF-NOTE(weird): obj_stopInterpolating / obj_isInterpolating only look at position, not alpha/scale/rotation/color as the generic names suggest
/// obj_stopInterpolating(RenderObject o)
luaFunc(obj_stopInterpolating)
{
	RenderObject *r = robj(L);
	if (r)
		r->position.stop();
	luaReturnNil();
}

/// obj_isInterpolating(RenderObject o) -> bool!
/// Position only.
luaFunc(obj_isInterpolating)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->position.isInterpolating() : false);
}

/// obj_followCamera(RenderObject o, float f)
/// 0 = world space, 1 = screen space (moves with the camera).
luaFunc(obj_followCamera)
{
	RenderObject *r = robj(L);
	if (r)
		r->followCamera = lua_tonumber(L, 2);
	luaReturnNil();
}

/// obj_update(RenderObject o, float dt)
luaFunc(obj_update)
{
	RenderObject *r = robj(L);
	if (r)
		r->update(lua_tonumber(L, 2));
	luaReturnNil();
}

/// obj_getWorldPosition(RenderObject o, float x = 0, float y = 0) -> float!, float
/// World position of the local offset (x, y).
luaFunc(obj_getWorldPosition)
{
	RenderObject *b = robj(L);
	float x = 0, y = 0;
	if (b)
	{
		Vector v = b->getWorldCollidePosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
		x = v.x;
		y = v.y;
	}
	luaReturnVec2(x, y);
}

/// obj_getWorldRotation(RenderObject o) -> float!
luaFunc(obj_getWorldRotation)
{
	RenderObject *r = robj(L);
	luaReturnNum(r ? r->getWorldRotation() : 0.0f);
}

/// obj_getWorldPositionAndRotation(RenderObject o) -> float!, float, float
luaFunc(obj_getWorldPositionAndRotation)
{
	RenderObject *r = robj(L);
	if (r)
	{
		Vector v = r->getWorldPositionAndRotation();
		luaReturnVec3(v.x, v.y, v.z);
	}
	luaReturnVec3(0.0f, 0.0f, 0.0f);
}

/// obj_moveToFront(RenderObject o)
luaFunc(obj_moveToFront)
{
	RenderObject *r = robj(L);
	if (r)
		r->moveToFront();
	luaReturnNil();
}

/// obj_moveToBack(RenderObject o)
luaFunc(obj_moveToBack)
{
	RenderObject *r = robj(L);
	if (r)
		r->moveToBack();
	luaReturnNil();
}

// LUAWTF-NOTE(bug): no range check on the layer: Core::switchRenderObjectLayer only asserts `toLayer < renderObjectLayers.size()` and `o->layer != LR_NONE` (child objects), so in release a bad layer or a child object indexes out of bounds
/// obj_setLayer(RenderObject o, Layer layer)
luaFunc(obj_setLayer)
{
	RenderObject *r = robj(L);
	if (r)
		core->switchRenderObjectLayer(r, lua_tointeger(L, 2));
	luaReturnNil();
}

/// obj_getLayer(RenderObject o) -> Layer!
luaFunc(obj_getLayer)
{
	RenderObject *r = robj(L);
	luaReturnInt(r ? r->layer : 0);
}

/// obj_setRenderPass(RenderObject o, int pass)
luaFunc(obj_setRenderPass)
{
	RenderObject *r = robj(L);
	int pass = lua_tointeger(L, 2);
	if (r)
		r->setRenderPass(pass);
	luaReturnNil();
}

/// obj_getRenderPass(RenderObject o) -> int!
luaFunc(obj_getRenderPass)
{
	RenderObject *r = robj(L);
	int pass = 0;
	if (r)
		pass = r->getRenderPass();
	luaReturnInt(pass);
}

/// obj_fh(RenderObject o)
/// obj_flipHorizontal(RenderObject o)
luaFunc(obj_fh)
{
	RenderObject *r = robj(L);
	if (r)
		r->flipHorizontal();
	luaReturnNil();
}

/// obj_fhTo(RenderObject o, boolx fh)
luaFunc(obj_fhTo)
{
	RenderObject *r = robj(L);
	bool b = getBool(L, 2);
	if (r)
		r->fhTo(b);
	luaReturnNil();
}

/// obj_fv(RenderObject o)
/// obj_flipVertical(RenderObject o)
luaFunc(obj_fv)
{
	RenderObject *r = robj(L);
	if (r)
		r->flipVertical();
	luaReturnNil();
}

/// obj_isfh(RenderObject o) -> bool!
luaFunc(obj_isfh)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isfh() : false);
}

/// obj_isfv(RenderObject o) -> bool!
luaFunc(obj_isfv)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isfv() : false);
}

/// obj_isfhr(RenderObject o) -> bool!
luaFunc(obj_isfhr)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isfhr() : false);
}

/// obj_isfvr(RenderObject o) -> bool!
luaFunc(obj_isfvr)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->isfvr() : false);
}

/// obj_damageFlash(RenderObject o, int type = 0)
/// type 1 flashes yellow, anything else red.
luaFunc(obj_damageFlash)
{
	RenderObject *r = robj(L);
	int type = lua_tointeger(L, 2);
	if (r)
	{
		Vector toColor = Vector(1, 0.1f, 0.1f);
		if (type == 1)
			toColor = Vector(1, 1, 0.1f);
		r->color = Vector(1,1,1);
		r->color.interpolateTo(toColor, 0.1f, 5, 1);
	}
	luaReturnNil();
}

/// obj_setCull(RenderObject o, boolx cull)
luaFunc(obj_setCull)
{
	RenderObject *r = robj(L);
	if (r)
		r->cull = getBool(L, 2);
	luaReturnNil();
}

/// obj_setCullRadius(RenderObject o, float radius)
luaFunc(obj_setCullRadius)
{
	RenderObject *r = robj(L);
	if (r)
		r->setOverrideCullRadius(lua_tonumber(L, 2));
	luaReturnNil();
}

/// obj_isScaling(RenderObject o) -> bool!
luaFunc(obj_isScaling)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->scale.isInterpolating() : false);
}

/// obj_isRotating(RenderObject o) -> bool!
luaFunc(obj_isRotating)
{
	RenderObject *r = robj(L);
	luaReturnBool(r ? r->rotation.isInterpolating() : false);
}

/// obj_setUpdateCull(RenderObject o, float dist)
/// -1 = always update.
luaFunc(obj_setUpdateCull)
{
	RenderObject *r = robj(L);;
	if (r)
		r->updateCull = lua_tonumber(L, 2);
	luaReturnNil();
}

/// obj_getUpdateCull(RenderObject o) -> float!
luaFunc(obj_getUpdateCull)
{
	RenderObject *r = robj(L);;
	luaReturnNum(r ? r->updateCull : 0.0f);
}

/// obj_setPositionX(RenderObject o, float x)
luaFunc(obj_setPositionX)
{
	RenderObject *r = robj(L);
	if (r)
		r->position.x = lua_tonumber(L, 2);
	luaReturnNil();
}

/// obj_setPositionY(RenderObject o, float y)
luaFunc(obj_setPositionY)
{
	RenderObject *r = robj(L);
	if (r)
		r->position.y = lua_tonumber(L, 2);
	luaReturnNil();
}

// LUAWTF-NOTE(api): no parameters; always enableMotionBlur(10, 2), unlike the C++ method which takes the sizes
/// obj_enableMotionBlur(RenderObject o)
luaFunc(obj_enableMotionBlur)
{
	RenderObject *r = robj(L);
	if (r)
		r->enableMotionBlur(10, 2);
	luaReturnNil();
}

/// obj_disableMotionBlur(RenderObject o)
luaFunc(obj_disableMotionBlur)
{
	RenderObject *r = robj(L);
	if (r)
		r->disableMotionBlur();
	luaReturnNil();
}

/// obj_fadeAlphaWithLife(RenderObject o, boolx on)
luaFunc(obj_fadeAlphaWithLife)
{
	RenderObject *r = robj(L);
	if (r)
		r->fadeAlphaWithLife = getBool(L, 2);
	luaReturnNil();
}

/// obj_getWorldScale(RenderObject o) -> float!, float
luaFunc(obj_getWorldScale)
{
	RenderObject *r = robj(L);
	Vector s;
	if (r)
		s = r->getRealScale();
	luaReturnVec2(s.x, s.y);
}

/// obj_getParent(RenderObject o) -> RenderObject|NoObj!
luaFunc(obj_getParent)
{
	RenderObject *r = robj(L);
	luaReturnPtr(r ? r->getParent() : NULL);
}


// ----- end RenderObject common functions -----

// ----- Quad common functions -----

/// quad_isVisible(Quad q) -> bool!
luaFunc(quad_isVisible)
{
	Quad *q = getQuad(L);
	luaReturnBool(q ? q->renderQuad : false);
}

/// quad_setVisible(Quad q, boolx visible)
luaFunc(quad_setVisible)
{
	Quad *q = getQuad(L);
	if (q)
		q->renderQuad = getBool(L, 2);
	luaReturnNil();
}

/// quad_getWidth(Quad q) -> float!
luaFunc(quad_getWidth)
{
	Quad *q = getQuad(L);
	luaReturnNum(q ? q->width : 0.0f);
}

/// quad_getHeight(Quad q) -> float!
luaFunc(quad_getHeight)
{
	Quad *q = getQuad(L);
	luaReturnNum(q ? q->height : 0.0f);
}

/// quad_setWidth(Quad q, float w)
luaFunc(quad_setWidth)
{
	Quad *q = getQuad(L);
	if (q)
		q->setWidth(lua_tonumber(L, 2));
	luaReturnNil();
}

/// quad_setHeight(Quad q, float h)
luaFunc(quad_setHeight)
{
	Quad *q = getQuad(L);
	if (q)
		q->setHeight(lua_tonumber(L, 2));
	luaReturnNil();
}

/// quad_setSegs(Quad q, int x, int y, float offX, float offY, float multX, float multY, float timeMult, boolx out)
/// x or y = 0 removes the grid.
luaFunc(quad_setSegs)
{
	Quad *b = getQuad(L);
	if (b)
		b->setSegs(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), getBool(L, 9));
	luaReturnNil();
}

/// quad_setRepeatTexture(Quad q, boolx on)
luaFunc(quad_setRepeatTexture)
{
	Quad *b = getQuad(L);
	if (b)
		b->repeatTextureToFill(getBool(L, 2));
	luaReturnNil();
}

/// quad_setRepeatScale(Quad q, float x, float y)
luaFunc(quad_setRepeatScale)
{
	Quad *b = getQuad(L);
	if (b)
		b->setRepeatScale(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	luaReturnNil();
}

/// quad_getRepeatScale(Quad q) -> float!, float
luaFunc(quad_getRepeatScale)
{
	Vector s;
	Quad *b = getQuad(L);
	if (b)
		s = b->getRepeatScale();
	luaReturnVec2(s.x, s.y);
}

/// quad_isRepeatTexture(Quad q) -> bool!
luaFunc(quad_isRepeatTexture)
{
	Quad *b = getQuad(L);
	luaReturnBool(b ? b->isRepeatingTextureToFill() : false);
}

/// quad_setTexOffset(Quad q, float x, float y)
luaFunc(quad_setTexOffset)
{
	Quad *b = getQuad(L);
	if (b)
		b->setRepeatOffset(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	luaReturnNil();
}

/// quad_getTexOffset(Quad q) -> float!, float
luaFunc(quad_getTexOffset)
{
	Quad *b = getQuad(L);
	Vector v;
	if (b)
		v = b->getRepeatOffset();
	luaReturnVec2(v.x, v.y);
}

/// quad_setRenderBorder(Quad q, boolx on)
luaFunc(quad_setRenderBorder)
{
	Quad *b = getQuad(L);
	if (b)
		b->renderBorder = getBool(L, 2);
	luaReturnNil();
}

/// quad_isRenderBorder(Quad q) -> bool!
luaFunc(quad_isRenderBorder)
{
	Quad *b = getQuad(L);
	luaReturnBool(b ? b->renderBorder : false);
}

/// quad_setRenderCenter(Quad q, boolx on)
luaFunc(quad_setRenderCenter)
{
	Quad *b = getQuad(L);
	if (b)
		b->renderCenter = getBool(L, 2);
	luaReturnNil();
}

/// quad_isRenderCenter(Quad q) -> bool!
luaFunc(quad_isRenderCenter)
{
	Quad *b = getQuad(L);
	luaReturnBool(b ? b->renderCenter : false);
}


/// quad_borderAlpha(Quad q, float a)
luaFunc(quad_borderAlpha)
{
	Quad *b = getQuad(L);
	if (b)
		b->borderAlpha = lua_tonumber(L, 2);
	luaReturnNil();
}

/// quad_getBorderAlpha(Quad q) -> float!
luaFunc(quad_getBorderAlpha)
{
	Quad *b = getQuad(L);
	luaReturnNum(b ? b->borderAlpha : 0.0f);
}

/// quad_borderColor(Quad q, float r, float g, float b)
luaFunc(quad_borderColor)
{
	Quad *b = getQuad(L);
	if (b)
		b->renderBorderColor = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	luaReturnNil();
}

/// quad_getBorderColor(Quad q) -> float!, float, float
luaFunc(quad_getBorderColor)
{
	Quad *b = getQuad(L);
	Vector c;
	if(b)
		c = b->renderBorderColor;
	luaReturnVec3(c.x, c.y, c.z);
}

// --- CollideQuad only --- These are intentionally quad_*() but don't work with regular quads! ---
// (These are intended to be used with derived types: entity_*, bone_*, shot_*)

/// quad_getCollideRadius(CollideQuad o) -> float!
luaFunc(quad_getCollideRadius)
{
	CollideQuad *r = getCollideQuad(L);
	luaReturnNum(r ? r->collideRadius : 0);
}

/// quad_setCollideRadius(CollideQuad o, float r)
luaFunc(quad_setCollideRadius)
{
	CollideQuad *r = getCollideQuad(L);
	if (r)
		r->collideRadius = lua_tonumber(L, 2);
	luaReturnNil();
}

/// quad_collideCircleVsLine(CollideQuad o, float x1, float y1, float x2, float y2, float size) -> bool!
luaFunc(quad_collideCircleVsLine)
{
	CollideQuad *r = getCollideQuad(L);
	float x1, y1, x2, y2, sz;
	x1 = lua_tonumber(L, 2);
	y1 = lua_tonumber(L, 3);
	x2 = lua_tonumber(L, 4);
	y2 = lua_tonumber(L, 5);
	sz = lua_tonumber(L, 6);
	bool v = false;
	if (r)
		v = game->collideCircleVsLine(r, Vector(x1, y1), Vector(x2, y2), sz);
	luaReturnBool(v);
}

// LUAWTF-NOTE(bug): same as obj_collideCircleVsLine: getCollideQuad() on slot 1, but generated for quad_/web_/text_/pe_ too, where it always fails
/// quad_collideCircleVsLineAngle(CollideQuad o, float angle, float start, float end, float radius, float x, float y) -> bool!
luaFunc(quad_collideCircleVsLineAngle)
{
	CollideQuad *r = getCollideQuad(L);
	float angle = lua_tonumber(L, 2);
	float start=lua_tonumber(L, 3), end=lua_tonumber(L, 4), radius=lua_tonumber(L, 5);
	float x=lua_tonumber(L, 6);
	float y=lua_tonumber(L, 7);
	bool v = false;
	if (r)
		v = game->collideCircleVsLineAngle(r, angle, start, end, radius, Vector(x,y));
	luaReturnBool(v);
}


// --- standard set/get functions for each type, wrapping RenderObject functions ---

#define MK_FUNC(base, getter, prefix, suffix)	\
	luaFunc(prefix##_##suffix) \
	{									\
		typecheckOnly(getter(L));		\
		return forwardCall(base##_##suffix);		\
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
	RO_FUNC(getter, prefix,  getAlphaMod	) \
	RO_FUNC(getter, prefix,  color			) \
	RO_FUNC(getter, prefix,  getColor		) \
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
	RO_FUNC(getter, prefix,  getBlendType	) \
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
	RO_FUNC(getter, prefix,  setGravity		) \
	RO_FUNC(getter, prefix,  getGravity		) \
	RO_FUNC(getter, prefix,  getNormal		) \
	RO_FUNC(getter, prefix,  stopInterpolating)\
	RO_FUNC(getter, prefix,  isInterpolating)\
	RO_FUNC(getter, prefix,  followCamera	) \
	RO_FUNC(getter, prefix,  update			) \
	RO_FUNC(getter, prefix,  getWorldPosition) \
	RO_FUNC(getter, prefix,  getWorldRotation) \
	RO_FUNC(getter, prefix,  getWorldPositionAndRotation)\
	RO_FUNC(getter, prefix,  moveToFront	) \
	RO_FUNC(getter, prefix,  moveToBack		) \
	RO_FUNC(getter, prefix,  setLayer		) \
	RO_FUNC(getter, prefix,  getLayer		) \
	RO_FUNC(getter, prefix,  setRenderBeforeParent) \
	RO_FUNC(getter, prefix,  isRenderBeforeParent) \
	RO_FUNC(getter, prefix,  addChild		) \
	RO_FUNC(getter, prefix,  getChild		) \
	RO_FUNC(getter, prefix,  removeChild	) \
	RO_FUNC(getter, prefix,  removeChildIdx	) \
	RO_FUNC(getter, prefix,  removeAllChildren) \
	RO_FUNC(getter, prefix,  getNumChildren	) \
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
	RO_FUNC(getter, prefix,  getUpdateCull	) \
	RO_FUNC(getter, prefix,  setRenderPass	) \
	RO_FUNC(getter, prefix,  getRenderPass	) \
	RO_FUNC(getter, prefix,  setPositionX	) \
	RO_FUNC(getter, prefix,  setPositionY	) \
	RO_FUNC(getter, prefix,  enableMotionBlur	) \
	RO_FUNC(getter, prefix,  disableMotionBlur	) \
	RO_FUNC(getter, prefix,  getVectorToObj	) \
	RO_FUNC(getter, prefix,  fadeAlphaWithLife	) \
	RO_FUNC(getter, prefix,  getWorldScale	) \
	RO_FUNC(getter, prefix,  getParent		) \
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
	Q_FUNC(getter, prefix,  setSegs			) \
	Q_FUNC(getter, prefix,  setRepeatTexture) \
	Q_FUNC(getter, prefix,  isRepeatTexture	) \
	Q_FUNC(getter, prefix,  setRepeatScale	) \
	Q_FUNC(getter, prefix,  getRepeatScale	) \
	Q_FUNC(getter, prefix,  setRenderBorder	) \
	Q_FUNC(getter, prefix,  isRenderBorder	) \
	Q_FUNC(getter, prefix,  setRenderCenter	) \
	Q_FUNC(getter, prefix,  isRenderCenter	) \
	Q_FUNC(getter, prefix,  borderAlpha		) \
	Q_FUNC(getter, prefix,  getBorderAlpha	) \
	Q_FUNC(getter, prefix,  borderColor		) \
	Q_FUNC(getter, prefix,  getBorderColor	) \
	Q_FUNC(getter, prefix,  setTexOffset	) \
	Q_FUNC(getter, prefix,  getTexOffset	) \
	Q_FUNC(getter, prefix,  getCollideRadius) \
	Q_FUNC(getter, prefix,  setCollideRadius) \
	Q_FUNC(getter, prefix,  collideCircleVsLine) \
	Q_FUNC(getter, prefix,  collideCircleVsLineAngle)

// This should reflect the internal class hierarchy,
// e.g. a Beam is a Quad, so it can use quad_* functions
#define EXPAND_FUNC_PROTOTYPES \
	MAKE_QUAD_FUNCS(entity,   entity) \
	MAKE_QUAD_FUNCS(bone,     bone	) \
	MAKE_QUAD_FUNCS(getShot,  shot	) \
	MAKE_QUAD_FUNCS(beam,     beam	) \
	MAKE_ROBJ_FUNCS(getQuad,  quad	) \
	MAKE_ROBJ_FUNCS(getWeb,   web	) \
	MAKE_ROBJ_FUNCS(getText,  text	) \
	MAKE_ROBJ_FUNCS(getParticle, pe	)

// first time, create them. (There is a second use of this further down, with different MK_* macros)
EXPAND_FUNC_PROTOTYPES


/// debugBreak()
luaFunc(debugBreak)
{
	debugLog("DEBUG BREAK");
	triggerBreakpoint();
	luaReturnNil();
}

/// setIgnoreAction(Action action, boolx ignore)
luaFunc(setIgnoreAction)
{
	game->setIgnoreAction((AquariaActions)lua_tointeger(L, 1), getBool(L, 2));
	luaReturnNil();
}

/// isIgnoreAction(Action action) -> bool!
luaFunc(isIgnoreAction)
{
	luaReturnBool(game->isIgnoreAction((AquariaActions)lua_tointeger(L, 1)));
}

/// sendAction(Action action, int state, int mask = 0, int source = 0, InputDevice device = 0)
/// mask: 1 = game, 2 = avatar, 0 = both.
luaFunc(sendAction)
{
	AquariaActions ac = (AquariaActions)lua_tointeger(L, 1);
	int state = lua_tointeger(L, 2);
	int mask = lua_tointeger(L, 3);
	int source = lua_tointeger(L, 4);
	InputDevice device = (InputDevice)lua_tointeger(L, 5);
	if(!mask)
		mask = -1;
	if(mask & 1)
		game->action(ac, state, source, device);
	if((mask & 2) && game->avatar)
		game->avatar->action(ac, state, source, device);
	luaReturnNil();
}

/// randRange(int from, int to) -> int!
/// Inclusive.
luaFunc(randRange)
{
	int n1 = lua_tointeger(L, 1);
	int n2 = lua_tointeger(L, 2);
	int r = randRange(n1, n2);
	luaReturnNum(r);
}

/// upgradeHealth()
luaFunc(upgradeHealth)
{
	dsq->continuity.upgradeHealth();
	luaReturnNil();
}

/// shakeCamera(float mag, float time)
luaFunc(shakeCamera)
{
	dsq->shakeCamera(lua_tonumber(L,1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// rumble(float left, float right, float time, int source = 0, InputDevice device = 0)
/// source: 1-based action set, 0 = all; device 0 = the avatar's last input device.
luaFunc(rumble)
{
	int source = lua_tointeger(L, 4) - 1; // 0 underflows and means all of them
	Avatar *a = game->avatar;
	InputDevice device = (InputDevice)lua_tointeger(L, 5);
	if(device == INPUT_NODEVICE) // default: use avatar status
		device = a ? a->getLastActionInputDevice() : INPUT_NODEVICE;
	dsq->rumble(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), source, device);
	luaReturnNil();
}

/// changeForm(Form form)
luaFunc(changeForm)
{
	if(Avatar *a = game->avatar)
		a->changeForm((FormType)lua_tointeger(L, 1));
	luaReturnNil();
}

/// getWaterLevel() -> float!
luaFunc(getWaterLevel)
{
	luaReturnNum(game->getWaterLevel());
}

/// setPoison(float m, float time)
luaFunc(setPoison)
{
	dsq->continuity.setPoison(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// cureAllStatus()
luaFunc(cureAllStatus)
{
	dsq->continuity.cureAllStatus();
	luaReturnNil();
}

/// setMusicToPlay(string? name)
/// Does nothing if name isn't a string.
luaFunc(setMusicToPlay)
{
	if (lua_isstring(L, 1))
		game->setMusicToPlay(getString(L, 1));
	luaReturnNil();
}

/// setActivePet(Flag pet) -> Entity|NoObj
/// pet: a FLAG_PET_* id, 0 = none; returns the new pet.
luaFunc(setActivePet)
{
	Entity *e = game->setActivePet(lua_tointeger(L, 1));

	luaReturnPtr(e);
}

/// setWaterLevel(float level, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false) -> float
/// Returns the current (not the target) level.
luaFunc(setWaterLevel)
{
	interpolateVec1(L, game->waterLevel, 1);
	luaReturnNum(game->waterLevel.x);
}

/// getForm() -> Form!
luaFunc(getForm)
{
	luaReturnNum(dsq->continuity.form);
}

/// isForm(Form form) -> bool!
luaFunc(isForm)
{
	FormType form = FormType(lua_tointeger(L, 1));
	bool v = (form == dsq->continuity.form);
	luaReturnBool(v);
}

/// learnFormUpgrade(int upgrade)
luaFunc(learnFormUpgrade)
{
	dsq->continuity.learnFormUpgrade((FormUpgradeType)lua_tointeger(L, 1));
	luaReturnNil();
}

/// hasLi() -> bool!
luaFunc(hasLi)
{
	luaReturnBool(dsq->continuity.hasLi());
}

/// hasFormUpgrade(int upgrade) -> bool!
luaFunc(hasFormUpgrade)
{
	luaReturnBool(dsq->continuity.hasFormUpgrade((FormUpgradeType)lua_tointeger(L, 1)));
}

// This used to be castSong(), but that name is already taken by an interface function. -- FG
/// singSong(Song song)
luaFunc(singSong)
{
	dsq->continuity.castSong(lua_tointeger(L, 1));
	luaReturnNil();
}

/// isStory(float story) -> bool!
luaFunc(isStory)
{
	luaReturnBool(dsq->continuity.isStory(lua_tonumber(L, 1)));
}

// LUAWTF-NOTE(weird): `mag` read with lua_tonumber, so leaving it out gives 0 (a null vector), while the C++ default is mag=1
/// getNoteVector(int note, float mag) -> float!, float
luaFunc(getNoteVector)
{
	int note = lua_tointeger(L, 1);
	float mag = lua_tonumber(L, 2);
	Vector v = dsq->getNoteVector(note, mag);
	luaReturnVec2(v.x, v.y);
}

/// getNoteColor(int note) -> float!, float, float
luaFunc(getNoteColor)
{
	int note = lua_tointeger(L, 1);
	Vector v = dsq->getNoteColor(note);
	luaReturnVec3(v.x, v.y, v.z);
}

/// getRandNote() -> int!
luaFunc(getRandNote)
{
	luaReturnNum(dsq->getRandNote());
}

/// getStory() -> float!
luaFunc(getStory)
{
	luaReturnNum(dsq->continuity.getStory());
}

// Note: with none of FLAG_SECRETxx set, num = 0 -> it shows string 799, which is "Secret error".
/// foundLostMemory()
luaFunc(foundLostMemory)
{
	int num = 0;
	if (dsq->continuity.getFlag(FLAG_SECRET01)) num++;
	if (dsq->continuity.getFlag(FLAG_SECRET02)) num++;
	if (dsq->continuity.getFlag(FLAG_SECRET03)) num++;

	int sbank = 800+(num-1);
	game->setControlHint(stringbank.get(sbank), 0, 0, 0, 4, "13/face");

	dsq->sound->playSfx("memory-found");

	luaReturnNil();
}

/// setStory(float story)
luaFunc(setStory)
{
	dsq->continuity.setStory(lua_tonumber(L, 1));
	luaReturnNil();
}

/// confirm(string text, string image = "") -> bool
luaFunc(confirm)
{
	std::string text = getString(L, 1);
	std::string img = getString(L, 2);
	bool b = dsq->confirm(text, img);
	luaReturnBool(b);
}

/// createWeb() -> Web
luaFunc(createWeb)
{
	Web *web = new Web();
	game->addRenderObject(web, LR_PARTICLES);
	luaReturnPtr(web);
}

// spore has base entity
/// createSpore(float x, float y) -> CollideEntity|NoObj
/// NoObj if the position is obstructed.
luaFunc(createSpore)
{
	Vector pos(lua_tonumber(L, 1), lua_tonumber(L, 2));
	if (Spore::isPositionClear(pos))
	{
		Spore *spore = new Spore(pos);
		game->addRenderObject(spore, LR_ENTITIES);
		luaReturnPtr(spore);
	}
	else
		luaReturnPtr(NULL);
}

/// web_addPoint(Web w, float x, float y) -> int?
/// Returns the new point's index.
luaFunc(web_addPoint)
{
	Web *w = getWeb(L);
	if(!w)
		luaReturnNil();

	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	int r = w->addPoint(Vector(x,y));
	luaReturnInt(r);
}

// LUAWTF-NOTE(weird): returns the index argument unchanged, even when the web is invalid or the index is out of range (Web::setPoint silently ignores it)
/// web_setPoint(Web w, int pt, float x, float y) -> int
luaFunc(web_setPoint)
{
	Web *w = getWeb(L);
	int pt = lua_tointeger(L, 2);
	float x = lua_tonumber(L, 3);
	float y = lua_tonumber(L, 4);
	if (w)
	{
		w->setPoint(pt, Vector(x, y));
	}
	luaReturnInt(pt);
}

/// web_getNumPoints(Web web) -> int!
luaFunc(web_getNumPoints)
{
	Web *web = getWeb(L);
	int num = 0;
	if (web)
	{
		num = (int)web->getNumPoints();
	}
	luaReturnInt(num);
}

/// getFirstShot() -> Shot|NoObj
/// Starts the iteration over all shots; continue with getNextShot().
luaFunc(getFirstShot)
{
	luaReturnPtr(Shot::getFirstShot());
}

/// getNextShot() -> Shot|NoObj
luaFunc(getNextShot)
{
	luaReturnPtr(Shot::getNextShot());
}

/// shot_getName(Shot s) -> string!
luaFunc(shot_getName)
{
	Shot *s = getShot(L);
	luaReturnStr(s ? s->getName() : "");
}

/// shot_setOut(Shot s, float outness)
/// Moves the shot forward along its velocity by `outness`.
luaFunc(shot_setOut)
{
	Shot *shot = getShot(L);
	float outness = lua_tonumber(L, 2);
	if (shot)
	{
		Vector adjust = shot->velocity;
		adjust.setLength2D(outness);
		shot->position += adjust;

	}
	luaReturnNil();
}

/// shot_setAimVector(Shot s, float ax, float ay)
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

/// shot_getEffectTime(Shot|string s) -> float!
luaFunc(shot_getEffectTime)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnNum(data ? data->effectTime : 0.0f);
	}

	Shot *shot = getShot(L);
	luaReturnNum((shot && shot->shotData) ? shot->shotData->effectTime : 0.0f);
}

/// shot_isIgnoreShield(Shot|string s) -> bool!
luaFunc(shot_isIgnoreShield)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnBool(data ? data->ignoreShield : false);
	}

	Shot *shot = getShot(L);
	luaReturnBool((shot && shot->shotData) ? shot->shotData->ignoreShield : false);
}

/// shot_getFirer(Shot s) -> Entity|NoObj!
luaFunc(shot_getFirer)
{
	Shot *shot = getShot(L);
	luaReturnPtr(shot ? shot->firer : NULL);
}

/// shot_setFirer(Shot s, Entity|NoObj? firer)
luaFunc(shot_setFirer)
{
	Shot *shot = getShot(L);
	if(shot)
	{
		Entity *e = entityOpt(L, 2);
		shot->firer = e;
	}

	luaReturnNil();
}

/// shot_setTarget(Shot s, Entity|NoObj? target)
luaFunc(shot_setTarget)
{
	Shot *shot = getShot(L);
	if(shot)
	{
		Entity *e = entityOpt(L, 2);
		shot->setTarget(e);
	}
	luaReturnNil();
}

/// shot_getTarget(Shot s) -> Entity|NoObj!
luaFunc(shot_getTarget)
{
	Shot *shot = getShot(L);
	luaReturnPtr(shot ? shot->target : NULL);
}

/// shot_setExtraDamage(Shot s, float dmg)
luaFunc(shot_setExtraDamage)
{
	Shot *shot = getShot(L);
	if(shot)
		shot->extraDamage = lua_tonumber(L, 2);
	luaReturnNil();
}

/// shot_getExtraDamage(Shot s) -> float!
luaFunc(shot_getExtraDamage)
{
	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->extraDamage : 0.0f);
}

/// shot_getDamage(Shot s) -> float!
luaFunc(shot_getDamage)
{
	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->getDamage() : 0.0f);
}

/// shot_getDamageType(Shot|string s) -> DamageType!
luaFunc(shot_getDamageType)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnInt(data ? data->damageType : DT_NONE);
	}

	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->getDamageType() : DT_NONE);
}

/// shot_getMaxSpeed(Shot|string s) -> float!
luaFunc(shot_getMaxSpeed)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnNum(data ? data->maxSpeed : 0.0f);
	}

	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->maxSpeed : 0.0f);
}

/// shot_setMaxSpeed(Shot s, float speed)
luaFunc(shot_setMaxSpeed)
{
	Shot *shot = getShot(L);
	if (shot)
		shot->maxSpeed = lua_tonumber(L, 2);
	luaReturnNil();
}

/// shot_getHomingness(Shot|string s) -> float!
luaFunc(shot_getHomingness)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnNum(data ? data->homing : 0.0f);
	}

	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->homingness : 0.0f);
}

/// shot_setHomingness(Shot s, float homing)
luaFunc(shot_setHomingness)
{
	Shot *shot = getShot(L);
	if (shot)
		shot->homingness = lua_tonumber(L, 2);
	luaReturnNil();
}

/// shot_getLifeTime(Shot|string s) -> float!
luaFunc(shot_getLifeTime)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnNum(data ? data->lifeTime : 0.0f);
	}

	Shot *shot = getShot(L);
	luaReturnNum(shot ? shot->lifeTime : 0.0f);
}

/// shot_setLifeTime(Shot s, float t)
luaFunc(shot_setLifeTime)
{
	Shot *shot = getShot(L);
	if (shot)
		shot->lifeTime = lua_tonumber(L, 2);
	luaReturnNil();
}

/// shot_setDamageType(Shot s, DamageType dt)
luaFunc(shot_setDamageType)
{
	Shot *shot = getShot(L);
	if (shot)
		shot->damageType = (DamageType)lua_tointeger(L, 2);
	luaReturnNil();
}

/// shot_setCheckDamageTarget(Shot s, boolx on)
luaFunc(shot_setCheckDamageTarget)
{
	Shot *shot = getShot(L);
	if (shot)
		shot->checkDamageTarget = getBool(L, 2);
	luaReturnNil();
}

/// shot_isCheckDamageTarget(Shot|string s) -> bool!
luaFunc(shot_isCheckDamageTarget)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnBool(data ? data->checkDamageTarget : false);
	}

	Shot *shot = getShot(L);
	luaReturnBool(shot ? shot->checkDamageTarget : false);
}

/// shot_setTrailPrt(Shot s, string prt)
luaFunc(shot_setTrailPrt)
{
	Shot *shot = getShot(L);
	if (shot)
		shot->setParticleEffect(getString(L, 2));
	luaReturnNil();
}

/// shot_setTargetPoint(Shot s, int pt)
luaFunc(shot_setTargetPoint)
{
	Shot *shot = getShot(L);
	if (shot)
		shot->setTargetPoint(lua_tointeger(L, 2));
	luaReturnNil();
}

/// shot_getTargetPoint(Shot s) -> int!
luaFunc(shot_getTargetPoint)
{
	Shot *shot = getShot(L);
	luaReturnInt(shot ? shot->targetPt : 0);
}

/// shot_canHitEntity(Shot s, Entity e) -> bool!
luaFunc(shot_canHitEntity)
{
	Shot *shot = getShot(L);
	Entity *e = entity(L, 2);
	bool hit = false;
	if(shot && e && shot->isActive() && game->isEntityCollideWithShot(e, shot))
	{
		DamageData d;
		d.attacker = shot->firer;
		d.damage = shot->getDamage();
		d.damageType = shot->getDamageType();
		d.shot = shot;
		hit = e->canShotHit(d);
	}
	luaReturnBool(hit);
}

/// shot_setHitEnts(Shot s, boolx on)
luaFunc(shot_setHitEnts)
{
	Shot *shot = getShot(L);
	if(shot)
		shot->hitEnts = getBool(L, 2);
	luaReturnNil();
}

/// shot_getHitEnts(Shot|string s) -> bool!
luaFunc(shot_getHitEnts)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnBool(data ? data->hitEnts : false);
	}

	Shot *shot = getShot(L);
	luaReturnBool(shot && shot->hitEnts);
}

/// shot_setHitWalls(Shot s, boolx on)
luaFunc(shot_setHitWalls)
{
	Shot *shot = getShot(L);
	if(shot)
		shot->hitWalls = getBool(L, 2);
	luaReturnNil();
}

/// shot_getHitWalls(Shot|string s) -> bool!
luaFunc(shot_getHitWalls)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnBool(data ? data->hitWalls : false);
	}

	Shot *shot = getShot(L);
	luaReturnBool(shot && shot->hitWalls);
}

/// shot_setAlwaysMaxSpeed(Shot s, boolx on)
luaFunc(shot_setAlwaysMaxSpeed)
{
	if(Shot *shot = getShot(L))
		shot->alwaysMaxSpeed = getBool(L, 2);
	luaReturnNil();
}

/// shot_isAlwaysMaxSpeed(Shot s) -> bool!
luaFunc(shot_isAlwaysMaxSpeed)
{
	if (lua_isstring(L, 1))
	{
		ShotData *data = Shot::getShotData(lua_tostring(L, 1));
		luaReturnBool(data ? data->alwaysMaxSpeed : false);
	}

	Shot *shot = getShot(L);
	luaReturnBool(shot && shot->alwaysMaxSpeed);
}


typedef std::pair<Shot*, float> ShotDistancePair;
static std::vector<ShotDistancePair> filteredShots(20);
static size_t filteredShotIdx = 0;

static bool _shotDistanceCmp(const ShotDistancePair& a, const ShotDistancePair& b)
{
	return a.second < b.second;
}
static bool _shotDistanceEq(const ShotDistancePair& a, const ShotDistancePair& b)
{
	return a.first == b.first;
}

static size_t _shotFilter(lua_State *L)
{
	const Vector p(lua_tonumber(L, 1), lua_tonumber(L, 2));
	const float radius = lua_tonumber(L, 3);
	const Shot *ignore = getShotOpt(L, 4);
	const DamageType dt = lua_isnumber(L, 5) ? (DamageType)lua_tointeger(L, 5) : DT_NONE;

	const float sqrRadius = radius * radius;
	float distsq;
	const bool skipRadiusCheck = radius <= 0;
	size_t added = 0;
	for(Shot::Shots::iterator it = Shot::shots.begin(); it != Shot::shots.end(); ++it)
	{
		Shot *s = *it;

		if (s != ignore && s->isActive() && s->life >= 1.0f)
		{
			if (dt == DT_NONE || s->getDamageType() == dt)
			{
				distsq = (s->position - p).getSquaredLength2D();
				if (skipRadiusCheck || distsq <= sqrRadius)
				{
					filteredShots.push_back(std::make_pair(s, distsq));
					++added;
				}
			}
		}
	}
	if(added)
	{
		std::sort(filteredShots.begin(), filteredShots.end(), _shotDistanceCmp);
		std::vector<ShotDistancePair>::iterator newend = std::unique(filteredShots.begin(), filteredShots.end(), _shotDistanceEq);
		filteredShots.resize(std::distance(filteredShots.begin(), newend));
	}

	// Add terminator if there is none
	if(!filteredShots.size() || filteredShots.back().first)
		filteredShots.push_back(std::make_pair((Shot*)NULL, 0.0f)); // terminator

	filteredShotIdx = 0; // Reset getNextFilteredShot() iteration index

	return added;
}

/// filterNearestShots(float x, float y, float radius = 0, Shot? ignore, DamageType? dt) -> int
/// radius <= 0: no limit. Returns the number of shots found; iterate with getNextFilteredShot().
luaFunc(filterNearestShots)
{
	filteredShots.clear();
	luaReturnInt(_shotFilter(L));
}

/// filterNearestShotsAdd(float x, float y, float radius = 0, Shot? ignore, DamageType? dt) -> int
luaFunc(filterNearestShotsAdd)
{
	// Remove terminator if there is one
	if(filteredShots.size() && !filteredShots.back().first)
		filteredShots.pop_back();
	luaReturnInt(_shotFilter(L));
}

/// getNextFilteredShot() -> Shot|NoObj, float
/// Returns the next shot and its squared distance; 0 at the end.
luaFunc(getNextFilteredShot)
{
	ShotDistancePair sp = filteredShots[filteredShotIdx];
	if (sp.first)
		++filteredShotIdx;
	luaPushPointer(L, sp.first);
	lua_pushnumber(L, sp.second);
	return 2;
}


/// entity_setVel(Entity e, float vx, float vy)
luaFunc(entity_setVel)
{
	Entity *e = entity(L);
	if (e)
		e->vel = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

/// entity_setVelLen(Entity e, float len)
luaFunc(entity_setVelLen)
{
	Entity *e = entity(L);
	if (e)
		e->vel.setLength2D(lua_tonumber(L, 2));
	luaReturnNil();
}

/// entity_getVelLen(Entity e) -> float!
luaFunc(entity_getVelLen)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->vel.getLength2D() : 0.0f);
}


/// entity_getVel(Entity e) -> float!, float
luaFunc(entity_getVel)
{
	Vector v;
	Entity *e = entity(L);
	if (e)
		v = e->vel;
	luaReturnVec2(v.x, v.y);
}

/// entity_velx(Entity e) -> float!
luaFunc(entity_velx)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->vel.x : 0.0f);
}

/// entity_vely(Entity e) -> float!
luaFunc(entity_vely)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->vel.y : 0.0f);
}

/// entity_addVel(Entity e, float vx, float vy)
luaFunc(entity_addVel)
{
	Entity *e = entity(L);
	if (e)
		e->vel += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

/// entity_addRandomVel(Entity e, float len) -> float, float
/// Returns the added velocity.
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

/// entity_isVelIn(Entity e, float len) -> bool!
luaFunc(entity_isVelIn)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->vel.isLength2DIn(lua_tonumber(L, 2)) : false);

}

/// entity_clearVel(Entity e)
luaFunc(entity_clearVel)
{
	Entity *e = entity(L);
	if (e)
		e->vel = Vector(0,0,0);
	luaReturnNil();
}
// end extra Entity::vel functions

/// entity_getPushVec(Entity e) -> float!, float
luaFunc(entity_getPushVec)
{
	Entity *e = entity(L);
	Vector v;
	if (e)
		v = e->getPushVec();
	luaReturnVec2(v.x, v.y);
}

/// entity_addIgnoreShotDamageType(Entity e, DamageType dt)
/// Makes entity_doSpellAvoidance() not dodge shots of certain DamageTypes
luaFunc(entity_addIgnoreShotDamageType)
{
	Entity *e = entity(L);
	if (e)
	{
		e->addIgnoreShotDamageType((DamageType)lua_tointeger(L, 2));
	}
	luaReturnNil();
}

/// entity_warpLastPosition(Entity e)
luaFunc(entity_warpLastPosition)
{
	Entity *e = entity(L);
	if (e)
	{
		e->warpLastPosition();
	}
	luaReturnNil();
}


// LUAWTF-NOTE(weird): adds to `vel2`, not `vel` like the surrounding entity_*Vel functions
/// entity_velTowards(Entity e, float x, float y, float velLen, float range = 0)
/// range 0: no range limit.
luaFunc(entity_velTowards)
{
	Entity *e = entity(L);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	float velLen = lua_tonumber(L, 4);
	float range = lua_tonumber(L, 5);
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

/// entity_getBoneLockEntity(Entity e) -> Entity|NoObj!
luaFunc(entity_getBoneLockEntity)
{
	Entity *e = entity(L);
	Entity *ent = NULL;
	if (e)
	{
		BoneLock *b = e->getBoneLock();
		ent = b->entity;

	}
	luaReturnPtr(ent);
}

/// entity_getBoneLockData(Entity e) -> bool?!, Bone|NoObj?, float?, float?, float?, float?, float?
/// on, bone, origRot, wallNormal x/y, circleOffset x/y; nothing for an invalid entity.
luaFunc(entity_getBoneLockData)
{
	Entity *e = entity(L);
	if (!e)
		luaReturnNil();
	BoneLock b = *e->getBoneLock(); // always safe to deref

	lua_pushboolean(L, b.on);
	luaPushPointer(L, b.bone);
	lua_pushnumber(L, b.origRot);
	lua_pushnumber(L, b.wallNormal.x);
	lua_pushnumber(L, b.wallNormal.y);
	lua_pushnumber(L, b.circleOffset.x);
	lua_pushnumber(L, b.circleOffset.y);
	return 7;
}

/// entity_ensureLimit(Entity e, int num, State state = 0)
/// Over `num` entities named like e: kills the extra ones if state is 0, else sets `state` on them.
luaFunc(entity_ensureLimit)
{
	if(Entity *e = entity(L))
		game->ensureLimit(e, lua_tointeger(L, 2), lua_tointeger(L, 3));
	luaReturnNil();
}

/// entity_setRidingPosition(Entity e, float x, float y)
luaFunc(entity_setRidingPosition)
{
	Entity *e = entity(L);
	if (e)
		e->setRidingPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	luaReturnNil();
}

/// entity_setRidingData(Entity e, float x, float y, float rot, boolx flip)
luaFunc(entity_setRidingData)
{
	Entity *e = entity(L);
	if (e)
		e->setRidingData(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), getBool(L, 5));
	luaReturnNil();
}

/// entity_getRidingPosition(Entity e) -> float!, float
luaFunc(entity_getRidingPosition)
{
	Entity *e = entity(L);
	Vector v;
	if (e)
		v = e->getRidingPosition();
	luaReturnVec2(v.x, v.y);
}

/// entity_getRidingRotation(Entity e) -> float!
luaFunc(entity_getRidingRotation)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->getRidingRotation() : 0.0f);
}

/// entity_getRidingFlip(Entity e) -> bool!
luaFunc(entity_getRidingFlip)
{
	Entity *e = entity(L);
	luaReturnBool(e && e->getRidingFlip());
}


/// entity_setBoneLock(Entity e, Entity|NoObj? target, Bone|NoObj? b) -> bool
/// No target: release the lock. Returns whether the lock changed; it will fail when the internal delay hasn't expired yet.
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
		}
		ret = e->setBoneLock(bl);
	}
	luaReturnBool(ret);
}

/// entity_setBoneLockRotation(Entity e, float rot) -> bool
luaFunc(entity_setBoneLockRotation)
{
	bool ret = false;
	if(Entity *e = entity(L))
	{
		if(BoneLock *bl = e->getBoneLock())
		{
			bl->origRot = lua_tonumber(L, 2);
			e->updateBoneLock();
			ret = true;
		}
	}
	luaReturnBool(ret);
}


/// entity_setBoneLockOffset(Entity e, float x, float y) -> bool
luaFunc(entity_setBoneLockOffset)
{
	bool ret = false;
	if(Entity *e = entity(L))
	{
		if(BoneLock *bl = e->getBoneLock())
		{
			bl->circleOffset.x = lua_tonumber(L, 2);
			bl->circleOffset.y = lua_tonumber(L, 3);
			e->updateBoneLock();
			ret = true;
		}
	}
	luaReturnBool(ret);
}

/// entity_setBoneLockDelay(Entity e, float t)
luaFunc(entity_setBoneLockDelay)
{
	if(Entity *e = entity(L))
		e->boneLockDelay = lua_tonumber(L, 2);
	luaReturnNil();
}

/// entity_getBoneLockDelay(Entity e) -> float!
luaFunc(entity_getBoneLockDelay)
{
	float t = 0;
	if(Entity *e = entity(L))
		t = e->boneLockDelay;
	luaReturnNum(t);
}

// No game script uses this; and i'm not aware of any mods using it either -- FG
/// entity_setIngredient(Entity e, string name); deprecated
luaFunc(entity_setIngredient)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setIngredientData(getString(L,2));
	}
	luaReturnNil();
}

/// entity_setSegsMaxDist(ScriptedEntity e, float dist)
luaFunc(entity_setSegsMaxDist)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->setMaxDist(lua_tonumber(L, 2));

	luaReturnNil();
}

// LUAWTF-NOTE(weird): returns the value passed in (`luaReturnNum(v)`), even for an invalid entity; no range check on the cast to BounceType
/// entity_setBounceType(Entity e, Bounce bt) -> Bounce
luaFunc(entity_setBounceType)
{
	Entity *e = entity(L);
	int v = lua_tointeger(L, 2);
	if (e)
	{
		e->setBounceType((BounceType)v);
	}
	luaReturnInt(v);
}


/// user_set_demo_intro(int intro)
/// Sets whether the intro video has played (if set to 1, the intro will play on the next game start)
luaFunc(user_set_demo_intro)
{
#ifndef AQUARIA_DEMO
	dsq->user.demo.intro = lua_tointeger(L, 1);
#endif
	luaReturnNil();
}

/// user_save()
/// Saves usersettings.xml to disk.
luaFunc(user_save)
{
	dsq->user.save();
	luaReturnNil();
}

// LUAWTF-NOTE(weird): returns the value passed in, even for an invalid entity
/// entity_setAutoSkeletalUpdate(ScriptedEntity e, boolx on) -> bool
luaFunc(entity_setAutoSkeletalUpdate)
{
	ScriptedEntity *e = scriptedEntity(L);
	bool v = getBool(L, 2);
	if (e)
		e->setAutoSkeletalUpdate(v);
	luaReturnBool(v);
}

/// entity_getBounceType(Entity e) -> Bounce!
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

/// entity_setDieTimer(Entity e, float t)
luaFunc(entity_setDieTimer)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDieTimer(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

/// entity_setLookAtPoint(Entity e, float x, float y)
luaFunc(entity_setLookAtPoint)
{
	Entity *e = entity(L);
	if (e)
	{
		e->lookAtPoint = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnNil();
}

/// entity_getLookAtPoint(Entity e) -> float!, float
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

/// entity_setRiding(Entity e, Entity|NoObj mount)
luaFunc(entity_setRiding)
{
	Entity *e = entity(L);
	Entity *e2 = entityOpt(L, 2);
	if (e)
	{
		e->setRiding(e2);
	}
	luaReturnNil();
}

/// entity_getHealthPerc(Entity e) -> float!
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

/// entity_getRiding(Entity e) -> Entity|NoObj!
luaFunc(entity_getRiding)
{
	Entity *e = entity(L);
	Entity *ret = 0;
	if (e)
		ret = e->getRiding();
	luaReturnPtr(ret);
}

/// entity_setTargetPriority(Entity e, int prio)
luaFunc(entity_setTargetPriority)
{
	Entity *e = entity(L);
	if (e)
	{
		e->targetPriority = lua_tointeger(L, 2);
	}
	luaReturnNil();
}

/// entity_getTargetPriority(Entity e) -> int!
luaFunc(entity_getTargetPriority)
{
	Entity *e = entity(L);
	luaReturnInt(e ? e->targetPriority : 0);
}

// returns nil for non-scripted entities
/// entity_v(Entity e) -> table?!
/// The instance's `v` table; nothing for non-scripted entities.
luaFunc(entity_v)
{
	ScriptedEntity *se = scriptedEntityOpt(L);
	return se ? se->pushLuaVars(L) : 0;
}

/// node_v(Node n) -> table?!
luaFunc(node_v)
{
	Path *n = path(L);
	return n ? n->pushLuaVars(L) : 0;
}

/// shot_v(Shot s) -> table?!
luaFunc(shot_v)
{
	Shot *s = getShot(L);
	return s ? s->pushLuaVars(L) : 0;
}

/// isQuitFlag() -> bool!
luaFunc(isQuitFlag)
{
	luaReturnBool(dsq->isQuitFlag());
}

/// isDeveloperKeys() -> bool!
luaFunc(isDeveloperKeys)
{
	luaReturnBool(dsq->isDeveloperKeys());
}

/// isDemo() -> bool!
luaFunc(isDemo)
{
#ifdef AQUARIA_DEMO
	luaReturnBool(true);
#else
	luaReturnBool(false);
#endif
}

/// isWithin(float x1, float y1, float x2, float y2, float dist) -> bool!
luaFunc(isWithin)
{
	Vector v1 = getVector(L, 1);
	Vector v2 = getVector(L, 3);
	float dist = lua_tonumber(L, 5);

	Vector d = v2-v1;
	bool v = false;
	if (d.isLength2DIn(dist))
	{
		v = true;
	}
	luaReturnBool(v);
}

/// toggleDamageSprite(boolx on)
luaFunc(toggleDamageSprite)
{
	game->toggleDamageSprite(getBool(L));
	luaReturnNil();
}

// LUAWTF-NOTE(weird): `time` left out from Lua is 0 (instant), not the C++ default -1 (= 0.1 s); scripts must pass -1 explicitly to get the default fade
/// toggleCursor(boolx on, float time = 0)
luaFunc(toggleCursor)
{
	dsq->toggleCursor(getBool(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// toggleBlackBars(boolx on)
luaFunc(toggleBlackBars)
{
	dsq->toggleBlackBars(getBool(L, 1));
	luaReturnNil();
}

/// setBlackBarsColor(float r, float g, float b)
luaFunc(setBlackBarsColor)
{
	Vector c(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
	dsq->setBlackBarsColor(c);
	luaReturnNil();
}

/// toggleLiCombat(boolx on)
luaFunc(toggleLiCombat)
{
	dsq->continuity.toggleLiCombat(getBool(L));
	luaReturnNil();
}

/// getNoteName(int note, string prefix = "") -> string!
luaFunc(getNoteName)
{
	luaReturnStr(game->getNoteName(lua_tonumber(L, 1), getString(L, 2)).c_str());
}

/// getWorldType() -> WorldType!
luaFunc(getWorldType)
{
	luaReturnInt(dsq->continuity.getWorldType());
}

/// setWorldType(WorldType wt, boolx transition = false)
luaFunc(setWorldType)
{
	WorldType wt = (WorldType)lua_tointeger(L, 1);
	bool trans = getBool(L, 2);
	dsq->continuity.applyWorldEffects(wt, trans);
	luaReturnNil();
}

/// isWorldPaused() -> bool!
luaFunc(isWorldPaused)
{
	luaReturnBool(game->isWorldPaused());
}

/// setWorldPaused(boolx on)
luaFunc(setWorldPaused)
{
	game->setWorldPaused(getBool(L, 1));
	luaReturnNil();
}

/// getNearestNodeByType(float x, float y, PathType type) -> Node|NoObj!
luaFunc(getNearestNodeByType)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	int type = lua_tointeger(L, 3);

	luaReturnPtr(game->getNearestPath(Vector(x,y), (PathType)type));
}

/// fadeOutMusic(float time)
luaFunc(fadeOutMusic)
{
	dsq->sound->fadeMusic(SFT_OUT, lua_tonumber(L, 1));
	luaReturnNil();
}

/// getNode(string name) -> Node|NoObj!
luaFunc(getNode)
{
	luaReturnPtr(pathFromName(L));
}

/// getNodeToActivate() -> Node|NoObj!
luaFunc(getNodeToActivate)
{
	luaReturnPtr(game->avatar->pathToActivate);
}

/// setNodeToActivate(Node|NoObj n)
luaFunc(setNodeToActivate)
{
	game->avatar->pathToActivate = path(L, 1);
	luaReturnNil();
}

/// getEntityToActivate() -> Entity|NoObj!
luaFunc(getEntityToActivate)
{
	luaReturnPtr(game->avatar->entityToActivate);
}

/// setEntityToActivate(Entity|NoObj e)
luaFunc(setEntityToActivate)
{
	game->avatar->entityToActivate = entityOpt(L, 1);
	luaReturnNil();
}

/// hasThingToActivate() -> bool!
luaFunc(hasThingToActivate)
{
	luaReturnBool(game->avatar->hasThingToActivate());
}

/// setActivation(boolx on)
/// Used to suppress entity/node activation on the title screen when menus are open.
luaFunc(setActivation)
{
	game->activation = getBool(L, 1);
	luaReturnNil();
}

/// debugLog(string msg) -> string
luaFunc(debugLog)
{
	const char *s = lua_tostring(L, 1);
	if(s)
		debugLog(s);
	luaReturnStr(s);
}

/// errorLog(string msg) -> string
luaFunc(errorLog)
{
	const char *s = lua_tostring(L, 1);
	if(s)
		errorLog(s);
	luaReturnStr(s);
}

/// reconstructGrid()
luaFunc(reconstructGrid)
{
	game->reconstructGrid(true, true);
	luaReturnNil();
}

/// reconstructEntityGrid()
luaFunc(reconstructEntityGrid)
{
	game->reconstructEntityGrid();
	luaReturnNil();
}

/// dilateGrid(int radius, int test, ObsType set, ObsType allowOverwrite)
/// `test` is an OT_* mask (sum of OT_* values).
luaFunc(dilateGrid)
{
	unsigned int radius = lua_tointeger(L, 1);
	ObsType test = (ObsType)lua_tointeger(L, 2);
	ObsType set = (ObsType)lua_tointeger(L, 3);
	ObsType allowOverwrite = (ObsType)lua_tointeger(L, 4);
	game->dilateGrid(radius, test, set, allowOverwrite);
	luaReturnNil();
}

/// entity_setCanLeaveWater(Entity e, boolx on)
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

/// entity_setSegmentTexture(ScriptedEntity e, int segment, string tex)
/// `segment` is 0-based.
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

/// entity_findNearestEntityOfType(Entity e, EntityType type, int maxRange = 0) -> Entity|NoObj!
/// maxRange 0 = unlimited.
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

/// createShot(string name, Entity firer, Entity|NoObj? target, float x = 0, float y = 0, float aimx = 0, float aimy = 0) -> Shot|NoObj
/// Position 0,0 = the firer's; aim 0,0 = towards the target
luaFunc(createShot)
{
	Entity *e = entity(L,2);
	Entity *t = entityOpt(L, 3);
	Vector pos = getVector(L, 4);
	Vector aim = getVector(L, 6);

	Shot *s = game->fireShot(getString(L, 1), e, t, pos, aim);

	luaReturnPtr(s);
}

/// isSkippingCutscene() -> bool!
luaFunc(isSkippingCutscene)
{
	luaReturnBool(dsq->isSkippingCutscene());
}

// deprecated, use entity_playSfx
/// entity_sound(Entity e, string name, float freq = 1, float fadeOut = 0); deprecated("use entity_playSfx")
/// freq >= 100 is scaled by 0.001 (old scripts use ~1000 as normal pitch); <= 0 means 1.
/// entity_soundFreq(Entity e, string name, float freq = 1, float fadeOut = 0); deprecated("use entity_playSfx")
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

/// entity_playSfx(Entity e, string name, float freq = 1, float vol = 1, int loops = 0, float fadeOut = 0, float maxdist = 0) -> lightuserdata|NoObj
/// Returns the sound handle. freq/vol <= 0 mean 1; maxdist 0 = default, -1 = no attenuation.
luaFunc(entity_playSfx)
{
	Entity *e = entity(L);
	void *h = NULL;
	if (e && !dsq->isSkippingCutscene())
	{
		PlaySfx sfx;
		sfx.name = getString(L, 2);
		sfx.freq = lua_tonumber(L, 3);
		sfx.vol = lua_tonumber(L, 4);
		if(sfx.vol <= 0)
			sfx.vol = 1;
		sfx.loops = lua_tonumber(L, 5);

		float fadeOut = lua_tonumber(L, 6);
		sfx.maxdist = lua_tonumber(L, 7);
		sfx.relative = false;
		sfx.positional = true;
		Vector pos = e->position + e->offset;
		sfx.x = pos.x;
		sfx.y = pos.y;

		h = core->sound->playSfx(sfx);
		if (fadeOut != 0)
		{
			sound->fadeSfx(h, SFT_OUT, fadeOut);
		}

		e->linkSound(h);
		e->updateSoundPosition();
	}
	luaReturnPtr(h);
}

/// entity_setStopSoundsOnDeath(Entity e, boolx on)
luaFunc(entity_setStopSoundsOnDeath)
{
	Entity *e = entity(L);
	if (e)
		e->setStopSoundsOnDeath(getBool(L, 2));
	luaReturnNil();
}

/// entity_setSpiritFreeze(Entity e, boolx on)
luaFunc(entity_setSpiritFreeze)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setSpiritFreeze(getBool(L,2));
	}
	luaReturnNil();
}

/// entity_setPauseFreeze(Entity e, boolx on)
luaFunc(entity_setPauseFreeze)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setPauseFreeze(getBool(L,2));
	}
	luaReturnNil();
}

/// node_setSpiritFreeze(Node n, boolx on)
luaFunc(node_setSpiritFreeze)
{
	Path *e = path(L);
	if (e)
		e->spiritFreeze = getBool(L,2);
	luaReturnNil();
}

/// node_setPauseFreeze(Node n, boolx on)
luaFunc(node_setPauseFreeze)
{
	Path *e = path(L);
	if (e)
		e->pauseFreeze = getBool(L,2);
	luaReturnNil();
}

// LUAWTF-NOTE(api): accepts 2 (= untrimmed) via `getBoolOrInt`, but entity_isFillGrid pushes booleans (`lua_pushboolean(L, e->fillGridFromQuad)`), so 1 vs 2 can't be read back
/// entity_setFillGrid(Entity e, boolx fromQuad, boolx fromSkel = false)
/// true/1 = fill trimmed, 2 = untrimmed.
luaFunc(entity_setFillGrid)
{
	Entity *e = entity(L);
	if (e)
	{
		e->fillGridFromQuad = getBoolOrInt(L, 2);
		e->fillGridFromSkel = getBoolOrInt(L, 3);
	}
	luaReturnNil();
}

/// entity_isFillGrid(Entity e) -> bool?!, bool?
luaFunc(entity_isFillGrid)
{
	Entity *e = entity(L);
	if(!e)
		return 0;

	lua_pushboolean(L, e->fillGridFromQuad);
	lua_pushboolean(L, e->fillGridFromSkel);
	return 2;
}

/// entity_getAimVector(Entity e, float adjust, float len, boolx flip = false) -> float!, float
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

/// entity_getVectorToEntity(Entity e1, Entity e2) -> float!, float
luaFunc(entity_getVectorToEntity)
{
	typecheckOnly(entity(L));
	typecheckOnly(entity(L, 2));
	return forwardCall(obj_getVectorToObj);
}

/// entity_setDropChance(Entity e, int chance, float amount = 0)
/// amount 0 keeps the current mana ball amount (scripted entities only).
luaFunc(entity_setDropChance)
{
	Entity *e = entity(L);
	if (e)
	{
		e->dropChance = lua_tonumber(L, 2);
		float amount = lua_tonumber(L, 3);
		ScriptedEntity *se = dynamic_cast<ScriptedEntity*>(e);
		if (se && amount)
		{
			se->manaBallAmount = amount;
		}
	}
	luaReturnNil();
}

/// entity_warpToNode(Entity e, Node n)
luaFunc(entity_warpToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->position.stopPath();
		e->position = p->nodes[0].position;
		e->rotateToVec(Vector(0,-1), 0.1f);
	}
	luaReturnNil();
}

/// entity_stopPull(Entity e)
luaFunc(entity_stopPull)
{
	Entity *e = entity(L);
	if (e)
		e->stopPull();
	luaReturnNil();
}

/// entity_moveToNode(Entity e, Node n, Speed speed = SPEED_NORMAL, boolx dieOnPathEnd = false) -> float
/// Returns how long moving will take at the given speed.
luaFunc(entity_moveToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	float time = 0;
	if (e && p)
	{
		float speed = dsq->continuity.getSpeedType(lua_tointeger(L, 3));
		bool dieOnPathEnd = getBool(L, 4);
		time = e->moveToPos(p->nodes[0].position, speed, dieOnPathEnd, false);
	}
	luaReturnNum(time);
}

/// entity_swimToNode(Entity e, Node n, Speed speed = SPEED_NORMAL, boolx dieOnPathEnd = false) -> float
/// Returns how long swimming will take at the given speed.
luaFunc(entity_swimToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	float time = 0;
	if (e && p)
	{
		float speed = dsq->continuity.getSpeedType(lua_tointeger(L, 3));
		bool dieOnPathEnd = getBool(L, 4);
		time = e->moveToPos(p->nodes[0].position, speed, dieOnPathEnd, true);
	}
	luaReturnNum(time);
}

/// entity_swimToPosition(Entity e, float x, float y, Speed speed = SPEED_NORMAL, boolx dieOnPathEnd = false) -> float
luaFunc(entity_swimToPosition)
{
	Entity *e = entity(L);
	float time = 0;
	if (e)
	{
		float speed = dsq->continuity.getSpeedType(lua_tointeger(L, 4));
		bool dieOnPathEnd = getBool(L, 5);
		time = e->moveToPos(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), speed, dieOnPathEnd, true);
	}
	luaReturnNum(time);
}

/// entity_moveToNodeSpeed(Entity e, Node n, float speed, boolx dieOnPathEnd = false) -> float
luaFunc(entity_moveToNodeSpeed)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	float time = 0;
	if (e && p)
		time = e->moveToPos(p->nodes[0].position, lua_tonumber(L, 3), getBool(L, 4), false);
	luaReturnNum(time);
}

/// entity_swimToNodeSpeed(Entity e, Node n, float speed, boolx dieOnPathEnd = false) -> float
luaFunc(entity_swimToNodeSpeed)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	float time = 0;
	if (e && p)
		time = e->moveToPos(p->nodes[0].position, lua_tonumber(L, 3), getBool(L, 4), true);
	luaReturnNum(time);
}

/// entity_swimToPositionSpeed(Entity e, float x, float y, float speed, boolx dieOnPathEnd = false) -> float
luaFunc(entity_swimToPositionSpeed)
{
	Entity *e = entity(L);
	float time = 0;
	if (e)
		time = e->moveToPos(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), getBool(L, 5), true);
	luaReturnNum(time);
}


/// avatar_setCanDie(boolx on)
luaFunc(avatar_setCanDie)
{
	game->avatar->canDie = getBool(L, 1);

	luaReturnNil();
}

// not naming this avatar_* because it rather belongs into the UI category...
/// setCanActivate(boolx on)
luaFunc(setCanActivate)
{
	game->avatar->setCanActivateStuff(getBool(L, 1));
	luaReturnNil();
}

/// setSeeMapMode(SeeMap mode)
luaFunc(setSeeMapMode)
{
	game->avatar->setSeeMapMode((SeeMapMode)lua_tointeger(L, 1));
	luaReturnNil();
}

/// avatar_setCanBurst(boolx on)
luaFunc(avatar_setCanBurst)
{
	game->avatar->setCanBurst(getBool(L, 1));
	luaReturnNil();
}

/// avatar_canBurst() -> bool!
luaFunc(avatar_canBurst)
{
	luaReturnBool(game->avatar->canBurst());
}

/// avatar_setCanLockToWall(boolx on)
luaFunc(avatar_setCanLockToWall)
{
	game->avatar->setCanLockToWall(getBool(L, 1));
	luaReturnNil();
}

/// avatar_canLockToWall() -> bool!
luaFunc(avatar_canLockToWall)
{
	luaReturnBool(game->avatar->canLockToWall());
}

/// avatar_setCanSwimAgainstCurrents(boolx on)
luaFunc(avatar_setCanSwimAgainstCurrents)
{
	game->avatar->setCanSwimAgainstCurrents(getBool(L, 1));
	luaReturnNil();
}

/// avatar_canSwimAgainstCurrents() -> bool!
luaFunc(avatar_canSwimAgainstCurrents)
{
	luaReturnBool(game->avatar->canSwimAgainstCurrents());
}

/// avatar_setCanCollideWithShots(boolx on)
luaFunc(avatar_setCanCollideWithShots)
{
	game->avatar->setCollideWithShots(getBool(L, 1));
	luaReturnNil();
}

/// avatar_canCollideWithShots() -> bool!
luaFunc(avatar_canCollideWithShots)
{
	luaReturnBool(game->avatar->canCollideWithShots());
}

/// avatar_setCollisionAvoidanceData(int range, float mod)
luaFunc(avatar_setCollisionAvoidanceData)
{
	game->avatar->setCollisionAvoidanceData(lua_tointeger(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// avatar_toggleCape(boolx on)
luaFunc(avatar_toggleCape)
{
	game->avatar->toggleCape(getBool(L,1));

	luaReturnNil();
}

/// avatar_setBlockSinging(boolx on)
luaFunc(avatar_setBlockSinging)
{
	bool b = getBool(L);
	game->avatar->setBlockSinging(b);
	luaReturnNil();
}

/// avatar_isBlockSinging() -> bool!
luaFunc(avatar_isBlockSinging)
{
	luaReturnBool(game->avatar->isBlockSinging());
}

/// avatar_setBlockBackflip(boolx on)
luaFunc(avatar_setBlockBackflip)
{
	game->avatar->blockBackFlip = getBool(L);
	luaReturnNil();
}

/// avatar_isBlockBackflip() -> bool!
luaFunc(avatar_isBlockBackflip)
{
	luaReturnBool(game->avatar->blockBackFlip);
}

/// avatar_fallOffWall()
luaFunc(avatar_fallOffWall)
{
	game->avatar->fallOffWall();
	luaReturnNil();
}

/// avatar_isBursting() -> bool!
luaFunc(avatar_isBursting)
{
	luaReturnBool(game->avatar->bursting);
}

/// avatar_isLockable() -> bool!
luaFunc(avatar_isLockable)
{
	luaReturnBool(game->avatar->isLockable());
}

/// avatar_isRolling() -> bool!
luaFunc(avatar_isRolling)
{
	luaReturnBool(game->avatar->isRolling());
}

/// avatar_isSwimming() -> bool!
luaFunc(avatar_isSwimming)
{
	luaReturnBool(game->avatar->isSwimming());
}

/// avatar_isOnWall() -> bool!
luaFunc(avatar_isOnWall)
{
	bool v = game->avatar->state.lockedToWall;
	luaReturnBool(v);
}

/// avatar_isShieldActive() -> bool!
luaFunc(avatar_isShieldActive)
{
	bool v = (game->avatar->activeAura == AURA_SHIELD);
	luaReturnBool(v);
}

/// avatar_setShieldActive(boolx on)
luaFunc(avatar_setShieldActive)
{
	bool on = getBool(L, 1);
	if (on)
		game->avatar->activateAura(AURA_SHIELD);
	else
		game->avatar->stopAura();
	luaReturnNil();
}

/// avatar_getStillTimer() -> float!
luaFunc(avatar_getStillTimer)
{
	luaReturnNum(game->avatar->stillTimer.getValue());
}

/// avatar_getRollDirection() -> int!
/// 0 when not rolling.
luaFunc(avatar_getRollDirection)
{
	int v = 0;
	if (game->avatar->isRolling())
		v = game->avatar->rollDir;
	luaReturnNum(v);
}

/// avatar_getSpellCharge() -> float!; deprecated
luaFunc(avatar_getSpellCharge)
{
	luaReturnNum(game->avatar->state.spellCharge);
}

/// avatar_setSpeedMult(float mult, float time)
/// Uses the same internal functionality as a food item that boosts speed
luaFunc(avatar_setSpeedMult)
{
	dsq->continuity.setSpeedMultiplier(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// avatar_setSpeedMult2(float mult); deprecated
luaFunc(avatar_setSpeedMult2)
{
	dsq->continuity.speedMult2 = lua_tonumber(L, 1);
	luaReturnNil();
}

/// avatar_getSpeedMult() -> float!
luaFunc(avatar_getSpeedMult)
{
	luaReturnNum(dsq->continuity.speedMult);
}

/// avatar_getSpeedMult2() -> float!; deprecated
luaFunc(avatar_getSpeedMult2)
{
	luaReturnNum(dsq->continuity.speedMult2);
}

/// avatar_setMaxBeastEats(int n) -> int
/// Returns the old maximum.
luaFunc(avatar_setMaxBeastEats)
{
	size_t oldeats = dsq->continuity.maxEats;
	dsq->continuity.maxEats = lua_tointeger(L, 1);
	luaReturnInt(oldeats);
}

/// jumpState(string state, boolx force = false)
luaFunc(jumpState)
{
	dsq->enqueueJumpState(getString(L, 1), getBool(L, 2));
	luaReturnNil();
}

/// goToTitle()
luaFunc(goToTitle)
{
	dsq->title(true);
	luaReturnNil();
}

/// getEnqueuedState() -> string!
luaFunc(getEnqueuedState)
{
	luaReturnStr(dsq->getEnqueuedJumpState().c_str());
}

/// learnSong(Song song)
luaFunc(learnSong)
{
	dsq->continuity.learnSong(lua_tointeger(L, 1));
	luaReturnNil();
}

/// unlearnSong(Song song)
luaFunc(unlearnSong)
{
	dsq->continuity.unlearnSong(lua_tointeger(L, 1));
	luaReturnNil();
}

/// showInGameMenu(boolx force = false, boolx optionsOnly = false, MenuPage page = MENUPAGE_NONE)
luaFunc(showInGameMenu)
{
	game->getInGameMenu()->show(getBool(L, 1), getBool(L, 2), (MenuPage)lua_tointeger(L, 3));
	luaReturnNil();
}

/// hideInGameMenu(boolx skipEffect = false, boolx cancel = false)
luaFunc(hideInGameMenu)
{
	bool skipEffect = getBool(L, 1);
	bool cancel = getBool(L, 2);
	game->getInGameMenu()->hide(!skipEffect, cancel);
	luaReturnNil();
}

/// showImage(string gfx)
luaFunc(showImage)
{
	game->showImage(getString(L));
	luaReturnNil();
}

/// hideImage()
luaFunc(hideImage)
{
	game->hideImage();
	luaReturnNil();
}

/// hasSong(Song song) -> bool!
luaFunc(hasSong)
{
	bool b = dsq->continuity.hasSong(lua_tointeger(L, 1));
	luaReturnBool(b);
}

/// loadSound(string name) -> lightuserdata|NoObj
luaFunc(loadSound)
{
	void *handle = core->sound->loadLocalSound(getString(L, 1));
	luaReturnPtr(handle);
}

/// loadMap(string scene, string node = "")
/// Without node: transition to the scene; with node: warp to that node. Empty scene does nothing.
luaFunc(loadMap)
{
	std::string s = getString(L, 1);
	std::string n = getString(L, 2);

	if (!s.empty())
	{
		if (!n.empty())
		{
			if (game->avatar)
				game->avatar->disableInput();
			game->warpToSceneNode(s, n);
		}
		else
		{
			if (game->avatar)
				game->avatar->disableInput();
			game->transitionToScene(s);
		}
	}
	luaReturnNil();
}

/// entity_followPath(Entity e, Node path, Speed speedType = SPEED_NORMAL, boolx reverse = false) -> float
/// Returns the travel time.
luaFunc(entity_followPath)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	float time = 0;
	if (e && p)
	{
		int speedType = lua_tointeger(L, 3);
		bool reverse = getBool(L, 4);
		float speed = dsq->continuity.getSpeedType(speedType);
		time = e->followPath(p, speed, reverse);
	}
	luaReturnNum(time);
}

/// entity_followPathSpeed(Entity e, Node path, float speed, boolx reverse = false) -> float
/// speed 0 = the entity's max speed. Returns the travel time.
luaFunc(entity_followPathSpeed)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	float time = 0;
	if (e && p)
	{
		float speed = lua_tonumber(L, 3);
		bool reverse = getBool(L, 4);
		time = e->followPath(p, speed, reverse);
	}
	luaReturnNum(time);
}

/// spawnIngredient(string name, float x, float y, int times = 1, boolx out = false) -> Ingredient|NoObj
luaFunc(spawnIngredient)
{
	int times = lua_tointeger(L, 4);
	if (times == 0) times = 1;
	bool out = getBool(L, 5);
	Entity *e = game->spawnIngredient(getString(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), times, out);

	luaReturnPtr(e);
}

/// getNearestIngredient(float x, float y, float radius) -> Ingredient|NoObj!
luaFunc(getNearestIngredient)
{
	Ingredient *i = game->getNearestIngredient(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)), lua_tonumber(L, 3));
	luaReturnPtr(i);
}

/// spawnAllIngredients(float x, float y)
luaFunc(spawnAllIngredients)
{
	dsq->spawnAllIngredients(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)));
	luaReturnNil();
}

/// spawnParticleEffect(string name, float x, float y, float delay = 0, float rot = 0, Layer layer = LR_PARTICLES, float follow = 0) -> ParticleEffect|NoObj
luaFunc(spawnParticleEffect)
{
	float t = lua_tonumber(L, 4);
	// having t and rot reversed compared to the DSQ function is intentional
	float rot = lua_tonumber(L, 5);
	int layer = lua_tointeger(L, 6);
	if (!layer)
		layer = LR_PARTICLES;
	float follow = lua_tonumber(L, 7);
	ParticleEffect *pe = dsq->spawnParticleEffect(getString(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)),
		rot, t, layer, follow);
	luaReturnPtr(pe);
}

/// setNumSuckPositions(int n)
luaFunc(setNumSuckPositions)
{
	particleManager->setNumSuckPositions(lua_tointeger(L, 1));
	luaReturnNil();
}

/// setSuckPosition(int idx, float x, float y)
luaFunc(setSuckPosition)
{
	particleManager->setSuckPosition(lua_tointeger(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	luaReturnNil();
}

/// getSuckPosition(int idx) -> float!, float
/// 0, 0 if idx is out of range.
luaFunc(getSuckPosition)
{
	Vector *v = particleManager->getSuckPosition(lua_tointeger(L, 1));
	if(v)
		luaReturnVec2(v->x, v->y);
	luaReturnVec2(0.0f, 0.0f);
}


/// bone_showFrame(Bone b, int frame)
/// Switch to texture index `frame` for bones that have multiple textures set up.
luaFunc(bone_showFrame)
{
	Bone *b = bone(L);
	if (b)
		b->showFrame(lua_tointeger(L, 2));
	luaReturnNil();
}

/// bone_setSegmentOffset(Bone b, float x, float y)
luaFunc(bone_setSegmentOffset)
{
	Bone *b = bone(L);
	if (b)
		b->segmentOffset = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

/// bone_setSegmentProps(Bone b, int minDist, int maxDist, boolx reverse = false)
luaFunc(bone_setSegmentProps)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setSegmentProps(lua_tonumber(L, 2), lua_tonumber(L, 3), getBool(L, 4));
	}
	luaReturnNil();
}

/// bone_setSegmentRotationInterpolationTime(Bone b, float time)
luaFunc(bone_setSegmentRotationInterpolationTime)
{
	Bone *b = bone(L);
	if (b)
	{
		b->segmentRotInterpTime = lua_tonumber(L, 2);
	}
	luaReturnNil();
}

/// bone_setSegmentChainHead(Bone b, boolx on)
luaFunc(bone_setSegmentChainHead)
{
	Bone *b = bone(L);
	if (b)
		b->segmentChain = getBool(L, 2);
	luaReturnNil();
}

/// bone_addSegment(Bone b, Bone segment); distinct(b, segment)
luaFunc(bone_addSegment)
{
	Bone *b = bone(L);
	Bone *b2 = bone(L, 2);
	if (b && b2)
		b->addSegment(b2);
	luaReturnNil();
}

/// bone_setAnimated(Bone b, AnimMode mode)
luaFunc(bone_setAnimated)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setAnimated(lua_tointeger(L, 2));
	}
	luaReturnNil();
}

/// bone_lookAtEntity(Bone b, Entity target, float time, float minAngle, float maxAngle, float offset = 0)
luaFunc(bone_lookAtEntity)
{
	Bone *b = bone(L);
	Entity *e = entity(L, 2);
	if (b && e)
	{
		Vector pos = e->position;
		if (e->getEntityType() == ET_AVATAR)
		{
			if(Bone *head = e->skeletalSprite.getBoneByIdx(1)) // HACK: Look Naija in the eyes
				pos = head->getWorldPosition();
		}
		b->lookAt(pos, lua_tonumber(L, 3), lua_tonumber(L, 4),lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	luaReturnNil();
}

/// bone_lookAtPosition(Bone b, float x, float y, float time, float minAngle, float maxAngle, float offset = 0)
luaFunc(bone_lookAtPosition)
{
	Bone *b = bone(L);
	if (b)
	{
		b->lookAt(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	luaReturnNil();
}

/// bone_toggleCollision(Bone b, boolx on)
luaFunc(bone_toggleCollision)
{
	Bone *b = bone(L);
	if(b)
		b->enableCollision = getBool(L, 2);
	luaReturnNil();
}

/// entity_stopFollowingPath(Entity e)
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

/// entity_slowToStopPath(Entity e, float time); deprecated
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

/// entity_createEntity(Entity e, string name) -> Entity|NoObj
/// Creates the entity at e's position.
luaFunc(entity_createEntity)
{
	Entity *e = entity(L);
	Entity *ret = NULL;
	const char *name = lua_tostring(L, 2);
	if (e && name)
		ret = game->createEntityTemp(name, e->position, true);
	luaReturnPtr(ret);
}

/// entity_checkSplash(Entity e, float x = 0, float y = 0) -> bool
/// Updates entity underwater state and does a splash effect if changed. If position is (0, 0), use the current entity's position.
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

/// entity_isInCurrent(Entity e) -> bool!
luaFunc(entity_isInCurrent)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->isInCurrent() : false);
}

/// entity_isUnderWater(Entity e, float x = 0, float y = 0) -> bool!
/// If position is (0, 0), use the current entity's position.
luaFunc(entity_isUnderWater)
{
	Entity *e = entity(L);
	bool b = false;
	if (e)
	{
		b = e->isUnderWater(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	}
	luaReturnBool(b);
}

/// entity_isBeingPulled(Entity e) -> bool!
luaFunc(entity_isBeingPulled)
{
	Entity *e = entity(L);
	bool v= false;
	if (e)
		v = (game->avatar && game->avatar->pullTarget == e);
	luaReturnBool(v);
}

/// avatar_setPullTarget(Entity|NoObj e)
/// 0 = stop pulling.
luaFunc(avatar_setPullTarget)
{
	if(Avatar *a = game->avatar)
	{
		Entity *e = entityOpt(L, 1);

		if (a->pullTarget != 0)
			a->pullTarget->stopPull();

		a->pullTarget = e;

		if (e)
			e->startPull();
	}

	luaReturnNil();
}

/// entity_isDead(Entity e) -> bool!
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


/// getLastCollidePosition() -> float!, float
luaFunc(getLastCollidePosition)
{
	luaReturnVec2(game->lastCollidePosition.x, game->lastCollidePosition.y);
}

/// getLastCollideTileType() -> ObsType!
luaFunc(getLastCollideTileType)
{
	luaReturnInt(game->lastCollideTileType);
}

/// collideCircleWithGrid(float x, float y, float radius) -> bool!
luaFunc(collideCircleWithGrid)
{
	bool c = game->collideCircleWithGrid(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)), lua_tonumber(L, 3));
	luaReturnBool(c);
}

/// entity_isNearGround(Entity e, int sampleArea) -> bool!
luaFunc(entity_isNearGround)
{
	Entity *e = entity(L);
	bool value = false;
	if (e)
	{
		int sampleArea = lua_tointeger(L, 2);
		Vector v = game->getWallNormal(e->position, sampleArea);
		if (!v.isZero())
		{

			if (v.y < 0 && fabsf(v.x) < 0.6f)
			{
				value = true;
			}
		}

	}
	luaReturnBool(value);
}

/// entity_isHit(Entity e) -> bool!
/// Returns whether an entity would accept damage (this is not the case when the temporary invincibility timer is on, after taking a hit).
luaFunc(entity_isHit)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
		v = e->isHit();
	luaReturnBool(v);
}

/// entity_waitForPath(Entity e)
luaFunc(entity_waitForPath)
{
	Entity *e = entity(L);
	while (e && e->isFollowingPath())
	{
		core->run(FRAME_TIME);
	}
	luaReturnNil();
}

/// quitNestedMain()
luaFunc(quitNestedMain)
{
	core->quitNestedMain();
	luaReturnNil();
}

/// isNestedMain() -> bool!
luaFunc(isNestedMain)
{
	luaReturnBool(core->isNested());
}

/// entity_watchForPath(Entity e)
luaFunc(entity_watchForPath)
{
	game->avatar->disableInput();

	Entity *e = entity(L);
	while (e && e->isFollowingPath())
	{
		core->run(FRAME_TIME);
	}

	game->avatar->enableInput();
	luaReturnNil();
}


/// watchForVoice()
luaFunc(watchForVoice)
{
	while (dsq->sound->isPlayingVoice())
	{
		dsq->watch(FRAME_TIME);
		if (dsq->isQuitFlag())
		{
			dsq->sound->stopVoice();
			break;
		}
	}
	luaReturnNil();
}


/// entity_isSlowingToStopPath(Entity e) -> bool!; deprecated
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

/// entity_resumePath(Entity e); deprecated
luaFunc(entity_resumePath)
{
	Entity *e = entity(L);
	if (e)
	{
		e->position.resumePath();
	}
	luaReturnNil();
}

/// entity_isAnimating(Entity e, int layer = 0) -> bool!
luaFunc(entity_isAnimating)
{
	Entity *e = entity(L);
	bool v= false;
	if (e)
	{
		v = e->skeletalSprite.isAnimating(lua_tointeger(L, 2));
	}
	luaReturnBool(v);
}

/// entity_getAnimationName(Entity e, int layer = 0) -> string?!
luaFunc(entity_getAnimationName)
{
	Entity *e = entity(L);
	int layer = lua_tointeger(L, 2);
	if (e)
	{
		if (Animation *anim = e->skeletalSprite.getCurrentAnimation(layer))
		{
			luaReturnStr(anim->name.c_str());
		}
	}
	luaReturnNil();
}

/// entity_getAnimationLength(Entity e, string anim) -> float!
/// entity_getAnimationLength(Entity e, int layer = 0) -> float!
/// By animation name, or the current animation on a layer; 0 if not found.
luaFunc(entity_getAnimationLength)
{
	Entity *e = entity(L);
	float ret=0;
	if (e)
	{
		Animation *anim = 0;
		if (lua_type(L, 2) == LUA_TSTRING) // lua_isstring() is incorrect here
			anim = e->skeletalSprite.getAnimation(lua_tostring(L, 2));
		else
		{
			int layer = lua_tointeger(L, 2);
			anim = e->skeletalSprite.getCurrentAnimation(layer);
		}
		if (anim)
			ret = anim->getAnimationLength();
	}
	luaReturnNum(ret);
}

/// entity_hasAnimation(Entity e, string anim) -> bool!
luaFunc(entity_hasAnimation)
{
	Entity *e = entity(L);
	Animation *anim = e ? e->skeletalSprite.getAnimation(getString(L, 2)) : NULL;
	luaReturnBool(anim != NULL);
}

/// entity_isFollowingPath(Entity e) -> bool!
luaFunc(entity_isFollowingPath)
{
	Entity *e = entity(L);
	luaReturnBool(e && e->isFollowingPath());
}

/// entity_toggleBone(Entity e, Bone b, int visible); deprecated
/// Sets the bone's alpha to visible (0/1) directly.
luaFunc(entity_toggleBone)
{
	Entity *e = entity(L);
	Bone *b = bone(L, 2);
	if (e && b)
	{
		e->skeletalSprite.toggleBone(e->skeletalSprite.getBoneIdx(b), lua_tointeger(L, 3));
	}
	luaReturnNil();
}

/// entity_setEntityType(Entity e, EntityType type)
luaFunc(entity_setEntityType)
{
	Entity *e = entity(L);
	if (e)
		e->setEntityType(EntityType(lua_tointeger(L, 2)));
	luaReturnNil();
}

/// entity_getEntityType(Entity e) -> EntityType!
luaFunc(entity_getEntityType)
{
	Entity *e = entity(L);
	luaReturnInt(e ? e->getEntityType() : 0);
}

/// cam_snap()
luaFunc(cam_snap)
{
	game->snapCam();
	luaReturnNil();
}

/// cam_toNode(Node node)
/// Does nothing if node isn't valid.
luaFunc(cam_toNode)
{
	Path *p = path(L);
	if (p)
	{
		game->setCameraFollow(&p->nodes[0].position);
	}
	luaReturnNil();
}

/// cam_toEntity(Entity|NoObj e)
/// 0 = follow nothing.
luaFunc(cam_toEntity)
{
	Entity *e = entityOpt(L);
	if(e)
		game->setCameraFollowEntity(e);
	else
		game->setCameraFollow((Vector*)NULL);
	luaReturnNil();
}

/// cam_setPosition(float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(cam_setPosition)
{
	InterpolatedVector& camvec = game->cameraInterp;
	camvec.stop();
	interpolateVec2(L, camvec, 1);

	dsq->cameraPos = game->getCameraPositionFor(camvec);
	luaReturnNil();
}

template<typename T>
void spawnParticlesFromCollisionMask(lua_State *L, T *t)
{
	const char *p = getCString(L, 2);
	if(!p)
		luaL_error(L, "expected string partcle name");
	int intv = lua_tointeger(L, 3);
	if (intv <= 0)
		intv = 1;
	int lr = luaL_optinteger(L, 4, LR_PARTICLES);
	float rotz = lua_tonumber(L, 5);

	t->spawnParticlesFromCollisionMask(p, intv, lr, rotz);
}


/// entity_spawnParticlesFromCollisionMask(Entity e, string particle, int interval = 1, Layer layer = LR_PARTICLES, float rotz = 0)
luaFunc(entity_spawnParticlesFromCollisionMask)
{
	Entity *e = entity(L);
	if (e)
		spawnParticlesFromCollisionMask(L, e);

	luaReturnNil();
}

/// bone_spawnParticlesFromCollisionMask(Bone b, string particle, int interval = 1, Layer layer = LR_PARTICLES, float rotz = 0)
luaFunc(bone_spawnParticlesFromCollisionMask)
{
	Bone *b = bone(L);
	if (b)
		spawnParticlesFromCollisionMask(L, b);

	luaReturnNil();
}
/// entity_initEmitter(ScriptedEntity e, int idx, string file)
luaFunc(entity_initEmitter)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	std::string pfile = getString(L, 3);
	if (se && e >= 0)
	{
		se->initEmitter(e, pfile);
	}
	luaReturnNil();
}

/// entity_startEmitter(ScriptedEntity e, int idx)
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

/// entity_stopEmitter(ScriptedEntity e, int idx)
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

/// entity_getEmitter(ScriptedEntity e, int idx) -> ParticleEffect|NoObj!
luaFunc(entity_getEmitter)
{
	ScriptedEntity *se = scriptedEntity(L);
	luaReturnPtr(se ? se->getEmitter(lua_tointeger(L, 2)) : NULL);
}

/// entity_getNumEmitters(ScriptedEntity e) -> int!
luaFunc(entity_getNumEmitters)
{
	ScriptedEntity *se = scriptedEntity(L);
	luaReturnInt(se ? se->getNumEmitters() : 0);
}

/// entity_initStrands(ScriptedEntity e, int num, int segs, int dist, int spacing, float r, float g, float b)
luaFunc(entity_initStrands)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		e->initStrands(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), Vector(lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8)));
	}
	luaReturnNil();
}

/// entity_initSkeletal(Entity e, string skel, string? skin)
luaFunc(entity_initSkeletal)
{
	Entity *e = entity(L);
	if (e)
	{
		e->renderQuad = false;
		e->setWidthHeight(128, 128);
		e->skeletalSprite.loadSkeletal(getString(L, 2));
		const char *s = getCString(L, 3);
		if (s && *s)
			e->skeletalSprite.loadSkin(s);
	}
	luaReturnNil();
}

/// entity_loadSkin(Entity e, string skin)
luaFunc(entity_loadSkin)
{
	Entity *e = entity(L);
	if (e && e->skeletalSprite.isLoaded())
	{
		const char *s = getCString(L, 2);
		if (s && *s)
			e->skeletalSprite.loadSkin(s);
	}
	luaReturnNil();
}

/// entity_getSkeletalName(Entity e) -> string!
/// "" if none loaded.
luaFunc(entity_getSkeletalName)
{
	Entity *e = entity(L);
	const char *s = "";
	if (e && e->skeletalSprite.isLoaded())
		s = e->skeletalSprite.filenameLoaded.c_str();
	luaReturnStr(s);
}

/// entity_hasSkeletal(Entity e) -> bool!
luaFunc(entity_hasSkeletal)
{
	Entity *e = entity(L);
	luaReturnBool(e && e->skeletalSprite.isLoaded());
}

/// entity_getNumAnimLayers(Entity e) -> int!
luaFunc(entity_getNumAnimLayers)
{
	Entity *e = entity(L);
	luaReturnInt(e ? e->skeletalSprite.getNumAnimLayers() : 0);
}

/// entity_idle(Entity e)
luaFunc(entity_idle)
{
	Entity *e = entity(L);
	if (e)
	{
		e->idle();
	}
	luaReturnNil();
}

/// entity_stopAllAnimations(Entity e)
luaFunc(entity_stopAllAnimations)
{
	Entity *e = entity(L);
	if (e)
		e->skeletalSprite.stopAllAnimations();
	luaReturnNil();
}

// LUAWTF-NOTE(weird): returns the requested multiplier (slot 3), not the previous one; returns it even when the layer doesn't exist (0 only for an invalid entity)
/// entity_setAnimLayerTimeMult(Entity e, AnimLayer layer, float mult, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false) -> float
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
			interpolateVec1(L, l->timeMultiplier, 3);
		}
	}
	luaReturnNum(t);
}

/// entity_getAnimLayerTimeMult(Entity e, AnimLayer layer = 0) -> float!
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

/// entity_animate(Entity e, string anim, int loop = 0, AnimLayer layer = 0, float transition = 0) -> float
/// transition: 0 = default 0.2 s, -1 = none. Returns the animation's length (0 on failure).
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
			transition = 0.2f;
		ret = skel->transitionAnimate(getString(L, 2), transition, lua_tointeger(L, 3), lua_tointeger(L, 4));
	}
	luaReturnNum(ret);
}

/// entity_stopAnimation(Entity e, AnimLayer layer = 0)
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

/// entity_getAnimationLoop(Entity e, AnimLayer layer = 0) -> int!
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

/// entity_getAnimationTime(Entity e, AnimLayer layer = 0) -> float?!, float?
/// Returns timer, animationLength.
luaFunc(entity_getAnimationTime)
{
	SkeletalSprite *skel = getSkeletalSprite(entity(L));
	int layer = lua_tointeger(L, 2);
	if (skel)
	{
		const AnimationLayer *a = skel->getAnimationLayer(layer);
		if(a)
		{
			luaReturnVec2(a->timer, a->animationLength);
		}
	}
	luaReturnNil();
}

/// entity_setAnimationTime(Entity e, float t, AnimLayer layer = 0, boolx runkeyframes = false)
luaFunc(entity_setAnimationTime)
{
	SkeletalSprite *skel = getSkeletalSprite(entity(L));
	int layer = lua_tointeger(L, 3);
	if (skel)
	{
		AnimationLayer *a = skel->getAnimationLayer(layer);
		if(a)
		{
			float t = lua_tonumber(L, 2);
			bool runkeyframes = getBool(L, 4);
			a->setTimer(t, runkeyframes);
		}
	}
	luaReturnNil();
}

/// entity_move(Entity e, float x, float y, float time = 0, boolx ease = false, boolx relative = false); deprecated("use entity_setPosition()")
luaFunc(entity_move)
{
	Entity *e = entity(L);
	if (e)
	{
		Vector p(lua_tonumber(L, 2), lua_tonumber(L, 3));
		if (getBool(L, 6))
			p = e->position + p;

		e->position.interpolateTo(p, lua_tonumber(L, 4), 0, 0, getBool(L, 5));
	}
	luaReturnNil();
}

/// spawnManaBall(float x, float y, float amount)
luaFunc(spawnManaBall)
{
	Vector p;
	p.x = lua_tonumber(L, 1);
	p.y = lua_tonumber(L, 2);
	float amount = lua_tonumber(L, 3);
	game->spawnManaBall(p, amount);
	luaReturnNil();
}

/// spawnAroundEntity(Entity e, int num, float radius, string type, string name = "")
luaFunc(spawnAroundEntity)
{
	Entity *e = entity(L);
	int num = lua_tointeger(L, 2);
	float radius = lua_tonumber(L, 3);
	std::string entType = getString(L, 4);
	std::string name = getString(L, 5);
	if (e)
	{
		const Vector center = e->position;
		for (int i = 0; i < num; i++)
		{
			float angle = i*((2*PI)/float(num));
			Vector spawnPos = center + Vector(sinf(angle)*radius, cosf(angle)*radius);
			Entity *spawned = game->createEntityTemp(entType.c_str(), spawnPos, true);
			if(spawned && !name.empty())
				spawned->setName(name);
		}
	}
	luaReturnNil();
}

/// createBeam(float x, float y, float angle = 0, int layer = 0) -> Beam!
/// layer: 1 = LR_PARTICLES, anything else = LR_ENTITIES_MINUS2.
luaFunc(createBeam)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float a = lua_tonumber(L, 3);
	int l = lua_tointeger(L, 4);
	Beam *b = new Beam(Vector(x,y), a);
	if (l == 1)
		game->addRenderObject(b, LR_PARTICLES);
	else
		game->addRenderObject(b, LR_ENTITIES_MINUS2);
	luaReturnPtr(b);
}

/// beam_setDamage(Beam b, float damage)
luaFunc(beam_setDamage)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setDamage(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

/// beam_setDamageType(Beam b, DamageType type)
luaFunc(beam_setDamageType)
{
	Beam *b = beam(L);
	if (b)
	{
		b->damageData.damageType = (DamageType)lua_tointeger(L, 2);
	}
	luaReturnNil();
}

/// beam_setBeamWidth(Beam b, float width)
luaFunc(beam_setBeamWidth)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setBeamWidth(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

/// beam_setAngle(Beam b, float angle)
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

/// beam_setFirer(Beam b, Entity|NoObj firer)
luaFunc(beam_setFirer)
{
	Beam *b = beam(L);
	if (b)
		b->setFirer(entityOpt(L, 2));
	luaReturnNil();
}

// Note the additional trace() call
/// beam_setPosition(Beam b, float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(beam_setPosition_override)
{
	Beam *b = beam(L);
	if (b)
	{
		forwardCall(obj_setPosition);
		b->trace(); // Note: This has to be continuously done (eg. via beam_setAngle()) if setPosition() does interpolation
	}
	luaReturnNil();
}

/// beam_getEndPos(Beam b) -> float!, float
luaFunc(beam_getEndPos)
{
	Beam *b = beam(L);
	Vector v;
	if (b)
		v = b->endPos;
	luaReturnVec2(v.x, v.y);
}

/// getStringBank(int id) -> string!
luaFunc(getStringBank)
{
	luaReturnStr(stringbank.get(lua_tointeger(L, 1)).c_str());
}

/// isPlat(int plat) -> bool!
/// plat: PLAT_WIN, PLAT_MAC, PLAT_LNX.
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

/// createEntity(string type, string name = "", int x = 0, int y = 0) -> Entity|NoObj
luaFunc(createEntity)
{
	std::string type = getString(L, 1);
	std::string name;
	if (lua_isstring(L, 2))
		name = lua_tostring(L, 2);
	int x = lua_tointeger(L, 3); // FIXME: float truncation! Fixing this may have side effects, leaving this as-is until i'm certain -- FG
	int y = lua_tointeger(L, 4);

	Entity *e = game->createEntityTemp(type.c_str(), Vector(x, y), true);
	if(e && !name.empty())
		e->setName(name);

	luaReturnPtr(e);
}

/// savePoint(Node n)
luaFunc(savePoint)
{
	Path *p = path(L);
	if (p)
	{
		Vector position = p->nodes[0].position;
		dsq->doSavePoint(position);
	}
	luaReturnNil();
}

/// saveMenu()
luaFunc(saveMenu)
{
	dsq->doSaveSlotMenu(SSM_SAVE);
	luaReturnNil();
}

/// setSceneDisplayNameInSave(string name)
luaFunc(setSceneDisplayNameInSave)
{
	game->sceneDisplayName = getString(L);
	luaReturnNil();
}

/// pause()
luaFunc(pause)
{
	game->togglePause(1);
	luaReturnNil();
}

/// unpause()
luaFunc(unpause)
{
	game->togglePause(0);
	luaReturnNil();
}

/// isPaused() -> bool!
luaFunc(isPaused)
{
	luaReturnBool(game->isPaused());
}

/// isInGameMenu() -> bool!
luaFunc(isInGameMenu)
{
	luaReturnBool(game->isInGameMenu());
}

/// isInEditor() -> bool!
luaFunc(isInEditor)
{
	luaReturnBool(game->isSceneEditorActive());
}

/// clearControlHint()
luaFunc(clearControlHint)
{
	game->clearControlHint();
	luaReturnNil();
}

/// setSceneColor(float r, float g, float b, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(setSceneColor)
{
	interpolateVec3(L, game->sceneColor3, 1);
	luaReturnNil();
}

/// getSceneColor() -> float!, float, float
luaFunc(getSceneColor)
{
	const Vector& c = game->sceneColor3;
	luaReturnVec3(c.x, c.y, c.z);
}

/// setSceneColor2(float r, float g, float b, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(setSceneColor2)
{
	interpolateVec3(L, game->sceneColor2, 1);
	luaReturnNil();
}

/// getSceneColor2() -> float!, float, float
luaFunc(getSceneColor2)
{
	const Vector& c = game->sceneColor2;
	luaReturnVec3(c.x, c.y, c.z);
}

/// setCameraLerpDelay(float delay)
luaFunc(setCameraLerpDelay)
{
	game->cameraLerpDelay = lua_tonumber(L, 1);
	luaReturnNil();
}

/// setControlHint(string text, boolx left = false, boolx right = false, boolx middle = false, float time = 0, string image = "", int songType = 0, float scale = 1)
/// scale 0 = 1.
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

	game->setControlHint(str, left, right, middle, t, s, false, songType, scale);
	luaReturnNil();
}

/// setCanChangeForm(boolx on)
luaFunc(setCanChangeForm)
{
	if(Avatar *a = game->avatar)
		a->canChangeForm = getBool(L);
	luaReturnNil();
}

/// setInvincibleOnNested(boolx on)
luaFunc(setInvincibleOnNested)
{
	game->invincibleOnNested = getBool(L);
	luaReturnNil();
}

/// setCanWarp(boolx on)
luaFunc(setCanWarp)
{
	if(Avatar *a = game->avatar)
		a->canWarp = getBool(L);
	luaReturnNil();
}

/// entity_generateCollisionMask(Entity e, float radius = 0)
/// radius: overrides the radius of the generated circle colliders; 0 = use default (TILE_SIZE/2); only works when entity has a skeleton.
luaFunc(entity_generateCollisionMask)
{
	Entity *e = entity(L);
	float radius = lua_tonumber(L, 2);
	if (e)
	{
		e->generateCollisionMask(radius); // TODO: Might want to support per-bone collide radius at some point?
	}
	luaReturnNil();
}

/// entity_damage(Entity e, Entity|NoObj|nil attacker, float damage, DamageType type = 0, float effectTime = 0, boolx noTimer = false, Shot|NoObj? shot, float hitX = 0, float hitY = 0) -> bool
luaFunc(entity_damage)
{
	Entity *e = entity(L);
	bool didDamage = false;
	if (e)
	{
		DamageData d;

		d.attacker = lua_isuserdata(L, 2) ? entity(L, 2) : NULL;
		d.damage = lua_tonumber(L, 3);
		d.damageType = (DamageType)lua_tointeger(L, 4);
		d.effectTime = lua_tonumber(L, 5);
		d.useTimer = !getBool(L, 6);
		d.shot = lua_isuserdata(L, 7) ? getShot(L, 7) : NULL;
		d.hitPos = Vector(lua_tonumber(L, 8), lua_tonumber(L, 9));
		didDamage = e->damage(d);
	}
	luaReturnBool(didDamage);
}

// must be called in init
/// entity_setEntityLayer(ScriptedEntity e, int layer)
/// Has its own layer numbering; prefer using entity_setLayer() that takes actual LR_* constants
luaFunc(entity_setEntityLayer)
{
	ScriptedEntity *e = scriptedEntity(L);
	int l = lua_tointeger(L, 2);
	if (e)
	{
		e->setEntityLayer(l);
	}
	luaReturnNil();
}

// intended to be used for setting max health and refilling it all
/// entity_setHealth(Entity e, float health)
/// Sets both health and maxHealth.
luaFunc(entity_setHealth)
{
	Entity *e = entity(L, 1);
	if (e)
		e->health = e->maxHealth = lua_tonumber(L, 2);
	luaReturnNil();
}

/// entity_setCurrentHealth(Entity e, float health)
luaFunc(entity_setCurrentHealth)
{
	Entity *e = entity(L, 1);
	if (e)
		e->health = lua_tonumber(L, 2);
	luaReturnNil();
}

/// entity_setMaxHealth(Entity e, float health)
luaFunc(entity_setMaxHealth)
{
	Entity *e = entity(L, 1);
	if (e)
		e->maxHealth = lua_tonumber(L, 2);
	luaReturnNil();
}

/// entity_changeHealth(Entity e, float delta)
luaFunc(entity_changeHealth)
{
	Entity *e = entity(L, 1);
	if (e)
		e->health += lua_tonumber(L, 2);
	luaReturnNil();
}

/// entity_heal(Entity e, float amount)
luaFunc(entity_heal)
{
	Entity *e = entity(L);
	if (e)
		e->heal(lua_tonumber(L, 2));
	luaReturnNil();
}

/// entity_revive(Entity e, float health)
luaFunc(entity_revive)
{
	Entity *e = entity(L);
	if (e)
		e->revive(lua_tonumber(L, 2));
	luaReturnNil();
}

/// screenFadeCapture()
luaFunc(screenFadeCapture)
{
	dsq->screenTransition->capture();
	luaReturnNil();
}


/// screenFadeTransition(float time)
luaFunc(screenFadeTransition)
{
	dsq->screenTransition->transition(lua_tonumber(L, 1));
	luaReturnNil();
}

/// screenFadeGo(float time)
luaFunc(screenFadeGo)
{
	dsq->screenTransition->go(lua_tonumber(L, 1));
	luaReturnNil();
}

/// isEscapeKey(int source = 0) -> bool!
/// source: 1-based input source, 0 = any.
luaFunc(isEscapeKey)
{
	int source = lua_tointeger(L, 1) - 1;
	bool isDown = game->isActing(ACTION_ESC, source);
	luaReturnBool(isDown);
}

/// isLeftMouse(int source = 0) -> bool!
/// source: 1-based input source, 0 = any (includes the mouse).
luaFunc(isLeftMouse)
{
	int source = lua_tointeger(L, 1) - 1;
	bool isDown = (source < 0 && core->mouse.buttons.left)
		|| (game->avatar && game->avatar->isActing(ACTION_PRIMARY, source));
	luaReturnBool(isDown);
}

/// isRightMouse(int source = 0) -> bool!
/// source: 1-based input source, 0 = any (includes the mouse).
luaFunc(isRightMouse)
{
	int source = lua_tointeger(L, 1) - 1;
	bool isDown = (source < 0 && core->mouse.buttons.right)
		|| (game->avatar && game->avatar->isActing(ACTION_SECONDARY, source));
	luaReturnBool(isDown);
}

/// setTimerTextAlpha(float alpha, float time = 0)
luaFunc(setTimerTextAlpha)
{
	game->setTimerTextAlpha(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// setTimerText(float time)
luaFunc(setTimerText)
{
	game->setTimerText(lua_tonumber(L, 1));
	luaReturnNil();
}

/// getWallNormal(float x, float y, int range = 5, int obs = 0) -> float!, float
/// range 0 = 5; obs: OT_* mask, 0 = OT_BLOCKING.
luaFunc(getWallNormal)
{
	float x,y;
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	int range = lua_tointeger(L, 3);
	int obs  = lua_tointeger(L, 4);
	if (range == 0)
		range = 5;
	if (!obs)
		obs = OT_BLOCKING;

	Vector n = game->getWallNormal(Vector(x, y), range, obs);

	luaReturnVec2(n.x, n.y);
}

/// incrFlag(string name, int amount = 1); deprecated
luaFunc(incrFlag)
{
	std::string f = getString(L, 1);
	int v = 1;
	if (lua_isnumber(L, 2))
		v = lua_tointeger(L, 2);
	dsq->continuity.setFlag(f, dsq->continuity.getFlag(f)+v);
	luaReturnNil();
}

/// decrFlag(string name, int amount = 1); deprecated
luaFunc(decrFlag)
{
	std::string f = getString(L, 1);
	int v = 1;
	if (lua_isnumber(L, 2))
		v = lua_tointeger(L, 2);
	dsq->continuity.setFlag(f, dsq->continuity.getFlag(f)-v);
	luaReturnNil();
}

/// setFlag(Flag id, int value)
luaFunc(setFlag)
{

	dsq->continuity.setFlag(lua_tointeger(L, 1), lua_tointeger(L, 2));
	luaReturnNil();
}

/// getFlag(Flag id) -> int!
luaFunc(getFlag)
{
	int v = 0;

	v = dsq->continuity.getFlag(lua_tointeger(L, 1));

	luaReturnNum(v);
}

/// getStringFlag(string name) -> string!
luaFunc(getStringFlag)
{
	luaReturnStr(dsq->continuity.getStringFlag(getString(L, 1)).c_str());
}

/// node_setEmitter(Node n, string name) -> ParticleEffect|NoObj
luaFunc(node_setEmitter)
{
	Path *p = path(L);
	ParticleEffect *pe = NULL;
	if(p)
	{
		p->setEmitter(getString(L, 2));
		pe = p->emitter;
	}
	luaReturnPtr(pe);
}

/// node_getEmitter(Node n) -> ParticleEffect|NoObj!
luaFunc(node_getEmitter)
{
	Path *p = path(L);
	luaReturnPtr(p ? p->emitter : NULL);
}


/// node_setActive(Node n, boolx on)
/// node_setEffectOn(Node n, boolx on); deprecated("use node_setActive")
luaFunc(node_setActive)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->active = v;
		if(p->emitter)
		{
			if(v)
				p->emitter->start();
			else
				p->emitter->stop();
		}
	}
	luaReturnNil();
}

/// node_isActive(Node n) -> bool!
/// node_isEffectOn(Node n) -> bool!; deprecated("use node_isActive")
luaFunc(node_isActive)
{
	Path *p = path(L);
	luaReturnBool(p ? p->active : false);
}

/// node_setCursorActivation(Node n, boolx on)
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

/// node_setActivationRange(Node n, float range)
luaFunc(node_setActivationRange)
{
	Path *p = path(L);
	if(p)
		p->activationRange = lua_tonumber(L, 2);
	luaReturnNil();
}

/// node_setCatchActions(Node n, boolx on)
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

/// node_isEntityInRange(Node n, Entity e, float range) -> bool!
luaFunc(node_isEntityInRange)
{
	Path *p = path(L);
	Entity *e = entity(L,2);
	float range = lua_tonumber(L, 3);
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

/// node_isEntityPast(Node n, Entity e, boolx checkY, int dir, float range) -> bool!; deprecated
/// checkY false: e within range in x and above the node (dir 0) / below (dir != 0); true: same with x and y swapped (left/right).
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
			bool checkY = getBool(L, 3);
			int dir = lua_tointeger(L, 4);
			float range = lua_tonumber(L, 5);
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

/// node_x(Node n) -> float!
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

/// node_y(Node n) -> float!
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

/// entity_isName(Entity e, string name) -> bool!
/// Case-insensitive.
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


/// entity_getName(Entity e) -> string!
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

/// node_getContent(Node n) -> string!
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

/// node_getAmount(Node n) -> float!
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

/// node_getSize(Node node) -> int!, int
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

/// node_getName(Node node) -> string!
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

/// node_getLabel(Node node) -> string!
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

/// node_getPathPosition(Node node, int idx) -> float!, float
luaFunc(node_getPathPosition)
{
	Path *p = path(L);
	int idx = lua_tointeger(L, 2);
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

/// node_getPosition(Node node) -> float!, float
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

/// node_setPosition(Node node, float x, float y)
/// Gotcha: Updates the node position permanently; ie. when the map is saved, the node position set with this function gets saved too.
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

/// node_getShape(Node node) -> PathShape!
luaFunc(node_getShape)
{
	Path *p = path(L);
	luaReturnInt(p ? p->pathShape : 0);
}


/// registerSporeDrop(float x, float y, int t)
/// In a fixed radius of 1024, calls each entity's sporesDropped() interface function with the same values passed here
luaFunc(registerSporeDrop)
{
	float x, y;
	int t=0;
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	t = lua_tointeger(L, 3);

	game->registerSporeDrop(Vector(x,y), t);

	luaReturnNil();
}

/// setStringFlag(string name, string value)
luaFunc(setStringFlag)
{
	std::string n = getString(L, 1);
	std::string v = getString(L, 2);
	dsq->continuity.setStringFlag(n, v);
	luaReturnNil();
}

/// centerText(string text)
luaFunc(centerText)
{
	dsq->centerText(getString(L, 1));
	luaReturnNil();
}

// this used to be msg(), but this is already an interface function name
/// screenMessage(string text)
luaFunc(screenMessage)
{
	dsq->screenMessage(getString(L, 1));
	luaReturnNil();
}

/// chance(int percent) -> bool!
luaFunc(chance)
{
	luaReturnBool(chance(lua_tointeger(L, 1)));
}

/// entity_handleShotCollisions(Entity e)
luaFunc(entity_handleShotCollisions)
{
	Entity *e = entity(L);
	if (e)
	{
		game->handleShotCollisions(e);
	}
	luaReturnNil();
}

/// entity_handleShotCollisionsSkeletal(Entity e)
luaFunc(entity_handleShotCollisionsSkeletal)
{
	Entity *e = entity(L);
	if (e)
	{
		game->handleShotCollisionsSkeletal(e);
	}
	luaReturnNil();
}

/// entity_handleShotCollisionsHair(Entity e, int num = 0, float perc = 0)
/// num: hair segments to test (0 = all); perc: fraction of hair width as radius (0 = 1)
luaFunc(entity_handleShotCollisionsHair)
{
	Entity *e = entity(L);
	if (e)
	{
		game->handleShotCollisionsHair(e, lua_tointeger(L, 2), lua_tonumber(L, 3));
	}
	luaReturnNil();
}

/// entity_collideSkeletalVsCircle(Entity e, CollideQuad circle) -> Bone|NoObj!
luaFunc(entity_collideSkeletalVsCircle)
{
	Entity *e = entity(L);
	CollideQuad *e2 = getCollideQuad(L,2);
	Bone *b = 0;
	if (e && e2)
	{
		b = game->collideSkeletalVsCircle(e,e2);
	}
	luaReturnPtr(b);
}

/// entity_collideSkeletalVsCirclePos(Entity e, float x, float y, float radius) -> Bone|NoObj!
luaFunc(entity_collideSkeletalVsCirclePos)
{
	Entity *e = entity(L);
	Bone *b = 0;
	if (e)
	{
		b = game->collideSkeletalVsCircle(e, Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4));
	}
	luaReturnPtr(b);
}

/// entity_collideSkeletalVsLine(Entity e, float x1, float y1, float x2, float y2, float radius) -> Bone|NoObj!
luaFunc(entity_collideSkeletalVsLine)
{
	Entity *e = entity(L);
	float x1 = lua_tonumber(L, 2);
	float y1 = lua_tonumber(L, 3);
	float x2 = lua_tonumber(L, 4);
	float y2 = lua_tonumber(L, 5);
	float sz = lua_tonumber(L, 6);
	Bone *b = 0;
	if (e)
	{
		b = game->collideSkeletalVsLine(e, Vector(x1, y1), Vector(x2, y2), sz);
	}
	luaReturnPtr(b);
}

/// entity_collideHairVsCircle(Entity e, Entity target, int num, float perc = 0) -> bool!, int?
/// num: hair segments to test (0 = all); perc: fraction of hair width (0 = 1). Second result: the colliding segment index (0-based)
luaFunc(entity_collideHairVsCircle)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	bool col=false;
	if (e && e2)
	{
		size_t num = (size_t)lua_tointeger(L, 3);
		// perc: percent of hairWidth to use as collide radius
		float perc = lua_tonumber(L, 4);
		size_t colSegment;
		col = game->collideHairVsCircle(e, num, e2->position, e2->collideRadius, perc, &colSegment);
		if(col)
		{
			lua_pushboolean(L, true);
			lua_pushinteger(L, colSegment);
			return 2;
		}
	}
	luaReturnBool(false);
}

// LUAWTF-NOTE(weird): not a query despite the name: every entity named `name` with `life == 1` that touches e's skeleton damages e (`e->damage(d)` with attacker e2); returns nothing
/// entity_collideSkeletalVsCircleForListByName(Entity e, string name); deprecated
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
				Bone *b = game->collideSkeletalVsCircle(e, e2);
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

/// entity_debugText(Entity e, string text)
luaFunc(entity_debugText)
{
	Entity *e = entity(L);
	const char *txt = lua_tostring(L, 2);
	if (e && txt)
	{
		BitmapText *f = new BitmapText(dsq->smallFont);
		f->setText(txt);
		f->position = e->position;
		core->getTopStateObject()->addRenderObject(f, LR_DEBUG_TEXT);
		f->setLife(5);
		f->setDecayRate(1);
		f->fadeAlphaWithLife=1;
	}
	luaReturnNil();
}

/// entity_getHealth(Entity e) -> float!
luaFunc(entity_getHealth)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->health : 0);
}

/// entity_getMaxHealth(Entity e) -> float!
luaFunc(entity_getMaxHealth)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->maxHealth : 0);
}

/// entity_initSegments(ScriptedEntity e, int numSegments, int minDist, int maxDist, string bodyTex, string tailTex, int w, int h, float taper, boolx reverseSegments = false)
luaFunc(entity_initSegments)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->initSegments(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), getString(L, 5), getString(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8), lua_tonumber(L, 9), getBool(L, 10));

	luaReturnNil();
}

/// entity_warpSegments(ScriptedEntity e)
luaFunc(entity_warpSegments)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->warpSegments();

	luaReturnNil()
}

// LUAWTF-NOTE(bug): dereferences `game->avatar` without a NULL check (setNaijaHeadTexture checks it); same in avatar_decrLeaches and avatar_setElementEffectMult
/// avatar_incrLeaches()
luaFunc(avatar_incrLeaches)
{
	if(Avatar *a = game->avatar)
		a->leaches++;
	luaReturnNil();
}

/// avatar_decrLeaches()
luaFunc(avatar_decrLeaches)
{
	// Not checking for > 0 here because this allows some neat tricks when it becomes negative.
	if(Avatar *a = game->avatar)
		a->leaches--;
	luaReturnNil();
}

/// entity_incrTargetLeaches(Entity e); deprecated("use avatar_incrLeaches")
luaFunc(entity_incrTargetLeaches) // DEPRECATED
{
	Entity *e = entity(L);
	if (e && e->getTargetEntity() == game->avatar)
		game->avatar->leaches++;
	luaReturnNil();
}

/// entity_decrTargetLeaches(Entity e); deprecated("use avatar_decrLeaches")
luaFunc(entity_decrTargetLeaches) // DEPRECATED
{
	Entity *e = entity(L);
	if (e && e->getTargetEntity() == game->avatar)
		game->avatar->leaches--;
	luaReturnNil();
}

/// entity_rotateToVel(Entity e, float time = 0, float offsetAngle = 0)
luaFunc(entity_rotateToVel)
{
	Entity *e = entity(L);
	if (e)
	{
		if (!e->vel.isZero())
		{
			e->rotateToVec(e->vel, lua_tonumber(L, 2), lua_tonumber(L, 3));
		}
	}
	luaReturnNil();
}

/// entity_rotateToEntity(Entity e, Entity target, float time = 0, float offsetAngle = 0)
luaFunc(entity_rotateToEntity)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);

	if (e && e2)
	{
		Vector vec = e2->position - e->position;
		if (!vec.isZero())
		{
			e->rotateToVec(vec, lua_tonumber(L, 3), lua_tonumber(L, 4));
		}
	}
	luaReturnNil();
}

/// entity_rotateToVec(Entity e, float x, float y, float time = 0, float offsetAngle = 0)
luaFunc(entity_rotateToVec)
{
	Entity *e = entity(L);
	Vector vec(lua_tonumber(L, 2), lua_tonumber(L, 3));
	if (e)
	{
		if (!vec.isZero())
		{
			e->rotateToVec(vec, lua_tonumber(L, 4), lua_tonumber(L, 5));
		}
	}
	luaReturnNil();
}

/// entity_updateSkeletal(Entity e, float dt)
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

/// entity_msg(Entity e, ...) -> Any...
/// Calls the entity script's msg(me, ...) and returns its results
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

/// node_msg(Node node, ...) -> Any...
luaFunc(node_msg)
{
	Path *p = path(L);
	if (p)
	{
		// pass everything on the stack except the entity pointer
		int res = p->messageVariadic(L, lua_gettop(L) - 1);
		if (res >= 0)
			return res;
	}
	luaReturnNil();
}

/// shot_msg(Shot s, ...) -> Any...
luaFunc(shot_msg)
{
	Shot *s = getShot(L);
	if (s)
	{
		// pass everything on the stack except the entity pointer
		int res = s->messageVariadic(L, lua_gettop(L) - 1);
		if (res >= 0)
			return res;
	}
	luaReturnNil();
}

/// entity_updateCurrents(Entity e, float dt) -> bool
luaFunc(entity_updateCurrents)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->updateCurrents(lua_tonumber(L, 2)) : false);
}

/// entity_updateLocalWarpAreas(Entity e, boolx affectAvatar = false) -> bool
luaFunc(entity_updateLocalWarpAreas)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->updateLocalWarpAreas(getBool(L, 2)) : false);
}

/// entity_updateMovement(ScriptedEntity e, float dt)
luaFunc(entity_updateMovement)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
		e->updateMovement(lua_tonumber(L, 2));
	luaReturnNil();
}

/// entity_applySurfaceNormalForce(Entity e, float force)
/// Launch off a surface. When attached to a rock, jump off.
luaFunc(entity_applySurfaceNormalForce)
{
	Entity *e = entity(L);
	if (e)
	{
		Vector v;
		if (!e->ridingOnEntity)
		{
			v = game->getWallNormal(e->position, 8);
		}
		else
		{
			v = e->position - e->ridingOnEntity->position;
			e->ridingOnEntity = 0;
		}
		v.setLength2D(lua_tonumber(L, 2));
		e->vel += v;
	}
	luaReturnNil();
}

/// entity_doElementInteraction(Entity e, float mult, float touchWidth = 16)
luaFunc(entity_doElementInteraction)
{
	Entity *e = entity(L);
	if (e)
	{
		float mult = lua_tonumber(L, 2);
		float touchWidth = lua_tonumber(L, 3);
		if (!touchWidth)
			touchWidth = 16;

		dsq->tilemgr.doTileInteraction(e->position, e->vel, mult, touchWidth);
	}
	luaReturnNil();
}

/// avatar_setElementEffectMult(float mult)
luaFunc(avatar_setElementEffectMult)
{
	if(Avatar *a = game->avatar)
		a->elementEffectMult = lua_tonumber(L, 1);
	luaReturnNil();
}

/// flingMonkey(Entity e)
luaFunc(flingMonkey)
{
	Entity *e = entity(L);
	if (e)
		dsq->continuity.flingMonkey(e);

	luaReturnNil();
}

/// entity_getDistanceToTarget(Entity e, int targetIndex = 0) -> float!
luaFunc(entity_getDistanceToTarget)
{
	Entity *e = entity(L);
	float dist = 0;
	if (e)
	{
		Entity *t = e->getTargetEntity(lua_tointeger(L, 2));
		if (t)
		{
			dist = (t->position - e->position).getLength2D();
		}
	}
	luaReturnNum(dist);
}

/// entity_getDistanceToPoint(Entity e, float x, float y) -> float!
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

/// setNaijaHeadTexture(string name, float time = 0)
luaFunc(setNaijaHeadTexture)
{
	Avatar *a = game->avatar;
	if (a)
	{
		a->setHeadTexture(getString(L, 1), lua_tonumber(L, 2));
	}
	luaReturnNil();
}

/// entity_flipToSame(Entity e, Entity other)
luaFunc(entity_flipToSame)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	if (e && e2)
	{
		if (e->isfh() != e2->isfh())
		{
			e->flipHorizontal();
		}
	}
	luaReturnNil();
}

/// entity_flipToEntity(Entity e, Entity target)
luaFunc(entity_flipToEntity)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	if (e && e2)
	{
		e->flipToPos(e2->position);
	}
	luaReturnNil();
}

/// entity_flipToNode(Entity e, Node node)
luaFunc(entity_flipToNode)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->flipToPos(p->nodes[0].position);
	}
	luaReturnNil();
}

/// entity_flipToVel(Entity e)
luaFunc(entity_flipToVel)
{
	Entity *e = entity(L);
	if (e)
	{
		e->flipToVel();
	}
	luaReturnNil();
}

/// node_isEntityIn(Node node, Entity e) -> bool!
luaFunc(node_isEntityIn)
{
	Path *p = path(L,1);
	Entity *e = entity(L,2);
	luaReturnBool(e && p && p->isCoordinateInside(e->position));
}

/// node_isPositionIn(Node node, float x, float y, float rad = 0) -> bool!
luaFunc(node_isPositionIn)
{
	Path *p = path(L,1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	float rad = lua_tonumber(L, 4);
	luaReturnBool(p && p->isCoordinateInside(Vector(x,y), rad));
}

/// entity_isInDarkness(Entity e) -> bool!
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

/// entity_isInRect(Entity e, float x1, float y1, float x2, float y2) -> bool!
luaFunc(entity_isInRect)
{
	Entity *e = entity(L);
	bool v= false;
	float x1, y1, x2, y2;
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

static void setupQuadCommon(lua_State *L, RenderObject *q, int idx)
{
	q->setTexture(getString(L, idx));
	// HACK: Super annoying layer fuckery but all scripts expect 13 == dark layer
	int layer = lua_tointeger(L, idx + 1);
	if (layer == 13)
		layer = 13;
	else
		layer = (LR_PARTICLES+1) - LR_ELEMENTS1;
	game->addRenderObject(q, LR_ELEMENTS1+(layer-1));
	q->moveToFront();
}

/// createQuad(string tex = "", int layer = 0) -> PauseQuad
/// layer: 13 = LR_ELEMENTS13 (dark layer), anything else = LR_PARTICLES. Use quad_setLayer() later to set a proper layer later if needed.
luaFunc(createQuad)
{
	PauseQuad *q = new PauseQuad();
	setupQuadCommon(L, q, 1);
	luaReturnPtr(q);
}

/// quad_setPauseLevel(PauseQuad q, int level)
luaFunc(quad_setPauseLevel)
{
	Quad *q = getQuad(L);
	ENSURE_TYPE(q, SCO_PAUSEQUAD);
	if (q)
		((PauseQuad*)q)->pauseLevel = lua_tointeger(L, 2);
	luaReturnNil();
}

/// setupEntity(ScriptedEntity e, string tex = "", int layer = 0)
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
		se->setupEntity(tex, lua_tointeger(L, 3));
	}
	luaReturnNil();
}

/// setupBasicEntity(ScriptedEntity e, string tex, float health, float manaBall, int exp_unused, int money_unused, float collideRadius, State state, float w, float h, int expType_unused = 0, boolx hitEntity_unused = false, float updateCull_unused = 0, int layer = 0)
/// Semi-deprecated but widely used; consider using setupEntity() instead. Lots of unused parameters, beware!
/// layer is the same numbering used by entity_setEntityLayer(), so not the actual LR_* values!
luaFunc(setupBasicEntity)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		std::string tex = getString(L, 2);
		float health = lua_tonumber(L, 3);
		float manaBall = lua_tonumber(L, 4);
		int exp = lua_tointeger(L, 5); // unused
		int money = lua_tointeger(L, 6); // unused
		float collideRadius = lua_tonumber(L, 7);
		int state = lua_tointeger(L, 8);
		float w = lua_tonumber(L, 9);
		float h = lua_tonumber(L, 10);
		int expType = lua_tointeger(L, 11); // unused
		bool hitEntity = getBool(L, 12); // unused
		float updateCull = lua_tonumber(L, 13); // unused
		int layer = lua_tointeger(L, 14);

		se->setupBasicEntity(tex, health, manaBall, exp, money, collideRadius, state, w, h, expType, hitEntity, updateCull, layer);
	}
	luaReturnNil();
}

/// entity_setBeautyFlip(Entity e, boolx on)
luaFunc(entity_setBeautyFlip)
{
	Entity *e = entity(L);
	if (e)
	{
		e->beautyFlip = getBool(L, 2);
	}
	luaReturnNil();
}

/// setInvincible(boolx on) -> bool
luaFunc(setInvincible)
{
	game->invinciblity = getBool(L, 1);

	luaReturnBool(game->invinciblity);
}

/// entity_setInvincible(Entity e, boolx on)
luaFunc(entity_setInvincible)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setInvincible(getBool(L, 2));

	}
	luaReturnNil();
}

/// entity_setDeathSound(Entity e, string sound)
luaFunc(entity_setDeathSound)
{
	Entity *e = entity(L);
	if (e)
	{
		e->deathSound = getString(L, 2);
	}
	luaReturnNil();
}

/// entity_setDeathParticleEffect(ScriptedEntity e, string name)
luaFunc(entity_setDeathParticleEffect)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->deathParticleEffect = getString(L, 2);
	}
	luaReturnNil();
}

/// entity_setNaijaReaction(Entity e, string reaction = "")
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

/// entity_setName(Entity e, string name)
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

/// entity_pathBurst(Entity e, boolx wallJump = false) -> bool
luaFunc(entity_pathBurst)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
		v = e->pathBurst(getBool(L, 2));
	luaReturnBool(v);
}

/// entity_moveTowardsAngle(Entity e, float angle, float dt, float speed)
luaFunc(entity_moveTowardsAngle)
{
	Entity *e = entity(L);
	if (e)
		e->moveTowardsAngle(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	luaReturnNil();
}

/// entity_moveAroundAngle(Entity e, float angle, float dt, float speed, int dir); deprecated
luaFunc(entity_moveAroundAngle)
{
	Entity *e = entity(L);
	if (e)
		e->moveAroundAngle(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tointeger(L, 5));
	luaReturnNil();
}

/// entity_moveTowards(Entity e, float x, float y, float dt, float speed)
luaFunc(entity_moveTowards)
{
	Entity *e = entity(L);
	if (e)
		e->moveTowards(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5));
	luaReturnNil();
}

/// entity_moveAround(Entity e, float x, float y, float dt, float speed, int dir)
luaFunc(entity_moveAround)
{
	Entity *e = entity(L);
	if (e)
		e->moveAround(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tointeger(L, 6));
	luaReturnNil();
}

/// entity_addVel2(Entity e, float x, float y)
luaFunc(entity_addVel2)
{
	Entity *e = entity(L);
	if (e)
		e->vel2 += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

/// entity_setVel2(Entity e, float x, float y)
luaFunc(entity_setVel2)
{
	Entity *e = entity(L);
	if (e)
	{
		e->vel2 = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnNil();
}

/// entity_getVel2Len(Entity e) -> float!
luaFunc(entity_getVel2Len)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->vel2.getLength2D() : 0.0f);
}

/// entity_setVel2Len(Entity e, float len)
luaFunc(entity_setVel2Len)
{
	Entity *e = entity(L);
	if(e)
		e->vel2.setLength2D(lua_tonumber(L, 2));
	luaReturnNil();
}

/// entity_getVel2(Entity e) -> float!, float
luaFunc(entity_getVel2)
{
	Entity *e = entity(L);
	Vector vel2;
	if(e)
		vel2 = e->vel2;
	luaReturnVec2(vel2.x, vel2.y);
}

/// entity_isValidTarget(Entity e) -> bool!
luaFunc(entity_isValidTarget)
{
	luaReturnBool(game->isValidTarget(entity(L)));
}

/// entity_clearVel2(Entity e)
luaFunc(entity_clearVel2)
{
	Entity *e = entity(L);
	if (e)
		e->vel2 = Vector(0,0,0);
	luaReturnNil();
}

/// getScreenCenter() -> float!, float
luaFunc(getScreenCenter)
{
	luaReturnVec2(core->screenCenter.x, core->screenCenter.y);
}

/// entity_isState(Entity e, State state) -> bool!
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

/// entity_getState(Entity e) -> State!
luaFunc(entity_getState)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getState();
	luaReturnNum(state);
}

/// entity_getEnqueuedState(Entity e) -> State!
luaFunc(entity_getEnqueuedState)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getEnqueuedState();
	luaReturnNum(state);
}

/// entity_getPrevState(Entity e) -> State!
luaFunc(entity_getPrevState)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getPrevState();
	luaReturnNum(state);
}

/// entity_setTarget(Entity e, Entity|NoObj target)
luaFunc(entity_setTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setTargetEntity(entityOpt(L, 2));
	}
	luaReturnNil();
}

/// entity_setBounce(CollideEntity e, float bounce)
luaFunc(entity_setBounce)
{
	CollideEntity *e = collideEntity(L);
	if (e)
		e->bounceAmount = lua_tonumber(L, 2);
	luaReturnNil();
}

/// avatar_isSinging() -> bool!
luaFunc(avatar_isSinging)
{
	bool b = game->avatar->isSinging();
	luaReturnBool(b);
}

/// avatar_isTouchHit() -> bool!
luaFunc(avatar_isTouchHit)
{
	bool b = !(game->avatar->bursting && dsq->continuity.form == FORM_BEAST);
	luaReturnBool(b);
}

/// avatar_clampPosition()
luaFunc(avatar_clampPosition)
{
	game->avatar->clampPosition();
	luaReturnNil();
}

/// entity_setMaxSpeed(Entity e, float speed)
luaFunc(entity_setMaxSpeed)
{
	Entity *e = entity(L);
	if (e)
		e->setMaxSpeed(lua_tonumber(L, 2));

	luaReturnNil();
}

/// entity_getMaxSpeed(Entity e) -> float!
luaFunc(entity_getMaxSpeed)
{
	Entity *e = entity(L);
	float v = 0;
	if (e)
		v = e->getMaxSpeed();

	luaReturnNum(v);
}

/// entity_setMaxSpeedLerp(Entity e, float lerp, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(entity_setMaxSpeedLerp)
{
	Entity *e = entity(L);
	if (e)
		interpolateVec1(L, e->maxSpeedLerp, 2);

	luaReturnNil();
}

/// entity_getMaxSpeedLerp(Entity e) -> float!
luaFunc(entity_getMaxSpeedLerp)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->maxSpeedLerp.x : 0.0f);
}

// note: this is a weaker setState than perform
// this is so that things can override it
// for example getting PUSH-ed (Force) or FROZEN (bubbled)
/// entity_setState(Entity e, State state, float time = 0, boolx force = false)
/// time 0 = no time limit
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

/// entity_getBoneByIdx(Entity e, int idx) -> Bone|NoObj!
luaFunc(entity_getBoneByIdx)
{
	Entity *e = entity(L);
	Bone *b = 0;
	if (e)
	{
		int n = 0;
		if (lua_isnumber(L, 2))
		{
			n = lua_tointeger(L, 2);
			b = e->skeletalSprite.getBoneByIdx(n);
		}
	}
	luaReturnPtr(b);
}

/// entity_getBoneByName(Entity e, string name) -> Bone|NoObj!
luaFunc(entity_getBoneByName)
{
	Entity *e = entity(L);
	luaReturnPtr(e ? e->skeletalSprite.getBoneByName(getString(L, 2)) : NULL);
}

/// entity_getBoneByInternalId(Entity e, int id) -> Bone|NoObj!
luaFunc(entity_getBoneByInternalId)
{
	Entity *e = entity(L);
	size_t i = lua_tointeger(L, 2);
	luaReturnPtr((e && i < e->skeletalSprite.bones.size()) ? e->skeletalSprite.bones[i] : NULL);
}

/// entity_getNumBones(Entity e) -> int!
luaFunc(entity_getNumBones)
{
	Entity *e = entity(L);
	luaReturnInt(e ? (int)e->skeletalSprite.bones.size() : 0);
}

/// bone_getIndex(Bone b) -> int!
luaFunc(bone_getIndex)
{
	Bone *b = bone(L);
	int idx = -1;
	if (b)
		idx = (int)b->boneIdx;
	luaReturnInt(idx);
}

/// bone_getName(Bone b) -> string!
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

/// bone_getCollisionMaskNormal(Bone b, float x, float y, float dist) -> float!, float
luaFunc(bone_getCollisionMaskNormal)
{
	Vector n;
	Bone *b = bone(L);
	if(b)
		n = b->getCollisionMaskNormal(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4));
	luaReturnVec2(n.x, n.y);
}

/// bone_isName(Bone b, string name) -> bool!
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

/// overrideZoom(float zoom, float time = 0)
/// zoom 0 = stop overriding
luaFunc(overrideZoom)
{
	game->overrideZoom(lua_tonumber(L, 1), lua_tonumber(L, 2));

	luaReturnNil();
}

/// getZoom() -> float!
luaFunc(getZoom)
{
	luaReturnNum(dsq->globalScale.x);
}

/// disableOverrideZoom()
luaFunc(disableOverrideZoom)
{
	game->toggleOverrideZoom(false);
	luaReturnNil();
}

/// setMaxLookDistance(float dist)
luaFunc(setMaxLookDistance)
{
	game->maxLookDistance = lua_tonumber(L, 1);
	luaReturnNil();
}


// dt, range, mod
/// entity_doSpellAvoidance(Entity e, float dt, float range, float mod)
luaFunc(entity_doSpellAvoidance)
{
	Entity *e = entity(L);
	if (e)
		e->doSpellAvoidance(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	luaReturnNil();
}

// dt, range, mod, ignore
/// entity_doEntityAvoidance(Entity e, float dt, float range, float mod)
/// ignores the entity's current target
luaFunc(entity_doEntityAvoidance)
{
	Entity *e = entity(L);
	if (e)
		e->doEntityAvoidance(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), e->getTargetEntity());
	luaReturnNil();
}

/// entity_doCollisionAvoidance(Entity e, float dt, int search, float mod, float overrideMaxSpeed = 0, boolx useVel2 = false, boolx onlyVP = false, int ignoreObs = 0) -> bool
luaFunc(entity_doCollisionAvoidance)
{
	Entity *e = entity(L);
	bool ret = false;

	bool useVel2 = getBool(L, 6);
	bool onlyVP = getBool(L, 7);
	int ignoreObs = lua_tointeger(L, 8);

	if (e)
	{
		if (useVel2)
			ret = e->doCollisionAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), &e->vel2, lua_tonumber(L, 5), ignoreObs, onlyVP);
		else
			ret = e->doCollisionAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), 0, lua_tonumber(L, 5), ignoreObs);
	}
	luaReturnBool(ret);
}

/// setOverrideMusic(string name)
luaFunc(setOverrideMusic)
{
	game->overrideMusic = getString(L, 1);
	luaReturnNil();
}

/// setOverrideVoiceFader(float v)
/// -1 = disable override, 0 = mute music/sfx, 1 = normal volume
luaFunc(setOverrideVoiceFader)
{
	dsq->sound->setOverrideVoiceFader(lua_tonumber(L, 1));
	luaReturnNil();
}

/// setGameSpeed(float speed, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(setGameSpeed)
{
	dsq->gameSpeed.stop();
	dsq->gameSpeed.stopPath();
	interpolateVec1(L, dsq->gameSpeed, 1);
	luaReturnNil();
}

/// bedEffects(float x = 0, float y = 0); deprecated
/// If pos == (0,0), use Naija's position. Runs for 1+6 seconds, then reloads the current map.
luaFunc(bedEffects)
{
	dsq->overlay->alpha.interpolateTo(1, 2);
	dsq->sound->fadeMusic(SFT_OUT, 1);
	core->run(1);
	// music goes here
	dsq->sound->fadeMusic(SFT_CROSS, 1);
	dsq->sound->playMusic("Sleep");
	core->run(6);
	Vector bedPosition(lua_tonumber(L, 1), lua_tonumber(L, 2));
	if (bedPosition.x == 0 && bedPosition.y == 0)
	{
		bedPosition = game->avatar->position;
	}
	game->positionToAvatar = bedPosition;
	game->transitionToScene(game->sceneName);

	luaReturnNil();
}

/// entity_setDeathScene(Entity e, boolx on)
luaFunc(entity_setDeathScene)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDeathScene(getBool(L, 2));
	}
	luaReturnNil();
}

/// entity_isDeathScene(Entity e) -> bool!
luaFunc(entity_isDeathScene)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->isDeathScene() : false);
}

/// entity_setCurrentTarget(Entity e, int idx)
luaFunc(entity_setCurrentTarget)
{
	Entity *e = entity(L);
	if (e)
		e->currentEntityTarget = lua_tointeger(L, 2);
	luaReturnNil();
}

/// toggleInput(boolx on)
luaFunc(toggleInput)
{
	bool v = getBool(L, 1);
	if (v)
		game->avatar->enableInput();
	else
		game->avatar->disableInput();
	luaReturnNil();
}

/// warpAvatar(string scene, float x, float y)
luaFunc(warpAvatar)
{
	game->positionToAvatar = Vector(lua_tonumber(L, 2),lua_tonumber(L, 3));
	game->transitionToScene(getString(L, 1));

	luaReturnNil();
}

/// warpNaijaToSceneNode(string scene, string node, string? flip)
/// flip "l" or "r" (case-insensitive)
luaFunc(warpNaijaToSceneNode)
{
	std::string scene = getString(L, 1);
	std::string node = getString(L, 2);
	std::string flip = getString(L, 3);
	if (!scene.empty() && !node.empty())
	{
		game->toNode = node;
		stringToLower(flip);
		if (flip == "l")
			game->toFlip = 0;
		if (flip == "r")
			game->toFlip = 1;
		game->transitionToScene(scene);
	}

	luaReturnNil();
}

/// entity_setDamageTarget(Entity e, DamageType dt, boolx on)
luaFunc(entity_setDamageTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDamageTarget((DamageType)lua_tointeger(L, 2), getBool(L, 3));
	}
	luaReturnNil();
}

/// entity_setAllDamageTargets(Entity e, boolx on)
luaFunc(entity_setAllDamageTargets)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setAllDamageTargets(getBool(L, 2));
	}
	luaReturnNil();
}

/// entity_isDamageTarget(Entity e, DamageType dt) -> bool!
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

/// entity_setTargetRange(Entity e, float range)
luaFunc(entity_setTargetRange)
{
	Entity *e = entity(L);
	if (e)
	{
		e->targetRange = lua_tonumber(L, 2);
	}
	luaReturnNil();
}

/// entity_getTargetRange(Entity e) -> float!
luaFunc(entity_getTargetRange)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->getTargetRange() : 0.0f);
}

/// entity_clearTargetPoints(Entity e)
luaFunc(entity_clearTargetPoints)
{
	Entity *e = entity(L);
	if (e)
		e->clearTargetPoints();
	luaReturnNil();
}

/// entity_addTargetPoint(Entity e, float x, float y)
luaFunc(entity_addTargetPoint)
{
	Entity *e = entity(L);
	if (e)
		e->addTargetPoint(Vector(lua_tonumber(L,2), lua_tonumber(L, 3)));
	luaReturnNil();
}

/// entity_getTargetPoint(Entity e, int idx) -> float!, float
/// idx is 0-based; out of range gives the default energy shot target position
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

/// entity_getRandomTargetPoint(Entity e) -> int!
/// a 0-based index (0 if there are none)
luaFunc(entity_getRandomTargetPoint)
{
	Entity *e = entity(L);
	Vector v;
	int idx = 0;
	if (e)
	{
		idx = e->getRandomTargetPoint();
	}
	luaReturnInt(idx);
}

/// entity_getNumTargetPoints(Entity e) -> int!
luaFunc(entity_getNumTargetPoints)
{
	Entity *e = entity(L);
	luaReturnInt(e ? e->getNumTargetPoints() : 0);
}

/// playVisualEffect(Vfx vfx, float x, float y, Entity? target)
luaFunc(playVisualEffect)
{
	Entity *target = entityOpt(L, 4);
	dsq->playVisualEffect(lua_tonumber(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), target);
	luaReturnNil();
}

/// playNoEffect()
luaFunc(playNoEffect)
{
	dsq->playNoEffect();
	luaReturnNil();
}

/// createShockEffect(float x, float y, float origX, float origY, float amplitude, float amplitudeDecay, float frequency, float waveLength, float timeMult)
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

/// emote(Emote emote)
luaFunc(emote)
{
	int emote = lua_tointeger(L, 1);
	dsq->emote.playSfx(emote);
	luaReturnNil();
}

/// playSfx(string name, float freq = 0, float vol = 0, int loops = 0, float? x, float? y, float maxdist = 0, boolx? relative) -> lightuserdata|NoObj
/// freq/vol <= 0 = 1; maxdist 0 = default, -1 = no attenuation; relative defaults to (x, y) == (0, 0)
luaFunc(playSfx)
{
	PlaySfx sfx;
	sfx.name = getString(L, 1);
	sfx.freq = lua_tonumber(L, 2);
	sfx.vol = lua_tonumber(L, 3);
	if (sfx.vol <= 0)
		sfx.vol = 1;
	sfx.loops = lua_tointeger(L, 4);
	if(lua_isnumber(L, 5) && lua_isnumber(L, 6))
	{
		sfx.x = lua_tonumber(L, 5);
		sfx.y =  lua_tonumber(L, 6);
		sfx.positional = true;
	}
	sfx.maxdist = lua_tonumber(L, 7);
	if(lua_isnoneornil(L, 8))
		sfx.relative = (sfx.x == 0 && sfx.y == 0);
	else
		sfx.relative = getBool(L, 8);

	void *handle = NULL;

	if (!dsq->isSkippingCutscene())
	{
		handle = core->sound->playSfx(sfx);
	}

	luaReturnPtr(handle);
}

/// fadeSfx(lightuserdata|NoObj handle, float time = 0.8) -> lightuserdata|NoObj
luaFunc(fadeSfx)
{
	void *header = lua_touserdata(L, 1);
	float ft = lua_isnoneornil(L, 2) ? 0.8f : lua_tonumber(L, 2);

	core->sound->fadeSfx(header, SFT_OUT, ft);

	luaReturnPtr(header);
}

/// resetTimer()
luaFunc(resetTimer)
{
	dsq->resetTimer();
	luaReturnNil();
}

/// stopMusic()
luaFunc(stopMusic)
{
	dsq->sound->stopMusic();
	luaReturnNil();
}

/// playMusic(string name)
luaFunc(playMusic)
{
	float crossfadeTime = 0.8f;
	dsq->sound->playMusic(getString(L, 1), SLT_LOOP, SFT_CROSS, crossfadeTime);
	luaReturnNil();
}

/// playMusicStraight(string name)
luaFunc(playMusicStraight)
{
	dsq->sound->setMusicFader(1,0);
	dsq->sound->playMusic(getString(L, 1), SLT_LOOP, SFT_IN, 0.5);
	luaReturnNil();
}

/// playMusicOnce(string name)
luaFunc(playMusicOnce)
{
	float crossfadeTime = 0.8f;
	dsq->sound->playMusic(getString(L, 1), SLT_NONE, SFT_CROSS, crossfadeTime);
	luaReturnNil();
}

/// addInfluence(float x, float y, float size, float spd, boolx pull = false)
luaFunc(addInfluence)
{
	ParticleInfluence pinf;
	pinf.pos.x = lua_tonumber(L, 1);
	pinf.pos.y = lua_tonumber(L, 2);
	pinf.size = lua_tonumber(L, 3);
	pinf.spd = lua_tonumber(L, 4);
	pinf.pull = getBool(L, 5);
	dsq->particleManager->addInfluence(pinf);
	luaReturnNil();
}

/// updateMusic()
luaFunc(updateMusic)
{
	game->updateMusic();
	luaReturnNil();
}

/// entity_clampToHit(Entity e)
luaFunc(entity_clampToHit)
{
	Entity *e = entity(L);
	if (e)
		e->clampToHit();
	luaReturnNil();
}

/// entity_clampToSurface(Entity e, int tcheck = 0) -> bool
luaFunc(entity_clampToSurface)
{
	Entity *e = entity(L);
	bool ret = e && e->clampToSurface(lua_tointeger(L, 2));

	luaReturnBool(ret);
}

/// entity_checkSurface(Entity e, int tcheck, State state, float stateTime) -> bool
/// on a surface: clamps to it and sets the state
luaFunc(entity_checkSurface)
{
	Entity *e = entity(L);
	bool c = false;

	if (e)
	{
		c = e->checkSurface(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4));
	}

	luaReturnBool(c);
}

/// entity_switchSurfaceDirection(ScriptedEntity e, int? dir)
/// dir 0 or 1; left out/-1 = toggle
luaFunc(entity_switchSurfaceDirection)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (!e)
		luaReturnNil();

	int dir = -1;
	if (lua_isnumber(L, 2))
	{
		dir = lua_tointeger(L, 2);
	}

	if (e->isv(EV_SWITCHCLAMP, 1))
	{
		Vector oldPos = e->position;
		if (e->isNearObstruction(0))
		{
			Vector n = game->getWallNormal(e->position);
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

	if (dir == -1)
	{
		if (e->surfaceMoveDir)
			e->surfaceMoveDir = 0;
		else
			e->surfaceMoveDir = 1;
	}
	else
	{
		e->surfaceMoveDir = dir;
	}

	luaReturnNil();
}

/// entity_adjustPositionBySurfaceNormal(ScriptedEntity e, float dist)
luaFunc(entity_adjustPositionBySurfaceNormal)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e && !e->ridingOnEntity)
	{
		Vector v = game->getWallNormal(e->position);
		if (v.x != 0 || v.y != 0)
		{
			v.setLength2D(lua_tonumber(L, 2));
			e->position += v;
		}
		e->setv(EV_CRAWLING, 0);

	}
	luaReturnNil();
}

// HACK: move functionality inside entity class
/// entity_moveAlongSurface(ScriptedEntity e, float dt, float speed)
luaFunc(entity_moveAlongSurface)
{
	ScriptedEntity *e = scriptedEntity(L);

	if (e && e->isv(EV_CLAMPING,0))
	{
		e->lastPosition = e->position;


		{



			Vector v;
			if (e->ridingOnEntity)
			{
				v = (e->position - e->ridingOnEntity->position);
				v.normalize2D();
			}
			else
				v = game->getWallNormal(e->position);

			int outFromWall = e->getv(EV_WALLOUT);
			bool invisibleIn = e->isSittingOnInvisibleIn();



			if (invisibleIn)
				outFromWall -= TILE_SIZE;
			float t = 0.1f;
			e->offset.interpolateTo(v*outFromWall, t);



			float dt = lua_tonumber(L, 2);
			float speed = lua_tonumber(L, 3);

			Vector mov;
			if (e->surfaceMoveDir==1)
				mov = Vector(v.y, -v.x);
			else
				mov = Vector(-v.y, v.x);
			e->position += mov * speed * dt;

			if (e->ridingOnEntity)
				e->ridingOnEntityOffset = e->position - e->ridingOnEntity->position;

			e->vel = 0;



		}
	}

	luaReturnNil();
}

/// entity_rotateToSurfaceNormal(Entity e, float time = 0, int n = 0, int rot = 0)
luaFunc(entity_rotateToSurfaceNormal)
{

	Entity *e = entity(L);
	float t = lua_tonumber(L, 2);
	int n = lua_tointeger(L, 3);
	float rot = lua_tonumber(L, 4);
	if (e)
	{
		e->rotateToSurfaceNormal(t, n, rot);
	}


	luaReturnNil();
}

/// esetv(Entity e, EntityValue ev, int n) -> int
luaFunc(esetv)
{
	Entity *e = entity(L);
	size_t ev = lua_tointeger(L, 2);
	int n = lua_tointeger(L, 3);
	if (e && ev < EV_MAX)
		e->setv((EV)ev, n);
	luaReturnInt(n);
}

/// egetv(Entity e, EntityValue ev) -> int!
luaFunc(egetv)
{
	Entity *e = entity(L);
	size_t ev = lua_tointeger(L, 2);
	int v = 0;
	if (e && ev < EV_MAX)
		v = e->getv((EV)ev);
	luaReturnInt(v);
}

/// esetvf(Entity e, EntityValue ev, float n) -> float
luaFunc(esetvf)
{
	Entity *e = entity(L);
	size_t ev = lua_tointeger(L, 2);
	float n = lua_tonumber(L, 3);
	if (e  && ev < EV_MAX)
		e->setvf((EV)ev, n);
	luaReturnNum(n);
}

/// egetvf(Entity e, EntityValue ev) -> float!
luaFunc(egetvf)
{
	Entity *e = entity(L);
	size_t ev = lua_tointeger(L, 2);
	float vf = 0;
	if (e && ev < EV_MAX)
		vf = e->getvf((EV)ev);
	luaReturnNum(vf);
}

/// eisv(Entity e, EntityValue ev, int n) -> bool!
luaFunc(eisv)
{
	Entity *e = entity(L);
	size_t ev = lua_tointeger(L, 2);
	int n = lua_tointeger(L, 3);
	bool b = e && ev < EV_MAX && e->isv((EV)ev, n);
	luaReturnBool(b);
}

/// vector_normalize(float x, float y) -> float!, float
luaFunc(vector_normalize)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.normalize2D();
	luaReturnVec2(v.x, v.y);
}

/// vector_getLength(float x, float y) -> float!
luaFunc(vector_getLength)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	float len = v.getLength2D();
	luaReturnNum(len);
}

/// vector_setLength(float x, float y, float len) -> float!, float
luaFunc(vector_setLength)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.setLength2D(lua_tonumber(L, 3));
	luaReturnVec2(v.x, v.y);
}

/// vector_dot(float x, float y, float x2, float y2) -> float!
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

/// vector_cap(float x, float y, float maxLen) -> float!, float
luaFunc(vector_cap)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.capLength2D(lua_tonumber(L, 3));
	luaReturnVec2(v.x, v.y);
}

/// vector_isLength2DIn(float x, float y, float len) -> bool!
luaFunc(vector_isLength2DIn)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	bool ret = v.isLength2DIn(lua_tonumber(L, 3));
	luaReturnBool(ret);
}

/// entity_push(Entity e, float x, float y, float time, float maxSpeed = 0, float dmg = 0)
luaFunc(entity_push)
{
	Entity *e = entity(L);
	if (e)
	{
		e->push(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	luaReturnNil();
}

/// entity_pushTarget(Entity e, float len)
luaFunc(entity_pushTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		Entity *target = e->getTargetEntity();
		if (target)
		{
			Vector diff = target->position - e->position;
			diff.setLength2D(lua_tonumber(L, 2));
			target->vel += diff;
		}
	}

	luaReturnNil();
}

/// entity_getPushDamage(Entity e) -> float!
luaFunc(entity_getPushDamage)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->getPushDamage() : 0.0f);
}

/// watch(float t)
luaFunc(watch)
{
	float t = lua_tonumber(L, 1);
	dsq->watch(t);
	luaReturnNil();
}

/// wait(float t)
luaFunc(wait)
{
	dsq->run(lua_tonumber(L, 1)); // run() with recursion checking
	luaReturnNil();
}

/// warpNaijaToEntity(string name, float ox = 0, float oy = 0); deprecated
luaFunc(warpNaijaToEntity)
{
	Entity *e = dsq->getEntityByName(getString(L, 1));
	if (e)
	{
		dsq->overlay->alpha.interpolateTo(1, 1);
		core->run(1);

		Vector offset(lua_tonumber(L, 2), lua_tonumber(L, 3));
		game->avatar->position = e->position + offset;

		dsq->overlay->alpha.interpolateTo(0, 1);
		core->run(1);
	}
	luaReturnNil();
}

/// getTimer(float mod = 1) -> float!
luaFunc(getTimer)
{
	float n = lua_tonumber(L, 1);
	if (n == 0)
		n = 1;
	luaReturnNum(game->getTimer(n));
}

/// getHalfTimer(float mod = 1) -> float!
luaFunc(getHalfTimer)
{
	float n = lua_tonumber(L, 1);
	if (n == 0)
		n = 1;
	luaReturnNum(game->getHalfTimer(n));
}

/// getOldDT() -> float!
luaFunc(getOldDT)
{
	luaReturnNum(core->get_old_dt());
}

/// getDT() -> float!
luaFunc(getDT)
{
	luaReturnNum(core->get_current_dt());
}

/// getFPS() -> int!
luaFunc(getFPS)
{
	luaReturnInt(core->fps);
}

/// isNested() -> bool!
luaFunc(isNested)
{
	luaReturnBool(core->isNested());
}

/// getNumberOfEntitiesNamed(string name) -> int!
luaFunc(getNumberOfEntitiesNamed)
{
	std::string s = getString(L);
	int c = game->getNumberOfEntitiesNamed(s);
	luaReturnNum(c);
}

/// entity_pullEntities(Entity e, float x, float y, float range, float len, float dt)
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
			if (ent != e && (ent->getEntityType() == ET_ENEMY || ent->getEntityType() == ET_AVATAR) && ent->isUnderWater())
			{
				Vector diff = ent->position - pos;
				if (diff.isLength2DIn(range))
				{
					Vector pull = pos - ent->position;
					pull.setLength2D(float(len) * dt);
					ent->vel2 += pull;

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
/// entity_delete(Entity e, float time = 0)
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
	luaReturnNil();
}

/// entity_isRidingOnEntity(Entity e) -> Entity|NoObj!; deprecated("Use entity_getRidingOnEntity instead")
/// entity_getRidingOnEntity(Entity e) -> Entity|NoObj!
/// This returns the entity that e is current "riding" on. This is for sitting on a pulled rock and has nothing to do with riding on seahorses etc!
luaFunc(entity_getRidingOnEntity)
{
	Entity *e = entity(L);
	luaReturnPtr(e ? e->ridingOnEntity : NULL);
}


/// entity_isProperty(Entity e, EntityProperty prop) -> bool!
luaFunc(entity_isProperty)
{
	Entity *e = entity(L);
	bool v = false;
	size_t ep = lua_tointeger(L, 2);
	if (e)
	{
		v = ep < EP_MAX && e->isEntityProperty((EntityProperty)ep);
	}
	luaReturnBool(v);
}


/// entity_setProperty(Entity e, EntityProperty prop, boolx on = false)
luaFunc(entity_setProperty)
{
	Entity *e = entity(L);
	size_t ep = lua_tointeger(L, 2);
	if (e && ep < EP_MAX)
	{
		e->setEntityProperty((EntityProperty)ep, getBool(L, 3));
	}
	luaReturnNil();
}

/// entity_setActivation(Entity e, Activation type, float radius = 0, float range = 0)
luaFunc(entity_setActivation)
{
	Entity *e = entity(L);
	if (e)
	{
		e->activationType = (Entity::ActivationType)lua_tointeger(L, 2);
		e->activationRadius = lua_tonumber(L, 3); // cursor radius
		e->activationRange = lua_tonumber(L, 4);
	}

	luaReturnNil();
}

/// entity_getActivation(Entity e) -> Activation?!, float?, float?
luaFunc(entity_getActivation)
{
	Entity *e = entity(L);
	if (e)
	{
		lua_pushinteger(L, e->activationType);
		lua_pushnumber(L, e->activationRadius);
		lua_pushnumber(L, e->activationRange);
		return 3;
	}

	luaReturnNil();
}

/// entity_setActivationType(Entity e, Activation type)
luaFunc(entity_setActivationType)
{
	Entity *e = entity(L);
	if (e)
		e->activationType = (Entity::ActivationType)lua_tointeger(L, 2);

	luaReturnNil();
}

/// entity_hasTarget(Entity e) -> bool!
luaFunc(entity_hasTarget)
{
	Entity *e = entity(L);
	luaReturnBool(e && e->getTargetEntity(e->currentEntityTarget));
}

/// entity_hurtTarget(Entity e, float damage)
luaFunc(entity_hurtTarget)
{
	Entity *e = entity(L);
	if (e)
		if(Entity *t = e->getTargetEntity(e->currentEntityTarget))
		{
			DamageData d;
			d.attacker = e;
			d.damage = lua_tonumber(L, 2);
			t->damage(d);
		}

	luaReturnNil();
}

/// entity_touchAvatarDamage(Entity e, int radius, float dmg, float speed = 0, float pushTime = 0, float ox = 0, float oy = 0) -> bool
/// Returns whether damage was done.
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

/// entity_getDistanceToEntity(Entity e, Entity e2) -> float!
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

/// entity_isEntityInside(Entity e) -> bool!
luaFunc(entity_isEntityInside)
{
	Entity *e = entity(L);
	luaReturnBool(e ? e->isEntityInside() : false);
}

/// entity_isTargetInRange(Entity e, float range) -> bool!
luaFunc(entity_isTargetInRange)
{
	Entity *e = entity(L);
	luaReturnBool(e && e->isTargetInRange(lua_tonumber(L, 2), e->currentEntityTarget));
}

/// randAngle360() -> int!
luaFunc(randAngle360)
{
	luaReturnNum(randAngle360());
}

/// randVector(float len = 1) -> float!, float
luaFunc(randVector)
{
	float num = lua_tonumber(L, 1);
	if (num == 0)
		num = 1;
	Vector v = randVector(num);
	luaReturnVec2(v.x, v.y);
}

/// getNaija() -> Entity|NoObj!
luaFunc(getNaija)
{
	luaReturnPtr(game->avatar);
}

/// getLi() -> Entity|NoObj!
luaFunc(getLi)
{
	luaReturnPtr(game->li);
}

/// setLi(Entity|NoObj li)
luaFunc(setLi)
{
	game->li = entityOpt(L);

	luaReturnNil();
}

/// entity_isPositionInRange(Entity e, float x, float y, float range) -> bool!
luaFunc(entity_isPositionInRange)
{
	Entity *e = entity(L);
	bool v = false;
	Vector pos = getVector(L, 2);
	if (e)
	{
		v = (e->position - pos).isLength2DIn(lua_tonumber(L, 4));
	}
	luaReturnBool(v);
}

/// entity_isEntityInRange(Entity e, Entity e2, float range) -> bool!
luaFunc(entity_isEntityInRange)
{
	Entity *e1 = entity(L);
	Entity *e2 = entity(L, 2);
	bool v = false;
	if (e1 && e2)
	{
		v = (e2->position - e1->position).isLength2DIn(lua_tonumber(L, 3));
	}
	luaReturnBool(v);
}

/// entity_moveTowardsTarget(Entity e, float dt, float speed)
luaFunc(entity_moveTowardsTarget)
{
	Entity *e = entity(L);
	if (e)
		e->moveTowardsTarget(lua_tonumber(L, 2), lua_tonumber(L, 3), e->currentEntityTarget);
	luaReturnNil();
}

/// entity_moveAroundTarget(Entity e, float dt, float speed, int dir = 0)
luaFunc(entity_moveAroundTarget)
{
	Entity *e = entity(L);
	if (e)
		e->moveAroundTarget(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tointeger(L, 4), e->currentEntityTarget);
	luaReturnNil();
}

/// entity_rotateToTarget(Entity e, float time = 0, float offsetAngle = 0)
luaFunc(entity_rotateToTarget)
{
	Entity *e = entity(L);
	if (e)
	{
		Entity *t = e->getTargetEntity(e->currentEntityTarget);
		if (t)
		{
			Vector v = t->position - e->position;
			e->rotateToVec(v, lua_tonumber(L, 2), lua_tonumber(L, 3));
		}
	}
	luaReturnNil();
}



// 1 = entity, 2 = part name
// This entity partMap is some legacy crap that can't be sensibly removed at this point because some scripts use it
static RenderObject *getPartMapRO(lua_State *L)
{
	RenderObject *r = NULL;
	if(ScriptedEntity *e = scriptedEntity(L))
	{
		ScriptedEntity::PartMap::iterator it = e->partMap.find(getString(L, 2));
		if(it != e->partMap.end())
			return it->second;
	}
	return NULL;
}

static Quad *getPartMapQuad(lua_State *L)
{
	return dynamic_cast<Quad*>(getPartMapRO(L));
}

/// entity_partWidthHeight(ScriptedEntity e, string part, float w, float h)
luaFunc(entity_partWidthHeight)
{
	Quad *r = getPartMapQuad(L);
	if(r)
		r->setWidthHeight(lua_tonumber(L, 3), lua_tonumber(L, 4));
	luaReturnNil();
}

/// entity_partSetSegs(ScriptedEntity e, string part, int x, int y, float dgox, float dgoy, float dgmx, float dgmy, float dgtm, boolx dgo = false)
luaFunc(entity_partSetSegs)
{
	Quad *r = getPartMapQuad(L);
	if (r)
	{
		r->setSegs(lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tonumber(L, 9), getBool(L, 10));
	}
	luaReturnNil();
}

/// entity_getID(Entity e) -> int!
luaFunc(entity_getID)
{
	Entity *e = entity(L);
	luaReturnInt(e ? e->getID() : 0);
}

/// getEntityByID(int id) -> Entity|NoObj!
luaFunc(getEntityByID)
{
	int v = lua_tointeger(L, 1);
	Entity *found = game->getEntityByID(v);
	luaReturnPtr(found);
}

/// node_activate(Node node, ...) -> Any...
luaFunc(node_activate)
{
	Path *p = path(L);
	if (p)
	{
		// pass everything on the stack except the node pointer
		int res = p->activateVariadic(L, lua_gettop(L) - 1);
		if (res >= 0)
			return res;
	}
	luaReturnNil();
}

/// entity_activate(Entity e, ...) -> Any...
luaFunc(entity_activate)
{
	Entity *e = entity(L);
	if (e)
	{
		// pass everything on the stack except the entity pointer
		int res = e->activateVariadic(L, lua_gettop(L) - 1);
		if (res >= 0)
			return res;
	}
	luaReturnNil();
}

/// node_setElementsInLayerActive(Node node, int layer, boolx on, int tag = 0)
// tag == 0 affects all tiles, otherwise only tiles with the exact tag are affected
luaFunc(node_setElementsInLayerActive)
{
	Path *p = path(L);
	if (p)
	{
		unsigned l = lua_tointeger(L, 2);
		bool v = getBool(L, 3);
		int tag = lua_tointeger(L, 4);

		if(l < MAX_TILE_LAYERS)
		{
			TileStorage& ts = dsq->tilemgr.tilestore[l];
			const size_t N = ts.tiles.size();
			for(size_t i = 0; i < N; ++i)
			{
				TileData& t = ts.tiles[i];
				if((!tag || t.tag == tag) && p->isCoordinateInside(Vector(t.x, t.y)))
					t.setVisible(v);
			}
			ts.refreshAll();
		}
	}
	luaReturnNil();
}

static int pushTileData(lua_State *L, const TileData& t, unsigned layer)
{
	/*  1 */ lua_pushinteger(L, t.et->idx);
	/*  2 */ lua_pushstring(L, t.et->gfx.c_str());
	/*  3 */ lua_pushboolean(L, t.isVisible());
	/*  4 */ lua_pushinteger(L, layer);
	/*  5 */ lua_pushinteger(L, t.tag);
	/*  6 */ lua_pushnumber(L, t.x);
	/*  7 */ lua_pushnumber(L, t.y);
	/*  8 */ lua_pushnumber(L, t.scalex);
	/*  9 */ lua_pushnumber(L, t.scaley);
	/* 10 */ lua_pushnumber(L, t.rotation);
	/* 11 */ lua_pushboolean(L, !!(t.flags & TILEFLAG_FH));
	/* 12 */ lua_pushboolean(L, !!(t.flags & TILEFLAG_FV));

	// 13, 14
	if((t.flags & TILEFLAG_REPEAT) && t.rep)
	{
		lua_pushnumber(L, t.rep->texscaleX);
		lua_pushnumber(L, t.rep->texscaleY);
	}
	else
	{
		lua_pushnil(L);
		lua_pushnil(L);
	}

	return 14;
}

/// refreshElementsOnLayerCallback(int layer, function callback) -> int
/// Calls callback(idx, gfx, visible, layer, tag, x, y, scalex, scaley, rotation, fh, fv, texscalex?, texscaley?) returns the new visibility; result = number of tiles visited
luaFunc(refreshElementsOnLayerCallback)
{
	const unsigned layer = lua_tointeger(L, 1);
	size_t done = 0;

	if(layer < MAX_TILE_LAYERS)
	{
		TileStorage& ts = dsq->tilemgr.tilestore[layer];
		const size_t N = ts.tiles.size();
		for(size_t i = 0; i < N; ++i)
		{
			lua_pushvalue(L, 2); // the callback
			TileData& t = ts.tiles[i];
			int args = pushTileData(L, t, layer);
			lua_call(L, args, 1);
			bool newon = lua_toboolean(L, -1);
			lua_pop(L, 1);
			t.setVisible(newon);
			++done;
		}
		if(done)
			ts.refreshAll();
	}
	luaReturnInt(done);
}

/// refreshElementsWithTagCallback(int tag, function callback) -> int
/// Callback as in refreshElementsOnLayerCallback; result = number of tiles visited
luaFunc(refreshElementsWithTagCallback)
{
	const int tag = lua_tointeger(L, 1);
	size_t total = 0;
	for(unsigned layer = 0; layer < MAX_TILE_LAYERS; ++layer)
	{
		TileStorage& ts = dsq->tilemgr.tilestore[layer];
		const size_t N = ts.tiles.size();
		size_t done = 0;
		for(size_t i = 0; i < N; ++i)
		{
			TileData& t = ts.tiles[i];
			if(t.tag == tag)
			{
				lua_pushvalue(L, 2); // the callback
				int args = pushTileData(L, t, layer);
				lua_call(L, args, 1);
				bool newon = lua_toboolean(L, -1);
				lua_pop(L, 1);
				t.setVisible(newon);
				++done;
			}
		}
		if(done)
			ts.refreshAll();
		total += done;
	}
	luaReturnInt(total);
}

/// node_getNumEntitiesIn(Node node, string? name) -> int!
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

/// node_getNearestEntity(Node node, string? name, Entity|NoObj? ignore) -> Entity|NoObj!
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

/// node_getNearestNode(Node n, string? label, Node|NoObj? ignore) -> Node|NoObj!
/// Nearest other node (by first point) with that label; no label = any.
luaFunc(node_getNearestNode)
{
	Path *p = path(L);
	Path *closest = 0;
	if (p && !p->nodes.empty())
	{
		std::string name;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);
		Path *ignore = pathOpt(L, 3);
		closest = game->getNearestPath(p->nodes[0].position, name, ignore);
	}
	luaReturnPtr(closest);
}

/// entity_getNearestBoneToPosition(Entity e, float x, float y) -> Bone|NoObj!
luaFunc(entity_getNearestBoneToPosition)
{
	Entity *me = entity(L);
	Vector p(lua_tonumber(L, 2), lua_tonumber(L, 3));
	float smallestDist = HUGE_VALF;
	Bone *closest = 0;
	if (me)
	{
		for (size_t i = 0; i < me->skeletalSprite.bones.size(); i++)
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

/// entity_getNearestNode(Entity e, string? label, Node|NoObj? ignore) -> Node|NoObj!
luaFunc(entity_getNearestNode)
{
	Entity *me = entity(L);
	Path *closest = 0;
	if (me)
	{
		std::string name;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);
		Path *ignore = pathOpt(L, 3);
		closest = game->getNearestPath(me->position, name, ignore);
	}
	luaReturnPtr(closest);
}

/// ing_hasIET(Ingredient ing, IngredientEffect iet) -> bool!
luaFunc(ing_hasIET)
{
	Ingredient *i = getIng(L, 1);
	bool has = i && i->hasIET((IngredientEffectType)lua_tointeger(L, 2));
	luaReturnBool(has);
}

/// ing_getIngredientName(Ingredient ing) -> string!
luaFunc(ing_getIngredientName)
{
	Ingredient *i = getIng(L, 1);
	IngredientData *data = i ? i->getIngredientData() : 0;
	luaReturnStr(data ? data->name.c_str() : "");
}

/// entity_getNearestEntity(Entity e, string? name, float range = 0, EntityType? type, DamageType? dt, Entity|NoObj? ignore) -> Entity|NoObj!
/// name prefixed with '!' or '~' = any name except that; range 0 = unlimited.
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

/// getNearestEntity(float x, float y, float radius, Entity|NoObj? ignore, EntityType? type = ET_ENEMY, DamageType? dt, Layer? lrStart, Layer? lrEnd) -> Entity|NoObj!
luaFunc(getNearestEntity)
{
	Vector p(lua_tonumber(L, 1), lua_tonumber(L, 2));
	float radius = lua_tonumber(L, 3);
	Entity *ignore = entityOpt(L, 4);
	EntityType et = lua_isnumber(L, 5) ? (EntityType)lua_tointeger(L, 5) : ET_ENEMY;
	DamageType dt = lua_isnumber(L, 6) ? (DamageType)lua_tointeger(L, 6) : DT_NONE;
	int lrStart = lua_isnumber(L, 7) ? lua_tointeger(L, 7) : -1;
	int lrEnd = lua_isnumber(L, 8) ? lua_tointeger(L, 8) : -1;

	Entity *target = game->getNearestEntity(p, radius, ignore, et, dt, lrStart, lrEnd);
	luaReturnPtr(target);
}

// LUAWTF-NOTE(weird): with both dirx and diry nonzero the result is x (y is overwritten); a float position is truncated to int
/// findWall(float x, float y, int dirx, int diry) -> int!
/// World x (dirx ~= 0) or y of the first obstructed tile walking from (x, y) in tile steps. Returns dirx when going diagonally. Consider using findWall2() instead.
luaFunc(findWall)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	int dirx = lua_tointeger(L, 3);
	int diry = lua_tointeger(L, 4);
	if (dirx == 0 && diry == 0){ debugLog("dirx && diry are zero!"); luaReturnNum(0); }

	TileVector t(Vector(x, y));
	while (!game->isObstructed(t))
	{
		t.x += dirx;
		t.y += diry;
	}
	Vector v = t.worldVector();
	int wall = 0;
	if (diry != 0)		wall = v.y;
	if (dirx != 0)		wall = v.x;
	luaReturnInt(wall);
}

/// findWall2(float x, float y, int dirx, int diry) -> float!, float
/// Returns x, y where the nearest wall is in the given direction. Returns (0, 0) when dirx, diry are both 0.
luaFunc(findWall2)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	int dirx = lua_tointeger(L, 3);
	int diry = lua_tointeger(L, 4);
	Vector v;
	if (!(dirx == 0 && diry == 0))
	{
		TileVector t(Vector(x, y));
		while (!game->isObstructed(t))
		{
			t.x += dirx;
			t.y += diry;
		}
		v = t.worldVector();
	}
	luaReturnVec2(v.x, v.y);
}

/// isUnderWater(float x, float y, float radius = 0) -> bool!
luaFunc(isUnderWater)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float rad = lua_tonumber(L, 3);
	luaReturnBool(game->isUnderWater(Vector(x, y), rad).uw);
}


/// setCutscene(boolx on, boolx canSkip = false)
luaFunc(setCutscene)
{
	dsq->setCutscene(getBool(L, 1), getBool(L, 2));
	luaReturnNil();
}

/// isInCutscene() -> bool!
luaFunc(isInCutscene)
{
	luaReturnBool(dsq->isInCutscene());
}

/// toggleSteam(boolx on) -> bool
luaFunc(toggleSteam)
{
	bool on = getBool(L, 1);
	for (Path *p = game->getFirstPathOfType(PATH_STEAM); p; p = p->nextOfType)
	{
		p->setActive(on);
	}
	luaReturnBool(on);
}

/// getFirstEntity() -> Entity|NoObj
luaFunc(getFirstEntity)
{
	luaReturnPtr(dsq->getFirstEntity());
}

/// getNextEntity() -> Entity|NoObj
luaFunc(getNextEntity)
{
	luaReturnPtr(dsq->getNextEntity());
}

typedef std::pair<Entity*, float> EntityDistancePair;
static std::vector<EntityDistancePair> filteredEntities(20);
static size_t filteredEntityIdx = 0;

static bool _entityDistanceCmp(const EntityDistancePair& a, const EntityDistancePair& b)
{
	return a.second < b.second;
}
static bool _entityDistanceEq(const EntityDistancePair& a, const EntityDistancePair& b)
{
	return a.first == b.first;
}

template<typename F>
static size_t _entityFilterT(lua_State *L, F& func)
{
	const Vector p(lua_tonumber(L, 1), lua_tonumber(L, 2));
	const float radius = lua_tonumber(L, 3);
	const Entity *ignore = lua_isuserdata(L, 4) ? entity(L, 4) : NULL;
	const EntityType et = lua_isnumber(L, 5) ? (EntityType)lua_tointeger(L, 5) : ET_NOTYPE;
	const DamageType dt = lua_isnumber(L, 6) ? (DamageType)lua_tointeger(L, 6) : DT_NONE;
	const int lrStart = lua_isnumber(L, 7) ? lua_tointeger(L, 7) : -1;
	const int lrEnd = lua_isnumber(L, 8) ? lua_tointeger(L, 8) : -1;

	const float sqrRadius = radius * radius;
	float distsq;
	const bool skipLayerCheck = lrStart == -1 || lrEnd == -1;
	const bool skipRadiusCheck = radius <= 0;
	size_t added = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		distsq = (e->position - p).getSquaredLength2D();
		if (skipRadiusCheck || distsq <= sqrRadius)
		{
			if (e != ignore && e->isPresent())
			{
				if (skipLayerCheck || (e->layer >= lrStart && e->layer <= lrEnd))
				{
					if (et == ET_NOTYPE || e->getEntityType() == et)
					{
						if (dt == DT_NONE || e->isDamageTarget(dt))
						{
							func.add(L, e, distsq);
							++added;
						}
					}
				}
			}
		}
	}
	return added;
}

struct StaticEntityFilter
{
	inline void add(lua_State *L, Entity *e, float distsq)
	{
		filteredEntities.push_back(std::make_pair(e, distsq));
	}
};

static size_t _entityFilter(lua_State *L)
{
	StaticEntityFilter filt;
	const size_t added = _entityFilterT(L, filt);
	if(added)
	{
		std::sort(filteredEntities.begin(), filteredEntities.end(), _entityDistanceCmp);
		std::vector<EntityDistancePair>::iterator newend = std::unique(filteredEntities.begin(), filteredEntities.end(), _entityDistanceEq);
		filteredEntities.resize(std::distance(filteredEntities.begin(), newend));
	}

	// Add terminator if there is none
	if(!filteredEntities.size() || filteredEntities.back().first)
		filteredEntities.push_back(std::make_pair((Entity*)NULL, 0.0f)); // terminator

	filteredEntityIdx = 0; // Reset getNextFilteredEntity() iteration index

	return added;
}

struct EntityFilterTableWriter
{
	EntityFilterTableWriter() : _idx(0) {}
	inline void add(lua_State *L, Entity *e, float distsq)
	{
		luaPushPointer(L, e);
		lua_rawseti(L, -2, ++_idx);
	}
	unsigned _idx;
};

/// filterNearestEntities(float x, float y, float radius = 0, Entity|NoObj? ignore, EntityType? type, DamageType? dt, Layer? lrStart, Layer? lrEnd) -> int
/// Fills the list read by getNextFilteredEntity(), sorted by distance; radius <= 0 = unlimited. Returns the count.
luaFunc(filterNearestEntities)
{
	filteredEntities.clear();
	luaReturnInt(_entityFilter(L));
}

// LUAWTF-NOTE(weird): dedup is `std::unique` after sorting by distance: an entity found again from another center has a different distsq, isn't adjacent, and stays duplicated; distances from different centers are mixed in one list
/// filterNearestEntitiesAdd(float x, float y, float radius = 0, Entity|NoObj? ignore, EntityType? type, DamageType? dt, Layer? lrStart, Layer? lrEnd) -> int
luaFunc(filterNearestEntitiesAdd)
{
	// Remove terminator if there is one
	if(filteredEntities.size() && !filteredEntities.back().first)
		filteredEntities.pop_back();
	luaReturnInt(_entityFilter(L));
}

/// getNextFilteredEntity() -> Entity|NoObj, float
/// Second result: squared distance.
luaFunc(getNextFilteredEntity)
{
	const EntityDistancePair& ep = filteredEntities[filteredEntityIdx];
	luaPushPointer(L, ep.first);
	lua_pushnumber(L, ep.second);
	if (ep.first)
		++filteredEntityIdx;
	return 2;
}

// FIXME: make sure this is compatible with the android version. -- fg
/// getEntityList([Entity]? t) -> [Entity]!, int
/// Fills t (or a new table) from index 1; the count tells where valid entries end (t isn't cleared).
luaFunc(getEntityList)
{
	int tabidx = 1;
	if(!lua_istable(L, tabidx))
	{
		lua_createtable(L, (int)dsq->entities.size(), 0); // rough guess
		tabidx = lua_gettop(L);
	}
	int i = 1; // always next index
	FOR_ENTITIES(ep)
	{
		Entity *e = *ep;
		luaPushPointerNonNull(L, e);
		lua_rawseti(L, tabidx, i++);
	}
	lua_pushinteger(L, (lua_Integer)i - 1);
	return 2;
}

// FIXME: make sure this is compatible with the android version. -- fg
static int getEntityListInRange(lua_State *L, const Vector& pos, float range, int tabidx)
{
	if(!lua_istable(L, tabidx))
	{
		lua_newtable(L);
		tabidx = lua_gettop(L);
	}
	int i = 1; // always next index
	FOR_ENTITIES(ep)
	{
		Entity *e = *ep;
		if((e->position - pos).isLength2DIn(range))
		{
			luaPushPointerNonNull(L, e);
			lua_rawseti(L, tabidx, i++);
		}
	}
	lua_pushinteger(L, (lua_Integer)i - 1);
	return 2;
}

/// entity_getEntityListInRange(Entity e, float range, [Entity]? t) -> [Entity]?, int?
luaFunc(entity_getEntityListInRange)
{
	Entity *me = entity(L);
	if(!me)
		luaReturnNil();
	return getEntityListInRange(L, me->position, lua_tonumber(L, 2), 3);
}

/// vector_getEntityListInRange(float x, float y, float range) -> [Entity]?, int?
luaFunc(vector_getEntityListInRange)
{
	Vector pos(lua_tonumber(L, 1), lua_tonumber(L, 2));
	return getEntityListInRange(L, pos, lua_tonumber(L, 3), 4);
}

/// getEntity(string name) -> Entity|NoObj!
luaFunc(getEntity)
{
	Entity *ent = 0;
	if (lua_isstring(L, 1))
	{
		std::string s = lua_tostring(L, 1);
		ent = dsq->getEntityByName(s);
	}
	luaReturnPtr(ent);
}

/// entity_partAlpha(ScriptedEntity e, string name, float startAlpha = 0, float targetAlpha = 0, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
/// start -1 = from the current alpha.
luaFunc(entity_partAlpha)
{
	RenderObject *r = getPartMapRO(L);
	if (r)
	{
		float start = lua_tonumber(L, 3);
		if (start != -1)
			r->alpha = start;
		interpolateVec1(L, r->alpha, 4);
	}
	luaReturnNil();
}

/// entity_partBlendType(ScriptedEntity e, string part, BlendType bt)
luaFunc(entity_partBlendType)
{
	RenderObject *r = getPartMapRO(L);
	if (r)
		r->setBlendType(getBlendType(L, 3));
	luaReturnNil();
}

/// entity_partRotate(ScriptedEntity e, string part, float rot, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(entity_partRotate)
{
	RenderObject *r = getPartMapRO(L);
	if (r)
		interpolateVec1z(L, r->rotation, 3);
	luaReturnNil();
}

/// entity_getStateTime(Entity e) -> float!
luaFunc(entity_getStateTime)
{
	Entity *e = entity(L);
	float t = 0;
	if (e)
		t = e->getStateTime();
	luaReturnNum(t);
}

/// entity_setStateTime(Entity e, float t) -> float
luaFunc(entity_setStateTime)
{
	Entity *e = entity(L);
	float t = lua_tonumber(L, 2);
	if (e)
		e->setStateTime(t);
	luaReturnNum(t);
}

/// entity_offsetUpdate(Entity e); deprecated
luaFunc(entity_offsetUpdate)
{
	Entity *e = entity(L);
	if (e)
	{
		float uc = e->updateCull;
		e->updateCull = -1;
		float t = float(Randomness::getBits32()%10000)/1000.0f;
		e->update(t);
		e->updateCull = uc;
	}
	luaReturnNil();
}

/// entity_switchLayer(Entity e, int lcode = 0); deprecated
/// Uses the same layer IDs as entity_setEntityLayer(), not actual LR_* values; not to be confused with obj_setLayer or entity_setEntityLayer!!
luaFunc(entity_switchLayer)
{
	Entity *e = entity(L);
	if (e)
	{
		int lcode = lua_tointeger(L, 2);
		int toLayer = LR_ENTITIES;

		toLayer = dsq->getEntityLayerToLayer(lcode);

		if (e->getEntityType() == ET_AVATAR)
			toLayer = LR_ENTITIES;

		core->switchRenderObjectLayer(e, toLayer);
	}
	luaReturnNil();
}

/// entity_initHair(Entity e, int numSegments, float segmentLength, float width, string tex)
luaFunc(entity_initHair)
{
	Entity *se = entity(L);
	if (se)
	{
		se->initHair(lua_tointeger(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), getString(L, 5));
	}
	luaReturnNil();
}

/// entity_getHair(Entity e) -> Hair|NoObj!
luaFunc(entity_getHair)
{
	Entity *e = entity(L);
	luaReturnPtr(e ? e->hair : NULL);
}

/// entity_clearHair(Entity e)
luaFunc(entity_clearHair)
{
	Entity *e = entity(L);
	if (e && e->hair)
	{
		e->hair->safeKill();
		e->hair = 0;
	}
	luaReturnNil();
}

/// entity_getHairPosition(Entity e, int idx) -> float!, float
luaFunc(entity_getHairPosition)
{
	Entity *se = entity(L);
	Vector p;
	size_t idx = lua_tointeger(L, 2);
	if (se && se->hair)
	{
		if(idx < se->hair->_points.size())
		{
			p = se->hair->_points[idx];
		}
	}
	luaReturnVec2(p.x, p.y);
}

/// entity_setHairHeadPosition(Entity e, float x, float y)
luaFunc(entity_setHairHeadPosition)
{
	Entity *se = entity(L);
	if (se)
	{
		se->setHairHeadPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	}
	luaReturnNil();
}

/// entity_updateHair(Entity e, float dt)
luaFunc(entity_updateHair)
{
	Entity *se = entity(L);
	if (se)
	{
		se->updateHair(lua_tonumber(L, 2));
	}
	luaReturnNil();
}

/// entity_exertHairForce(Entity e, float x, float y, float dt, int usePerc = 0)
/// usePerc: 0 = scale by segment weight, 1 = inverse weight, other = unscaled.
luaFunc(entity_exertHairForce)
{
	Entity *se = entity(L);
	if (se && se->hair)
		se->hair->exertForce(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tointeger(L, 5));
	luaReturnNil();
}

/// entity_exertHairSegmentForce(Entity e, int idx, float x, float y, float dt, int usePerc = 0)
luaFunc(entity_exertHairSegmentForce)
{
	Entity *se = entity(L);
	if (se && se->hair)
		se->hair->exertNodeForce(lua_tointeger(L, 2), Vector(lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5), lua_tointeger(L, 6));
	luaReturnNil();
}

/// entity_setHairTextureFlip(Entity e, boolx flip)
luaFunc(entity_setHairTextureFlip)
{
	Entity *se = entity(L);
	if (se && se->hair)
		se->hair->setTextureFlip(getBool(L, 2));
	luaReturnNil();
}

/// entity_setHairWidth(Entity e, float w)
luaFunc(entity_setHairWidth)
{
	Entity *se = entity(L);
	if (se && se->hair)
		se->hair->setHairWidth(lua_tonumber(L, 2));
	luaReturnNil();
}

/// entity_initPart(ScriptedEntity e, string name, string tex, float x, float y, boolx renderAfter = false, boolx flipH = false, boolx flipV = false, float offx = 0, float offy = 0, float offTime = 0)
/// A nonzero (offx, offy) starts an endless ping-pong offset interpolation.
luaFunc(entity_initPart)
{
	std::string partName(getString(L, 2));
	std::string partTex(getString(L, 3));
	Vector partPosition(lua_tonumber(L, 4), lua_tonumber(L, 5));
	bool renderAfter = getBool(L, 6);
	bool partFlipH = getBool(L, 7);
	bool partFlipV = getBool(L,8);
	Vector offsetInterpolateTo(lua_tonumber(L, 9), lua_tonumber(L, 10));
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

	luaReturnNil();
}

/// entity_findTarget(Entity e, float range, EntityType type = 0)
/// type defaults to 0 = ET_AVATAR.
luaFunc(entity_findTarget)
{
	Entity *e = entity(L);
	if (e)
		e->findTarget(lua_tonumber(L, 2), lua_tointeger(L, 3), e->currentEntityTarget);

	luaReturnNil();
}

/// entity_doFriction(Entity e, float dt, float len)
luaFunc(entity_doFriction)
{
	Entity *e = entity(L);
	if (e)
	{
		e->doFriction(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	luaReturnNil();
}

/// entity_doGlint(Entity e, string gfx, BlendType bt = 0)
luaFunc(entity_doGlint)
{
	Entity *e = entity(L);
	if (e)
		e->doGlint(e->position, Vector(2,2), getString(L,2), getBlendType(L, 3));
	luaReturnNil();
}

/// entity_getTarget(Entity e, int idx = 0) -> Entity|NoObj!
luaFunc(entity_getTarget)
{
	Entity *e = entity(L);
	Entity *retEnt = NULL;
	if (e)
	{
		retEnt = e->getTargetEntity(lua_tointeger(L, 2));

	}
	luaReturnPtr(retEnt);
}

/// entity_getTargetPositionX(Entity e) -> float!
luaFunc(entity_getTargetPositionX)
{
	Entity *e = entity(L);
	Entity *t = e ? e->getTargetEntity() : NULL;
	luaReturnNum(t ? t->position.x : 0.0f);
}

/// entity_getTargetPositionY(Entity e) -> float!
luaFunc(entity_getTargetPositionY)
{
	Entity *e = entity(L);
	Entity *t = e ? e->getTargetEntity() : NULL;
	luaReturnNum(t ? t->position.y : 0.0f);
}

/// entity_isNearObstruction(Entity e, int sz, ObsCheck type = 0) -> bool!
luaFunc(entity_isNearObstruction)
{
	Entity *e = entity(L);
	int sz = lua_tointeger(L, 2);
	int type = lua_tointeger(L, 3);
	bool v = false;
	if (e)
	{
		v = e->isNearObstruction(sz, type);
	}
	luaReturnBool(v);
}

/// entity_isInvincible(Entity e) -> bool!
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

/// entity_setEatType(Entity e, EatType et, string? file)
luaFunc(entity_setEatType)
{
	Entity *e = entity(L);
	int et = lua_tointeger(L, 2);
	if (e)
		e->setEatType((EatType)et, getString(L, 3));
	luaReturnNil();
}

/// getMapName() -> string!
luaFunc(getMapName)
{
	luaReturnStr(game->sceneName.c_str());
}

/// isMapName(string name) -> bool!
/// Case-insensitive.
luaFunc(isMapName)
{
	std::string s1 = game->sceneName;
	std::string s2 = getString(L, 1);
	stringToLower(s1);
	stringToLower(s2);
	bool ret = (s1 == s2);

	luaReturnBool(ret);
}

/// mapNameContains(string s) -> bool!
luaFunc(mapNameContains)
{
	std::string s = game->sceneName;
	std::string s2 = getString(L, 1);
	stringToLower(s);
	stringToLower(s2);
	bool b = (s.find(s2) != std::string::npos);
	luaReturnBool(b);
}

/// entity_fireGas(Entity e, int radius, float life, float damage, string gfx, float r, float g, float b, float offx = 0, float offy = 0, float poisonTime = 0)
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
		core->getTopStateObject()->addRenderObject(c, LR_PARTICLES);
	}
	luaReturnNil();
}

/// isInputEnabled() -> bool!
luaFunc(isInputEnabled)
{
	Avatar *a = game->avatar;
	luaReturnBool(a && a->isInputEnabled());
}

/// enableInput()
luaFunc(enableInput)
{
	if(Avatar *a = game->avatar)
		a->enableInput();
	luaReturnNil();
}

/// disableInput()
luaFunc(disableInput)
{
	if(Avatar *a = game->avatar)
		a->disableInput();
	luaReturnNil();
}

/// getInputMode(int source = 0) -> InputDevice!
/// source 0 = the last used input mode.
luaFunc(getInputMode)
{
	int source = lua_tointeger(L, 1);
	luaReturnInt(dsq->getInputModeSafe(source - 1));
}

static Joystick *_getJoystick(lua_State *L, int idx = 1)
{
	int source = lua_tointeger(L, idx) - 1;
	if(source < 0)
		return core->getJoystick(0); // HACK: FIXME: do something sensible instead

	return core->getJoystickForSourceID(source);
}

/// getJoystickAxisLeft(int source = 0) -> float!, float
/// source: 1-based joystick source id; 0/nil = first joystick
luaFunc(getJoystickAxisLeft)
{
	Vector v;
	if(Joystick *j = _getJoystick(L))
		v = j->position;
	luaReturnVec2(v.x, v.y);
}

/// getJoystickAxisRight(int source = 0) -> float!, float
luaFunc(getJoystickAxisRight)
{
	Vector v;
	if(Joystick *j = _getJoystick(L))
		v = j->rightStick;
	luaReturnVec2(v.x, v.y);
}

/// quit()
luaFunc(quit)
{
#ifdef AQUARIA_DEMO
	dsq->nag(NAG_QUIT);
#else
	dsq->quit();
#endif

	luaReturnNil();
}

/// doModSelect()
luaFunc(doModSelect)
{
	dsq->doModSelect();
	luaReturnNil();
}

/// doLoadMenu()
luaFunc(doLoadMenu)
{
	dsq->doLoadMenu();
	luaReturnNil();
}

/// resetContinuity()
luaFunc(resetContinuity)
{
	dsq->continuity.reset();
	luaReturnNil();
}

/// toWindowFromWorld(float x, float y) -> float!, float
luaFunc(toWindowFromWorld)
{
	Vector p(lua_tonumber(L, 1), lua_tonumber(L, 2));
	p = core->getWindowPosition(p);
	luaReturnVec2(p.x, p.y);
}

/// toWorldFromWindow(float x, float y) -> float!, float
luaFunc(toWorldFromWindow)
{
	Vector p(lua_tonumber(L, 1), lua_tonumber(L, 2));
	p = core->getGamePosition(p);
	luaReturnVec2(p.x, p.y);
}

/// setMousePos(float x, float y)
luaFunc(setMousePos)
{
	core->setMousePosition(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)));
	luaReturnNil();
}

/// getMousePos() -> float!, float
luaFunc(getMousePos)
{
	luaReturnVec2(core->mouse.position.x, core->mouse.position.y);
}

/// getMouseWorldPos() -> float!, float
luaFunc(getMouseWorldPos)
{
	Vector v = dsq->getGameCursorPosition();
	luaReturnVec2(v.x, v.y);
}

/// getMouseWheelChange() -> int!
luaFunc(getMouseWheelChange)
{
	luaReturnNum(core->mouse.scrollWheelChange);
}

/// setMouseConstraintCircle(float x, float y, float radius)
luaFunc(setMouseConstraintCircle)
{
	core->setMouseConstraintCircle(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)), lua_tonumber(L, 3));
	luaReturnNil();
}

/// setMouseConstraint(boolx on)
luaFunc(setMouseConstraint)
{
	core->setMouseConstraint(getBool(L, 1));
	luaReturnNil();
}

/// fade(float alpha, float time = 0, float r = 0, float g = 0, float b = 0, float colorTime = 0)
luaFunc(fade)
{
	dsq->overlay->color.interpolateTo(Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5)), lua_tonumber(L, 6));
	dsq->overlay->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// fade2(float alpha, float time = 0, float r = 0, float g = 0, float b = 0, float colorTime = 0)
luaFunc(fade2)
{
	dsq->overlay2->color.interpolateTo(Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5)), lua_tonumber(L, 6));
	dsq->overlay2->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// fade3(float alpha, float time = 0, float r = 0, float g = 0, float b = 0, float colorTime = 0)
luaFunc(fade3)
{
	dsq->overlay3->color.interpolateTo(Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5)), lua_tonumber(L, 6));
	dsq->overlay3->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// vision(string folder, int num, boolx ignoreMusic = false)
luaFunc(vision)
{
	dsq->vision(getString(L, 1), lua_tonumber(L, 2), getBool(L, 3));
	luaReturnNil();
}

/// musicVolume(float vol, float time = 0)
luaFunc(musicVolume)
{
	dsq->sound->setMusicFader(lua_tonumber(L, 1), lua_tonumber(L, 2));
	luaReturnNil();
}

/// voice(string name, float volMod = 0)
/// volMod: 0/nil = default volume, -1 = 0
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

/// voiceOnce(string name)
luaFunc(voiceOnce)
{
	dsq->voiceOnce(getString(L, 1));
	luaReturnNil();
}

/// voiceInterupt(string name)
luaFunc(voiceInterupt)
{
	dsq->voiceInterupt(getString(L, 1));
	luaReturnNil();
}

/// stopVoice()
luaFunc(stopVoice)
{
	dsq->stopVoice();
	luaReturnNil();
}

/// stopAllSfx()
luaFunc(stopAllSfx)
{
	dsq->sound->stopAllSfx();
	luaReturnNil();
}

/// stopAllVoice()
luaFunc(stopAllVoice)
{
	dsq->sound->stopAllVoice();
	luaReturnNil();
}

/// fadeIn(float time = 0)
luaFunc(fadeIn)
{
	dsq->overlay->alpha.interpolateTo(0, lua_tonumber(L, 1));
	luaReturnNil();
}

/// fadeOut(float time = 0)
luaFunc(fadeOut)
{
	dsq->overlay->color = 0;
	dsq->overlay->alpha.interpolateTo(1, lua_tonumber(L, 1));
	luaReturnNil();
}

/// entity_setWeight(CollideEntity e, float weight)
luaFunc(entity_setWeight)
{
	CollideEntity *e = collideEntity(L);
	if (e)
		e->weight = lua_tonumber(L, 2);
	luaReturnNil();
}

/// entity_getWeight(CollideEntity e) -> float!
luaFunc(entity_getWeight)
{
	CollideEntity *e = collideEntity(L);
	luaReturnNum(e ? e->weight : 0.0f);
}

/// pickupGem(string name, boolx noEffects = false) -> int
/// Returns the new gem's 0-based index (for setGem*/removeGem).
luaFunc(pickupGem)
{
	dsq->continuity.pickupGem(getString(L), !getBool(L, 2));
	luaReturnInt(dsq->continuity.gems.size() - 1);
}

/// setGemPosition(int gemId, float x, float y, string? mapName) -> bool
luaFunc(setGemPosition)
{
	size_t gemId = lua_tointeger(L, 1);
	std::string mapname = getString(L, 4);
	if(mapname.empty())
		mapname = game->sceneName;
	Vector pos(lua_tonumber(L, 2), lua_tonumber(L, 3));
	bool result = false;

	WorldMapTileContainer *tc = game->worldMapRender->getTileByName(mapname.c_str());
	if(tc)
	{
		if(gemId < dsq->continuity.gems.size())
		{
			Continuity::Gems::iterator it = dsq->continuity.gems.begin();
			std::advance(it, gemId);
			GemData& gem = *it;
			gem.pos = gem.global ? tc->worldPosToMapPos(pos) : pos;
			gem.mapName = mapname;
			game->worldMapRender->refreshGem(&gem);
			result = true;
		}
		else
		{
			debugLog("setGemPosition: invalid index");
		}
	}
	else
	{
		debugLog("setGemPosition: Map tile does not exist: " + mapname);
	}
	luaReturnBool(result);
}

/// setGemName(int gemId, string name) -> bool
luaFunc(setGemName)
{
	size_t gemId = lua_tointeger(L, 1);
	bool result = false;

	if(gemId < dsq->continuity.gems.size())
	{
		Continuity::Gems::iterator it = dsq->continuity.gems.begin();
		std::advance(it, gemId);
		GemData& gem = *it;
		gem.name = getString(L, 2);
		game->worldMapRender->refreshGem(&gem);
		result = true;
	}
	else
		debugLog("setGemName: invalid index");

	luaReturnBool(result);
}

/// setGemBlink(int gemId, boolx blink) -> bool
luaFunc(setGemBlink)
{
	size_t gemId = lua_tointeger(L, 1);
	bool result = false;

	if(gemId < dsq->continuity.gems.size())
	{
		Continuity::Gems::iterator it = dsq->continuity.gems.begin();
		std::advance(it, gemId);
		GemData& gem = *it;
		gem.blink = getBool(L, 2);
		result = true;
	}
	else
		debugLog("setGemBlink: invalid index");

	luaReturnBool(result);
}

/// removeGem(int gemId) -> bool
luaFunc(removeGem)
{
	size_t gemId = lua_tointeger(L, 1);
	bool removed = false;
	if(gemId < dsq->continuity.gems.size())
	{
		Continuity::Gems::iterator it = dsq->continuity.gems.begin();
		std::advance(it, gemId);
		removed = game->worldMapRender->removeGem(&(*it));
	}
	luaReturnBool(removed);
}

/// beaconEffect(Beacon index)
luaFunc(beaconEffect)
{
	int index = lua_tointeger(L, 1);

	BeaconData *b = dsq->continuity.getBeaconByIndex(index);
	if(b)
	{
		float p1 = 0.7f;
		float p2 = 1.0f - p1;
		Vector pos = game->miniMapRender->getWorldPosition();
		dsq->clickRingEffect(pos, 0, (b->color*p1) + Vector(p2, p2, p2), 1);
		dsq->clickRingEffect(pos, 1, (b->color*p1) + Vector(p2, p2, p2), 1);

		dsq->sound->playSfx("ping");
	}

	luaReturnNil();
}

/// setBeacon(Beacon index, boolx on, float x = 0, float y = 0, float r = 0, float g = 0, float b = 0)
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

/// getBeacon(Beacon index) -> bool!
/// Returns true if the beacon is currently on
luaFunc(getBeacon)
{
	int index = lua_tointeger(L, 1);
	const BeaconData *b = dsq->continuity.getBeaconByIndex(index);
	luaReturnBool(b && b->on);
}

/// getCostume() -> string!
luaFunc(getCostume)
{
	luaReturnStr(dsq->continuity.costume.c_str());
}

/// setCostume(string name)
luaFunc(setCostume)
{
	dsq->continuity.setCostume(getString(L));
	luaReturnNil();
}

/// setLayerRenderPass(Layer layer, int startPass, int endPass)
luaFunc(setLayerRenderPass)
{
	size_t layer = lua_tointeger(L, 1);
	int startPass = lua_tointeger(L, 2);
	int endPass = lua_tointeger(L, 3);
	if(layer < core->renderObjectLayers.size())
	{
		core->renderObjectLayers[layer].startPass = startPass;
		core->renderObjectLayers[layer].endPass = endPass;
	}
	luaReturnNil();
}

/// setElementLayerVisible(int layer, boolx visible)
/// layer: element layer 0-based (LR_ELEMENTS1 + layer), not a LR_* constant
luaFunc(setElementLayerVisible)
{
	int l = lua_tointeger(L, 1);
	bool v = getBool(L, 2);
	game->setElementLayerVisible(l, v);
	luaReturnNil();
}

/// isElementLayerVisible(int layer) -> bool!
/// layer: element layer 0-based (LR_ELEMENTS1 + layer), not a LR_* constant
luaFunc(isElementLayerVisible)
{
	luaReturnBool(game->isElementLayerVisible(lua_tointeger(L, 1)));
}

/// isStreamingVoice() -> bool!
luaFunc(isStreamingVoice)
{
	bool v = dsq->sound->isPlayingVoice();
	luaReturnBool(v);
}

/// isObstructed(float x, float y, int obs = 0) -> bool!
/// obs: OT_* mask; 0 = any obstruction bit
luaFunc(isObstructed)
{
	int obs = lua_tointeger(L, 3);
	luaReturnBool(game->isObstructed(TileVector(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2))), obs ? obs : -1));
}

/// isObstructedRaw(float x, float y, int obs) -> bool!
luaFunc(isObstructedRaw)
{
	int obs = lua_tointeger(L, 3);
	luaReturnBool(game->isObstructedRaw(TileVector(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2))), obs));
}

/// getObstruction(float x, float y) -> int!
luaFunc(getObstruction)
{
	luaReturnInt(game->getGrid(TileVector(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)))));
}

/// getGridRaw(float x, float y) -> int!
luaFunc(getGridRaw)
{
	luaReturnInt(game->getGridRaw(TileVector(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)))));
}

/// isObstructedBlock(float x, float y, int span = 0) -> bool!
luaFunc(isObstructedBlock)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	int span = lua_tointeger(L, 3);
	TileVector t(Vector(x,y));

	bool obs = false;
	for (int xx = t.x-span; xx <= t.x+span; xx++)
	{
		for (int yy = t.y-span; yy <= t.y+span; yy++)
		{
			if (game->isObstructed(TileVector(xx, yy)))
			{
				obs = true;
				goto out;
			}
		}
	}
out:
	luaReturnBool(obs);
}

/// node_getFlag(Node n) -> int!
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

/// node_isFlag(Node n, int value) -> bool!
luaFunc(node_isFlag)
{
	Path *p = path(L);
	int c = lua_tointeger(L, 2);
	bool ret = false;
	if (p)
	{
		ret = (c == dsq->continuity.getPathFlag(p));
	}
	luaReturnBool(ret);
}

/// node_setFlag(Node n, int value)
luaFunc(node_setFlag)
{
	Path *p = path(L);
	int v = lua_tointeger(L, 2);
	if (p)
	{
		dsq->continuity.setPathFlag(p, v);
	}
	luaReturnNil();
}

/// entity_isFlag(Entity e, int value) -> bool!
luaFunc(entity_isFlag)
{
	Entity *e = entity(L);
	int v = lua_tointeger(L, 2);
	bool b = false;
	if (e)
	{
		b = (dsq->continuity.getEntityFlag(game->sceneName, e->getID())==v);
	}
	luaReturnBool(b);
}

/// entity_setFlag(Entity e, int value)
luaFunc(entity_setFlag)
{
	Entity *e = entity(L);
	int v = lua_tointeger(L, 2);
	if (e)
	{
		dsq->continuity.setEntityFlag(game->sceneName, e->getID(), v);
	}
	luaReturnNil();
}

/// entity_setPoison(Entity e, float amount, float time)
luaFunc(entity_setPoison)
{
	Entity *e = entity(L);
	if(e)
		e->setPoison(lua_tonumber(L, 2), lua_tonumber(L, 3));
	luaReturnNil();
}

/// entity_getPoison(Entity e) -> float!
luaFunc(entity_getPoison)
{
	Entity *e = entity(L);
	luaReturnNum(e ? e->getPoison() : 0.0f);
}

/// entity_getFlag(Entity e) -> int!
luaFunc(entity_getFlag)
{
	Entity *e = entity(L);
	int ret = 0;
	if (e)
	{
		ret = dsq->continuity.getEntityFlag(game->sceneName, e->getID());
	}
	luaReturnNum(ret);
}

/// isFlag(Flag|string flag, int value) -> bool!
luaFunc(isFlag)
{
	int v = 0;

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

	luaReturnBool(f);
}

/// avatar_updatePosition()
luaFunc(avatar_updatePosition)
{
	if(Avatar *a = game->avatar)
		a->updatePosition();
	luaReturnNil();
}

/// avatar_toggleMovement(boolx on)
luaFunc(avatar_toggleMovement)
{
	if(Avatar *a = game->avatar)
		a->toggleMovement(getBool(L));
	luaReturnNil();
}

/// clearShots()
luaFunc(clearShots)
{
	Shot::killAllShots();
	luaReturnNil();
}

/// clearHelp()
luaFunc(clearHelp)
{
	float t = 0.4f;

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

/// setLiPower(float power, float time)
luaFunc(setLiPower)
{
	float m = lua_tonumber(L, 1);
	float t = lua_tonumber(L, 2);
	dsq->continuity.setLiPower(m, t);
	luaReturnNil();
}

/// getLiPower() -> float!
luaFunc(getLiPower)
{
	luaReturnNum(dsq->continuity.liPower);
}

/// getPetPower() -> float!
luaFunc(getPetPower)
{
	luaReturnNum(dsq->continuity.petPower);
}

/// appendUserDataPath(string path) -> string!
luaFunc(appendUserDataPath)
{
	std::string path = getString(L, 1);

	if (!dsq->getUserDataFolder().empty())
		path = dsq->getUserDataFolder() + "/" + path;

	luaReturnStr(path.c_str());
}

/// getScreenVirtualOff() -> int!, int
luaFunc(getScreenVirtualOff)
{
	luaReturnVec2(core->getVirtualOffX(), core->getVirtualOffY());
}

/// getScreenSize() -> int!, int
luaFunc(getScreenSize)
{
	luaReturnVec2(core->width, core->height);
}

/// getScreenVirtualSize() -> int!, int
luaFunc(getScreenVirtualSize)
{
	luaReturnVec2(core->getVirtualWidth(), core->getVirtualHeight());
}

/// isMiniMapCursorOkay() -> bool!
luaFunc(isMiniMapCursorOkay)
{
	luaReturnBool(dsq->isMiniMapCursorOkay());
}

/// isShuttingDownGameState() -> bool!
luaFunc(isShuttingDownGameState)
{
	luaReturnBool(game->isShuttingDownGameState());
}

static PathFinding::State *getPathfinder(lua_State *L, int idx = 1)
{
	return *(PathFinding::State**)luaL_checkudata(L, idx, "pathfinder");
}


// Leaves 2 tables on the Lua stack
static int _fillPathfindTables(lua_State *L, VectorPath& path, int xs_idx, int ys_idx)
{
	const unsigned num = (unsigned)path.getNumPathNodes();

	if(lua_istable(L, xs_idx))
		lua_pushvalue(L, xs_idx);
	else
		lua_createtable(L, num, 0);

	if(lua_istable(L, ys_idx))
		lua_pushvalue(L, ys_idx);
	else
		lua_createtable(L, num, 0);

	// [..., xs, yx]
	for(unsigned i = 0; i < num; ++i)
	{
		const VectorPathNode *n = path.getPathNode(i);
		lua_pushnumber(L, n->value.x);  // [..., xs, ys, x]
		lua_rawseti(L, -3, i+1);        // [..., xs, ys]
		lua_pushnumber(L, n->value.y);  // [..., xs, ys, y]
		lua_rawseti(L, -2, i+1);        // [..., xs, ys]
	}
	// terminate tables
	lua_pushnil(L);            // [..., xs, ys, nil]
	lua_rawseti(L, -3, num+1); // [..., xs, ys]
	lua_pushnil(L);            // [..., xs, ys, nil]
	lua_rawseti(L, -2, num+1); // [..., xs, ys]
	return 2;
}

static int _pathfindDelete(lua_State *L)
{
	PathFinding::State *state = getPathfinder(L);
	PathFinding::deleteFindPath(state);
	return 0;
}

// startx, starty, endx, endy [, step, xtab, ytab, obsMask]
/// findPath(float x1, float y1, float x2, float y2, int step = 0, [float]? xs, [float]? ys, int obs = 0) -> int|bool, [float]?, [float]?
/// Returns false, or (node count, xs, ys); xs/ys is reused if passed. obs: OT_* mask, 0 = OT_BLOCKING
luaFunc(findPath)
{
	VectorPath path;
	Vector start(lua_tonumber(L, 1), lua_tonumber(L, 2));
	Vector end(lua_tonumber(L, 3), lua_tonumber(L, 4));
	ObsType obs = ObsType(lua_tointeger(L, 8));
	if(!PathFinding::generatePathSimple(path, start, end, lua_tointeger(L, 5), obs))
		luaReturnBool(false);

	const unsigned num = (unsigned)path.getNumPathNodes();
	lua_pushinteger(L, num);

	return 1 + _fillPathfindTables(L, path, 6, 7); // found path?, x positions, y positions
}

/// createFindPath() -> pathfinder!
luaFunc(createFindPath)
{
	PathFinding::State **statep = (PathFinding::State**)lua_newuserdata(L, sizeof(void*));
	*statep = PathFinding::initFindPath();
	luaL_getmetatable(L, "pathfinder");
	lua_setmetatable(L, -2);
	return 1;
}

/// findPathBegin(pathfinder pf, float x1, float y1, float x2, float y2, int obs = 0)
luaFunc(findPathBegin)
{
	PathFinding::State *state = getPathfinder(L);
	Vector start(lua_tonumber(L, 2), lua_tonumber(L, 3));
	Vector end(lua_tonumber(L, 4), lua_tonumber(L, 5));
	ObsType obs = ObsType(lua_tointeger(L, 6));
	PathFinding::beginFindPath(state, start, end, obs);
	luaReturnNil();
}

/// findPathUpdate(pathfinder pf, int limit) -> bool
/// true when done
luaFunc(findPathUpdate)
{
	PathFinding::State *state = getPathfinder(L);
	int limit = lua_tointeger(L, 2);
	bool done = PathFinding::updateFindPath(state, limit);
	luaReturnBool(done);
}

/// findPathFinish(pathfinder pf, int step = 0, [float]? xs, [float]? ys) -> int|bool, [float]?, [float]?
luaFunc(findPathFinish)
{
	PathFinding::State *state = getPathfinder(L);
	VectorPath path;
	bool found = PathFinding::finishFindPath(state, path, lua_tointeger(L, 2));
	if(!found)
		luaReturnBool(false);

	lua_pushinteger(L, (int)path.getNumPathNodes());
	return 1 + _fillPathfindTables(L, path, 3, 4);
}

/// findPathGetStats(pathfinder pf) -> int!, int
luaFunc(findPathGetStats)
{
	PathFinding::State *state = getPathfinder(L);
	unsigned stepsDone, nodesExpanded;
	PathFinding::getStats(state, stepsDone, nodesExpanded);
	lua_pushinteger(L, stepsDone);
	lua_pushinteger(L, nodesExpanded);
	return 2;
}

/// findPathFreeMemory(pathfinder pf)
luaFunc(findPathFreeMemory)
{
	PathFinding::State *state = getPathfinder(L);
	PathFinding::purgeFindPath(state);
	luaReturnNil();
}

// LUAWTF-NOTE(weird): hit test uses getGridRaw() & obs, but returns getGrid() (non-raw), which can be 0 for a raw-only hit. Steps are truncated (length / TILE_SIZE), so the last partial step and the end tile are never tested.
/// castLine(float x1, float y1, float x2, float y2, int obs = 0, boolx invert = false) -> int|bool, float, float
/// Returns the grid value (or false if nothing hit) and the hit/end position. obs 0 = OT_BLOCKING
luaFunc(castLine)
{
	Vector v(lua_tonumber(L, 1), lua_tonumber(L, 2));
	Vector end(lua_tonumber(L, 3), lua_tonumber(L, 4));
	int tiletype = lua_tointeger(L, 5);
	if(!tiletype)
		tiletype = OT_BLOCKING;
	bool invert = getBool(L, 6);
	Vector step = end - v;
	int steps = step.getLength2D() / TILE_SIZE;
	step.setLength2D(TILE_SIZE);

	if(!invert)
	{
		for(int i = 0; i < steps; ++i)
		{
			if(game->getGridRaw(TileVector(v)) & tiletype)
			{
				lua_pushinteger(L, game->getGrid(TileVector(v)));
				lua_pushnumber(L, v.x);
				lua_pushnumber(L, v.y);
				return 3;
			}
			v += step;
		}
	}
	else
	{
		for(int i = 0; i < steps; ++i)
		{
			if(!(game->getGridRaw(TileVector(v)) & tiletype))
			{
				lua_pushinteger(L, game->getGrid(TileVector(v)));
				lua_pushnumber(L, v.x);
				lua_pushnumber(L, v.y);
				return 3;
			}
			v += step;
		}
	}

	lua_pushboolean(L, false);
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 3;
}

/// getUserInputString(string label, string default = "") -> string!
luaFunc(getUserInputString)
{
	luaReturnStr(dsq->getUserInputString(getString(L, 1), getString(L, 2), true).c_str());
}

/// getMaxCameraValues() -> float!, float, float, float
luaFunc(getMaxCameraValues)
{
	luaReturnVec4(game->cameraMin.x, game->cameraMin.y, game->cameraMax.x, game->cameraMax.y);
}


/// inv_isFull(string name) -> bool!
luaFunc(inv_isFull)
{
	IngredientData *data = dsq->continuity.getIngredientDataByName(getString(L, 1));
	bool full = false;
	if(data)
		full = dsq->continuity.isIngredientFull(data);
	luaReturnBool(full);
}

/// inv_getMaxAmount(string name) -> int!
luaFunc(inv_getMaxAmount)
{
	IngredientData *data = dsq->continuity.getIngredientDataByName(getString(L, 1));
	luaReturnInt(data ? data->maxAmount : 0);
}

/// inv_getAmount(string name) -> int!
luaFunc(inv_getAmount)
{
	IngredientData *data = dsq->continuity.getIngredientDataByName(getString(L, 1));
	luaReturnInt(data ? data->amount : 0);
}

/// inv_add(string name, int amount)
/// Does nothing for amount <= 0
luaFunc(inv_add)
{
	IngredientData *data = dsq->continuity.getIngredientDataByName(getString(L, 1));
	int amount = lua_tointeger(L, 2);
	if(data && amount > 0)
		dsq->continuity.pickupIngredient(data, amount, false, false);
	luaReturnNil();
}

/// inv_getGfx(string name) -> string!
/// getIngredientGfx(string name) -> string!
luaFunc(inv_getGfx)
{
	luaReturnStr(dsq->continuity.getIngredientGfx(getString(L, 1)).c_str());
}

/// inv_remove(string name)
/// Removes one item.
luaFunc(inv_remove)
{
	IngredientData *data = dsq->continuity.getIngredientDataByName(getString(L, 1));
	if(data && data->amount > 0)
	{
		data->amount--;
		dsq->continuity.removeEmptyIngredients();
		if(game->isInGameMenu())
			game->getInGameMenu()->refreshFoodSlots(true);
	}
	luaReturnNil();
}

/// inv_getType(string name) -> IngredientType!
luaFunc(inv_getType)
{
	IngredientData *data = dsq->continuity.getIngredientDataByName(getString(L, 1));
	luaReturnInt(data ? data->type : IT_NONE);
}

/// inv_getDisplayName(string name) -> string!
luaFunc(inv_getDisplayName)
{
	IngredientData *data = dsq->continuity.getIngredientDataByName(getString(L, 1));
	luaReturnStr(data ? data->displayName.c_str() : "");
}

/// inv_pickupEffect(string name)
luaFunc(inv_pickupEffect)
{
	IngredientData *data = dsq->continuity.getIngredientDataByName(getString(L, 1));
	if(data)
		game->pickupIngredientEffects(data);
	luaReturnNil();
}

/// inv_getNumItems() -> int!
luaFunc(inv_getNumItems)
{
	luaReturnInt(dsq->continuity.getIngredientHeldSize());
}

/// inv_getItemName(int idx) -> string!
/// idx is 0-based; "" if out of range.
luaFunc(inv_getItemName)
{
	IngredientData *data = dsq->continuity.getIngredientHeldByIndex(lua_tointeger(L, 1));
	luaReturnStr(data ? data->name.c_str() : "");
}


/// getIngredientDataSize() -> int!
luaFunc(getIngredientDataSize)
{
	luaReturnInt(dsq->continuity.getIngredientDataSize());
}

/// getIngredientDataName(int idx) -> string!
/// idx is 0-based; "" if out of range.
luaFunc(getIngredientDataName)
{
	IngredientData *data = dsq->continuity.getIngredientDataByIndex(lua_tointeger(L, 1));
	luaReturnStr(data ? data->name.c_str() : "");
}

/// learnRecipe(string name, boolx show = false)
luaFunc(learnRecipe)
{
	std::string name = getString(L, 1);
	bool show = getBool(L, 2);
	dsq->continuity.learnRecipe(name, show);
	luaReturnNil();
}

/// setBGGradient(float r1, float g1, float b1, float r2, float g2, float b2, boolx horizontal = false)
luaFunc(setBGGradient)
{
	if(!game->grad)
		game->createGradient();
	Vector c1(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
	Vector c2(lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	if(getBool(L, 7))
		game->grad->makeHorizontal(c1, c2);
	else
		game->grad->makeVertical(c1, c2);
	luaReturnNil();
}

/// createHair(int nodes, float segLen, float width, string tex, Layer|int layer = 0) -> Hair|NoObj!
/// layer 0 = LR_ENTITIES, < 0 = not added to the scene.
luaFunc(createHair)
{
	lua_Integer nodes = lua_tointeger(L, 1);
	Hair *h = NULL;
	if(nodes > 0)
	{
		h = new Hair(nodes, lua_tonumber(L, 2), lua_tonumber(L, 3));
		h->setTexture(getString(L, 4));
		int layer = lua_tointeger(L, 5);
		if(!layer)
			layer = LR_ENTITIES;
		if(layer > 0)
			game->addRenderObject(h, layer);
	}
	luaReturnPtr(h);
}

/// hair_getHairPosition(Hair h, int idx) -> float!, float
/// idx is 0-based; 0, 0 if out of range.
luaFunc(hair_getHairPosition)
{
	Hair *h = hair(L);
	Vector p;
	size_t idx = lua_tointeger(L, 2);
	if (h)
	{
		if(idx < h->_points.size())
		{
			p = h->_points[idx];
		}
	}
	luaReturnVec2(p.x, p.y);
}

// hair x y z
/// hair_setHeadPosition(Hair h, float x, float y)
luaFunc(hair_setHeadPosition)
{
	Hair *h = hair(L);
	if (h)
	{
		h->setHeadPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	}
	luaReturnNil();
}

/// hair_update(Hair h, float? ignored)
/// Extra arguments (old scripts pass dt) are ignored.
luaFunc(hair_update)
{
	Hair *h = hair(L);
	if (h)
	{
		h->updatePositions();
	}
	luaReturnNil();
}

// hair x y dt
/// hair_exertForce(Hair h, float fx, float fy, float dt, int usePerc = 0)
luaFunc(hair_exertForce)
{
	Hair *h = hair(L);
	if (h)
	{
		h->exertForce(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tointeger(L, 5));
	}
	luaReturnNil();
}

// hair idx x y dt
/// hair_exertSegmentForce(Hair h, int idx, float fx, float fy, float dt, int usePerc = 0)
luaFunc(hair_exertSegmentForce)
{
	Hair *h = hair(L);
	if (h)
	{
		h->exertNodeForce(lua_tointeger(L, 2), Vector(lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5), lua_tointeger(L, 6));
	}
	luaReturnNil();
}

/// hair_setTextureFlip(Hair h, boolx flip)
luaFunc(hair_setTextureFlip)
{
	Hair *h = hair(L);
	if (h)
		h->setTextureFlip(getBool(L, 2));
	luaReturnNil();
}

/// hair_setWidth(Hair h, float width)
luaFunc(hair_setWidth)
{
	Hair *h = hair(L);
	if (h)
		h->setHairWidth(lua_tonumber(L, 2));
	luaReturnNil();
}

/// hair_setSegmentLength(Hair h, float minlen, float maxlen = 0)
luaFunc(hair_setSegmentLength)
{
	Hair *h = hair(L);
	if (h)
	{
		h->segmentMinLength = lua_tonumber(L, 2);
		h->segmentMaxLength = std::max(h->segmentMinLength, (float)lua_tonumber(L, 3));
	}
	luaReturnNil();
}

/// hair_getNumNodes(Hair h) -> int!
luaFunc(hair_getNumNodes)
{
	Hair *h = hair(L);
	size_t n = h ? h->_points.size() : 0;
	luaReturnInt(n);
}

/// hair_setNodeWidthMult(Hair h, int idx, float mult)
/// idx is 0-based
luaFunc(hair_setNodeWidthMult)
{
	Hair *h = hair(L);
	size_t idx = lua_tointeger(L, 2);
	float m = lua_tonumber(L, 3);
	if (h)
	{
		if(idx < h->_points.size())
			h->_points[idx].z = m;
	}
	luaReturnNil();
}

/// createDebugText(string text, int size, float x = 0, float y = 0) -> Text
luaFunc(createDebugText)
{
	DebugFont *txt = new DebugFont(lua_tointeger(L, 2), getString(L, 1));
	txt->position = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4));
	game->addRenderObject(txt, LR_DEBUG_TEXT);
	luaReturnPtr(txt);
}

/// createBitmapText(string text, int size, float x = 0, float y = 0) -> Text
luaFunc(createBitmapText)
{
	BitmapText *txt = new BitmapText(dsq->smallFont);
	txt->setText(getString(L, 1));
	txt->setFontSize(lua_tointeger(L, 2));
	txt->position = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4));
	game->addRenderObject(txt, LR_HUD);
	luaReturnPtr(txt);
}

static int createArialText(lua_State *L, TTFFont *font)
{
	TTFText *txt = new TTFText(font);
	txt->setText(getString(L, 1));
	//txt->setFontSize(lua_tointeger(L, 2)); // not supported; param is ignored for API compat with the create*Text functions
	txt->position = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4));
	game->addRenderObject(txt, LR_HUD);
	luaReturnPtr(txt);
}

/// createArialTextBig(string text, int? sizeIgnored, float x = 0, float y = 0) -> Text
/// size is ignored.
luaFunc(createArialTextBig)
{
	return createArialText(L, &dsq->fontArialBig);
}

/// createArialTextSmall(string text, int? sizeIgnored, float x = 0, float y = 0) -> Text
/// size is ignored.
luaFunc(createArialTextSmall)
{
	return createArialText(L, &dsq->fontArialSmall);
}


/// text_setText(Text t, string text)
luaFunc(text_setText)
{
	BaseText *txt = getText(L);
	if (txt)
		txt->setText(getString(L, 2));
	luaReturnNil();
}

/// text_setFontSize(Text t, float size)
luaFunc(text_setFontSize)
{
	BaseText *txt = getText(L);
	if (txt)
		txt->setFontSize(lua_tonumber(L, 2));
	luaReturnNil();
}

/// text_setWidth(Text t, float width)
luaFunc(text_setWidth)
{
	BaseText *txt = getText(L);
	if (txt)
		txt->setWidth(lua_tonumber(L, 2));
	luaReturnNil();
}

/// text_setAlign(Text t, Align align)
luaFunc(text_setAlign)
{
	BaseText *txt = getText(L);
	if (txt)
		txt->setAlign((Align)lua_tointeger(L, 2));
	luaReturnNil();
}

/// text_getHeight(Text t) -> float!
luaFunc(text_getHeight)
{
	BaseText *txt = getText(L);
	luaReturnNum(txt ? txt->getHeight() : 0.0f);
}

/// text_getLineHeight(Text t) -> float!
luaFunc(text_getLineHeight)
{
	BaseText *txt = getText(L);
	luaReturnNum(txt ? txt->getLineHeight() : 0.0f);
}

/// text_getNumLines(Text t) -> int!
luaFunc(text_getNumLines)
{
	BaseText *txt = getText(L);
	luaReturnInt(txt ? txt->getNumLines() : 0);
}

/// text_getStringWidth(Text t, string s) -> float!
luaFunc(text_getStringWidth)
{
	BaseText *txt = getText(L);
	luaReturnNum(txt ? txt->getStringWidth(getString(L, 2)) : 0.0f);
}

/// text_getActualWidth(Text t) -> float!
luaFunc(text_getActualWidth)
{
	BaseText *txt = getText(L);
	luaReturnNum(txt ? txt->getActualWidth() : 0.0f);
}

/// loadShader(string|nil vert, string? frag) -> Shader|NoObj
/// Returns a shader handle, 0 on failure.
luaFunc(loadShader)
{
	int handle = 0;
	const char *vertRaw = getCString(L, 1);
	const char *fragRaw = getCString(L, 2);
	std::string vert, frag;
	if(vertRaw)
		findFile_helper(vertRaw, vert);
	if(fragRaw)
		findFile_helper(fragRaw, frag);

	if(core->afterEffectManager)
		handle = core->afterEffectManager->loadShaderFile(vert.c_str(), frag.c_str());

	luaReturnInt(handle);
}

/// createShader(string|nil vert, string? frag) -> Shader|NoObj
/// Shader from source strings; returns a handle >= 1, 0 on failure.
luaFunc(createShader)
{
	int handle = 0;
	if(core->afterEffectManager)
		handle = core->afterEffectManager->loadShaderSrc(getCString(L, 1), getCString(L, 2));
	luaReturnInt(handle);
}

/// shader_setAsAfterEffect(Shader handle, int pos) -> bool
/// pos is 0-based; handle 0 clears the slot.
luaFunc(shader_setAsAfterEffect)
{
	int handle = getShaderHandle(L);
	int pos = lua_tointeger(L, 2);
	bool done = false;

	if(core->afterEffectManager)
		done = core->afterEffectManager->setShaderPipelinePos(handle, pos);

	luaReturnBool(done);
}

// LUAWTF-NOTE(bug): negative n goes through lua_tointeger into size_t: `shaderPipeline.resize(SIZE_MAX)` throws/crashes
/// shader_setNumAfterEffects(int n)
luaFunc(shader_setNumAfterEffects)
{
	lua_Integer n = lua_tointeger(L, 1);
	if(core->afterEffectManager && n >= 0)
		core->afterEffectManager->setShaderPipelineSize(n);
	luaReturnNil();
}

/// shader_setInt(Shader handle, string name, int x, int y = 0, int z = 0, int w = 0)
luaFunc(shader_setInt)
{
	Shader *sh = getShaderPtr(L);
	const char *name = getCString(L, 2);
	if(sh && name)
		sh->setInt(name, lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	luaReturnNil();
}

/// shader_setFloat(Shader handle, string name, float x, float y = 0, float z = 0, float w = 0)
luaFunc(shader_setFloat)
{
	Shader *sh = getShaderPtr(L);
	const char *name = getCString(L, 2);
	if(sh && name)
		sh->setFloat(name, lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	luaReturnNil();
}

/// shader_delete(Shader handle)
luaFunc(shader_delete)
{
	if(core->afterEffectManager)
		core->afterEffectManager->deleteShader(getShaderHandle(L));
	luaReturnNil();
}

/// pe_start(ParticleEffect pe)
luaFunc(pe_start)
{
	ParticleEffect *pe = getParticle(L);
	if (pe)
		pe->start();
	luaReturnNil();
}

/// pe_stop(ParticleEffect pe)
luaFunc(pe_stop)
{
	ParticleEffect *pe = getParticle(L);
	if (pe)
		pe->stop();
	luaReturnNil();
}

/// pe_isRunning(ParticleEffect pe) -> bool!
luaFunc(pe_isRunning)
{
	ParticleEffect *pe = getParticle(L);
	luaReturnBool(pe && pe->isRunning());
}

/// pe_getName(ParticleEffect pe) -> string?!
luaFunc(pe_getName)
{
	ParticleEffect *pe = getParticle(L);
	if(pe)
		luaReturnStr(pe->name.c_str());
	luaReturnNil();
}


/// getPerformanceCounter() -> int!
luaFunc(getPerformanceCounter)
{
#if SDL_VERSION_ATLEAST(2,0,0)
	luaReturnNum((lua_Number)SDL_GetPerformanceCounter());
#else
	luaReturnNum((lua_Number)SDL_GetTicks());
#endif
}

/// getPerformanceFreq() -> int!
/// Ticks per second of getPerformanceCounter().
luaFunc(getPerformanceFreq)
{
#if SDL_VERSION_ATLEAST(2,0,0)
	luaReturnNum((lua_Number)SDL_GetPerformanceFrequency());
#else
	luaReturnNum((lua_Number)1000);
#endif
}

// ----------- Grid Quad -----------------------

/// createQuadGrid(int nx, int ny, string tex, int layer = 0) -> QuadGrid|NoObj
/// NoObj if nx or ny is 0.
luaFunc(createQuadGrid)
{
	QuadGrid *q = QuadGrid::New(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
	if(q)
		setupQuadCommon(L, q, 3);
	luaReturnPtr(q);
}

/// quadgrid_numPoints(QuadGrid q) -> int?!, int?
luaFunc(quadgrid_numPoints)
{
	QuadGrid *q = getQuadGrid(L);
	if(!q)
		luaReturnNil();
	lua_pushinteger(L, q->pointsX());
	lua_pushinteger(L, q->pointsY());
	return 2;
}

/// quadgrid_setPoint(QuadGrid q, int ix, int iy, float? x, float? y, float? u, float? v)
/// ix, iy are 0-based; raises "out of range". Omitted/nil values stay unchanged.
luaFunc(quadgrid_setPoint)
{
	QuadGrid *q = getQuadGrid(L);
	if(!q)
		luaReturnNil();
	size_t ix = luaL_checkinteger(L, 2);
	size_t iy = luaL_checkinteger(L, 3);
	if(ix < q->pointsX() && iy < q->pointsY())
	{
		QuadGrid::Point &p = (*q)(ix, iy);
		if(!lua_isnoneornil(L, 4))
			p.x = lua_tonumber(L, 4);
		if(!lua_isnoneornil(L, 5))
			p.y = lua_tonumber(L, 5);
		if(!lua_isnoneornil(L, 6))
			p.u = lua_tonumber(L, 6);
		if(!lua_isnoneornil(L, 7))
			p.v = lua_tonumber(L, 7);

		luaReturnNil();
	}
	return luaL_error(L, "out of range");
}

/// quadgrid_texOffset(QuadGrid q, float x, float y, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false)
luaFunc(quadgrid_texOffset)
{
	QuadGrid *q = getQuadGrid(L);
	if(q)
		interpolateVec2(L, q->texOffset, 2);
	luaReturnNil();
}

/// quadgrid_getTexOffset(QuadGrid q) -> float!, float
luaFunc(quadgrid_getTexOffset)
{
	QuadGrid *q = getQuadGrid(L);
	Vector v;
	if(q)
		v = q->texOffset;
	luaReturnVec2(v.x, v.y);
}

/// quadgrid_setPauseLevel(QuadGrid q, int level)
luaFunc(quadgrid_setPauseLevel)
{
	QuadGrid *q = getQuadGrid(L);
	if(q)
		q->pauseLevel = lua_tointeger(L, 2);
	luaReturnNil();
}

/// quadgrid_getPauseLevel(QuadGrid q) -> int!
luaFunc(quadgrid_getPauseLevel)
{
	QuadGrid *q = getQuadGrid(L);
	luaReturnInt(q ? q->pauseLevel : 0);
}

/// quadgrid_resetUV(QuadGrid q, float xmul = 1, float ymul = 1)
luaFunc(quadgrid_resetUV)
{
	QuadGrid *q = getQuadGrid(L);
	if(q)
	{
		const float xmul = luaL_optnumber(L, 2, 1);
		const float ymul = luaL_optnumber(L, 3, 1);
		q->resetUV(xmul, ymul);
	}
	luaReturnNil();
}

/// quadgrid_resetPos(QuadGrid q, float w = 1, float h = 1, float xoffs = 0, float yoffs = 0)
luaFunc(quadgrid_resetPos)
{
	QuadGrid *q = getQuadGrid(L);
	if(q)
	{
		const float w     = luaL_optnumber(L, 2, 1);
		const float h     = luaL_optnumber(L, 3, 1);
		const float xoffs = luaL_optnumber(L, 4, 0);
		const float yoffs = luaL_optnumber(L, 5, 0);
		q->resetPos(w, h, xoffs, yoffs);
	}
	luaReturnNil();
}

// ------------------------------------

/// createSplineQuad(int nodes, float width, float off, string tex, int layer = 0) -> SplineQuad|NoObj!
/// layer 0 becomes LR_ENTITIES
luaFunc(createSplineQuad)
{
	SplineQuad *q = NULL;
	lua_Integer nodes = lua_tointeger(L, 1);
	if(nodes > 0)
	{
		float width = lua_tonumber(L, 2);
		float off = lua_tonumber(L, 3);
		off = off * 0.5f + 0.5f; // Transform [-1 .. 0 .. 1] into [0 .. 0.5 .. 1]
		q = new SplineQuad(nodes, width, off);
		q->setTexture(getString(L, 4));
		int layer = lua_tointeger(L, 5);
		if(!layer)
			layer = LR_ENTITIES;
		if(layer > 0)
			game->addRenderObject(q, layer);
	}
	luaReturnPtr(q);
}

/// splinequad_setInterpolation(SplineQuad q, int ctrlp, int deg, boolx interpolate = false) -> bool
luaFunc(splinequad_setInterpolation)
{
	SplineQuad *q = splineq(L);
	bool ok = false;
	if(q)
	{
		size_t ctrlp = lua_tointeger(L, 2);
		int deg = lua_tointeger(L, 3);
		bool interpolate = getBool(L, 4);
		ok = q->init(ctrlp, interpolate, deg);
	}
	luaReturnBool(ok);
}

/// splinequad_setControlPoint(SplineQuad q, int idx, float x, float y, float w)
luaFunc(splinequad_setControlPoint)
{
	SplineQuad *q = splineq(L);
	if(q)
	{
		size_t idx = lua_tointeger(L, 2);
		float x = lua_tonumber(L, 3);
		float y = lua_tonumber(L, 4);
		float w = lua_tonumber(L, 5);
		if(idx < q->numcp())
			(*q)(idx) = Vector(x, y, w);
	}
	luaReturnNil();
}

// ---------- Minimap related ------------------

/// getMinimapRender() -> RenderObject|NoObj!
luaFunc(getMinimapRender)
{
	luaReturnPtr(game->miniMapRender);
}

/// minimap_setWaterBitTex(string tex) -> bool
luaFunc(minimap_setWaterBitTex)
{
	luaReturnBool(MiniMapRender::setWaterBitTex(getString(L)));
}
/// minimap_setTopTex(string tex) -> bool
luaFunc(minimap_setTopTex)
{
	luaReturnBool(MiniMapRender::setTopTex(getString(L)));
}
/// minimap_setBottomTex(string tex) -> bool
luaFunc(minimap_setBottomTex)
{
	luaReturnBool(MiniMapRender::setBottomTex(getString(L)));
}
/// minimap_setAvatarIconTex(string tex) -> bool
luaFunc(minimap_setAvatarIconTex)
{
	luaReturnBool(MiniMapRender::setAvatarTex(getString(L)));
}
/// minimap_setHealthBarTex(string tex) -> bool
luaFunc(minimap_setHealthBarTex)
{
	luaReturnBool(MiniMapRender::setHealthBarTex(getString(L)));
}
/// minimap_setMaxHealthMarkerTex(string tex) -> bool
luaFunc(minimap_setMaxHealthMarkerTex)
{
	luaReturnBool(MiniMapRender::setMaxHealthMarkerTex(getString(L)));
}

template<typename T>
int mmicon_delete(lua_State *L, T *obj)
{
	if(obj)
	{
		delete obj->minimapIcon;
		obj->minimapIcon = NULL;
	}
	luaReturnNil();
}
template<typename T>
int mmicon_tex(lua_State *L, T *obj)
{
	if(!obj)
		luaReturnNil();
	MinimapIcon *ico = obj->ensureMinimapIcon();
	bool good = ico->setTexture(getString(L, 2));
	luaReturnBool(good);
}
template<typename T>
int mmicon_size(lua_State *L, T *obj)
{
	if(!obj)
		luaReturnNil();
	luaReturnNum(interpolateVec2(L, obj->ensureMinimapIcon()->size, 2));
}
template<typename T>
int mmicon_color(lua_State *L, T *obj)
{
	if(!obj)
		luaReturnNil();
	luaReturnNum(interpolateVec3(L, obj->ensureMinimapIcon()->color, 2));
}
template<typename T>
int mmicon_alpha(lua_State *L, T *obj)
{
	if(!obj)
		luaReturnNil();
	luaReturnNum(interpolateVec1(L, obj->ensureMinimapIcon()->alpha, 2));
}
template<typename T>
int mmicon_scaleWithDistance(lua_State *L, T *obj)
{
	if(!obj)
		luaReturnNil();
	obj->ensureMinimapIcon()->scaleWithDistance = getBool(L, 2);
	luaReturnNil();
}
template<typename T>
int mmicon_throb(lua_State *L, T *obj)
{
	if(!obj)
		luaReturnNil();
	obj->ensureMinimapIcon()->throbMult = lua_tonumber(L, 2);
	luaReturnNil();
}

// LUAWTF-NOTE(bug): mmicon_delete has no `if(!obj)` check unlike the other mmicon_* helpers: an invalid entity/node (0) dereferences NULL (`obj->minimapIcon`)
/// entity_mmicon_delete(Entity e)
luaFunc(entity_mmicon_delete) { return mmicon_delete(L, entity(L)); }
/// entity_mmicon_tex(Entity e, string tex) -> bool?
luaFunc(entity_mmicon_tex) { return mmicon_tex(L, entity(L)); }
/// entity_mmicon_size(Entity e, float w, float h, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false) -> float?
luaFunc(entity_mmicon_size) { return mmicon_size(L, entity(L)); }
/// entity_mmicon_color(Entity e, float r, float g, float b, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false) -> float?
luaFunc(entity_mmicon_color) { return mmicon_color(L, entity(L)); }
/// entity_mmicon_alpha(Entity e, float a, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false) -> float?
luaFunc(entity_mmicon_alpha) { return mmicon_alpha(L, entity(L)); }
/// entity_mmicon_scaleWithDistance(Entity e, boolx on)
luaFunc(entity_mmicon_scaleWithDistance) { return mmicon_scaleWithDistance(L, entity(L)); }
/// entity_mmicon_throb(Entity e, float mult)
luaFunc(entity_mmicon_throb) { return mmicon_throb(L, entity(L)); }

/// node_mmicon_delete(Node n)
luaFunc(node_mmicon_delete) { return mmicon_delete(L, path(L)); }
/// node_mmicon_tex(Node n, string tex) -> bool?
luaFunc(node_mmicon_tex) { return mmicon_tex(L, path(L)); }
/// node_mmicon_size(Node n, float w, float h, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false) -> float?
luaFunc(node_mmicon_size) { return mmicon_size(L, path(L)); }
/// node_mmicon_color(Node n, float r, float g, float b, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false) -> float?
luaFunc(node_mmicon_color) { return mmicon_color(L, path(L)); }
/// node_mmicon_alpha(Node n, float a, float time = 0, int loopType = 0, boolx pingPong = false, boolx ease = false) -> float?
luaFunc(node_mmicon_alpha) { return mmicon_alpha(L, path(L)); }
/// node_mmicon_scaleWithDistance(Node n, boolx on)
luaFunc(node_mmicon_scaleWithDistance) { return mmicon_scaleWithDistance(L, path(L)); }
/// node_mmicon_throb(Node n, float mult)
luaFunc(node_mmicon_throb) { return mmicon_throb(L, path(L)); }


class LuaXMLConverter : public tinyxml2::XMLVisitor
{
public:
	LuaXMLConverter(lua_State *lua) : L(lua) {}
	virtual ~LuaXMLConverter() {}

	virtual bool VisitEnter( const tinyxml2::XMLDocument& /*doc*/ )
	{
		lua_newtable(L);
		indexes.push_back(0);
		return true;
	}

	virtual bool VisitExit( const tinyxml2::XMLDocument& /*doc*/ )
	{
		indexes.pop_back();
		assert(indexes.empty());
		// Leave the root table on the stack
		return true;
	}

	virtual bool VisitEnter( const tinyxml2::XMLElement& element, const tinyxml2::XMLAttribute *a)
	{
		lua_newtable(L);
		indexes.push_back(0);
		if(a)
		{
			lua_newtable(L);
			for( ; a; a = a->Next())
			{
				lua_pushstring(L, a->Value());
				lua_setfield(L, -2, a->Name());
			}
			lua_setfield(L, -2, "attr");
		}
		if(const char *name = element.Value())
		{
			lua_pushstring(L, name);
			lua_setfield(L, -2, "name");
		}
		if(const char *text = element.GetText())
		{
			lua_pushstring(L, text);
			lua_setfield(L, -2, "text");
		}
		return true;
	}

	virtual bool VisitExit( const tinyxml2::XMLElement& /*element*/ )
	{
		indexes.pop_back();
		lua_rawseti(L, -2, ++indexes.back());
		return true;
	}

protected:
	std::vector<unsigned> indexes;
	lua_State * const L;
};

/// loadXMLTable(string file) -> table|bool?!, int?
/// Root = list of elements; element = {name = string, attr = {..}?, text = string?, [i] = child elements}. false, err on XML error.
luaFunc(loadXMLTable)
{
	const std::string s = getString(L);
	safePath(L, s);
	std::string fn;
	if(!findFile_helper(s.c_str(), fn))
		luaReturnNil();

	tinyxml2::XMLDocument xml;
	tinyxml2::XMLError err = readXML(fn, xml);
	if(err != tinyxml2::XML_SUCCESS)
	{
		lua_pushboolean(L, false);
		lua_pushinteger(L, err);
		return 2;
	}

	LuaXMLConverter cvt(L);
	xml.Accept(&cvt);
	return 1;
}

/// worldmap_forgetMap(string name) -> bool
luaFunc(worldmap_forgetMap)
{
	const std::string s = getString(L);
	bool ok = dsq->continuity.worldMap.forgetMap(s);
	luaReturnBool(ok);
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

	luaRegister(fileExists),
	luaRegister(getModName),
	luaRegister(getModPath),
	luaRegister(getInterfaceFunctionNames),

	luaRegister(debugBreak),
	luaRegister(setIgnoreAction),
	luaRegister(isIgnoreAction),
	luaRegister(sendAction),

	luaRegister(rumble),
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
	luaRegister(getOldDT),
	luaRegister(getDT),
	luaRegister(getFPS),
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
	luaRegister(dilateGrid),

	luaRegister(ing_hasIET),
	luaRegister(ing_getIngredientName),

	luaRegister(esetv),
	luaRegister(esetvf),
	luaRegister(egetv),
	luaRegister(egetvf),
	luaRegister(eisv),

	luaRegister(entity_addIgnoreShotDamageType),
	luaRegister(entity_ensureLimit),
	luaRegister(entity_getBoneLockEntity),
	luaRegister(entity_getBoneLockData),

	luaRegister(entity_setRidingPosition),
	luaRegister(entity_setRidingData),
	luaRegister(entity_getRidingPosition),
	luaRegister(entity_getRidingRotation),
	luaRegister(entity_getRidingFlip),
	luaRegister(entity_setBoneLock),
	luaRegister(entity_setBoneLockRotation),
	luaRegister(entity_setBoneLockOffset),
	luaRegister(entity_setBoneLockDelay),
	luaRegister(entity_getBoneLockDelay),
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
	luaRegister(entity_setPauseFreeze),
	luaRegister(node_setSpiritFreeze),
	luaRegister(node_setPauseFreeze),

	luaRegister(entity_setCanLeaveWater),

	luaRegister(entity_pullEntities),

	luaRegister(entity_setEntityLayer),

	luaRegister(entity_clearTargetPoints),
	luaRegister(entity_addTargetPoint),


	luaRegister(entity_setCullRadius),

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

	luaRegister(avatar_setMaxBeastEats),

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
    luaRegister(entity_setStopSoundsOnDeath),

	luaRegister(entity_setDamageTarget),
	luaRegister(entity_setAllDamageTargets),

	luaRegister(entity_isDamageTarget),
	luaRegister(entity_isValidTarget),

	luaRegister(entity_isUnderWater),
	luaRegister(entity_checkSplash),
	luaRegister(entity_isInCurrent),

	luaRegister(entity_getRandomTargetPoint),
	luaRegister(entity_getTargetPoint),
	luaRegister(entity_getNumTargetPoints),

	luaRegister(entity_setTargetRange),
	luaRegister(entity_getTargetRange),

	luaRegister(bone_addSegment),

	luaRegister(bone_setSegmentOffset),
	luaRegister(bone_setSegmentProps),
	luaRegister(bone_setSegmentChainHead),
	luaRegister(bone_setSegmentRotationInterpolationTime),
	luaRegister(bone_setAnimated),
	luaRegister(bone_showFrame),

	luaRegister(bone_lookAtEntity),
	luaRegister(bone_lookAtPosition),
	luaRegister(bone_spawnParticlesFromCollisionMask),
	luaRegister(bone_toggleCollision),

	luaRegister(entity_partSetSegs),

	luaRegister(entity_adjustPositionBySurfaceNormal),
	luaRegister(entity_applySurfaceNormalForce),

	luaRegister(entity_doElementInteraction),
	luaRegister(avatar_setElementEffectMult),

	luaRegister(createBeam),
	luaRegister(beam_setAngle),
	luaRegister(beam_setDamage),
	luaRegister(beam_setBeamWidth),
	luaRegister(beam_setFirer),
	luaRegister(beam_setDamageType),
	luaRegister(beam_getEndPos),

	luaRegister(getStringBank),

	luaRegister(isPlat),


	luaRegister(createEntity),
	luaRegister(entity_setWeight),
	luaRegister(entity_getWeight),

	luaRegister(entity_setActivationType),

	luaRegister(entity_v),
	luaRegister(node_v),
	luaRegister(shot_v),

	luaRegister(isQuitFlag),
	luaRegister(isDeveloperKeys),
	luaRegister(isDemo),

	luaRegister(isInputEnabled),
	luaRegister(disableInput),

	luaRegister(getInputMode),
	luaRegister(getJoystickAxisLeft),
	luaRegister(getJoystickAxisRight),

	luaRegister(setMousePos),
	luaRegister(getMousePos),
	luaRegister(getMouseWorldPos),
	luaRegister(getMouseWheelChange),
	luaRegister(setMouseConstraintCircle),
	luaRegister(setMouseConstraint),

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
	luaRegister(entity_getActivation),
	luaRegister(entity_rotateToEntity),

	luaRegister(entity_fireGas),
	luaRegister(entity_rotateToTarget),

	luaRegister(entity_switchSurfaceDirection),

	luaRegister(entity_moveAlongSurface),
	luaRegister(entity_rotateToSurfaceNormal),
	luaRegister(entity_clampToSurface),
	luaRegister(entity_checkSurface),
	luaRegister(entity_clampToHit),

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
	luaRegister(entity_getPushDamage),
	luaRegister(entity_msg),
	luaRegister(node_msg),
	luaRegister(shot_msg),
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
	luaRegister(entity_stopPull),
	luaRegister(entity_setTargetPriority),
	luaRegister(entity_getTargetPriority),

	luaRegister(entity_setEntityType),
	luaRegister(entity_getEntityType),

	luaRegister(entity_setSegmentTexture),

	luaRegister(entity_spawnParticlesFromCollisionMask),
	luaRegister(entity_initEmitter),
	luaRegister(entity_startEmitter),
	luaRegister(entity_stopEmitter),
	luaRegister(entity_getEmitter),
	luaRegister(entity_getNumEmitters),

	luaRegister(entity_initPart),
	luaRegister(entity_initSegments),
	luaRegister(entity_warpSegments),
	luaRegister(entity_initSkeletal),
	luaRegister(entity_loadSkin),
	luaRegister(entity_hasSkeletal),
	luaRegister(entity_getNumAnimLayers),
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

	luaRegister(entity_getRidingOnEntity),

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
	luaRegister(entity_getAnimationTime),
	luaRegister(entity_setAnimationTime),

	luaRegister(entity_setCurrentTarget),
	luaRegister(entity_stopInterpolating),

	luaRegister(entity_followPath),
	luaRegister(entity_followPathSpeed),
	luaRegister(entity_isFollowingPath),
	luaRegister(entity_sound),
	luaRegister(entity_playSfx),

	luaRegister(registerSporeDrop),

	luaRegister(spawnIngredient),
	luaRegister(spawnAllIngredients),
	luaRegister(spawnParticleEffect),
	luaRegister(spawnManaBall),

	luaRegister(isEscapeKey),

	luaRegister(resetTimer),

	luaRegister(addInfluence),
    luaRegister(getSuckPosition),
	luaRegister(setSuckPosition),
	luaRegister(setNumSuckPositions),
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
	luaRegister(toWorldFromWindow),

	luaRegister(toggleDamageSprite),

	luaRegister(toggleLiCombat),

	luaRegister(toggleCursor),
	luaRegister(toggleBlackBars),
	luaRegister(setBlackBarsColor),

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
	luaRegister(saveMenu),
	luaRegister(setSceneDisplayNameInSave),
	luaRegister(wait),
	luaRegister(watch),

	luaRegister(quitNestedMain),
	luaRegister(isNestedMain),


	luaRegister(screenMessage),
	luaRegister(centerText),
	luaRegister(watchForVoice),

	luaRegister(setLayerRenderPass),
	luaRegister(setElementLayerVisible),
	luaRegister(isElementLayerVisible),

	luaRegister(isWithin),



	luaRegister(pickupGem),
	luaRegister(setGemPosition),
	luaRegister(setGemName),
	luaRegister(setGemBlink),
	luaRegister(removeGem),
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
	luaRegister(isObstructedRaw),
	luaRegister(isObstructedBlock),
	luaRegister(getObstruction),
	luaRegister(getGridRaw),
	luaRegister(findPath),
	luaRegister(createFindPath),
	luaRegister(findPathBegin),
	luaRegister(findPathUpdate),
	luaRegister(findPathFinish),
	luaRegister(findPathGetStats),
	luaRegister(findPathFreeMemory),
	luaRegister(castLine),
	luaRegister(getUserInputString),
	luaRegister(getMaxCameraValues),

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
	luaRegister(avatar_isSwimming),
	luaRegister(avatar_isOnWall),
	luaRegister(avatar_isShieldActive),
	luaRegister(avatar_setShieldActive),
	luaRegister(avatar_getRollDirection),

	luaRegister(avatar_fallOffWall),
	luaRegister(avatar_setBlockSinging),
	luaRegister(avatar_isBlockSinging),
	luaRegister(avatar_setBlockBackflip),
	luaRegister(avatar_isBlockBackflip),

	luaRegister(avatar_setSpeedMult),
	luaRegister(avatar_setSpeedMult2),
	luaRegister(avatar_getSpeedMult),
	luaRegister(avatar_getSpeedMult2),


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
	luaRegister(filterNearestEntities),
	luaRegister(filterNearestEntitiesAdd),
	luaRegister(getNextFilteredEntity),
	luaRegister(getEntityList),
	luaRegister(entity_getEntityListInRange),
	luaRegister(vector_getEntityListInRange),

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
	luaRegister(getEntityToActivate),
	luaRegister(setEntityToActivate),
	luaRegister(hasThingToActivate),
	luaRegister(setActivation),

	luaRegister(entity_warpToNode),
	luaRegister(entity_moveToNode),
	luaRegister(entity_moveToNodeSpeed),

	luaRegister(cam_toNode),
	luaRegister(cam_snap),
	luaRegister(cam_toEntity),
	luaRegister(cam_setPosition),


	luaRegister(entity_flipToEntity),
	luaRegister(entity_flipToSame),

	luaRegister(entity_flipToNode),
	luaRegister(entity_flipToVel),

	luaRegister(entity_swimToNode),
	luaRegister(entity_swimToNodeSpeed),
	luaRegister(entity_swimToPosition),
	luaRegister(entity_swimToPositionSpeed),


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
	luaRegister(shot_getMaxSpeed),
	luaRegister(shot_setMaxSpeed),
	luaRegister(shot_getHomingness),
	luaRegister(shot_setHomingness),
	luaRegister(shot_getLifeTime),
	luaRegister(shot_setLifeTime),
	luaRegister(shot_setDamageType),
	luaRegister(shot_setCheckDamageTarget),
	luaRegister(shot_isCheckDamageTarget),
	luaRegister(shot_setTrailPrt),
	luaRegister(shot_setTargetPoint),
	luaRegister(shot_getTargetPoint),
	luaRegister(shot_canHitEntity),
	luaRegister(shot_setAlwaysMaxSpeed),
	luaRegister(shot_isAlwaysMaxSpeed),
	luaRegister(shot_setHitEnts),
	luaRegister(shot_getHitEnts),
	luaRegister(shot_setHitWalls),
	luaRegister(shot_getHitWalls),

	luaRegister(filterNearestShots),
	luaRegister(filterNearestShotsAdd),
	luaRegister(getNextFilteredShot),
	luaRegister(entity_pathBurst),
	luaRegister(entity_handleShotCollisions),
	luaRegister(entity_handleShotCollisionsSkeletal),
	luaRegister(entity_handleShotCollisionsHair),
	luaRegister(entity_collideSkeletalVsCircle),
	luaRegister(entity_collideSkeletalVsCirclePos),
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
	luaRegister(entity_exertHairSegmentForce),
	luaRegister(entity_setHairTextureFlip),
	luaRegister(entity_setHairWidth),

	luaRegister(entity_setName),

	luaRegister(getNumberOfEntitiesNamed),

	luaRegister(isNested),
	luaRegister(isSkippingCutscene),

	luaRegister(entity_idle),
	luaRegister(entity_stopAllAnimations),

	luaRegister(entity_getBoneByIdx),
	luaRegister(entity_getBoneByName),
	luaRegister(entity_getBoneByInternalId),
	luaRegister(entity_getNumBones),



	luaRegister(toggleInput),

	luaRegister(entity_setTarget),

	luaRegister(getScreenCenter),



	luaRegister(debugLog),
	luaRegister(errorLog),
	luaRegister(loadMap),

	luaRegister(loadSound),

	luaRegister(node_activate),
	luaRegister(entity_activate),
	luaRegister(node_getName),
	luaRegister(node_getLabel),
	luaRegister(node_getPathPosition),
	luaRegister(node_getPosition),
	luaRegister(node_setPosition),
	luaRegister(node_getContent),
	luaRegister(node_getAmount),
	luaRegister(node_getSize),
	luaRegister(node_getShape),

	luaRegister(toggleSteam),

	luaRegister(appendUserDataPath),

	luaRegister(setCutscene),
	luaRegister(isInCutscene),



	luaRegister(node_getNumEntitiesIn),


	luaRegister(entity_getName),
	luaRegister(entity_isName),


	luaRegister(node_setActivationRange),
	luaRegister(node_setCursorActivation),
	luaRegister(node_setCatchActions),

	luaRegister(node_setElementsInLayerActive),
	luaRegister(refreshElementsOnLayerCallback),
	luaRegister(refreshElementsWithTagCallback),


	luaRegister(entity_setHealth),
	luaRegister(entity_setCurrentHealth),
	luaRegister(entity_setMaxHealth),
	luaRegister(entity_changeHealth),

	luaRegister(node_setActive),
	luaRegister(node_isActive),
	luaRegister(node_setEmitter),
	luaRegister(node_getEmitter),


	luaRegister(setSceneColor),
	luaRegister(getSceneColor),
	luaRegister(setSceneColor2),
	luaRegister(getSceneColor2),

	luaRegister(entity_isEntityInRange),
	luaRegister(entity_isPositionInRange),

	luaRegister(entity_stopFollowingPath),
	luaRegister(entity_slowToStopPath),
	luaRegister(entity_isSlowingToStopPath),

	luaRegister(entity_findNearestEntityOfType),
	luaRegister(entity_resumePath),

	luaRegister(entity_generateCollisionMask),

	luaRegister(entity_isAnimating),
	luaRegister(entity_getAnimationName),
	luaRegister(entity_getAnimationLength),
	luaRegister(entity_hasAnimation),

	luaRegister(entity_setCull),

	luaRegister(entity_setFillGrid),
	luaRegister(entity_isFillGrid),

	luaRegister(entity_push),

	luaRegister(entity_alpha),

	luaRegister(findWall),
	luaRegister(findWall2),
	luaRegister(isUnderWater),


	luaRegister(overrideZoom),
	luaRegister(disableOverrideZoom),
	luaRegister(getZoom),
	luaRegister(setMaxLookDistance),



	luaRegister(spawnAroundEntity),

	luaRegister(entity_toggleBone),

	luaRegister(bone_getName),
	luaRegister(bone_isName),
	luaRegister(bone_getIndex),
	luaRegister(bone_getCollisionMaskNormal),
	luaRegister(node_x),
	luaRegister(node_y),
	luaRegister(node_isEntityPast),
	luaRegister(node_isEntityInRange),
	luaRegister(node_isPositionIn),



	luaRegister(entity_warpLastPosition),
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

	luaRegister(entity_getPushVec),


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
	luaRegister(getScreenVirtualSize),
	luaRegister(isMiniMapCursorOkay),
	luaRegister(isShuttingDownGameState),
	luaRegister(setBGGradient),

	luaRegister(createHair),
	luaRegister(hair_getHairPosition),
	luaRegister(hair_setHeadPosition),
	luaRegister(hair_update),
	luaRegister(hair_exertForce),
	luaRegister(hair_exertSegmentForce),
	luaRegister(hair_setTextureFlip),
	luaRegister(hair_setWidth),
	luaRegister(hair_setSegmentLength),
	luaRegister(hair_getNumNodes),
	luaRegister(hair_setNodeWidthMult),


	luaRegister(inv_isFull),
	luaRegister(inv_getMaxAmount),
	luaRegister(inv_getAmount),
	luaRegister(inv_add),
	luaRegister(inv_getGfx),
	luaRegister(inv_remove),
	luaRegister(inv_getType),
	luaRegister(inv_getDisplayName),
	luaRegister(inv_pickupEffect),
	luaRegister(inv_getNumItems),
	luaRegister(inv_getItemName),
	luaRegister(learnRecipe),
	luaRegister(getIngredientDataSize),
	luaRegister(getIngredientDataName),

	luaRegister(createDebugText),
	luaRegister(createBitmapText),
	luaRegister(createArialTextBig),
	luaRegister(createArialTextSmall),
	luaRegister(text_setText),
	luaRegister(text_setFontSize),
	luaRegister(text_setWidth),
	luaRegister(text_setAlign),
	luaRegister(text_getHeight),
	luaRegister(text_getStringWidth),
	luaRegister(text_getActualWidth),
	luaRegister(text_getLineHeight),
	luaRegister(text_getNumLines),

	luaRegister(loadShader),
	luaRegister(createShader),
	luaRegister(shader_setAsAfterEffect),
	luaRegister(shader_setNumAfterEffects),
	luaRegister(shader_setFloat),
	luaRegister(shader_setInt),
	luaRegister(shader_delete),

	luaRegister(pe_start),
	luaRegister(pe_stop),
	luaRegister(pe_isRunning),
	luaRegister(pe_getName),


	luaRegister(isQuad),
	luaRegister(isNode),
	luaRegister(isObject),
	luaRegister(isEntity),
	luaRegister(isScriptedEntity),
	luaRegister(isBone),
	luaRegister(isShot),
	luaRegister(isWeb),
	luaRegister(isIng),
	luaRegister(isBeam),
	luaRegister(isText),
	luaRegister(isShader),
	luaRegister(isParticleEffect),

	luaRegister(getPerformanceCounter),
	luaRegister(getPerformanceFreq),

	luaRegister(getMinimapRender),
	luaRegister(minimap_setWaterBitTex),
	luaRegister(minimap_setTopTex),
	luaRegister(minimap_setBottomTex),
    luaRegister(minimap_setAvatarIconTex),
    luaRegister(minimap_setHealthBarTex),
    luaRegister(minimap_setMaxHealthMarkerTex),
	luaRegister(entity_mmicon_delete),
	luaRegister(entity_mmicon_tex),
	luaRegister(entity_mmicon_size),
	luaRegister(entity_mmicon_color),
	luaRegister(entity_mmicon_alpha),
	luaRegister(entity_mmicon_scaleWithDistance),
	luaRegister(entity_mmicon_throb),
	luaRegister(node_mmicon_delete),
	luaRegister(node_mmicon_tex),
	luaRegister(node_mmicon_size),
	luaRegister(node_mmicon_color),
	luaRegister(node_mmicon_alpha),
	luaRegister(node_mmicon_scaleWithDistance),
	luaRegister(node_mmicon_throb),

	luaRegister(loadXMLTable),

	luaRegister(createQuadGrid),
	luaRegister(isQuadGrid),
	luaRegister(quadgrid_numPoints),
	luaRegister(quadgrid_setPoint),
	luaRegister(quadgrid_texOffset),
	luaRegister(quadgrid_getTexOffset),
	luaRegister(quadgrid_setPauseLevel),
	luaRegister(quadgrid_getPauseLevel),
	luaRegister(quadgrid_resetUV),
	luaRegister(quadgrid_resetPos),

	luaRegister(createSplineQuad),
	luaRegister(splinequad_setInterpolation),
	luaRegister(splinequad_setControlPoint),

	luaRegister(worldmap_forgetMap),

#undef MK_FUNC
#undef MK_ALIAS
#define MK_FUNC(base, getter, prefix, suffix) luaRegister(prefix##_##suffix),
#define MK_STR(s) #s
#define MK_ALIAS(prefix, suffix, alias) {MK_STR(prefix) "_" MK_STR(alias), l_##prefix##_##suffix},

	EXPAND_FUNC_PROTOTYPES

	// obj_* are not in the define above
	MAKE_ROBJ_FUNCS(_, obj)
	// same for quad_* base functions
	MAKE_QUAD_FUNCS(_, quad)

	// -- overrides / special cases--

	{"bone_getPosition", l_bone_getWorldPosition},
	{ "entity_delete", l_entity_delete_override },
	{ "beam_setPosition", l_beam_setPosition_override },

	// -- deprecated/compatibility related functions below here --

	luaRegister(entity_incrTargetLeaches),
	luaRegister(entity_decrTargetLeaches),
	{"entity_soundFreq", l_entity_sound},
	{"entity_interpolateTo", l_entity_setPosition},
	{"entity_isFlippedHorizontal", l_entity_isfh},
	{"entity_isFlippedVertical", l_entity_isfv},
	{"entity_rotateTo", l_entity_rotate},
	{"entity_setColor", l_entity_color},
	{"entity_setInternalOffset", l_entity_internalOffset},
	{"getIngredientGfx", l_inv_getGfx},

	{"bone_setColor", l_bone_color},

	{"node_setEffectOn", l_node_setActive},
	{"node_isEffectOn", l_node_isActive},
	{"entity_isRidingOnEntity", l_entity_getRidingOnEntity}

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
	{"AT_NONE",    Entity::ACT_NONE},
	{"AT_NORMAL",  Entity::ACT_CLICK},
	{"AT_CLICK",   Entity::ACT_CLICK},
	{"AT_RANGE",   Entity::ACT_RANGE},

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

	luaConstant(BLEND_DISABLED),
	luaConstant(BLEND_DEFAULT),
	luaConstant(BLEND_ADD),
	{"BLEND_ADDITIVE",			BLEND_ADD},
	luaConstant(BLEND_SUB),
	luaConstant(BLEND_MULT),

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
	luaConstant(OT_USER1),
	luaConstant(OT_USER2),
	luaConstant(OT_MASK_BLACK),
	luaConstant(OT_BLOCKING),
	luaConstant(OT_USER_MASK),
	luaConstant(OT_OUTOFBOUNDS),

	luaConstant(SEE_MAP_NEVER),
	luaConstant(SEE_MAP_DEFAULT),
	luaConstant(SEE_MAP_ALWAYS),

	luaConstant(ALIGN_CENTER),
	luaConstant(ALIGN_LEFT),

	luaConstant(PATHSHAPE_RECT),
	luaConstant(PATHSHAPE_CIRCLE),
};

template<typename T>
static void luaRegisterEnums(lua_State *L, T first, T last)
{
	assert(first <= last);
	for(T i = first; i <= last; i = T(i + 1))
	{
		if(const char *name = EnumName(i))
		{
			lua_pushinteger(L, i);
			lua_setglobal(L, name);
		}
	}
}

//============================================================================================
// F U N C T I O N S
//============================================================================================

ScriptInterface::ScriptInterface()
: baseState(NULL), _LA(NULL)
{
}

void ScriptInterface::init()
{
	bool devmode = dsq->isDeveloperKeys();

	// Everything on in dev mode, everything off otherwise.
	loudScriptErrors = devmode;
	complainOnGlobalVar = devmode;
	complainOnUndefLocal = devmode;

	allowUnsafeFunctions = dsq->user.system.allowDangerousScriptFunctions;

	if(!_LA)
		_LA = luaalloc_create(NULL, NULL);
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
	lua_State *state = lua_newstate(_LA ? luaalloc : NULL, _LA);	/* opens Lua */
	luaL_openlibs(state);

#ifdef LUAAPI_HAS_CLIPBOARD
	lua_getglobal(state, "os");
		lua_pushcfunction(state, l_os_getclipboard);
		lua_setfield(state, -2, "getclipboard");
		lua_pushcfunction(state, l_os_setclipboard);
		lua_setfield(state, -2, "setclipboard");
	lua_pop(state, 1);
#endif

	if(!allowUnsafeFunctions)
	{
		lua_pushnil(state);
		lua_setglobal(state, "os");
		lua_pushnil(state);
		lua_setglobal(state, "io");
		lua_pushnil(state);
		lua_setglobal(state, "package");
	}

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
	luaRegisterEnums(state, LR_ZERO, LR_MAX);
	luaRegisterEnums(state, ACTION_PRIMARY, ACTION_MAX);

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

	// Register Lua classes
	luaL_newmetatable(state, "pathfinder");
	lua_pushliteral(state, "__gc");
	lua_pushcfunction(state, _pathfindDelete);
	lua_settable(state, -3);
	lua_pop(state, 1);

	// All done, return the new state.
	return state;
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
	if (baseState)
	{
		lua_close(baseState);
		baseState = NULL;
	}
	if(_LA)
	{
		luaalloc_delete(_LA);
		_LA = NULL;
	}
}

Script *ScriptInterface::openScript(const std::string &file, bool ignoremissing /* = false */)
{
	std::string realFile = localisePathInternalModpath(file);
	realFile = adjustFilenameCase(realFile);
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

bool Script::call(const char *name, void *param1, int param2, int param3, int param4, bool *ret1)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	lua_pushinteger(L, param2);
	lua_pushinteger(L, param3);
	lua_pushinteger(L, param4);
	if (!doCall(4, 1))
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

bool Script::call(const char *name, void *param1, void *param2, int param3)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	luaPushPointer(L, param2);
	lua_pushinteger(L, param3);
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

bool Script::call(const char *name, void *param1, bool *ret1)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	if (!doCall(1, 1))
		return false;
	*ret1 = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return true;
}

bool Script::call(const char *name, void *param1, bool param2, bool *ret1)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	lua_pushboolean(L, param2);
	if (!doCall(2, 1))
		return false;
	*ret1 = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return true;
}

bool Script::call(const char *name, void *param1, void *param2, bool *ret1)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	luaPushPointer(L, param2);
	if (!doCall(2, 1))
		return false;
	*ret1 = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return true;
}

bool Script::call(const char* name, void* param1, void* param2, void* param3, bool* ret1)
{
	lookupFunc(name);
	luaPushPointer(L, param1);
	luaPushPointer(L, param2);
	luaPushPointer(L, param3);
	if (!doCall(3, 1))
		return false;
	*ret1 = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return true;
}


bool Script::call(const char *name, const char *param, bool *ret)
{
	lookupFunc(name);
	lua_pushstring(L, param);
	if (!doCall(1, 1))
		return false;
	*ret = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return true;
}

bool Script::call(const char *name, const char *param, std::string *ret)
{
	lookupFunc(name);
	lua_pushstring(L, param);
	if (!doCall(1, 1))
		return false;
	*ret = getString(L, -1);
	lua_pop(L, 1);
	return true;
}

bool Script::call(const char *name, const char *param1, const char *param2, const char *param3, std::string *ret)
{
	lookupFunc(name);
	lua_pushstring(L, param1);
	lua_pushstring(L, param2);
	lua_pushstring(L, param3);
	if (!doCall(3, 1))
		return false;
	*ret = getString(L, -1);
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

int Script::pushLocalVars(lua_State *Ltarget)
{
	pushLocalVarTab(Ltarget, L);
	return 1;
}

std::string ScriptInterface::MakeScriptFileName(const std::string& name, const char *subdir)
{
	if(name.empty())
		return name;

	std::string file;

	if (name[0]=='@' && dsq->mod.isActive())
	{
		file = dsq->mod.getPath() + "scripts/" + name.substr(1, name.size()) + ".lua";
		return file;
	}
	else if (dsq->mod.isActive())
	{
		file = dsq->mod.getPath() + "scripts/" + name + ".lua";
		if(exists(file))
			return file;
	}

	if(subdir && *subdir)
	{
		std::ostringstream os;
		os << "scripts/" << subdir << "/" << name << ".lua";
		file = os.str();
		if(exists(file))
			return file;
	}

	return "scripts/" + name + ".lua";
}
