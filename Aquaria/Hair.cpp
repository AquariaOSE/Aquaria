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
#include "../BBGE/MathFunctions.h"

#include "Hair.h"
#include "DSQ.h"

// nodes = 40
// segmentLength = 3
Hair::Hair(int nodes, float segmentLength, float hairWidth) : RenderObject()
{
	this->segmentLength = segmentLength;
	this->hairWidth = hairWidth;

	cull = false;

	hairNodes.resize(nodes);
	// nodes: 20 length: 6
	//segmentLength = 3;
	for (int i = 0; i < hairNodes.size(); i++)
	{
		float perc = (float(i)/(float(hairNodes.size())));
		if (perc < 0)
			perc = 0;
		hairNodes[i].percent = 1.0f-perc;
		hairNodes[i].position = hairNodes[i].originalPosition = hairNodes[i].defaultPosition = Vector(0, i*segmentLength, 0);
	}
}

void Hair::exertWave(float dt)
{
	/*
	Vector diff = headPos - position;
	Vector diff2;
	if (!isFlippedHorizontal())
		diff2 = diff.getPerpendicularLeft();
	else
		diff2 = diff.getPerpendicularRight();

	Vector diff3 = position - headPos;
	float len =diff2.getLength2D();
	diff3.setLength2D(len);

	hairTimer += dt;
	while (hairTimer > 2.0f)
	{
		hairTimer -= 2.0f;
	}
	float useTimer = hairTimer;
	if (useTimer > 1.0f)
		useTimer = 1.0f - (hairTimer-1);
	float frc = 0.5;
	diff = (diff2*(frc*(1.0f-(useTimer*0.5f))) + diff3*(frc);

	diff.setLength2D(400);
	//if (!vel.isLength2DIn(10))
	exertForce(diff, dt);
	*/
}

void Hair::exertGravityWave(float dt)
{
	/*
	Vector diff = headPos - position;
	Vector diff2;
	if (!isFlippedHorizontal())
		diff2 = diff.getPerpendicularLeft();
	else
		diff2 = diff.getPerpendicularRight();

	Vector diff3 = position - headPos;
	float len =diff2.getLength2D();
	diff3.setLength2D(len);

	hairTimer += dt;
	while (hairTimer > 2.0f)
	{
		hairTimer -= 2.0f;
	}
	float useTimer = hairTimer;
	if (useTimer > 1.0f)
		useTimer = 1.0f - (hairTimer-1);
	float frc = 0.333333;
	diff = (diff2*(frc*(1.0f-(useTimer*0.5f))) + diff3*(frc) + Vector(0,len)*(frc*(0.5f+useTimer*0.5f)));

	diff.setLength2D(400);
	//if (!vel.isLength2DIn(10))
	exertForce(diff, dt);
	*/
}

void Hair::setHeadPosition(const Vector &vec)
{
	hairNodes[0].position = vec;
}

HairNode *Hair::getHairNode(int idx)
{
	HairNode *h = 0;
	int sz = hairNodes.size();
	if (!(idx < 0 || idx >= sz))
	{
		h = &hairNodes[idx];
	}
	return h;
}

void Hair::onRender()
{
#ifdef BBGE_BUILD_OPENGL
	//glDisable(GL_CULL_FACE);

	glBegin(GL_QUAD_STRIP);
	float texBits = 1.0f / (hairNodes.size()-1);
	//float height2 = 2.5f;
	Vector pl, pr;
	for (int i = 0; i < hairNodes.size(); i++)
	{
		//glNormal3f( 0.0f, 0.0f, 1.0f);

		if (i != hairNodes.size()-1)
		{
			Vector diffVec = hairNodes[i+1].position - hairNodes[i].position;
			diffVec.setLength2D(hairWidth);
			pl = diffVec.getPerpendicularLeft();
			pr = diffVec.getPerpendicularRight();
		}

		/*
		if (hairNodes[i].problem)
		{
			glColor3f(1,0,0);
		}
		else
			glColor3f(1,1,1);
		*/


		glTexCoord2f(0, texBits*i);
		glVertex3f(hairNodes[i].position.x + pl.x,  hairNodes[i].position.y + pl.y, 0);
		glTexCoord2f(1, texBits*i);
		glVertex3f( hairNodes[i].position.x + pr.x,  hairNodes[i].position.y + pr.y, 0);

		//float angle = 0;
		/*
		float angle = 0;
		if (i < hairNodes.size()-1)
		{
			MathFunctions::calculateAngleBetweenVectorsInDegrees(hairNodes[i+1].position, hairNodes[i].position, angle);
			angle += 90;
			angle = (angle*PI)/180.0f;
		}
		*/
		/*
		glTexCoord2f(0, 1-texBits*i);
		glVertex3f(hairNodes[i].position.x -sinf(angle)*hairWidth,  hairNodes[i].position.y + cosf(angle)*height2, 0);
		glTexCoord2f(1, 1-texBits*i);
		glVertex3f( hairNodes[i].position.x + sinf(angle)*hairWidth,  hairNodes[i].position.y + cosf(angle)*height2,  0);
		*/
	}
	glEnd();

	/*
	glColor3f(1,1,1);
	for (int i = 0; i < hairNodes.size(); i++)
	{
		std::ostringstream os;
		os << hairNodes[i].angleDiff;
		core->print(hairNodes[i].position.x, hairNodes[i].position.y, os.str().c_str(), 6);
	}

	*/
	//glEnable(GL_CULL_FACE);
#endif
}

