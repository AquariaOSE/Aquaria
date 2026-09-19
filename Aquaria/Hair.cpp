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

	// Hair is always drawn in the world coordinate system.
	// The base game never attaches hair as a child object to something else,
	// and the Hair object is always technically located at (0, 0) -- just the hair nodes are moved.
	// However, if a parent is set, this Hair would be drawn relative to the parent,
	// which is incorrect.
	// Note: This is for hairs added as children to other objects which ONLY mods do!
	this->pointsAreInWorldCoordSystem = true;

	cull = false;

	percs.resize(nodes, 0);

	const float m = 1.0f / float(nodes);
	for (size_t i = 0; i < nodes; i++)
	{
		const float perc = float(i) * m;
		percs[i] = 1.0f-perc;
		Vector p(0, i*segmentLength, 1);
		_points[i] = p;
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
	_points[0].x = vec.x;
	_points[0].y = vec.y;
}

void Hair::onUpdate(float dt)
{
	updateVBO();
	SpineQuad::onUpdate(dt);
}

void Hair::updatePositions()
{
	for (size_t i = 1; i < _points.size(); i++)
	{
		Vector diff = _points[i] - _points[i-1];
		diff.z = 0;
		float len = diff.getLength2D();
		len = std::min(segmentMaxLength, std::max(segmentMinLength, len));
		diff.setLength2D(len);
		_points[i] = _points[i-1] + diff;
	}
}

void Hair::exertForce(const Vector &force, float dt, int usePerc)
{
	Vector fbase = force * dt;
	for (size_t i = _points.size(); i --> 1; )
	{
		Vector f = fbase;
		switch (usePerc)
		{
		case 0:
			f *= percs[i];
		break;
		case 1:
			f *= (1.0f-percs[i]);
		break;
		default:
		break; // Do nothinhg, use f as-is
		}
		_points[i].x += f.x;
		_points[i].y += f.y;
	}
}

void Hair::exertNodeForce(size_t i, const Vector& force, float dt, int usePerc)
{
	if(i >= _points.size())
		return;

	Vector f = force * dt;

	switch (usePerc)
	{
	case 0:
		 f *= percs[i];
	break;
	case 1:
		f *= (1.0f-percs[i]);
	break;
	default:
	break; // Do nothinhg, use f as-is
	}

	_points[i].x += f.x;
	_points[i].y += f.y;
}

