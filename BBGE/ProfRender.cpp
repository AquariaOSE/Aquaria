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
#include "ProfRender.h"

void prof_print(float x, float y, char *str)
{
	core->print(x, y-1.5f*4, str, 6);

	//Prof_set_report_mode(Prof_HIERARCHICAL_TIME);
	/*
     Prof_SELF_TIME: flat times sorted by self time
     Prof_HIERARCHICAL_TIME: flat times sorted by hierarchical time
     Prof_CALL_GRAPH: call graph parent/children information
	 */
}

float prof_width(char *str)
{
	int c = 0;
	float x = 0;
	while (str[c] != '\0')
	{
		c++;
		x += 1.2f;
	}
	x *= 6;
	return x;
}

ProfRender::ProfRender() : RenderObject()
{
	followCamera = 1;
	cull = false;
	alpha = 0.5;

	//Prof_set_report_mode(Prof_CALL_GRAPH);
}

void ProfRender::onRender()
{
	/*
	for (int i = 0; i < 20; i++)
	{
		core->print(10, 10+i*10, "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyz");
	}
	*/
#ifdef BBGE_BUILD_WINDOWS
	/*
	Prof_draw_gl(20, 50, 400, 400, 20, 2, prof_print, prof_width);
	Prof_draw_graph_gl(430, 50, 2, 8);
	*/
#endif
}

