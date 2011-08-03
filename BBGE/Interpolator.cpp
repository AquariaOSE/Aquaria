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
#include "Interpolator.h"

void Interpolator::stop ()
{
	interpolating = false;
}

void Interpolator::setUpdatee (float *u) 
{ 
	updatee = u; 
}

void Interpolator::interpolateTo (float interTo, float time, int ltype)
{
	if (!updatee) 
	{
		errorLog ("No updatee set for interpolator!"); 
		return;
	}

	loopType = ltype;
	to = interTo;
	timePeriod = time;
	from = *updatee;
	interpolating = true;
	timePassed = 0.0f;
	fakeTimePassed = 0.0f;
	useSpeed = false;
}

void Interpolator::interpolateBySpeed (float interTo, float speed, int ltype)
{
	this->speed = speed;
	loopType = ltype;
	to = interTo;
	from = *updatee;
	timePassed = 0.0f;
	useSpeed = false;
	timePeriod = fabsf(to-from) / speed;

	interpolating = true;
}

void Interpolator::setSpeed (float s)
{
	useSpeed = true;
	interpolating = true;
	speed = s;
}

void Interpolator::update (float dt)
{

	if (interpolating)
	{
		if (initialDelay > 0)
		{
			initialDelay -= dt;
		}
		else
		{
		if (!useSpeed)
		{
    		timePassed += dt*timeMultiplier;
 			if (timePassed >= timePeriod)
    		{
    			*updatee = to;
    			interpolating = false;
    			if (loopType != 0)
    			{
    				if (loopType > 0)
    					loopType -= 1;
   					if (pingPong)
     				 interpolateTo (from, timePeriod, loopType);
   				    else
			        {
	                 *updatee = from;
	                 interpolateTo (to, timePeriod, loopType);
			        }
    			}
    		}
    		else
    		{
    			*updatee = (to - from) * (timePassed / timePeriod);
    			*updatee += from;
    		}
		}
		else
		{
			*updatee += speed * dt;
			if(*updatee > 255) *updatee = 0;
			if(*updatee < 0) *updatee = 255;
			/*
			timePassed += speed *dt;

 			if (timePassed >= timePeriod)
    		{
    			*updatee = to;
    			interpolating = false;
    			if (loopType != 0)
    			{
    				if (loopType > 0)
    					loopType -= 1;
   					if (pingPong)
     				 interpolateTo (from, timePeriod, loopType);
   				    else
			        {
	                 *updatee = from;
	                 interpolateTo (to, timePeriod, loopType);
			        }
    			}
    		}
    		else
    		{
    			*updatee = (to - from) * (timePassed / timePeriod);
    			*updatee += from;
    		}
			*/
			/*
			if (updatee >= to)
			{
				if (pingPong)
				{
					interpolateTo (from, timePeriod, loopType);
					//if(*updatee > to) *updatee = 0;
					//if(*updatee < from) *updatee = 255;
				}
				else
				{
					*updatee = from;
					interpolateTo (to, timePeriod, loopType);
				}
			}
			*/
		}
		}
	}
}



