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
#include "Texture.h"
#include "Core.h"
#include "Image.h"
#include "ByteBuffer.h"
#include "RenderBase.h"
#include "bithacks.h"
#include <assert.h>
#include "GLLoad.h"
#include "stb_image_resize.h"


Texture::Texture()
{
	textures[0] = 0;
	width = height = 0;

	repeat = false;
	repeating = false;
	ow = oh = -1;
	loadResult = TEX_FAILED;
	_mipmap = false;
}

Texture::~Texture()
{
	destroy();
}

void Texture::read(int tx, int ty, int w, int h, unsigned char *pixels)
{
	if (tx == 0 && ty == 0 && w == this->width && h == this->height)
	{
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::ostringstream os;
		os << "Unable to read a texture subimage (size = "
		   << this->width << "x" << this->height << ", requested = "
		   << tx << "," << ty << "+" << w << "x" << h << ")";
		debugLog(os.str());
	}
}

void Texture::write(int tx, int ty, int w, int h, const unsigned char *pixels)
{
	glBindTexture(GL_TEXTURE_2D, textures[0]);

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
	/*
	  target   Specifies the target	texture.  Must be
		   GL_TEXTURE_2D.

	  level	   Specifies the level-of-detail number.  Level	0 is
		   the base image level.  Level	n is the nth mipmap
		   reduction image.

	  xoffset  Specifies a texel offset in the x direction within
		   the texture array.

	  yoffset  Specifies a texel offset in the y direction within
		   the texture array.

	  width	   Specifies the width of the texture subimage.

	  height   Specifies the height	of the texture subimage.

	  format   Specifies the format	of the pixel data.  The
		   following symbolic values are accepted:
		   GL_COLOR_INDEX, GL_RED, GL_GREEN, GL_BLUE,
		   GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and
		   GL_LUMINANCE_ALPHA.

	  type	   Specifies the data type of the pixel	data.  The
		   following symbolic values are accepted:
		   GL_UNSIGNED_BYTE, GL_BYTE, GL_BITMAP,
		   GL_UNSIGNED_SHORT, GL_SHORT,	GL_UNSIGNED_INT,
		   GL_INT, and GL_FLOAT.

	  pixels   Specifies a pointer to the image data in memory.
	  */
}

void Texture::unload()
{
	if (textures[0])
	{
		ow = width;
		oh = height;

		if (core->debugLogTextures)
		{
			debugLog("UNLOADING TEXTURE: " + name);
		}


		glDeleteTextures(1, &textures[0]);
		textures[0] = 0;
	}
}

void Texture::destroy()
{
	unload();

	core->removeTexture(this);
}

// FIXME: this should be recorded at load-time -- fg
int Texture::getPixelWidth()
{
	int w = 0, h = 0;
	unsigned int size = 0;
	unsigned char *data = getBufferAndSize(&w, &h, &size);
	if (!data)
		return 0;

	size_t smallestx = -1, largestx = 0;
	for (unsigned int x = 0; x < unsigned(w); x++)
	{
		for (unsigned int y = 0; y < unsigned(h); y++)
		{
			unsigned int p = (y*unsigned(w)*4) + (x*4) + 3;
			if (p < size && data[p] >= 254)
			{
				if (x < smallestx)
					smallestx = x;
				if (x > largestx)
					largestx = x;
			}
		}
	}
	free(data);
	return largestx - smallestx;
}

// FIXME: same as above
int Texture::getPixelHeight()
{
	int w = 0, h = 0;
	unsigned int size = 0;
	unsigned char *data = getBufferAndSize(&w, &h, &size);
	if (!data)
		return 0;

	size_t smallesty = -1, largesty = 0;
	for (unsigned int x = 0; x < unsigned(w); x++)
	{
		for (unsigned int y = 0; y < unsigned(h); y++)
		{
			size_t p = (y*unsigned(w)*4) + (x*4) + 3;
			if (p < size && data[p] >= 254)
			{
				if (y < smallesty)
					smallesty = y;
				if (y > largesty)
					largesty = y;
			}
		}
	}
	free(data);
	return largesty - smallesty;
}

void Texture::reload()
{
	debugLog("RELOADING TEXTURE: " + name + " with loadName " + loadName + "...");

	unload();
	load(loadName, _mipmap);


	debugLog("DONE");
}

