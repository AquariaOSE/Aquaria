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

#include "Shader.h"
#include "algorithmx.h"
#include "RenderBase.h"
#include <sstream>

bool Shader::_wasInited = false;
bool Shader::_useShaders = false;

void Shader::staticInit()
{
	if (_wasInited)
		return;

	_wasInited = true;
	debugLog("Initializing shaders...");

	bool use = true;

#ifndef BBGE_BUILD_OPENGL_STATIC
	if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
		!glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB ||
		!glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB ||
		!glLinkProgramARB || !glGetUniformLocationARB || !glGetActiveUniformARB ||
		!glUniform1fvARB || !glUniform2fvARB || !glUniform3fvARB || !glUniform4fvARB ||
		!glUniform1ivARB || !glUniform2ivARB || !glUniform3ivARB || !glUniform4ivARB)
	{
		glCreateProgramObjectARB = 0;
		debugLog("One or more GL_ARB_shader_objects functions were not found");
		use = false;
	}
#endif

	_useShaders = use;
	if (use)
		debugLog("Shader support enabled.");
	else
		debugLog("Shader support not enabled.");
}

Shader::Shader()
{
	numUniforms = -1;
	uniformsDirty = false;

	g_programObj = 0;
}

Shader::~Shader()
{
	unload();
}

void Shader::unload()
{
	if (!_useShaders)
		return;
	if (g_programObj)
	{
		glDeleteObjectARB( g_programObj );
		g_programObj = 0;
	}
}

bool Shader::isLoaded() const
{
	return g_programObj != 0;
}

void Shader::reload()
{
	if (vertFile.size() || fragFile.size())
		load(vertFile, fragFile);
	else
		loadSrc(vertSrc.c_str(), fragSrc.c_str());
}

void Shader::bind()
{
	if (!_useShaders)
		return;
	glUseProgramObjectARB(g_programObj);
	_flushUniforms();
}

void Shader::unbind()
{
	if (!_useShaders)
		return;
	glUseProgramObjectARB(0);
}

