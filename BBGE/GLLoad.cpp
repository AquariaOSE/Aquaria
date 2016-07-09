#include "Base.h"

#if BBGE_BUILD_OPENGL_DYNAMIC

#include "RenderBase.h"
#include "GLLoad.h"
#include <sstream>

#ifdef GLAPIENTRY
#undef GLAPIENTRY
#endif

#ifdef BBGE_BUILD_WINDOWS
#define GLAPIENTRY __stdcall
#else
#define GLAPIENTRY
#endif



unsigned g_dbg_numRenderCalls = 0; // extern


#ifdef BBGE_BUILD_OPENGL_DYNAMIC
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
	return retval;
}
#endif

void unload_all_glsyms()
{
#if BBGE_BUILD_OPENGL_DYNAMIC
// reset all the entry points to NULL, so we know exactly what happened
//  if we call a GL function after shutdown.
#define GL_FUNC(ret,fn,params,call,rt) \
	p##fn = NULL;
#include "OpenGLStubs.h"
#undef GL_FUNC
#endif
}

#endif
