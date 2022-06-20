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
#include <SDL.h>
#include <SDL_main.h>


extern "C" int main(int argc,char *argv[])
{
	std::string dsqParam = ""; // fileSystem
	std::string extraDataDir = "";

	const char *envPath = 0;
#ifdef BBGE_BUILD_UNIX
	envPath = getenv("AQUARIA_DATA_PATH");
	if (envPath)
	{
		dsqParam = envPath;
	}
#endif
#ifdef AQUARIA_DEFAULT_DATA_DIR
	if(!envPath)
		dsqParam = AQUARIA_DEFAULT_DATA_DIR;
#endif
#ifdef AQUARIA_EXTRA_DATA_DIR
	extraDataDir = AQUARIA_EXTRA_DATA_DIR;
#endif

	{
		DSQ dsql(dsqParam, extraDataDir);
		dsql.init();
		dsql.run();
		dsql.shutdown();
	}

	return (0);
}
