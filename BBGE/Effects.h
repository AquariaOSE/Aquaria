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

#include "Vector.h"
#include "Base.h"

enum FXTypes
{
	FTX_NONE		=-1,
	FXT_RADIALBLUR	=0,
	FXT_MAX
};

class PostProcessingFX
{
public:
	PostProcessingFX();
	void init();
	void update(float dt);
	void preRender();
	void render();
	void toggle(FXTypes type);
	void enable(FXTypes type);
	void disable(FXTypes type);
	bool isEnabled(FXTypes type);

	// blur
	Vector radialBlurColor;
	int blurTimes;

	/// misc
	int renderLayer;
	int layer;
	float intensity;
	int blendType;

protected:
	bool enabled[FXT_MAX];
};


