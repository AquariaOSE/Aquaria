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
#include "Quad.h"


OutlineRect::OutlineRect() : RenderObject()
{
	lineSize = 1;
	renderCenter = false;
}

void OutlineRect::setWidthHeight(int w, int h)
{
	this->w = w;
	this->h = h;
	w2 = w/2;
	h2 = h/2;
}

void OutlineRect::setLineSize(int ls)
{
	lineSize = ls;
}

void OutlineRect::onRender()
{
#ifdef BBGE_BUILD_OPENGL
	glLineWidth(lineSize);
	glBegin(GL_LINES);
		// l
		glVertex2f(-w2,-h2);
		glVertex2f(-w2,h2);
		// r
		glVertex2f(w2,-h2);
		glVertex2f(w2,h2);
		// u
		glVertex2f(-w2,-h2);
		glVertex2f(w2,-h2);
		// d
		glVertex2f(-w2,h2);
		glVertex2f(w2,h2);
	glEnd();

	if (renderCenter)
	{
		glColor3f(0.9, 0.9, 1);
		glBegin(GL_LINES);
			// lr
			glVertex2f(-w2,0);
			glVertex2f(w2,0);
			// ud
			glVertex2f(0,-h2);
			glVertex2f(0,h2);
		glEnd();
	}
#endif
}


