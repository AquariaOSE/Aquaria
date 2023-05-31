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
#ifndef BBGE_CORE_H
#define BBGE_CORE_H

#include "Base.h"
#include "RenderObject.h"
#include "SoundManager.h"
#include "Event.h"
#include "StateManager.h"
#include "Localization.h"
#include "Window.h"
#include "TextureMgr.h"

#include "DarkLayer.h"

#include "GameKeys.h"

class ParticleEffect;
class Joystick;

class ParticleManager;

struct ScreenMode
{
	ScreenMode() { x = y = hz = 0; }
	ScreenMode(int x, int y, int hz) : x(x), y(y), hz(hz) {}

	int x, y, hz;
};

struct CoreSettings
{
	CoreSettings() { renderOn = true; runInBackground = false; prebufferSounds = false; }
	bool renderOn;
	bool runInBackground;
	bool prebufferSounds;
};

enum CoreLayers
{
	LR_NONE		= -1
};

class AfterEffectManager;

class Texture;

const int baseVirtualWidth		= 800;
const int baseVirtualHeight		= 600;

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
	ButtonState extra[mouseExtraButtons];
};

struct Mouse
{
	Mouse()
	{
		scrollWheelChange = 0;
		buttonsEnabled = true;
		_wasMoved = false;
		_enableMotionEvents = true;
	}
	Vector position, lastPosition;
	MouseButtons buttons;
	MouseButtons pure_buttons;
	unsigned rawButtonMask;
	Vector change;
	bool buttonsEnabled;
	int scrollWheelChange;

	// transient; not for use outside of event handling
	bool _wasMoved;
	bool _enableMotionEvents;
};

enum FollowCameraLock
{
	FCL_NONE		= 0,
	FCL_HORZ		= 1,
	FCL_VERT		= 2
};

typedef std::vector <RenderObject*> RenderObjects;

class RenderObjectLayer
{
public:
	RenderObjectLayer();
	~RenderObjectLayer();
	void add(RenderObject* r);
	void remove(RenderObject* r);
	void moveToFront(RenderObject *r);
	void moveToBack(RenderObject *r);

	void reloadDevice();

	void prepareRender();
	void render() const;

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
		const size_t size = renderObjects.size();
		size_t i;
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
	Vector followCameraMult; // calculated based on followCameraLock
	int followCameraLock;

	bool update;

protected:
	RenderObjects renderObjects;
	std::vector<const RenderObject*> toRender;
	size_t objectCount;
	size_t firstFreeIdx;
	size_t iter;
};

class CoreWindow : public Window
{
public:
	virtual ~CoreWindow();

protected:
	virtual void onEvent(const SDL_Event& ev);
	virtual void onResize(unsigned w, unsigned h);
	virtual void onQuit();
};


class Core : public ActionMapper, public StateManager
{
	friend class CoreWindow;
public:

	// init
	Core(const std::string &filesystem, const std::string& extraDataDir, int numRenderLayers, const std::string &appName="BBGE", int particleSize=1024, std::string userDataSubFolder="");
	void initPlatform(const std::string &filesystem);
	~Core();

	virtual void init();

	void initRenderObjectLayers(int num);

	void applyState(const std::string &state);

	void clearBuffers();
	void render(int startLayer=-1, int endLayer=-1, bool useFrameBufferIfAvail=true);
	void showBuffer();
	void quit();
	bool isShuttingDown();
	bool isWindowFocus();

	void cacheRender();

	void reloadResources();
	void unloadResources();

	std::string getPreferencesFolder();
	std::string getUserDataFolder();
	std::string getDebugLogPath();

	void resetCamera();

	virtual void shutdown();

	void run(float runTime = -1); // can use main



	// state functions

	void setMousePosition(const Vector &p);

	void setFullscreen(bool full);

	void enable2D(int pixelScaleX, int pixelScaleY);
	void addRenderObject(RenderObject *o, unsigned layer);
	void switchRenderObjectLayer(RenderObject *o, unsigned toLayer);
	CountedPtr<Texture> getTexture(const std::string &name);

	enum RemoveRenderObjectFlag { DESTROY_RENDER_OBJECT=0, DO_NOT_DESTROY_RENDER_OBJECT };
	void removeRenderObject(RenderObject *r, RemoveRenderObjectFlag flag = DESTROY_RENDER_OBJECT);

	void setMouseConstraint(bool on);
	void setMouseConstraintCircle(const Vector& pos, float mouseCircle);

	virtual void action(int id, int state, int source, InputDevice device){}

	void enqueueRenderObjectDeletion(RenderObject *object);
	void clearGarbage();


	bool isNested() const { return nestedMains > 1; }
	int getNestedMains() const { return nestedMains; }
	void quitNestedMain();

	int getWindowWidth() const { return width; }
	int getWindowHeight() const { return height; }

	unsigned getTicks();

	void updateWindowDrawSize(int w, int h);

	Vector getGameCursorPosition();
	Vector getGamePosition(const Vector &v);

	Vector screenCenter;

	void print(int x, int y, const char *str, float sz=1);

	RenderObjectLayer *getRenderObjectLayer(int i);
	std::vector <int> renderObjectLayerOrder;

	typedef std::vector<RenderObjectLayer> RenderObjectLayers;
	RenderObjectLayers renderObjectLayers;

	RenderObjects garbage;

	SoundManager *sound;

