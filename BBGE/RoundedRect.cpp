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
#include "RoundedRect.h"
#include "Core.h"
#include "TTFFont.h"
#include "RenderBase.h"

#include <assert.h>

RoundedRect *RoundedRect::moving=0;

RoundedRect::RoundedRect() : RenderObject()
{
	alphaMod = 0.75;
	color = 0;

	width = 100;
	height = 100;

	radius = 20;

	cull = false;

	canMove = false;
	moving = 0;

	followCamera = 1;
}

void RoundedRect::setWidthHeight(int w, int h, int radius)
{
	this->radius = radius;
	width = w-radius*2;
	height = h-radius*2;
}

void RoundedRect::setCanMove(bool on)
{
	canMove = on;
}

void RoundedRect::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
	if (canMove)
	{
		if (core->mouse.buttons.left)
		{
			if (moving == this)
			{
				position = core->mouse.position + d;

				if (position.x + ((width/2)+radius) > (core->getVirtualWidth() - core->getVirtualOffX()))
					position.x = (core->getVirtualWidth()- core->getVirtualOffX()) - ((width/2)+radius);
				if (position.y + ((height/2)+radius) > core->getVirtualHeight())
					position.y = core->getVirtualHeight() - ((height/2)+radius);
				if (position.x - ((width/2)+radius) < 0 - core->getVirtualOffX())
					position.x = -core->getVirtualOffX() + ((width/2)+radius);
				if (position.y - ((height/2)+radius) < 0 - core->getVirtualOffY())
					position.y = -core->getVirtualOffY() + ((height/2)+radius);
			}
			else if (moving == 0)
			{
				Vector p = core->mouse.position;
				if ((p.x >= (position.x - (width/2 + radius))) && (p.y >= (position.y - (height/2 + radius)))
					&& (p.x <= (position.x + (width/2 + radius)) && (p.y <= (position.y - height/2))))
				{
					d = position - core->mouse.position;
					moving = this;
				}
			}
		}
		else
		{
			if (moving == this)
			{


				moving = 0;
			}

		}
	}
}

void RoundedRect::onRender(const RenderState& rs) const
{

	int w2 = width/2;
	int h2 = height/2;

	float iter = 0.1f;

	glBegin(GL_QUADS);
	for (float angle = 0; angle < PI_HALF - iter; angle+=iter)
	{
		// top right
		{
			float x1 = sinf(angle)*radius, y1 = -cosf(angle)*radius;
			float x2 = sinf(angle+iter)*radius, y2 = -cosf(angle+iter)*radius;
			glVertex3f(w2 + x1, -h2 + y1, 0);
			glVertex3f(w2 + x2, -h2 + y2, 0);
			glVertex3f(w2 + x2, -h2 + 0, 0);
			glVertex3f(w2 + x1, -h2 + 0, 0);
		}
		// top left
		{
			float x1 = -sinf(angle)*radius, y1 = -cosf(angle)*radius;
			float x2 = -sinf(angle+iter)*radius, y2 = -cosf(angle+iter)*radius;
			glVertex3f(-w2 + x1, -h2 + y1, 0);
			glVertex3f(-w2 + x2, -h2 + y2, 0);
			glVertex3f(-w2 + x2, -h2 + 0, 0);
			glVertex3f(-w2 + x1, -h2 + 0, 0);
		}
		{
			float x1 = sinf(angle)*radius, y1 = cosf(angle)*radius;
			float x2 = sinf(angle+iter)*radius, y2 = cosf(angle+iter)*radius;
			glVertex3f(w2 + x1, h2 + y1, 0);
			glVertex3f(w2 + x2, h2 + y2, 0);
			glVertex3f(w2 + x2, h2 + 0, 0);
			glVertex3f(w2 + x1, h2 + 0, 0);
		}
		{
			float x1 = -sinf(angle)*radius, y1 = cosf(angle)*radius;
			float x2 = -sinf(angle+iter)*radius, y2 = cosf(angle+iter)*radius;
			glVertex3f(-w2 + x1, h2 + y1, 0);
			glVertex3f(-w2 + x2, h2 + y2, 0);
			glVertex3f(-w2 + x2, h2 + 0, 0);
			glVertex3f(-w2 + x1, h2 + 0, 0);
		}
	}

	//middle, top, btm
	glVertex3f(-w2, -h2 - radius, 0);
	glVertex3f(w2, -h2 - radius, 0);
	glVertex3f(w2, h2 + radius, 0);
	glVertex3f(-w2, h2 + radius, 0);

	// left
	glVertex3f(-w2 - radius, -h2, 0);
	glVertex3f(-w2, -h2, 0);
	glVertex3f(-w2, h2, 0);
	glVertex3f(-w2 - radius, h2, 0);

	// right
	glVertex3f(w2 + radius, -h2, 0);
	glVertex3f(w2, -h2, 0);
	glVertex3f(w2, h2, 0);
	glVertex3f(w2 + radius, h2, 0);

	glEnd();


}

