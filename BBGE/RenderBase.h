#ifndef BBGE_RENDERBASE_H
#define BBGE_RENDERBASE_H

#include "SDL.h"

#define GL_GLEXT_LEGACY 1
#include "gl.h"

#ifdef _WINDOWS_
#error windows.h was included! euuugh!
#endif

#ifdef APIENTRY
#undef APIENTRY
#endif

#ifdef WINGDIAPI
#undef WINGDIAPI
#endif


#endif
