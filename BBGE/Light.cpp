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
#include "Light.h"

Light::Light()
{
	num = 0;
	enabled = false;
	ambient = Vector(0.0f,0.0f,0.0f);
	diffuse = Vector(1,1,1);
	position = Vector(0,0,0);
}

void Light::apply()
{
	int t = GL_LIGHT0;
	switch (num)
	{
	case 0: t=GL_LIGHT0; break;
	case 1: t=GL_LIGHT1; break;
	case 2: t=GL_LIGHT2; break;
	case 3: t=GL_LIGHT3; break;
	case 4: t=GL_LIGHT4; break;
	case 5: t=GL_LIGHT5; break;
	case 6: t=GL_LIGHT6; break;
	case 7: t=GL_LIGHT7; break;
	}
	if (enabled)
	{
		glEnable(t);
		float v[4];
		glLightfv(t, GL_AMBIENT, ambient.getv4(v, 1));
		glLightfv(t, GL_DIFFUSE, diffuse.getv4(v, 1));
		glLightfv(t, GL_POSITION, position.getv4(v, 0));
		
	}
	else
	{
		glDisable(t);
	}
}

void Light::update (float dt)
{
	diffuse.update(dt);
	ambient.update(dt);
	position.update(dt);
}
