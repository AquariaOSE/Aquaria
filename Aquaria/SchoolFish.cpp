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

float strengthSeparation = 1;
float strengthAlignment = 0.8;
float strengthAvoidance = 1;
float strengthCohesion = 0.5;

float avoidanceDistance = 128;
float separationDistance = 128;
float minUrgency = 5;//0.05;
float maxUrgency = 10;//0.1;
float maxSpeed = 400; // 1
float maxChange = maxSpeed*maxUrgency;

float velocityScale = 1;


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

	/*
	float randColor = float(rand()%250)/1000.0f;
	color = Vector(1-randColor, 1-randColor, 1-randColor);
	*/

	//color.interpolateTo(Vector(0.5, 0.5, 0.5), 2, -1, 1);
	color.ensureData();
	color.data->path.addPathNode(Vector(1,1,1), 0);
	color.data->path.addPathNode(Vector(1,1,1), 0.5);
	color.data->path.addPathNode(Vector(0.8, 0.8, 0.8), 0.7);
	color.data->path.addPathNode(Vector(1,1,1), 1.0);
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
	//scale = Vector(0.5, 0.5);
	avoidTime=0;
	vel = Vector(-minUrgency, 0);
	setTexture(texname);
	flockType = FLOCK_FISH;
	//updateCull = -1;
	updateCull = 4000;
	collideRadius = 20;

	//2 32 0.1 0.1 -0.03 0 4 0
	setSegs(8, 2, 0.1, 0.9, 0, -0.03, 8, 0);

	/*
	setDamageTarget(DT_AVATAR_SPORECHILD, false);
	setDamageTarget(DT_AVATAR_ENERGYBLAST, false);
	*/
	//setAllDamageTargets(true);
	setDamageTarget(DT_AVATAR_LIZAP, false);
	/*
	setDamageTarget(DT_AVATAR_ENERGYBLAST, false);
	setDamageTarget(DT_AVATAR_SHOCK, false);
	*/
	//setDamageTarget(DT_AVATAR_BITE, true);
//	updateCull = 10248;
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
		//rotation.interpolateTo(Vector(0,0,180), 2);
		vel.setLength2D(vel.getLength2D()*-1);

		oldFlockID = flock ? flock->flockID : -1;
		removeFromFlock();

		doDeathEffects(0,0,0);

		/*
		alpha = 0;
		alphaMod = 0;
		*/
		respawnTimer = 20 + rand()%20;
		alphaMod = 0;
		/*
		this->setLife(2);
		this->setDecayRate(1);
		this->fadeAlphaWithLife = true;
		*/

		/*
		dsq->game->spawnIngredient("FishOil", position, 10);
		dsq->game->spawnIngredient("FishMeat", position, 10);
		dsq->game->spawnIngredient("SmallEgg", position, 10);
		dsq->game->spawnIngredient("PlantLeaf", position, 60);
		*/

		if (!isGoingToBeEaten())
		{
			if (dsq->game->firstSchoolFish)
			{
				if (chance(50))
					dsq->game->spawnIngredient("FishOil", position, 1);
				else
					dsq->game->spawnIngredient("FishMeat", position, 1);

				dsq->game->firstSchoolFish = false;
			}
			else
			{
				if (chance(8))
					dsq->game->spawnIngredient("FishOil", position, 1);
				if (chance(5))
					dsq->game->spawnIngredient("FishMeat", position, 1);
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
		/*
		float sign = vel.y / fabsf(vel.y);
		vel.y = fabsf(vel.x) * sign;
		*/
		//std::swap(vel.x, vel.y);
		// going up 
		/*
		float len = vel.getLength2D();
		if (vel.y < 0)
		{
			if (vel.x < 0)
				vel.y = vel.x;
			else
				vel.y = -vel.x;
		}
		else
		{
			if (vel.x < 0)
				vel.y = -vel.x;
			else
				vel.y = vel.x;
		}
		vel.setLength2D(len);
		*/
	}
}

