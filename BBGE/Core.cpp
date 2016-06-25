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

#include <time.h>
#include <iostream>

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
#include <shlobj.h>
#include <direct.h>
#endif

	#include "SDL_syswm.h"
	#ifdef BBGE_BUILD_SDL2
	static SDL_Window *gScreen=0;
	static SDL_GLContext gGLctx=0;
	#else
	static SDL_Surface *gScreen=0;
	#endif

	bool ignoreNextMouse=false;
	Vector unchange;

#ifdef BBGE_BUILD_VFS
#include "ttvfs.h"
#endif

Core *core = 0;

#ifdef BBGE_BUILD_WINDOWS
	HICON icon_windows = 0;
#endif

#ifndef KMOD_GUI
	#define KMOD_GUI KMOD_META
#endif

void Core::initIcon()
{
#ifdef BBGE_BUILD_WINDOWS
	HINSTANCE handle = ::GetModuleHandle(NULL);



	icon_windows = ::LoadIcon(handle, "icon");

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version)
	if (SDL_GetWindowWMInfo(gScreen, &wminfo) != 1)
	{


	}

	HWND hwnd = wminfo.info.win.window;

	::SetClassLong(hwnd, GCL_HICON, (LONG) icon_windows);
#endif
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
	for (int i = 0; i < renderObjectLayers.size(); i++)
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
	for (int i = 0; i < renderObjectLayers.size(); i++)
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

void Core::resetGraphics(int w, int h, int fullscreen, int vsync, int bpp)
{
	if (fullscreen == -1)
		fullscreen = _fullscreen;

	if (vsync == -1)
		vsync = _vsync;

	if (w == -1)
		w = width;

	if (h == -1)
		h = height;

	if (bpp == -1)
		bpp = _bpp;

	unloadDevice();
	unloadResources();

	shutdownGraphicsLibrary();

	initGraphicsLibrary(w, h, fullscreen, vsync, bpp);

	enable2DWide(w, h);

	reloadResources();
	reloadDevice();


	resetTimer();
}

void Core::toggleScreenMode(int t)
{
	sound->pause();
	resetGraphics(-1, -1, t);
	cacheRender();
	resetTimer();
	sound->resume();
}

void Core::updateCursorFromJoystick(float dt, int spd)
{


	core->mouse.position += joystick.position*dt*spd;



	doMouseConstraint();
}

void Core::setWindowCaption(const std::string &caption, const std::string &icon)
{
#ifndef BBGE_BUILD_SDL2
	SDL_WM_SetCaption(caption.c_str(), icon.c_str());
#endif
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

bool Core::getMetaState()
{
	return getKeyState(KEY_LMETA) || getKeyState(KEY_RMETA);
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

#ifdef BBGE_BUILD_WINDOWS
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
			MessageBoxA(NULL, os.str().c_str(), "Need to write but can't!", MB_OK | MB_ICONERROR);
		}
		if(critical)
			exit(1);
	}
	return writeable;
}
#endif


