#include "Window.h"
#include <SDL.h>
#include <assert.h>
#include "OSFunctions.h"
#include "Base.h"

#define SDL2_BACKEND SDL_VERSION_ATLEAST(2,0,0)

#if SDL2_BACKEND

struct Backend
{
	Backend()
		: win(NULL)
		, glctx(NULL)
	{}
	SDL_Window *win;
	SDL_GLContext glctx;
};

#define BACKEND (static_cast<Backend*>(_backend))
#define WIN (BACKEND->win)
#define GLCTX (BACKEND->glctx)

void Window::_ctor()
{
}

void *Window::_initBackend()
{
	return new Backend;
}


Window::~Window()
{
	SDL_GL_MakeCurrent(WIN, NULL);
	SDL_GL_DeleteContext(GLCTX);
	SDL_DestroyWindow(WIN);

	delete BACKEND;
}

bool Window::isOpen() const
{
	return !!WIN;
}

static void setvsync(bool vsync)
{
	if(vsync)
	{
		if(SDL_GL_SetSwapInterval(-1) != 0)
			SDL_GL_SetSwapInterval(1);
	}
	else
		SDL_GL_SetSwapInterval(0);
}

void Window::_open(unsigned w, unsigned h, bool full, unsigned bpp, bool vsync, unsigned display, unsigned hz)
{
	assert(!WIN);
	assert(!GLCTX);
#  ifdef _DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#  endif
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0 );
	SDL_SetHint("SDL_VIDEO_HIGHDPI_DISABLED", "1");

	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if(full)
	{
		if(!w || !h)
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		else
			flags |= SDL_WINDOW_FULLSCREEN;
	}

	unsigned usew = w;
	unsigned useh = h;

	if(!usew || !useh)
	{
		SDL_DisplayMode displaymode;
		if(SDL_GetDesktopDisplayMode(display, &displaymode) == 0)
		{
			usew = displaymode.w;
			useh = displaymode.h;
		}
	}

	int pos = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
	WIN = SDL_CreateWindow("", pos, pos, usew, useh, flags);
	if(!WIN)
		exit_error("Failed to create window");

	::initIcon(WIN);

	GLCTX = SDL_GL_CreateContext(WIN);
	if(!GLCTX)
		exit_error("Failed to create GL context");
	SDL_GL_MakeCurrent(WIN, GLCTX);

	setvsync(vsync);

	if(!full) // When we're in fullscreen mode everything is fine by now
		_adjust(w, h, full, bpp, vsync, display, hz);
}

void Window::_adjust(unsigned w, unsigned h, bool full, unsigned bpp, bool vsync, unsigned display, unsigned hz)
{
	const bool useDesktop = w == 0 || h == 0;

	SDL_DisplayMode displaymode;
	if(useDesktop)
	{
		if(SDL_GetDesktopDisplayMode(display, &displaymode) != 0)
		{
			// Failed to get this; use sane defaults
			displaymode.w = 800;
			displaymode.h = 600;
			displaymode.driverdata = 0;
			displaymode.refresh_rate = 0;
			displaymode.format = 0;
			display = 0;
		}
		w = displaymode.w;
		h = displaymode.h;
	}

	setvsync(vsync);

	if(full)
	{
		int screenflags = useDesktop ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
		// must not be already in fullscreen here, otherwise new display mode doesn't apply properly
		SDL_SetWindowFullscreen(WIN, 0);
		SDL_SetWindowDisplayMode(WIN, &displaymode);
		SDL_SetWindowFullscreen(WIN, screenflags);
	}
	else
	{
		SDL_SetWindowFullscreen(WIN, 0);
		SDL_SetWindowSize(WIN, w, h);
		int center = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
		SDL_SetWindowPosition(WIN, center, center);

		if(useDesktop)
			SDL_MaximizeWindow(WIN);
	}
}

void Window::updateSize()
{
	int ww, hh;
	SDL_GetWindowSize(WIN, &ww, &hh);
	onResize(ww, hh);
}

bool Window::isDesktopResolution() const
{
	return !!(SDL_GetWindowFlags(WIN) & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN_DESKTOP));
}

void Window::setGrabInput(bool on)
{
	SDL_SetWindowGrab(WIN, (SDL_bool)on);
}

void Window::present()
{
	SDL_GL_SwapWindow(WIN);
}

int Window::getDisplayIndex() const
{
	return SDL_GetWindowDisplayIndex(WIN);
}

bool Window::hasInputFocus() const
{
	return (SDL_GetWindowFlags(WIN) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void Window::setTitle(const char *s)
{
	SDL_SetWindowTitle(WIN, s);
}

void Window::warpMouse(int x, int y)
{
	SDL_WarpMouseInWindow(WIN, x, y);
}

void Window::_onEventImpl(const SDL_Event& ev)
{
	switch(ev.type)
	{
		case SDL_KEYDOWN:
#if __APPLE__
		if ((ev.key.keysym.sym == SDLK_q) && (ev.key.keysym.mod & KMOD_GUI))
			onQuit();
#endif
		break;

		case SDL_WINDOWEVENT:
		{
			switch(ev.window.event)
			{
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					_hasFocus = true;
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					_hasFocus = false;
					break;
				case SDL_WINDOWEVENT_CLOSE:
					onQuit();
					break;
				case SDL_WINDOWEVENT_RESIZED:
					onResize(ev.window.data1, ev.window.data2);
					break;
			}
		}
		break;
	}
}


#endif // SDL2_BACKEND