unsigned int Shader::_compileShader(int type, const char *src, char *errbuf, size_t errbufsize)
{
	GLint compiled = 0;
	GLhandleARB handle = glCreateShaderObjectARB(type);
	if(!handle)
	{
		std::ostringstream os;
		os << "Failed to create shader object of type " << type;
		debugLog(os.str());
		return 0;
	}

	glShaderSourceARB( handle, 1, &src, NULL );
	glCompileShaderARB( handle);

	glGetObjectParameterivARB(handle, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
	glGetInfoLogARB(handle, errbufsize, NULL, errbuf);
	if(!compiled)
	{
		glDeleteObjectARB(handle);
		handle = 0;
	}
	GLint err = glGetError();
	if(err != GL_NO_ERROR)
	{
		std::ostringstream os;
		os << "Shader::_compileShader: Unexpected error " << err;
		errorLog(os.str());
	}
	return handle;
}

void Shader::load(const std::string &file, const std::string &fragFile)
{
	staticInit();
	if(!_useShaders)
		return;

	debugLog("Shader::load("+file+", "+fragFile+")");

	this->vertFile = file;
	this->fragFile = fragFile;

	char *vertCode = file.length()	 ? readFile(file.c_str())	 : NULL;
	char *fragCode = fragFile.length() ? readFile(fragFile.c_str()) : NULL;

	loadSrc(vertCode, fragCode);

	delete [] vertCode;
	delete [] fragCode;
}

void Shader::loadSrc(const char *vertCode, const char *fragCode)
{
	staticInit();
	unload();

	if(!_useShaders)
		return;

	char str[4096];

	GLhandleARB vertexShader = 0;
	GLhandleARB fragmentShader = 0;

	//
	// Create the vertex shader...
	//
	if(vertCode && *vertCode && !(vertexShader = _compileShader(GL_VERTEX_SHADER_ARB, vertCode, str, sizeof(str))))
	{
		std::ostringstream os;
		os << "Vertex Shader Compile Error [" << vertFile << "]:\n" << str;
		errorLog(os.str());
		return;
	}

	//
	// Create the fragment shader...
	//
	if(fragCode && *fragCode && !(fragmentShader = _compileShader(GL_FRAGMENT_SHADER_ARB, fragCode, str, sizeof(str))))
	{
		std::ostringstream os;
		os << "Fragment Shader Compile Error [" << fragFile << "]:\n" << str;
		errorLog(os.str());
		return;
	}

	//
	// Create a program object and attach the two compiled shaders...
	//

	g_programObj = glCreateProgramObjectARB();

	if (!(g_programObj && (vertexShader || fragmentShader)))
	{
		errorLog("programObj / vertexShader / fragmentShader problem");
		unload();
		return;
	}

	//
	// Link the program object and print out the info log...
	//
	if(vertexShader)
		glAttachObjectARB( g_programObj, vertexShader );
	if(fragmentShader)
		glAttachObjectARB( g_programObj, fragmentShader );

	glLinkProgramARB( g_programObj );

	// Shader objects will be deleted as soon as the program object is deleted
	if(vertexShader)
		glDeleteObjectARB(vertexShader);
	if(fragmentShader)
		glDeleteObjectARB(fragmentShader);

	GLint bLinked;
	glGetObjectParameterivARB( g_programObj, GL_OBJECT_LINK_STATUS_ARB, &bLinked );


	if(!bLinked)
	{
		glGetInfoLogARB( g_programObj, sizeof(str), NULL, str );
		std::ostringstream os;
		os << "Shader Linking Error: " << str;
		errorLog(os.str());
		unload();
		return;
	}

	vertSrc = vertCode ? vertCode : "";
	fragSrc = fragCode ? fragCode : "";

	_queryUniforms();
}

void Shader::_setUniform(Uniform *u)
{
	switch(u->type)
	{
		case GL_FLOAT:          glUniform1fvARB(u->location, 1, u->data.f.f); break;
		case GL_FLOAT_VEC2_ARB: glUniform2fvARB(u->location, 1, u->data.f.f); break;
		case GL_FLOAT_VEC3_ARB: glUniform3fvARB(u->location, 1, u->data.f.f); break;
		case GL_FLOAT_VEC4_ARB: glUniform4fvARB(u->location, 1, u->data.f.f); break;
		case GL_INT:            glUniform1ivARB(u->location, 1, u->data.i.i); break;
		case GL_INT_VEC2_ARB:   glUniform2ivARB(u->location, 1, u->data.i.i); break;
		case GL_INT_VEC3_ARB:   glUniform3ivARB(u->location, 1, u->data.i.i); break;
		case GL_INT_VEC4_ARB:   glUniform4ivARB(u->location, 1, u->data.i.i); break;
	}
	u->dirty = false;
}

void Shader::_flushUniforms()
{
	if(!uniformsDirty)
		return;
	uniformsDirty = false;

	for(size_t i = 0; i < uniforms.size(); ++i)
	{
		Uniform &u = uniforms[i];
		if(u.dirty)
			_setUniform(&u);
	}
}

// for sorting
bool Shader::_sortUniform(const Uniform& a, const char *bname)
{
	return strcmp(a.name, bname) < 0;
}

bool Shader::Uniform::operator< (const Uniform& b) const
{
	return Shader::_sortUniform(*this, &b.name[0]);
}

void Shader::_queryUniforms()
{
	glGetObjectParameterivARB(g_programObj, GL_OBJECT_ACTIVE_UNIFORMS_ARB , (GLint*)&numUniforms);

	if (numUniforms == 0 || numUniforms == -1)
	{
		uniforms.clear();
		return;
	}

	uniforms.reserve(numUniforms);
	size_t total = 0;

	for (unsigned int i = 0; i < numUniforms; ++i)
	{
		Uniform u;
		GLint size = 0;
		GLenum type = 0;
		glGetActiveUniformARB(g_programObj, i, sizeof(u.name), NULL, &size, &type, &u.name[0]);
		if(!type || !size)
			continue;
		u.location = glGetUniformLocationARB(g_programObj, u.name);
		if(u.location == -1)
			continue;

		bool add = total >= uniforms.size();
		if(add || type != u.type) // keep data intact on reload
			memset(&u.data, 0, sizeof(u.data));
		u.dirty = true;
		u.type = type;

		if(add)
			uniforms.push_back(u);
		else
			uniforms[total] = u;

		++total;
	}

	uniforms.resize(total);

	// sort to be able to do binary search later
	std::sort(uniforms.begin(), uniforms.end());

	uniformsDirty = true;
}

int Shader::_getUniformIndex(const char *name)
{
	// binary search
	UniformVec::iterator it = stdx_fg::lower_bound(uniforms.begin(), uniforms.end(), name, _sortUniform);
	// because lower_bound returns the first element that compares less, it might not be the correct one
	if(it != uniforms.end() && strcmp(it->name, name))
		return -1;
	return int(it - uniforms.begin());
}

void Shader::setInt(const char *name, int x, int y /* = 0 */, int z /* = 0 */, int w /* = 0 */)
{
	if(!g_programObj || numUniforms == 0 || numUniforms == -1)
		return;
	int idx = _getUniformIndex(name);
	if(unsigned(idx) >= uniforms.size())
		return;
	Uniform& u = uniforms[idx];
	u.data.i.i[0] = x;
	u.data.i.i[1] = y;
	u.data.i.i[2] = z;
	u.data.i.i[3] = w;
	u.dirty = true;
	uniformsDirty = true;
}

void Shader::setFloat(const char *name, float x, float y /* = 0 */, float z /* = 0 */, float w /* = 0 */)
{
	if(!g_programObj || numUniforms == 0 || numUniforms == -1)
		return;
	int idx = _getUniformIndex(name);
	if(unsigned(idx) >= uniforms.size())
		return;
	Uniform& u = uniforms[idx];
	u.data.f.f[0] = x;
	u.data.f.f[1] = y;
	u.data.f.f[2] = z;
	u.data.f.f[3] = w;
	u.dirty = true;
	uniformsDirty = true;
}
