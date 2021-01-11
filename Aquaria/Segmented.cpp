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
#include "Segmented.h"

#include "../BBGE/MathFunctions.h"

Segmented::Segmented(float minDist, float maxDist) : minDist(minDist), maxDist(maxDist)
{
	sqrMinDist = sqr(minDist);
	sqrMaxDist = sqr(maxDist);
}

void Segmented::setMaxDist(float m)
{
	maxDist = m;
	sqrMaxDist = sqr(maxDist);
}

void Segmented::initSegments(const Vector &position)
{
	for (size_t i = 0; i < segments.size(); i++)
		segments[i]->position = position;
	numSegments = segments.size();
}

void Segmented::destroySegments(float life)
{
	for (size_t i = 0; i < segments.size(); i++)
	{
		segments[i]->setLife(life);
		segments[i]->setDecayRate(1.0f);
		segments[i]->fadeAlphaWithLife = true;
	}
	segments.clear();
}

RenderObject *Segmented::getSegment(size_t seg)
{
	if (seg >= segments.size())
		return 0;
	return segments[seg];
}

void Segmented::updateSegment(int i, const Vector &diff)
{
	const float sqrLength = diff.getSquaredLength2D();

	if (sqrLength < sqrMinDist)
		return;

	if (sqrLength > sqrMaxDist)
	{
		Vector useDiff = diff;
		useDiff.setLength2D(maxDist);
		Vector reallyUseDiff = diff - useDiff;
		segments[i]->position += reallyUseDiff;
	}
	else
	{
		segments[i]->position += diff*0.05f;
	}

	float angle;
	MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), diff, angle);
	segments[i]->rotation.interpolateTo(Vector(0,0,angle), 0.2f);
}

void Segmented::updateAlpha(float a)
{
	for (size_t i = 0; i < segments.size(); i++)
	{
		segments[i]->alpha = a;
	}
}

void Segmented::warpSegments(const Vector &position)
{
	for (size_t i = 0; i < segments.size(); i++)
	{
		segments[i]->position = position;
	}
}

void Segmented::updateSegments(const Vector &position, bool reverse)
{
	const int top = segments.size()-1;
	const Vector *lastPosition = &position;
	if (!reverse)
	{
		for (int i = 0; i <= top; i++)
		{
			const Vector diff = *lastPosition - segments[i]->position;
			updateSegment(i, diff);
			lastPosition = &segments[i]->position;
		}
	}
	else
	{
		for (int i = top; i >= 0; i--)
		{
			const Vector diff = *lastPosition - segments[i]->position;
			updateSegment(i, diff);
			lastPosition = &segments[i]->position;
		}
	}
}

