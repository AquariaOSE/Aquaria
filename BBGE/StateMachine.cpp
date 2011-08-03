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
#include "StateMachine.h"

StateMachine::StateMachine ()
{
	//debugLog("StateMachine::StateMachine()");
	stateExtraDT = 0;
	enqueuedTime = stateTime = -1;
	enqueuedState = nextState = prevState = currentState = STATE_NONE;

	stateCounter = 0;
	currentStateData = enqueuedStateData = 0;
}

int StateMachine::getState()
{
	return currentState;
}

bool StateMachine::isState(int state)
{
	return currentState == state;
}

int StateMachine::getPrevState()
{
	return prevState;
}
  
void StateMachine::perform(int state, float time, void *stateData)
{
	//debugLog("in perform");
	stateExtraDT = 0;
	prevState = currentState;
	nextState = state;
	//debugLog("onExitState");
	
	// do this to prevent scripts from repeating endlessly when running main loops
	enqueuedState = STATE_NONE;
	enqueuedStateData = 0;

 	onExitState(currentState);
	stateCounter = 0;
	stateTime = time;
	currentState = state;
	nextState = STATE_NONE;
	this->currentStateData = stateData;
	//debugLog("onActionInit");
	onEnterState(currentState);
	
	//debugLog("done");
}	 

void StateMachine::setState(int state, float time, bool force, void *stateData)
{
	if (canSetState(state))
	{
		if (force)
		{
			perform(state, time, stateData);
		}
		else
		{
			enqueuedState = state;
			enqueuedTime = time;
			enqueuedStateData = stateData;
		}
	}
}

bool StateMachine::canSetState(int state)
{
	return true;
}

void StateMachine::onEnterState(int state)
{
}

void StateMachine::onExitState(int state)
{
}
 
void StateMachine::stopState(int state)
{
	onExitState(state);
	currentState = STATE_NONE;
}

float StateMachine::getStateCounter()
{
	return stateCounter;
}

float StateMachine::getStateTime()
{
	return stateTime;
}

void StateMachine::setStateTime(float t)
{
	stateTime = t;
}

void StateMachine::onUpdate (float dt)
{
	if (currentState != STATE_NONE)
	{
		if (stateTime != -1)
		{
			stateCounter += dt;
			if (stateCounter >= stateTime)
			{
				stateExtraDT = stateCounter - stateTime;
				stateCounter = stateTime;
				onExitState (currentState);
				currentState = -1;
				stateExtraDT = 0;
			}
		}
	}
	if (enqueuedState != STATE_NONE)
	{
		perform(enqueuedState, enqueuedTime, enqueuedStateData);
		enqueuedState = STATE_NONE;
		enqueuedTime = -1;
	}
}

void StateMachine::addCooldown(int state, float time)
{
	Cooldown c;
	c.state = state;
	c.timer.start(time);
	cooldowns.push_back(c);
}

bool StateMachine::isCooldown(int state)
{
	for (Cooldowns::iterator i = cooldowns.begin(); i != cooldowns.end(); i++)
	{
		if ((*i).state == state && (*i).timer.isActive())
		{
			return true;
		}
	}
	return false;
}

void StateMachine::removeCooldown(int state)
{
	for (Cooldowns::iterator i = cooldowns.begin(); i != cooldowns.end(); i++)
	{
		if ((*i).state == state)
		{
			cooldowns.erase(i);
			break;
		}
	}
}

void StateMachine::updateCooldowns(float dt)
{
	std::queue<int> coolqueue;

	for (Cooldowns::iterator i = cooldowns.begin(); i != cooldowns.end(); i++)
	{
		Cooldown *c = &((*i));
		if (c->timer.updateCheck(dt)) {
			coolqueue.push(c->state);
		}
	}

	while (!coolqueue.empty())
	{
		removeCooldown(coolqueue.back());
		coolqueue.pop();
	}
}

void StateMachine::clearCooldowns()
{
	cooldowns.clear();
}
