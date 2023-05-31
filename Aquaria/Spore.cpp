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

#include "Spore.h"
#include "Shot.h"
#include "Game.h"
#include "Avatar.h"


Spore::Spores Spore::spores;

Spore::Spore (const Vector &position) : CollideEntity()
{
	spores.push_back(this);
	scale = Vector(0.1f, 0.1f);
	alpha = 0;

	this->position = position;
	alpha.interpolateTo(1, 0.5f);
	scale.interpolateTo(Vector(1, 1), 4);

	setTexture("Spore");

	setAllDamageTargets(false);
	setEntityType(ET_ENEMY);
	setDamageTarget(DT_AVATAR_ENERGYBLAST, true);
	setDamageTarget(DT_AVATAR_SHOCK, true);
	setDamageTarget(DT_AVATAR_ENERGYROLL, true);

	health = maxHealth = 1;
}

bool Spore::isPositionClear(const Vector &position)
{
	if (game->isObstructed(TileVector(position)))
		return false;
	for (Spores::iterator i = spores.begin(); i != spores.end(); i++)
	{
		Spore *s = *i;
		if (s->position == position)
		{
			return false;
		}
	}
	return true;
}

void Spore::destroy()
{
	spores.remove(this);
	CollideEntity::destroy();
}

void Spore::onEndOfLife()
{

	spores.remove(this);
}

void Spore::onEnterState(int state)
{
	CollideEntity::onEnterState(state);
	if (state == STATE_DEAD)
	{
		setLife(1);
		setDecayRate(4);
		fadeAlphaWithLife = true;
	}
}

void Spore::killAllSpores()
{
	Spores sporesToDelete = spores; // copy

	for (Spores::iterator it = sporesToDelete.begin(); it != sporesToDelete.end(); it++)
		if(Spore *s = *it)
			s->safeKill();

	spores.clear();
}

void Spore::onUpdate(float dt)
{

	CollideEntity::onUpdate(dt);

	if (life < 1) return;
	if (!(game->avatar->position - position).isLength2DIn(1024))
	{
		safeKill();
	}
	else
	{
		int sporeCr = 48;

		collideRadius = scale.x * sporeCr;

		if (touchAvatarDamage(collideRadius, 1, Vector(-1,-1,-1), 500))
		{
			// YAY!
		}

		game->handleShotCollisions(this);
	}
}
