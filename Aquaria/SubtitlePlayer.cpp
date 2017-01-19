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

#include "SubtitlePlayer.h"
#include "../BBGE/DebugFont.h"
#include "../BBGE/BitmapFont.h"
#include "DSQ.h"
#include "ttvfs_stdio.h"


SubtitlePlayer::SubtitlePlayer()
{
	curLine = 0;
	vis = false;
	hidden = false;
}

bool SubtitlePlayer::isVisible()
{
	return vis;
}

void SubtitlePlayer::go(const std::string &subs)
{
	subLines.clear();

	std::string f;
	bool checkAfter = true;
	if (dsq->mod.isActive())
	{
		f = dsq->mod.getPath() + "audio/" + subs + ".txt";
		f = localisePath(f, dsq->mod.getPath());
		f = adjustFilenameCase(f);
		if (exists(f))
			checkAfter = false;
	}

	if (checkAfter)
	{
		f = "scripts/vox/" + subs + ".txt";
		f = localisePath(f);
		f = adjustFilenameCase(f);
		if (!exists(f))
		{
			debugLog("Could not find subs file [" + subs + "]");
		}
	}

	InStream in(f.c_str());
	std::string line;
	while (std::getline(in, line))
	{
		SubLine sline;
		const char *s = line.c_str();
		int minutes = (int)strtol(s, const_cast<char **>(&s), 10);
		if (*s == ':')
		{
			float seconds = strtof(s+1, const_cast<char **>(&s));
			s += strspn(s, " \t");
			sline.line.assign(s);
			sline.timeStamp = minutes*60 + seconds;
			subLines.push_back(sline);
		}
	}
	curLine = 0;
}

void SubtitlePlayer::end()
{
	dsq->subtext->alpha.interpolateTo(0, 1.0f);
	dsq->subbox->alpha.interpolateTo(0, 1.2f);
	vis = false;
}

void SubtitlePlayer::hide(float t)
{
	if (vis && !hidden)
	{
		dsq->subtext->alpha.interpolateTo(0, t/1.2f);
		dsq->subbox->alpha.interpolateTo(0, t);
	}
	hidden = true;
}

void SubtitlePlayer::show(float t)
{
	if (vis && hidden)
	{
		dsq->subtext->alpha.interpolateTo(1, t/1.2f);
		dsq->subbox->alpha.interpolateTo(1, t);
	}
	hidden = false;
}

void SubtitlePlayer::update(float dt)
{
	dt = core->get_old_dt();

	float time = dsq->sound->getVoiceTime();

	if (dsq->subtext && dsq->subbox)
	{
		dsq->subtext->useOldDT = true;
		dsq->subbox->useOldDT = true;
	}

#ifndef DISABLE_SUBS
	if (curLine < subLines.size())
	{
		if (subLines[curLine].timeStamp <= time)
		{
			// present line
			// set text
			debugLog(subLines[curLine].line);
			dsq->subtext->scrollText(subLines[curLine].line, 0.02f);

			// advance
			curLine++;
		}

		if (!vis)
		{
			if (!hidden)
			{
				dsq->subtext->alpha.interpolateTo(1, 1);
				dsq->subbox->alpha.interpolateTo(1, 0.1f);
			}
			vis = true;
		}
	}
#endif

	if (vis && !sound->isPlayingVoice())
	{
		end();
	}
}

