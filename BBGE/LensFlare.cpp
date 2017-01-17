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
#include "LensFlare.h"
#include "Core.h"

LensFlare::LensFlare()
{
	cull = false;
	inc = 0.5;
	maxLen = 1500;
}

void LensFlare::addFlare(const std::string &tex, Vector color, int w, int h)
{
	Quad *q = new Quad(tex, Vector(0,0,0));
	q->color = color;
	q->setBlendType(BLEND_ADD);
	if (w != -1)
		q->setWidth(w);
	if (h != -1)
		q->setHeight(h);
	flares.push_back(q);
	addChild(q, PM_POINTER);
}

void LensFlare::onUpdate(float dt)
{

	RenderObject::onUpdate(dt);
	Vector v = core->screenCenter - this->position;
	if (v.getSquaredLength2D() > sqr(maxLen))
		return;
	else
	{
		float l = v.getLength2D();
		float a = 1.0f-(l/(float)maxLen);
		a*=0.8f;

		Vector vbit = v;
		vbit *= inc;
		for (size_t i = 0; i < flares.size(); i++)
		{
			flares[i]->position = vbit*i;
			flares[i]->alpha = a;
		}
	}
}
