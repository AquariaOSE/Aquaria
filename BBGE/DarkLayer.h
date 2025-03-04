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
#ifndef DARKLAYER_H
#define DARKLAYER_H

#include "Base.h"
#include "FrameBuffer.h"
#include "VertexBuffer.h"

struct RenderState;

class DarkLayer
{
public:
	DarkLayer();
	void init(int quality, bool useFrameBuffer=true);
	void toggle(bool on);
	void setSrcLayer(int layer);
	void preRender(); // call before rendering anything else. this is for render-to-texture mode
	void beginCapture(); // call this + endCapture() when rendering the layer where dark layer objects are located
	void endCapture();

	void render(const RenderState& rs) const;
	bool isUsed() const;
	bool shouldRenderLayer(int lr) const;

	void unloadDevice();
	void reloadDevice();

	int beginLayer;
	int lastLayer; // inclusive

protected:
	bool useFrameBuffer;
	FrameBuffer frameBuffer;
	int quality;
	int layer;
	bool active;
	unsigned texture;
	DynamicGPUBuffer vbo;
};

#endif
