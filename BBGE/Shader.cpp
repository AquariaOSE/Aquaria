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
	PFNGLUNIFORM4FARBPROC            glUniform4fARB            = NULL;
	PFNGLUNIFORM1IARBPROC            glUniform1iARB            = NULL;
	PFNGLUNIFORM1FARBPROC            glUniform1fARB            = NULL;
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
		/*
		MessageBox(NULL,"GL_ARB_shading_language_100 extension was not found",
		"ERROR",MB_OK|MB_ICONEXCLAMATION);
		*/
		goto end;
	}

	if( strstr( ext, "GL_ARB_shader_objects" ) == NULL )
	{
		debugLog("GL_ARB_shader_objects extension was not found");
		goto end;
	}
	else
	{
#ifdef BBGE_BUILD_SDL
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
		glUniform4fARB            = (PFNGLUNIFORM4FARBPROC)SDL_GL_GetProcAddress("glUniform4fARB");
		glUniform1iARB            = (PFNGLUNIFORM1IARBPROC)SDL_GL_GetProcAddress("glUniform1iARB");
		glUniform1fARB            = (PFNGLUNIFORM1FARBPROC)SDL_GL_GetProcAddress("glUniform1fARB");
#endif

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
			!glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB || 
			!glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB || 
			!glLinkProgramARB || !glGetUniformLocationARB || !glUniform4fARB ||
			!glUniform1iARB || !glUniform1fARB )
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
#ifdef BBGE_BUILD_OPENGL
	g_programObj = 0;
#endif
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

bool Shader::isLoaded()
{
	return g_programObj != 0;
}

void Shader::reload()
{
	load(vertFile, fragFile);
}

void Shader::bind()
{
#ifdef BBGE_BUILD_SHADERS
	if (!_useShaders)
		return;
	glUseProgramObjectARB(g_programObj);
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
	if(vertCode && !(vertexShader = _compileShader(GL_VERTEX_SHADER_ARB, vertCode, str, sizeof(str))))
	{
		std::ostringstream os;
		os << "Vertex Shader Compile Error [" << vertFile << "]:\n" << str;
		errorLog(os.str());
		return;
	}

	//
	// Create the fragment shader...
	//
	if(fragCode && !(fragmentShader = _compileShader(GL_FRAGMENT_SHADER_ARB, fragCode, str, sizeof(str))))
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

#endif
}

// TODO: I'm not quite sure but i bet this sucks.
// Design a good caching policy and simplify the implementation,
// but keep shader dynamism and shaders generated at runtime in mind.
// No idea if and how much runtime performance it costs
// to query the uniform locations everytime.
// -- FG

static void shaderUniformError(const char *func, const char *var)
{
	GLint err = glGetError();
	/*std::ostringstream os;
	os << "Shader::" << func << "(" << var << ") -- undef uniform (Error: " << err << ")";
	debugLog(os.str());*/
}

void Shader::setInt(const char *name, int x)
{
#if BBGE_BUILD_SHADERS
	if(!g_programObj)
		return;
	GLint loc = glGetUniformLocationARB(g_programObj, name);
	if(loc != -1)
		glUniform1iARB(loc, x);
	else
		shaderUniformError("setInt", name);
#endif
}

void Shader::setFloat(const char *name, float x)
{
#if BBGE_BUILD_SHADERS
	if(!g_programObj)
		return;
	GLint loc = glGetUniformLocationARB(g_programObj, name);
	if(loc != -1)
		glUniform1fARB(loc, x);
	else
		shaderUniformError("setFloat", name);
#endif
}

void Shader::setFloat4(const char *name, float x, float y,  float z, float w)
{
#if BBGE_BUILD_SHADERS
	if(!g_programObj)
		return;
	GLint loc = glGetUniformLocationARB(g_programObj, name);
	if(loc != -1)
		glUniform4fARB(loc, x, y, z, w);
	else
		shaderUniformError("setFloat4", name);
#endif
}
