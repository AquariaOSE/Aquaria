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
#include "WaterFont.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"

#include "../BBGE/Particles.h"

WaterFont::WaterFont() : BitmapText(&dsq->font)
{
	pTime = 0;
	setBitmapFontEffect(BFE_SHADOWBLUR);
	setFontSize(26);
}

//const float interval = 0.015;
void spawnWaterFontParticle(RenderObject *me, Vector position, int size, float life, int layer)
{
	float t = life;
	int angle = rand()%360;
	int spd = 10;
	PauseQuad *q = new PauseQuad;
	q->setTexture("particles/spark");

	q->setBlendType(RenderObject::BLEND_ADD);

	q->color.ensureData();
	q->color.data->path.addPathNode(Vector(1,1,1),0);
	q->color.data->path.addPathNode(Vector(0.1,0.75,1),0.3);
	q->color.data->path.addPathNode(Vector(0.05,0.5,1),0.8);
	q->color.data->path.addPathNode(Vector(0,0,1),1.0);
	q->color.startPath(t);

	//q->color = Vector(1, 0.5, 0);
	q->alpha.ensureData();
	q->alpha.data->path.addPathNode(0, 0);
	q->alpha.data->path.addPathNode(0.2, 0.5);
	q->alpha.data->path.addPathNode(0, 1.0);
	q->alpha.startPath(t);
	q->velocity = Vector(sinf(angle)*spd,cosf(angle)*spd);
	//q->velocity += vel;
	q->setLife(1.0);
	q->setDecayRate(1.0f/t);
	q->rotation.z = rand()%360;
	q->setWidthHeight(size, size);
	q->position = position;
	q->cull = false;
	//q->influenced = 16;

	dsq->game->addRenderObject(q, layer);
	//me->addChild(q);
	//me->renderBeforeParent = false;
	//me->parentManagedPointer = true;
	//q->update(interval);
}


//const float interval = 0.012;
const float interval = 0.08;
void WaterFont::onUpdate(float dt)
{
	BitmapText::onUpdate(dt);

	Vector sp(position.x-alignWidth*0.5f-15, 5+position.y+5);
	Vector ep(position.x+alignWidth*0.5f+15, 5+position.y+10);
	pTime += dt;
	while (pTime > interval)
	{
		int dist = alignWidth / 40;
		for (int i = 0; i < dist; i++)
		{
			Vector p(rand()%(int(ep.x-sp.x))+sp.x, rand()%(int(ep.y-sp.y))+sp.y);
			/*
			std::ostringstream os;
			os << "p(" << p.x << ", " << p.y << ")";
			debugLog(os.str());
			*/
			Vector d(rand()%200-100,rand()%200-100);
			d *= 0.01f;
			spawnWaterFontParticle(this, p, 64, 1.2, LR_PARTICLES);
		}
		pTime -= interval;
	}
}

