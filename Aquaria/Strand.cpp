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

Strand::Strand(const Vector &position, size_t segs, size_t dist)
	: RenderObject(), Segmented(dist, dist)
	, gpubuf(GPUBUF_DYNAMIC | GPUBUF_VERTEXBUF)
{
	assert(segs);
	cull = false;
	segments.resize(segs);
	for (size_t i = 0; i < segments.size(); i++)
	{
		// FIXME: This is super costly to waste an entire RenderObject just to store a position.
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

	const size_t numSegments = segments.size();

	do
	{
		const size_t bytes = (numSegments+1) * 6 * sizeof(float);
		float *p = (float*)gpubuf.beginWrite(GPUBUFTYPE_VEC2_RGBA, bytes, GPUACCESS_DEFAULT);

		// Note: We're rewriting all of the vertex data here.
		// Ideally we'd only rewrite position data since vertex colors never change,
		// but currently this is to keep buffer layouts simple.

		const float factor = 1.0f / 50.0f;
		const size_t colorLimit = numSegments<50 ? numSegments : 50;
		const Vector falloff = color * factor;
		const float falloffAlpha = 1.0f / float(numSegments);
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
			*p++ = segments[i]->position.x;
			*p++ = segments[i]->position.y;

			*p++ = c.x;
			*p++ = c.y;
			*p++ = c.z;
			*p++ = a;

			c -= falloff;
			a -= falloffAlpha;
		}
		for( ; i < numSegments; ++i)
		{
			*p++ = segments[i]->position.x;
			*p++ = segments[i]->position.y;

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
	glDrawArrays(GL_LINE_STRIP, 0, GLsizei(segments.size() + 1));
}
