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
#ifndef AQ_WEB_H
#define AQ_WEB_H

#include "../BBGE/Quad.h"
#include "Entity.h"

class Web : public RenderObject
{
public:
	Web();
	int addPoint(const Vector &point = Vector(0,0));
	void setPoint(int pt, const Vector &v);
	Vector getPoint(int pt) const;
	void setParentEntity(Entity *e);
	int getNumPoints();
	typedef std::list<Web*> Webs;
	static Webs webs;
	static void killAllWebs();
	void setExistence(float e);
protected:
	float existence;
	Entity *parentEntity;
	void onEndOfLife();
	std::vector<Vector> points;
	void onUpdate(float dt);
	void onRender();
};

#endif
