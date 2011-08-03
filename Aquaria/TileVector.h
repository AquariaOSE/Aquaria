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

#include "../BBGE/Vector.h"

const int TILE_SIZE = 20;

class TileVector
{
public:
	TileVector(const Vector &vec)
	{
		x = int(vec.x/TILE_SIZE);
		y = int(vec.y/TILE_SIZE);
	}

	TileVector(int x, int y) : x(x),y(y) {}

	TileVector() : x(0),y(0) {}

	Vector worldVector() const
	{
		return Vector(x*TILE_SIZE+TILE_SIZE/2, y*TILE_SIZE+TILE_SIZE/2);
	}

	bool isZero() const
	{
		return (x==0 && y==0);
	}

	int x,y;
};

