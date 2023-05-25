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
#ifndef GRADIENT_H
#define GRADIENT_H

#include "RenderObject.h"


class Gradient : public RenderObject
{
public:
	Gradient();
	void makeVertical(Vector c1, Vector c2);
	void makeHorizontal(Vector c1, Vector c2);

	void onUpdate(float dt) OVERRIDE;

	int autoWidth, autoHeight;
protected:
	void onRender(const RenderState& rs) const OVERRIDE;
	Vector ulc0, ulc1, ulc2, ulc3;
};

#endif
