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
#include "Event.h"
#include "ActionMapper.h"


EventPtr::EventPtr() : e(0)
{
	this->_evm = EVM_NONE;

	this->actionMapperCallback = 0;
	this->actionValue = 0;
	this->stateToCall = 0;
}

EventPtr::~EventPtr()
{
}

void EventPtr::clean()
{
	// check to see if we have a pointer + want to clean up
	// if so, do so!
	if (e && evm()==EVM_CLEANUP)
	{
		delete e;
		e=0;
	}
}

const EventPtr& EventPtr::operator= (const EventPtr &e)
{
	this->e = e.e;
	this->_evm = e._evm;

	return *this;
}

void EventPtr::call()
{
	if (e) e->act();
	if (e && evm()==EVM_CLEANUP)
	{
		delete e;
		e=0;
	}
	if (actionMapperCallback)
	{
		actionMapperCallback->action(actionValue, stateToCall, -1, INPUT_NODEVICE);
	}
}

void EventPtr::set(Event *e, EventManage _evm)
{
	this->e = e;
	this->_evm = _evm;
}

void EventPtr::setActionMapperCallback(ActionMapper *actionMapperCallback, int actionValue, int stateToCall)
{
	this->actionMapperCallback = actionMapperCallback;
	this->actionValue = actionValue;
	this->stateToCall = stateToCall;
}

EventPulser::EventPulser()
{
	interval = 1;
	time = 1;
	randomVariance = 0;
	times = -1;
}

void EventPulser::setInterval(float t)
{
	interval = time = t;
}

void EventPulser::update(float dt)
{
	if (times == 0) return;
	time -= dt;
	if (time <= 0)
	{
		if (times != -1)
		{
			times --;
		}
		e.call();
		time = interval;
		if (randomVariance)
		{
			time += rand()%randomVariance;
		}
	}
}

EventTimer::EventTimer(const EventPtr &p, float time)
{
	eventPtr = p;
	timer.start(time);
}

void EventTimer::update(float dt)
{
	if (timer.updateCheck(dt))
	{
		eventPtr.call();
	}
}


EventQueue::EventQueue()
{
}

void EventQueue::addEvent(const EventPtr &eventPtr, float t)
{
	eventTimers.push_back(EventTimer(eventPtr, t));
}

void EventQueue::update(float dt)
{
	for (EventTimers::iterator i = eventTimers.begin(); i != eventTimers.end();)
	{
		i->update(dt);
		if (i->timer.isDone())
		{
			i = eventTimers.erase(i);
		}
		else
		{
			i++;
		}
	}
}

void EventQueue::clear()
{
	for (EventTimers::iterator i = eventTimers.begin(); i != eventTimers.end(); i++)
	{
		i->eventPtr.clean();
	}
	eventTimers.clear();
}

int EventQueue::getSize()
{
	return eventTimers.size();
}

Timer::Timer()
{
	timer = time = 0;
	running = 0;
}

float Timer::getPerc()
{
	if (timer <= 0 || time <= 0) return 0;
	return timer/time;
}

void Timer::start(float t)
{
	timer = t;
	time = t;
	running = 1;
}

void Timer::stop()
{
	timer = 0;
	running = 0;
}

void Timer::startStopWatch()
{
	timer = 0;
	running = 2;
}

bool Timer::isDone()
{
	return running == 0;
}

bool Timer::isActive()
{
	return running != 0;
}

void Timer::update(float dt)
{
	switch (running)
	{
	case 1:
		timer -= dt;
		if (timer < 0)
		{
			timer = 0;
			running = 0;
		}
	break;
	case 2:
		timer += dt;
	break;
	}
}

bool Timer::updateCheck(float dt)
{
	int wasRunning = running;
	update(dt);
	if (wasRunning && !running)
		return true;
	return false;
}

float Timer::getValue()
{
	return timer;
}


