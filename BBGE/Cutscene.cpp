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
#include "Cutscene.h"

Cutscene::Cutscene()
{
	time = playing = 0;
}

void Cutscene::play()
{
	time = 0;
	playing = 1;
}

void Cutscene::clear()
{
	for (IDMap::iterator i = idMap.begin(); i != idMap.end(); ++i)
	{
		(*i).second->safeKill();
	}

	idMap.clear();
	markers.clear();
}

void Cutscene::load(const std::string &f)
{
	clear();

	doc.LoadFile(f.c_str());

	TiXmlElement *e = doc.FirstChildElement("time");
	while (e)
	{
		CutsceneMarker m;
		m.e = e;

		std::string s = e->Attribute("t");
		int p = 0, c1=0,c2=0;
		float t = 0;
		for (int c=0; c < s.size(); c++)
		{
			switch(c)
			{
			case 0:
				t += atoi(s[c]) * (600);
			break;
			case 1:
				t += atoi(s[c]) * 60;
			break;
			case 3:
				t += atoi(s[c]) * 10;
			break;
			case 4:
				t += atoi(s[c]);
			break;
			case 6:
				t += atoi(s[c]) * 0.1f;
			break;
			case 7:
				t += atoi(s[c]) * 0.01f;
			break;
			}
		}

		markers.push_back(m);
	}
}

void Cutscene::save(const std::string &f)
{
}

void Cutscene::playMarker(CutsceneMarker *m)
{
	if (m)
	{
		TiXmlElement *r=0;
		if (r = m->e->FirstChildElement("quad"))
		{
			id = r->Attribute("id");
			if (idMap[id])
			{
				errorLog("id [" + id + "] already exists");
			}

			std::string gfx = m->e->Attribute("gfx");
			std::istringstream is(m->e->Attribute("pos"));
			Vector pos;
			is >> pos.x >> pos.y >> pos.z;
			int layer = atoi(m->e->Attribute("layer"));

			Quad *q = new Quad(gfx, pos);
			addRenderObject(q, layer);
		}
		if (r = m->e->FirstChildElement(""))
		{
			
		}
	}
}

void Cutscene::update(float dt)
{
	if (playing)
	{
		time += dt;

		while ((curMarker < markers.size()) && (time >= markers[curMarker].t))
		{
			playMarker(&markers[curMarker]);

			curMarker ++;
		}
	}
}
