#include "Window.h"
#include <SDL.h>
#include <assert.h>
#include "OSFunctions.h"

#define SDL2_BACKEND SDL_VERSION_ATLEAST(2,0,0)

#if !SDL2_BACKEND // ... to end of file

static Window *s_theWindow; // since SDL1 can only create a single window, keep it around to make sure only one exists.

struct Backend
{
	Backend()
		: win(NULL)
	{}

	SDL_Surface *win;
};

#define BACKEND (static_cast<Backend*>(_backend))
#define WIN (BACKEND->win)

void *Window::_initBackend()
{
	return new Backend;
}

void Window::_ctor()
{
	assert(!s_theWindow);
	s_theWindow = this;
}

Window::~Window()
{
	delete BACKEND;
	s_theWindow = NULL;
}

bool Window::_open(unsigned w, unsigned h, bool full, unsigned bpp, bool vsync, unsigned display, unsigned hz)
{
	// ignored for SDL1
	(void)display;
	(void)hz;

	assert(w && h);

	// have to cast away constness, since SDL_putenv() might be #defined to
	//  putenv(), which takes a (char *), and freaks out newer GCC releases
	//  when you try to pass a (const!) string literal here...  --ryan.
	SDL_putenv((char *) "SDL_VIDEO_CENTERED=1");

	// SDL 1.2 can't set this on an existing context
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	Uint32 flags = SDL_OPENGL;
	if(full)
		flags |= SDL_FULLSCREEN;
	SDL_Surface *surf = SDL_SetVideoMode(w, h, bpp, flags);
	if(!surf)
		return false;

	WIN = surf;
	return true;
}

void Window::warpMouse(int x, int y)
{
	SDL_WarpMouse(x, y);
}

void Window::setGrabInput(bool on)
{
	SDL_WM_GrabInput(on ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

void Window::present()
{
	SDL_GL_SwapBuffers();
}

void Window::setTitle(const char *s)
{
	SDL_WM_SetCaption(s, s);
}

void Window::initIcon()
{
	::initIcon(WIN);
}

int Window::getDisplayIndex() const
{
	return -1;
}

bool Window::isDesktopResolution() const
{
	return false;
}

bool Window::hasInputFocus() const
{
	return ((SDL_GetAppState() & SDL_APPINPUTFOCUS) != 0);
}


#endif // !SDL2_BACKEND
