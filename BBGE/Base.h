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
#ifndef BBGE_BASE_H
#define BBGE_BASE_H

#ifdef BBGE_BUILD_WINDOWS

	#define WIN32_LEAN_AND_MEAN
	#define WIN32_NOMINMAX
	#include <windows.h>
	#undef min
	#undef max

    #ifdef _MSC_VER
        #define strtof (float)strtod
        #define snprintf _snprintf
    #endif
#endif

#include "BBGECompileConfig.h"

#ifdef BBGE_BUILD_WINDOWS

	//#include "iprof/prof.h"
	//#define BBGE_PROF(x) Prof(x)

	#define BBGE_PROF(x)

/*
	//#ifdef BBGE_BUILD_DIRECTX
	#define DIRECTINPUT_VERSION 0x0800
	#include <dinput.h>
	//#endif
*/

#else
	#define BBGE_PROF(x)

#endif

#ifdef BBGE_BUILD_GLFW
	#include <glfw.h>
	#include "glext.h"
	//#include "glext.h"
#elif BBGE_BUILD_DIRECTX

	#if defined(BBGE_BUILD_X360) && !defined(BBGE_BUILD_WINDOWS)

		#include <xtl.h>
		#include <ppcintrinsics.h>

	#endif // _XBOX

	#if defined(BBGE_BUILD_X360) && defined(BBGE_BUILD_WINDOWS)
		// Using the win32\ path prefix on the D3D include files makes sure that the Xbox 360 
		// version of D3D is used, not the DirectX SDK version.
		#include <win32\vs2005\d3d9.h>
		#include <win32\vs2005\d3dx9.h>
		#pragma warning(disable:4100)

		#include "XTLOnPC.h"

	#endif // _PC

	#if defined(BBGE_BUILD_X360)
		#include <xgraphics.h>
		#include <xboxmath.h>
		#include <stdio.h>
		#include <stdarg.h>
		#include <assert.h>
	#endif
#endif

#ifdef BBGE_BUILD_SDL

	#include "SDL.h"

#endif

#if defined(BBGE_BUILD_OPENGL)
	#define GL_GLEXT_LEGACY 1
	#include "gl.h"
	#include "glext.h"
#endif

#define compile_assert(pred) switch(0){case 0:case (pred):;}

#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4005)
#pragma warning(disable:4305)

#pragma warning(disable:4018) // signed/unsigned mismatch
#pragma warning(disable:4244) // conversion from types with possible loss of data
#pragma warning(disable:4800) // forcing value to bool 'true' or 'false (performance warning)

//W4
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4505) // unreferenced local function has been removed
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4706) // assignment within conditional expression

#pragma warning(disable:4389) // signed/unsigned mismatch

#pragma warning(disable:4189) // UqqqqSEFUL: local variable is initialized but not referenced
#endif

#undef GetCharWidth

#include <string.h>

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>
#include <queue>
#include <map>
#include <stack>
//#include <typeinfo.h>

#include "Rect.h"

#include "math.h"
#include "FileAPI.h"

#ifdef BBGE_BUILD_LINUX
#  include <sys/types.h>
#  include <stdint.h>
#endif

// dumb win32 includes/defines cleanup
#undef GetCharWidth


enum Align { ALIGN_CENTER=0, ALIGN_LEFT };

enum Direction
{
	DIR_NONE		= -1,
	DIR_UP			= 0,
	DIR_DOWN		= 1,
	DIR_LEFT		= 2,
	DIR_RIGHT		= 3,
	DIR_UPLEFT		= 4,
	DIR_UPRIGHT		= 5,
	DIR_DOWNLEFT	= 6,
	DIR_DOWNRIGHT	= 7,
	DIR_MAX			= 8
};

#include "Event.h"

#include "Vector.h"


#define FOR_ALL(object, type, object_iterator)\
	{\
		for (type::iterator object_iterator = object.begin(); object_iterator != object.end(); ++object_iterator)\
		{\

#define END_FOR_ALL\
		}\
	}\


const float SQRT2		= 1.41421356;

const float PI			= 3.14159265;
const float PI_HALF		= 1.57079633;

