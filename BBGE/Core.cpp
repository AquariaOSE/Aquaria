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
#include "Core.h"
#include "Texture.h"
#include "AfterEffect.h"
#include "Particles.h"
#include "GLLoad.h"
#include "RenderBase.h"
#include "Window.h"

#include <time.h>
#include <iostream>
#include <fstream>

#ifdef BBGE_BUILD_UNIX
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <assert.h>

#if __APPLE__
#include <Carbon/Carbon.h>
#endif

#if BBGE_BUILD_WINDOWS
#include <direct.h>
#endif

#ifdef BBGE_BUILD_VFS
#include "ttvfs.h"
#endif

Core *core = 0;

static 	std::ofstream _logOut;

CoreWindow::~CoreWindow()
{
}

void CoreWindow::onResize(unsigned w, unsigned h)
{
	core->updateWindowDrawSize(w, h);
}

void CoreWindow::onQuit()
{
	// smooth
	//core->quitNestedMain();
	//core->quit();

	// instant
	SDL_Quit();
	_exit(0);
}

void CoreWindow::onEvent(const SDL_Event& ev)
{
	core->onEvent(ev);
}

void Core::resetCamera()
{
	cameraPos = Vector(0,0);
}

ParticleEffect* Core::createParticleEffect(const std::string &name, const Vector &position, int layer, float rotz)
{
	ParticleEffect *e = new ParticleEffect();
	e->load(name);
	e->position = position;
	e->start();
	e->setDie(true);
	e->rotation.z = rotz;
	core->getTopStateData()->addRenderObject(e, layer);
	return e;
}

void Core::unloadDevice()
{
	for (size_t i = 0; i < renderObjectLayers.size(); i++)
	{
		RenderObjectLayer *r = &renderObjectLayers[i];
		RenderObject *robj = r->getFirst();
		while (robj)
		{
			robj->unloadDevice();
			robj = r->getNext();
		}
	}
	frameBuffer.unloadDevice();

	if (afterEffectManager)
		afterEffectManager->unloadDevice();
}

void Core::reloadDevice()
{
	for (size_t i = 0; i < renderObjectLayers.size(); i++)
	{
		RenderObjectLayer *r = &renderObjectLayers[i];
		r->reloadDevice();
		RenderObject *robj = r->getFirst();
		while (robj)
		{
			robj->reloadDevice();
			robj = r->getNext();
		}
	}
	frameBuffer.reloadDevice();

	if (afterEffectManager)
		afterEffectManager->reloadDevice();
}

void Core::setup_opengl()
{
	glViewport(0, 0, width, height);

	SDL_ShowCursor(SDL_DISABLE);

	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glClearDepth(1.0);								// Depth Buffer Setup
	glDisable(GL_CULL_FACE);

	glLoadIdentity();

	setClearColor(clearColor);

	frameBuffer.init(-1, -1, true);
	if(afterEffectManager)
		afterEffectManager->updateDevice();
}

void Core::resizeWindow(int w, int h, int full, int bpp, int vsync, int display, int hz)
{
	window->open(w, h, full, bpp, vsync, display, hz);
}

void Core::updateWindowDrawSize(int w, int h)
{
	width = w;
	height = h;
	setup_opengl();
	enable2DWide(w, h);
	reloadDevice();
	resetTimer();
}

void Core::onWindowResize(int w, int h)
{
	updateWindowDrawSize(w, h);

	bool reloadRes = false;
#ifndef BBGE_BUILD_SDL2
	reloadRes = true; // SDL1.2 loses the GL context on resize, so all resources must be reloaded
#endif

	if(reloadRes)
	{
		unloadResources();
		reloadResources();
		resetTimer();
	}

	updateWindowDrawSize(w, h);
}

void Core::setFullscreen(bool full)
{
	//sound->pause();
	window->setFullscreen(full);
	cacheRender();
	resetTimer();
	//sound->resume();
}

RenderObjectLayer *Core::getRenderObjectLayer(int i)
{
	if (i == LR_NONE)
		return 0;
	return &renderObjectLayers[i];
}

bool Core::getShiftState()
{
	return getKeyState(KEY_LSHIFT) || getKeyState(KEY_RSHIFT);
}

bool Core::getAltState()
{
	return getKeyState(KEY_LALT) || getKeyState(KEY_RALT);
}

bool Core::getCtrlState()
{
	return getKeyState(KEY_LCONTROL) || getKeyState(KEY_RCONTROL);
}

void Core::errorLog(const std::string &s)
{
	messageBox("Error!", s);
	debugLog(s);
}

void Core::messageBox(const std::string &title, const std::string &msg)
{
	::messageBox(title, msg);
}

void Core::debugLog(const std::string &s)
{
	if (debugLogActive)
	{
		_logOut << s << std::endl;
	}
#ifdef _DEBUG
	std::cout << s << std::endl;
#endif
}
static bool checkWritable(const std::string& path, bool warn, bool critical)
{
	bool writeable = false;
	std::string f = path + "/~chk_wrt.tmp";
	FILE *fh = fopen(f.c_str(), "w");
	if(fh)
	{
		writeable = fwrite("abcdef", 5, 1, fh) == 1;
		fclose(fh);
		unlink(f.c_str());
	}
	if(!writeable)
	{
		if(warn)
		{
			std::ostringstream os;
			os << "Trying to use \"" << path << "\" as user data path, but it is not writeable.\n"
				<< "Please make sure the game is allowed to write to that directory.\n"
				<< "You can move the game to another location and run it there,\n"
				<< "or try running it as administrator, that may help as well.";
			if(critical)
				os << "\n\nWill now exit.";
			errorLog(os.str());
		}
		if(critical)
			exit(1);
	}
	return writeable;
}


