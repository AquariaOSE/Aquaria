#ifndef BBGE_WINDOW_H
#define BBGE_WINDOW_H

// SDL2 and 1.2-compatible window class.
// Note: With SDL1.2, only one window can exist.

union SDL_Event;

class Window
{
public:
	Window();
	virtual ~Window();

	void handleInput();
	// pass -1 to any to leave unchanged
	void open(int w, int h, int full, int bpp, int vsync, int display, int hz);
	void setGrabInput(bool on);
	void present();
	void setFullscreen(bool on);
	void setTitle(const char *s);
	inline bool hasFocus() const { return _hasFocus; }
	int getDisplayIndex() const; // -1 on error/unsupported
	int getRefreshRate() const { return _hz; }
	void warpMouse(int x, int y);
	void updateSize();

	inline bool isFullscreen() const { return _full; }
	bool isOpen() const;
	bool isDesktopResolution() const;
	bool hasInputFocus() const;

protected:

	virtual void onEvent(const SDL_Event& ev);
	virtual void onResize(unsigned w, unsigned h);
	virtual void onQuit();


	static void *_initBackend();
	void _ctor();
	void _fixOpenParams(int& w, int& h, int& full, int& bpp, int& vsync, int& display, int& hz);
	void _open(unsigned w, unsigned h, bool full, unsigned bpp, bool vsync, unsigned display, unsigned hz);
	void _adjust(unsigned w, unsigned h, bool full, unsigned bpp, bool vsync, unsigned display, unsigned hz);
	void _onEventImpl(const SDL_Event& ev);
	void * const _backend; // backend-specific struct
	unsigned _w, _h, _display, _bpp, _hz;
	bool _full, _vsync;
	bool _hasFocus;
};


#endif
