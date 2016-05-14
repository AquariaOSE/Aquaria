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
#ifndef __core__
#define __core__

/*
valid BUILD_ flags
WIN32/BUILD_WIN32
BUILD_MACOSX
BUILD_X360
BUILD_LINUX
*/

#include "Base.h"
#include "RenderObject.h"
#include "SoundManager.h"
#include "ActionMapper.h"
#include "Event.h"
#include "StateManager.h"
#include "Effects.h"
#include "Localization.h"

#include "DarkLayer.h"



#include "FrameBuffer.h"
#include "Shader.h"

class ParticleEffect;

class ParticleManager;

void initInputCodeMap();
void clearInputCodeMap();
std::string getInputCodeToString(int key);
std::string getInputCodeToUserString(int key);
int getStringToInputCode(const std::string &string);

enum TimeUpdateType
{
	TIMEUPDATE_DYNAMIC	= 0,
	TIMEUPDATE_FIXED	= 1
};

struct ScreenMode
{
	ScreenMode() { idx = x = y = hz = 0; }
	ScreenMode(int i, int x, int y, int hz) : idx(i), x(x), y(y), hz(hz) {}

	int idx, x, y, hz;
};

struct CoreSettings
{
	CoreSettings() { renderOn = true; updateOn = true; runInBackground = false; prebufferSounds = false; }
	bool renderOn;
	bool runInBackground;
	bool updateOn; // NOT IMPLEMENTED YET
	bool prebufferSounds;
};

enum CoreFlags
{
	CF_CLEARBUFFERS	= 0x00000001,
	CF_SORTENABLED	= 0x00000010
};

enum CoreLayers
{
	LR_NONE		= -1
};

const int NO_FOLLOW_CAMERA = -999;

class AfterEffectManager;

class Texture;

const int baseVirtualWidth		= 800;
const int baseVirtualHeight		= 600;

enum GameKeys
{



	KEY_LSUPER,
	KEY_RSUPER,
	KEY_LMETA,
	KEY_RMETA,
	KEY_BACKSPACE,
	KEY_PRINTSCREEN,



	KEY_LALT,
	KEY_RALT,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_NUMPADMINUS,
	KEY_NUMPADPERIOD,
	KEY_NUMPADPLUS,
	KEY_NUMPADSLASH,
	KEY_NUMPADSTAR,
	KEY_PGDN,
	KEY_PGUP,
	KEY_APOSTROPHE,
	KEY_EQUALS,
	KEY_SEMICOLON,
	KEY_LBRACKET,
	KEY_RBRACKET,

	KEY_TILDE,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_NUMPAD1,
	KEY_NUMPAD2,
	KEY_NUMPAD3,
	KEY_NUMPAD4,
	KEY_NUMPAD5,
	KEY_NUMPAD6,
	KEY_NUMPAD7,
	KEY_NUMPAD8,
	KEY_NUMPAD9,
	KEY_NUMPAD0,
	KEY_DELETE,
	KEY_SPACE,
	KEY_RETURN,
	KEY_PERIOD,
	KEY_MINUS,
	KEY_CAPSLOCK,
	KEY_SYSRQ,
	KEY_TAB,
	KEY_HOME,
	KEY_END,
	KEY_COMMA,
	KEY_SLASH,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
	KEY_ESCAPE,
	KEY_ANYKEY,
	KEY_MAXARRAY
};


enum ButtonState { UP = 0, DOWN };

struct MouseButtons
{
	MouseButtons ()
	{
		left = UP;
		right = UP;
		middle = UP;
	}

	ButtonState left, right, middle;
};

struct Mouse
{
	Mouse()
	{
		scrollWheel = scrollWheelChange = lastScrollWheel = 0;
		buttonsEnabled = true;
		movementEnabled = true;
	}
	Vector position, lastPosition;
	MouseButtons buttons;
	MouseButtons pure_buttons;
	Vector change;


	// movementEnabled is not implemented yet
	bool buttonsEnabled, movementEnabled;

	int scrollWheel, scrollWheelChange, lastScrollWheel;
};

const int maxJoyBtns = 64;

class Joystick
{
public:
	Joystick();
	void init(int stick=0);
	void shutdown();
	//Ranges from 0 to 65535 (full speed).
	void rumble(float leftMotor, float rightMotor, float time);
	void update(float dt);
	Vector position, lastPosition;
	ButtonState buttons[maxJoyBtns];
	float deadZone1, deadZone2;
	float clearRumbleTime;

	void callibrate(Vector &vec, float dead);

