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

#ifndef __BBGE_SLIDER__
#define __BBGE_SLIDER__

#include "Quad.h"

class Slider : public RenderObject
{
public:
	Slider(int len, int grabRadius, const std::string &bgQuad, const std::string &sliderGfx);
	void setValue(float v);
	float getValue();
	bool isGrabbed();
protected:
	bool wasDown, wasDown2;
	int off;
	Quad slider;
	Quad bg;
	int grabRadius;
	void onUpdate(float dt);
	float value;
	bool inSlider;
	float sliderLength;
};

class CheckBox : public RenderObject
{
public:
	CheckBox(int clickRadius, const std::string &bgQuad, const std::string &fgQuad, const std::string &sfx="");
	void setValue(int v);
	int getValue();
protected:
	std::string sfx;
	void updateVisual(float t=0);
	bool wasDown;
	int value, clickRadius;
	Quad fg, bg;
	void onUpdate(float dt);
};

#endif
