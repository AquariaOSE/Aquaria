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


class FrameBuffer
{
public:
	FrameBuffer();
	~FrameBuffer();
	bool init(int width, int height, bool fitToScreen=false);
	bool isInited() const { return inited; }
	void startCapture() const;
	void endCapture() const;
	void bindTexture() const;
	int getTexWidth() const { return texw; }
	int getTexHeight() const { return texh; }
	float getWidthP() const;
	float getHeightP() const;

	void unloadDevice();
	void reloadDevice();

protected:
	int _w, _h;
	bool _fitToScreen;
	unsigned g_frameBuffer;
	unsigned g_depthRenderBuffer;
	unsigned g_dynamicTextureID;
	int texw, texh;
	bool inited;
};

#endif
