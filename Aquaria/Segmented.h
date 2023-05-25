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
#ifndef SEGMENTED_H
#define SEGMENTED_H

#include "../BBGE/Quad.h"

class Segmented
{
public:
	Segmented(float minDist, float maxDist);
	RenderObject *getSegment(size_t seg);
	void updateAlpha(float a);
	void warpSegments(const Vector &position);
	void setMaxDist(float m);
protected:

	float minDist, maxDist;
	float sqrMinDist, sqrMaxDist;
	void initSegments(const Vector &position);
	void updateSegments(const Vector &position, bool reverse=false);
	void updateSegment(int i, const Vector &diff);
	void destroySegments(float life = 0.01f);
	std::vector<Vector> lastPositions;
	int numSegments;
	std::vector<RenderObject *> segments;
};

class Strand : public RenderObject, public Segmented
{
public:
	Strand(const Vector &position, size_t segs, size_t dist=32);
	void destroy() OVERRIDE;
protected:
	void onUpdate(float dt) OVERRIDE;
	void onRender(const RenderState& rs) const OVERRIDE;
};

#endif
