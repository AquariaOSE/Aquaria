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
#ifndef BBGE_TEXTURE_H
#define BBGE_TEXTURE_H

#include <string>
#include "Refcounted.h"


enum TextureLoadResult
{
	TEX_FAILED,
	TEX_SUCCESS
};

struct ImageData;

class Texture : public Refcounted
{
public:
	Texture();
	~Texture();

	bool load(std::string file, bool mipmap);
	void apply(bool repeatOverride=false);
	void unbind();
	void unload();

	int getPixelWidth();
	int getPixelHeight();

	void destroy();

	int width, height;

	bool repeat, repeating;

	unsigned textures[1];

	void reload();

	void write(int tx, int ty, int w, int h, const unsigned char *pixels);
	void read(int tx, int ty, int w, int h, unsigned char *pixels);

	unsigned char *getBufferAndSize(int *w, int *h, unsigned int *size); // returned memory must be free()'d

	std::string name;

	TextureLoadResult getLoadResult() const { return loadResult; }

protected:
	std::string loadName;

	bool loadInternal(const ImageData& img, bool mipmap);

	int ow, oh;
	TextureLoadResult loadResult;
	bool _mipmap;
};

#define UNREFTEX(x) if (x) {x = NULL;}

#endif
