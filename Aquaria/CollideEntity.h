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
#pragma once

#include "Entity.h"

class CollideEntity : public Entity
{
public:
	CollideEntity();
	float bounceAmount, bounceEntityAmount;
	float weight;
	Vector collideOffset;
	void updateMovement(float dt);	
	void entityDied(Entity *e);
protected:	
	virtual void onHitWall(){}
	virtual void onHitEntity(const CollideData &c){}
	void onUpdateFrozen(float dt);
	
	virtual void onBounce() {}
	float friction;
	bool doCusion;
	void bounce(float ba);
	
};
