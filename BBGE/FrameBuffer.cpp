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
#include "FrameBuffer.h"
#include "Core.h"

//WARNING: FrameBuffer objects have to have reloadDevice/unloadDevice called manually!

#ifdef BBGE_BUILD_FRAMEBUFFER
	PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT = NULL;
	PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = NULL;
	PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = NULL;
	PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = NULL;
	PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = NULL;
	PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT = NULL;
	PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT = NULL;
	PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = NULL;
	PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = NULL;
	PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = NULL;
	PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
	PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT = NULL;
	PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = NULL;
	PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT = NULL;
	PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = NULL;
	PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT = NULL;
	PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = NULL;
#endif

FrameBuffer::FrameBuffer()
{
	inited = false;
	enabled = false;
	w = 0;
	h = 0;
	g_frameBuffer = 0;
	g_depthRenderBuffer = 0;
	g_dynamicTextureID = 0;
	_w = _h = 0;
}

FrameBuffer::~FrameBuffer()
{
	unloadDevice();
}

float FrameBuffer::getWidthP()
{
	float px=0;
	int sw=core->getWindowWidth();
	px = (float)sw/(float)w;
	return px;
}

float FrameBuffer::getHeightP()
{
	float py=0;
	int sh=core->getWindowHeight();
	py = (float)sh/(float)h;
	return py;
}

bool FrameBuffer::init(int width, int height, bool fitToScreen, GLint filter)
{
// !!! FIXME: check for common GMA GL_RENDERER strings on Linux, too.
#ifdef BBGE_BUILD_MACOSX
	std::ostringstream oss;
	oss << "Vendor: [" << glGetString(GL_VENDOR) << "] Renderer: [" << glGetString(GL_RENDERER) << "]";
	debugLog(oss.str());
	
	std::string renderer = (const char*)glGetString(GL_RENDERER);
	if (renderer.find("Intel GMA 950") != std::string::npos)
	{
		debugLog("Video Card is Intel GMA 950. Disabling FrameBuffer Effects.");
		return false;
	}

#endif
	
#ifdef BBGE_BUILD_FRAMEBUFFER

	_w = width;
	_h = height;

	if (width == -1)
		width = core->width;

	if (height == -1)
		height = core->height;

	if (fitToScreen)
	{
		sizePowerOf2Texture(width);
		sizePowerOf2Texture(height);
	}

	_fitToScreen = fitToScreen;
	if (width == 0 || height == 0)
		return false;
	/*
	if (width > height)
		height = width;
	*/

	w=width;
	h=height;

	char *ext = (char*)glGetString( GL_EXTENSIONS );

	std::ostringstream os;
	os << "Loading EXT_framebuffer_object (" << w << ", " << h << ")";
	debugLog(os.str());

	if( strstr( ext, "EXT_framebuffer_object" ) == NULL )
	{
		debugLog("EXT_framebuffer_object extension was not found");
		return false;
	}
	else
	{
		if (!glIsRenderbufferEXT)
		{
			glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glIsRenderbufferEXT");
			glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindRenderbufferEXT");
			glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");
			glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
			glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
			glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)SDL_GL_GetProcAddress("glGetRenderbufferParameterivEXT");
			glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glIsFramebufferEXT");
			glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindFramebufferEXT");
			glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
			glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenFramebuffersEXT");
			glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
			glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture1DEXT");
			glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
			glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture3DEXT");
			glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glFramebufferRenderbufferEXT");
			glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)SDL_GL_GetProcAddress("glGetFramebufferAttachmentParameterivEXT");
			glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)SDL_GL_GetProcAddress("glGenerateMipmapEXT");
		}

		if( !glIsRenderbufferEXT || !glBindRenderbufferEXT || !glDeleteRenderbuffersEXT ||
			!glGenRenderbuffersEXT || !glRenderbufferStorageEXT || !glGetRenderbufferParameterivEXT ||
			!glIsFramebufferEXT || !glBindFramebufferEXT || !glDeleteFramebuffersEXT ||
			!glGenFramebuffersEXT || !glCheckFramebufferStatusEXT || !glFramebufferTexture1DEXT ||
			!glFramebufferTexture2DEXT || !glFramebufferTexture3DEXT || !glFramebufferRenderbufferEXT||
			!glGetFramebufferAttachmentParameterivEXT || !glGenerateMipmapEXT )
		{
			debugLog("One or more EXT_framebuffer_object functions were not found");
			return false;
		}

		//
		// Create a frame-buffer object and a render-buffer object...
		//

		glGenFramebuffersEXT( 1, &g_frameBuffer );
		glGenRenderbuffersEXT( 1, &g_depthRenderBuffer );

		// Initialize the render-buffer for usage as a depth buffer.
		// We don't really need this to render things into the frame-buffer object,
		// but without it the geometry will not be sorted properly.
		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, g_depthRenderBuffer );
		glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height );

		//
		// Check for errors...
		//

		GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );

		switch( status )
		{
			case GL_FRAMEBUFFER_COMPLETE_EXT:
				break;

			case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
				debugLog("GL_FRAMEBUFFER_UNSUPPORTED_EXT!");
				return false;
				break;

			default:
				return false;
		}


		glGenTextures( 1, &g_dynamicTextureID );

		glBindTexture( GL_TEXTURE_2D, g_dynamicTextureID );
		// GL_LINEAR
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB,
					width, height,
					0, GL_BGR, GL_UNSIGNED_BYTE, 0 );
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	
	debugLog("Done");
	inited = true;
	enabled = true;
	return true;
