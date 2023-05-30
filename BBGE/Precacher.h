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
#ifndef PRECACHER_H
#define PRECACHER_H

#include <vector>
#include <string>
#include "Texture.h"

class RenderObject;

class Precacher
{
public:
	typedef void (*ProgressCallback)(void);

	Precacher();
	~Precacher();
	void precacheList(const std::string &list, ProgressCallback progress = NULL);
	void precacheTex(const std::string &tex, ProgressCallback progress = NULL);
	void clear();
	void setBaseDir(const std::string& dir);
private:
	static void _Callback(const std::string &file, void *param);
	void _precacheTex(const std::string &tex);
	std::string basedir;
	std::vector<CountedPtr<Texture> > texkeep;
	std::vector<std::string> todo;
	void doCache(ProgressCallback progress = NULL);
};

#endif
