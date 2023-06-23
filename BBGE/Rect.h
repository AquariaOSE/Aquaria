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
#ifndef BBGE_RECT_H
#define BBGE_RECT_H

#include "Vector.h"

class RectShape
{
public:
	RectShape(int x1, int y1, int x2, int y2) : x1(x1), y1(y1), x2(x2), y2(y2) {}
	RectShape()
	{
		x1 = y1 = x2 = y2 = 0;
	}
	int getWidth() const { return x2-x1; }
	int getHeight() const { return y2-y1; }
	void setWidth(int v)
	{
		x1 = -v/2;
		x2 = v/2;
	}
	void setHeight(int v)
	{
		y1 = -v/2;
		y2 = v/2;
	}
	void setCWH(int x, int y, int w, int h)
	{
		const int w2 = w / 2;
		const int h2 = h / 2;
		x1 = x - w2;
		y1 = y - h2;
		x2 = x + w2;
		y2 = y + h2;
	}
	void getCWH(int *x, int *y, int *w, int *h) const
	{
		*w = x2 - x1;
		*h = y2 - y1;
		*x = x1 + ((*w) / 2);
		*y = y1 + ((*h) / 2);
	}
	bool isCoordinateInside(const Vector &vec, float radius=0) const
	{
		return ((vec.x >= x1-radius && vec.x <= x2+radius) && (vec.y >= y1-radius && vec.y <= y2+radius));
	}
	bool isEmpty() const
	{
		return x1 != 0 || y1 != 0 || x2 != 0 || y2 != 0;
	}
	int x1, y1, x2, y2;
};

#endif