#ifndef HUGE_VALF
	#define HUGE_VALF	((float)1e38)
#endif

struct IntPair
{
	IntPair(unsigned short int x, unsigned short int y) : x(x), y(y) {}
	unsigned short int x, y;
};

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

std::string numToZeroString(int num, int zeroes);
bool chance(int perc);
bool chancef(float p);
void initCharTranslationTables(const std::map<unsigned char, unsigned char>& tab);
void stringToUpper(std::string &s);
void stringToLower(std::string &s);
void stringToLowerUserData(std::string &s);
void glColor3_256(int r, int g, int b);
float sqr(float x);
bool exists(const std::string &f, bool makeFatal = false, bool skipVFS = false);
void errorLog(const std::string &s);
void debugLog(const std::string &s);
char *readFile(const std::string& path, unsigned long *size_ret = 0);
char *readCompressedFile(std::string path, unsigned long *size_ret = 0);
void forEachFile(std::string path, std::string type, void callback(const std::string &filename, intptr_t param), intptr_t param);
std::string stripEndlineForUnix(const std::string &in);
std::vector<std::string> getFileList(std::string path, std::string type, int param);
#ifdef HAVE_STRCASECMP
static inline int nocasecmp(const std::string &s1, const std::string &s2)
	{ return strcasecmp(s1.c_str(), s2.c_str()); }
static inline int nocasecmp(const std::string &s1, const char *s2)
	{ return strcasecmp(s1.c_str(), s2); }
static inline int nocasecmp(const char *s1, const std::string &s2)
	{ return strcasecmp(s1, s2.c_str()); }
static inline int nocasecmp(const char *s1, const char *s2)
	{ return strcasecmp(s1, s2); }
#else
int nocasecmp(const std::string &s1, const std::string &s2);
#endif
Vector getNearestPointOnLine(Vector start, Vector end, Vector point);
bool isTouchingLine(Vector lineStart, Vector lineEnd, Vector point, int radius=1, Vector* closest=0);
void sizePowerOf2Texture(int &v);
Vector getDirVector(Direction dir);
Direction getOppositeDir(Direction dir);
Direction getNextDirClockwise(Direction dir);
Vector colorRGB(int r, int g, int b);

#ifdef BBGE_BUILD_DIRECTX
	typedef unsigned int GLuint;
#endif
GLuint generateEmptyTexture(int res);

//void pForEachFile(std::string path, std::string type, void callback(const std::string &filename, int param), int param);

/*
void pfread(void *buffer, PHYSFS_uint32 size, PHYSFS_uint32 objs, PHYSFS_file *handle);
void pfseek(PHYSFS_file *handle,PHYSFS_uint64 byte,int origin);
void pfclose(PHYSFS_file *handle);


PHYSFS_file *openRead(const std::string &f);
std::string pLoadStream(const std::string &filename);
void pSaveStream(const std::string &filename, std::ostringstream &os);
*/

void drawCircle(float radius, int steps=1);
bool isVectorInRect(const Vector &vec, const Vector &coord1, const Vector &coord2);

std::string parseCommand(const std::string &line, const std::string &command);

void messageBox(const std::string &title, const std::string& msg);

void exit_error(const std::string &message);

unsigned hash(const std::string &string);

inline
float sqr(float x)
{
	return x*x;
}

int randAngle360();
Vector randVector(float magnitude);
std::string splitCamelCase(const std::string &input);
std::string removeSpaces(const std::string &input);
int randRange(int r1, int r2);

enum LerpType
{
	LERP_LINEAR			= 0,
	LERP_EASE			= 1,
	LERP_EASEIN			= 2,
	LERP_EASEOUT		= 3
};

#define DOUBLE_CLICK_DELAY	0.5f


float lerp(const float &v1, const float &v2, float dt, int lerpType);

//int packFile(const std::string &sourcef, const std::string &destf, int level);
//int unpackFile(const std::string &sourcef, const std::string &destf);

void openURL(const std::string &url);

std::string underscoresToSpaces(const std::string &str);
std::string spacesToUnderscores(const std::string &str);

void triggerBreakpoint();

bool createDir(const std::string& d);


#endif