Core::Core(const std::string &filesystem, const std::string& extraDataDir, int numRenderLayers, const std::string &appName, int particleSize, std::string userDataSubFolder)
: ActionMapper(), StateManager(), appName(appName)
{
	window = NULL;
	sound = NULL;
	_extraDataDir = extraDataDir;

	if (userDataSubFolder.empty())
		userDataSubFolder = appName;

#if defined(BBGE_BUILD_UNIX)
	const char *envr = getenv("HOME");
	if (envr == NULL)
		envr = ".";  // oh well.
	const std::string home(envr);

	createDir(home);  // just in case.

	// "/home/icculus/.Aquaria" or something. Spaces are okay.
	#ifdef BBGE_BUILD_MACOSX
	const std::string prefix("Library/Application Support/");
	#else
	const std::string prefix(".");
	#endif

	userDataFolder = home + "/" + prefix + userDataSubFolder;
	createDir(userDataFolder);
	debugLogPath = userDataFolder + "/";
	createDir(userDataFolder + "/screenshots");
	std::string prefpath(getPreferencesFolder());
	createDir(prefpath);

#else
	debugLogPath = "";
	userDataFolder = ".";

	#ifdef BBGE_BUILD_WINDOWS
	{
		if(checkWritable(userDataFolder, true, true)) // working dir?
		{
			puts("Using working directory as user directory.");
		}
		// TODO: we may want to use a user-specific path under windows as well
		// if the code below gets actually used, pass 2x false to checkWritable() above.
		// not sure about this right now -- FG
		/*else
		{
			puts("Working directory is not writable...");
			char pathbuf[MAX_PATH];
			if(SHGetSpecialFolderPathA(NULL, &pathbuf[0], CSIDL_APPDATA, 0))
			{
				userDataFolder = pathbuf;
				userDataFolder += '/';
				userDataFolder += userDataSubFolder;
				for(uint32 i = 0; i < userDataFolder.length(); ++i)
					if(userDataFolder[i] == '\\')
						userDataFolder[i] = '/';
				debugLogPath = userDataFolder + "/";
				puts(("Using \"" + userDataFolder + "\" as user directory.").c_str());
				createDir(userDataFolder);
				checkWritable(userDataFolder, true, true);
			}
			else
				puts("Failed to retrieve appdata path, using working dir."); // too bad, but can't do anything about it
		}
		*/
	}
	#endif
#endif

	_logOut.open((debugLogPath + "debug.log").c_str());
	debugLogActive = true;

	debugLogTextures = true;

	grabInput = false;

	srand(time(NULL));
	old_dt = 0;
	current_dt = 0;

	virtualOffX = virtualOffY = 0;

	particleManager = new ParticleManager(particleSize);
	nowTicks = thenTicks = 0;
	lib_graphics = lib_sound = lib_input = false;
	mouseConstraint = false;
	mouseCircle = 0;
	overrideStartLayer = 0;
	overrideEndLayer = 0;
	particlesPaused = false;
	joystickAsMouse = false;
	currentLayerPass = 0;
	flipMouseButtons = 0;
	joystickEnabled = false;
	doScreenshot = false;
	baseCullRadius = 1;
	width = height = 0;
	_lastEnumeratedDisplayIndex = -1;
	afterEffectManagerLayer = 0;
	renderObjectLayers.resize(1);
	invGlobalScale = 1.0;
	invGlobalScaleSqr = 1.0;
	renderObjectCount = 0;
	avgFPS.resize(1);
	minimized = false;
	shuttingDown = false;
	nestedMains = 0;
	afterEffectManager = 0;
	loopDone = false;
	core = this;

	globalResolutionScale = globalScale = Vector(1,1,1);

	initRenderObjectLayers(numRenderLayers);

	initPlatform(filesystem);
}

