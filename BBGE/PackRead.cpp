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
#include "PackRead.h"
#include <fcntl.h>

#if defined(BBGE_BUILD_UNIX)
#include <sys/types.h>
#include <sys/stat.h>
#define _open open
#define _read read
#define _lseek lseek
#define _close close
#else
#include <io.h>
#endif

#define LOC_UNIT			long int

void packGetLoc(const std::string &pack, const std::string &file, long int *location, int *size)
{
	*location = 0;
	*size = 0;

	int fd = _open(pack.c_str(), O_RDONLY);
	int numFiles, nameSize, fileSize; LOC_UNIT loc;

	_read(fd, &numFiles, sizeof(int));

	for (int i = 0; i < numFiles; i++)
	{
		_lseek(fd, (i * sizeof(LOC_UNIT)) + sizeof(int), SEEK_SET);

		_read(fd, &loc, sizeof(LOC_UNIT));

		_lseek(fd, loc, SEEK_SET);

		_read(fd, &fileSize, sizeof(int));

		_read(fd, &nameSize, sizeof(int));

		char *name = (char*)malloc(nameSize+1);
		_read(fd, name, nameSize);
		name[nameSize] = '\0';



		if (nocasecmp(file, std::string(name))==0)
		{
			char *buffer = (char*)malloc(fileSize);
			_read(fd, buffer, fileSize);

			free(name);
			_close(fd);

			*location = loc + sizeof(int) + sizeof(int) + nameSize;
			*size = fileSize;

			return;

		}

		free(name);
	}

	_close(fd);

	return;
}

void packReadInfo(const char *pack)
{
	debugLog("pack read info");



	int fd = _open(pack, O_RDONLY);

	int numFiles, nameSize, fileSize;
	LOC_UNIT loc;

	_read(fd, &numFiles, sizeof(int));



	for (int i = 0; i < numFiles; i++)
	{
		_lseek(fd, (i * sizeof(LOC_UNIT)) + sizeof(int), SEEK_SET);

		_read(fd, &loc, sizeof(LOC_UNIT));



		_lseek(fd, loc, SEEK_SET);

		_read(fd, &fileSize, sizeof(int));

		_read(fd, &nameSize, sizeof(int));

		char *name = (char*)malloc(nameSize+1);
		_read(fd, name, nameSize);
		name[nameSize] = '\0';



		free(name);
	}

	_close(fd);
}
