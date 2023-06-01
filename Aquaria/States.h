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
#ifndef __title__
#define __title__

#include "StateManager.h"


class ParticleEffect;
class Quad;

class GameOver : public StateObject
{
public:
	GameOver();
	void applyState();
	void removeState();
	void update(float dt);

	void onClick();
	Quad *frame1, *frame2, *frame3;

	float timer;
};

class Intro2 : public StateObject
{
public:
	Intro2();
	void applyState();
	void removeState();
	void update(float dt);
	void skipIntro();
};

class BitBlotLogo : public StateObject
{
public:
	BitBlotLogo();
	void applyState();
	void removeState();
	void update(float dt);

	void doShortBitBlot();
	void getOut();

	bool watchQuit(float time);
private:
	void showSequence();
};

class Hair;

class ParticleEditor : public StateObject
{
public:
	ParticleEditor();
	void applyState();
	void removeState();
	ParticleEffect *emitter;
	void load();
	void start();
	void stop();
	void reload();
	void goToTitle();
	void update(float dt);
	void toggleHair();
	Quad *test;
protected:
	Hair *hair;
	std::string lastLoadedParticle;
};

class Credits : public StateObject
{
public:
	Credits();
	void applyState();
	void removeState();

	void update(float dt);
};

class Nag : public StateObject
{
public:
	Nag();
	void applyState();
	void removeState();

	void update(float dt);

	void onBuy();
	void onExit();

protected:
	int click;
	bool grab;
	bool hitBuy;
};

#endif

