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
#ifdef BBGE_BUILD_WINDOWS
	#include <sys/stat.h>
#endif

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
#ifdef BBGE_BUILD_GLFW
		glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)glfwGetProcAddress("glCreateProgramObjectARB");
		glDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC)glfwGetProcAddress("glDeleteObjectARB");
		glUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC)glfwGetProcAddress("glUseProgramObjectARB");
		glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)glfwGetProcAddress("glCreateShaderObjectARB");
		glShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC)glfwGetProcAddress("glShaderSourceARB");
		glCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC)glfwGetProcAddress("glCompileShaderARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)glfwGetProcAddress("glGetObjectParameterivARB");
		glAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC)glfwGetProcAddress("glAttachObjectARB");
		glGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC)glfwGetProcAddress("glGetInfoLogARB");
		glLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC)glfwGetProcAddress("glLinkProgramARB");
		glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)glfwGetProcAddress("glGetUniformLocationARB");
		glUniform4fARB            = (PFNGLUNIFORM4FARBPROC)glfwGetProcAddress("glUniform4fARB");
		glUniform1iARB            = (PFNGLUNIFORM1IARBPROC)glfwGetProcAddress("glUniform1iARB");
#endif

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
#endif

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
			!glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB || 
			!glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB || 
			!glLinkProgramARB || !glGetUniformLocationARB || !glUniform4fARB ||
			!glUniform1iARB )
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
	loaded = false;
	mode = 0;
#ifdef BBGE_BUILD_OPENGL
	g_vertexShader = 0;
	g_fragmentShader = 0;
	g_programObj = 0;
	vx = vy = vz = vw = 0;
	g_location_texture = 0;
	g_location_mode = 0;
	g_location_value = 0;
#endif
}

Shader::~Shader()
{
#ifdef BBGE_BUILD_SHADERS
	if (!_useShaders)
		return;
	if (g_vertexShader)
		glDeleteObjectARB( g_vertexShader );
	if (g_fragmentShader)
		glDeleteObjectARB( g_fragmentShader );
	if (g_programObj)
		glDeleteObjectARB( g_programObj );
#endif
}

bool Shader::isLoaded()
{
	return loaded;
}

void Shader::setMode(int mode)
{
	this->mode = mode;
}

void Shader::setValue(float x, float y, float z, float w)
{
	vx = x;
	vy = y;
	vz = z;
	vw = w;
}

unsigned char *readShaderFile( const char *fileName )
{
	debugLog("readShaderFile()");
#ifdef BBGE_BUILD_WINDOWS
    FILE *file = fopen( fileName, "r" ); // FIXME: should this code ever be re-activated, adjust to VFS! -- fg

    if( file == NULL )
    {
        errorLog("Cannot open shader file!");
		return 0;
    }

    struct _stat fileStats;

    if( _stat( fileName, &fileStats ) != 0 )
    {
        errorLog("Cannot get file stats for shader file!");
        return 0;
    }


    unsigned char *buffer = new unsigned char[fileStats.st_size];

	int bytes = fread( buffer, 1, fileStats.st_size, file );

    buffer[bytes] = 0;

	fclose( file );

	debugLog("End readShaderFile()");

	return buffer;
	
#else
	debugLog("End readShaderFile()");
	return 0;
#endif
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
	glUseProgramObjectARB( g_programObj );
	if( g_location_texture != -1 )
		glUniform1iARB( g_location_texture, 0 );
	if ( g_location_mode )
		glUniform1iARB( g_location_mode, mode);
	if ( g_location_value )
		glUniform4fARB( g_location_value, vx, vy, vz, vw);
#endif
}

void Shader::unbind()
{
#ifdef BBGE_BUILD_SHADERS
	if (!_useShaders)
		return;
	glUseProgramObjectARB( NULL );
#endif
}

void Shader::load(const std::string &file, const std::string &fragFile)
{
	staticInit();
	loaded = false;

#ifdef BBGE_BUILD_SHADERS
	if(!_useShaders)
		return;

	debugLog("Shader::load("+file+", "+fragFile+")");

	g_location_texture	= 0;
	g_location_mode		= 0;
	g_location_value	= 0;

	try
	{

		debugLog("Shader::load 1");
		this->vertFile = file;
		this->fragFile = fragFile;
		//
		// If the required extension is present, get the addresses of its 
		// functions that we wish to use...
		//

		const char *vertexShaderStrings[1];
		const char *fragmentShaderStrings[1];
		GLint bVertCompiled;
		GLint bFragCompiled;
		GLint bLinked;
		char str[4096];

		//
		// Create the vertex shader...
		//

		debugLog("Shader::load 2");

		g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );

		unsigned char *vertexShaderAssembly = readShaderFile( file.c_str() );
		vertexShaderStrings[0] = (char*)vertexShaderAssembly;
		glShaderSourceARB( g_vertexShader, 1, vertexShaderStrings, NULL );
		glCompileShaderARB( g_vertexShader);
		delete[] vertexShaderAssembly;

		glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, 
								&bVertCompiled );
		if( bVertCompiled  == false )
		//if (true)
		{
			glGetInfoLogARB(g_vertexShader, sizeof(str), NULL, str);
			std::ostringstream os;
			os << "Vertex Shader Compile Error: " << str;
			debugLog(os.str());
			return;
		}

		//
		// Create the fragment shader...
		//

		debugLog("Shader::load 3");

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );

		unsigned char *fragmentShaderAssembly = readShaderFile( fragFile.c_str() );
		fragmentShaderStrings[0] = (char*)fragmentShaderAssembly;
		glShaderSourceARB( g_fragmentShader, 1, fragmentShaderStrings, NULL );
		glCompileShaderARB( g_fragmentShader );
		delete[] fragmentShaderAssembly;

		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, 
								&bFragCompiled );
		if( bFragCompiled == false )
		{
			glGetInfoLogARB( g_fragmentShader, sizeof(str), NULL, str );
			std::ostringstream os;
			os << "Fragment Shader Compile Error: " << str;
			debugLog(os.str());
			return;
		}

		debugLog("Shader::load 4");

		//
		// Create a program object and attach the two compiled shaders...
		//
		

		g_programObj = glCreateProgramObjectARB();

		if (!g_programObj || !g_vertexShader || !g_fragmentShader)
		{
			debugLog("programObj / vertexShader / fragmentShader problem");
			return;
		}

		glAttachObjectARB( g_programObj, g_vertexShader );
		glAttachObjectARB( g_programObj, g_fragmentShader );

		//
		// Link the program object and print out the info log...
		//

		glLinkProgramARB( g_programObj );
		glGetObjectParameterivARB( g_programObj, GL_OBJECT_LINK_STATUS_ARB, &bLinked );

		debugLog("Shader::load 5");

		if( bLinked == false )
		{
			glGetInfoLogARB( g_programObj, sizeof(str), NULL, str );
			std::ostringstream os;
			os << "Shader Linking Error: " << str;
			debugLog(os.str());
			return;
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		debugLog("Shader::load 6");

		g_location_texture = glGetUniformLocationARB( g_programObj, "tex" );
		g_location_mode = glGetUniformLocationARB( g_programObj, "mode" );
		g_location_value = glGetUniformLocationARB( g_programObj, "value" );

		debugLog("Shader::load 7");

		loaded = true;
	}
	catch(...)
	{
		debugLog("caught exception in shader::load");
		loaded = false;
	}
#endif
	debugLog("End Shader::load()");
}

