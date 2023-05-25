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
#ifndef ROUNDEDRECT_H
#define ROUNDEDRECT_H

#include "RenderObject.h"
#include "Event.h"

class TTFText;
struct TTFFont;


class RoundedRect : public RenderObject
{
public:
	RoundedRect();
	void setWidthHeight(int w, int h, int radius);
	void show();
	void hide();
	void setCanMove(bool on);
	int getWidth() { return width; }
	int getHeight() { return height; }
	int getRadius() { return radius; }


protected:
	void onUpdate(float dt) OVERRIDE;
	void onRender(const RenderState& rs) const OVERRIDE;

	bool canMove;
	static RoundedRect *moving;
	int width, height, radius;
	Vector d;

};

class RoundButton : public RenderObject
{
public:
	RoundButton(const std::string &label, TTFFont *font);
	void setWidthHeight(int w, int h, int radius);

	EventPtr event;
protected:
	void onUpdate(float dt) OVERRIDE;
	void onRender(const RenderState& rs) const OVERRIDE;

	TTFText *label;
	int width, height, radius;
	bool mbd;
	bool noNested;
};

#endif
