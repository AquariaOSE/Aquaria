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

SFXLoops::SFXLoops()
{
	bg = bg2 = roll = shield = charge = current = trip = BBGE_AUDIO_NOCHANNEL;
}

void SFXLoops::updateVolume()
{
	core->sound->updateChannelVolume(bg);
	core->sound->updateChannelVolume(bg2);
	core->sound->updateChannelVolume(roll);
	core->sound->updateChannelVolume(charge);
	core->sound->updateChannelVolume(shield);
	core->sound->updateChannelVolume(current);
	core->sound->updateChannelVolume(trip);
}

void stopLoop(void **loop)
{
	if (*loop != BBGE_AUDIO_NOCHANNEL)
	{
		core->sound->fadeSfx(*loop, SFT_OUT, 0.25);
		*loop = BBGE_AUDIO_NOCHANNEL;
	}
}

void SFXLoops::stopAll()
{
	stopLoop(&bg);
	stopLoop(&bg2);
	stopLoop(&roll);
	stopLoop(&charge);
	stopLoop(&shield);
	stopLoop(&current);
	stopLoop(&trip);
}
