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
#ifndef RENDEROBJECT_INLINE_H
#define RENDEROBJECT_INLINE_H

inline bool RenderObject::isOnScreen() const
{
	if (followCamera == 1) return true;


	// note: radii are sqr-ed for speed
	const float cullRadiusSqr = (getCullRadiusSqr() * core->invGlobalScaleSqr) + core->cullRadiusSqr;

	return ((this->getFollowCameraPosition() - core->cullCenter).getSquaredLength2D() < cullRadiusSqr);
}

Vector RenderObject::getFollowCameraPosition() const
{
	return getFollowCameraPosition(position);
}

Vector RenderObject::getFollowCameraPosition(const Vector& v) const
{
	assert(layer != LR_NONE);
	assert(!parent); // this makes no sense when we're not a root object
	const RenderObjectLayer &rl = core->renderObjectLayers[layer];
	Vector M = rl.followCameraMult;
	float F = followCamera;
	if(!F)
		F = rl.followCamera;
	if (F <= 0)
		return v;

	/* Originally, not accounting for parallax lock on an axis, this was:
		pos = v - core->screenCenter;
		pos *= F;
		pos = core->screenCenter + pos;
	*/

	// uppercase are effectively constants that are not per-object
	// lowercase are per-object

	// more concise math:
	//const Vector pos = (v - core->screenCenter) * F + core->screenCenter;
	//return v * (Vector(1,1) - M) + (pos * M); // lerp

	// optimized and rearranged
	const Vector C = core->screenCenter;
	const Vector M1 = Vector(1,1) - M;
	const Vector T = C * (1 - F);

	const Vector pos = T + (F * v);
	return v * M1 + (pos * M); // lerp, used to select whether to use original v or parallax-corrected v
}

#endif