#endif

	return false;
}

void FrameBuffer::unloadDevice()
{
	debugLog("frameBuffer::unloadDevice");


#ifdef BBGE_BUILD_FRAMEBUFFER



	if (glDeleteFramebuffersEXT == NULL)
	{
		debugLog("Already shut down the GL, don't delete framebuffers");
		return;
	}

	if (g_frameBuffer)
	{
		debugLog("bind 0");
		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, 0 );

		debugLog("frameBuffer handle present, deleting");
		glDeleteFramebuffersEXT(1, &g_frameBuffer);
		g_frameBuffer = 0;
	}

	if (g_dynamicTextureID)
	{
		debugLog("bind 0");
		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, 0 );

		debugLog("dynamic texture ID handle present, deleting");
		glDeleteTextures(1, &g_dynamicTextureID);
		g_dynamicTextureID = 0;
	}

	if (g_depthRenderBuffer)
	{
		debugLog("bind 0");
		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, 0 );

		debugLog("depth render buffer handle present, deleting");
		glDeleteRenderbuffersEXT(1, &g_depthRenderBuffer);
		g_depthRenderBuffer = 0;
	}

#endif
	debugLog("done");
}

void FrameBuffer::resetOpenGL()
{
#if defined(BBGE_BUILD_FRAMEBUFFER)
	// set these back to NULL and reload them upon reinit, otherwise they
	//  might point to a bogus address when the shared library is reloaded.
	glIsRenderbufferEXT = NULL;
	glBindRenderbufferEXT = NULL;
	glDeleteRenderbuffersEXT = NULL;
	glGenRenderbuffersEXT = NULL;
	glRenderbufferStorageEXT = NULL;
	glGetRenderbufferParameterivEXT = NULL;
	glIsFramebufferEXT = NULL;
	glBindFramebufferEXT = NULL;
	glDeleteFramebuffersEXT = NULL;
	glGenFramebuffersEXT = NULL;
	glCheckFramebufferStatusEXT = NULL;
	glFramebufferTexture1DEXT = NULL;
	glFramebufferTexture2DEXT = NULL;
	glFramebufferTexture3DEXT = NULL;
	glFramebufferRenderbufferEXT = NULL;
	glGetFramebufferAttachmentParameterivEXT = NULL;
	glGenerateMipmapEXT = NULL;
#endif
}

void FrameBuffer::reloadDevice()
{
	debugLog("frameBuffer::reloadDevice");
	init(_w, _h, _fitToScreen);
}

void FrameBuffer::startCapture()
{
#ifdef BBGE_BUILD_FRAMEBUFFER

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, g_frameBuffer );
	//glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, g_depthRenderBuffer );
	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, g_dynamicTextureID, 0 );
	glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, g_depthRenderBuffer );

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


#endif
}

void FrameBuffer::endCapture()
{
#ifdef BBGE_BUILD_FRAMEBUFFER

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

#endif
}

void FrameBuffer::bindTexture()
{
#ifdef BBGE_BUILD_FRAMEBUFFER

	glBindTexture( GL_TEXTURE_2D, g_dynamicTextureID );

#endif
}

void FrameBuffer::setEnabled(bool e)
{
	enabled = e;
}

