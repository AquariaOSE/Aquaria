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
#include "RenderBase.h"


Hair::Hair(int nodes, float segmentLength, float hairWidth) : RenderObject()
{
	this->segmentLength = segmentLength;
	this->hairWidth = hairWidth;

	cull = false;

	hairNodes.resize(nodes);


	for (size_t i = 0; i < hairNodes.size(); i++)
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


}

void Hair::exertGravityWave(float dt)
{


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

void Hair::onRender(const RenderState& rs) const
{


	glBegin(GL_QUAD_STRIP);
	float texBits = 1.0f / (hairNodes.size()-1);

	Vector pl, pr;
	for (size_t i = 0; i < hairNodes.size(); i++)
	{


		if (i != hairNodes.size()-1)
		{
			Vector diffVec = hairNodes[i+1].position - hairNodes[i].position;
			diffVec.setLength2D(hairWidth);
			pl = diffVec.getPerpendicularLeft();
			pr = diffVec.getPerpendicularRight();
		}



		glTexCoord2f(0, texBits*i);
		glVertex3f(hairNodes[i].position.x + pl.x,  hairNodes[i].position.y + pl.y, 0);
		glTexCoord2f(1, texBits*i);
		glVertex3f( hairNodes[i].position.x + pr.x,  hairNodes[i].position.y + pr.y, 0);



	}
	glEnd();



}

void Hair::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);



}

void Hair::updatePositions()
{
	for (size_t i = 1; i < hairNodes.size(); i++)
	{
		Vector diff = hairNodes[i].position - hairNodes[i-1].position;



		if (diff.getLength2D() < segmentLength)
		{
			diff.setLength2D(segmentLength);
			hairNodes[i].position = hairNodes[i-1].position + diff;
		}
		else if (diff.getLength2D() > segmentLength)
		{

			diff.setLength2D(segmentLength);
			hairNodes[i].position = hairNodes[i-1].position + diff;
		}



	}


}

void Hair::returnToDefaultPositions(float dt)
{
	for (size_t i = 0; i < hairNodes.size(); i++)
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

	}
}

