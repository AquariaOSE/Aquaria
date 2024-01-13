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


static void collectRows(std::vector<ObsRow>& rows, ObsType obs)
{
	const TileVector gs = game->getGridSize();
	const size_t endX = std::min(gs.x, MAX_GRID);
	const size_t endY = std::min(gs.y, MAX_GRID);

	for(size_t y = 0; y < endY; ++y)
	{
		bool on = game->getGridRaw(TileVector(0, y)) == obs;
		size_t startx = 0;
		for(size_t x = 1; x < endX; ++x)
		{
			const ObsType ot = game->getGridRaw(TileVector(x, y));
			if(ot == obs)
			{
				if(!on)
				{
					startx = x;
					on = true;
				}
			}
			else
			{
				if(on)
				{
					// previous tile is the last one, so -1
					rows.push_back(ObsRow(startx, y, x - startx));
					on = false;
				}
			}
		}
		if(on)
			rows.push_back(ObsRow(startx, y, endX - startx));
	}
}

GridRender::GridRender(ObsType obsType)
	: RenderObject()
	, vbo(GPUBUF_VERTEXBUF | GPUBUF_STATIC)
	, primsToDraw(0)
	, obsType(obsType)
	//, ibo(GPUBUF_INDEXBUF | GPUBUF_STATIC)
	, markedForRebuild(true)
{
	color = Vector(1, 0, 0);

	position.z = 5;
	cull = false;
	alpha = 0.5f;
	this->scale.x = TILE_SIZE;
	this->scale.y = TILE_SIZE;
}

void GridRender::rebuildBuffers()
{
	std::vector<ObsRow> rows;
	collectRows(rows, obsType);
	rebuildBuffers(rows);
}

void GridRender::rebuildBuffers(const std::vector<ObsRow>& rows)
{
	markedForRebuild = false;

	const size_t N = rows.size();
	primsToDraw = N * 6;
	if(!N)
		return;

	// 2 tris = 6 verts  per ObsRow, each vertex is 2x uint16, makes 24b per quad.
	// We could use indexed rendering and use 2 verts less (16b),
	// but the 6 indices would cost another 12b so it's definitely cheaper to send
	// triangle soup to the gpu in this case, since each vertex is only 4 bytes.
	const size_t szv = N * 6 * 2 * sizeof(unsigned short);

	do
	{
		unsigned short *pxy =  (unsigned short*)vbo.beginWrite(GPUBUFTYPE_UVEC2, szv, GPUACCESS_DEFAULT);

		for(size_t i = 0; i < N; ++i)
		{
			const ObsRow& row = rows[i];

			// Don't bother to transform to float. The GPU can do that better.
			// The scale factor of a GridRender is set to TILE_SIZE, that pre-bakes the
			// required scaling multiplication into the object scale so we can get away
			// with using raw, unscaled values here

			const unsigned short x0 = row.tx;
			const unsigned short x1 = row.tx + row.len;
			const unsigned short y0 = row.ty;
			const unsigned short y1 = row.ty + 1;

			// top left triangle
			*pxy++ = x0;
			*pxy++ = y0;

			*pxy++ = x1;
			*pxy++ = y0;

			*pxy++ = x0;
			*pxy++ = y1;

			// bottom right triangle
			*pxy++ = x1;
			*pxy++ = y0;

			*pxy++ = x1;
			*pxy++ = y1;

			*pxy++ = x0;
			*pxy++ = y1;
		}
	}
	while(!vbo.commitWrite());
}

void GridRender::rebuildBuffersIfNecessary()
{
	if(markedForRebuild)
		rebuildBuffers();
}

void GridRender::rebuildBuffersIfNecessary(const std::vector<ObsRow>& rows)
{
	if(markedForRebuild)
		rebuildBuffers(rows);
}

void GridRender::onRender(const RenderState& rs) const
{
	if(!primsToDraw)
		return;

	/*
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
	*/

	// TODO: keep track of prim index at which a row starts
	// then horizontally span only as much prims as are necessary?

	vbo.apply();
	glDrawArrays(GL_TRIANGLES, 0, primsToDraw);
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

