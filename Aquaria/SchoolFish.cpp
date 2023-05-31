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
#include "SchoolFish.h"
#include "Game.h"
#include "Avatar.h"
#include "../BBGE/AfterEffect.h"

const float strengthSeparation = 1;
const float strengthAlignment = 0.8f;
//const float strengthAvoidance = 1;
const float strengthCohesion = 0.5f;

//const float avoidanceDistance = 128;
const float separationDistance = 128;
const float minUrgency = 5;//0.05;
const float maxUrgency = 10;//0.1;
const float maxSpeed = 400; // 1
const float maxChange = maxSpeed*maxUrgency;

const float velocityScale = 1;


SchoolFish::SchoolFish(const std::string &texname) : FlockEntity()
{
	burstDelay = 0;

	naijaReaction = "smile";
	rippleTimer = 0;
	oldFlockID = -1;
	respawnTimer = 0;
	dodgeAbility = (rand()%900)/1000.0f + 0.1f;
	float randScale = float(rand()%200)/1000.0f;
	scale = Vector(0.6f-randScale, 0.6f-randScale);



	color.ensureData();
	color.data->path.addPathNode(Vector(1,1,1), 0);
	color.data->path.addPathNode(Vector(1,1,1), 0.5f);
	color.data->path.addPathNode(Vector(0.8f, 0.8f, 0.8f), 0.7f);
	color.data->path.addPathNode(Vector(1,1,1), 1.0f);
	color.startPath(2);
	color.data->loopType = -1;
	color.update((rand()%1000)/1000.0f);

	flipDelay = 0;
	swimSound = "SchoolFishSwim";
	soundDelay = (rand()%300)/100.0f;
	range = 0;
	setEntityType(ET_ENEMY);
	canBeTargetedByAvatar = true;
	health = maxHealth = 1;

	avoidTime=0;
	vel = Vector(-minUrgency, 0);
	setTexture(texname);
	flockType = FLOCK_FISH;

	updateCull = 4000;
	collideRadius = 20;


	setSegs(8, 2, 0.1f, 0.9f, 0, -0.03f, 8, 0);



	setDamageTarget(DT_AVATAR_LIZAP, false);



	targetPriority = -1;

	setEatType(EAT_FILE, "SchoolFish");

	this->deathSound = "";
	this->targetPriority = -1;

	setDamageTarget(DT_AVATAR_PET, false);
}

void SchoolFish::onEnterState(int action)
{
	Entity::onEnterState(action);
	if (action == STATE_DEAD)
	{

		vel.setLength2D(vel.getLength2D()*-1);

		oldFlockID = flock ? flock->flockID : -1;
		removeFromFlock();

		doDeathEffects(0, false);


		respawnTimer = 20 + rand()%20;
		alphaMod = 0;



		if (!isGoingToBeEaten())
		{
			if (game->firstSchoolFish)
			{
				if (chance(50))
					game->spawnIngredient("FishOil", position, 1);
				else
					game->spawnIngredient("FishMeat", position, 1);

				game->firstSchoolFish = false;
			}
			else
			{
				if (chance(8))
					game->spawnIngredient("FishOil", position, 1);
				if (chance(5))
					game->spawnIngredient("FishMeat", position, 1);
			}
		}
	}
	else if (action == STATE_IDLE)
	{
		revive(3000);
		alpha.interpolateTo(1, 1);
		alphaMod = 1;
		if (oldFlockID != -1)
			addToFlock(oldFlockID);
	}
}

void SchoolFish::updateVelocity(Vector &accumulator)
{
	// Ok, now limit speeds
	accumulator.capLength2D(maxChange);
	// Save old speed
	lastSpeed = vel.getLength2D();

	// Calculate new velocity and constrain it
	vel += accumulator;

	vel.capLength2D(getMaxSpeed() * maxSpeedLerp.x);
	vel.z = 0;
	if (fabsf(vel.y) > fabsf(vel.x))
	{



	}
}

inline
void SchoolFish::avoid(Vector &accumulator, Vector pos, bool inv)
{

	Vector change;
	if (inv)
		change = pos - this->position;
	else
		change = this->position - pos;


	change.setLength2D(maxUrgency);



	accumulator += change;
}

void SchoolFish::applyLayer(int layer)
{
	switch (layer)
	{
	case -3:
		setv(EV_LOOKAT, 0);
	break;
	}
}

