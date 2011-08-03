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

PathRender::PathRender() : RenderObject()
{
	//color = Vector(1, 0, 0);
	position.z = 5;
	cull = false;
	alpha = 0.5f;
}

void PathRender::onRender()
{	
#ifdef BBGE_BUILD_OPENGL
	const int pathcount = dsq->game->getNumPaths();
	if (pathcount <= 0)
		return;

	for (int i = 0; i < pathcount; i++)
	{
		Path *p = dsq->game->getPath(i);
#ifdef AQUARIA_BUILD_SCENEEDITOR
		if (dsq->game->sceneEditor.selectedIdx == i)
			glColor4f(1, 1, 1, 0.75);
		else
#endif
			glColor4f(1, 0.5, 0.5, 0.75);

		glBegin(GL_LINES);
		for (int n = 0; n < p->nodes.size()-1; n++)
		{
			PathNode *nd = &p->nodes[n];
			PathNode *nd2 = &p->nodes[n+1];
			glVertex3f(nd->position.x, nd->position.y, 0);
			glVertex3f(nd2->position.x, nd2->position.y, 0);
		}
		glEnd();

		for (int n = 0; n < p->nodes.size(); n++)
		{
			PathNode *nd = &p->nodes[n];

			if (n == 0)
			{
				if (p->pathShape == PATHSHAPE_RECT)
				{
					glColor4f(0.5, 0.5, 1, 0.2);
					glBegin(GL_QUADS);
						glVertex2f(nd->position.x+p->rect.x1, nd->position.y+p->rect.y2);
						glVertex2f(nd->position.x+p->rect.x2, nd->position.y+p->rect.y2);
						glVertex2f(nd->position.x+p->rect.x2, nd->position.y+p->rect.y1);
						glVertex2f(nd->position.x+p->rect.x1, nd->position.y+p->rect.y1);														
					glEnd();
			
					glColor4f(1, 1, 1, 0.3);
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
					glColor4f(0.5, 0.5, 1, 0.5);
					glTranslatef(nd->position.x, nd->position.y, 0);
					drawCircle(p->rect.getWidth()*0.5f, 16);
					glTranslatef(-nd->position.x, -nd->position.y, 0);
				}
			}

			float a = 0.75;
			if (!p->active)
				a = 0.3;

#ifdef AQUARIA_BUILD_SCENEEDITOR
			if (dsq->game->sceneEditor.selectedIdx == i)
				glColor4f(1, 1, 1, a);
			else
#endif
				glColor4f(1, 0.5, 0.5, a);

			glPushMatrix();
			glTranslatef(nd->position.x, nd->position.y, 0);			
			drawCircle(32);
			glPopMatrix();
		}
	}
#endif
}
