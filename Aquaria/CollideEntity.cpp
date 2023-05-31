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
#include "CollideEntity.h"
#include "DSQ.h"
#include "Game.h"


CollideEntity::CollideEntity() : Entity()
{
	addType(SCO_COLLIDE_ENTITY);
	this->canBeTargetedByAvatar = true;
	weight = 0;
	bounceAmount = 0.5f;
	this->updateCull = 4000;
}

CollideEntity::~CollideEntity()
{
}

void CollideEntity::entityDied(Entity *e)
{
	Entity::entityDied(e);
}

void CollideEntity::onUpdateFrozen(float dt)
{
	updateMovement(dt);
}

void CollideEntity::bounce(float ba)
{
	if (getState() == STATE_PUSH)
	{
		dsq->spawnParticleEffect("HitSurface", game->lastCollidePosition);

		sound("RockHit");
		// HACK: replace damage function
		//damage(pushDamage);
		setState(STATE_PUSHDELAY, 0.3f);
	}

	switch (bounceType)
	{
	case BOUNCE_REAL:
	{
		if (!vel.isZero())
		{
			float len = vel.getLength2D();
			Vector I = vel/len;
			Vector N = game->getWallNormal(game->lastCollidePosition);

			if (!N.isZero())
			{

				vel = 2*(-I.dot(N))*N + I;
				vel.setLength2D(len*ba);
			}
		}
	}
	break;
	case BOUNCE_SIMPLE:
	{
		if (!vel.isZero())
		{
			float len = vel.getLength2D();
			Vector mov = vel;
			mov.setLength2D(len*ba);
			if (mov.x > mov.y)
				mov.x = -mov.x;
			else
				mov.y = -mov.y;
			vel = mov;
			vel.z = 0;
		}
	}
	break;
	case BOUNCE_NONE:
	break;
	}



	onBounce();
}

void CollideEntity::updateMovement(float dt)
{
	if (isEntityDead()) return;
	if (position.isFollowingPath()) return;
	vel.capLength2D(getMaxSpeed()*maxSpeedLerp.x);



	updateVel2(dt);

	Vector lastPosition = position;

	bool underWater = isUnderWater();
	if (!canLeaveWater)
	{
		if (!underWater && wasUnderWater)
		{
			// do clamp
			if (waterBubble)
			{
				waterBubble->clampPosition(&position, collideRadius);
			}
			else
			{
				position.y = game->getWaterLevel()+collideRadius;
			}

		}
	}


	bool collided = false;

	if (vel.x != 0 || vel.y != 0)
	{

		const float hw = collideRadius;
		bool freeRange = false;

		if (isv(EV_COLLIDELEVEL,1))
		{



			bool doesFreeRange = !isPullable();
			if (doesFreeRange)
			{
				if (game->collideCircleWithGrid(position, hw))
				{
					// starting in a collision state
					freeRange = true;
				}
			}
		}


		Vector newPosition = position + (getMoveVel() * dt);
		position = newPosition;

		if (isv(EV_COLLIDELEVEL,1))
		{
			if (getState() == STATE_PUSH)
			{
				if (!freeRange && game->collideCircleWithGrid(position, hw))
				{
					position = lastPosition;
					collided = true;
					bounce(bounceAmount);
				}
			}
			else
			{
				if (!freeRange && ((!canLeaveWater && !isUnderWater() && wasUnderWater) || game->collideCircleWithGrid(position, hw)))
				{
					position = lastPosition;
					onHitWall();
					bounce(bounceAmount);
					collided = true;
				}
			}
		}
	}

	if (!collided && weight != 0)
	{
		vel += Vector(0, weight*dt);
	}

	wasUnderWater = underWater;
}
