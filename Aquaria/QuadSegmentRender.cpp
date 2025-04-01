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

#include "QuadSegmentRender.h"
#include "RenderBase.h"
#include "Game.h"


CurrentRender::CurrentRender()
	: QuadSegmentRender(PATH_CURRENT, true)
{
	setTexture("particles/current");
}

CurrentRender::~CurrentRender()
{
}

SteamRender::SteamRender()
	: QuadSegmentRender(PATH_STEAM, false)
{
	setTexture("particles/steam");
	alpha = 0.7f;
	setBlendType(BLEND_ADD);
}

SteamRender::~SteamRender()
{
}

QuadSegmentRender::QuadSegmentRender(PathType pt, bool shortenSegs)
	: RenderObject()
	, vbo(GPUBUF_VERTEXBUF | GPUBUF_DYNAMIC)
	, _verticesToRender(0)
	, pathtype(pt)
	, _shortenSegs(shortenSegs)
{
	cull = false;
	repeatTexture = true;
}

QuadSegmentRender::~QuadSegmentRender()
{
}


void QuadSegmentRender::onRender(const RenderState& rs) const
{
	if(const size_t N = _verticesToRender)
	{
		vbo.apply();
		for(size_t i = 0; i < N; i += 8)
		{
			glDrawArrays(GL_TRIANGLE_STRIP, i, 8);
		}
	}
}

void QuadSegmentRender::onUpdate(float dt)
{
	// Ideally we wouldn't have to update this per-frame.
	// With a specialized shader and a uniform texcoordOffset variable the actual vertex data
	// could be map-static and this would all be so much simpler...

	size_t num = 0;
	for (const Path *P = game->getFirstPathOfType(pathtype); P; P = P->nextOfType)
	{
		if (!P->active)
			continue;

		num += P->nodes.size() - 1;
	}
	size_t verts = 0;
	if(num)
	{
		size_t bytes = num * 8 * 8 * sizeof(float); // num * 8 verts<xyuvrgba>
		size_t usedsize;
		do
		{
			// Usually we write much less data than maximally possible to the GPU,
			// because most segments are off-screen and not visible.
			// The hostcopy flag accumulates the used data in the host heap
			// and only uploads what is necessary in the end
			float *p = (float*)vbo.beginWrite(GPUBUFTYPE_VEC2_TC_RGBA, bytes, GPUACCESS_HOSTCOPY);
			verts = writeVBOData(p);
			usedsize = verts * 8 * sizeof(float);
		}
		while(!vbo.commitWrite(usedsize));
	}

	_verticesToRender = verts;
}

size_t QuadSegmentRender::writeVBOData(float *p)
{
	size_t ret = 0;
	const bool shorten = _shortenSegs;

	for (const Path *P = game->getFirstPathOfType(pathtype); P; P = P->nextOfType)
	{
		if (!P->active)
			continue;

		const float w = P->rect.getWidth();
		const float w2 = w * 0.5f;
		const float ao = P->animOffset;
		const float a = P->amount * alpha.x;

		size_t sz = P->nodes.size()-1;
		for (size_t n = 0; n < sz; n++)
		{
			const PathNode *n1 = &P->nodes[n];
			const PathNode *n2 = &P->nodes[n+1];
			Vector p1 = n1->position;
			Vector p2 = n2->position;
			Vector diff = p2-p1;
			if(shorten)
			{
				Vector d = diff;
				d.setLength2D(w);
				p1 -= d*0.75f;
				p2 += d*0.75f;
			}
			diff = p2 - p1;

			if (diff.isZero())
				continue;

			if (isTouchingLine(p1, p2, dsq->screenCenter, dsq->cullRadius+w2))
			{
				Vector pl = diff.getPerpendicularLeft();
				Vector pr = diff.getPerpendicularRight();
				pl.setLength2D(w2);
				pr.setLength2D(w2);

				Vector p15 = p1 + diff * 0.25f;
				Vector p25 = p2 - diff * 0.25f;
				Vector r1 = p1+pl;
				Vector r2 = p1+pr;
				Vector r3 = p15+pl;
				Vector r4 = p15+pr;
				Vector r5 = p25+pl;
				Vector r6 = p25+pr;
				Vector r7 = p2+pl;
				Vector r8 = p2+pr;

				const float len = diff.getLength2D();
				const float texScale = len/256.0f;

				/* This builds a structure like this:
				a = 0 alpha
				A = set alpha
				<---25%-->                    <---25%-->
				a        A                    A        a
				+--------+--------------------+--------+
				|        |                    |        |
				|        |                    |        |
				+--------+--------------------+--------+
				where each path point is roughly at A,
				and the fading overlaps patch over the gaps.
				*/

#define VERTEX(X, Y, TX, TY, A) do { \
*p++ = X; *p++ = Y; *p++ = TX; *p++ = TY; \
*p++ = 1; *p++ = 1; *p++ = 1; *p++ = A; } while(0)

				// These 8 verts form a triangle strip
				ret += 8;
				VERTEX(r1.x, r1.y, (0      )*texScale+ao, 0,    0);
				VERTEX(r2.x, r2.y, (0      )*texScale+ao, 1,    0);
				VERTEX(r3.x, r3.y, (0+0.25f)*texScale+ao, 0,    a);
				VERTEX(r4.x, r4.y, (0+0.25f)*texScale+ao, 1,    a);
				VERTEX(r5.x, r5.y, (1-0.25f)*texScale+ao, 0,    a);
				VERTEX(r6.x, r6.y, (1-0.25f)*texScale+ao, 1,    a);
				VERTEX(r7.x, r7.y, (1      )*texScale+ao, 0,    0);
				VERTEX(r8.x, r8.y, (1      )*texScale+ao, 1,    0);
			}
		}
	}

	return ret;
}

