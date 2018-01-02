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
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"

Demo::Demo()
{
	time = 0;
	mode = DEMOMODE_NONE;
	frame = 0;
}

void Demo::toggleRecord(bool on)
{
	if (on)
	{
		togglePlayback(false);
		frames.clear();
		mode = DEMOMODE_RECORD;
		time = 0;
		timeDiff = 0;
		frame = 0;
	}
	else
	{
		mode = DEMOMODE_NONE;
	}
}

void Demo::togglePlayback(bool on)
{
	if (on)
	{
		core->updateMouse = false;
		mode = DEMOMODE_PLAYBACK;
		time = 0;
		timeDiff = 0;
		frame = 0;
	}
	else
	{
		core->frameOutputMode = false;
		core->updateMouse = true;
		mode = DEMOMODE_NONE;
	}
}

void Demo::renderFramesToDisk()
{
	core->frameOutputMode = true;
	togglePlayback(true);
}

void Demo::save(const std::string &name)
{
	togglePlayback(false);
	toggleRecord(false);

	std::string filename = "" + name + ".demo";



}

void Demo::load(const std::string &name)
{
	togglePlayback(false);
	toggleRecord(false);

	// UNFINISHED
	std::string filename = "" + name + ".demo";



}

void Demo::clearRecordedFrames()
{
	frames.clear();
}

bool Demo::getQuitKey()
{
	return core->getKeyState(KEY_ESCAPE) && core->getShiftState();
}

void Demo::update(float dt)
{
	if (core->getNestedMains() > 1) return;

	if (mode == DEMOMODE_RECORD)
	{
		DemoFrame f;
		f.avatarPos = dsq->game->avatar->position;
		f.vel = dsq->game->avatar->vel;
		f.vel2 = dsq->game->avatar->vel2;
		f.rot = dsq->game->avatar->rotation.z;

		f.mouse = core->mouse;
		f.t = time;

		frames.push_back(f);


		time += dt;

		if (getQuitKey())
		{
			toggleRecord(false);
			dsq->centerMessage(stringbank.get(2010));
		}
	}
	else if (mode == DEMOMODE_PLAYBACK)
	{
		while (frame < frames.size())
		{
			DemoFrame *f = &frames[frame];
			if (f->t <= time) {
				// temporarily deactivate for seahorse footage

				dsq->game->avatar->vel = f->vel;
				dsq->game->avatar->vel2 = f->vel2;
				dsq->game->avatar->rotation.z = f->rot;
				dsq->game->avatar->position = f->avatarPos;


				core->mouse = f->mouse;

				frame++;
			}
			else
			{
				break;
			}


		}
		time += dt;


		if (getQuitKey() || (!frames.empty() && frame >= frames.size())) {
			togglePlayback(false);
			dsq->centerMessage(stringbank.get(2011));
		}


	}
}

