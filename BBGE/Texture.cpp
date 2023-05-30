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

#include <sstream>
#include "Base.h"
#include "Texture.h"
#include "Image.h"
#include "ByteBuffer.h"
#include "RenderBase.h"
#include "bithacks.h"
#include <assert.h>
#include "GLLoad.h"
#include "stb_image_resize.h"


Texture::Texture()
{
	gltexid = 0;
	width = height = 0;

	_repeating = false;
	ow = oh = -1;
	_mipmap = false;
	success = false;
}

Texture::~Texture()
{
	unload();
}

void Texture::readRGBA(unsigned char *pixels)
{
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, gltexid);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::writeRGBA(int tx, int ty, int w, int h, const unsigned char *pixels)
{
	glBindTexture(GL_TEXTURE_2D, gltexid);

	glTexSubImage2D(GL_TEXTURE_2D, 0,
					tx,
					ty,
					w,
					h,
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					pixels
					);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::unload()
{
	if (gltexid)
	{
		ow = width;
		oh = height;

		glDeleteTextures(1, &gltexid);
		gltexid = 0;
	}
}

static const GLenum repeatLUT[] = { GL_CLAMP_TO_EDGE, GL_REPEAT };
void Texture::apply(bool repeat) const
{
	glBindTexture(GL_TEXTURE_2D, gltexid);
	if(repeat != _repeating)
	{
		_repeating = repeat;
		GLenum rep = repeatLUT[repeat];
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, rep);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, rep);
	}
}

struct GlTexFormat
{
	int internalformat, format, type;
	int alphachan; // for stb_image_resize; index of alpha channel
};
static const GlTexFormat formatLUT[] =
{
	{ GL_LUMINANCE,       GL_R,               GL_UNSIGNED_BYTE, STBIR_ALPHA_CHANNEL_NONE },
	{ GL_LUMINANCE_ALPHA, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 1 },
	{ GL_RGB,             GL_RGB,             GL_UNSIGNED_BYTE, STBIR_ALPHA_CHANNEL_NONE },
	{ GL_RGBA,            GL_RGBA,            GL_UNSIGNED_BYTE, 3 }
};

bool Texture::upload(const ImageData& img, bool mipmap)
{
	if(!img.pixels || !img.channels || img.channels > 4 || !img.w || !img.h)
		return false;

	//if(!bithacks::isPowerOf2(img.w))
	//	__debugbreak();

	// work around bug in older ATI drivers that would cause glGenerateMipmapEXT() to fail otherwise
	// via https://www.khronos.org/opengl/wiki/Common_Mistakes#Automatic_mipmap_generation
	glEnable(GL_TEXTURE_2D);
	// no padding
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if(!gltexid)
		glGenTextures(1, &gltexid);
	glBindTexture(GL_TEXTURE_2D, gltexid);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	_repeating = false;

	const GlTexFormat& f = formatLUT[img.channels - 1];

	int minfilter = GL_LINEAR;
	int ismip = 0;

	// if our super old OpenGL supports it, request automatic mipmap generation
	// but not if glGenerateMipmapEXT is present, as it's the much better choice
	if(mipmap && !glGenerateMipmapEXT && g_has_GL_GENERATE_MIPMAP)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glGetTexParameteriv(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, &ismip);
	}

	// attach base level first
	glTexImage2D(GL_TEXTURE_2D, 0, f.internalformat, img.w, img.h, 0, f.format, f.type, img.pixels);

	if(mipmap && !ismip)
	{
		// now that the base is attached, generate mipmaps
		if(glGenerateMipmapEXT)
		{
			glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
			glGenerateMipmapEXT(GL_TEXTURE_2D);
			ismip = 1;
		}

		if(!ismip)
		{
			debugLog("Failed to mipmap in hardware, using software fallback");
			ismip = 1;
			unsigned mw = img.w;
			unsigned mh = img.h;
			unsigned char *pmip = img.pixels;
			unsigned level = 0;
			while(mw > 1 || mh > 1)
			{
				const unsigned oldw = mw, oldh = mh;
				mw = bithacks::prevPowerOf2(mw);
				mh = bithacks::prevPowerOf2(mh);
				assert(mw && mh);
				++level;
				unsigned char *out = (unsigned char*)malloc(mw * mh * img.channels);
				// when we're on hardware old enough not to have glGenerateMipmapEXT we'll
				// likely not want to spend too much time generating mipmaps,
				// so something fast & cheap like a box filter is enough
				int res = stbir_resize_uint8_generic(pmip, oldw, oldh, 0, out, mw, mh, 0, img.channels,
					f.alphachan, 0, STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR, NULL);
				if(!res)
				{
					debugLog("Failed to calculate software mipmap");
					free(out);
					ismip = 0;
					break;
				}
				glTexImage2D(GL_TEXTURE_2D, level, f.internalformat, mw, mh, 0, f.format, f.type, out);

				if(pmip != img.pixels)
					free(pmip);
				pmip = out;
			}
			if(pmip != img.pixels)
				free(pmip);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL, level);
		}
	}
	if(ismip)
		minfilter = GL_LINEAR_MIPMAP_LINEAR;

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, minfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	width = img.w;
	height = img.h;
	return true;
}

unsigned char * Texture::getBufferAndSize(int *wparam, int *hparam, size_t *sizeparam)
{
	const size_t bytes = size_t(width) * size_t(height) * 4;
	unsigned char *data = (unsigned char*)malloc(bytes);
	if (!data)
	{
		std::ostringstream os;
		os << "Game::getBufferAndSize allocation failure, bytes = " << bytes;
		errorLog(os.str());
		return NULL;
	}
	this->readRGBA(data);

	*wparam = width;
	*hparam = height;
	*sizeparam = bytes;
	return data;
}
