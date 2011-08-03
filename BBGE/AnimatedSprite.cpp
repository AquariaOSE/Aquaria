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
#include "AnimatedSprite.h"

AnimData::AnimData()
{
	frameStart = frameEnd = 0;
	time = 0;
	loop = 0;
	pingPong = false;
	datafile = 0;
}

AnimatedSprite::AnimatedSprite() : Quad()
{
	//debugLog("AnimatedSprite::AnimatedSprite()");
	frame = 0;
	animator.setUpdatee(&frame);
	animationTime.setUpdatee (&animator.timePeriod);
	//debugLog("End AnimatedSprite::AnimatedSprite()");
}

void AnimatedSprite::animate (AnimData &animData)
{
	onAnimData(animData);
	animate (animData.name, animData.frameStart, animData.frameEnd, animData.time, animData.loop, animData.pingPong);
}

void AnimatedSprite::onAnimData(AnimData &animData)
{
}

void AnimatedSprite::animate (const std::string &name, int from, int to, float time, int loopType, bool pingPong, float initialDelay)
{
	if (from == to && to == 0)
	{
		debugLog ("null animation");
		animator.stop();
		return;
	}
	currentAnim = name;
	frame = from;
	animator.interpolateTo (to, time, loopType);
	animator.pingPong = pingPong;
	animator.initialDelay = initialDelay;
}

void AnimatedSprite::onUpdate (float dt)
{
	Quad::onUpdate (dt);
	animator.update (dt);
	animationTime.update (dt);
	if (!animator.interpolating && currentAnim != "")
	{
		animComplete(currentAnim);
		lastAnim = currentAnim;
		currentAnim = "";
	}
}


