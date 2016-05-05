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
#include "States.h"

StarRenderer::StarRenderer(int num, int range) : RenderObject()
{
	stars.resize(num);
	int rh = range/2;
	for (int i = 0; i < stars.size(); i++)
	{
		stars[i] = Vector((rand()%range)-rh, (rand()%range)-rh, (rand()%range)-rh);
	}
	//rotation.interpolateTo(Vector(0,360,0), 30, -1);
	//position.interpolateTo(Vector(0,0,-100), 10, -1);
}

void StarRenderer::render()
{
	core->enable3D();
	glLoadIdentity();
	glTranslatef(position.x, position.y, position.z);
	glRotatef(rotation.x, 1, 0, 0);
	glRotatef(rotation.y, 0, 1, 0);
	glRotatef(rotation.z, 0, 0, 1);
	glPointSize(1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_POINTS);
	for (int i = 0; i < stars.size(); i++)
	{
		float c = 1.0f-fabsf(stars[i].z)/50.0f;
		glColor3f(c,c,c);
		glVertex3f(stars[i].x, stars[i].y, stars[i].z);
	}
	glEnd();

	core->enable2D(core->getVirtualWidth());
	/*
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	*/
}

void StarRenderer::onRender()
{

}

