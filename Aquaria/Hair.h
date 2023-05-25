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
#ifndef __hair__
#define __hair__

#include "../BBGE/Quad.h"

struct HairNode
{
	HairNode() : percent(0), problem(false), angleDiff(0)
	{}
	float percent; // percent of how much force is affected on this node
	Vector position; // position of the hair node
	Vector defaultPosition; // default position of the hair node
	Vector originalPosition;
	bool problem;
	float angleDiff;
};

class Hair : public RenderObject
{
public:
	Hair(int nodes=40, float segmentLength=3, float width=18);

	void exertForce(const Vector &force, float dt, int usePerc=0);
	void updatePositions();
	void returnToDefaultPositions(float dt);

	float hairWidth;

	std::vector<HairNode> hairNodes;

	void setHeadPosition(const Vector &pos);

	void exertWave(float dt);
	void exertGravityWave(float dt);
	HairNode *getHairNode(int idx);
protected:
	float segmentLength;
	void onUpdate(float dt) OVERRIDE;
	void onRender(const RenderState& rs) const OVERRIDE;
};

#endif

