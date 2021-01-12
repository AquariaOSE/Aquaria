#include "Window.h"
#include <SDL.h>
#include <assert.h>
#include "OSFunctions.h"
#include "Base.h"

#define SDL2_BACKEND SDL_VERSION_ATLEAST(2,0,0)

#if !SDL2_BACKEND // ... to end of file

static Window *s_theWindow; // since SDL1 can only create a single window, keep it around to make sure only one exists.

#define WIN ((SDL_Surface*&)(_backend))

void *Window::_initBackend()
{
	return NULL;
}

void Window::_ctor()
{
	assert(!s_theWindow);
	s_theWindow = this;
}

Window::~Window()
{
	s_theWindow = NULL;
}

bool Window::isOpen() const
{
	return !!WIN;
}

void Window::_open(unsigned w, unsigned h, bool full, unsigned bpp, bool vsync, unsigned display, unsigned hz)
{
	// ignored for SDL1
	(void)display;
	(void)hz;

	if(!w) w = 800;
	if(!h) h = 600;

	// have to cast away constness, since SDL_putenv() might be #defined to
	//  putenv(), which takes a (char *), and freaks out newer GCC releases
	//  when you try to pass a (const!) string literal here...  --ryan.
	SDL_putenv((char *) "SDL_VIDEO_CENTERED=1");

	// SDL 1.2 can't set this on an existing context
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	Uint32 flags = SDL_OPENGL | SDL_ANYFORMAT;
	if(full)
		flags |= SDL_FULLSCREEN;
	SDL_Surface *surf = SDL_SetVideoMode(w, h, bpp, flags);
	if(!surf)
		exit_error("SDL_SetVideoMode failed");

	WIN = surf;

	::initIcon(WIN);
}

void Window::_adjust(unsigned w, unsigned h, bool full, unsigned bpp, bool vsync, unsigned display, unsigned hz)
{
	_open(w, h, full, bpp, vsync, display, hz);
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

void Window::_onEventImpl(const SDL_Event& ev)
{
	switch(ev.type)
	{
		case SDL_KEYDOWN:
		{
#if __APPLE__
			if ((ev.key.keysym.sym == SDLK_q) && (ev.key.keysym.mod & KMOD_META))
#else
			if ((ev.key.keysym.sym == SDLK_F4) && (ev.key.keysym.mod & KMOD_ALT))
#endif
			{
				onQuit();
			}
		}
		break;

		case SDL_VIDEORESIZE:
			onResize(ev.resize.w, ev.resize.h);
			break;

		case SDL_ACTIVEEVENT:
			_hasFocus = ev.active.state;
	}
}

void Window::updateSize()
{
	onResize(WIN->w, WIN->h);
}



#endif // !SDL2_BACKEND
