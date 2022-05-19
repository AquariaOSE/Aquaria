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
#ifndef SCHOOLFISH_H
#define SCHOOLFISH_H

#include "FlockEntity.h"

class SchoolFish : public FlockEntity
{
public:
	SchoolFish(const std::string &texname = "flock-0001");
	int range;
	std::string swimSound;
	void applyLayer(int layer);
protected:
	float burstDelay;
	float soundDelay, flipDelay, dodgeAbility, respawnTimer, rippleTimer;
	int oldFlockID;
	float lastSpeed;

	void avoid(Vector &accumulator, Vector pos, bool inv=false);
	void applySeparation(Vector &accumulator);
	void applyCohesion(Vector &accumulator);
	void applyAlignment(Vector &accumulator, const Vector &dir);
	void applyAvoidance(Vector &accumulator);
	void updateVelocity(Vector &accumulator);

	void onEnterState(int action);
	void onUpdate(float dt);

	float avoidTime;

};

#endif
