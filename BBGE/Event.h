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
#ifndef BBGE_EVENT_H
#define BBGE_EVENT_H

#include <stdlib.h>

#include <list>


class Event
{
public:
	Event() { data = 0; }
	virtual ~Event() {}
	virtual void act()=0;
	void *data;
};


template <class T>
class FunctionEvent : public Event
{
public:
	FunctionEvent(T *who,void (T::*func)(void)):
		callee(who),callback(func){}

	void act()
	{
		(callee->*callback)();
	}

private:
	T *callee;
	void (T::*callback)(void);
};

template <class T>
class FunctionEventWithData : public Event
{
public:
	FunctionEventWithData(T *who,void (T::*func)(Event*)):
		callee(who),callback(func){}

	void act()
	{
		(callee->*callback)(this);
	}

private:
	T *callee;
	void (T::*callback)(Event *e);
};

#define EVENT(x,y) class x : public Event { public: void act(); }; x y;

#define MakeFunctionEvent(type,function) addCreatedEvent(new FunctionEvent<type>(this, &type::function))
#define MakeFunctionEventPointer(type,function,pointer) addCreatedEvent(new FunctionEvent<type>(pointer, &type::function))
#define FuncEvent(type,function) addCreatedEvent(new FunctionEventWithData<type>(this, &type::function))

enum EventManage
{
	EVM_NONE	= 0,
	EVM_CLEANUP	= 1,
	EVM_MAX
};

class ActionMapper;

class EventPtr
{
public:
	EventPtr();
	~EventPtr();
	void call();
	void set(Event *e, EventManage _evm=EVM_NONE);
	const EventPtr& operator= (const EventPtr &e);
	void clean();
	void setActionMapperCallback(ActionMapper *actionMapperCallback, int actionValue, int stateToCall=0);
protected:
	Event *e;
	unsigned char _evm;  // Keep the structure size down.
	inline EventManage evm() const {return EventManage(_evm);}

	bool stateToCall;

	ActionMapper *actionMapperCallback;
	int actionValue;
};


class Timer
{
public:
	Timer();
	void startStopWatch();
	void start(float t);
	void stop();
	bool isDone();
	bool isActive();
	void update(float dt);
	bool updateCheck(float dt);
	float getValue();
	float getPerc();
	EventPtr endEvent, startEvent;
protected:
	int running;
	float timer, time;
};

class EventPulser
{
public:
	EventPulser();

	void setInterval(float t);

	void update(float dt);

	float interval;
	float time;

	int times;

	int randomVariance;

	EventPtr e;
};

class EventTimer
{
public:
	EventTimer(const EventPtr &p, float time);

	void update(float dt);

	EventPtr eventPtr;

	Timer timer;
};

class EventQueue
{
public:
	EventQueue();
	void addEvent(const EventPtr &eventPtr, float t);
	void update(float dt);
	void clear();
	int getSize();

private:
	typedef std::list<EventTimer> EventTimers;
	EventTimers eventTimers;
};

#endif
