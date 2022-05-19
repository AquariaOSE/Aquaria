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
#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "../BBGE/BitmapFont.h"
#include "../BBGE/Quad.h"

class ToolTip : public RenderObject
{
public:
	ToolTip();

	void setText(const std::string &text, const Vector &center, int width);

	void setArea(const Vector &p1, const Vector &p2);
	void setAreaFromCenter(const Vector &center, int width, int height);
	void setCircularAreaFromCenter(const Vector &center, int diameter);

	void render(const RenderState& rs) const OVERRIDE;

	static bool toolTipsOn;

	bool required;

protected:
	void onUpdate(float dt) OVERRIDE;

	int areaType;
	Vector areaCenter;
	int areaWidth, areaHeight;

	BitmapText *text;
	Quad *back;
};

#endif
