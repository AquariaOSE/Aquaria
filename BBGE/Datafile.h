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
#ifndef __datafile__
#define __datafile__

#include "Core.h"

class Datafile
{
public:
	Datafile();
	~Datafile();
	void destroy();

	void addTexture(Texture *t);
	Texture* get(int idx);
	void loadTextureRange(const std::string &file, const std::string &type, int start, int end);
	void loadFromAVI(const std::string &aviFile);

	const std::string &getName();

	void setName(const std::string &name);
	int getSize()
	{
		return textures.size();
	}
protected:

	std::string name;


	int w, h;

	std::vector <Texture*> textures;
};

#endif
