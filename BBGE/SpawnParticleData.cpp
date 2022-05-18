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
#include "Particles.h"

SpawnParticleData::SpawnParticleData()
{
	suckIndex = -1;
	suckStr = 0;

	randomScale1 = 1;
	randomScale2 = 1;
	randomAlphaMod1 = 1;
	randomAlphaMod2 = 1;
	influenced = 0;
	spawnLocal = false;
	useSpawnRate = false;
	counter = 0;
	life = 1;
	blendType = BLEND_DEFAULT;
	spawnRate = 1;
	scale = Vector(1,1,1);
	width = 64;
	height = 64;
	fadeAlphaWithLife = false;
	color = Vector(1,1,1);
	alpha = 1;
	randomSpawnRadius = 0;
	lastDTDifference = 0;
	addAsChild = false;

	randomRotationRange = 0;
	number = 1;

	randomParticleAngleRange = 0;
	velocityMagnitude = 1;

	calculateVelocityToCenter = false;

	randomSpawnRadiusRange = 0;
	randomSpawnMod = Vector(1,1);

	spawnArea = SPAWN_CIRCLE;
	randomVelocityMagnitude = 0;
	randomVelocityRange = 360;

	copyParentRotation = 0;
	justOne = didOne = false;
	flipH = flipV = 0;
	spawnTimeOffset = 0;
	pauseLevel = 0;
	copyParentFlip = 0;
	inheritColor = false;
	inheritAlpha = false;
}
