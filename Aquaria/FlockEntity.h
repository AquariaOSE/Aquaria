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
#ifndef FLOCKENTITY_H
#define FLOCKENTITY_H

#include "CollideEntity.h"


class FlockEntity;

struct Flock
{
	Flock(int id): flockID(id)
	{
		firstEntity = 0;
	}

	int flockID;
	FlockEntity *firstEntity;
	Vector center, heading;
};

class FlockEntity : public CollideEntity
{
public:
	FlockEntity();
	enum FlockType {
		FLOCK_FISH = 0,
		MAX_FLOCKTYPES
	};
	void addToFlock(size_t id);
	void removeFromFlock();
	void destroy();

	static void updateFlockData(void);

	FlockType flockType;
	typedef std::vector<Vector> VectorSet;
	float angle;

protected:

	Vector getFlockCenter() const {return flock ? flock->center : Vector(0,0,0);}
	Vector getFlockHeading() const {return flock ? flock->heading : Vector(0,0,0);}
	FlockEntity *getNearestFlockEntity() const {return nearestFlockMate;}
	float getNearestFlockEntityDist() const {return nearestDistance;}

	Flock *flock;
	FlockEntity *nextInFlock, *prevInFlock;
	FlockEntity *nearestFlockMate;
	float nearestDistance;
};

#endif