void Core::initPlatform(const std::string &filesystem)
{
#if defined(BBGE_BUILD_MACOSX) && !defined(BBGE_BUILD_MACOSX_NOBUNDLEPATH)
	// FIXME: filesystem not handled
	CFBundleRef mainBundle = CFBundleGetMainBundle();

	CFURLRef resourcesURL = CFBundleCopyBundleURL(mainBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
	{
		// error!
		debugLog("CFURLGetFileSystemRepresentation");
	}
	CFRelease(resourcesURL);
	debugLog(path);
	chdir(path);
#elif defined(BBGE_BUILD_UNIX)
	if (!filesystem.empty())
	{
		if (chdir(filesystem.c_str()) == 0)
			return;
		else
			debugLog("Failed to chdir to filesystem path " + filesystem);
	}
#ifdef BBGE_DATA_PREFIX
	if (chdir(BBGE_DATA_PREFIX) == 0 && chdir(appName.c_str()) == 0)
		return;
	else
		debugLog("Failed to chdir to filesystem path " BBGE_DATA_PREFIX + appName);
#endif
	char path[PATH_MAX];
	// always a symlink to this process's binary, on modern Linux systems.
	const ssize_t rc = readlink("/proc/self/exe", path, sizeof (path));
	if ( (rc == -1) || (rc >= (ssize_t) sizeof (path)) )
	{
		// error!
		debugLog("readlink");
	}
	else
	{
		path[rc] = '\0';
		char *ptr = strrchr(path, '/');
		if (ptr != NULL)
		{
			*ptr = '\0';
			debugLog(path);
			if (chdir(path) != 0)
				debugLog("Failed to chdir to executable path" + std::string(path));
		}
	}
#endif
#ifdef BBGE_BUILD_WINDOWS
	if(filesystem.length())
	{
		if(_chdir(filesystem.c_str()) != 0)
		{
			debugLog("chdir failed: " + filesystem);
		}
	}
	// FIXME: filesystem not handled
#endif
}

std::string Core::getPreferencesFolder()
{
#ifdef BBGE_BUILD_UNIX
	return userDataFolder + "/preferences";
#endif
#ifdef BBGE_BUILD_WINDOWS
	return "";
#endif
}

std::string Core::getUserDataFolder()
{
	return userDataFolder;
}

Core::~Core()
{
	if (particleManager)
	{
		delete particleManager;
	}
	if (sound)
	{
		delete sound;
		sound = 0;
	}
	debugLog("~Core()");
	_logOut.close();
	core = 0;
}

void Core::updateInputGrab()
{
	// Can and MUST always ungrab if window is not in focus
	const bool on = grabInput && isWindowFocus();
	window->setGrabInput(on);
}

void Core::setInputGrab(bool on)
{
	grabInput = on;
	updateInputGrab();
}

bool Core::isFullscreen()
{
	return window->isFullscreen();
}

bool Core::isDesktopResolution()
{
	return window->isDesktopResolution();
}

int Core::getDisplayIndex()
{
	return window->getDisplayIndex();
}

int Core::getRefreshRate()
{
	return window->getRefreshRate();
}

bool Core::isShuttingDown()
{
	return shuttingDown;
}

void Core::init()
{
	setupFileAccess();

	unsigned sdlflags = SDL_INIT_EVERYTHING & ~SDL_INIT_TIMER;

	quitNestedMainFlag = false;
#ifdef BBGE_BUILD_SDL2
	// Haptic is inited separately, in Jostick.cpp, when a joystick is actually plugged in
	sdlflags &= ~SDL_INIT_HAPTIC;
#else
	// Disable relative mouse motion at the edges of the screen, which breaks
	// mouse control for absolute input devices like Wacom tablets and touchscreens.
	SDL_putenv((char *) "SDL_MOUSE_RELATIVE=0");
#endif

	// Haptic is inited separately, in Jostick.cpp, when a joystick is actually plugged in
	if((SDL_Init(sdlflags))==-1)
	{
		std::string msg("Failed to init SDL: ");
		msg.append(SDL_GetError());
		exit_error(msg);
	}

	loopDone = false;

	initLocalization();
}

void Core::initRenderObjectLayers(int num)
{
	renderObjectLayers.resize(num);
	renderObjectLayerOrder.resize(num);
	for (int i = 0; i < num; i++)
	{
		renderObjectLayerOrder[i] = i;
	}
}

bool Core::initSoundLibrary(const std::string &defaultDevice)
{
	debugLog("Creating SoundManager");
	sound = new SoundManager(defaultDevice);
	debugLog("Done");
	return sound != 0;
}

Vector Core::getGameCursorPosition()
{
	return getGamePosition(mouse.position);
}

Vector Core::getGamePosition(const Vector &v)
{
	return cameraPos + (v * invGlobalScale);
}

bool Core::getKeyState(unsigned k) const
{
	return rawInput.isKeyPressed(k);
}

void Core::initJoystickLibrary()
{
#ifndef BBGE_BUILD_SDL2
	detectJoysticks();
#endif

	joystickEnabled = true;
}

void Core::clearJoysticks()
{
	for(size_t i = 0; i < joysticks.size(); ++i)
		delete joysticks[i];
	joysticks.clear();
}


// Only used for SDL 1.2 code path.
// SDL2 automatically fires joystick added events upon startup
void Core::detectJoysticks()
{
	clearJoysticks();

	std::ostringstream os;
	const unsigned n = SDL_NumJoysticks();
	os << "Found [" << n << "] joysticks";
	debugLog(os.str());

	joysticks.reserve(n);
	for(unsigned i = 0; i < n; ++i)
	{
		Joystick *j = new Joystick;
		if(j->init(i))
			joysticks.push_back(j);
		else
			delete j;
	}
}

bool Core::initInputLibrary()
{
	core->mouse.position = Vector(getWindowWidth()/2, getWindowHeight()/2);
	return true;
}

void Core::onUpdate(float dt)
{
	if (minimized) return;

	core->mouse.lastPosition = core->mouse.position;

	pollEvents(dt);

	ActionMapper::onUpdate(dt);
	StateManager::onUpdate(dt);

	mouse.update(dt);
	onMouseInput();

	globalScale.update(dt);
	core->globalScaleChanged();

	if (afterEffectManager)
	{
		afterEffectManager->update(dt);
	}
}

void Core::globalScaleChanged()
{
	invGlobalScale = 1.0f/globalScale.x;
	invGlobalScaleSqr = invGlobalScale * invGlobalScale;
}

void Core::setClearColor(const Vector &c)
{
	glClearColor(c.x, c.y, c.z, 0.0);
	clearColor = c;
}

void Core::initGraphicsLibrary(int width, int height, bool fullscreen, bool vsync, int bpp, int display, int hz)
{
	if(!window)
		window = new CoreWindow;

	window->open(width, height, fullscreen, bpp, vsync, display, hz);
	window->setTitle(appName.c_str());

	// get GL symbols AFTER opening the window, otherwise we get a super old GL context on windows and nothing works
	if (!lookup_all_glsyms())
	{
		std::ostringstream os;
		os << "Couldn't load OpenGL symbols we need\n";
		SDL_Quit();
		exit_error(os.str());
	}

	debugLog("GL vendor, renderer & version:");
	debugLog((const char*)glGetString(GL_VENDOR));
	debugLog((const char*)glGetString(GL_RENDERER));
	debugLog((const char*)glGetString(GL_VERSION));

	enumerateScreenModes(window->getDisplayIndex());

	window->initSize();
	cacheRender(); // Clears the window bg to black early; prevents flickering
	lib_graphics = true;
}

void Core::enumerateScreenModesIfNecessary(int display /* = -1 */)
{
	if(display == -1)
	{
#ifdef BBGE_BUILD_SDL2
		if(window)
			display = window->getDisplayIndex();
		else
#endif
			display = 0;
	}
	if(_lastEnumeratedDisplayIndex == display)
		return;

	enumerateScreenModes(display);
}

void Core::enumerateScreenModes(int display)
{
	_lastEnumeratedDisplayIndex = display;
	screenModes.clear();

#ifdef BBGE_BUILD_SDL2
	screenModes.push_back(ScreenMode(0, 0, 0)); // "Desktop" screen mode

	SDL_DisplayMode mode;
	const int modecount = SDL_GetNumDisplayModes(display);
	if(modecount == 0){
		debugLog("No modes available!");
		return;
	}

	for (int i = 0; i < modecount; i++) {
		SDL_GetDisplayMode(display, i, &mode);
		if (mode.w && mode.h && (mode.w > mode.h))
		{
			screenModes.push_back(ScreenMode(mode.w, mode.h, mode.refresh_rate));
		}
	}

#else
	SDL_Rect **modes;
	int i;

	modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

	if(modes == (SDL_Rect **)0){
		debugLog("No modes available!");
		return;
	}

	if(modes == (SDL_Rect **)-1){
		debugLog("All resolutions available.");
	}
	else{
		int c=0;
		for(i=0;modes[i];++i){
			c++;
		}
		for (i=c-1;i>=0;i--)
		{
			if (modes[i]->w > modes[i]->h)
			{
				screenModes.push_back(ScreenMode(i, modes[i]->w, modes[i]->h));
			}
		}
	}
#endif

	std::ostringstream os;
	os << "Screen modes available: " << screenModes.size();
	debugLog(os.str());
}

void Core::shutdownSoundLibrary()
{
}

void Core::shutdownGraphicsLibrary()
{
	setInputGrab(false);

	glFinish();

	delete window;
	window = NULL;
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	unload_all_glsyms();

	lib_graphics = false;

	destroyIcon();
}

void Core::quit()
{
	enqueueJumpState("STATE_QUIT");
}

void Core::applyState(const std::string &state)
{
	if (nocasecmp(state, "state_quit")==0)
	{
		loopDone = true;
	}
	StateManager::applyState(state);
}

void Core::setPixelScale(int pixelScaleX, int pixelScaleY)
{
	virtualWidth = pixelScaleX;
	virtualHeight = pixelScaleY;	//assumes 4:3 aspect ratio
	this->baseCullRadius = 1.1f * sqrtf(sqr(getVirtualWidth()/2) + sqr(getVirtualHeight()/2));

	std::ostringstream os;
	os << "virtual(" << virtualWidth << ", " << virtualHeight << ")";
	debugLog(os.str());

	center = Vector(baseVirtualWidth/2, baseVirtualHeight/2);

	int diffw = virtualWidth-baseVirtualWidth;
	if (diffw > 0)
		virtualOffX = ((virtualWidth-baseVirtualWidth)/2);
	else
		virtualOffX = 0;

	int diffh = virtualHeight-baseVirtualHeight;
	if (diffh > 0)
		virtualOffY = ((virtualHeight-baseVirtualHeight)/2);
	else
		virtualOffY = 0;
}

void Core::enable2DWide(int rx, int ry)
{
	float aspect = float(rx) / float(ry);
	if (aspect >= 1.3f)
	{
		int vw = int(float(baseVirtualHeight) * (float(rx)/float(ry)));
		enable2D(vw, baseVirtualHeight);
	}
	else
	{
		int vh = int(float(baseVirtualWidth) * (float(ry)/float(rx)));
		enable2D(baseVirtualWidth, vh);
	}
}

static void bbgeOrtho2D(float left, float right, float bottom, float top)
{
	glOrtho(left, right, bottom, top, -1.0, 1.0);
}

void Core::enable2D(int pixelScaleX, int pixelScaleY)
{
	assert(pixelScaleX && pixelScaleY);

	GLint viewPort[4];
	glGetIntegerv(GL_VIEWPORT, viewPort);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float vw=0,vh=0;

	int viewOffX = 0;
	int viewOffY = 0;

	float aspect = float(width)/float(height);

	if (aspect >= 1.4f)
	{
		vw = float(baseVirtualWidth * viewPort[3]) / float(baseVirtualHeight);
		viewOffX = (viewPort[2] - vw) * 0.5f;
	}
	else if (aspect < 1.3f)
	{
		vh = float(baseVirtualHeight * viewPort[2]) / float(baseVirtualWidth);
		viewOffY = (viewPort[3] - vh) * 0.5f;
	}

	bbgeOrtho2D(0.0f-viewOffX,viewPort[2]-viewOffX,viewPort[3]-viewOffY,0.0f-viewOffY);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	setupRenderPositionAndScale();

	float widthFactor = core->width/float(pixelScaleX);
	float heightFactor = core->height/float(pixelScaleY);
	core->globalResolutionScale = Vector(widthFactor,heightFactor,1.0f);
	setPixelScale(pixelScaleX, pixelScaleY);
}

void Core::quitNestedMain()
{
	if (getNestedMains() > 1)
	{
		quitNestedMainFlag = true;
	}
}

void Core::resetTimer()
{
	nowTicks = thenTicks = SDL_GetTicks();

	for (size_t i = 0; i < avgFPS.size(); i++)
	{
		avgFPS[i] = 0;
	}
}

void Core::setMousePosition(const Vector &p)
{
	core->mouse.position = p;
	float px = p.x + virtualOffX;
	float py = p.y;

	window->warpMouse(
		px * (float(width)/float(virtualWidth)),
		py * (float(height)/float(virtualHeight))
	);
}

// used to update all render objects either uniformly or as part of a time sliced update process
void Core::updateRenderObjects(float dt)
{
	for (size_t c = 0; c < renderObjectLayers.size(); c++)
	{

		RenderObjectLayer *rl = &renderObjectLayers[c];

		if (!rl->update)
			continue;

		for (RenderObject *r = rl->getFirst(); r; r = rl->getNext())
		{
			r->update(dt);
		}
	}

	if (loopDone)
		return;
}

std::string Core::getEnqueuedJumpState()
{
	return this->enqueuedJumpState;
}

int screenshotNum = 0;
std::string getScreenshotFilename(bool png)
{
	std::string prefix = core->getUserDataFolder() + "/screenshots/screen";
	std::string ext = png ? ".png" : ".tga";
	while (true)
	{
		std::ostringstream os;
		os << prefix << screenshotNum << ext;
		screenshotNum ++;
		std::string str(os.str());
		if (!core->exists(str))  // keep going until we hit an unused filename.
			return str;
	}
}

unsigned Core::getTicks()
{
	return SDL_GetTicks();
}

bool Core::isWindowFocus()
{
	return window->hasInputFocus();
}

void Core::onBackgroundUpdate()
{
	SDL_Delay(200);
}

void Core::run(float runTime)
{
	// cannot nest loops when the game is over
	if (loopDone) return;



	float dt;
	float counter = 0;
	int frames = 0;

#if !defined(_DEBUG)
	bool wasInactive = false;
#endif

	nowTicks = thenTicks = SDL_GetTicks();
	nestedMains++;

	while((runTime == -1 && !loopDone) || (runTime >0))
	{
		BBGE_PROF(Core_main);

		nowTicks = SDL_GetTicks();
		dt = (nowTicks-thenTicks)/1000.0;
		thenTicks = nowTicks;

		if (!avgFPS.empty())
		{

			size_t i = 0;
			for (i = avgFPS.size()-1; i > 0; i--)
			{
				avgFPS[i] = avgFPS[i-1];
			}
			avgFPS[0] = dt;

			float c=0;
			int n = 0;
			for (i = 0; i < avgFPS.size(); i++)
			{
				if (avgFPS[i] > 0)
				{
					c += avgFPS[i];
					n ++;
				}
			}
			if (n > 0)
			{
				c /= n;
				dt = c;
			}

		}

#if !defined(_DEBUG)

		if (lib_graphics && (wasInactive || !settings.runInBackground))
		{
			if (isWindowFocus())
			{
				if (wasInactive)
				{
					debugLog("WINDOW ACTIVE");
					updateInputGrab();
					wasInactive = false;
				}
			}
			else
			{
				if (!wasInactive)
					debugLog("WINDOW INACTIVE");

				wasInactive = true;
				updateInputGrab();
				sound->pause();

				while (!isWindowFocus())
				{
					pollEvents(dt);

					onBackgroundUpdate();

					resetTimer();
				}

				debugLog("app back in focus");

				resetTimer();

				sound->resume();

				resetTimer();

				SDL_ShowCursor(SDL_DISABLE);

				continue;
			}
		}
#endif

		old_dt = dt;

		modifyDt(dt);

		current_dt = dt;

		if (quitNestedMainFlag)
		{
			quitNestedMainFlag = false;
			break;
		}
		if (runTime>0)
		{
			runTime -= dt;
			if (runTime < 0)
				runTime = 0;
		}

		// UPDATE
		postProcessingFx.update(dt);

		updateRenderObjects(dt);

		if (particleManager)
			particleManager->update(dt);

		sound->update(dt);

		onUpdate(dt);

		if (nestedMains == 1)
			clearGarbage();

		if (loopDone)
			break;

		updateCullData();

		g_dbg_numRenderCalls = 0;

		if (settings.renderOn)
		{
			if (darkLayer.isUsed())
			{
				darkLayer.preRender();
			}

			render();

			showBuffer();

			BBGE_PROF(STOP);

			if (nestedMains == 1)
				clearGarbage();

			frames++;

			counter += dt;
			if (counter > 1)
			{
				fps = frames;
				frames = counter = 0;
			}
		}

		sound->setListenerPos(screenCenter.x, screenCenter.y);

		if (doScreenshot)
		{
			doScreenshot = false;
			const bool png = true;
			saveScreenshot(getScreenshotFilename(png), png);
			prepScreen(0);
			resetTimer();
		}
	}
	quitNestedMainFlag = false;
	if (nestedMains==1)
		clearGarbage();
	nestedMains--;
}

void Core::clearBuffers()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
}

