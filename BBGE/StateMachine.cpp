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

void StateMachine::perform(int state, float time)
{
	//debugLog("in perform");
	stateExtraDT = 0;
	prevState = currentState;
	nextState = state;
	//debugLog("onExitState");

	// do this to prevent scripts from repeating endlessly when running main loops
	enqueuedState = STATE_NONE;

 	onExitState(currentState);
	stateCounter = 0;
	stateTime = time;
	currentState = state;
	nextState = STATE_NONE;
	//debugLog("onActionInit");
	onEnterState(currentState);

	//debugLog("done");
}

void StateMachine::setState(int state, float time, bool force)
{
	if (canSetState(state))
	{
		if (force)
		{
			perform(state, time);
		}
		else
		{
			enqueuedState = state;
			enqueuedTime = time;
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
		perform(enqueuedState, enqueuedTime);
		enqueuedState = STATE_NONE;
		enqueuedTime = -1;
	}
}

