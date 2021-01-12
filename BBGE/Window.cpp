#include "Window.h"
#include <SDL.h>
#include "Base.h"

Window::Window()
: _backend(_initBackend())
, _w(800), _h(600)
, _display(0)
, _bpp(32)
, _hz(60)
, _full(false)
, _vsync(false)
, _hasFocus(false)
{
}

void Window::handleInput()
{
	SDL_Event event;
	while ( SDL_PollEvent (&event) ) // This function is the same for SDL1 & 2 ...
	{
		switch(event.type)
		{
			case SDL_QUIT:
				onQuit();
				break;
		}
		_onEventImpl(event);
		onEvent(event);
	}
}

void Window::onEvent(const SDL_Event& ev)
{
}

void Window::onResize(unsigned w, unsigned h)
{
}

void Window::onQuit()
{
}

void Window::setFullscreen(bool on)
{
	if(!!_full != on)
		open(-1, -1, on, -1, -1, -1, -1);
}

void Window::open(int w, int h, int full, int bpp, int vsync, int display, int hz)
{
	_fixOpenParams(w, h, full, bpp, vsync, display, hz);

	_w = w;
	_h = h;
	_display = display;
	_bpp = bpp;
	_full = !!full;
	_vsync = !!vsync;

	if(isOpen())
		_adjust(w, h, !!full, bpp, !!vsync, display, hz);
	else
		_open(w, h, !!full, bpp, !!vsync, display, hz);
}

void Window::_fixOpenParams(int& w, int& h, int& full, int& bpp, int& vsync, int& display, int& hz)
{
	if(w < 0)
		w = _w;
	if(h < 0)
		h = _h;
	if(full < 0)
		full = _full;
	if(bpp < 0)
		bpp = _bpp;
	if(vsync < 0)
		vsync = _vsync;
	if(display < 0)
		display = _display;
	if(hz < 0)
		hz = _hz;
}