void Core::setupRenderPositionAndScale()
{
	glScalef(globalScale.x*globalResolutionScale.x, globalScale.y*globalResolutionScale.y, globalScale.z*globalResolutionScale.z);
	glTranslatef(-(cameraPos.x+cameraOffset.x), -(cameraPos.y+cameraOffset.y), -(cameraPos.z+cameraOffset.z));
}

void Core::setupGlobalResolutionScale()
{
	glScalef(globalResolutionScale.x, globalResolutionScale.y, globalResolutionScale.z);
}

void Core::setMouseConstraint(bool on)
{

	mouseConstraint = on;
}

void Core::setMouseConstraintCircle(const Vector& pos, float circle)
{
	mouseConstraint = true;
	mouseCircle = circle;
	mouseConstraintCenter = pos;
	mouseConstraintCenter.z = 0;
}



int Core::getVirtualOffX()
{
	return virtualOffX;
}

int Core::getVirtualOffY()
{
	return virtualOffY;
}

void Core::centerMouse()
{
	setMousePosition(Vector((virtualWidth/2) - core->getVirtualOffX(), virtualHeight/2));
}

bool Core::doMouseConstraint()
{
	if (mouseConstraint)
	{
		Vector h = mouseConstraintCenter;
		Vector d = mouse.position - h;
		if (!d.isLength2DIn(mouseCircle))
		{
			d.setLength2D(mouseCircle);
			mouse.position = h+d;

			return true;
		}
	}
	return false;
}

