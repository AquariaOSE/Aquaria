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
#ifndef BBGE_SHADER_H
#define BBGE_SHADER_H

#include "Base.h"

class Shader
{
public:
	Shader();
	~Shader();
	bool isLoaded();
	void load(const std::string &file, const std::string &fragFile);
	void loadSrc(const char *vertCode, const char *fragCode);
	void reload();
	void unload();
	void bind();
	void unbind();

	// TODO: design a good API for this...
	void setInt(const char *name, int x);
	void setFloat(const char *name, float x);
	void setFloat4(const char *name, float x, float y, float z, float w);


protected:
	std::string vertFile, fragFile;
#ifdef BBGE_BUILD_OPENGL
	GLuint g_programObj;
#endif

private:
	static void staticInit();
	static bool _wasInited;
	static bool _useShaders;

	static unsigned int _compileShader(int type, const char *src, char *errbuf, size_t errbufsize);
};

#endif