	int width, height;

	InterpolatedVector globalScale;
	Vector globalResolutionScale;

	virtual void onResetScene(){}

	virtual void onPlayedVoice(const std::string &name){}

	Vector cameraPos;

	int fps;
	bool loopDone;

	Mouse mouse;

	AfterEffectManager *afterEffectManager;

	ParticleManager *particleManager;

	void setExtraTexturePath(const char *dir); // pass NULL to disable secondary
	const char *getExtraTexturePath() const; // NULL when no secondary
	const std::string& getBaseTexturePath() const;

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

	unsigned char *grabScreenshot(size_t x, size_t y, size_t w, size_t h);
	unsigned char *grabCenteredScreenshot(size_t w, size_t h);
	bool saveScreenshot(const std::string &filename, bool png);

	bool minimized;
	std::string getEnqueuedJumpState();
	float cullRadius;
	float cullRadiusSqr;
	Vector cullCenter;
	size_t renderObjectCount, processedRenderObjectCount, totalRenderObjectCount;
	float invGlobalScale, invGlobalScaleSqr;

	void globalScaleChanged();

	void screenshot();

	void clearRenderObjects();

	bool getKeyState(int k);
	bool getMouseButtonState(int m);

	int keys[KEY_MAXARRAY];
	virtual void _debugLog(const std::string &s);
	virtual void _errorLog(const std::string &s);
	void messageBox(const std::string &title, const std::string &msg);
	bool getShiftState();
	bool getAltState();
	bool getCtrlState();

	virtual void generateCollisionMask(RenderObject *r){}

	DarkLayer darkLayer;

	void setupRenderPositionAndScale();
	void setupGlobalResolutionScale();


	int particlesPaused;


	bool joystickEnabled;

	void setup_opengl();
	void setClearColor(const Vector &c);
	int flipMouseButtons;
	FrameBuffer frameBuffer;
	void updateRenderObjects(float dt);
	bool joystickAsMouse;
	virtual void prepScreen(bool t){}

	ParticleEffect* createParticleEffect(const std::string &name, const Vector &position, int layer, float rotz=0);

	float get_old_dt() { return old_dt; }
	float get_current_dt() { return current_dt; }

	bool debugLogActive;
	bool debugOutputActive;

	void setInputGrab(bool on);
	void updateInputGrab();

	bool isFullscreen();
	bool isDesktopResolution();
	int getDisplayIndex();
	int getRefreshRate();

	int getVirtualOffX();
	int getVirtualOffY();

	void centerMouse();

	Vector center;

	void enable2DWide(int rx, int ry);

	void enumerateScreenModes(int display);
	void enumerateScreenModesIfNecessary(int display = -1);

	void resizeWindow(int w, int h, int full, int bpp, int vsync, int display, int hz);

	std::vector<ScreenMode> screenModes;

	void pollEvents(float dt);
	void onEvent(const SDL_Event& event);

	CoreSettings settings;

	volatile int dbg_numThreadDecoders;

	virtual void onBackgroundUpdate();

	void initLocalization();

	TextureMgr texmgr;

protected:

	CoreWindow *window;

	void updateCullData();

	std::string userDataFolder;

	bool grabInput;

	int virtualOffX, virtualOffY;

	float old_dt;
	float current_dt;

	std::string debugLogPath;

	virtual void onReloadResources();

	bool lib_graphics, lib_sound, lib_input;
	Vector clearColor;
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
	void initJoystickLibrary();
	void initGraphicsLibrary(int w, int h, bool fullscreen, bool vsync, int bpp, int display, int hz);
	void shutdownInputLibrary();
	void shutdownJoystickLibrary();
	void shutdownGraphicsLibrary();
	void shutdownSoundLibrary();

	void detectJoysticks();
	void clearJoysticks();
	virtual void onJoystickAdded(int deviceID);
	virtual void onJoystickRemoved(int instanceID);

	int afterEffectManagerLayer;
	Vector cameraOffset;
	std::vector<float> avgFPS;
	virtual void modifyDt(float &dt){}
	void setPixelScale(int pixelScaleX, int pixelScaleY);

	virtual void onWindowResize(int w, int h);


	int virtualHeight, virtualWidth;

	bool shuttingDown;
	bool quitNestedMainFlag;
	int nestedMains;

	int nowTicks, thenTicks;

	int _lastEnumeratedDisplayIndex;

	virtual void onUpdate(float dt);
	virtual void onRender(){}

	void setupFileAccess();
	std::string _extraDataDir;

	std::vector<ActionButtonStatus*> actionStatus; // contains at least 1 element (the sentinel)
	virtual void updateActionButtons();
	void clearActionButtons();

public:
	// inclusive!
	inline int getMaxActionStatusIndex() const { return int(actionStatus.size()) - 2; }
	// pass -1 for is a sentinel that captures all input
	inline ActionButtonStatus *getActionStatus(size_t idx) { return actionStatus[idx + 1]; }

	Joystick *getJoystick(size_t idx); // warning: may return NULL/contain holes
	// not the actual number of joysticks!
	size_t getNumJoysticks() const { return joysticks.size(); }
	Joystick *getJoystickForSourceID(int sourceID);
private:
	std::vector<Joystick*> joysticks;
	unsigned sdlUserMouseEventID;
};

extern Core *core;

#include "RenderObject_inline.h"

#endif
