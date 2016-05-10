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

#ifdef BBGE_BUILD_SHADERS
	// GL_ARB_shader_objects
	PFNGLCREATEPROGRAMOBJECTARBPROC  glCreateProgramObjectARB  = NULL;
	PFNGLDELETEOBJECTARBPROC         glDeleteObjectARB         = NULL;
	PFNGLUSEPROGRAMOBJECTARBPROC     glUseProgramObjectARB     = NULL;
	PFNGLCREATESHADEROBJECTARBPROC   glCreateShaderObjectARB   = NULL;
	PFNGLSHADERSOURCEARBPROC         glShaderSourceARB         = NULL;
	PFNGLCOMPILESHADERARBPROC        glCompileShaderARB        = NULL;
	PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB = NULL;
	PFNGLATTACHOBJECTARBPROC         glAttachObjectARB         = NULL;
	PFNGLGETINFOLOGARBPROC           glGetInfoLogARB           = NULL;
	PFNGLLINKPROGRAMARBPROC          glLinkProgramARB          = NULL;
	PFNGLGETUNIFORMLOCATIONARBPROC   glGetUniformLocationARB   = NULL;
	PFNGLGETACTIVEUNIFORMARBPROC     glGetActiveUniformARB     = NULL;
	PFNGLUNIFORM1FVARBPROC           glUniform1fvARB            = NULL;
	PFNGLUNIFORM2FVARBPROC           glUniform2fvARB            = NULL;
	PFNGLUNIFORM3FVARBPROC           glUniform3fvARB            = NULL;
	PFNGLUNIFORM4FVARBPROC           glUniform4fvARB            = NULL;
	PFNGLUNIFORM1IVARBPROC           glUniform1ivARB            = NULL;
	PFNGLUNIFORM2IVARBPROC           glUniform2ivARB            = NULL;
	PFNGLUNIFORM3IVARBPROC           glUniform3ivARB            = NULL;
	PFNGLUNIFORM4IVARBPROC           glUniform4ivARB            = NULL;

#endif

bool Shader::_wasInited = false;
bool Shader::_useShaders = false;

void Shader::staticInit()
{
	if (_wasInited)
		return;

	_wasInited = true;
	debugLog("Initializing shaders...");

#if defined(BBGE_BUILD_SHADERS) && defined(BBGE_BUILD_OPENGL)
	char *ext = (char*)glGetString( GL_EXTENSIONS );

	if( strstr( ext, "GL_ARB_shading_language_100" ) == NULL )
	{
		//This extension string indicates that the OpenGL Shading Language,
		// version 1.00, is supported.
		debugLog("GL_ARB_shading_language_100 extension was not found");

		goto end;
	}

	if( strstr( ext, "GL_ARB_shader_objects" ) == NULL )
	{
		debugLog("GL_ARB_shader_objects extension was not found");
		goto end;
	}
	else
	{
		glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)SDL_GL_GetProcAddress("glCreateProgramObjectARB");
		glDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC)SDL_GL_GetProcAddress("glDeleteObjectARB");
		glUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC)SDL_GL_GetProcAddress("glUseProgramObjectARB");
		glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)SDL_GL_GetProcAddress("glCreateShaderObjectARB");
		glShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC)SDL_GL_GetProcAddress("glShaderSourceARB");
		glCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC)SDL_GL_GetProcAddress("glCompileShaderARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)SDL_GL_GetProcAddress("glGetObjectParameterivARB");
		glAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC)SDL_GL_GetProcAddress("glAttachObjectARB");
		glGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC)SDL_GL_GetProcAddress("glGetInfoLogARB");
		glLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC)SDL_GL_GetProcAddress("glLinkProgramARB");
		glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)SDL_GL_GetProcAddress("glGetUniformLocationARB");
		glGetActiveUniformARB     = (PFNGLGETACTIVEUNIFORMARBPROC)SDL_GL_GetProcAddress("glGetActiveUniformARB");
		glUniform1fvARB           = (PFNGLUNIFORM1FVARBPROC)SDL_GL_GetProcAddress("glUniform1fvARB");
		glUniform2fvARB           = (PFNGLUNIFORM2FVARBPROC)SDL_GL_GetProcAddress("glUniform2fvARB");
		glUniform3fvARB           = (PFNGLUNIFORM3FVARBPROC)SDL_GL_GetProcAddress("glUniform3fvARB");
		glUniform4fvARB           = (PFNGLUNIFORM4FVARBPROC)SDL_GL_GetProcAddress("glUniform4fvARB");
		glUniform1ivARB           = (PFNGLUNIFORM1IVARBPROC)SDL_GL_GetProcAddress("glUniform1ivARB");
		glUniform2ivARB           = (PFNGLUNIFORM2IVARBPROC)SDL_GL_GetProcAddress("glUniform2ivARB");
		glUniform3ivARB           = (PFNGLUNIFORM3IVARBPROC)SDL_GL_GetProcAddress("glUniform3ivARB");
		glUniform4ivARB           = (PFNGLUNIFORM4IVARBPROC)SDL_GL_GetProcAddress("glUniform4ivARB");

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
			!glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB ||
			!glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB ||
			!glLinkProgramARB || !glGetUniformLocationARB || !glGetActiveUniformARB ||
			!glUniform1fvARB || !glUniform2fvARB || !glUniform3fvARB || !glUniform4fvARB ||
			!glUniform1ivARB || !glUniform2ivARB || !glUniform3ivARB || !glUniform4ivARB)
		{
			glCreateProgramObjectARB = 0;
			debugLog("One or more GL_ARB_shader_objects functions were not found");
			goto end;
		}
	}

	// everything fine when we are here
	_useShaders = true;

