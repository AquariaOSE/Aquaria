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

static const char *glDebugTypeToStr(unsigned e)
{
	switch(e)
	{
		case GL_DEBUG_TYPE_ERROR_ARB: return " ERR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "depr";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: return " UB!";
		case GL_DEBUG_TYPE_PORTABILITY_ARB: return "port";
		case GL_DEBUG_TYPE_PERFORMANCE_ARB: return "perf";
		case GL_DEBUG_TYPE_OTHER_ARB: return "othr";
	}
	return "unknown";
}
static const char *glDebugSeverityToStr(unsigned e)
{
	switch(e)
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB: return "###";
		case GL_DEBUG_SEVERITY_MEDIUM_ARB: return "+++";
		case GL_DEBUG_SEVERITY_LOW_ARB: return "---";
		//case GL_DEBUG_SEVERITY_NOTIFICATION: return "   ";
	}
	return "";
}


static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
	switch(severity)
	{
		case GL_DEBUG_SEVERITY_HIGH_ARB:
		case GL_DEBUG_SEVERITY_MEDIUM_ARB:
		case GL_DEBUG_SEVERITY_LOW_ARB:
		{
			const char *ty = glDebugTypeToStr(type);
			const char *sev = glDebugSeverityToStr(severity);

			std::ostringstream os;
			os << sev << " GL[" << ty << "]: " << message;
			debugLog(os.str());
		}
		break;

		/*case GL_DEBUG_SEVERITY_NOTIFICATION:
			break;
		default:
			assert(false);*/
	}
}

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
PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = NULL;
PFNGLDRAWBUFFERSARBPROC glDrawBuffersARB = NULL;

// ARB_vertex_buffer_object
PFNGLGENBUFFERSARBPROC glGenBuffersARB = NULL;
PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB = NULL;
PFNGLBUFFERDATAARBPROC glBufferDataARB = NULL;
PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB = NULL;
PFNGLBINDBUFFERARBPROC glBindBufferARB = NULL;
PFNGLMAPBUFFERARBPROC glMapBufferARB = NULL;
PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB = NULL;

PFNGLCOPYIMAGESUBDATAEXTPROC glCopyImageSubDataEXT = NULL;

// extern
unsigned g_dbg_numRenderCalls = 0;
bool g_has_GL_GENERATE_MIPMAP = false;
bool g_has_GL_BUFFERS = false;


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
	glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glIsFramebufferEXT");
	glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)SDL_GL_GetProcAddress("glBindFramebufferEXT");
	glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
	glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)SDL_GL_GetProcAddress("glGenFramebuffersEXT");
	glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
	glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
	glDrawBuffersARB          = NULL;//(PFNGLDRAWBUFFERSARBPROC)SDL_GL_GetProcAddress("glDrawBuffersARB");

	// GL 4.3+, but maybe available as an extension
	glCopyImageSubDataEXT     = (PFNGLCOPYIMAGESUBDATAEXTPROC)SDL_GL_GetProcAddress("glCopyImageSubDataEXT");

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

	// VBOs
	glGenBuffersARB           = (PFNGLGENBUFFERSARBPROC)SDL_GL_GetProcAddress("glGenBuffersARB");
	glDeleteBuffersARB        = (PFNGLDELETEBUFFERSARBPROC)SDL_GL_GetProcAddress("glDeleteBuffersARB");
	glBufferDataARB           = (PFNGLBUFFERDATAARBPROC)SDL_GL_GetProcAddress("glBufferData");
	glBufferSubDataARB        = (PFNGLBUFFERSUBDATAARBPROC)SDL_GL_GetProcAddress("glBufferSubData");
	glBindBufferARB           = (PFNGLBINDBUFFERARBPROC)SDL_GL_GetProcAddress("glBindBufferARB");
	glMapBufferARB            = (PFNGLMAPBUFFERARBPROC)SDL_GL_GetProcAddress("glMapBufferARB");
	glUnmapBufferARB          = (PFNGLUNMAPBUFFERARBPROC)SDL_GL_GetProcAddress("glUnmapBufferARB");


#if _DEBUG
	//PFNGLDEBUGMESSAGECONTROLARBPROC glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC)SDL_GL_GetProcAddress("glDebugMessageControlARB");
	PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)SDL_GL_GetProcAddress("glDebugMessageCallbackARB");
	//PFNGLDEBUGMESSAGEINSERTARBPROC glDebugMessageInsertARB = (PFNGLDEBUGMESSAGEINSERTARBPROC)SDL_GL_GetProcAddress("glDebugMessageInsertARB");
	if(glDebugMessageCallbackARB)
	{
		/*glDebugMessageCallbackARB(debugCallback, NULL);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);*/
	}

	//if(glDebugMessageControlARB)
	//	glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, NULL, 0, GL_TRUE);

	//if(glDebugMessageInsertARB)
	//	glDebugMessageInsertARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_OTHER_ARB, 0, GL_DEBUG_SEVERITY_HIGH_ARB, 0, "GL debug test");

#endif

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
	glIsFramebufferEXT = NULL;
	glBindFramebufferEXT = NULL;
	glDeleteFramebuffersEXT = NULL;
	glGenFramebuffersEXT = NULL;
	glCheckFramebufferStatusEXT = NULL;
	glFramebufferTexture2DEXT = NULL;
	glDrawBuffersARB = NULL;

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

	glGenBuffersARB           = NULL;
	glDeleteBuffersARB        = NULL;
	glBufferDataARB           = NULL;
	glBufferSubDataARB        = NULL;
	glBindBufferARB           = NULL;
	glMapBufferARB            = NULL;
	glUnmapBufferARB          = NULL;

}

#endif // BBGE_BUILD_OPENGL_STATIC
