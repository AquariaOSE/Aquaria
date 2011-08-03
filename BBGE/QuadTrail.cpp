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
#include "QuadTrail.h"
#include <assert.h>

QuadTrail::QuadTrail(int maxPoints, float pointDist)
: RenderObject(), maxPoints(maxPoints), pointDist(pointDist), numPoints(0)
{
	quadTrailAlphaEffect = QTAE_NORMAL;
	cull = false;
	repeatTexture = 1;

	lifeRate = 0.5;
}

void QuadTrail::addPoint(const Vector &point)
{
	if (numPoints > 0)
	{
		if ((points.back().point - point).isLength2DIn(pointDist))
		{
			backOffset = point - points.back().point;
			return;
		}
	}

	QuadTrailPoint p;
	p.point = point;

	points.push_back(p);
	numPoints++;
	if (numPoints >= maxPoints)
		points.pop_front();

	backOffset.x = 0;
	backOffset.y = 0;
}

void QuadTrail::onRender()
{
	if (numPoints < 2) return;

#ifdef BBGE_BUILD_OPENGL
	glDisable(GL_CULL_FACE);
	int c = 0;
	Vector p, diff, dl, dr;
	Vector lastPoint;

	const float texScale = texture ? float(numPoints*pointDist)/texture->width : 1.0f;

	glBegin(GL_QUAD_STRIP);
		for (Points::iterator i = points.begin(); i != points.end(); i++)
		{
			if (quadTrailAlphaEffect == QTAE_NORMAL)
			{
				glColor4f(1, 1, 1, (*i).life);
			}
			if (c == 0)
			{
				lastPoint = (*i).point;
				c++;
				continue;
			}
			p = (*i).point;

			if (c == numPoints-1)
				p += backOffset;

			diff = p - lastPoint;
			//possible opt here
			if (texture)
				diff.setLength2D(texture->width*0.5f);
			else
				diff.setLength2D(32);
			dl = diff.getPerpendicularLeft();
			dr = diff.getPerpendicularRight();

			glTexCoord2f(0, (float(c)/numPoints)*texScale);
			glVertex2f(p.x+dl.x, p.y+dl.y);
			glTexCoord2f(1, (float(c+1)/numPoints)*texScale);
			glVertex2f(p.x+dr.x, p.y+dr.y);

			c++;
			lastPoint = (*i).point;
		}
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);

	glPointSize(4);
	glColor4f(0, 1, 0, 0.5);

	glBegin(GL_POINTS);
		for (Points::iterator i = points.begin(); i != points.end(); i++)
		{
			glVertex2f((*i).point.x, (*i).point.y);
		}
	glEnd();
#endif
}

void QuadTrail::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	for (Points::iterator i = points.begin(); i != points.end(); i++)
	{
		(*i).life -= dt * lifeRate;
		if ((*i).life <= 0)
			(*i).life = 0;
	}
}