inline
void SchoolFish::avoid(Vector &accumulator, Vector pos, bool inv)
{
	//accumulator = Vector(0,0,0);
	Vector change;
	if (inv)
		change = pos - this->position;
	else
		change = this->position - pos;
	//change = position;
	//change -= this->position;
	change.setLength2D(maxUrgency);
    //change |= maxUrgency;

	//change = Vector(100,0);
	//avoidTime = 2;
	/*
	std::ostringstream os;
	os << "change(" << change.x << ", " << change.y << ")";
	debugLog (os.str());
	*/
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
		if ((dsq->game->avatar->position - this->position).isLength2DIn(128))
		{
			avoid(accumulator, dsq->game->avatar->position);
		}
	}


	//return;

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
			if (dsq->game->isObstructed(t))
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
		//change |= 200;

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
		if ((dsq->game->avatar->position - this->position).isLength2DIn(256))
		{
			change = dsq->game->avatar->position - position;
			change.setLength2D(maxUrgency*strengthCohesion);
			accumulator += change;
			//avoid(accumulator, dsq->game->avatar->position, true);
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
	BBGE_PROF(SchoolFish_onUpdate);
	/*
	Quad::onUpdate(dt);
	return;
	*/

	/*
	if (dsq->continuity.form == FORM_BEAST)
		this->activationType = ACT_CLICK;
	else
		this->activationType = ACT_NONE;
	*/


	/*
	if (burstDelay == 0)
	{
		maxSpeedLerp = 2;
		Vector v = getNormal();
		vel = 0;
		v *= -5000;
		vel += v;
		//float t = (100 + rand()%100)/100.0f;
		float t = 2;
		maxSpeedLerp.interpolateTo(1, t);
		burstDelay = 10;// + (rand()%100)/100.0f;
		//rotateToVec(v, 0, 90);
		//rotation.interpolateTo(0, 1);

		if (v.x > 0 && !isfh())
		{
			flipHorizontal();
			flipDelay = 0.5;
		}
		if (v.x < 0 && isfh())
		{
			flipHorizontal();
			flipDelay = 0.5;
		}
	}
	else
	*/

	{
		burstDelay -= dt;
		if (burstDelay < 0)
		{
			burstDelay = 0;
		}
	}

	if (stickToNaijasHead && alpha.x < 0.1f)
		stickToNaijasHead = false;

	if (this->layer < LR_ENTITIES)
	{
		//debugLog("background fish!");
		/*
		setDamageTarget(DT_AVATAR_SHOCK, false);
		setDamageTarget(DT_AVATAR_BITE, false);
		setDamageTarget(DT_AVATAR_VOMIT, false);
		setDamageTarget(DT_AVATAR_ENERGYBLAST, false);
		*/
		setEntityType(ET_NEUTRAL);
		collideRadius = 0;
	}

	if (getState() == STATE_DEAD)
	{
		FlockEntity::onUpdate(dt);
		respawnTimer -= dt;
		if (!(dsq->game->avatar->position - this->position).isLength2DIn(2000))
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
		/*
		if (layer == LR_ENTITIES || layer == LR_ENTITIES2)
		{
			rippleTimer -= dt;
			if (rippleTimer < 0)
			{
				if (core->afterEffectManager)
					core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),position,0.04,0.06,15,0.2f));
				rippleTimer = 0.5;
			}
		}
		*/

		FlockEntity::onUpdate(dt);

		if (dsq->game->isValidTarget(this, 0))
			dsq->game->handleShotCollisions(this);


		/*
		soundDelay -= dt;
		if (soundDelay <= 0)
		{
			//sound(swimSound, 1000 + rand()%100);
			soundDelay = 4+(rand()%50)/100.0f;
		}
		*/
		/*
	1.    if distance_to(closest_boid) <= too_close then set direction away from closest_boid
	2.    speed_of_neighbors := average(speed(x), for all x where distance_to(x) <= neighborhood_size)
			direction_of_neighbors := avg(direction(x), for all x where distance_to(x) <= neighborhood_size)
			if speed < speed_of_neighbors then increase speed
			if speed > speed_of_neighbors then decrease speed
			turn towards direction_of_neighbors
	3.    position_of_neighbors := avg(position(x), for all x where distance_to(x) <= neighborhood_size)
			turn towards position_of_neighbors
		*/


		/*
		FlockPiece flock;
		getFlockInRange(160, &flock);
		*/
		// if flock in 160 ?
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

			/*
			if (dsq->game->isValidTarget(this, 0))
				doSpellAvoidance(dt, 96, dodgeAbility);
			*/


			Vector lastPosition = position;
			position += (vel*velocityScale + vel2) * dt;

			if (dsq->game->isObstructed(position))
			{
				position = lastPosition;
				/*
				Vector newPosition = position;
				position = Vector(newPosition.x, lastPosition.y);
				if (dsq->game->isObstructed(position))
				{
					position = Vector(lastPosition.x, newPosition.y);
					if (dsq->game->isObstructed(position))
					{
						position = lastPosition;
					}
				}
				*/
			}

			//updateCurrents(dt);
			updateVel2(dt);


			/*
			if (flipDelay > 0)
			{
				flipDelay -= dt;
				if (flipDelay < 0)
				{
					flipDelay = 0;
				}
			}
			*/

			flipDelay = 0;

			//dir.normalize2D();
			if (flipDelay <= 0)
			{
				const float amt = 0;
				/*

				if (fabsf(dir.x) > fabsf(dir.y))
				{
					if (dir.x > amt && !isfh())
					{
						flipHorizontal();
						flipDelay = 0.5;
					}
					if (dir.x < -amt && isfh())
					{
						flipHorizontal();
						flipDelay = 0.5;
					}
				}
				*/
				if (vel.x > amt && !isfh())
				{
					flipHorizontal();
				}
				if (vel.x < -amt && isfh())
				{
					flipHorizontal();
				}
			}


			//rotateToVec(accumulator, 5, 90);

			float angle = atan2f(dir.x<0 ? -dir.y : dir.y, fabsf(dir.x));
			angle = ((angle*180)/PI);

			if (angle > 45)
				angle = 45;
			if (angle < -45)
				angle = -45;
			

			rotation = Vector(0,0,angle);

			//rotation.interpolateTo(Vector(0, 0, angle), 0);
		}

	}
}

void SchoolFish::onRender()
{
	FlockEntity::onRender();
	/*
	glDisable(GL_BLEND);
	glPointSize(12);
	glDisable(GL_LIGHTING);
	glColor3f(1,1,1);
	glBegin(GL_POINTS);

		glVertex3f(0,0,0);
	glEnd();
	glBegin(GL_LINES);
		glVertex3f(0,0,0);
		glVertex3f(vel.x*50, vel.y*50, 0);
	glEnd();
	*/
}

