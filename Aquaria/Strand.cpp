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
#include "Strand.h"
#include "RenderBase.h"

Strand::Strand(const Vector &position, size_t segs, float dist)
	: RenderObject()
	, points(segs)
	, gpubuf(GPUBUF_DYNAMIC | GPUBUF_VERTEXBUF)
	, dist(dist)
{
	assert(segs);
	cull = false;
	for (size_t i = 0; i < segs; i++)
		points[i] = position;
}

void Strand::updatePoints()
{
	Vector last = position;
	const float distsq = sqr(dist);
	const size_t N = points.size();
	for(size_t i = 0; i < N; ++i)
	{
		Vector pt = points[i];
		const Vector diff = last - pt;
		last = pt;
		const float sqrLength = diff.getSquaredLength2D();

		if (sqrLength < distsq)
			continue;

		Vector useDiff = diff;
		useDiff.setLength2D(dist);
		Vector reallyUseDiff = diff - useDiff;
		points[i] = pt + reallyUseDiff;
	}
}

void Strand::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
	updatePoints();

	const size_t N = points.size();

	do
	{
		const size_t bytes = (N+1) * 6 * sizeof(float); // +1 for the initial origin vertex
		float *p = (float*)gpubuf.beginWrite(GPUBUFTYPE_VEC2_RGBA, bytes, GPUACCESS_DEFAULT);

		// Note: We're rewriting all of the vertex data here.
		// Ideally we'd only rewrite position data since vertex colors never change,
		// but currently this is to keep buffer layouts simple.

		const float factor = 1.0f / 50.0f;
		const size_t colorLimit = N<50 ? N : 50;
		const Vector falloff = color * factor;
		const float falloffAlpha = 1.0f / float(N);
		float a = 1.0f;
		Vector c = color;

		// initial vertex (this is the +1)
		*p++ = position.x;
		*p++ = position.y;

		*p++ = c.x;
		*p++ = c.y;
		*p++ = c.z;
		*p++ = a;

		size_t i;
		for(i = 0; i < colorLimit; ++i)
		{
			*p++ = points[i].x;
			*p++ = points[i].y;

			*p++ = c.x;
			*p++ = c.y;
			*p++ = c.z;
			*p++ = a;

			c -= falloff;
			a -= falloffAlpha;
		}
		for( ; i < N; ++i)
		{
			*p++ = points[i].x;
			*p++ = points[i].y;

			*p++ = 0.0f;
			*p++ = 0.0f;
			*p++ = 0.0f;
			*p++ = a;

			a -= falloffAlpha;
		}
	}
	while(!gpubuf.commitWrite());
}

void Strand::onRender(const RenderState& rs) const
{
	glTranslatef(-position.x, -position.y, 0);
	glLineWidth(1);

	gpubuf.apply();
	glDrawArrays(GL_LINE_STRIP, 0, GLsizei(points.size() + 1));
}
