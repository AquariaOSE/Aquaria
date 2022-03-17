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
    #define WIN32_NOMINMAX
    #if defined(_MSC_VER) && _MSC_VER <= 1600
        #define strtof (float)strtod
        #define snprintf _snprintf
    #endif
#endif

#include "BBGECompileConfig.h"

#define compile_assert(pred) switch(0){case 0:case (pred):;}

#ifdef _MSC_VER
//#pragma warning(disable:4786)
//#pragma warning(disable:4005)
//#pragma warning(disable:4305)

//#pragma warning(disable:4018) // signed/unsigned mismatch
#pragma warning(disable:4244) // conversion from types with possible loss of data
#pragma warning(disable:4800) // forcing value to bool 'true' or 'false (performance warning)

//W4
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4505) // unreferenced local function has been removed
#pragma warning(disable:4702) // unreachable code
#pragma warning(disable:4127) // conditional expression is constant
//#pragma warning(disable:4706) // assignment within conditional expression

//#pragma warning(disable:4389) // signed/unsigned mismatch

//#pragma warning(disable:4189) // UqqqqSEFUL: local variable is initialized but not referenced
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "math.h"

#include "Vector.h"
#include "OSFunctions.h"

// --- Defined in RenderBase.cpp -- Declared here to avoid pulling in gl.h via RenderBase.h --
void drawCircle(float radius, int stepSize);
unsigned generateEmptyTexture(int res);
void sizePowerOf2Texture(int &v);
// ----------------------

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

const float PI			= 3.14159265f;
const float PI_HALF		= 1.57079633f;

#ifndef HUGE_VALF
	#define HUGE_VALF	((float)1e38)
#endif

typedef int CharTranslationTable[256]; // -1 entries are skipped


#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

std::string numToZeroString(int num, size_t zeroes);
bool chance(int perc);
void initCharTranslationTables(const CharTranslationTable *ptab);
void stringToUpper(std::string &s);
void stringToLower(std::string &s);
void stringToLowerUserData(std::string &s);
float sqr(float x);
bool exists(const std::string &f, bool makeFatal = false, bool skipVFS = false);
void errorLog(const std::string &s);
void debugLog(const std::string &s);
char *readFile(const std::string& path, unsigned long *size_ret = 0);

char *readCompressedFile(std::string path, unsigned long *size_ret = 0);
std::string stripEndlineForUnix(const std::string &in);
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
bool isTouchingLine(Vector lineStart, Vector lineEnd, Vector point, int radius=1, Vector* closest=0);


void drawCircle(float radius, int steps=1);


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



void openURL(const std::string &url);

std::string underscoresToSpaces(const std::string &str);
std::string spacesToUnderscores(const std::string &str);

void triggerBreakpoint();

bool createDir(const std::string& d);

#ifdef BBGE_BUILD_VFS
namespace ttvfs { class Root; }
extern ttvfs::Root vfs; // in Base.cpp
#endif

#endif
