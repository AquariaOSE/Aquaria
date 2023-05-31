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
#include "Game.h"
#include "Avatar.h"
#include "ManaBall.h"

const float MULT_MANABALL_EASY = 1.5;

ManaBall::ManaBall(Vector pos, float amount) : Quad()
{
	scale = Vector(0.25f*amount, 0.25f*amount);
	this->position = pos;
	this->amount = amount;
	setTexture("manaballRing");
	Quad *pop = new Quad;
	pop->scale = Vector(0,0);
	pop->scale.interpolateTo(Vector(2, 2), 1, -1, 1);
	pop->setBlendType(BLEND_ADD);
	pop->setTexture("manaballLines");
	pop->rotation.interpolateTo(Vector(0,0,360), 2, -1);
	addChild(pop, PM_POINTER);

	lifeSpan = 15;
	setBlendType(BLEND_ADD);
	used = false;
	addChild(&healEmitter, PM_STATIC);
}

void ManaBall::destroy()
{
	Quad::destroy();
}

bool ManaBall::isUsed()
{
	return used;
}

void ManaBall::use(Entity *entity)
{
	core->sound->playSfx("CollectMana");
	entity->heal(amount, 1);
	scale.ensureData();
	scale.data->path.addPathNode(scale, 0);
	scale.data->path.addPathNode(Vector(1.25f, 1.25f), 0.5f);
	scale.data->path.addPathNode(Vector(0,0), 1);
	scale.startPath(1);
	setLife(1.1f);
	used = true;
}

void ManaBall::onUpdate(float dt)
{
	if (game->isPaused()) return;

	Quad::onUpdate(dt);

	if (lifeSpan > 0)
	{
		lifeSpan -= dt;
		if (lifeSpan <= 0)
		{
			lifeSpan = 0;
			this->scale.interpolateTo(Vector(0,0),1);
			setLife(1);
			setDecayRate(1);


		}
	}

	if (game->avatar)
	{
		if (!used)
		{
			Vector diff = (game->avatar->position - position);
			if (diff.isLength2DIn(96))
			{
				use(game->avatar);

			}
			else
			{
				float len = 1000;
				if (game->avatar->isRolling() && diff.isLength2DIn(len))
				{
					float maxSpeed = 800;
					Vector maxV = diff;
					maxV.setLength2D(len);
					diff = maxV - diff;
					diff *= maxSpeed/len;
					velocity = diff;
				}
				else
				{
					velocity = Vector(0,0);
				}
			}
		}
		else
		{
			position.interpolateTo(game->avatar->position, 0.2f);

		}
	}
	position.z = 0.5f;
}

