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
#include "AutoMap.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"

#define AUTOMAP_GRIDTILE		512



AutoMap::AutoMap() : RenderObject(), ActionMapper()
{

	followCamera = 1;
	scale = Vector(0.4, 0.4);
	position = Vector(400,300);
	alpha = 0;
	vis = false;

	blip = Vector(1,1);
	blip.interpolateTo(Vector(4,4), 0.75, -1, 1, 1);

	initedGrid = false;


	setTexture("particles/glow");

	paintColor = Vector(1,1,1);
	autoMapMode = 0;

	addAction(MakeFunctionEvent(AutoMap, lmb), ActionMapper::MOUSE_BUTTON_LEFT);

	shadowTex = 0;
}



void AutoMap::destroy()
{
	RenderObject::destroy();
	if (shadowTex)
	{
		shadowTex->destroy();
		delete shadowTex;
		shadowTex = 0;
	}
}

bool AutoMap::isOn()
{
	return vis;
}

void AutoMap::toggle(bool on)
{
	const float t = 0.2;
	if (on)
	{
		autoMapMode = 0;
		dsq->game->togglePause(true);
		dsq->fade(1, t);
		dsq->main(t);
		core->overrideStartLayer = LR_AFTER_EFFECTS;
		core->overrideEndLayer = LR_MAX;
		vis = true;



		alpha = 1;

		dsq->fade(0, t);
		dsq->main(t);
	}
	else
	{
		dsq->fade(1, t);
		dsq->main(t);

		core->overrideStartLayer = 0;
		core->overrideEndLayer = 0;
		vis = false;
		alpha.interpolateTo(0, 0.5);
		alpha = 0;
		dsq->game->togglePause(false);

		dsq->fade(0, t);
		dsq->main(t);
	}
}

void AutoMap::setGridFromWorld(Vector worldPos, int gridValue)
{
	int gx = worldPos.x / AUTOMAP_GRIDTILE;
	int gy = worldPos.y / AUTOMAP_GRIDTILE;

	if (gx<0 || gy<0 || gx >= MAX_AUTOMAP_GRID || gy >= MAX_AUTOMAP_GRID)
	{
		std::ostringstream os;
		os << "AutoMap::setGridFromWorld - exceeded grid size";
		os << "(" << gx <<", " << gy << ")";
		debugLog(os.str());
		return;
	}

	grid[gx][gy] = gridValue;
}

void AutoMap::initGrid()
{


	for (int x=0;x<MAX_AUTOMAP_GRID;x++)
	{
		for (int y=0;y<MAX_AUTOMAP_GRID;y++)
		{
			grid[x][y] = 1;
		}
	}

	initedGrid = true;
}

void AutoMap::paint(const Vector &pos)
{
	Quad *q = new Quad("particles/WhiteGlow", pos);
	q->setLife(1);
	q->setDecayRate(0.5);
	q->color = paintColor;
	q->followCamera = 1;
	q->setWidthHeight(8,8);
	dsq->game->addRenderObject(q, this->layer);
}

void AutoMap::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (!initedGrid)
	{
		initGrid();
	}


	if (dsq->game->avatar)
	{
		setGridFromWorld(dsq->game->avatar->position, 0);
	}


	if (vis && alpha.x == 1)
	{
		const float maxScale=1.25, minScale=0.5;
		blip.update(dt);



		Vector sTarget=scale;
		float spd = 0.5;
		if (core->mouse.scrollWheelChange < 0)
		{
			scale -= Vector(spd*0.05f,spd*0.05f);
		}
		else if (core->mouse.scrollWheelChange > 0)
		{
			scale += Vector(spd*0.05f,spd*0.05f);
		}
		if (scale.x < minScale)
			scale = Vector(minScale, minScale);
		if (scale.x > maxScale)
			scale = Vector(maxScale, maxScale);
		if (core->mouse.buttons.middle)
		{
			offset += core->mouse.change*scale;
		}

	}
}

void AutoMap::lmb()
{
	if (autoMapMode == 0)
		autoMapMode = 1;
	else
		autoMapMode = 0;
}

void AutoMap::onRender()
{

	if (alpha.x == 0) return;

	glBindTexture(GL_TEXTURE_2D, 0);
	RenderObject::lastTextureApplied = 0;
	float alphaValue = alpha.x;



	int ysz = dsq->game->cameraMax.y/TILE_SIZE;
	int xsz = dsq->game->cameraMax.x/TILE_SIZE;


	TileVector t(Vector(dsq->game->cameraMax.x/2, dsq->game->cameraMax.y/2));



	int skip = 4;
	glLineWidth(skip);

	if (alphaValue > 0)
	{



		for (int y = 0; y < ysz; y += skip)
		{
			float f = float(y)/float(ysz);
			f = 0.8f-(f*0.5f);
			glColor4f(0.5f*f, 0.75f*f, 1*f, alphaValue);

			glBegin(GL_LINES);
			int rowStart = -1;
			int x = 0;

			for (x = 0; x < xsz; x++)
			{
				if (dsq->game->getGrid(TileVector(x,y))!=OT_BLACK)
				{
					if (rowStart == -1)
					{
						rowStart = x;
					}
				}
				else
				{
					if (rowStart != -1)
					{
						glVertex3f((rowStart-t.x), (y-t.y),0);
						glVertex3f((x-t.x), (y-t.y),0);
						rowStart = -1;
					}
				}
			}
			if (rowStart != -1)
			{
				glVertex3f((rowStart-t.x), (y-t.y),0);
				glVertex3f((x-t.x), (y-t.y),0);
			}
			glEnd();
		}



	}

	TileVector nt(dsq->game->avatar->position);

	glTranslatef(nt.x - t.x, nt.y - t.y,0);
	glColor4f(0.5,0.7,1, alphaValue);
	glPointSize(4);

	glBegin(GL_POINTS);
		glVertex2f(0,0);
	glEnd();

	glColor4f(0.5,0.75,1, alphaValue*0.5f);
	drawCircle(blip.x*16, 8);


}