void Hair::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
	/*
	// straighten hair
	if (hairNodes.size()>2)
	{
		Vector d1 = hairNodes[1].position - hairNodes[0].position;
		for (int i = 2; i < hairNodes.size(); i++)
		{
			Vector d2 = hairNodes[i].position - hairNodes[i-1].position;

			Vector wantPos = hairNodes[i-1].position + d1;

			float perc = 1.0f-float(i)/float(hairNodes.size());
			hairNodes[i].position += (wantPos - hairNodes[i].position)*dt*(40 * perc);
			//Vector d1 = hairNodes[i-1].position - hairNodes[i-2].position;
			//Vector d2 = hairNodes[i].position - hairNodes[i-1].position;
			//float prod = d1.dot2D(d2);
			////if (prod < 0.5f)
			//{
			//	d1.setLength2D(segmentLength);
			//
			//	Vector wantPos = (hairNodes[i-1].position + d1)*0.5f + hairNodes[i].position*0.5f;
			//	hairNodes[i].position += (wantPos - hairNodes[i].position)*dt*10;
			//	break;
			//}
		}
	}
	*/
}

void Hair::updatePositions()
{
	BBGE_PROF(Hair_updatePositions);
	//int minLength = 1;
	/*
	Vector accum;
	for (int i = 1; i < hairNodes.size(); i++)
	{
		Vector diff = hairNodes[i].position - hairNodes[i-1].position;
		accum += diff;
	}
	accum /= float(hairNodes.size()-1);
	*/
	for (int i = 1; i < hairNodes.size(); i++)
	{
		Vector diff = hairNodes[i].position - hairNodes[i-1].position;
		/*
		if (diff.getLength2D() < 1)
		{
			diff = accum;
		}
		*/
		/*
		if (diff.getLength2D() <= 1)
		{
			diff = hairNodes[i].position - hairNodes[0].position;
		}
		*/

		if (diff.getLength2D() < segmentLength)
		{
			diff.setLength2D(segmentLength);
			hairNodes[i].position = hairNodes[i-1].position + diff;
		}
		else if (diff.getLength2D() > segmentLength)
		{
			//diff |= segmentLength;
			diff.setLength2D(segmentLength);
			hairNodes[i].position = hairNodes[i-1].position + diff;
		}


		/*
		if (i > 1)
		{
			Vector d1 = hairNodes[i-1].position - hairNodes[i-2].position;
			Vector d2 = hairNodes[i].position - hairNodes[i-1].position;
			float prod = d1.dot2D(d2);
			float a1 = 0, a2 = 0;
			float maxAngle = 0.3;
			MathFunctions::calculateAngleBetweenVectorsInRadians(d1, Vector(0,0,0), a1);
			MathFunctions::calculateAngleBetweenVectorsInRadians(d2, Vector(0,0,0), a2);
			float a = a2 - a1;
			hairNodes[i].angleDiff = a;
			if (fabsf(a) > maxAngle)
			{

				float len = d2.getLength2D();
				//d2 = d1;
				Vector dt1 = d1;
				Vector dt2 = d1;
				dt1.rotate2D(-maxAngle);
				dt2.rotate2D(maxAngle);

				//if (a < 0)
				//	d2.rotate2D(-maxAngle);
				//else
				//	d2.rotate2D(maxAngle);

				//d2.setLength2D(len);
				//hairNodes[i].position = hairNodes[i-1].position + d2;

				dt1 = hairNodes[i-1].position + dt1;
				dt2 = hairNodes[i-1].position + dt2;
				if ((hairNodes[i].position - dt1).getSquaredLength2D() < (hairNodes[i].position - dt2).getSquaredLength2D())
				{
					hairNodes[i].position = dt1;
				}
				else
					hairNodes[i].position = dt2;
				hairNodes[i].problem = true;
			}
			else
			{
				hairNodes[i].problem = false;
			}
		}
		*/

		/*
		int diffLength = segmentLength * 2;
		Vector accum;
		int c=0;
		for (int j = 0; j < hairNodes.size(); j++)
		{
			if (j != i && j != i-1 && j != i+1)
			{
				Vector diff = hairNodes[i].position - hairNodes[j].position;

				if (diff.getLength2D() < diffLength)
				{
					diff.setLength2D(diffLength);
					//hairNodes[i].position = hairNodes[j].position + diff;
					accum += hairNodes[j].position + diff;
					c++;
					//break;
				}
			}
		}

		if (!accum.isZero())
		{
			accum /= c;

			//hairNodes[i].position = (hairNodes[i].position + accum)*0.5f;
			hairNodes[i].position = accum;
		}
		*/

		/*
		if (i > 1)
		{
			Vector d1 = hairNodes[i-1].position - hairNodes[i-2].position;
			Vector d2 = hairNodes[i].position - hairNodes[i-1].position;
			float prod = d1.dot2D(d2);
			if (prod < -0.5f)
			{
				//d1.setLength2D(d2.getLength2D());
				Vector wantPos = hairNodes[i-1].position + d1;
				hairNodes[i].position = wantPos;//hairNodes[i].position*0.75f + wantPos*0.25f;
				//
				//d2 = (d2 + d1)*0.5f;
				//hairNodes[i].position = hairNodes[i-1].position + d1;
			}
		}
		*/
		/*
		if (i > 1)
		{
			//float a1,a2;
			float a1=0;
		//	MathFunctions::calculateAngleBetweenVectorsInRadians(hairNodes[i].position, hairNodes[i-1].position, a1);

			MathFunctions::calculateAngleBetweenVectorsInRadians(hairNodes[i-1].position, hairNodes[i-2].position, a1);

			Vector d1 = hairNodes[i-1].position - hairNodes[i-2].position;
			Vector d2 = hairNodes[i].position - hairNodes[i-1].position;

			float a=0;
			MathFunctions::calculateAngleBetweenVectorsInRadians(d2, d1, a);
			//float d = a2 - a1, c=PI/2;

			float c=PI/2;
			float d=0;

			bool adjust = 0;
			if (a > c)
			{
				d = a1 + c;
				adjust = 1;
			}
			else if (a < -c)
			{
				d = a1 - c;
				adjust = -1;
			}
			if (adjust)
			{
				Vector add(sinf(d), cosf(d));
				add.setLength2D(d2.getLength2D());
				hairNodes[i].position = hairNodes[i-1].position + add;
			}
			//Vector diff2 = hairNodes[i-1] - hairNodes[i-2].position;

			//hairNodes[i].position = hairNodes[i-1].position + diff;
		}
		*/
		/*
		if (i < hairNodes.size()-1)
		{
			Vector diff2 = hairNodes[i+1].position - hairNodes[i].position;
			float a=0,c=PI/2;
			bool adjust = false;
			MathFunctions::calculateAngleBetweenVectorsInRadians(hairNodes[i+1].position, hairNodes[i].position, a);
			if (a > c)
			{
				a = c;
				adjust = true;
			}
			else if (a < -c)
			{
				a = -c;
				adjust = true;
			}
			if (adjust)
			{
				Vector add(sinf(a), cosf(a));
				add *= diff2.getLength2D();
				hairNodes[i+1].position = hairNodes[i].position + add;
			}
		}
		*/
		/*
		else if (diff.getLength2D() < minLength)
		{
		*/
			/*
			diff.setLength2D(minLength);
			hairNodes[i].position = hairNodes[i-1].position + diff;
			*/
		//}
	}


}

void Hair::returnToDefaultPositions(float dt)
{
	for (int i = 0; i < hairNodes.size(); i++)
	{
		Vector mov = hairNodes[i].defaultPosition - hairNodes[i].position;
		if (!mov.isLength2DIn(2))
		{
			if (mov.x != 0 || mov.y != 0)
			{
				mov *= dt;
				hairNodes[i].position += mov;
			}
		}
	}
}

void Hair::exertForce(const Vector &force, float dt, int usePerc)
{

	for (int i = hairNodes.size()-1; i >= 1; i--)
	{
		switch (usePerc)
		{
		case 0:
			hairNodes[i].position += force*dt*hairNodes[i].percent;
		break;
		case 1:
			hairNodes[i].position += force*dt*(1.0f-hairNodes[i].percent);
		break;
		case 2:
		default:
			hairNodes[i].position += force*dt;
		break;
		}
		/*
		Vector diff = hairNodes[i].position - hairNodes[i-1].position;

		if (diff.getSquaredLength2D() > sqr(segmentLength))
		{
			diff |= segmentLength;
			hairNodes[i].position = hairNodes[i-1].position + diff;
		}
		*/
	}
}

