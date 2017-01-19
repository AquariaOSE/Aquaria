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
#include "Base.h"

#ifdef __GNUC__
	#define UNUSED __attribute__((unused))  // Avoid "unused function" warnings
#else
	#define UNUSED //nothing
#endif

namespace MathFunctions
{
	UNUSED static void calculateAngleBetweenVectorsInDegrees(const Vector &vector1, const Vector &vector2, float &solutionAngle)
	{
		Vector dist = vector1 - vector2;

		// 0 is down
		// 90 is right
		// 180 is up
		// 270 is left
		// 360 is down
		solutionAngle = atan2f(dist.y, fabsf(dist.x));
		solutionAngle = (solutionAngle/PI)*180;
		if (dist.x < 0)
			solutionAngle = 180-solutionAngle;
		solutionAngle += 90;
	}

	UNUSED static float toRadians(float rot)
	{
		return PI-(rot*PI)/180.0f;
	}

	UNUSED static float getAngleToVector(const Vector &addVec, float offsetAngle = 0)
	{
		float angle=0;
		MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), addVec, angle);
		angle = 180-(360-angle);
		angle += offsetAngle;
		return angle;
	}

}

