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
#pragma once

#include "Base.h"

class Shader
{
public:
	Shader();
	~Shader();
	bool isLoaded();
	void load(const std::string &file, const std::string &fragFile);
	void reload();
	void bind();
	void unbind();
	void setMode(int mode);
	void setValue(float x, float y, float z, float w);
	std::string vertFile, fragFile;
protected:
#ifdef BBGE_BUILD_OPENGL
	GLuint g_programObj;
	GLuint g_vertexShader;
	GLuint g_fragmentShader;
	GLuint g_location_texture;
	GLuint g_location_mode;
	GLuint g_location_value;
#endif
	int mode;
	float vx, vy, vz, vw;
	bool loaded;

	static void staticInit();
	static bool _wasInited;
	static bool _useShaders;
};