	float leftTrigger, rightTrigger;
	bool leftThumb, rightThumb, leftShoulder, rightShoulder, dpadLeft, dpadRight, dpadUp, dpadDown;
	bool btnStart, btnSelect;
	Vector rightStick;
	bool inited, xinited;
	bool anyButton();
#  ifdef BBGE_BUILD_SDL2
	SDL_GameController *sdl_controller;
	SDL_Haptic *sdl_haptic;
#  endif
	SDL_Joystick *sdl_joy;
#if defined(__LINUX__) && !defined(BBGE_BUILD_SDL2)
	int eventfd;
	int16_t effectid;
#endif
	int stickIndex;

	int s1ax, s1ay, s2ax, s2ay;
};

enum FollowCameraLock
{
	FCL_NONE		= 0,
	FCL_HORZ		= 1,
	FCL_VERT		= 2
};



#define RLT_FIXED


typedef std::vector <RenderObject*> RenderObjects;
typedef std::list <RenderObject*> RenderObjectList;
typedef std::map <intptr_t, RenderObject*> RenderObjectMap;

class RenderObjectLayer
{
public:
	RenderObjectLayer();
	~RenderObjectLayer();
	void add(RenderObject* r);
	void remove(RenderObject* r);
	void moveToFront(RenderObject *r);
	void moveToBack(RenderObject *r);
	void setCull(bool cull);
	void setOptimizeStatic(bool opt);
	void sort();
	void renderPass(int pass);
	void reloadDevice();

	inline bool empty()
	{
		return objectCount == 0;
	}

	inline RenderObject *getFirst()
	{
		iter = 0;
		return getNext();
	}

	RenderObject *getNext()
	{
		const int size = renderObjects.size();
		int i;
		for (i = iter; i < size; i++)
		{
			if (renderObjects[i] != 0)
				break;
		}
		if (i < size)
		{
			iter = i+1;
			return renderObjects[i];
		}
		else
		{
			iter = i;
			return 0;
		}
		return 0;
	}

	//inclusive
	int startPass, endPass;
	bool visible;
	float followCamera;

	int followCameraLock;
	bool cull;
	bool update;

	int mode;

	Vector color;

protected:

	void clearDisplayList();
	void generateDisplayList();
	inline void renderOneObject(RenderObject *robj);

	bool optimizeStatic;
	bool displayListValid;
	int displayListGeneration;
	struct DisplayListElement {
		DisplayListElement() {isList = false; u.robj = 0;}
		bool isList;  // True if this is a GL display list
		union {
			RenderObject *robj;
			GLuint listID;
		} u;
	};
	std::vector<DisplayListElement> displayList;

	RenderObjects renderObjects;
	int objectCount;
	int firstFreeIdx;
	int iter;
};

class Core : public ActionMapper, public StateManager
{
public:

	// init
	Core(const std::string &filesystem, const std::string& extraDataDir, int numRenderLayers, const std::string &appName="BBGE", int particleSize=1024, std::string userDataSubFolder="");
	void initPlatform(const std::string &filesystem);
	~Core();

	virtual void init();

	void initRenderObjectLayers(int num);

	void applyState(const std::string &state);

	bool createWindow(int width, int height, int bits, bool fullscreen, std::string windowTitle="");

	void clearBuffers();
	void render(int startLayer=-1, int endLayer=-1, bool useFrameBufferIfAvail=true);
	void showBuffer();
	void quit();
	bool isShuttingDown();
	bool isWindowFocus();

	void instantQuit();

	void cacheRender();

	void setSDLGLAttributes();

	void reloadResources();
	void unloadResources();

	std::string getPreferencesFolder();
	std::string getUserDataFolder();

	std::string adjustFilenameCase(const char *buf);
	std::string adjustFilenameCase(const std::string &str) { return adjustFilenameCase(str.c_str()); };

	void resetCamera();

	virtual void shutdown();

	void main(float runTime = -1); // can use main



	// state functions

	std::string getTextureLoadName(const std::string &texture);

	void setMousePosition(const Vector &p);

	void toggleScreenMode(int t=0);

	void enable2D(int pixelScaleX=0, int pixelScaleY=0, bool forcePixelScale=false);
	void addRenderObject(RenderObject *o, int layer=0);
	void switchRenderObjectLayer(RenderObject *o, int toLayer);
	void addTexture(Texture *r);
	CountedPtr<Texture> findTexture(const std::string &name);
	void removeTexture(Texture *res);
	void clearResources();

	CountedPtr<Texture> addTexture(const std::string &texture);