inline
void SchoolFish::applyAvoidance(Vector &accumulator)
{
	// only avoid the player if not in the background
	if (this->layer < LR_ELEMENTS10)
	{
		if ((game->avatar->position - this->position).isLength2DIn(128))
		{
			avoid(accumulator, game->avatar->position);
		}
	}



	if (avoidTime>0) return;

	const int range = 10;
	const int step = 1;
	int radius = range*TILE_SIZE;
	int obsSumX = 0, obsSumY = 0;  // Not a Vector (avoid using floats)
	int obsCount = 0;
	const TileVector t0(position);
	TileVector t;
	for (t.x = t0.x-range; t.x <= t0.x+range; t.x += step)
	{
		for (t.y = t0.y-range; t.y <= t0.y+range; t.y += step)
		{
			if (game->isObstructed(t))
			{
				obsSumX += t0.x - t.x;
				obsSumY += t0.y - t.y;
				obsCount++;
			}
		}
	}

	if (obsCount > 0)
	{
		const float tileMult = (float)TILE_SIZE / (float)obsCount;
		Vector change(obsSumX*tileMult, obsSumY*tileMult);
		change += position - t0.worldVector();


		float dist = change.getLength2D();
		float ratio = dist / radius;
		if (ratio < minUrgency) ratio = minUrgency;
		else if (ratio > maxUrgency) ratio = maxUrgency;
		change *= (ratio + lastSpeed*0.1f) / dist;

		accumulator += change;
	}

	if (this->range!=0)
	{
		if (!((position - startPos).isLength2DIn(this->range)))
		{
			Vector diff = startPos - position;
			diff.setLength2D(lastSpeed);
			accumulator += diff;
		}
	}
}

inline
void SchoolFish::applyAlignment(Vector &accumulator, const Vector &flockHeading)
{
	if (strengthAlignment <= 0) return;
	Vector change;
	// Flocking Rule: Alignment
	// Try to align boid's heading with its flockmates
	// Copy the heading of the flock
	change = getFlockHeading();
	// normalize change to look more natural
	//change |= (minUrgency * strengthAlignment );
	change.setLength2D(minUrgency*strengthAlignment);
	// Add Change to Accumulator
	accumulator += change;
}

inline
void SchoolFish::applyCohesion(Vector &accumulator)
{
	if (strengthCohesion<=0) return;
	Vector change;
	// Flocking Rule: Cohesion
	// Try to go toward where all the flockmates are (the flock's center point)
	// Copy the position of center of flock
	change = getFlockCenter();
	// Average with our position
	change -= position;
	// normalize change to look more natural
	if (!change.isZero())
		change.setLength2D(minUrgency*strengthCohesion);
	accumulator += change;

	if (dsq->continuity.form == FORM_FISH)
	{
		if ((game->avatar->position - this->position).isLength2DIn(256))
		{
			change = game->avatar->position - position;
			change.setLength2D(maxUrgency*strengthCohesion);
			accumulator += change;

		}
	}
}

inline
void SchoolFish::applySeparation(Vector &accumulator)
{
	FlockEntity *e = getNearestFlockEntity();
	if (e)
	{
		const float dist = getNearestFlockEntityDist();
		if (dist < separationDistance)
		{
			float ratio = dist / separationDistance;
			if (ratio < minUrgency) ratio = minUrgency;
			else if (ratio > maxUrgency) ratio = maxUrgency;
			ratio *= strengthSeparation;
			Vector change(e->position - this->position);
			if (!change.isZero())
			{
				change.setLength2D(-ratio);
					accumulator += change;
			}
		}
			// Are we too far from nearest flockmate?  Then Move Closer
		/*
			else if (dist > separationDistance)
			change |= ratio;
		*/
	}
}

void SchoolFish::onUpdate(float dt)
{
	burstDelay -= dt;
	if (burstDelay < 0)
		burstDelay = 0;

	if (stickToNaijasHead && alpha.x < 0.1f)
		stickToNaijasHead = false;

	if (this->layer < LR_ENTITIES)
	{
		setEntityType(ET_NEUTRAL);
		collideRadius = 0;
	}

	if (getState() == STATE_DEAD)
	{
		FlockEntity::onUpdate(dt);
		respawnTimer -= dt;
		if (!(game->avatar->position - this->position).isLength2DIn(2000))
		{
			if (respawnTimer < 0)
			{
				respawnTimer = 0;
				perform(STATE_IDLE);
			}
		}
	}
	else
	{


		FlockEntity::onUpdate(dt);

		if (game->isValidTarget(this, 0))
			game->handleShotCollisions(this);



		if (true)
		{
			VectorSet newDirection;

			if (avoidTime>0)
			{
				avoidTime -= dt;
				if (avoidTime<0)
					avoidTime=0;
			}

			Vector dir = getFlockHeading();

			Vector accumulator;


			applyCohesion(accumulator);
			applyAlignment(accumulator, dir);
			// alignment
			applySeparation(accumulator);
			applyAvoidance(accumulator);
			updateVelocity(accumulator);



			Vector lastPosition = position;
			position += (vel*velocityScale + vel2) * dt;

			if (game->isObstructed(position))
			{
				position = lastPosition;

			}


			updateVel2(dt);



			flipDelay = 0;


			if (flipDelay <= 0)
			{
				const float amt = 0;

				if (vel.x > amt && !isfh())
				{
					flipHorizontal();
				}
				if (vel.x < -amt && isfh())
				{
					flipHorizontal();
				}
			}



			float angle = atan2f(dir.x<0 ? -dir.y : dir.y, fabsf(dir.x));
			angle = ((angle*180)/PI);

			if (angle > 45)
				angle = 45;
			if (angle < -45)
				angle = -45;


			rotation = Vector(0,0,angle);


		}

	}
}
