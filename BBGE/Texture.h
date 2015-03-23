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
#ifndef __texture__
#define __texture__

#include "Base.h"

enum TextureLoadResult
{
	TEX_FAILED  = 0x00,
	TEX_SUCCESS = 0x01,
	TEX_LOADED  = 0x02,
};

struct ImageTGA
{
	int channels;			// The channels in the image (3 = RGB : 4 = RGBA)
	int sizeX;				// The width of the image in pixels
	int sizeY;				// The height of the image in pixels
	unsigned char *data;	// The image pixel data
};

class Texture : public Refcounted
{
public:
	Texture();
	~Texture();

	bool load(std::string file);
	void apply(bool repeatOverride=false);
	void unbind();
	void unload();

	int getPixelWidth();
	int getPixelHeight();
	
	void destroy();
	

	int width, height;

	static ImageTGA *TGAload(const char* filename);
	static ImageTGA *TGAloadMem(void *mem, int size);
	
	static bool useMipMaps;
	bool repeat, repeating;

#ifdef BBGE_BUILD_OPENGL
	static GLint filter;
	static GLint format;
	GLuint textures[1];
#endif
#ifdef BBGE_BUILD_DIRECTX
	LPDIRECT3DTEXTURE9 d3dTexture;
#endif

	void reload();

	void write(int tx, int ty, int w, int h, const unsigned char *pixels);
	void read(int tx, int ty, int w, int h, unsigned char *pixels);

	unsigned char *getBufferAndSize(int *w, int *h, unsigned int *size); // returned memory must be free()'d

	std::string name;

protected:
	std::string loadName;

	// internal load functions
	bool loadPNG(const std::string &file);
	bool loadTGA(const std::string &file);
	bool loadZGA(const std::string &file);
	bool loadTGA(ImageTGA *tga);

	int ow, oh;
	
};

#define UNREFTEX(x) if (x) {x = NULL;}

#endif
