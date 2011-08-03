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
#include "PointSprites.h"

#ifdef BBGE_BUILD_WINDOWS

PointSprites::PointSprites()
{
	glPointParameterfARB		= NULL;
	glPointParameterfvARB		= NULL;
}

void PointSprites::init()
{
	char *ext = (char*)glGetString( GL_EXTENSIONS );

    if( strstr( ext, "GL_ARB_point_parameters" ) == NULL )
    {
        debugLog("PointSprites: GL_ARB_point_parameters extension was not found");
        return;
    }
    else
    {
#ifdef BBGE_BUILD_GLFW
        glPointParameterfARB  = (PFNGLPOINTPARAMETERFARBPROC)glfwGetProcAddress("glPointParameterfARB");
        glPointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)glfwGetProcAddress("glPointParameterfvARB");
#endif

#ifdef BBGE_BUILD_SDL
        glPointParameterfARB  = (PFNGLPOINTPARAMETERFARBPROC)SDL_GL_GetProcAddress("glPointParameterfARB");
        glPointParameterfvARB = (PFNGLPOINTPARAMETERFVARBPROC)SDL_GL_GetProcAddress("glPointParameterfvARB");
#endif

        if( !glPointParameterfARB || !glPointParameterfvARB )
        {
			debugLog("PointSprites: One or more GL_ARB_point_parameters functions were not found");
            return;
        }
    }
}

#endif
