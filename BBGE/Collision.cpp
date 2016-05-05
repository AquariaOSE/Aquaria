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
#include "Collision.h"

CollisionShape::CollisionShape()
{
	xw = yw = 0;
	setType(CIRCLE);
	active = true;
	project = true;
	radius = 32;
	corners.resize(4);
	layer = 0;
}

void CollisionShape::setType(Type type)
{
	this->type = type;
}

CollisionShape::Type CollisionShape::getType()
{
	return type;
}

/*
bool CollisionShape::compareMask(CollisionShape &c)
{
	for (CollisionLayerMask::iterator i = c.colliderMask.begin(); i != c.colliderMask.end(); i++)
	{
		for (CollisionLayerMask::iterator j = c.collideeMask.begin(); j != c.collideeMask.end(); j++)
		{
			if ((*i)) && (*j))
			{
				return true;
			}
		}
	}
	return false;
}
*/
bool CollisionShape::compareLayer(CollisionShape &c)
{	
	return (this->layer <= c.layer);
}

void CollisionShape::updatePosition(const Vector &position)
{
	this->position = position + offsetPosition;
	/*
	switch (getType())
	{
	case AABB:
	{
		corners[0] = position + Vector( - xw, - yw);
		corners[1] = position + Vector(xw, - yw);
		corners[2] = position + Vector(xw, yw);
		corners[3] = position  + Vector(-xw ,yw);
	}
	break;
	}
	*/
	
}

CollisionResult CollisionShape::findOverlap(CollisionShape &collisionShape)
{
	CollisionResult c;


	switch (getType())
	{
	case CIRCLE:
	{
		switch(collisionShape.getType())
		{
		case CIRCLE:
			c = collideCircleWithCircle(collisionShape);
		break;
		case AABB:
		{
			
			float txw = collisionShape.xw;
			float tyw = collisionShape.yw;
							
			Vector d = position - collisionShape.position;//tile->obj delta
			int px = (txw + radius) - fabsf(d.x);//penetration depth in x

			if(0 < px)
			{
				int py = (tyw + radius) - fabsf(d.y);//pen depth in y
								
				if(0 < py)
				{
					//object may be colliding with tile
												
					//determine grid/voronoi region of circle center
					float oH = 0;
					float oV = 0;
					if(d.x < -txw)
					{
						//circle is on left side of tile
						oH = -1;
					}
					else if(txw < d.x)
					{
						//circle is on right side of tile
						oH = 1;
					}
								
					if(d.y < -tyw)
					{
						//circle is on top side of tile
						oV = -1;
					}
					else if(tyw < d.y)
					{
						//circle is on bottom side of tile
						oV = 1;
					}			

					c = collideCircleWithAABB(collisionShape, px, py, oH, oV);
					//ResolveCircleTile(px,py,oH,oV,this,c);

				}
			}
			//return collideCircleWithAABB(collisionShape);
		}
		break;
		case TOP_HALF_CIRCLE:
			c = collideCircleWithTopHalfCircle(collisionShape);
		break;
		}
	}
	break;
	}
	return c;
}

CollisionResult CollisionShape::collideCircleWithCircle(CollisionShape &collisionShape)
{
	CollisionResult c;
	Vector dist = position - collisionShape.position;// - position;
	float fastLen = dist.getSquaredLength2D();
	float totalDist = (radius + collisionShape.radius);
	if (fastLen < (totalDist*totalDist))
	{
		/*
		std::ostringstream os;
		os << "len " << len << " totalDist " << totalDist;
		msg(os.str());
		*/
		float len = dist.getLength2D();
		c.collided = true;
		dist.setLength2D(totalDist - len);
		//dist |= totalDist;
		c.overlap = dist;
	}
	else
	{
		c.collided = false;
	}
	return c;
}

float CollisionShape::getY1()
{
	return position.y - yw;
}

float CollisionShape::getY2()
{
	return position.y + yw;
}

float CollisionShape::getX1()
{
	return position.x - xw;
}

float CollisionShape::getX2()
{
	return position.x + xw;
}

void CollisionShape::render()
{
	glTranslatef(offsetPosition.x, offsetPosition.y,0);
	switch(getType())
	{
	case CIRCLE:
		drawCircle(radius);
	break;
	case AABB:
	//case CIRCLE:
		//glColor3f(1,1,1);
		
		//glLineWidth(2);
		
		glBegin(GL_QUADS);
		{
			glVertex2f(-xw,yw);
			glVertex2f(xw,yw);
			glVertex2f(xw,-yw);
			glVertex2f(-xw,-yw);

			/*
			glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
			glVertex2f(topLeft.x, topLeft.y);
			glVertex2f(bottomRight.x, topLeft.y);

			glVertex2f(bottomRight.x, topLeft.y);
			glVertex2f(bottomRight.x, bottomRight.y);

			glVertex2f(bottomRight.x, bottomRight.y);
			glVertex2f(topLeft.x, bottomRight.y);

			glVertex2f(topLeft.x, bottomRight.y);
			glVertex2f(topLeft.x, topLeft.y);

			*/
		}
		glEnd();

	break;
	}

	glTranslatef(-offsetPosition.x, -offsetPosition.y,0);
	//glDisable(GL_BLEND);
	
}

