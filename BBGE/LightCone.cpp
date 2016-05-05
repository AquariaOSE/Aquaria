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
#include "LightCone.h"

LightCone::LightCone() : RenderObject()
{
	length = 64;
	spread = 32;
}

void LightCone::onRender()
{
	RenderObject::onRender();
	glBegin(GL_QUADS);
		//glNormal3f( 0.0f, 0, 1.0f);
		glColor4f(color.x, color.y, color.z, 1*alpha.x);
		glVertex3f(0, 0,  0);
		glColor4f(color.x, color.y, color.z,1*alpha.x);
		glVertex3f(0, 0,  0);
		glColor4f(color.x, color.y, color.z,0*alpha.x);
		glVertex3f(spread/2, length, 0);
		glColor4f(color.x, color.y, color.z,0*alpha.x);
		glVertex3f(-spread/2,  length,  0);
	glEnd();
}
