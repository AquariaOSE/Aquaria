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
#include "RenderBase.h"

//WARNING: FrameBuffer objects have to have reloadDevice/unloadDevice called manually!


struct FBOStack
{
	const FrameBuffer *fbo;
	unsigned page;
};

static FBOStack s_fbostack[4]; // first entry is always unused
static size_t s_stackpos = 0;
static unsigned s_lastFBO = 0;


static bool isOnTop(const FrameBuffer *fbo)
{
	return s_fbostack[s_stackpos].fbo == fbo;
}

static bool isInStack(const FrameBuffer *fbo, unsigned page)
{
	for(size_t i = 1; i <= s_stackpos; ++i) // first entry is always NULL
		if(s_fbostack[i].fbo == fbo && s_fbostack[i].page == page)
			return true;
	return false;
}


FrameBuffer::FrameBuffer()
{
	inited = false;
	texw = 0;
	texh = 0;
	viewportW = 0;
	viewportH = 0;
	_curpage = 0;
	_w = _h = 0;
	_numpages = 0;
	_numfbos = 0;
	for(size_t i = 0; i < Countof(_texs); ++i)
	{
		_fbos[i] = 0;
		_texs[i] = 0;
	}
}

FrameBuffer::~FrameBuffer()
{
	unloadDevice();
}

float FrameBuffer::getWidthP() const
{
	return (float)core->getWindowWidth()/(float)texw;
}

float FrameBuffer::getHeightP() const
{
	return (float)core->getWindowHeight()/(float)texh;
}

bool FrameBuffer::getCurrentPage() const
{
	assert(_curpage);
	return _curpage - 1;
}

bool FrameBuffer::init(int width, int height, unsigned pages)
{
	assert(pages && pages < Countof(_texs));
	_w = width;
	_h = height;
	_numpages = pages;

	if (width == -1)
		width = core->width;

	if (height == -1)
		height = core->height;

	viewportW = width;
	viewportH = height;

	sizePowerOf2Texture(width);
	sizePowerOf2Texture(height);

	if (width == 0 || height == 0)
		return false;

	texw = width;
	texh = height;

	std::ostringstream os;
	os << "Loading EXT_framebuffer_object (" << texw << ", " << texh << ")";
	debugLog(os.str());

	if( !glIsFramebufferEXT || !glBindFramebufferEXT || !glDeleteFramebuffersEXT ||
		!glGenFramebuffersEXT || !glCheckFramebufferStatusEXT || !glFramebufferTexture2DEXT)
	{
		debugLog("One or more EXT_framebuffer_object functions were not found");
		return false;
	}

	unloadDevice();

	// If glDrawBuffersARB() is present, we can attach multiple textures per FBO
	// and switch between them as render targets. More efficient than switching FBOs.
	_numfbos = glDrawBuffersARB ? 1 : pages;

	glGenFramebuffersEXT(_numfbos, &_fbos[0]);
	for(unsigned i = 0; i < _numfbos; ++i)
		if(!_fbos[i])
			return false;

	if(_numfbos == 1)
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _fbos[0] );

	glGenTextures( pages, &_texs[0] );

	for(unsigned i = 0; i < pages; ++i)
	{
		unsigned attach = GL_COLOR_ATTACHMENT0_EXT;
		if(_numfbos > 1)
			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _fbos[i] );
		else
			attach += i;

		glBindTexture(GL_TEXTURE_2D, _texs[i]);
		// GL_LINEAR
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA,
					width, height,
					0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attach, GL_TEXTURE_2D, _texs[i], 0);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	for(unsigned i = 0; i < _numfbos; ++i)
	{
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, _fbos[i] );
		GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

		switch( status )
		{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;

		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			debugLog("GL_FRAMEBUFFER_UNSUPPORTED_EXT!");
		default:
			unloadDevice();
			return false;
		}
	}

	debugLog("Done");
	inited = true;
	return true;
}

void FrameBuffer::unloadDevice()
{
	debugLog("frameBuffer::unloadDevice");
	inited = false;

	if (glDeleteFramebuffersEXT == NULL)
	{
		s_lastFBO = 0;
		debugLog("Already shut down the GL, don't delete framebuffers");
		return;
	}

	for(size_t i = 0; i < Countof(_texs); ++i)
		if (_fbos[i])
		{
			if(s_lastFBO == _fbos[i])
				s_lastFBO = 0;
			debugLog("frameBuffer handle present, deleting");
			glDeleteFramebuffersEXT(1, &_fbos[i]);
			_fbos[i] = 0;
		}

	for(size_t i = 0; i < Countof(_texs); ++i)
		if (_texs[i])
		{
			debugLog("delete framebuffer texture");
			glDeleteTextures(1, &_texs[i]);
			_texs[i] = 0;
		}


	debugLog("done");
}

void FrameBuffer::reloadDevice()
{
	if(!_numpages)
		return;
	s_lastFBO = 0;
	debugLog("frameBuffer::reloadDevice");
	init(_w, _h, _numpages);
}

void FrameBuffer::_bind(unsigned page) const
{
	assert(page < _numpages);
	_curpage = page + 1;
	unsigned fbo = _numfbos == 1 ? _fbos[0] : _fbos[page];
	assert(fbo);
	if(fbo != s_lastFBO)
	{
		s_lastFBO = fbo;
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	}

	if(glDrawBuffersARB)
	{
		GLenum buf = GL_COLOR_ATTACHMENT0_EXT + page;
		glDrawBuffersARB(1, &buf);
	}

	glViewport(0,0,viewportW,viewportH);
}

void FrameBuffer::pushCapture(unsigned page) const
{
	assert(inited);

	_bind(page);

	size_t idx = ++s_stackpos;
	assert(idx < Countof(s_fbostack));
	s_fbostack[idx].fbo = this;
	s_fbostack[idx].page = page;
}

unsigned FrameBuffer::popCapture() const
{
	assert(inited && s_stackpos && isOnTop(this));
	const unsigned page = s_fbostack[s_stackpos].page;

	FBOStack prev = s_fbostack[--s_stackpos];
	if(prev.fbo)
		prev.fbo->_bind(prev.page);
	else
	{
		s_lastFBO = 0;
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		if(glDrawBuffersARB)
			glDrawBuffer(GL_BACK);
		glViewport(0, 0, core->width, core->height);
	}
	return page;
}

void FrameBuffer::replaceCapture(unsigned page) const
{
	assert(inited && s_stackpos);

	_bind(page);
	s_fbostack[s_stackpos].fbo = this;
	s_fbostack[s_stackpos].page = page;
}

unsigned FrameBuffer::getTextureID(unsigned page) const
{
	assert(inited && page < _numpages);
	return _texs[page];
}

void FrameBuffer::bindTexture(unsigned page) const
{
	// Technically it's enough that this texture isn't the one currently rendered to,
	// but because we don't know when the topmost FBO is popped (and subsequently
	// writing to the texture we're now about to read from) let's be extra safe
	// and generalize this to: Textures part of the FBO stack are forbidden to bind.
	assert(!isInStack(this, page));
	unsigned tex = getTextureID(page);
	glBindTexture(GL_TEXTURE_2D, tex);
}
