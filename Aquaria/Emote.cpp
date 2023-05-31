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
#include "ttvfs_stdio.h"

Emote::Emote()
{
	// blah
	emoteTimer = 0;
	lastVariation = -1;
}

void Emote::load(const std::string &file)
{
	emotes.clear();
	InStream in(file.c_str());
	std::string line;

	while (std::getline(in, line))
	{
		std::istringstream is(line);
		EmoteData e;
		is >> e.index >> e.name >> e.variations;
		emotes.push_back(e);
	}
	emoteTimer = 0;
}

void Emote::playSfx(size_t index)
{
	if (index >= emotes.size())	return;
	if (emoteTimer > 0)							return;

	int r = 0;

	if (emotes[index].variations > 1)
	{
		r = (rand()%emotes[index].variations)+1;
		if (r == lastVariation)
		{
			r++;
			if (r > emotes[index].variations)
			{
				r = 1;
			}
		}
	}

	std::ostringstream os;
	os << emotes[index].name << r;

	lastVariation = r;

	PlaySfx play;
	play.name = os.str();

	dsq->sound->playSfx(play);
	emoteTimer = 0.5;
}

void Emote::update(float dt)
{
	if (!game->isPaused())
	{
		if (emoteTimer > 0)
		{
			emoteTimer -= dt;
			if (emoteTimer <= 0)
			{
				emoteTimer = 0;
			}
		}
	}
}
