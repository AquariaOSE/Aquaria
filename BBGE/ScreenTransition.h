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
#pragma once

#include "RenderObject.h"

class ScreenTransition : public RenderObject
{
public:
	ScreenTransition();
	void go(float time);
	virtual void capture();
	void transition(float time);
	void reloadDevice();
	void unloadDevice();
	bool isGoing();
protected:
	void createTexture();
	void destroyTexture();
	int textureWidth, textureHeight;
	int windowWidth, windowHeight;
	void onRender();
	float width, height;
	//void onUpdate(float dt);

#ifdef BBGE_BUILD_OPENGL
	GLuint screen_texture;
#endif
};