void Core::onEvent(const SDL_Event& event)
{
	const bool focus = window->hasFocus();

	if(focus)
		InputSystem::handleSDLEvent(&event);

	switch (event.type)
	{
		case SDL_KEYDOWN:
		{
			if ((event.key.keysym.sym == SDLK_g) && (event.key.keysym.mod & KMOD_CTRL))
			{
				setInputGrab(!grabInput);
			}
		}
		break;

		case SDL_MOUSEMOTION:
		{
			if (focus)
			{
				mouse.lastPosition = mouse.position;

				mouse.position.x = ((event.motion.x) * (float(virtualWidth)/float(getWindowWidth()))) - getVirtualOffX();
				mouse.position.y = event.motion.y * (float(virtualHeight)/float(getWindowHeight()));

				mouse.change = mouse.position - mouse.lastPosition;

				if (doMouseConstraint())
					setMousePosition(mouse.position);
			}
		}
		break;

#ifdef BBGE_BUILD_SDL2
		case SDL_JOYDEVICEADDED:
			onJoystickAdded(event.jdevice.which);
			break;

		case SDL_JOYDEVICEREMOVED:
			onJoystickRemoved(event.jdevice.which);
			break;
#endif
	}
}

void Core::pollEvents(float dt)
{
	window->handleInput();

	for(size_t i = 0; i < joysticks.size(); ++i)
		if(joysticks[i])
			joysticks[i]->update(dt);

	// all input done; update button states
	rawInput.update();
}

#define _VLN(x, y, x2, y2) glVertex2f(x, y); glVertex2f(x2, y2);

void Core::print(int x, int y, const char *str, float sz)
{



	glBindTexture(GL_TEXTURE_2D, 0);

	glPushMatrix();


	float xx = x;
	glTranslatef(x, y-0.5f*sz, 0);
	x = y = 0;
	xx = 0;
	int c=0;


	glLineWidth(1);
	glScalef(sz*0.75f, sz, 1);

	glBegin(GL_LINES);

	while (str[c] != '\0')
	{
		switch(toupper(str[c]))
		{
		case '_':
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '-':
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
		break;
		case '~':
			_VLN(xx, y+0.5f, xx+0.25f, y+0.4f)
			_VLN(xx+0.25f, y+0.4f, xx+0.75f, y+0.6f)
			_VLN(xx+0.75f, y+0.6f, xx+1, y+0.5f)
		break;
		case 'A':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
		break;
		case 'B':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'C':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'D':
			_VLN(xx, y, xx+1, y+0.2f)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx+1, y+0.2f, xx+1, y+1)
		break;
		case 'E':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'F':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
		break;
		case 'G':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx+1, y+0.5f, xx+1, y+1)
		break;
		case 'H':
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y, xx+1, y+1)
		break;
		case 'I':
			_VLN(xx+0.5f, y, xx+0.5f, y+1)
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'J':
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y+1, xx, y+0.75f)
		break;
		case 'K':
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.25f, xx+1, y)
			_VLN(xx, y+0.25f, xx+1, y+1)
		break;
		case 'L':
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'M':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx+0.5f, y+0.5f)
			_VLN(xx+1, y, xx+0.5f, y+0.5f)
		break;
		case 'N':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx+1, y+1)
		break;
		case 'O':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y, xx+1, y)
		break;
		case 'P':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y)
		break;
		case 'Q':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+0.5f, xx+1.25f, y+1.25f)
		break;
		case 'R':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y)
			_VLN(xx, y+0.5f, xx+1, y+1)
		break;
		case 'S':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+0.5f)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case 'T':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+0.5f, y, xx+0.5f, y+1)
		break;
		case 'U':
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
		break;
		case 'V':
			_VLN(xx, y, xx+0.5f, y+1)
			_VLN(xx+1, y, xx+0.5f, y+1)
		break;
		case 'W':
			_VLN(xx, y, xx+0.25f, y+1)
			_VLN(xx+0.25f, y+1, xx+0.5f, y+0.5f)
			_VLN(xx+0.5f, y+0.5f, xx+0.75f, y+1)
			_VLN(xx+1, y, xx+0.75f, y+1)
		break;
		case 'X':
			_VLN(xx, y, xx+1, y+1)
			_VLN(xx+1, y, xx, y+1)
		break;
		case 'Y':
			_VLN(xx, y, xx+0.5f, y+0.5f)
			_VLN(xx+1, y, xx+0.5f, y+0.5f)
			_VLN(xx+0.5f, y+0.5f, xx+0.5f, y+1)
		break;
		case 'Z':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+1, xx+1, y)
			_VLN(xx, y+1, xx+1, y+1)
		break;

		case '1':
			_VLN(xx+0.5f, y, xx+0.5f, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx+0.5f, y, xx+0.25f, y+0.25f)
		break;
		case '2':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+0.5f)
			_VLN(xx+1, y+0.5f, xx, y+0.5f)
			_VLN(xx, y+0.5f, xx, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '3':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx+1, y, xx+1, y+1)
		break;
		case '4':
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx+1, y, xx, y+0.5f)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
		break;
		case '5':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+0.5f)
			_VLN(xx+1, y+0.5f, xx, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '6':
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y+0.5f, xx, y+0.5f)
			_VLN(xx+1, y+0.5f, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '7':
			_VLN(xx+1, y, xx+0.5f, y+1)
			_VLN(xx, y, xx+1, y)
		break;
		case '8':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y, xx, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx, y+1, xx+1, y+1)
		break;
		case '9':
			_VLN(xx, y, xx+1, y)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y+0.5f, xx+1, y+0.5f)
			_VLN(xx, y+0.5f, xx, y)
		break;
		case '0':
			_VLN(xx, y, xx, y+1)
			_VLN(xx+1, y, xx+1, y+1)
			_VLN(xx, y+1, xx+1, y+1)
			_VLN(xx, y, xx+1, y)
			_VLN(xx, y, xx+1, y+1)
		break;
		case '.':
			_VLN(xx+0.4f, y+1, xx+0.6f, y+1)
		break;
		case ',':
			_VLN(xx+0.5f, y+0.75f, xx+0.5f, y+1.0f);
			_VLN(xx+0.5f, y+1.0f, xx+0.2f, y+1.25f);
		break;
		case ' ':
		break;
		case '(':
		case '[':
			_VLN(xx, y, xx, y+1);
			_VLN(xx, y, xx+0.25f, y);
			_VLN(xx, y+1, xx+0.25f, y+1);
		break;
		case ')':
		case ']':
			_VLN(xx+1, y, xx+1, y+1);
			_VLN(xx+1, y, xx+0.75f, y);
			_VLN(xx+1, y+1, xx+0.75f, y+1);
		break;
		case ':':
			_VLN(xx+0.5f, y, xx+0.5f, y+0.25f);
			_VLN(xx+0.5f, y+0.75f, xx+0.5f, y+1);
		break;
		case '/':
			_VLN(xx, y+1, xx+1, y);
		break;
		default:

		break;
		}
		c++;
		xx += 1.4f;
	}
	glEnd();

	glPopMatrix();


}

