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
#include "DSQ.h"
#include "ttvfs_stdio.h"

GameplayVariables *vars = 0;

void GameplayVariables::load()
{
	InStream inFile("data/variables.txt");
	if(!inFile)
	{
		exit_error(stringbank.get(2017));
	}
	std::string s;
	inFile >> s >> maxSlowSwimSpeed;
	inFile >> s >> maxSwimSpeed;
	inFile >> s >> maxBurstSpeed;
	inFile >> s >> maxDodgeSpeed;
	inFile >> s >> maxWallJumpSpeed;
	inFile >> s >> maxWallJumpBurstSpeed;
	inFile >> s >> maxDreamWorldSpeed;
	inFile >> s >> jumpVelocityMod;
	inFile >> s >> zoomStop;
	inFile >> s >> zoomMove;
	inFile >> s >> autoSaveTime;
	inFile >> s >> autoSaveFiles;
	inFile >> s >> afterEffectsXDivs;
	inFile >> s >> afterEffectsYDivs;
	inFile >> s >> unusedFPSSmoothing;
	inFile >> s >> frictionForce;
	inFile >> s >> maxSpringSpeed;
	inFile >> s >> springTime;
	inFile >> s >> grabSpringPlantVelCap;
	inFile >> s >> dodgeTime;
	inFile >> s >> entityDamageTime;
	inFile >> s >> pushTime;
	inFile >> s >> avatarDamageTime;
	inFile >> s >> zoomNaija;
	inFile >> s >> maxOutOfWaterSpeed;
	inFile >> s >> defaultCameraLerpDelay;

	inFile.close();
}

