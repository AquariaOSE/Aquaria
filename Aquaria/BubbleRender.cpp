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

BubbleRender::BubbleRender() : RenderObject()
{
	bubble.setTexture("particles/bubble");
	bubble.alpha = 0.5;
	bubble.scale = Vector(0.5,0.5);
	cull = false;
}

void BubbleRender::rebuild()
{
	bubbles.clear();
	for (int x = dsq->game->cameraMin.x; x < dsq->game->cameraMax.x; x+=64)
	{
		for (int y = dsq->game->cameraMin.y; y < dsq->game->cameraMax.y; y+=64)
		{
			bubbles.push_back(Vector(x,y) + Vector(rand()%16-32, rand()%16-32));
		}
	}
}

void BubbleRender::onRender()
{
	for (int i = 0; i < bubbles.size(); i++)
	{
		if (bubbles[i].x > core->screenCullX1 && bubbles[i].x < core->screenCullX2)
		{
			if (bubbles[i].y > core->screenCullY1 && bubbles[i].y < core->screenCullY2)
			{
				bubble.position = bubbles[i];
				bubble.render();
			}
		}
	}
}