Core::Core(const std::string &filesystem, const std::string& extraDataDir, int numRenderLayers, const std::string &appName, int particleSize, std::string userDataSubFolder)
: ActionMapper(), StateManager(), appName(appName)
{
	sound = NULL;
	screenCapScale = Vector(1,1,1);
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

	grabInputOnReentry = -1;

	srand(time(NULL));
	old_dt = 0;
	current_dt = 0;

	aspectX = 4;
	aspectY = 3;

	virtualOffX = virtualOffY = 0;

	viewOffX = viewOffY = 0;



	particleManager = new ParticleManager(particleSize);
	nowTicks = thenTicks = 0;
	_hasFocus = false;
	lib_graphics = lib_sound = lib_input = false;
	clearColor = Vector(0,0,0);
	updateCursorFromMouse = true;
	mouseConstraint = false;
	mouseCircle = 0;
	overrideStartLayer = 0;
	overrideEndLayer = 0;
	frameOutputMode = false;
	updateMouse = true;
	particlesPaused = false;
	joystickAsMouse = false;
	currentLayerPass = 0;
	flipMouseButtons = 0;
	joystickEnabled = false;
	doScreenshot = false;
	baseCullRadius = 1;
	width = height = 0;
	afterEffectManagerLayer = 0;
	renderObjectLayers.resize(1);
	invGlobalScale = 1.0;
	invGlobalScaleSqr = 1.0;
	renderObjectCount = 0;
	avgFPS.resize(1);
	minimized = false;
	numSavedScreenshots = 0;
	shuttingDown = false;
	nestedMains = 0;
	afterEffectManager = 0;
	loopDone = false;
	core = this;

	for (int i = 0; i < KEY_MAXARRAY; i++)
	{
		keys[i] = 0;
	}

	aspect = (aspectX/aspectY);


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
	if ( (rc == -1) || (rc >= sizeof (path)) )
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

#if BBGE_BUILD_UNIX
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

// based on code I wrote for PhysicsFS: http://icculus.org/physfs/
//  the zlib license on physfs allows this cut-and-pasting.
static int locateOneElement(char *buf)
{
	char *ptr;
	DIR *dirp;

	if (access(buf, F_OK) == 0)
		return(1);  // quick rejection: exists in current case.

	ptr = strrchr(buf, '/');  // find entry at end of path.
	if (ptr == NULL)
	{
		dirp = opendir(".");
		ptr = buf;
	}
	else
	{
		*ptr = '\0';
		dirp = opendir(buf);
		*ptr = '/';
		ptr++;  // point past dirsep to entry itself.
	}

	struct dirent *dent;
	while ((dent = readdir(dirp)) != NULL)
	{
		if (strcasecmp(dent->d_name, ptr) == 0)
		{
			strcpy(ptr, dent->d_name); // found a match. Overwrite with this case.
			closedir(dirp);
			return(1);
		}
	}

	// no match at all...
	closedir(dirp);
	return(0);
}
#endif


std::string Core::adjustFilenameCase(const char *_buf)
{
#ifdef BBGE_BUILD_UNIX  // any case is fine if not Linux.
	int rc = 1;
	char *buf = (char *) alloca(strlen(_buf) + 1);
	strcpy(buf, _buf);

	char *ptr = buf;
	while ((ptr = strchr(ptr + 1, '/')) != 0)
	{
		*ptr = '\0';  // block this path section off
		rc = locateOneElement(buf);
		*ptr = '/'; // restore path separator
		if (!rc)
			break;  // missing element in path.
	}

	// check final element...
	if (rc)
		rc = locateOneElement(buf);

	#if 0
	if (strcmp(_buf, buf) != 0)
	{
		fprintf(stderr, "Corrected filename case: '%s' => '%s (%s)'\n",
		        _buf, buf, rc ? "found" : "not found");
	}
	#endif

	return std::string(buf);
#else
	return std::string(_buf);
#endif
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

bool Core::hasFocus()
{
	return _hasFocus;
}

void Core::setInputGrab(bool on)
{
	if (isWindowFocus())
	{
		#ifdef BBGE_BUILD_SDL2
		SDL_SetWindowGrab(gScreen, on ? SDL_TRUE : SDL_FALSE);
		#else
		SDL_WM_GrabInput(on?SDL_GRAB_ON:SDL_GRAB_OFF);
		#endif
	}
}

void Core::setReentryInputGrab(int on)
{
	if (grabInputOnReentry == -1)
	{
		setInputGrab(on);
	}
	else
	{
		setInputGrab(grabInputOnReentry);
	}
}

bool Core::isFullscreen()
{
	return _fullscreen;
}

bool Core::isShuttingDown()
{
	return shuttingDown;
}

void Core::init()
{
	setupFileAccess();

	quitNestedMainFlag = false;
#ifndef BBGE_BUILD_SDL2
	// Disable relative mouse motion at the edges of the screen, which breaks
	// mouse control for absolute input devices like Wacom tablets and touchscreens.
	SDL_putenv((char *) "SDL_MOUSE_RELATIVE=0");
#endif

	if((SDL_Init(0))==-1)
	{
		exit_error("Failed to init SDL");
	}


	loopDone = false;

	initInputCodeMap();

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

bool Core::getMouseButtonState(int m)
{
	int mcode=m;

	switch(m)
	{
	case 0: mcode=1; break;
	case 1: mcode=3; break;
	case 2: mcode=2; break;
	}

	Uint8 mousestate = SDL_GetMouseState(0,0);

	return mousestate & SDL_BUTTON(mcode);
	return false;
}

bool Core::getKeyState(int k)
{

	if (k >= KEY_MAXARRAY || k < 0)
	{
		return 0;
	}
	return keys[k];

#ifdef BBGE_BUILD_WINDOWS
	if (k >= KEY_MAXARRAY || k < 0)
	{
		return 0;
	}
	return keys[k];
#endif

	return 0;
}



Vector joychange;
Vector lastjoy;
void readJoystickData()
{



}

void readMouseData()
{

}

void readKeyData()
{

}



bool Core::initJoystickLibrary()
{

#ifdef BBGE_BUILD_SDL2
	SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER);
#else
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
#endif

	joystick.init(0);

	joystickEnabled = true;

	return true;
}

bool Core::initInputLibrary()
{
	core->mouse.position = Vector(getWindowWidth()/2, getWindowHeight()/2);

	for (int i = 0; i < KEY_MAXARRAY; i++)
	{
		keys[i] = 0;
	}



	return true;
}

void Core::onUpdate(float dt)
{
	if (minimized) return;
	ActionMapper::onUpdate(dt);
	StateManager::onUpdate(dt);


	core->mouse.lastPosition = core->mouse.position;
	core->mouse.lastScrollWheel = core->mouse.scrollWheel;

	readKeyData();
	readMouseData();
	readJoystickData();
	pollEvents();
	joystick.update(dt);



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

Vector Core::getClearColor()
{
	return clearColor;
}

void Core::setClearColor(const Vector &c)
{
	clearColor = c;

	glClearColor(c.x, c.y, c.z, 0.0);

}

void Core::setSDLGLAttributes()
{
	std::ostringstream os;
	os << "setting vsync: " << _vsync;
	debugLog(os.str());

#ifndef BBGE_BUILD_SDL2
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, _vsync);
#endif

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}


#ifdef GLAPIENTRY
#undef GLAPIENTRY
#endif

#ifdef BBGE_BUILD_WINDOWS
#define GLAPIENTRY APIENTRY
#else
#define GLAPIENTRY
#endif

unsigned int Core::dbg_numRenderCalls = 0;

#ifdef BBGE_BUILD_OPENGL_DYNAMIC
#define GL_FUNC(ret,fn,params,call,rt) \
    extern "C" { \
        static ret (GLAPIENTRY *p##fn) params = NULL; \
        ret GLAPIENTRY fn params { ++Core::dbg_numRenderCalls; rt p##fn call; } \
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

static bool lookup_all_glsyms(void)
{
	bool retval = true;
	#define GL_FUNC(ret,fn,params,call,rt) \
		if (!lookup_glsym(#fn, (void **) &p##fn)) retval = false;
	#include "OpenGLStubs.h"
	#undef GL_FUNC
	return retval;
}
#endif


bool Core::initGraphicsLibrary(int width, int height, bool fullscreen, int vsync, int bpp, bool recreate)
{
	static bool didOnce = false;

	aspectX = width;
	aspectY = height;

	aspect = (aspectX/aspectY);



	this->width = width;
	this->height = height;
	_vsync = vsync;
	_fullscreen = fullscreen;
	_bpp = bpp;

	_hasFocus = false;



#ifndef BBGE_BUILD_SDL2
#if !defined(BBGE_BUILD_MACOSX)
	// have to cast away constness, since SDL_putenv() might be #defined to
	//  putenv(), which takes a (char *), and freaks out newer GCC releases
	//  when you try to pass a (const!) string literal here...  --ryan.
	SDL_putenv((char *) "SDL_VIDEO_CENTERED=1");
#endif
#endif


	if (recreate)
	{
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
		{
			exit_error(std::string("SDL Error: ") + std::string(SDL_GetError()));
		}

#if BBGE_BUILD_OPENGL_DYNAMIC
		if (SDL_GL_LoadLibrary(NULL) == -1)
		{
			std::string err = std::string("SDL_GL_LoadLibrary Error: ") + std::string(SDL_GetError());
			SDL_Quit();
			exit_error(err);
		}
#endif
	}

	setWindowCaption(appName, appName);

	initIcon();
    // Create window

	setSDLGLAttributes();


	{
#ifdef BBGE_BUILD_SDL2
		Uint32 flags = 0;
		flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
		if (fullscreen)
			flags |= SDL_WINDOW_FULLSCREEN;
		gScreen = SDL_CreateWindow(appName.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
		if (gScreen == NULL)
		{
			std::ostringstream os;
			os << "Couldn't set resolution [" << width << "x" << height << "]\n" << SDL_GetError();
			errorLog(os.str());
			SDL_Quit();
			exit(0);
		}
		gGLctx = SDL_GL_CreateContext(gScreen);
		if (gGLctx == NULL)
		{
			std::ostringstream os;
			os << "Couldn't create OpenGL context!\n" << SDL_GetError();
			errorLog(os.str());
			SDL_Quit();
			exit(0);
		}
#else
		Uint32 flags = 0;
		flags = SDL_OPENGL;
		if (fullscreen)
			flags |= SDL_FULLSCREEN;

		gScreen = SDL_SetVideoMode(width, height, bpp, flags);
		if (gScreen == NULL)
		{
			std::ostringstream os;
			os << "Couldn't set resolution [" << width << "x" << height << "]\n" << SDL_GetError();
			SDL_Quit();
			exit_error(os.str());
		}
#endif

#if BBGE_BUILD_OPENGL_DYNAMIC
		if (!lookup_all_glsyms())
		{
			std::ostringstream os;
			os << "Couldn't load OpenGL symbols we need\n";
			SDL_Quit();
			exit_error(os.str());
		}
#endif
	}

	setWindowCaption(appName, appName);

#ifdef BBGE_BUILD_SDL2
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(gScreen);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(gScreen);
	if ((_vsync != 1) || (SDL_GL_SetSwapInterval(-1) == -1))
		SDL_GL_SetSwapInterval(_vsync);
	const char *name = SDL_GetCurrentVideoDriver();
	SDL_SetWindowGrab(gScreen, SDL_TRUE);
#else
	SDL_WM_GrabInput(grabInputOnReentry==0 ? SDL_GRAB_OFF : SDL_GRAB_ON);
	char name[256];
	SDL_VideoDriverName((char*)name, 256);
#endif

	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);

	std::ostringstream os2;
	os2 << "Video Driver Name [" << name << "]";
	debugLog(os2.str());

	SDL_ShowCursor(SDL_DISABLE);
	SDL_PumpEvents();

	for (int i = 0; i < KEY_MAXARRAY; i++)
	{
		keys[i] = 0;
	}



	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0);								// Depth Buffer Setup
	glDisable(GL_CULL_FACE);



	glLoadIdentity();

	glFinish();



	setClearColor(clearColor);

	clearBuffers();
	showBuffer();

	lib_graphics = true;

	_hasFocus = true;

	enumerateScreenModes();

	if (!didOnce)
		didOnce = true;

	// init success
	return true;
}

void Core::enumerateScreenModes()
{
	screenModes.clear();

#ifdef BBGE_BUILD_SDL2
	SDL_DisplayMode mode;
	const int modecount = SDL_GetNumDisplayModes(0);
	if(modecount == 0){
		debugLog("No modes available!");
		return;
	}

	for (int i = 0; i < modecount; i++) {
		SDL_GetDisplayMode(0, i, &mode);
		if (mode.w && mode.h && (mode.w > mode.h))
		{
			screenModes.push_back(ScreenMode(i, mode.w, mode.h, mode.refresh_rate));
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
				screenModes.push_back(ScreenMode(i, modes[i]->w, modes[i]->h, 0));
			}
		}
	}
#endif
}

void Core::shutdownSoundLibrary()
{
}

void Core::shutdownGraphicsLibrary(bool killVideo)
{
	glFinish();
	if (killVideo) {
		#ifdef BBGE_BUILD_SDL2
		SDL_SetWindowGrab(gScreen, SDL_FALSE);
		SDL_GL_MakeCurrent(gScreen, NULL);
		SDL_GL_DeleteContext(gGLctx);
		SDL_DestroyWindow(gScreen);
		gGLctx = 0;
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		#else
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_WM_GrabInput(SDL_GRAB_OFF);
		#endif

		FrameBuffer::resetOpenGL();

		gScreen = 0;

#if BBGE_BUILD_OPENGL_DYNAMIC
		// reset all the entry points to NULL, so we know exactly what happened
		//  if we call a GL function after shutdown.
		#define GL_FUNC(ret,fn,params,call,rt) \
			p##fn = NULL;
		#include "OpenGLStubs.h"
		#undef GL_FUNC
#endif
	}

	_hasFocus = false;

	lib_graphics = false;

#ifdef BBGE_BUILD_WINDOWS
	if (icon_windows)
	{
		::DestroyIcon(icon_windows);
		icon_windows = 0;
	}
#endif

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



#ifdef BBGE_BUILD_WINDOWS
void centerWindow(HWND hwnd)
{
    int x, y;
    HWND hwndDeskTop;
    RECT rcWnd, rcDeskTop;
    // Get a handle to the desktop window
    hwndDeskTop = ::GetDesktopWindow();
    // Get dimension of desktop in a rect
    ::GetWindowRect(hwndDeskTop, &rcDeskTop);
    // Get dimension of main window in a rect
    ::GetWindowRect(hwnd, &rcWnd);
    // Find center of desktop
	x = (rcDeskTop.right - rcDeskTop.left)/2;
	y = (rcDeskTop.bottom - rcDeskTop.top)/2;
    x -= (rcWnd.right - rcWnd.left)/2;
	y -= (rcWnd.bottom - rcWnd.top)/2;
    // Set top and left to center main window on desktop
    ::SetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);

}
#endif



bool Core::createWindow(int width, int height, int bits, bool fullscreen, std::string windowTitle)
{
	this->width = width;
	this->height = height;
	return true;


}

// No longer part of C/C++ standard
#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

static void
bbgePerspective(float fovy, float aspect, float zNear, float zFar)
{
    float sine, cotangent, deltaZ;
    float radians = fovy / 2.0f * M_PI / 180.0f;

    deltaZ = zFar - zNear;
    sine = sinf(radians);
    if ((deltaZ == 0.0f) || (sine == 0.0f) || (aspect == 0.0f)) {
        return;
    }
    cotangent = cosf(radians) / sine;

    GLfloat m[4][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    };
    m[0][0] = (GLfloat) (cotangent / aspect);
    m[1][1] = (GLfloat) cotangent;
    m[2][2] = (GLfloat) (-(zFar + zNear) / deltaZ);
    m[2][3] = -1.0f;
    m[3][2] = (GLfloat) (-2.0f * zNear * zFar / deltaZ);
    m[3][3] = 0.0f;

    glMultMatrixf(&m[0][0]);
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

	virtualOffX = 0;
	virtualOffY = 0;

	int diff = 0;

	diff = virtualWidth-baseVirtualWidth;
	if (diff > 0)
		virtualOffX = ((virtualWidth-baseVirtualWidth)/2);
	else
		virtualOffX = 0;


	diff = virtualHeight-baseVirtualHeight;
	if (diff > 0)
		virtualOffY = ((virtualHeight-baseVirtualHeight)/2);
	else
		virtualOffY = 0;
}

// forcePixelScale used by Celu

void Core::enable2DWide(int rx, int ry)
{
	float aspect = float(rx) / float(ry);
	if (aspect >= 1.3f)
	{
		int vw = int(float(baseVirtualHeight) * (float(rx)/float(ry)));

		core->enable2D(vw, baseVirtualHeight, 1);
	}
	else
	{
		int vh = int(float(baseVirtualWidth) * (float(ry)/float(rx)));

		core->enable2D(baseVirtualWidth, vh, 1);
	}
}

static void bbgeOrtho2D(float left, float right, float bottom, float top)
{
    glOrtho(left, right, bottom, top, -1.0, 1.0);
}

void Core::enable2D(int pixelScaleX, int pixelScaleY, bool forcePixelScale)
{



    GLint viewPort[4];
    glGetIntegerv(GL_VIEWPORT, viewPort);

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

	float vw=0,vh=0;

	viewOffX = viewOffY = 0;

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


	if (forcePixelScale || (pixelScaleX!=0 && core->width!=pixelScaleX) || (pixelScaleY!=0 && core->height!=pixelScaleY))
	{


		float widthFactor = core->width/float(pixelScaleX);
		float heightFactor = core->height/float(pixelScaleY);

		core->globalResolutionScale = Vector(widthFactor,heightFactor,1.0f);
		setPixelScale(pixelScaleX, pixelScaleY);



	}
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

	for (int i = 0; i < avgFPS.size(); i++)
	{
		avgFPS[i] = 0;
	}
}

void Core::setDockIcon(const std::string &ident)
{
}

void Core::setMousePosition(const Vector &p)
{
	Vector lp = core->mouse.position;

	core->mouse.position = p;
	float px = p.x + virtualOffX;
	float py = p.y;

	#ifdef BBGE_BUILD_SDL2
	SDL_WarpMouseInWindow(gScreen, px * (float(width)/float(virtualWidth)), py * (float(height)/float(virtualHeight)));
	#else
	SDL_WarpMouse( px * (float(width)/float(virtualWidth)), py * (float(height)/float(virtualHeight)));
	#endif



}

// used to update all render objects either uniformly or as part of a time sliced update process
void Core::updateRenderObjects(float dt)
{
	for (int c = 0; c < renderObjectLayers.size(); c++)
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
std::string getScreenshotFilename()
{
	while (true)
	{
		std::ostringstream os;
		os << core->getUserDataFolder() << "/screenshots/screen" << screenshotNum << ".tga";
		screenshotNum ++;
        std::string str(os.str());
		if (!core->exists(str))  // keep going until we hit an unused filename.
			return str;
	}
}

unsigned Core::getTicks()
{
	return SDL_GetTicks();
	return 0;
}

bool Core::isWindowFocus()
{
	#ifdef BBGE_BUILD_SDL2
	return ((SDL_GetWindowFlags(gScreen) & SDL_WINDOW_INPUT_FOCUS) != 0);
	#else
	return ((SDL_GetAppState() & SDL_APPINPUTFOCUS) != 0);
	#endif
	return true;
}

void Core::onBackgroundUpdate()
{
	SDL_Delay(200);
}

void Core::main(float runTime)
{
	// cannot nest loops when the game is over
	if (loopDone) return;



	float dt;
	float counter = 0;
	int frames = 0;
	float real_dt = 0;


#if (!defined(_DEBUG) || defined(BBGE_BUILD_UNIX)) && defined(BBGE_BUILD_SDL)
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

			int i = 0;
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

#if !defined(_DEBUG) && defined(BBGE_BUILD_SDL)

		if (lib_graphics && (wasInactive || !settings.runInBackground))
		{
			if (isWindowFocus())
			{
				_hasFocus = true;
				if (wasInactive)
				{
					debugLog("WINDOW ACTIVE");

					setReentryInputGrab(1);

					wasInactive = false;
				}
			}
			else
			{
				if (_hasFocus)
				{
					if (!wasInactive)
						debugLog("WINDOW INACTIVE");

					wasInactive = true;
					_hasFocus = false;

					setReentryInputGrab(0);

					sound->pause();

					core->joystick.rumble(0,0,0);

					while (!isWindowFocus())
					{
						pollEvents();

						onBackgroundUpdate();

						resetTimer();
					}

					debugLog("app back in focus, reset");

					// Don't do this on Linux, it's not necessary and causes big stalls.
					//  We don't actually _lose_ the device like Direct3D anyhow.
					#ifndef BBGE_BUILD_UNIX
					if (_fullscreen)
					{
						// calls reload device - reloadDevice()
						resetGraphics(width, height);
					}
					#endif

					resetTimer();

					sound->resume();

					resetTimer();

					SDL_ShowCursor(SDL_DISABLE);

					continue;
				}
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

		dbg_numRenderCalls = 0;

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

			saveScreenshotTGA(getScreenshotFilename());
			prepScreen(0);
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
	glScalef(globalScale.x*globalResolutionScale.x*screenCapScale.x, globalScale.y*globalResolutionScale.y*screenCapScale.y, globalScale.z*globalResolutionScale.z);
	glTranslatef(-(cameraPos.x+cameraOffset.x), -(cameraPos.y+cameraOffset.y), -(cameraPos.z+cameraOffset.z));
}

void Core::setupGlobalResolutionScale()
{
	glScalef(globalResolutionScale.x, globalResolutionScale.y, globalResolutionScale.z);
}

void Core::initFrameBuffer()
{
	frameBuffer.init(-1, -1, true);
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


#if defined(BBGE_BUILD_SDL2)
typedef std::map<SDL_Keycode,int> sdlKeyMap;
#else
typedef std::map<SDLKey,int> sdlKeyMap;
#endif

static sdlKeyMap *initSDLKeymap(void)
{
	sdlKeyMap *_retval = new sdlKeyMap;
	sdlKeyMap &retval = *_retval;

	#define SETKEYMAP(gamekey,sdlkey) retval[sdlkey] = gamekey

#ifdef BBGE_BUILD_SDL2
	SETKEYMAP(KEY_LSUPER, SDLK_LGUI);
	SETKEYMAP(KEY_RSUPER, SDLK_RGUI);
	SETKEYMAP(KEY_LMETA, SDLK_LGUI);
	SETKEYMAP(KEY_RMETA, SDLK_RGUI);
	SETKEYMAP(KEY_PRINTSCREEN, SDLK_PRINTSCREEN);
	SETKEYMAP(KEY_NUMPAD1, SDLK_KP_1);
	SETKEYMAP(KEY_NUMPAD2, SDLK_KP_2);
	SETKEYMAP(KEY_NUMPAD3, SDLK_KP_3);
	SETKEYMAP(KEY_NUMPAD4, SDLK_KP_4);
	SETKEYMAP(KEY_NUMPAD5, SDLK_KP_5);
	SETKEYMAP(KEY_NUMPAD6, SDLK_KP_6);
	SETKEYMAP(KEY_NUMPAD7, SDLK_KP_7);
	SETKEYMAP(KEY_NUMPAD8, SDLK_KP_8);
	SETKEYMAP(KEY_NUMPAD9, SDLK_KP_9);
	SETKEYMAP(KEY_NUMPAD0, SDLK_KP_0);
#else
	SETKEYMAP(KEY_LSUPER, SDLK_LSUPER);
	SETKEYMAP(KEY_RSUPER, SDLK_RSUPER);
	SETKEYMAP(KEY_LMETA, SDLK_LMETA);
	SETKEYMAP(KEY_RMETA, SDLK_RMETA);
	SETKEYMAP(KEY_PRINTSCREEN, SDLK_PRINT);
	SETKEYMAP(KEY_NUMPAD1, SDLK_KP1);
	SETKEYMAP(KEY_NUMPAD2, SDLK_KP2);
	SETKEYMAP(KEY_NUMPAD3, SDLK_KP3);
	SETKEYMAP(KEY_NUMPAD4, SDLK_KP4);
	SETKEYMAP(KEY_NUMPAD5, SDLK_KP5);
	SETKEYMAP(KEY_NUMPAD6, SDLK_KP6);
	SETKEYMAP(KEY_NUMPAD7, SDLK_KP7);
	SETKEYMAP(KEY_NUMPAD8, SDLK_KP8);
	SETKEYMAP(KEY_NUMPAD9, SDLK_KP9);
	SETKEYMAP(KEY_NUMPAD0, SDLK_KP0);
#endif

	SETKEYMAP(KEY_BACKSPACE, SDLK_BACKSPACE);



	SETKEYMAP(KEY_LALT, SDLK_LALT);
	SETKEYMAP(KEY_RALT, SDLK_RALT);
	SETKEYMAP(KEY_LSHIFT, SDLK_LSHIFT);
	SETKEYMAP(KEY_RSHIFT, SDLK_RSHIFT);
	SETKEYMAP(KEY_LCONTROL, SDLK_LCTRL);
	SETKEYMAP(KEY_RCONTROL, SDLK_RCTRL);
	SETKEYMAP(KEY_NUMPADMINUS, SDLK_KP_MINUS);
	SETKEYMAP(KEY_NUMPADPERIOD, SDLK_KP_PERIOD);
	SETKEYMAP(KEY_NUMPADPLUS, SDLK_KP_PLUS);
	SETKEYMAP(KEY_NUMPADSLASH, SDLK_KP_DIVIDE);
	SETKEYMAP(KEY_NUMPADSTAR, SDLK_KP_MULTIPLY);
	SETKEYMAP(KEY_PGDN, SDLK_PAGEDOWN);
	SETKEYMAP(KEY_PGUP, SDLK_PAGEUP);
	SETKEYMAP(KEY_APOSTROPHE, SDLK_QUOTE);
	SETKEYMAP(KEY_EQUALS, SDLK_EQUALS);
	SETKEYMAP(KEY_SEMICOLON, SDLK_SEMICOLON);
	SETKEYMAP(KEY_LBRACKET, SDLK_LEFTBRACKET);
	SETKEYMAP(KEY_RBRACKET, SDLK_RIGHTBRACKET);

	SETKEYMAP(KEY_TILDE, SDLK_BACKQUOTE);
	SETKEYMAP(KEY_0, SDLK_0);
	SETKEYMAP(KEY_1, SDLK_1);
	SETKEYMAP(KEY_2, SDLK_2);
	SETKEYMAP(KEY_3, SDLK_3);
	SETKEYMAP(KEY_4, SDLK_4);
	SETKEYMAP(KEY_5, SDLK_5);
	SETKEYMAP(KEY_6, SDLK_6);
	SETKEYMAP(KEY_7, SDLK_7);
	SETKEYMAP(KEY_8, SDLK_8);
	SETKEYMAP(KEY_9, SDLK_9);
	SETKEYMAP(KEY_A, SDLK_a);
	SETKEYMAP(KEY_B, SDLK_b);
	SETKEYMAP(KEY_C, SDLK_c);
	SETKEYMAP(KEY_D, SDLK_d);
	SETKEYMAP(KEY_E, SDLK_e);
	SETKEYMAP(KEY_F, SDLK_f);
	SETKEYMAP(KEY_G, SDLK_g);
	SETKEYMAP(KEY_H, SDLK_h);
	SETKEYMAP(KEY_I, SDLK_i);
	SETKEYMAP(KEY_J, SDLK_j);
	SETKEYMAP(KEY_K, SDLK_k);
	SETKEYMAP(KEY_L, SDLK_l);
	SETKEYMAP(KEY_M, SDLK_m);
	SETKEYMAP(KEY_N, SDLK_n);
	SETKEYMAP(KEY_O, SDLK_o);
	SETKEYMAP(KEY_P, SDLK_p);
	SETKEYMAP(KEY_Q, SDLK_q);
	SETKEYMAP(KEY_R, SDLK_r);
	SETKEYMAP(KEY_S, SDLK_s);
	SETKEYMAP(KEY_T, SDLK_t);
	SETKEYMAP(KEY_U, SDLK_u);
	SETKEYMAP(KEY_V, SDLK_v);
	SETKEYMAP(KEY_W, SDLK_w);
	SETKEYMAP(KEY_X, SDLK_x);
	SETKEYMAP(KEY_Y, SDLK_y);
	SETKEYMAP(KEY_Z, SDLK_z);

	SETKEYMAP(KEY_LEFT, SDLK_LEFT);
	SETKEYMAP(KEY_RIGHT, SDLK_RIGHT);
	SETKEYMAP(KEY_UP, SDLK_UP);
	SETKEYMAP(KEY_DOWN, SDLK_DOWN);

	SETKEYMAP(KEY_DELETE, SDLK_DELETE);
	SETKEYMAP(KEY_SPACE, SDLK_SPACE);
	SETKEYMAP(KEY_RETURN, SDLK_RETURN);
	SETKEYMAP(KEY_PERIOD, SDLK_PERIOD);
	SETKEYMAP(KEY_MINUS, SDLK_MINUS);
	SETKEYMAP(KEY_CAPSLOCK, SDLK_CAPSLOCK);
	SETKEYMAP(KEY_SYSRQ, SDLK_SYSREQ);
	SETKEYMAP(KEY_TAB, SDLK_TAB);
	SETKEYMAP(KEY_HOME, SDLK_HOME);
	SETKEYMAP(KEY_END, SDLK_END);
	SETKEYMAP(KEY_COMMA, SDLK_COMMA);
	SETKEYMAP(KEY_SLASH, SDLK_SLASH);

	SETKEYMAP(KEY_F1, SDLK_F1);
	SETKEYMAP(KEY_F2, SDLK_F2);
	SETKEYMAP(KEY_F3, SDLK_F3);
	SETKEYMAP(KEY_F4, SDLK_F4);
	SETKEYMAP(KEY_F5, SDLK_F5);
	SETKEYMAP(KEY_F6, SDLK_F6);
	SETKEYMAP(KEY_F7, SDLK_F7);
	SETKEYMAP(KEY_F8, SDLK_F8);
	SETKEYMAP(KEY_F9, SDLK_F9);
	SETKEYMAP(KEY_F10, SDLK_F10);
	SETKEYMAP(KEY_F11, SDLK_F11);
	SETKEYMAP(KEY_F12, SDLK_F12);
	SETKEYMAP(KEY_F13, SDLK_F13);
	SETKEYMAP(KEY_F14, SDLK_F14);
	SETKEYMAP(KEY_F15, SDLK_F15);

	SETKEYMAP(KEY_ESCAPE, SDLK_ESCAPE);



	#undef SETKEYMAP

	return _retval;
}

#if defined(BBGE_BUILD_SDL2)
static int mapSDLKeyToGameKey(const SDL_Keycode val)
#else
static int mapSDLKeyToGameKey(const SDLKey val)
#endif
{
	static sdlKeyMap *keymap = NULL;
	if (keymap == NULL)
		keymap = initSDLKeymap();

	return (*keymap)[val];
}


void Core::pollEvents()
{
	bool warpMouse=false;



	if (updateMouse)
	{
		int x, y;
		Uint8 mousestate = SDL_GetMouseState(&x,&y);

		if (mouse.buttonsEnabled)
		{
			mouse.buttons.left		= mousestate & SDL_BUTTON(1)?DOWN:UP;
			mouse.buttons.right		= mousestate & SDL_BUTTON(3)?DOWN:UP;
			mouse.buttons.middle	= mousestate & SDL_BUTTON(2)?DOWN:UP;

			mouse.pure_buttons = mouse.buttons;

			if (flipMouseButtons)
			{
				std::swap(mouse.buttons.left, mouse.buttons.right);
			}
		}
		else
		{
			mouse.buttons.left = mouse.buttons.right = mouse.buttons.middle = UP;
		}

		mouse.scrollWheelChange = 0;
		mouse.change = Vector(0,0);
	}



	SDL_Event event;



	while ( SDL_PollEvent (&event) ) {
		switch (event.type) {
			case SDL_KEYDOWN:
			{
				#if __APPLE__
					#if SDL_VERSION_ATLEAST(2, 0, 0)
						if ((event.key.keysym.sym == SDLK_q) && (event.key.keysym.mod & KMOD_GUI))
					#else
						if ((event.key.keysym.sym == SDLK_q) && (event.key.keysym.mod & KMOD_META))
					#endif
				#else
				if ((event.key.keysym.sym == SDLK_F4) && (event.key.keysym.mod & KMOD_ALT))
				#endif
				{
					quitNestedMain();
					quit();
				}

				if ((event.key.keysym.sym == SDLK_g) && (event.key.keysym.mod & KMOD_CTRL))
				{

					grabInputOnReentry = (grabInputOnReentry)?0:-1;
					setReentryInputGrab(1);
				}
				else if (_hasFocus)
				{
					keys[mapSDLKeyToGameKey(event.key.keysym.sym)] = 1;
				}
			}
			break;

			case SDL_KEYUP:
			{
				if (_hasFocus)
				{
					keys[mapSDLKeyToGameKey(event.key.keysym.sym)] = 0;
				}
			}
			break;

			case SDL_MOUSEMOTION:
			{
				if (_hasFocus && updateMouse)
				{
					mouse.lastPosition = mouse.position;

					mouse.position.x = ((event.motion.x) * (float(virtualWidth)/float(getWindowWidth()))) - getVirtualOffX();
					mouse.position.y = event.motion.y * (float(virtualHeight)/float(getWindowHeight()));

					mouse.change = mouse.position - mouse.lastPosition;

					if (doMouseConstraint()) warpMouse = true;
				}
			}
			break;

			#ifdef BBGE_BUILD_SDL2
			case SDL_WINDOWEVENT:
			{
				if (event.window.event == SDL_WINDOWEVENT_CLOSE)
				{
					SDL_Quit();
					_exit(0);
				}
			}
			break;

			case SDL_MOUSEWHEEL:
			{
				if (_hasFocus && updateMouse)
				{
					if (event.wheel.y > 0)
						mouse.scrollWheelChange = 1;
					else if (event.wheel.y < 0)
						mouse.scrollWheelChange = -1;
				}
			}
			break;

			case SDL_JOYDEVICEADDED:
				onJoystickAdded(event.jdevice.which);
				break;

			case SDL_JOYDEVICEREMOVED:
				onJoystickRemoved(event.jdevice.which);
				break;

			#else
			case SDL_MOUSEBUTTONDOWN:
			{
				if (_hasFocus && updateMouse)
				{
					switch(event.button.button)
					{
					case 4:
						mouse.scrollWheelChange = 1;
					break;
					case 5:
						mouse.scrollWheelChange = -1;
					break;
					}
				}
			}
			break;

			case SDL_MOUSEBUTTONUP:
			{
				if (_hasFocus && updateMouse)
				{
					switch(event.button.button)
					{
					case 4:
						mouse.scrollWheelChange = 1;
					break;
					case 5:
						mouse.scrollWheelChange = -1;
					break;
					}
				}
			}
			break;
			#endif

			case SDL_QUIT:
				SDL_Quit();
				_exit(0);


			break;

			case SDL_SYSWMEVENT:
			{

			}
			break;

			default:
			break;
		}
	}

	if (updateMouse)
	{
		mouse.scrollWheel += mouse.scrollWheelChange;

		if (warpMouse)
		{
			setMousePosition(mouse.position);
		}
	}

}

#define _VLN(x, y, x2, y2) glVertex2f(x, y); glVertex2f(x2, y2);

void Core::print(int x, int y, const char *str, float sz)
{



	glBindTexture(GL_TEXTURE_2D, 0);

	glPushMatrix();


	float xx = x;
	float yy = y;
	glTranslatef(x, y-0.5f*sz, 0);
	x = y = 0;
	xx = 0; yy = 0;
	bool isLower = false, wasLower = false;
	int c=0;


	glLineWidth(1);
	glScalef(sz*0.75f, sz, 1);

	glBegin(GL_LINES);

	while (str[c] != '\0')
	{
		if (str[c] <= 'z' && str[c] >= 'a')
			isLower = true;
		else
			isLower = false;



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
		if (isLower)
		{
			wasLower = true;

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

	for (int c = 0; c < renderObjectLayerOrder.size(); c++)

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
	BBGE_PROF(Core_showBuffer);
#ifdef BBGE_BUILD_SDL2
	SDL_GL_SwapWindow(gScreen);
#else
	SDL_GL_SwapBuffers();

#endif

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
		joystick.shutdown();
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		joystickEnabled = false;
	}
}

void Core::clearRenderObjects()
{
	for (int i = 0; i < renderObjectLayers.size(); i++)
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

void Core::addRenderObject(RenderObject *o, int layer)
{
	if (!o) return;
	o->layer = layer;
	if (layer < 0 || layer >= renderObjectLayers.size())
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
	for (int i = 0; i < resources.size(); i++)
	{
		resources[i]->unload();
	}
}

void Core::onReloadResources()
{
}

void Core::reloadResources()
{
	for (int i = 0; i < resources.size(); i++)
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

void Core::deleteRenderObjectMemory(RenderObject *r)
{

	delete r;
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

			deleteRenderObjectMemory(r);
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
		deleteRenderObjectMemory(*i);
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

	unsigned char *imageData;

	unsigned int size = sizeof(unsigned char) * w * h * 4;
	imageData = new unsigned char[size];

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

// takes a screen shot and saves it to a TGA image
int Core::saveScreenshotTGA(const std::string &filename)
{
	int w = getWindowWidth(), h = getWindowHeight();
	unsigned char *imageData = grabCenteredScreenshot(w, h);
	return tgaSave(filename.c_str(),w,h,32,imageData);
}

void Core::saveCenteredScreenshotTGA(const std::string &filename, int sz)
{
	int w=sz, h=sz;
	int hsm = (w * 3.0f) / 4.0f;
	unsigned char *imageData = grabCenteredScreenshot(w, hsm);

	int imageDataSize = sizeof(unsigned char) * w * hsm * 4;
	int tgaImageSize = sizeof(unsigned char) * w * h * 4;
	unsigned char *tgaImage = new unsigned char[tgaImageSize];
	memcpy(tgaImage, imageData, imageDataSize);
	memset(tgaImage + imageDataSize, 0, tgaImageSize - imageDataSize);
	delete[] imageData;

	int savebits = 32;
	tgaSave(filename.c_str(),w,h,savebits,tgaImage);
}

void Core::saveSizedScreenshotTGA(const std::string &filename, int sz, int crop34)
{
	debugLog("saveSizedScreenshot");

	int w, h;
	unsigned char *imageData;
	w = sz;
	h = sz;
	float fsz = (float)sz;

	unsigned int size = sizeof(unsigned char) * w * h * 3;
	imageData = (unsigned char *)malloc(size);

	float wbit = fsz;
	float hbit = ((fsz)*(3.0f/4.0f));

	int width = core->width-1;
	int height = core->height-1;
	int diff = 0;

	if (crop34)
	{
		width = int((core->height*4.0f)/3.0f);
		diff = (core->width - width)/2;
		width--;
	}

	float zx = wbit/(float)width;
	float zy = hbit/(float)height;

	float copyw = w*(1/zx);
	float copyh = h*(1/zy);



	std::ostringstream os;
	os << "wbit: " << wbit << " hbit: " << hbit << std::endl;
	os << "zx: " << zx << " zy: " << zy << std::endl;
	os << "w: " << w << " h: " << h << std::endl;
	os << "width: " << width << " height: " << height << std::endl;
	os << "copyw: " << copyw << " copyh: " << copyh << std::endl;
	debugLog(os.str());

	glRasterPos2i(0, 0);



	debugLog("pixel zoom");
	glPixelZoom(zx,zy);
	glFlush();

	glPixelZoom(1,1);
	debugLog("copy pixels");
	glCopyPixels(diff, 0, width, height, GL_COLOR);
	glFlush();

	debugLog("read pixels");
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)imageData);
	glFlush();

	int savebits = 24;
	debugLog("saving bpp");
	tgaSave(filename.c_str(),w,h,savebits,imageData);

	debugLog("pop");


	debugLog("done");
}

void Core::save64x64ScreenshotTGA(const std::string &filename)
{
	int w, h;
	unsigned char *imageData;

// compute width and heidth of the image
	//w = xmax - xmin;
	//h = ymax - ymin;
	w = 64;
	h = 64;

// allocate memory for the pixels
	imageData = (unsigned char *)malloc(sizeof(unsigned char) * w * h * 4);

// read the pixels from the frame buffer

	//glReadPixels(xmin,ymin,xmax,ymax,GL_RGBA,GL_UNSIGNED_BYTE, (GLvoid *)imageData);
	glPixelZoom(64.0f/(float)getVirtualWidth(), 48.0f/(float)getVirtualHeight());
	glCopyPixels(0, 0, getVirtualWidth(), getVirtualHeight(), GL_COLOR);

	glReadPixels(0,0,64,64,GL_RGBA,GL_UNSIGNED_BYTE, (GLvoid *)imageData);


	unsigned char *c = imageData;
	for (int x=0; x < w; x++)
	{
		for (int y=0; y< h; y++)
		{
			c += 3;
			(*c) = 255;
			c ++;
		}
	}


// save the image
	tgaSave(filename.c_str(),64,64,32,imageData);
	glPixelZoom(1,1);



}



// saves an array of pixels as a TGA image (frees the image data passed in)
int Core::tgaSave(	const char	*filename,
		short int	width,
		short int	height,
		unsigned char	pixelDepth,
		unsigned char	*imageData) {

	unsigned char cGarbage = 0, type,mode,aux;
	short int iGarbage = 0;
	int i;
	FILE *file;

// open file and check for errors
	file = fopen(adjustFilenameCase(filename).c_str(), "wb");
	if (file == NULL) {
		delete [] imageData;
		return (int)false;
	}

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
		delete [] imageData;
		return (int)false;
	}

// convert the image data from RGB(A) to BGR(A)
	if (mode >= 3)
	for (i=0; i < width * height * mode ; i+= mode) {
		aux = imageData[i];
		imageData[i] = imageData[i+2];
		imageData[i+2] = aux;
	}

// save the image data
	if (fwrite(imageData, sizeof(unsigned char),
			width * height * mode, file) != width * height * mode)
	{
		fclose(file);
		delete [] imageData;
		return (int)false;
	}

	fclose(file);
	delete [] imageData;

	return (int)true;
}

// saves a series of files with names "filenameX"
int Core::tgaSaveSeries(char		*filename,
			 short int		width,
			 short int		height,
			 unsigned char	pixelDepth,
			 unsigned char	*imageData) {

	char *newFilename;
	int status;

// compute the new filename by adding the
// series number and the extension
	newFilename = (char *)malloc(sizeof(char) * strlen(filename)+8);

	sprintf(newFilename,"%s%d",filename,numSavedScreenshots);

// save the image
	status = tgaSave(newFilename,width,height,pixelDepth,imageData);

//increase the counter
	if (status == (int)true)
		numSavedScreenshots++;
	free(newFilename);
	return(status);
}

 void Core::screenshot()
 {
	 doScreenshot = true;

 }


 #include "DeflateCompressor.h"

 // saves an array of pixels as a TGA image (frees the image data passed in)
int Core::zgaSave(	const char	*filename,
		short int	w,
		short int	h,
		unsigned char	depth,
		unsigned char	*imageData) {

	ByteBuffer::uint8 type,mode,aux, pixelDepth = depth;
	ByteBuffer::uint8 cGarbage = 0;
	ByteBuffer::uint16 iGarbage = 0;
	ByteBuffer::uint16 width = w, height = h;

// open file and check for errors
	FILE *file = fopen(adjustFilenameCase(filename).c_str(), "wb");
	if (file == NULL) {
		delete [] imageData;
		return (int)false;
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

// save the image data
	if (fwrite(z.contents(), 1, z.size(), file) != z.size())
	{
		fclose(file);
		delete [] imageData;
		return (int)false;
	}

	fclose(file);
	delete [] imageData;

	return (int)true;
}



#include "ttvfs_zip/VFSZipArchiveLoader.h"

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
	std::map<unsigned char, unsigned char> trans;
	while(in)
	{
		in >> low >> up;
		trans[low[0]] = up[0];
	}
	initCharTranslationTables(trans);
}

void Core::onJoystickAdded(int deviceID)
{

}

void Core::onJoystickRemoved(int instanceID)
{
}
