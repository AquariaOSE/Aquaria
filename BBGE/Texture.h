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

struct TexCoordBox
{
	float u1, v1; // upper left (x,y)
	float u2, v2; // lower right (x,y)

	bool isStandard() const;
	void setStandard();

	bool operator==(const TexCoordBox& o) const
	{
		return u1 == o.u1 && v1 == o.v1 && u2 == o.u2 && v2 == o.v2;
	}
	inline bool operator!=(const TexCoordBox& o) const
	{
		return !(*this == o);
	}
};

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

	void apply() const;
	void unload();

	unsigned gltexid;
	int width, height;

	void writeRGBA(int tx, int ty, int w, int h, const unsigned char *pixels);
	void readRGBA(unsigned char *pixels) const;

	const unsigned char *getBufferAndSize(int *w, int *h, size_t *size) const; // returned memory is owned by the Texture
	size_t sizeBytes() const;

	std::string name, filename;
	bool upload(const ImageData& img, bool mipmap);
	bool uploadAndKeep(ImageData& img, bool mipmap);

	bool success;

protected:
	void _freePixbuf();

	int ow, oh;
	bool _mipmap;
	mutable unsigned char *_pixbuf; // retrieved when needed
};

#define UNREFTEX(x) {x = NULL;}

#endif