	PostProcessingFX postProcessingFx;

	enum RemoveRenderObjectFlag { DESTROY_RENDER_OBJECT=0, DO_NOT_DESTROY_RENDER_OBJECT };
	void removeRenderObject(RenderObject *r, RemoveRenderObjectFlag flag = DESTROY_RENDER_OBJECT);

	void setMouseConstraint(bool on);
	void setMouseConstraintCircle(const Vector& pos, float mouseCircle);

	void setReentryInputGrab(int on);

	void action(int id, int state){}

	bool exists(const std::string &file);

	void enqueueRenderObjectDeletion(RenderObject *object);
	void clearGarbage();


	bool isNested() { return nestedMains > 1; }
	int getNestedMains() { return nestedMains; }
	void quitNestedMain();

	int getWindowWidth() { return width; }
	int getWindowHeight() { return height; }

	void updateCursorFromJoystick(float dt, int spd);

	unsigned getTicks();

	float stopWatch(int d);


	float stopWatchStartTime;


	void resetGraphics(int w, int h, int fullscreen=-1, int vsync=-1, int bpp=-1);



	void setDockIcon(const std::string &ident);

	Vector getGameCursorPosition();
	Vector getGamePosition(const Vector &v);

	Vector joystickPosition;

	bool coreVerboseDebug;


	Vector screenCenter;

	void print(int x, int y, const char *str, float sz=1);

	// windows variables
	#ifdef BBGE_BUILD_WINDOWS
		HDC			hDC;		// Private GDI Device Context
		HGLRC		hRC;		// Permanent Rendering Context
		HWND		hWnd;		// Holds Our Window Handle
		HINSTANCE	hInstance;		// Holds The Instance Of The Application
	#endif

	std::vector<Texture*> resources;

	RenderObjectLayer *getRenderObjectLayer(int i);
	std::vector <int> renderObjectLayerOrder;

	typedef std::vector<RenderObjectLayer> RenderObjectLayers;
	RenderObjectLayers renderObjectLayers;

	RenderObjectList garbage;

	SoundManager *sound;

	float aspect;

	int width, height;

	enum Modes { MODE_NONE=-1, MODE_3D=0, MODE_2D };

	InterpolatedVector globalScale;
	Vector globalResolutionScale;
	Vector screenCapScale;

	virtual void onResetScene(){}

	virtual void onPlayedVoice(const std::string &name){}

	Vector cameraPos;

	int fps;
	bool loopDone;

	Mouse mouse;

	AfterEffectManager *afterEffectManager;

	ParticleManager *particleManager;



	void setBaseTextureDirectory(const std::string &baseTextureDirectory)
	{ this->baseTextureDirectory = baseTextureDirectory; }
	std::string getBaseTextureDirectory()
	{
		return baseTextureDirectory;
	}


	virtual bool canChangeState();
	void resetTimer();

	inline int getVirtualWidth()
	{
		return virtualWidth;
	}

	inline int getVirtualHeight()
	{
		return virtualHeight;
	}

	unsigned char *grabScreenshot(int x, int y, int w, int h);
	unsigned char *grabCenteredScreenshot(int w, int h);
	int saveScreenshotTGA(const std::string &filename);
	void save64x64ScreenshotTGA(const std::string &filename);
	void saveSizedScreenshotTGA(const std::string &filename, int sz, int crop34);
	void saveCenteredScreenshotTGA(const std::string &filename, int sz);

	bool minimized;
	std::string getEnqueuedJumpState();
	float cullRadius;
	float cullRadiusSqr;
	Vector cullCenter;
	unsigned int renderObjectCount, processedRenderObjectCount, totalRenderObjectCount;
	float invGlobalScale, invGlobalScaleSqr;

	void globalScaleChanged();

	void screenshot();

	void clearRenderObjects();

	void applyMatrixStackToWorld();
	void translateMatrixStack(float x, float y, float z=0);

	void rotateMatrixStack(float x, float y, float z);
	void scaleMatrixStack(float x, float y, float z=1);
	void rotateMatrixStack(float z);
	void setColor(float r, float g, float b, float a);

	void bindTexture(int stage, unsigned int handle);


	bool getKeyState(int k);
	bool getMouseButtonState(int m);

	int currentLayerPass;
	int keys[KEY_MAXARRAY];
	virtual void debugLog(const std::string &s);
	virtual void errorLog(const std::string &s);
	void messageBox(const std::string &title, const std::string &msg);
	bool getShiftState();
	bool getAltState();
	bool getCtrlState();
	bool getMetaState();

