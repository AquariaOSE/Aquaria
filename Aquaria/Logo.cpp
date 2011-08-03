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
#include "Logo.h"

void Logo::JumpTitle::act()
{
	//dsq->title();
}

Logo::Logo() : StateObject()
{
	registerState(this, "Logo");
}

void Logo::applyState()
{
	StateObject::applyState();


	core->setClearColor(Vector(1,1,1));
	//glClearColor(1,1,1,0);


	//core->enable2D(800);

	/*
	Quad *q = new Quad;
	{
		q->setTexture("zs");
		q->width=800;
		q->height=100;
		q->position = Vector(400,300);
		q->alpha = 0;
		q->alpha.interpolateTo(1, 3, 0);
	}
	addRenderObject(q);

	addAction(&jumpTitle, MOUSE_BUTTON_LEFT, 1);
	*/

	Quad *q = new Quad;
	{
		q->setTexture("sky");
		q->setWidthHeight(800, 600);
		q->position = Vector(400,300);
		q->alpha = 0;
		q->alpha.interpolateTo(1, 3, 0);
	}
	addRenderObject(q);
}

void Logo::removeState()
{
	StateObject::removeState();
}
