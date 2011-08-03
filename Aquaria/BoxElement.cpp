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
#include "Element.h"

#include "../BBGE/Core.h"

BoxElement::BoxElement(int width, int height) : Element(BOX)
{
	blendEnabled = false;
	this->width = width;
	this->height = height;
	height+=2;


	ww = width/2 + 2;
	hh = height/2 + 10;
	this->color = 0;
	cull = true;
	//cull = false;
}

/*
bool BoxElement::isOnScreen()
{
	// HACK: biased towards being fast for rows
	//if (alpha.x < 1.0f) return false;
	//if (!cull) return true;

	if (this->position.y + hh >= core->screenCullY1
		&& this->position.y - hh <= core->screenCullY2)
	{
		if (this->position.x + ww >= core->screenCullX1
			&& this->position.x - ww <= core->screenCullX2)
		{
			return true;
		}
	}

	return false;
}
*/