void RoundedRect::show()
{
	if (alpha.x == 0)
	{
		const float t = 0.1f;
		alpha = 0;
		alpha.interpolateTo(1, t);
		scale = Vector(0.5f, 0.5f);
		scale.interpolateTo(Vector(1,1), t);
	}
}

void RoundedRect::hide()
{
	const float t = 0.1f;
	alpha = 1.0;
	alpha.interpolateTo(0, t);
	scale = Vector(1, 1);
	scale.interpolateTo(Vector(0.5f,0.5f), t);
}



RoundButton::RoundButton(const std::string &labelText, TTFFont *font) : RenderObject()
{

	label = new TTFText(font);
	label->setAlign(ALIGN_CENTER);
	label->offset += Vector(0, 3);
	label->setText(labelText);
	addChild(label, PM_POINTER);
	width = 80;
	height = 20;

	mbd = false;

	noNested = true;
}

void RoundButton::setWidthHeight(int w, int h, int radius)
{
	width = w;
	height = h;
}

void RoundButton::onUpdate(float dt)
{
	if (noNested && core->isNested()) return;

	RenderObject::onUpdate(dt);

	RenderObject *top = getTopParent();
	if (alpha.x == 1 && top->alpha.x == 1)
	{
		Vector p = core->mouse.position;
		Vector c = getWorldPosition();
		int w2 = width/2;
		int h2 = height/2;
		if ((p.x > (c.x - w2)) && (p.x < (c.x + w2)) && (p.y > (c.y - h2)) && (p.y < (c.y + h2)))
		{
			if (core->mouse.buttons.left && !mbd)
			{
				mbd = true;
			}
			else if (!core->mouse.buttons.left && mbd)
			{
				mbd = false;

				event.call();
			}
		}
		else
		{
			mbd = false;
		}
		if (!core->mouse.buttons.left && mbd)
		{
			mbd = false;
		}
	}
	else
	{
		mbd = false;
	}
}

void RoundButton::onRender(const RenderState& rs) const
{
	int w2 = width/2, h2 = height/2;
	glLineWidth(1);

	glBegin(GL_LINES);
	glVertex3f(-w2, -h2, 0);
	glVertex3f(w2, -h2, 0);

	glVertex3f(w2, -h2, 0);
	glVertex3f(w2, h2, 0);

	glVertex3f(w2, h2, 0);
	glVertex3f(-w2, h2, 0);

	glVertex3f(-w2, h2, 0);
	glVertex3f(-w2, -h2, 0);
	glEnd();

	if (mbd)
	{
		glColor4f(1,1,1,0.5);
		glBegin(GL_QUADS);
		glVertex3f(-w2, h2, 0);
		glVertex3f(w2, h2, 0);
		glVertex3f(w2, -h2, 0);
		glVertex3f(-w2, -h2, 0);
		glEnd();
	}
}

