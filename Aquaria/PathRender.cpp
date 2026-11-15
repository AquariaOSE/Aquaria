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
#include "GridRender.h"
#include "RenderBase.h"
#include "Game.h"

PathRender::PathRender()
	: RenderObject()
	, vbo(GPUBUF_VERTEXBUF | GPUBUF_DYNAMIC)
{
	position.z = 5;
	cull = false;
	alpha = 0.5f;
}

enum { BIG_CIRCLE_VERTS = 50, SMALL_CIRCLE_VERTS = 24 };

void PathRender::onUpdate(float dt)
{
	if(alpha.x <= 0.0f)
		return;

	drawcalls.clear();

	const size_t pathcount = game->getNumPaths();
	if (pathcount == 0)
		return;

	const size_t selidx = game->sceneEditor.selectedIdx;

	size_t verts = 0;
	for (size_t i = 0; i < pathcount; i++)
	{
		const Path *p = game->getPath(i);
		switch(p->pathShape)
		{
			case PATHSHAPE_RECT:
				verts += 4; // outline rect, GL_LINE_LOOP, 4 verts. re-used for overlay rect.
				break;
			case PATHSHAPE_CIRCLE:
				verts += BIG_CIRCLE_VERTS;
				break;
		}
		const size_t n = p->nodes.size();
		assert(n);
		if(n > 1)
			verts += n; // connecting line through each path point
		verts += SMALL_CIRCLE_VERTS * n; // path point circles
	}

	size_t bytes = verts * 2 * sizeof(float);

	float *p;
	do
	{
		p = (float*)vbo.beginWrite(GPUBUFTYPE_VEC2, bytes, GPUACCESS_DEFAULT);
		const float * const origin = (const float*)p;
#define INDEXOF(p) (((p) - origin) / 2u)

		for (size_t i = 0; i < pathcount; i++)
		{
			const Path *P = game->getPath(i);
			DrawCallParams dc;
			const PathNode * const c = &P->nodes[0];

			dc.color = selidx == i ? Vector(1,1,1) : P->editorColor;
			dc.alpha = 0.75f;

			const size_t n = P->nodes.size();
			if(n > 1)
			{
				dc.first = INDEXOF(p);
				dc.count = n;
				dc.mode = GL_LINE_STRIP;
				dc.linewidth = 4;
				for(size_t k = 0; k < n; ++k)
				{
					const PathNode *nd = &P->nodes[k];
					*p++ = nd->position.x;
					*p++ = nd->position.y;
				}
				drawcalls.push_back(dc);
			}

			dc.count = SMALL_CIRCLE_VERTS;
			dc.mode = GL_TRIANGLE_FAN;
			if (!P->active)
			{
				dc.alpha = 0.3f;
				dc.color *= 0.5f;
			}
			for(size_t k = 0; k < n; ++k)
			{
				dc.first = INDEXOF(p);
				p = drawCircle(p, 32, SMALL_CIRCLE_VERTS, P->nodes[k].position);
				drawcalls.push_back(dc);
			}

			dc.first = INDEXOF(p);

			switch(P->pathShape)
			{
				case PATHSHAPE_RECT:
					*p++ = c->position.x + P->rect.x1;
					*p++ = c->position.y + P->rect.y2;
					*p++ = c->position.x + P->rect.x2;
					*p++ = c->position.y + P->rect.y2;
					*p++ = c->position.x + P->rect.x2;
					*p++ = c->position.y + P->rect.y1;
					*p++ = c->position.x + P->rect.x1;
					*p++ = c->position.y + P->rect.y1;

					dc.mode = GL_TRIANGLE_FAN;
					dc.color = Vector(0.5f, 0.5f, 1);
					dc.alpha = 0.2f;
					dc.count = 4;
					drawcalls.push_back(dc);

					dc.mode = GL_LINE_LOOP;
					dc.color = Vector(1, 1, 1);
					dc.alpha = 0.3f;
					dc.linewidth = 1;
					// count is still 4
					drawcalls.push_back(dc);
					break;

				case PATHSHAPE_CIRCLE:
					p = drawCircle(p, P->rect.getWidth()*0.5f, BIG_CIRCLE_VERTS, c->position);
					dc.mode = GL_TRIANGLE_FAN;
					dc.color = Vector(0.5f, 0.5f, 1);
					dc.alpha = 0.4f;
					dc.count = BIG_CIRCLE_VERTS;
					drawcalls.push_back(dc);
					break;
			}
		}
	}
	while(!vbo.commitWriteExact(p));
}

void PathRender::onRender(const RenderState& rs) const
{
	const size_t n = drawcalls.size();
	if(!n)
		return;

	vbo.apply();
	for(size_t i = 0; i < n; ++i)
	{
		const DrawCallParams& dc = drawcalls[i];
		switch(dc.mode)
		{
			case GL_LINE_LOOP:
			case GL_LINE_STRIP:
			case GL_LINES:
				glLineWidth(dc.linewidth);
			default:
				break;
		}

		glColor4f(dc.color.x, dc.color.y, dc.color.z, dc.alpha);
		glDrawArrays(dc.mode, dc.first, dc.count);
	}
}
