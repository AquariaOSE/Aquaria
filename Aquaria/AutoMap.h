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
#ifndef AUTOMAP_H
#define AUTOMAP_H

#include "../BBGE/ActionMapper.h"
#include "../BBGE/RenderObject.h"

#define MAX_AUTOMAP_GRID		64

class AutoMap : public RenderObject, public ActionMapper
{
public:
	AutoMap();
	void destroy();
	void toggle(bool on);
	bool isOn();
	void setGridFromWorld(Vector worldPos, int gridValue);
	void initGrid();
	Texture *shadowTex;
	void paint(const Vector &pos);
	void lmb();
protected:
	int autoMapMode;
	Vector paintColor;
	bool initedGrid;
	int grid[MAX_AUTOMAP_GRID][MAX_AUTOMAP_GRID];
	InterpolatedVector blip;
	bool vis;
	void onUpdate(float dt);
	void onRender();
};

#endif
