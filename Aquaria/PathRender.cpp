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

PathRender::PathRender() : RenderObject()
{

	position.z = 5;
	cull = false;
	alpha = 0.5f;
}

void PathRender::onRender(const RenderState& rs) const
{
	const size_t pathcount = game->getNumPaths();
	if (pathcount == 0)
		return;

	glLineWidth(4);

	const size_t selidx = game->sceneEditor.selectedIdx;

	for (size_t i = 0; i < pathcount; i++)
	{
		const Path *p = game->getPath(i);

		if (selidx == i)
			glColor4f(1, 1, 1, 0.75f);
		else if(p->hasScript())
			glColor4f(0.5f, 0.8f, 0.5f, 0.75f);
		else
			glColor4f(1, 0.5f, 0.5f, 0.75f);

		glBegin(GL_LINES);
		for (size_t n = 0; n < p->nodes.size()-1; n++)
		{
			const PathNode *nd = &p->nodes[n];
			const PathNode *nd2 = &p->nodes[n+1];
			glVertex3f(nd->position.x, nd->position.y, 0);
			glVertex3f(nd2->position.x, nd2->position.y, 0);
		}
		glEnd();
	}

	glLineWidth(1);

	for (size_t i = 0; i < pathcount; i++)
	{
		const Path *p = game->getPath(i);
		for (size_t n = 0; n < p->nodes.size(); n++)
		{
			const PathNode *nd = &p->nodes[n];

			if (n == 0)
			{
				if (p->pathShape == PATHSHAPE_RECT)
				{
					glColor4f(0.5f, 0.5f, 1, 0.2f);
					glBegin(GL_QUADS);
						glVertex2f(nd->position.x+p->rect.x1, nd->position.y+p->rect.y2);
						glVertex2f(nd->position.x+p->rect.x2, nd->position.y+p->rect.y2);
						glVertex2f(nd->position.x+p->rect.x2, nd->position.y+p->rect.y1);
						glVertex2f(nd->position.x+p->rect.x1, nd->position.y+p->rect.y1);
					glEnd();

					glColor4f(1, 1, 1, 0.3f);
					glBegin(GL_LINES);
						glVertex2f(nd->position.x+p->rect.x1, nd->position.y+p->rect.y1);
						glVertex2f(nd->position.x+p->rect.x2, nd->position.y+p->rect.y1);
						glVertex2f(nd->position.x+p->rect.x2, nd->position.y+p->rect.y1);
						glVertex2f(nd->position.x+p->rect.x2, nd->position.y+p->rect.y2);
						glVertex2f(nd->position.x+p->rect.x2, nd->position.y+p->rect.y2);
						glVertex2f(nd->position.x+p->rect.x1, nd->position.y+p->rect.y2);
						glVertex2f(nd->position.x+p->rect.x1, nd->position.y+p->rect.y2);
						glVertex2f(nd->position.x+p->rect.x1, nd->position.y+p->rect.y1);
					glEnd();
				}
				else
				{
					glColor4f(0.5f, 0.5f, 1, 0.4f);
					glTranslatef(nd->position.x, nd->position.y, 0);
					drawCircle(p->rect.getWidth()*0.5f, 16);
					glTranslatef(-nd->position.x, -nd->position.y, 0);
				}
			}

			Vector color = p->hasScript() ? Vector(0.5f, 0.8f, 0.5f) : Vector(1, 0.5f, 0.5f);
			float a = 0.75f;
			if (!p->active)
			{
				a = 0.3f;
				color *= 0.5f;
			}

			if (selidx == i)
				color.x = color.y = color.z = 1;

			glColor4f(color.x, color.y, color.z, a);
			glPushMatrix();
			glTranslatef(nd->position.x, nd->position.y, 0);
			drawCircle(32);
			glPopMatrix();
		}
	}
}
