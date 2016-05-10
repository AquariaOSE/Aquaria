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
#ifndef __animated_sprite__
#define __animated_sprite__

#include "Quad.h"
#include "Interpolator.h"

class AnimData
{
public:
	AnimData();
	std::string name;
	int frameStart, frameEnd, datafile;
	float time;
	int loop;
	bool pingPong;
};


class AnimatedSprite : public Quad
{
public:
	AnimatedSprite();

	Interpolator animator, animationTime;
	float frame;
	std::string currentAnim, lastAnim;

	virtual void animComplete(std::string name) {}

	void animate (const std::string &name, int from, int to, float time, int loopType, bool pingPong = false, float initialDelay = 0.0f);
	void animate (AnimData &animData);

	bool isAnimating()
	{
		return animator.interpolating;
	}



protected:
	virtual void onAnimData(AnimData &animData);
	virtual void onUpdate (float dt);

};

#endif


