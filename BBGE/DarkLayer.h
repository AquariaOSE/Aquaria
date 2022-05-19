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

class DarkLayer
{
public:
	DarkLayer();
	void init(int quality, bool useFrameBuffer=true);
	void toggle(bool on);
	void setLayers(int layer, int renderLayer);
	void preRender();
	void render() const;
	int getLayer();
	int getRenderLayer();
	bool isUsed();

	void unloadDevice();
	void reloadDevice();

	bool useFrameBuffer;
	FrameBuffer frameBuffer;
protected:
	float stretch;
	int quality;
	bool active;
	int layer, renderLayer;
	unsigned texture;
	unsigned format;
};

#endif
