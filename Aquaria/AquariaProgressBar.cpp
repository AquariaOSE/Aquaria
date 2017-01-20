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
#include "AquariaProgressBar.h"
#include "../BBGE/Core.h"


AquariaProgressBar::AquariaProgressBar() : RenderObject()
{
	perc = 0;
	spinner.setTexture("Progress");
	spinner.alphaMod = 0.1f;
	addChild(&spinner, PM_STATIC);
	followCamera = 1;
}

void AquariaProgressBar::progress(float addPerc)
{
	if (addPerc==0)
		addPerc = 0.01f;
	this->perc += addPerc;
	spinner.rotation = Vector(0,0,this->perc*360);
	core->render();
	core->showBuffer();
}

