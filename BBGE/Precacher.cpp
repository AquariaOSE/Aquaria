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
#include "Precacher.h"
#include "Quad.h"
#include "Core.h"
#include "ttvfs_stdio.h"

Precacher::Precacher()
{
}

Precacher::~Precacher()
{
}

void Precacher::setBaseDir(const std::string& dir)
{
	basedir = dir;
}

void Precacher::clear()
{
	texkeep.clear();
}

void Precacher::_Callback(const std::string &file, void *param)
{
	Precacher *p = (Precacher*)param;
	p->_precacheTex(file);
}

// precacheTex
// caches one texture
// also support simple wildcard to cache multiple textures
// e.g. naija/*.png
void Precacher::_precacheTex(const std::string &tex)
{
	assert(!basedir.empty());
	if (tex.empty()) return;

	if (tex.find('*')!=std::string::npos)
	{
		size_t loc = tex.find('*');
		std::string path  = tex.substr(0, loc);
		std::string type = tex.substr(loc+1, tex.size());
		path = basedir + path;
		forEachFile(path, type, _Callback, this);
	}
	else
	{
		std::string t = tex;
		if (tex.find(basedir) != std::string::npos)
		{
			t = tex.substr(basedir.size(), tex.size());
		}
		todo.push_back(t);
	}
}

static void texLoadProgressCallback(size_t done, void *ud)
{
	Precacher::ProgressCallback cb = (Precacher::ProgressCallback)(ud);
	cb();
}

void Precacher::doCache(ProgressCallback progress)
{
	if(!todo.empty())
	{
		std::ostringstream os;
		os << "Precacher: Batch-loading " << todo.size() << " textures...";
		debugLog(os.str());
		std::vector<Texture*> tmp(todo.size());
		core->texmgr.loadBatch(&tmp[0], &todo[0], todo.size(), TextureMgr::KEEP,
			progress ? texLoadProgressCallback : NULL, (void*)progress);
		todo.clear();
		texkeep.reserve(texkeep.size() + tmp.size());
		for(size_t i = 0; i < tmp.size(); ++i)
			texkeep.push_back(tmp[i]);
	}
	debugLog("Precacher: done");
}

void Precacher::precacheList(const std::string &list, ProgressCallback progress)
{
	assert(todo.empty());
	InStream in(list.c_str());
	std::string t;
	while (std::getline(in, t))
	{
		while (!t.empty())
		{
			if(t.back() == '\r' || t.back() == '\n') // linux doesn't like CRLF, make sure to trim that off
				t.pop_back();
			else
				break;
		}

		if(!t.empty())
			_precacheTex(t);
	}
	in.close();
	doCache(progress);
}

void Precacher::precacheTex(const std::string& tex, ProgressCallback progress)
{
	_precacheTex(tex);
	doCache(progress);
}
