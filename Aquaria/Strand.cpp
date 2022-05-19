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
#include "Segmented.h"
#include "RenderBase.h"

Strand::Strand(const Vector &position, size_t segs, size_t dist) : RenderObject(), Segmented(dist, dist)
{
	cull = false;
	segments.resize(segs);
	for (size_t i = 0; i < segments.size(); i++)
	{
		segments[i] = new RenderObject;
	}
	initSegments(position);
}

void Strand::destroy()
{
	RenderObject::destroy();
	for (size_t i = 0; i < segments.size(); i++)
	{
		segments[i]->destroy();
		delete segments[i];
	}
	segments.clear();
}

void Strand::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
	updateSegments(position);
}

void Strand::onRender(const RenderState& rs) const
{
	const int numSegments = segments.size();
	if (numSegments == 0) return;

	glEnable(GL_BLEND);
	glTranslatef(-position.x, -position.y, 0);
	glLineWidth(1);

	glBegin(GL_LINE_STRIP);


	unsigned int r = (unsigned int)(color.x * (255<<8));
	unsigned int g = (unsigned int)(color.y * (255<<8));
	unsigned int b = (unsigned int)(color.z * (255<<8));
	unsigned int a = (255<<8);
	unsigned int dr = r/50;
	unsigned int dg = g/50;
	unsigned int db = b/50;
	unsigned int da = a/numSegments;
	glColor4ub(r>>8, g>>8, b>>8, a>>8);
	glVertex2f(position.x, position.y);
	glVertex2f(segments[0]->position.x, segments[0]->position.y);
	const int colorLimit = numSegments<50 ? numSegments : 50;
	int i;
	for (i = 1; i < colorLimit; i++)
	{
		r -= dr;
		g -= dg;
		b -= db;
		a -= da;
		glColor4ub(r>>8, g>>8, b>>8, a>>8);
		glVertex2f(segments[i]->position.x, segments[i]->position.y);
	}
	for (; i < numSegments; i++)
	{
		a -= da;
		glColor4ub(0, 0, 0, a>>8);
		glVertex2f(segments[i]->position.x, segments[i]->position.y);
	}
	glEnd();
}