void Core::cacheRender()
{
	render();
	// what if the screen was full white? then you wouldn't want to clear buffers
	//clearBuffers();
	showBuffer();
	resetTimer();
}

void Core::updateCullData()
{
	cullRadius = baseCullRadius * invGlobalScale;
	cullRadiusSqr = cullRadius * cullRadius;
	screenCenter = cullCenter = cameraPos + Vector(400.0f*invGlobalScale,300.0f*invGlobalScale);
}

void Core::render(int startLayer, int endLayer, bool useFrameBufferIfAvail)
{

	BBGE_PROF(Core_render);


	if (startLayer == -1 && endLayer == -1 && overrideStartLayer != 0)
	{
		startLayer = overrideStartLayer;
		endLayer = overrideEndLayer;
	}

	globalScaleChanged();

	if (core->minimized) return;
	onRender();

	RenderObject::lastTextureApplied = 0;

	updateCullData();



	renderObjectCount = 0;
	processedRenderObjectCount = 0;
	totalRenderObjectCount = 0;


	glBindTexture(GL_TEXTURE_2D, 0);
	glLoadIdentity();									// Reset The View
	clearBuffers();

	if (afterEffectManager && frameBuffer.isInited() && useFrameBufferIfAvail)
	{
		frameBuffer.startCapture();
	}

	setupRenderPositionAndScale();



	RenderObject::rlayer = 0;

	for (size_t c = 0; c < renderObjectLayerOrder.size(); c++)

	{
		int i = renderObjectLayerOrder[c];
		if (i == -1) continue;
		if ((startLayer != -1 && endLayer != -1) && (i < startLayer || i > endLayer)) continue;

		if (i == postProcessingFx.layer)
		{
			postProcessingFx.preRender();
		}
		if (i == postProcessingFx.renderLayer)
		{
			postProcessingFx.render();
		}

		if (darkLayer.isUsed() )
		{

			if (i == darkLayer.getRenderLayer())
			{
				darkLayer.render();
			}

			if (i == darkLayer.getLayer() && startLayer != i)
			{
				continue;
			}
		}

		if (afterEffectManager && afterEffectManager->active && i == afterEffectManagerLayer)
		{
			afterEffectManager->render();
		}

		RenderObjectLayer *r = &renderObjectLayers[i];
		RenderObject::rlayer = r;
		if (r->visible)
		{
			if (r->startPass == r->endPass)
			{
				r->renderPass(RenderObject::RENDER_ALL);
			}
			else
			{
				for (int pass = r->startPass; pass <= r->endPass; pass++)
				{
					r->renderPass(pass);
				}
			}
		}
	}


}

void Core::showBuffer()
{
	window->present();
}

// WARNING: only for use during shutdown
// otherwise, textures will try to remove themselves
// when destroy is called on them
void Core::clearResources()
{
	if(resources.size())
	{
		debugLog("Warning: The following resources were not cleared:");
		for(size_t i = 0; i < resources.size(); ++i)
			debugLog(resources[i]->name);
		resources.clear(); // nothing we can do; refcounting is messed up
	}
}

void Core::shutdownInputLibrary()
{
}

void Core::shutdownJoystickLibrary()
{
	if (joystickEnabled) {
		clearJoysticks();
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		joystickEnabled = false;
	}
}

void Core::clearRenderObjects()
{
	for (size_t i = 0; i < renderObjectLayers.size(); i++)
	{

		RenderObject *r = renderObjectLayers[i].getFirst();
		while (r)
		{
			if (r)
			{
				removeRenderObject(r, DESTROY_RENDER_OBJECT);
			}
			r = renderObjectLayers[i].getNext();
		}
	}
}

void Core::shutdown()
{
	// pop all the states


	debugLog("Core::shutdown");
	shuttingDown = true;

	debugLog("Shutdown Joystick Library...");
		shutdownJoystickLibrary();
	debugLog("OK");

	debugLog("Shutdown Input Library...");
		shutdownInputLibrary();
	debugLog("OK");

	debugLog("Shutdown All States...");
		popAllStates();
	debugLog("OK");

	debugLog("Clear State Instances...");
		clearStateInstances();
	debugLog("OK");

	debugLog("Clear All Remaining RenderObjects...");
		clearRenderObjects();
	debugLog("OK");

	debugLog("Clear All Resources...");
		clearResources();
	debugLog("OK");


	debugLog("Clear State Objects...");
		clearStateObjects();
	debugLog("OK");

	if (afterEffectManager)
	{
		debugLog("Delete AEManager...");
			delete afterEffectManager;
			afterEffectManager = 0;
		debugLog("OK");
	}


	if (sound)
	{
		debugLog("Shutdown Sound Library...");
			sound->stopAll();
			delete sound;
			sound = 0;
		debugLog("OK");
	}

	debugLog("Dark layer...");
		darkLayer.unloadDevice();
	debugLog("OK");

	debugLog("Core's framebuffer...");
		frameBuffer.unloadDevice();
	debugLog("OK");

	debugLog("Shutdown Graphics Library...");
		shutdownGraphicsLibrary();
	debugLog("OK");



#ifdef BBGE_BUILD_VFS
	debugLog("Unload VFS...");
		vfs.Clear();
	debugLog("OK");
#endif


	debugLog("SDL Quit...");
		SDL_Quit();
	debugLog("OK");
}

//util funcs

bool Core::exists(const std::string &filename)
{
	return ::exists(filename, false); // defined in Base.cpp
}

CountedPtr<Texture> Core::findTexture(const std::string &name)
{
	int sz = resources.size();
	for (int i = 0; i < sz; i++)
	{
		//out << resources[i]->name << " is " << name << " ?" << std::endl;
		//NOTE: ensure all names are lowercase before this point
		if (resources[i]->name == name)
		{
			return resources[i];
		}
	}
	return 0;
}

// This handles unix/win32 relative paths: ./rel/path
// Unix abs paths: /home/user/...
// Win32 abs paths: C:/Stuff/.. and also C:\Stuff\...
#define ISPATHROOT(x) (x[0] == '.' || x[0] == '/' || ((x).length() > 1 && x[1] == ':'))

std::string Core::getTextureLoadName(const std::string &texture)
{
	std::string loadName = texture;

	if (texture.empty() || !ISPATHROOT(texture))
	{
		if (texture.find(baseTextureDirectory) == std::string::npos)
			loadName = baseTextureDirectory + texture;
	}
	return loadName;
}

