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
#ifndef _statemachine_
#define _statemachine_

#include "Base.h"

class StateMachine
{
public:
	StateMachine ();
	virtual ~StateMachine() {}

	void setState(int state, float time = -1, bool force = false);
	void stopState(int state);
	bool isState(int state);
	int getState();
	int getPrevState();
	int getEnqueuedState() { return enqueuedState; }
	float getStateTime();
	void setStateTime(float t);
	float getStateCounter();
	enum
	{
		STATE_NONE = -1
	};
	virtual bool canSetState(int state);

protected:
	void perform(int state, float time = -1);
	virtual void onEnterState(int state);
	virtual void onExitState(int state);

	int currentState, nextState, prevState, enqueuedState;
	float stateTime, enqueuedTime, stateExtraDT;

	void onUpdate (float dt);
	void resetStateCounter()
	{ stateCounter = 0; }

private:
	float stateCounter;

};

#endif

