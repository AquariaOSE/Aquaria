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
#pragma once

inline bool RenderObject::isOnScreen()
{
	if (followCamera == 1) return true;
	//if (followCamera != 0) return true;

	// note: radii are sqr-ed for speed
	const float cullRadiusSqr = (getCullRadiusSqr() * core->invGlobalScaleSqr) + core->cullRadiusSqr;

	return ((this->getFollowCameraPosition() - core->cullCenter).getSquaredLength2D() < cullRadiusSqr);
}

Vector RenderObject::getFollowCameraPosition() const
{
	float f = followCamera;
	if (f == 0 && layer != -1)
		f = core->renderObjectLayers[layer].followCamera;

	if (f <= 0)
	{
		return position;
	}
	else
	{
		Vector pos = position;
		int fcl = 0;
		if (layer != -1)
			fcl = core->renderObjectLayers[layer].followCameraLock;

		switch (fcl)
		{
		case FCL_HORZ:
			pos.x = position.x - core->screenCenter.x;
			pos.x *= f;
			pos.x = core->screenCenter.x + pos.x;
		break;
		case FCL_VERT:
			pos.y = position.y - core->screenCenter.y;
			pos.y *= f;
			pos.y = core->screenCenter.y + pos.y;
		break;
		default:
			pos = position - core->screenCenter;
			pos *= f;
			pos = core->screenCenter + pos;
		break;
		}

		return pos;
	}
}