/*

  // FOR EDGES

  	if (position.y > collisionShape.getY1() - radius && position.y < collisionShape.getY2() + radius)
	{
		float dist = collisionShape.getX1() - position.x;
		if (dist > 0 && dist < radius)
		{
			c.collided = true;
			c.overlap = Vector(radius - dist,0);
		}
		else if (dist < 0 && -dist < radius)
		{
			c.collided = true;
			c.overlap = -Vector(radius - (-dist),0);
		}
	}

	if (!c.collided)
	{
		if (position.y > collisionShape.getY1() - radius && position.y < collisionShape.getY2() + radius)
		{
			float dist = collisionShape.getX2() - position.x;
			if (dist > 0 && dist < radius)
			{
				c.collided = true;
				c.overlap += Vector(radius - dist,0);
			}
			else if (dist < 0 && -dist < radius)
			{
				c.collided = true;
				c.overlap += -Vector(radius - (-dist),0);
			}
		}
	}
*/

CollisionResult CollisionShape::collideCircleWithAABB(CollisionShape &collisionShape, float x, float y, int oH, int oV)
{
	CollisionResult c;
	if(oH == 0)
	{
		if(oV == 0)
		{

			//collision with current cell
			if(x < y)
			{					
				//penetration in x is smaller; project in x
				float dx = position.x - collisionShape.position.x;//get sign for projection along x-axis
				
		
				
//				msg("oH==0, oV ==0, x <y");
				//NOTE: should we handle the delta == 0 case?! and how? (project towards oldpos?)
				if(dx < 0)
				{
					c.reportCollision(Vector(-x, 0));
					return c;
				}
				else
				{
					c.reportCollision(Vector(x,0));
					return c;
				}
			}
			else
			{		
//				msg("oH==0, oV ==0, x >= y");
				//penetration in y is smaller; project in y		
				float  dy = position.y - collisionShape.position.y;//get sign for projection along y-axis

				//NOTE: should we handle the delta == 0 case?! and how? (project towards oldpos?)					
				if(dy < 0)
				{
					c.reportCollision(Vector(0, -y));
					return c;
				}
				else
				{					
					c.reportCollision(Vector(0, y));
					return c;
				}				
			}					
		}
		else
		{
//			msg ("oH == 0, oV != 0");
			c.reportCollision(Vector(0, y*oV));
			return c;
		}
	}
	else if(oV == 0)
	{
//		msg ("oV == 0");
		c.reportCollision(Vector(x*oH,0));
		return c;
	}
	else
	{			
		//diagonal collision
		
		//get diag vertex position
		float vx = collisionShape.position.x + (oH*collisionShape.xw);
		float vy = collisionShape.position.y + (oV*collisionShape.yw);
		
		float dx = position.x - vx - 1;//calc vert->circle vector		
		float dy = position.y - vy - 1;
		
		float len = sqrtf(dx*dx + dy*dy);
		float pen = radius - len;
		if(0 < pen)
		{
			//vertex is in the circle; project outward
			if(len == 0)
			{
				//project out by 45deg
				dx = oH / SQRT2;
				dy = oV / SQRT2;
			}
			else
			{
				dx /= len;
				dy /= len;
			}
			
			c.reportCollision(Vector(dx*pen, dy*pen));
			//obj.ReportCollisionVsWorld(dx*pen, dy*pen, dx, dy, t);
			
			return c;
		}
	}

	return c;
	
}

bool CollisionShape::isPointWithin(Vector point)
{
	switch (this->getType())
	{
	case CIRCLE:
	{
		Vector dist = point - this->position;
		return (dist.getSquaredLength2D() < sqr(this->radius));
	}
	break;
	case AABB:
	{
		if (point.x < position.x + xw && point.y < position.y + yw)
		{
			if (point.x > position.x - xw && point.y > position.y - yw)
			{
				return true;
			}
		}
	}
	break;
	}
	return false;
}

CollisionResult CollisionShape::collideCircleWithTopHalfCircle(CollisionShape &collisionShape)
{
	CollisionResult c;
	Vector dist = collisionShape.position - position;
	float len = dist.getLength2D();
	float totalDist = (radius + collisionShape.radius);

	//which edge did we hit?
	if (collisionShape.position.y > (position.y - (radius/2)))
	{
		if (len < collisionShape.radius)
		{
			c.collided = true;
			c.overlap = Vector(0, position.y - (collisionShape.position.y - collisionShape.radius));
		}
	}
	else if (len < totalDist && dist.y > 0)
	{
		c.collided = true;
		dist.setLength2D(totalDist - len);
		c.overlap = dist;
	}
	else
	{
		c.collided = false;
	}
	return c;
}
