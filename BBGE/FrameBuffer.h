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
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "Base.h"

class FrameBufferCapture;
struct RenderState;


class FrameBuffer
{
	friend class FrameBufferCapture;
public:
	FrameBuffer();
	~FrameBuffer();
	bool init(int width, int height, unsigned pages);
	bool isInited() const { return inited; }
	unsigned getTextureID(unsigned page) const;
	void bindTexture(unsigned page) const;
	int getTexWidth() const { return texw; }
	int getTexHeight() const { return texh; }
	float getWidthP() const;
	float getHeightP() const;
	bool getCurrentPage() const;

	void unloadDevice();
	void reloadDevice();

	// push/pop capture stack
	void pushCapture(unsigned page) const;
	unsigned popCapture() const; // returns page

	// replace top of capture stack with this
	void replaceCapture(unsigned page) const;

protected:
	void _bind(unsigned page) const;

	unsigned _fbos[8];
	unsigned _texs[8];

	int _w, _h;
	mutable unsigned _curpage; // 0 if not currently bound
	unsigned _numpages, _numfbos;
	int texw, texh;
	int viewportW, viewportH;
	bool inited;
};

#endif
