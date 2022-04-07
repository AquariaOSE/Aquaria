#include "GLLoad.h"

#ifdef BBGE_BUILD_OPENGL_STATIC

bool lookup_all_glsyms() { return true; }
bool void unload_all_glsyms() { return false; }

#else

#include "Base.h"

#include "RenderBase.h"

#include <sstream>

#ifdef GLAPIENTRY
#undef GLAPIENTRY
#endif

#ifdef BBGE_BUILD_WINDOWS
#define GLAPIENTRY __stdcall
#else
#define GLAPIENTRY
#endif

#include <GL/glext.h>


PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = NULL;

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

// GL_ARB_shader_objects and related
PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT = NULL;
PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT = NULL;

// extern
unsigned g_dbg_numRenderCalls = 0;
bool g_has_GL_GENERATE_MIPMAP = false;


#define GL_FUNC(ret,fn,params,call,rt) \
	extern "C" { \
	static ret (GLAPIENTRY *p##fn) params = NULL; \
	ret GLAPIENTRY fn params { ++g_dbg_numRenderCalls; rt p##fn call; } \
}
#include "OpenGLStubs.h"
#undef GL_FUNC

static bool lookup_glsym(const char *funcname, void **func)
{
	*func = SDL_GL_GetProcAddress(funcname);
	if (*func == NULL)
	{
		std::ostringstream os;
		os << "Failed to find OpenGL symbol \"" << funcname << "\"\n";
		errorLog(os.str());
		return false;
	}
	return true;
}

bool lookup_all_glsyms()
{
	bool retval = true;

#define GL_FUNC(ret,fn,params,call,rt) \
	if (!lookup_glsym(#fn, (void **) &p##fn)) retval = false;
#include "OpenGLStubs.h"
#undef GL_FUNC

	if(const char *ver = (const char*)glGetString(GL_VERSION))
	{
		unsigned major = 0, minor = 0;
		sscanf(ver, "%u.%u", &major, &minor);
		std::ostringstream os;
		os << "Detected OpenGL version: " << major << "." << minor;
		debugLog(os.str());
		// GL >= 1.4 and <= 2.x has GL_GENERATE_MIPMAP
		if(major < 3 && (major > 1 || (major == 1 && minor >= 4)))
			g_has_GL_GENERATE_MIPMAP = true;
	}

	// optional functions

	// mipmaps
	glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)SDL_GL_GetProcAddress("glGenerateMipmapEXT");
	{
		std::ostringstream os;
		os << "glGenerateMipmapEXT = " << glGenerateMipmapEXT;
		debugLog(os.str());
	}
	// framebuffer
	glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glIsRenderbufferEXT");
	glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindRenderbufferEXT");
	glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");
	glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
	glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
	glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)SDL_GL_GetProcAddress("glGetRenderbufferParameterivEXT");
	glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glIsFramebufferEXT");
	glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindFramebufferEXT");
	glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
	glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenFramebuffersEXT");
	glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
	glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture1DEXT");
	glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
	glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture3DEXT");
	glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)SDL_GL_GetProcAddress("glFramebufferRenderbufferEXT");
	glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)SDL_GL_GetProcAddress("glGetFramebufferAttachmentParameterivEXT");

	// shaders
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

	return retval;
}

void unload_all_glsyms()
{
// reset all the entry points to NULL, so we know exactly what happened
//  if we call a GL function after shutdown.
#define GL_FUNC(ret,fn,params,call,rt) \
	p##fn = NULL;
#include "OpenGLStubs.h"
#undef GL_FUNC

	glGenerateMipmapEXT = NULL;

	// set these back to NULL and reload them upon reinit, otherwise they
	//  might point to a bogus address when the shared library is reloaded.
	glIsRenderbufferEXT = NULL;
	glBindRenderbufferEXT = NULL;
	glDeleteRenderbuffersEXT = NULL;
	glGenRenderbuffersEXT = NULL;
	glRenderbufferStorageEXT = NULL;
	glGetRenderbufferParameterivEXT = NULL;
	glIsFramebufferEXT = NULL;
	glBindFramebufferEXT = NULL;
	glDeleteFramebuffersEXT = NULL;
	glGenFramebuffersEXT = NULL;
	glCheckFramebufferStatusEXT = NULL;
	glFramebufferTexture1DEXT = NULL;
	glFramebufferTexture2DEXT = NULL;
	glFramebufferTexture3DEXT = NULL;
	glFramebufferRenderbufferEXT = NULL;
	glGetFramebufferAttachmentParameterivEXT = NULL;

	glCreateProgramObjectARB  = NULL;
	glDeleteObjectARB         = NULL;
	glUseProgramObjectARB     = NULL;
	glCreateShaderObjectARB   = NULL;
	glShaderSourceARB         = NULL;
	glCompileShaderARB        = NULL;
	glGetObjectParameterivARB = NULL;
	glAttachObjectARB         = NULL;
	glGetInfoLogARB           = NULL;
	glLinkProgramARB          = NULL;
	glGetUniformLocationARB   = NULL;
	glGetActiveUniformARB     = NULL;
	glUniform1fvARB           = NULL;
	glUniform2fvARB           = NULL;
	glUniform3fvARB           = NULL;
	glUniform4fvARB           = NULL;
	glUniform1ivARB           = NULL;
	glUniform2ivARB           = NULL;
	glUniform3ivARB           = NULL;
	glUniform4ivARB           = NULL;
}

#endif // BBGE_BUILD_OPENGL_STATIC