bool Texture::load(std::string file, bool mipmap)
{
	loadResult = TEX_FAILED;
	if (file.size()<4)
	{
		errorLog("Texture Name is Empty or Too Short");
		return false;
	}

	stringToLowerUserData(file);
	file = adjustFilenameCase(file);

	loadName = file;
	repeating = false;
	_mipmap = mipmap;

	size_t pos = file.find_last_of('.');

	if (pos != std::string::npos)
	{
		// make sure this didn't catch the '.' in /home/username/.Aquaria/*  --ryan.
		const std::string userdata = core->getUserDataFolder();
		const size_t len = userdata.length();
		if (pos < len)
			pos = std::string::npos;
	}

	bool found = exists(file);

	if(!found && exists(file + ".png"))
	{
		found = true;
		file += ".png";
	}

	// .tga/.zga are never used as game graphics anywhere except save slot thumbnails.
	// if so, their file names are passed exact, not with a missing extension

	bool ok = false;
	if (found)
	{
		file = localisePathInternalModpath(file);
		file = adjustFilenameCase(file);


		std::string post = file.substr(file.size()-3, 3);
		stringToLower(post);

		ImageData img = {};
		if (post == "zga")
		{
			img = imageLoadZGA(file.c_str());
			if(img.pixels)
				mipmap = false;
			else
				debugLog("Can't load ZGA File: " + file);
		}
		else
		{
			img = imageLoadGeneric(file.c_str(), false);
			if(!img.pixels)
				debugLog("unknown image file type: " + file);
		}

		if(img.pixels)
		{
			ok = loadInternal(img, mipmap);
			free(img.pixels);
		}
	}
	else
	{
		// load default image / leave white
		if (core->debugLogTextures)
			debugLog("***Could not find texture: " + file);
	}
	return ok;
}

void Texture::apply(bool repeatOverride) const
{
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (repeat || repeatOverride)
	{
		if (!repeating)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			repeating = true;
		}
	}
	else
	{
		if (repeating)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			repeating = false;
		}
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

bool Texture::loadInternal(const ImageData& img, bool mipmap)
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

	glGenTextures(1, &textures[0]);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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
	loadResult = TEX_SUCCESS;
	return true;
}

unsigned char * Texture::getBufferAndSize(int *wparam, int *hparam, unsigned int *sizeparam)
{
	unsigned char *data = NULL;
	unsigned int size = 0;
	int tw = 0, th = 0;
	int w = 0, h = 0;

	// This can't happen. If it does we're doomed.
	if(width <= 0 || height <= 0)
		goto fail;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	// As returned by graphics driver

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

	// As we know it - but round to nearest power of 2 - OpenGL does this internally anyways.
	tw = bithacks::clp2(width); // known to be > 0.
	th = bithacks::clp2(height);

	if (w != tw || h != th)
	{
		std::ostringstream os;
		os << "Texture::getBufferAndSize() WARNING: width/height disagree: ";
		os << "Driver says (" << w << ", " << h << "); ";
		os << "Texture says (" << width << ", " << height << "); ";
		os << "Rounded to (" << tw << ", " << th << ")";
		debugLog(os.str());
		// choose max. for size calculation
		w = w > tw ? w : tw;
		h = h > th ? h : th;
	}

	size = w * h * 4;
	if (!size)
		goto fail;

	data = (unsigned char*)malloc(size + 32);
	if (!data)
	{
		std::ostringstream os;
		os << "Game::fillGridFromQuad allocation failure, size = " << size;
		errorLog(os.str());
		goto fail;
	}
	memcpy(data + size, "SAFE", 5);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Not sure but this might be the case with nouveau drivers on linux... still investigating. -- fg
	if(memcmp(data + size, "SAFE", 5))
	{
		errorLog("Texture::getBufferAndSize(): Broken graphics driver! Wrote past end of buffer!");
		free(data); // in case we are here, this will most likely cause a crash.
		goto fail;
	}

	*wparam = w;
	*hparam = h;
	*sizeparam = size;
	return data;


fail:
	*wparam = 0;
	*hparam = 0;
	*sizeparam = 0;
	return NULL;
}
