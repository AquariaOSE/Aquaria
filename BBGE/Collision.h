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

#include "Base.h"

class RenderObject;
struct CollisionResult
{
	CollisionResult()
	{
		collided = false;
		collider = 0;
		project = true;
	}
	bool project;
	bool collided;
	Vector overlap;
	RenderObject *collider;

	void reportCollision(Vector overlap)
	{
		collided = true;
		this->overlap = overlap;
	}
};


class CollisionShape
{
public:
	CollisionShape();

	/*
	void addCollisionGroup(int group);
	void removeCollisionGroup(int group);
	void canCollideWithGroup();
	*/

	void updatePosition(const Vector &position);

	void activate()
	{
		active = true;
	}
	void deactivate()
	{
		active = false;
	}

	bool isActive()
	{
		return active;
	}

	float getX1();
	float getX2();
	float getY1();
	float getY2();
	float xw, yw;
	float radius;

	enum Type { NONE=0, AABB, CIRCLE, TOP_HALF_CIRCLE, TRIANGLE };

	void setType(Type type);
	Type getType();



	bool isPointWithin(Vector point);
	void render();

	CollisionResult findOverlap(CollisionShape &collisionShape);
	Vector offsetPosition;

	bool compareLayer(CollisionShape &c);
	//bool compareMask(CollisionShape &c);
	int getLayer() { return layer; }
	void setLayer(int layer) { this->layer = layer; }

	bool project;
protected:
	int layer;
	/*
	typedef std::vector<int> CollisionLayerMask;
	CollisionLayerMask colliderMask, collideeMask;
	*/
	std::vector<Vector> corners;
	Vector position;
	bool active;
	CollisionResult collideCircleWithCircle(CollisionShape &collisionShape);
	CollisionResult collideCircleWithTopHalfCircle(CollisionShape &collisionShape);
	CollisionResult collideCircleWithAABB(CollisionShape &collisionShape, float x, float y, int oH, int oV);



	Type type;
};

/*
class CollisionObject
{
public:
	void onCollision();
	void collide()
	{
		CollisionResult c;
		for (int i = 0; i < collisionManager->colliders.size(); i++)
		{
			c = findOverlap(collisionManager->colliders[i]->collisionShape);
			if (c.collided)
			{
				position -= c.overlap;
				collisionShape.position =
			}
		}
	}
};

class CollisionManager
{
public:
	void addCollider(
};
*/