#endif

end:

	if (_useShaders)
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
#ifdef BBGE_BUILD_SHADERS
	if (!_useShaders)
		return;
	if (g_programObj)
	{
		glDeleteObjectARB( g_programObj );
		g_programObj = 0;
	}
#endif
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
#ifdef BBGE_BUILD_SHADERS
	if (!_useShaders)
		return;
	glUseProgramObjectARB(g_programObj);
	_flushUniforms();
#endif
}

void Shader::unbind()
{
#ifdef BBGE_BUILD_SHADERS
	if (!_useShaders)
		return;
	glUseProgramObjectARB(0);
#endif
}

unsigned int Shader::_compileShader(int type, const char *src, char *errbuf, size_t errbufsize)
{
#ifdef BBGE_BUILD_SHADERS
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
#endif
	return 0;
}

void Shader::load(const std::string &file, const std::string &fragFile)
{
	staticInit();
	if(!_useShaders)
		return;

	debugLog("Shader::load("+file+", "+fragFile+")");

	this->vertFile = file;
	this->fragFile = fragFile;

	char *vertCode = file.length()     ? readFile(file)     : NULL;
	char *fragCode = fragFile.length() ? readFile(fragFile) : NULL;

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

#ifdef BBGE_BUILD_SHADERS

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

#endif
}

void Shader::_setUniform(Uniform *u)
{
	switch(u->type)
	{
		case GL_FLOAT:          glUniform1fvARB(u->location, 1, u->data.f); break;
		case GL_FLOAT_VEC2_ARB: glUniform2fvARB(u->location, 1, u->data.f); break;
		case GL_FLOAT_VEC3_ARB: glUniform3fvARB(u->location, 1, u->data.f); break;
		case GL_FLOAT_VEC4_ARB: glUniform4fvARB(u->location, 1, u->data.f); break;
		case GL_INT:            glUniform1ivARB(u->location, 1, u->data.i); break;
		case GL_INT_VEC2_ARB:   glUniform2ivARB(u->location, 1, u->data.i); break;
		case GL_INT_VEC3_ARB:   glUniform3ivARB(u->location, 1, u->data.i); break;
		case GL_INT_VEC4_ARB:   glUniform4ivARB(u->location, 1, u->data.i); break;
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
	glGetObjectParameterivARB(g_programObj, GL_OBJECT_ACTIVE_UNIFORMS_ARB , &numUniforms);

	if (numUniforms <= 0)
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
#if BBGE_BUILD_SHADERS
	if(!g_programObj || numUniforms <= 0)
		return;
	int idx = _getUniformIndex(name);
	if(unsigned(idx) >= uniforms.size())
		return;
	Uniform& u = uniforms[idx];
	u.data.i[0] = x;
	u.data.i[1] = y;
	u.data.i[2] = z;
	u.data.i[3] = w;
	u.dirty = true;
	uniformsDirty = true;
#endif
}

void Shader::setFloat(const char *name, float x, float y /* = 0 */, float z /* = 0 */, float w /* = 0 */)
{
#if BBGE_BUILD_SHADERS
	if(!g_programObj || numUniforms <= 0)
		return;
	int idx = _getUniformIndex(name);
	if(unsigned(idx) >= uniforms.size())
		return;
	Uniform& u = uniforms[idx];
	u.data.f[0] = x;
	u.data.f[1] = y;
	u.data.f[2] = z;
	u.data.f[3] = w;
	u.dirty = true;
	uniformsDirty = true;
#endif
}
