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
#include "Slider.h"
#include "Core.h"

Slider::Slider(int len, int grabRadius, const std::string &bgQuad, const std::string &sliderGfx) : RenderObject()
{
	value = 0;
	inSlider = false;
	sliderLength = len;
	this->grabRadius = grabRadius;

	bg.setTexture(bgQuad);
	addChild(&bg, PM_STATIC);

	slider.setTexture(sliderGfx);
	addChild(&slider, PM_STATIC);

	// HACK: ish
	followCamera = 1;
	off = 0;

	wasDown = wasDown2 = false;

	shareAlphaWithChildren = 1;
}

void Slider::setValue(float v)
{
	value = v;

	if (value < 0)
		value = 0;
	if (value > 1)
		value = 1;

	slider.position.x = (value * sliderLength) - sliderLength/2.0f;
}

float Slider::getValue()
{
	return value;
}

bool Slider::isGrabbed()
{
	return inSlider;
}

void Slider::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (alpha.x != 1) return;

	bool b = (core->mouse.buttons.left || core->mouse.buttons.right) && fabsf(core->mouse.position.y-position.y) < grabRadius;
	if (!b && inSlider)
		inSlider = false;

	if ((core->mouse.position - this->position).isLength2DIn(5))
	{
		if (!wasDown && core->mouse.buttons.left)
			wasDown = true;
		else if (wasDown && !core->mouse.buttons.left)
		{
			wasDown = false;
			setValue(value - 0.1f);
		}
		if (!wasDown2 && core->mouse.buttons.right)
			wasDown2 = true;
		else if (wasDown2 && !core->mouse.buttons.right)
		{
			wasDown2 = false;
			setValue(value + 0.1f);
		}
	}
	else
	{
		wasDown = false;
		wasDown2 = false;
	}

	if (!inSlider)
	{
		inSlider = (b && (core->mouse.position - slider.getWorldPosition()).isLength2DIn(grabRadius));
		if (inSlider)
		{
			off = core->mouse.position.x - slider.getWorldPosition().x;
			off = -off;
		}
	}

	if (inSlider)
	{
		slider.position.x = core->mouse.position.x - position.x + off;

		int w2 = sliderLength/2;
		if (slider.position.x > w2)
		{
			slider.position.x = w2;
		}
		else if (slider.position.x < -w2)
		{
			slider.position.x = -w2;
		}
		value = float(slider.position.x + w2)/float(sliderLength);
	}
}

CheckBox::CheckBox(int clickRadius, const std::string &bgQuad, const std::string &fgQuad, const std::string &sfx) : RenderObject()
{
	this->sfx = sfx;
	this->clickRadius = clickRadius;

	bg.setTexture(bgQuad);
	addChild(&bg, PM_STATIC);

	fg.setTexture(fgQuad);
	addChild(&fg, PM_STATIC);

	// HACK: ish
	followCamera = 1;

	wasDown = false;
}

void CheckBox::setValue(int v)
{
	value = v;
	updateVisual(0);
}

void CheckBox::updateVisual(float t)
{
	if (value != 0)
	{
		fg.alpha.interpolateTo(1, t);
	}
	else
	{
		fg.alpha.interpolateTo(0, t);
	}
}

int CheckBox::getValue()
{
	return value;
}

void CheckBox::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
	fg.alphaMod = alpha.x;
	bg.alphaMod = alpha.x;
	if (alpha.x < 1) return;
	if ((core->mouse.position - getWorldPosition()).isLength2DIn(clickRadius))
	{
		if ((core->mouse.buttons.left || core->mouse.buttons.right))
		{
			if (!wasDown)
			{

				wasDown = true;
			}
		}
		else
		{
			if (wasDown)
			{

				wasDown = false;
				if (value !=0)
				{	value = 0;	}
				else
				{	value= 1;	}
				updateVisual(0);
				core->sound->playSfx(sfx);
			}
		}
	}
	else
	{
		wasDown = false;
	}
}

