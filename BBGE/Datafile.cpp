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
#include "Datafile.h"
#include "Texture.h"

Datafile::Datafile()
{
	w = 0; h = 0;
}

Datafile::~Datafile()
{
	destroy();
}

void Datafile::destroy()
{
	for (int i = 0; i < textures.size(); i++)
	{
		textures[i]->removeRef();
	}
	textures.clear();
}

//hacky... for now
void Datafile::addTexture(Texture *t)
{
	textures.push_back(t);
	w = t->width;
	h = t->height;
}

Texture* Datafile::get(int idx)
{
	if (idx < 0 || idx >= textures.size())
	{
		std::ostringstream os;
		os << "index [" << idx << "] out of range - textureName[" << this->name << "]";
		errorLog (os.str());
	}
	return textures[idx];
}



void Datafile::loadFromAVI(const std::string &aviFile)
{



}

void Datafile::loadTextureRange(const std::string &file, const std::string &type, int start, int end)
{
	for (int t = start; t < end; t++)
	{

		std::ostringstream num_os;
		num_os << t;

		std::ostringstream os;
		os << file;

		for (int j = 0; j < 4 - num_os.str().size(); j++)
		{
			os << "0";
		}

		os << t;

		os << type;

		addTexture(core->addTexture(os.str()));
	}
}

const std::string &Datafile::getName()
{
	return name;
}

void Datafile::setName(const std::string &name)
{
	this->name = name;
}

