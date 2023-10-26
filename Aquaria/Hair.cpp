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
#include "MathFunctions.h"

#include "Hair.h"
#include "DSQ.h"
#include "RenderBase.h"


Hair::Hair(int nodes, float segmentLength, float hairWidth)
	: RenderObject(), vbo(GPUBUF_DYNAMIC | GPUBUF_VERTEXBUF), ibo(GPUBUF_STATIC | GPUBUF_INDEXBUF)
{
	this->segmentLength = segmentLength;
	this->hairWidth = hairWidth;
	this->_hairfh = false;

	cull = false;

	hairNodes.resize(nodes);

	const float m = 1.0f / float(hairNodes.size());
	for (size_t i = 0; i < hairNodes.size(); i++)
	{
		const float perc = float(i) * m;
		hairNodes[i].percent = 1.0f-perc;
		Vector p(0, i*segmentLength, 0);
		hairNodes[i].position = p;
	}

	trisToDraw = ibo.initGridIndices_Triangles(2, nodes, false, GPUACCESS_DEFAULT);
}

void Hair::setHeadPosition(const Vector &vec)
{
	hairNodes[0].position = vec;
}

const HairNode *Hair::getHairNode(size_t idx) const
{
	return idx < hairNodes.size() ? &hairNodes[idx] : NULL;
}

void Hair::updateVBO()
{
	const float texBits = 1.0f / (hairNodes.size()-1);
	const Vector mul = !_hairfh ? Vector(1, 1) : Vector(-1, -1);

	Vector pl, pr;
	do
	{
		float *p = (float*)vbo.beginWrite(GPUBUFTYPE_VEC2_TC, hairNodes.size() * 2 * (2*2) * sizeof(float), GPUACCESS_DEFAULT);
		for (size_t i = 0; i < hairNodes.size(); i++)
		{
			const Vector pc = hairNodes[i].position;

			if (i != hairNodes.size()-1)
			{
				Vector diffVec = hairNodes[i+1].position - hairNodes[i].position;
				diffVec.setLength2D(hairWidth);
				pl = diffVec.getPerpendicularLeft() * mul;
				pr = diffVec.getPerpendicularRight() * mul;
			}
			*p++ = pc.x + pl.x;
			*p++ = pc.y + pl.y;
			*p++ = 0;
			*p++ = texBits*i;

			*p++ = pc.x + pr.x;
			*p++ = pc.y + pr.y;
			*p++ = 1;
			*p++ = texBits*i;
		}
	}
	while(!vbo.commitWrite());
}

void Hair::onUpdate(float dt)
{
	updateVBO();
	RenderObject::onUpdate(dt);
}

void Hair::onRender(const RenderState& rs) const
{
	vbo.apply();
	ibo.drawElements(GL_TRIANGLES, trisToDraw);
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


void Hair::exertForce(const Vector &force, float dt, int usePerc)
{
	const Vector f = force * dt;
	for (int i = hairNodes.size()-1; i >= 1; i--)
	{
		switch (usePerc)
		{
		case 0:
			hairNodes[i].position += f * hairNodes[i].percent;
		break;
		case 1:
			hairNodes[i].position += f * (1.0f-hairNodes[i].percent);
		break;
		case 2:
		default:
			hairNodes[i].position += f;
		break;
		}

	}
}

void Hair::exertNodeForce(size_t i, const Vector& force, float dt, int usePerc)
{
	const Vector f = force * dt;
	if(i >= hairNodes.size())
		return;

	switch (usePerc)
	{
	case 0:
		hairNodes[i].position += f * hairNodes[i].percent;
	break;
	case 1:
		hairNodes[i].position += f * (1.0f-hairNodes[i].percent);
	break;
	case 2:
	default:
		hairNodes[i].position += f;
	break;
	}
}

