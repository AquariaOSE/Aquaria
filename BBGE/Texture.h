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

#include "Resource.h"

struct ImageTGA
{
	int channels;			// The channels in the image (3 = RGB : 4 = RGBA)
	int sizeX;				// The width of the image in pixels
	int sizeY;				// The height of the image in pixels
	unsigned char *data;	// The image pixel data
};

enum TexErr
{
	TEXERR_OK				= 0,
	TEXERR_FILENOTFOUND		= 1,
	TEXERR_MAX
};

class Texture : public Resource
{
public:
	Texture();
	~Texture();

	void load(std::string file);
	void apply(bool repeatOverride=false);
	void unbind();
	void unload();
	void setLayer(int layer);

	int getPixelWidth();
	int getPixelHeight();
	
	void destroy();
	

	int width, height;

	static ImageTGA *TGAload(const char* filename);
	static ImageTGA *TGAloadMem(void *mem, int size);
	
	static bool useMipMaps;
	bool repeat;

	int components;
#ifdef BBGE_BUILD_OPENGL
	static GLint filter;
	static GLint format;
	void setID (int id);
	GLuint textures[1];
#endif
#ifdef BBGE_BUILD_DIRECTX
	LPDIRECT3DTEXTURE9 d3dTexture;
#endif
	//void setImageData(imageData);
	// HACK:
	unsigned char *imageData;
	void reload();

	static TexErr textureError;

	void write(int tx, int ty, int w, int h, const unsigned char *pixels);
	void read(int tx, int ty, int w, int h, unsigned char *pixels);
protected:
	std::string loadName;

	// internal load functions
	void loadPNG(const std::string &file);
	void loadTGA(const std::string &file);
	void loadZGA(const std::string &file);
	void loadTGA(ImageTGA *tga);

	int ow, oh;
	
};

#define UNREFTEX(x) if (x) {x->removeRef(); x=0;}

#endif
