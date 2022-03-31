#ifndef BBGE_GLLOAD_H
#define BBGE_GLLOAD_H

bool lookup_all_glsyms();
void unload_all_glsyms();

extern unsigned g_dbg_numRenderCalls;

extern bool g_has_GL_GENERATE_MIPMAP; // GL 1.4 and up, later deprecated

#endif
