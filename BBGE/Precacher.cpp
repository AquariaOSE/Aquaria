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
	loadProgressCallback = NULL;
	cleaned = true;
}

Precacher::~Precacher()
{
	if (!cleaned)
		errorLog ("Precacher shutdown unclean");
}

void Precacher::setBaseDir(const std::string& dir)
{
	basedirOverride = dir;
}

void Precacher::clean()
{
	for (unsigned int i = 0; i < renderObjects.size(); i++)
	{
		RenderObject *r = renderObjects[i];
		r->destroy();
		delete r;
	}
	renderObjects.clear();
	cleaned = true;
}

void Precacher::loadTextureRange(const std::string &file, const std::string &type, int start, int end)
{
	for (int t = start; t < end; t++)
	{

		std::ostringstream num_os;
		num_os << t;

		std::ostringstream os;
		os << file;

		if(num_os.str().size() <= 4) {
			for (size_t j = 0; j < 4 - num_os.str().size(); j++) {
				os << "0";
			}
		}

		os << t;

		os << type;

		precacheTex(os.str());
	}
}

void precacherCallback(const std::string &file, void *param)
{
	Precacher *p = (Precacher*)param;
	p->precacheTex(file);
}

// precacheTex
// caches one texture
// also support simple wildcard to cache multiple textures
// e.g. naija/*.png
void Precacher::precacheTex(const std::string &tex)
{
	if (tex.find("txt") != std::string::npos)
	{
		errorLog("Call precacheList to precache a text file of gfx names, not precacheTex!");
	}
	if (tex.empty()) return;

	std::string basedir = basedirOverride.empty() ? core->getBaseTextureDirectory() : basedirOverride;

	if (core->debugLogTextures)
		debugLog("PRECACHING: " + tex);

	if (tex.find('*')!=std::string::npos)
	{
		if (core->debugLogTextures)
			debugLog("searching directory");

		int loc = tex.find('*');
		std::string path  = tex.substr(0, loc);
		std::string type = tex.substr(loc+1, tex.size());
		path = basedir + path;
		forEachFile(path, type, precacherCallback, this);
		return;
	}
	else
	{
		if (loadProgressCallback)
			loadProgressCallback();
		std::string t = tex;
		if (tex.find(basedir) != std::string::npos)
		{
			t = tex.substr(basedir.size(), tex.size());
		}
		Quad *q = new Quad;
		q->setTexture(t);
		q->alpha = 0;
		renderObjects.push_back(q);
		cleaned = false;
	}
}

void Precacher::precacheList(const std::string &list, void progressCallback())
{
	loadProgressCallback = progressCallback;
	InStream in(list.c_str());
	std::string t;
	while (std::getline(in, t))
	{
		if (!t.empty())
		{
#if defined(BBGE_BUILD_UNIX)

			t = t.substr(0,t.size()-1);
			debugLog("precache["+t+"]");
#endif
			stringToLower(t);
			precacheTex(t);
		}
	}
	in.close();
	loadProgressCallback = NULL;
}

