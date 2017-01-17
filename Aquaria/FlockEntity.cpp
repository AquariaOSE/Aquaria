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
#include "FlockEntity.h"

const int DEFAULT_MAX_FLOCKS = 20;  // Will be increased as needed
std::vector<Flock*> flocks(DEFAULT_MAX_FLOCKS);

FlockEntity::FlockEntity() : CollideEntity()
{
	flockType = FLOCK_FISH;
	flock = 0;
	nextInFlock = prevInFlock = 0;
	nearestFlockMate = 0;
	nearestDistance = HUGE_VALF;

	angle = 0;

	collideRadius = 8;
}

void FlockEntity::addToFlock(size_t id)
{
	if (id >= flocks.size())
	{
		int curSize = flocks.size();
		flocks.resize(id+1);
		for (size_t i = curSize; i < id+1; i++)
			flocks[i] = 0;
	}
	if (!flocks[id])
	{
		flocks[id] = new Flock(id);
	}

	flock = flocks[id];
	nextInFlock = flock->firstEntity;
	prevInFlock = 0;
	if (flock->firstEntity)
		flock->firstEntity->prevInFlock = this;
	flock->firstEntity = this;

	nearestFlockMate = 0;
	nearestDistance = HUGE_VALF;
	int numEntities = 1;
	for (FlockEntity *e = nextInFlock; e; e = e->nextInFlock)
	{
		numEntities++;
		const float distance = (e->position - position).getLength2D();
		if (distance < nearestDistance)
		{
			nearestFlockMate = e;
			nearestDistance = distance;
		}
	}
	if (numEntities == 1)
	{
		flock->center = position;
		flock->heading = vel;
	}
	else
	{
		flock->center = (flock->center*(numEntities-1) + position) / numEntities;
		flock->heading = (flock->heading*(numEntities-1) + vel) / numEntities;
	}
}

void FlockEntity::removeFromFlock()
{
	if (flock)
	{
		if (nextInFlock)
			nextInFlock->prevInFlock = prevInFlock;
		if (prevInFlock)
			prevInFlock->nextInFlock = nextInFlock;
		else
			flock->firstEntity = nextInFlock;
		if (!flock->firstEntity)
		{
			flocks[flock->flockID] = 0;
			delete flock;
		}
	}
	flock = 0;
	nextInFlock = prevInFlock = 0;
	nearestFlockMate = 0;
	nearestDistance = HUGE_VALF;
}

void FlockEntity::destroy()
{
	removeFromFlock();
	CollideEntity::destroy();
}


void FlockEntity::updateFlockData(void)
{
	for (size_t flockID = 0; flockID < flocks.size(); flockID++)
	{
		Flock *flock = flocks[flockID];
		if (flock)
		{
			flock->center = Vector(0,0,0);
			flock->heading = Vector(0,0,0);
			int numEntities = 0;
			for (FlockEntity *e = flock->firstEntity; e; e = e->nextInFlock)
			{
				flock->center += e->position;
				flock->heading += e->vel;
				e->nearestFlockMate = 0;
				e->nearestDistance = HUGE_VALF;
				for (FlockEntity *e2 = flock->firstEntity; e2 != e; e2 = e2->nextInFlock)
				{
					const float distanceSqr = (e2->position - e->position).getSquaredLength2D();
					if (distanceSqr < e->nearestDistance)
					{
						e->nearestFlockMate = e2;
						// Record the square for now (we'll sqrt it later)
						e->nearestDistance = distanceSqr;
					}
					if (distanceSqr < e2->nearestDistance)
					{
						e2->nearestFlockMate = e;
						e2->nearestDistance = distanceSqr;
					}
				}
				numEntities++;
			}
			for (FlockEntity *e = flock->firstEntity; e; e = e->nextInFlock)
			{
				e->nearestDistance = sqrtf(e->nearestDistance);
			}
			flock->center /= numEntities;
			flock->heading /= numEntities;
		}
	}
}
