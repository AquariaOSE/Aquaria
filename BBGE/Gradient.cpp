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
#include "Gradient.h"
#include "Core.h"
#include "RenderBase.h"

Gradient::Gradient() : RenderObject()
{
	autoWidth = autoHeight = 0;
}

void Gradient::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (autoWidth == AUTO_VIRTUALWIDTH)
	{
		scale.x = core->getVirtualWidth();
	}

	if (autoHeight == AUTO_VIRTUALHEIGHT)
	{
		scale.y = core->getVirtualWidth();
	}
}

void Gradient::makeVertical(Vector c1, Vector c2)
{
	ulc0 = c1;
	ulc1 = c1;
	ulc2 = c2;
	ulc3 = c2;
}

void Gradient::makeHorizontal(Vector c1, Vector c2)
{
	ulc0 = c1;
	ulc1 = c2;
	ulc2 = c2;
	ulc3 = c1;
}

void Gradient::onRender(const RenderState& rs) const
{


	glBegin(GL_QUADS);


		glColor4f(ulc2.x*color.x, ulc2.y*color.y, ulc2.z*color.z, alpha.x);
		glVertex3f(-0.5, 0.5,  0.0f);


		glColor4f(ulc3.x*color.x, ulc3.y*color.y, ulc3.z*color.z, alpha.x);
		glVertex3f( 0.5, 0.5,  0.0f);


		glColor4f(ulc0.x*color.x, ulc0.y*color.y, ulc0.z*color.z, alpha.x);
		glVertex3f( 0.5,  -0.5,  0.0f);


		glColor4f(ulc1.x*color.x, ulc1.y*color.y, ulc1.z*color.z, alpha.x);
		glVertex3f(-0.5,  -0.5,  0.0f);

	glEnd();
}

