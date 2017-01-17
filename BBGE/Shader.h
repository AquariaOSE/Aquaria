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
#include "ScriptObject.h"

class Shader
{
public:
	Shader();
	~Shader();
	bool isLoaded() const;
	void load(const std::string &file, const std::string &fragFile);
	void loadSrc(const char *vertCode, const char *fragCode);
	void reload();
	void unload();
	void bind();
	void unbind();

	void setInt(const char *name, int x, int y = 0, int z = 0, int w = 0);
	void setFloat(const char *name, float x, float y = 0, float z = 0, float w = 0);
	// TODO: other setters needed?


protected:
	std::string vertFile, fragFile;
	std::string vertSrc, fragSrc;
	unsigned g_programObj;
	unsigned numUniforms;

private:
	static void staticInit();
	static bool _wasInited;
	static bool _useShaders;

	static unsigned int _compileShader(int type, const char *src, char *errbuf, size_t errbufsize);

	struct Uniform
	{
		int location; // GL location variable
		size_t type;
		bool dirty; // need to flush if true
		union
		{
			struct si
			{
				int i[4];
			} i;
			struct sf
			{
				float f[4];
			} f;
		} data;
		char name[64];

		bool operator< (const Uniform&) const;
	};

	static bool _sortUniform(const Uniform& a, const char *bname);

	void _queryUniforms();
	void _flushUniforms();
	void _registerUniform();

	void _setUniform(Uniform *u);
	int _getUniformIndex(const char *name);

	typedef std::vector<Uniform> UniformVec;
	UniformVec uniforms;

	bool uniformsDirty;
};

#endif
