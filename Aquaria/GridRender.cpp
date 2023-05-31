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
#include "RenderBase.h"

GridRender::GridRender(ObsType obsType) : RenderObject()
{
	color = Vector(1, 0, 0);

	position.z = 5;
	cull = false;
	alpha = 0.5f;
	this->obsType = obsType;
}

void GridRender::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
}

inline static void doRenderGrid(int x, int startCol, int endCol)
{
	const int drawx1 = x*TILE_SIZE;
	const int drawx2 = (x+1)*TILE_SIZE;
	const int drawy1 = startCol*TILE_SIZE;
	const int drawy2 = (endCol+1)*TILE_SIZE;

	glBegin(GL_QUADS);
	glVertex3i(drawx1, drawy2, 0.0f);
	glVertex3i(drawx2, drawy2, 0.0f);
	glVertex3i(drawx2, drawy1, 0.0f);
	glVertex3i(drawx1, drawy1, 0.0f);
	glEnd();

}

void GridRender::onRender(const RenderState& rs) const
{
	const signed char obsType = this->obsType;
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
	if (startY > endY)
		return;
	for (int x = startX; x <= endX; ++x)
	{
		const unsigned char *gridColumn = game->getGridColumn(x);
		int startCol = -1, y;

		// fast-forward to next drawable byte
		if(const unsigned char *next = (const unsigned char*)memchr(gridColumn + startY, obsType, endY - startY + 1)) // find next byte with correct obs type
		{
			y = next - gridColumn; // will get incremented right away, which is okay, because we alrady set startCol
			startCol = y;
		}
		else
			continue; // nothing do draw in this column

		for ( ; y < endY; ++y)
		{
			if (gridColumn[y] != obsType)
			{
				doRenderGrid(x, startCol, y - 1);

				// fast-forward to next drawable byte
				if(const unsigned char *next = (const unsigned char*)memchr(gridColumn + y, obsType, endY - y)) // find next byte with correct obs type
				{
					y = next - gridColumn; // will get incremented right away, which is okay, because we alrady set startCol
					startCol = y;
				}
				else
					break;
			}
		}
		if (y == endY)
		{
			doRenderGrid(x, startCol, y);
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
	size_t maxx = 40;
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
			pts.erase(pts.begin());
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

void SongLineRender::onRender(const RenderState& rs) const
{
	int w=core->getWindowWidth();

	int ls = (4*w)/1024.0f;
	if (ls < 0)
		ls = 1;
	glLineWidth(ls);
	const unsigned int alphaLine = pts.size()*(0.9f);
	float a = 1;
	glBegin(GL_LINE_STRIP);
	for (size_t i = 0; i < pts.size(); i++)
	{
		if (i < alphaLine)
			a = float(i)/float(alphaLine);
		else
			a = 1;
		glColor4f(pts[i].color.x, pts[i].color.y, pts[i].color.z, a);
		glVertex2f(pts[i].pt.x, pts[i].pt.y);
	}
	glEnd();
}

