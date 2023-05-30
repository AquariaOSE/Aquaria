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

	void apply(bool repeat = false) const;
	void unload();

	unsigned gltexid;
	int width, height;

	void writeRGBA(int tx, int ty, int w, int h, const unsigned char *pixels);
	void readRGBA(unsigned char *pixels);

	unsigned char *getBufferAndSize(int *w, int *h, size_t *size); // returned memory must be free()'d

	std::string name, filename;
	bool upload(const ImageData& img, bool mipmap);

	bool success;

protected:

	int ow, oh;
	bool _mipmap;
private:
	mutable bool _repeating; // modified during rendering
};

#define UNREFTEX(x) {x = NULL;}

#endif
