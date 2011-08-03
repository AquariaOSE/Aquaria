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
#pragma once

#include "Base.h"


class FrameBuffer
{
public:
	FrameBuffer();	
	~FrameBuffer();
	bool init(int width, int height, bool fitToScreen=false, GLint filter=GL_LINEAR);
	bool isInited() { return inited; }
	bool isEnabled() { return enabled; }
	void setEnabled(bool e);
	void startCapture();
	void endCapture();
	void bindTexture();
	int getWidth() { return w; }
	int getHeight() { return h; }
	float getWidthP();
	float getHeightP();
	
	void unloadDevice();
	void reloadDevice();

#if defined(BBGE_BUILD_SDL)
	static void resetOpenGL();
#endif

protected:
	int _w, _h;
	bool _fitToScreen;
	GLuint g_frameBuffer;
	GLuint g_depthRenderBuffer;
	GLuint g_dynamicTextureID;
	int w,h;
	bool enabled, inited;
};

