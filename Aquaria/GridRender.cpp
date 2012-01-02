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
#include "Game.h"

GridRender::GridRender(ObsType obsType) : RenderObject()
{
	color = Vector(1, 0, 0);
	//color = Vector(0.2,0.2,1);
	position.z = 5;
	cull = false;
	alpha = 0.5f;
	this->obsType = obsType;
	blendEnabled = false;
	//setTexture("grid");
}

void GridRender::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);	
	if (obsType != OT_BLACK) { blendEnabled = true; }
}

void GridRender::onRender()
{
	switch(obsType)
	{
	case OT_INVISIBLE:
		core->setColor(1, 0, 0, alpha.getValue());
	break;
	case OT_INVISIBLEIN:
		core->setColor(1, 0.5, 0, alpha.getValue());
	break;
	case OT_BLACK:
		core->setColor(0, 0, 0, 1);
	break;
	case OT_HURT:
		core->setColor(1, 1, 0, alpha.getValue());
	break;
	default:
	break;
	}

	const int obsType = int(this->obsType);
	Vector camPos = core->cameraPos;
	camPos.x -= core->getVirtualOffX() * (core->invGlobalScale);
	const TileVector ct(camPos);

	const int width = int((core->getVirtualWidth() * (core->invGlobalScale))/TILE_SIZE) + 1;
	const int height = int((600 * (core->invGlobalScale))/TILE_SIZE) + 1;

	int startX = ct.x-1, endX = ct.x+width+1;
	int startY = ct.y-1, endY = ct.y+height+1;
	if (startX < 0)
		startX = 0;
	if (endX >= MAX_GRID)
		endX = MAX_GRID-1;
	if (startY < 0)
		startY = 0;
	if (endY >= MAX_GRID)
		endY = MAX_GRID-1;
	for (int x = startX; x <= endX; x++)
	{
		const signed char *gridColumn = dsq->game->getGridColumn(x);
		int startCol = -1, endCol;
		for (int y = startY; y <= endY; y++)
		{
			int v = gridColumn[y];
			// HACK: Don't draw the leftmost or rightmost column of
			// black tiles (otherwise they "leak out" around the
			// edges of the Sun Temple).  --achurch
			if (v == OT_BLACK && ((dsq->game->getGridColumn(x-1))[y] != OT_BLACK || (dsq->game->getGridColumn(x+1))[y] != OT_BLACK))
				v = OT_EMPTY;

			if (v == obsType && startCol == -1)
			{
				startCol = y;
			}
			else if ((v != obsType || y == endY) && startCol != -1)
			{
				endCol = y;
				if (v != obsType)
					endCol--;

				const float drawx1 = x*TILE_SIZE;
				const float drawx2 = (x+1)*TILE_SIZE;
				const float drawy1 = startCol*TILE_SIZE;
				const float drawy2 = (endCol+1)*TILE_SIZE;

#ifdef BBGE_BUILD_OPENGL
				glBegin(GL_QUADS);
					glVertex3f(drawx1, drawy2, 0.0f);
					glVertex3f(drawx2, drawy2, 0.0f);
					glVertex3f(drawx2, drawy1, 0.0f);
					glVertex3f(drawx1, drawy1, 0.0f);
				glEnd();
#endif

#ifdef BBGE_BUILD_DIRECTX
				core->blitD3DVerts(0,
					drawx1, drawy1,
					drawx2, drawy1,
					drawx2, drawy2,
					drawx1, drawy2);
#endif
				startCol = -1;
			}
		}
	}
}


SongLineRender::SongLineRender()
{
	followCamera = 1;
	cull = false;
}

void SongLineRender::newPoint(const Vector &pt, const Vector &color)
{
	int maxx = 40;
	bool inRange = true;
	if (pts.size() > 1)
		inRange = (pt - pts[pts.size()-2].pt).isLength2DIn(4);
	if (pts.size()<2 || !inRange)
	{
		SongLinePoint s;
		s.pt = pt;
		s.color = color;
		pts.push_back(s);
		if (pts.size() > maxx)
		{
			std::vector<SongLinePoint> copy;
			copy = pts;
			pts.clear();
			for (int i = 1; i < copy.size(); i++)
			{
				pts.push_back(copy[i]);
			}
		}
			
	}
	else if (!pts.empty() && inRange)
	{
		pts[pts.size()-1].color = color;
		pts[pts.size()-1].pt = pt;
	}
}

void SongLineRender::clear()
{
	pts.clear();
}

void SongLineRender::onRender()
{
	int w=core->getWindowWidth();
	//core->getWindowWidth(&w);
	int ls = (4*w)/1024.0f;
	if (ls < 0)
		ls = 1;
#ifdef BBGE_BUILD_OPENGL
	glLineWidth(ls);
	const int alphaLine = pts.size()*(0.9f);
	float a = 1;
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < pts.size(); i++)
	{
		if (i < alphaLine)
			a = float(i)/float(alphaLine);
		else
			a = 1;		
		glColor4f(pts[i].color.x, pts[i].color.y, pts[i].color.z, a);
		glVertex2f(pts[i].pt.x, pts[i].pt.y);
	}
	glEnd();
#endif
}

