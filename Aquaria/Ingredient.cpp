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

#include "Ingredient.h"
#include "Game.h"
#include "Avatar.h"

IngredientData::IngredientData(const std::string &name, const std::string &gfx, IngredientType type)
	: name(name)
	, gfx(gfx)
	, displayName(dsq->continuity.getIngredientDisplayName(name))
	, type(type)
	, amount(0)
	, maxAmount(MAX_INGREDIENT_AMOUNT)
	, held(0)
	, marked(0)
	, rotKind(!(type == IT_OIL || type == IT_EGG))
{
}

int IngredientData::getIndex() const
{
	return dsq->continuity.indexOfIngredientData(this);
}

bool IngredientData::hasIET(IngredientEffectType iet)
{
	for (IngredientEffects::iterator i = effects.begin(); i != effects.end(); i++)
	{
		if ((*i).type == iet)
			return true;
	}
	return false;
}

Ingredient::Ingredient(const Vector &pos, IngredientData *data, int amount)
	: Entity()
	, data(data)
	, used(false)
	, gone(false)
	, amount(amount)
{
	addType(SCO_INGREDIENT);
	entityType = ET_INGREDIENT;
	position = pos;
	lifeSpan = 30;
	if (data)
	{
		setTexture("Ingredients/"+data->gfx);
	}
	int mag = 600;
	if (isRotKind())
		velocity = randVector(mag)*0.5f + Vector(0, -mag)*0.5f;
	else
		velocity = Vector(0,-mag*0.5f);
	gravity = Vector(0, 250);
	scale = Vector(0.2f,0.2f);
	scale.interpolateTo(Vector(1, 1), 0.75f);

	if (isRotKind())
		rotation.z = randAngle360();

	layer = LR_ENTITIES;
}

bool Ingredient::hasIET(IngredientEffectType iet)
{
	if (data)
		return data->hasIET(iet);
	return false;
}

void Ingredient::destroy()
{
	Entity::destroy();
	game->removeIngredient(this);
}

bool Ingredient::isRotKind()
{
	return data && data->rotKind;
}

IngredientData *Ingredient::getIngredientData()
{
	return data;
}

void Ingredient::eat(Entity *e)
{
	safeKill();

	dsq->spawnParticleEffect("IngredientCollect", position);
}

void Ingredient::onUpdate(float dt)
{
	if (game->isPaused()) return;
	if (game->isWorldPaused()) return;

	Vector lastPosition = position;
	Entity::onUpdate(dt);

	if (game->collideCircleWithGrid(position, 24))
	{
		position = lastPosition;

		velocity = 0;
	}

	if (lifeSpan > 0)
	{
		lifeSpan -= dt;
		if (lifeSpan <= 0)
		{
			lifeSpan = 0;
			gone = true;
			this->scale.interpolateTo(Vector(0,0),1);
			setLife(1);
			setDecayRate(1);
		}
	}

	Vector diff = (game->avatar->position - position);
	if (diff.isLength2DIn(64))
	{
		if (scale.x == 1)
		{
			// got
			safeKill();

			dsq->continuity.pickupIngredient(data, 1);

			game->pickupIngredientEffects(data);

			dsq->spawnParticleEffect("IngredientCollect", position);

			dsq->sound->playSfx("pickup-ingredient");
		}
	}
	else
	{
		float len = 1024;
		if (game->avatar->isRolling() && diff.isLength2DIn(len))
		{
			float maxSpeed = 1500;
			Vector maxV = diff;
			maxV.setLength2D(len);
			diff = maxV - diff;
			diff *= maxSpeed/len;
			velocity += diff * 1.5f * dt;
		}
	}

	velocity.capLength2D(1000);

	if (isRotKind() && !velocity.isZero())
	{
		int mag = velocity.getLength2D();
		if (velocity.x > 0)
			rotation.z += mag*0.01f;
		else
			rotation.z -= mag*0.01f;
	}


	Vector sub = velocity;
	if (!sub.isZero())
	{
		sub.setLength2D(100*dt);
		velocity -= sub;
	}
	// collision?
}