CountedPtr<Texture> Core::doTextureAdd(const std::string &texture, const std::string &loadName, std::string internalTextureName)
{
	if (texture.empty() || !ISPATHROOT(texture))
	{
		if (texture.find(baseTextureDirectory) != std::string::npos)
			internalTextureName = internalTextureName.substr(baseTextureDirectory.size(), internalTextureName.size());
	}

	if (internalTextureName.size() > 4)
	{
		if (internalTextureName[internalTextureName.size()-4] == '.')
		{
			internalTextureName = internalTextureName.substr(0, internalTextureName.size()-4);
		}
	}

	stringToLowerUserData(internalTextureName);
	CountedPtr<Texture> t = core->findTexture(internalTextureName);
	if (t)
		return t;

	t = new Texture;
	t->name = internalTextureName;

	if(t->load(loadName))
	{
		std::ostringstream os;
		os << "LOADED TEXTURE FROM DISK: [" << internalTextureName << "] idx: " << resources.size()-1;
		debugLog(os.str());
	}
	else
	{
		t->width = 64;
		t->height = 64;
	}

	return t;
}

CountedPtr<Texture> Core::addTexture(const std::string &textureName)
{
	BBGE_PROF(Core_addTexture);

	if (textureName.empty())
		return NULL;

	CountedPtr<Texture> ptex;
	std::string texture = textureName;
	stringToLowerUserData(texture);
	std::string internalTextureName = texture;
	std::string loadName = getTextureLoadName(texture);

	if (!texture.empty() && texture[0] == '@')
	{
		texture = secondaryTexturePath + texture.substr(1, texture.size());
		loadName = texture;
	}
	else if (!secondaryTexturePath.empty() && texture[0] != '.' && texture[0] != '/')
	{
		std::string t = texture;
		std::string ln = loadName;
		texture = secondaryTexturePath + texture;
		loadName = texture;
		ptex = doTextureAdd(texture, loadName, internalTextureName);
		if (!ptex || ptex->getLoadResult() == TEX_FAILED)
			ptex = doTextureAdd(t, ln, internalTextureName);
	}
	else
		ptex = doTextureAdd(texture, loadName, internalTextureName);

	addTexture(ptex.content());

	if(debugLogTextures)
	{
		if(!ptex || ptex->getLoadResult() != TEX_SUCCESS)
		{
			std::ostringstream os;
			os << "FAILED TO LOAD TEXTURE: [" << internalTextureName << "] idx: " << resources.size()-1;
			debugLog(os.str());
		}
	}
	return ptex;
}

void Core::addRenderObject(RenderObject *o, size_t layer)
{
	if (!o) return;
	o->layer = layer;
	if (layer >= renderObjectLayers.size())
	{
		std::ostringstream os;
		os << "attempted to add render object to invalid layer [" << layer << "]";
		errorLog(os.str());
	}
	renderObjectLayers[layer].add(o);
}

void Core::switchRenderObjectLayer(RenderObject *o, int toLayer)
{
	if (!o) return;
	renderObjectLayers[o->layer].remove(o);
	renderObjectLayers[toLayer].add(o);
	o->layer = toLayer;
}

void Core::unloadResources()
{
	for (size_t i = 0; i < resources.size(); i++)
	{
		resources[i]->unload();
	}
}

void Core::onReloadResources()
{
}

void Core::reloadResources()
{
	for (size_t i = 0; i < resources.size(); i++)
	{
		resources[i]->reload();
	}
	onReloadResources();
}

void Core::addTexture(Texture *r)
{
	for(size_t i = 0; i < resources.size(); ++i)
		if(resources[i] == r)
			return;

	resources.push_back(r);
	if (r->name.empty())
	{
		debugLog("Empty name resource added");
	}
}

void Core::removeTexture(Texture *res)
{
	std::vector<Texture*> copy;
	copy.swap(resources);

	for (size_t i = 0; i < copy.size(); ++i)
	{
		if (copy[i] == res)
		{
			copy[i]->destroy();
			copy[i] = copy.back();
			copy.pop_back();
			break;
		}
	}

	resources.swap(copy);
}

void Core::removeRenderObject(RenderObject *r, RemoveRenderObjectFlag flag)
{
	if (r)
	{
		if (r->layer != LR_NONE && !renderObjectLayers[r->layer].empty())
		{
			renderObjectLayers[r->layer].remove(r);
		}
		if (flag != DO_NOT_DESTROY_RENDER_OBJECT )
		{
			r->destroy();
			delete r;
		}
	}
}

void Core::enqueueRenderObjectDeletion(RenderObject *object)
{
	if (!object->_dead)
	{
		garbage.push_back (object);
		object->_dead = true;
	}
}

void Core::clearGarbage()
{
	BBGE_PROF(Core_clearGarbage);
	// HACK: optimize this (use a list instead of a queue)

	for (RenderObjects::iterator i = garbage.begin(); i != garbage.end(); i++)
	{
		removeRenderObject(*i, DO_NOT_DESTROY_RENDER_OBJECT);

		(*i)->destroy();
	}

	for (RenderObjects::iterator i = garbage.begin(); i != garbage.end(); i++)
	{
		delete *i;
	}

	garbage.clear();
}

bool Core::canChangeState()
{
	return (nestedMains<=1);
}



// Take a screenshot of the specified region of the screen and store it
// in a 32bpp pixel buffer.  delete[] the returned buffer when it's no
// longer needed.
unsigned char *Core::grabScreenshot(int x, int y, int w, int h)
{
	unsigned int size = sizeof(unsigned char) * w * h * 4;
	unsigned char *imageData = new unsigned char[size];

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST); glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST); glDisable(GL_DITHER); glDisable(GL_FOG);
	glDisable(GL_LIGHTING); glDisable(GL_LOGIC_OP);
	glDisable(GL_STENCIL_TEST); glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D); glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
	glPixelTransferi(GL_RED_SCALE, 1); glPixelTransferi(GL_RED_BIAS, 0);
	glPixelTransferi(GL_GREEN_SCALE, 1); glPixelTransferi(GL_GREEN_BIAS, 0);
	glPixelTransferi(GL_BLUE_SCALE, 1); glPixelTransferi(GL_BLUE_BIAS, 0);
	glPixelTransferi(GL_ALPHA_SCALE, 1); glPixelTransferi(GL_ALPHA_BIAS, 0);
	glRasterPos2i(0, 0);
	glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
	glPopAttrib();

	// Force all alpha values to 255.
	unsigned char *c = imageData;
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++, c += 4)
		{
			c[3] = 255;
		}
	}

	return imageData;

}

// Like grabScreenshot(), but grab from the center of the screen.
unsigned char *Core::grabCenteredScreenshot(int w, int h)
{
	return grabScreenshot(core->width/2 - w/2, core->height/2 - h/2, w, h);
}

// takes a screen shot and saves it to a TGA or PNG image
bool Core::saveScreenshot(const std::string &filename, bool png)
{
	int w = getWindowWidth(), h = getWindowHeight();
	unsigned char *imageData = grabCenteredScreenshot(w, h);
	bool ok = png
		? pngSave(filename.c_str(), w, h, imageData)
		: tgaSave(filename.c_str(),w,h,32,imageData);
	delete [] imageData;
	return ok;
}

