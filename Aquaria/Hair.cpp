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


Hair::Hair(size_t nodes, float segmentLength, float hairWidth)
	: SpineQuad(nodes, hairWidth * 2, 0.5f)
{
	addType(SCO_HAIR);
	this->segmentMinLength = segmentLength;
	this->segmentMaxLength = segmentLength;

	cull = false;

	percs.resize(nodes, 0);

	const float m = 1.0f / float(nodes);
	for (size_t i = 0; i < nodes; i++)
	{
		const float perc = float(i) * m;
		percs[i] = 1.0f-perc;
		Vector p(0, i*segmentLength, 0);
		points[i] = p;
	}

	updateVBO();
}

float Hair::getHairWidth() const
{
	return this->defaultWidth * 0.5f;
}

void Hair::setHairWidth(float w)
{
	this->defaultWidth = w * 2;
}

void Hair::setHeadPosition(const Vector &vec)
{
	points[0] = vec;
}

void Hair::onUpdate(float dt)
{
	updateVBO();
	SpineQuad::onUpdate(dt);
}

void Hair::onRender(const RenderState& rs) const
{
	if(parent)
	{
		// Hair is always drawn in the world coordinate system.
		// The base game never attaches hair as a child object to something else,
		// and the Hair object is always technically located at (0, 0) -- just the hair nodes are moved.
		// However, if a parent is set, this Hair would be drawn relative to the parent,
		// which is incorrect.
		// Note: This is for hairs added as children to other objects which ONLY mods do!
		glPushMatrix();
		glLoadIdentity();
		core->setupRenderPositionAndScale();
	}

	SpineQuad::onRender(rs);

	if(parent)
	{
		glPopMatrix();
	}
}

void Hair::updatePositions()
{
	for (size_t i = 1; i < points.size(); i++)
	{
		Vector diff = points[i] - points[i-1];
		float len = diff.getLength2D();
		len = std::min(segmentMaxLength, std::max(segmentMinLength, len));
		diff.setLength2D(len);
		points[i] = points[i-1] + diff;
	}
}

void Hair::exertForce(const Vector &force, float dt, int usePerc)
{
	const Vector f = force * dt;
	for (size_t i = points.size(); i --> 1; )
	{
		switch (usePerc)
		{
		case 0:
			points[i] += f * percs[i];
		break;
		case 1:
			points[i] += f * (1.0f-percs[i]);
		break;
		case 2:
		default:
			points[i] += f;
		break;
		}

	}
}

void Hair::exertNodeForce(size_t i, const Vector& force, float dt, int usePerc)
{
	const Vector f = force * dt;
	if(i >= points.size())
		return;

	switch (usePerc)
	{
	case 0:
		points[i] += f * percs[i];
	break;
	case 1:
		points[i] += f * (1.0f-percs[i]);
	break;
	case 2:
	default:
		points[i] += f;
	break;
	}
}