	virtual void generateCollisionMask(RenderObject *r){}

	DarkLayer darkLayer;

	int redBits, greenBits, blueBits, alphaBits;

	void setupRenderPositionAndScale();
	void setupGlobalResolutionScale();


	int particlesPaused;


	bool joystickEnabled;
	bool joystickOverrideMouse;


	bool debugLogTextures;


	Joystick joystick;
	void setClearColor(const Vector &c);
	Vector getClearColor();
	int flipMouseButtons;
	void initFrameBuffer();
	FrameBuffer frameBuffer;
	void updateRenderObjects(float dt);
	bool joystickAsMouse;
	virtual void prepScreen(bool t){}

	bool updateMouse;
	bool frameOutputMode;

	int overrideStartLayer, overrideEndLayer;

	void setWindowCaption(const std::string &caption, const std::string &icon);

	ParticleEffect* createParticleEffect(const std::string &name, const Vector &position, int layer, float rotz=0);

	std::string secondaryTexturePath;

	bool hasFocus();

	EventQueue eventQueue;

	float aspectX, aspectY;

	float get_old_dt() { return old_dt; }
	float get_current_dt() { return current_dt; }

	bool debugLogActive;

	void setInputGrab(bool on);

	bool isFullscreen();

	int viewOffX, viewOffY;

	int getVirtualOffX();
	int getVirtualOffY();

	void centerMouse();

	int vw2, vh2;
	Vector center;

	void enable2DWide(int rx, int ry);

	void enumerateScreenModes();

	std::vector<ScreenMode> screenModes;

	void pollEvents();

	CoreSettings settings;

	int tgaSave(const char *filename, short int width, short int height, unsigned char	pixelDepth, unsigned char	*imageData);
	int zgaSave(const char *filename, short int width, short int height, unsigned char	pixelDepth, unsigned char	*imageData);

	volatile int dbg_numThreadDecoders;
	static unsigned int dbg_numRenderCalls;

	virtual void onBackgroundUpdate();

	void initLocalization();

protected:

	std::string fpsDebugString;

	TimeUpdateType timeUpdateType;
	int fixedFPS;

	void updateCullData();

	std::string userDataFolder;

	int grabInputOnReentry;

	int virtualOffX, virtualOffY;

	void initIcon();

	float old_dt;
	float current_dt;

	std::string debugLogPath;

	virtual void onReloadResources();

	CountedPtr<Texture> doTextureAdd(const std::string &texture, const std::string &name, std::string internalTextureName);

	void deleteRenderObjectMemory(RenderObject *r);
	bool _hasFocus;
	bool lib_graphics, lib_sound, lib_input;
	Vector clearColor;
	bool updateCursorFromMouse;
	virtual void unloadDevice();
	virtual void reloadDevice();

	std::string appName;
	bool mouseConstraint;
	float mouseCircle;
	Vector mouseConstraintCenter;

	bool doMouseConstraint();

	virtual void onMouseInput(){}
	bool doScreenshot;
	float baseCullRadius;
	bool initSoundLibrary(const std::string &defaultDevice);
	bool initInputLibrary();
	bool initJoystickLibrary(int numSticks=1);
	bool initGraphicsLibrary(int w, int h, bool fullscreen, int vsync, int bpp, bool recreate=true);
	void shutdownInputLibrary();
	void shutdownJoystickLibrary();
	void shutdownGraphicsLibrary(bool kill=true);
	void shutdownSoundLibrary();

	int afterEffectManagerLayer;
	bool sortEnabled;
	Vector cameraOffset;
	std::vector<float> avgFPS;
	virtual void modifyDt(float &dt){}
	void setPixelScale(int pixelScaleX, int pixelScaleY);


	int virtualHeight, virtualWidth;

	bool shuttingDown;
	bool quitNestedMainFlag;
	bool clearedGarbageFlag;
	int nestedMains;
	std::string baseTextureDirectory;
#ifdef BBGE_BUILD_WINDOWS
	__int64 lastTime, curTime, freq;
#endif

	std::ofstream _logOut;

	int nowTicks, thenTicks;

	int _vsync, _bpp;
	bool _fullscreen;

	int numSavedScreenshots;

	CountedPtr<Texture> texError;



	int tgaSaveSeries(char	*filename,  short int width, short int height, unsigned char pixelDepth, unsigned char *imageData);
	virtual void onUpdate(float dt);
	virtual void onRender(){}

	void setupFileAccess();
	std::string _extraDataDir;
};

extern Core *core;

#include "RenderObject_inline.h"

#endif