// saves an array of pixels as a TGA image
bool Core::tgaSave(	const char	*filename,
		short unsigned int	width,
		short unsigned int	height,
		unsigned char	pixelDepth,
		unsigned char	*imageData) {

	unsigned char cGarbage = 0, type,mode,aux;
	short int iGarbage = 0;
	int i;
	FILE *file;

// open file and check for errors
	file = fopen(adjustFilenameCase(filename).c_str(), "wb");
	if (!file)
		return false;

// compute image type: 2 for RGB(A), 3 for greyscale
	mode = pixelDepth / 8;
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;

// write the header
	if (fwrite(&cGarbage, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&cGarbage, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&type, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&iGarbage, sizeof(short int), 1, file) != 1
		|| fwrite(&iGarbage, sizeof(short int), 1, file) != 1
		|| fwrite(&cGarbage, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&iGarbage, sizeof(short int), 1, file) != 1
		|| fwrite(&iGarbage, sizeof(short int), 1, file) != 1
		|| fwrite(&width, sizeof(short int), 1, file) != 1
		|| fwrite(&height, sizeof(short int), 1, file) != 1
		|| fwrite(&pixelDepth, sizeof(unsigned char), 1, file) != 1
		|| fwrite(&cGarbage, sizeof(unsigned char), 1, file) != 1)
	{
		fclose(file);
		return false;
	}

// convert the image data from RGB(A) to BGR(A)
	if (mode >= 3)
	for (i=0; i < width * height * mode ; i+= mode) {
		aux = imageData[i];
		imageData[i] = imageData[i+2];
		imageData[i+2] = aux;
	}

// save the image data
	size_t bytes = width * height * mode;
	bool ok =  fwrite(imageData, 1, bytes, file) == bytes;

	fclose(file);

	return ok;
}

void Core::screenshot()
{
	doScreenshot = true;
}


 #include "DeflateCompressor.h"

 // saves an array of pixels as a TGA image (frees the image data passed in)
bool Core::zgaSave(	const char	*filename,
		short unsigned int	w,
		short unsigned int	h,
		unsigned char	depth,
		unsigned char	*imageData) {

	ByteBuffer::uint8 type,mode,aux, pixelDepth = depth;
	ByteBuffer::uint8 cGarbage = 0;
	ByteBuffer::uint16 iGarbage = 0;
	ByteBuffer::uint16 width = w, height = h;

// open file and check for errors
	FILE *file = fopen(adjustFilenameCase(filename).c_str(), "wb");
	if (file == NULL) {
		return false;
	}

// compute image type: 2 for RGB(A), 3 for greyscale
	mode = pixelDepth / 8;
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;

// convert the image data from RGB(A) to BGR(A)
	if (mode >= 3)
	for (int i=0; i < width * height * mode ; i+= mode) {
		aux = imageData[i];
		imageData[i] = imageData[i+2];
		imageData[i+2] = aux;
	}

	ZlibCompressor z;
	z.SetForceCompression(true);
	z.reserve(width * height * mode + 30);
	z	<< cGarbage
		<< cGarbage
		<< type
		<< iGarbage
		<< iGarbage
		<< cGarbage
		<< iGarbage
		<< iGarbage
		<< width
		<< height
		<< pixelDepth
		<< cGarbage;

	z.append(imageData, width * height * mode);
	z.Compress(3);

	bool ok  = fwrite(z.contents(), 1, z.size(), file) == z.size();

	fclose(file);

	return ok;
}


#ifdef BBGE_BUILD_VFS
#include "ttvfs_zip/VFSZipArchiveLoader.h"
#include "ttvfs.h"
#include "ttvfs_stdio.h"
#endif

void Core::setupFileAccess()
{
#ifdef BBGE_BUILD_VFS
	debugLog("Init VFS...");

	if(!ttvfs::checkCompat())
		exit_error("ttvfs not compatible");

	ttvfs_setroot(&vfs);

	vfs.AddLoader(new ttvfs::DiskLoader);
	vfs.AddArchiveLoader(new ttvfs::VFSZipArchiveLoader);

	vfs.Mount("override", "");

	// If we ever want to read from a container...
	//vfs.AddArchive("aqfiles.zip");

	if(_extraDataDir.length())
	{
		debugLog("Mounting extra data dir: " + _extraDataDir);
		vfs.Mount(_extraDataDir.c_str(), "");
	}

	debugLog("Done");
#endif
}

void Core::initLocalization()
{
	InStream in(localisePath("data/localecase.txt"));
	if(!in)
	{
		debugLog("data/localecase.txt does not exist, using internal locale data");
		return;
	}

	std::string low, up;
	CharTranslationTable trans;
	memset(&trans[0], -1, sizeof(trans));
	while(in)
	{
		in >> low >> up;
		
		trans[(unsigned char)(low[0])] = (unsigned char)up[0];
	}
	initCharTranslationTables(&trans);
}

void Core::onJoystickAdded(int deviceID)
{
	debugLog("Add new joystick");
	Joystick *j = new Joystick;
	j->init(deviceID);
	for(size_t i = 0; i < joysticks.size(); ++i)
		if(!joysticks[i])
		{
			joysticks[i] = j;
			return;
		}
	joysticks.push_back(j);
}

void Core::onJoystickRemoved(int instanceID)
{
	debugLog("Joystick removed");
	for(size_t i = 0; i < joysticks.size(); ++i)
		if(Joystick *j = joysticks[i])
			if(j->getInstanceID() == instanceID)
			{
				delete j;
				joysticks[i] = NULL;
			}
}

Joystick *Core::getJoystick(size_t idx)
{
	size_t i = idx;
	return i < joysticks.size() ? joysticks[i] : NULL;
}

#include <png.h>

bool Core::pngSave(const char *filename, unsigned width, unsigned height, unsigned char *data)
{
	bool ok = true;
	std::vector<png_byte*> rowData(height);
	// passed in buffer is upside down; flip it
	for(unsigned i = 0; i < height; i++)
		rowData[height - i - 1] = i * width * 4 + data;

	FILE *fp = fopen(filename, "wb");
	if (!fp)
		return false;

	png_structp png;
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png)
		goto fail;

	png_infop info_ptr;
	info_ptr = png_create_info_struct(png);
	if (!info_ptr)
		goto fail;

	if (setjmp(png_jmpbuf(png)))
		goto fail;

	png_init_io(png, fp);

	if (setjmp(png_jmpbuf(png)))
		goto fail;

	png_set_IHDR(png, info_ptr, width, height,
		8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png, info_ptr);

	if (setjmp(png_jmpbuf(png)))
		goto fail;

	png_write_image(png, (png_byte**)&rowData[0]);

	if (setjmp(png_jmpbuf(png)))
		goto fail;

	png_write_end(png, NULL);

end:
	if(fp)
		fclose(fp);
	return ok;

fail:
	debugLog("Failed to save PNG");
	ok = false;
	goto end;
}
