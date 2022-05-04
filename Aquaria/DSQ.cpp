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
#include "../BBGE/DebugFont.h"
#include "../BBGE/AfterEffect.h"

#include "DSQ.h"
#include "States.h"
#include "Game.h"
#include "Logo.h"
#include "Avatar.h"
#include "Entity.h"
#include "Avatar.h"
#include "Shot.h"
#include "GridRender.h"
#include "AnimationEditor.h"
#include "Intro.h"

#include "RoundedRect.h"
#include "TTFFont.h"
#include "ModSelector.h"
#include "Network.h"
#include "ttvfs_stdio.h"
#include "GLLoad.h"
#include "RenderBase.h"
#include "Image.h"

#include <sys/stat.h>

#ifdef BBGE_BUILD_VFS
#include "ttvfs.h"
#endif

#ifdef BBGE_BUILD_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

static void Linux_CopyTree(const char *src, const char *dst)
{


	struct stat statbuf;
	if (stat(src, &statbuf) == -1)
		return;

	if (S_ISDIR(statbuf.st_mode))
	{
		createDir(dst);  // don't care if this fails.
		DIR *dirp = opendir(src);
		if (dirp == NULL)
			return;

		struct dirent *dent;
		while ((dent = readdir(dirp)) != NULL)
		{
			if ((strcmp(dent->d_name, ".") == 0) || (strcmp(dent->d_name, "..") == 0))
				continue;
			const size_t srclen = strlen(src) + strlen(dent->d_name) + 2;
			char *subsrc = new char[srclen];
			snprintf(subsrc, srclen, "%s/%s", src, dent->d_name);
			const size_t dstlen = strlen(dst) + strlen(dent->d_name) + 2;
			char *subdst = new char[dstlen];
			snprintf(subdst, dstlen, "%s/%s", dst, dent->d_name);
			Linux_CopyTree(subsrc, subdst);
			delete[] subdst;
			delete[] subsrc;
		}
		closedir(dirp);
	}

	else if (S_ISREG(statbuf.st_mode))
	{
		const int in = open(src, O_RDONLY);
		if (in == -1)
			return;

		// fail if it already exists. That's okay in this case.
		const int out = open(dst, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if (out == -1)
		{
			close(in);
			return;
		}

		const size_t buflen = 256 * 1024;
		char *buf = new char[buflen];
		bool failed = false;
		ssize_t br = 0;
		while ( (!failed) && ((br = read(in, buf, buflen)) > 0) )
			failed = (write(out, buf, br) != br);

		if (br < 0)
			failed = true;

		delete[] buf;

		if (close(out) < 0)
			failed = true;

		close(in);

		if (failed)
			unlink(dst);
	}

	else
	{
		fprintf(stderr, "WARNING: we should have copied %s to %s, but it's not a dir or file! Skipped it.\n", src, dst);
	}
}
#endif


const size_t saveSlotPageSize = 4;
size_t maxPages = 15;
const int MAX_CONSOLELINES	= 18;

DSQ *dsq = 0;

const bool isReleaseCandidate	= false;
const bool isFinalCandidate		= false;
const bool isGoldMaster			= true;

static const Vector savesz(750.0f/1024.0f, 750.0f/1024.0f);

/// WARNING: this is just to init, the actual value is set from user settings!
#define PARTICLE_AMOUNT_DEFAULT			2048

#ifdef AQUARIA_DEMO
	#define APPNAME "Aquaria Demo"
#else
	#define APPNAME "Aquaria"
#endif

DSQ::DSQ(const std::string& fileSystem, const std::string& extraDataDir)
: Core(fileSystem, extraDataDir, LR_MAX, APPNAME, PARTICLE_AMOUNT_DEFAULT, "Aquaria")
{
	assert(!dsq);
	dsq = this;

#ifdef AQUARIA_ENABLE_CONSOLE_LOG
	this->debugOutputActive = true;
#endif

	cutscene_bg = 0;
	cutscene_text = 0;
	cutscene_text2 = 0;

	doScreenTrans = false;

	cutscenePaused = false;
	inCutscene = false;
	_canSkipCutscene = false;
	skippingCutscene = false;

	bar_left = bar_right = bar_up = bar_down = barFade_left = barFade_right = 0;

	difficulty = DIFF_NORMAL;



	watchQuitFlag = false;
	watchForQuit = false;

	particleBank1 = "data/particles/";
	particleBank2 = "";

	shotBank1 = "data/shots/";
	shotBank2 = "";

	disableMiniMapOnNoInput = true;
	noEffectTimer = 0;
	saveSlotPageCount = 0;
	inModSelector = false;
	subtext = 0;
	subbox = 0;
	modSelectorScr = 0;
	blackout = 0;
	lastInputMode = INPUT_MOUSE;
	overlay = 0;
	recentSaveSlot = -1;
	arialFontData = 0;

	achievement_text = 0;
	achievement_box = 0;

	console = 0;
	cmDebug = 0;
	saveSlotMode = SSM_NONE;
	afterEffectManagerLayer = LR_AFTER_EFFECTS; // LR_AFTER_EFFECTS
	renderObjectLayers.resize(LR_MAX);

	entities.resize(64, 0);

	shakeCameraTimer = shakeCameraMag = 0;
	avgFPS.resize(dsq->user.video.fpsSmoothing);

	cursor = cursorGlow = 0;

	for (int i = 0; i < 16; i++)
		firstElementOnLayer[i] = 0;
}

DSQ::~DSQ()
{
	assert(dsq == this);
	dsq = 0;
}

// actually toggle
void DSQ::toggleFullscreen()
{
	bool newfull = !window->isFullscreen();
	setFullscreen(newfull);
	user.video.full = newfull;
}

// for handling the input, not the actual switch functionality
void DSQ::onSwitchScreenMode()
{
	if (getAltState())
		toggleFullscreen();
}

void DSQ::onWindowResize(int w, int h)
{
	Core::onWindowResize(w, h);
	screenTransition->reloadDevice();
}

void DSQ::rumble(float leftMotor, float rightMotor, float time, int source, InputDevice device)
{
	if (device == INPUT_JOYSTICK)
	{
		if(source < 0)
			for(size_t i = 0; i < user.control.actionSets.size(); ++i)
			{
				const ActionSet& as = user.control.actionSets[i];
				if(Joystick *j = core->getJoystick(as.joystickID))
					j->rumble(leftMotor, rightMotor, time);
			}
		else if(source < (int)user.control.actionSets.size())
		{
			const ActionSet& as = user.control.actionSets[source];
			if(Joystick *j = core->getJoystick(as.joystickID))
				j->rumble(leftMotor, rightMotor, time);
		}
	}
}

void DSQ::newGame()
{
	dsq->game->resetFromTitle();
	dsq->initScene = "NaijaCave";
	dsq->game->transitionToScene(dsq->initScene);
}

void DSQ::loadElementEffects()
{
	bool found = false;
	std::string fn;
	if (dsq->mod.isActive())
	{
		fn = dsq->mod.getPath() + "elementeffects.txt";
		if(exists(fn))
			found = true;
	}
	if(!found)
		fn = "data/elementeffects.txt";

	InStream inFile(fn.c_str());
	elementEffects.clear();
	std::string line;
	while (std::getline(inFile, line))
	{
		debugLog("Line: " + line);
		std::istringstream is(line);
		ElementEffect e;
		int efxType = EFX_NONE;
		int idx;
		std::string type;
		is >> idx >> type;
		if (type == "EFX_SEGS")
		{
			efxType = EFX_SEGS;
			is >> e.segsx >> e.segsy >> e.segs_dgox >> e.segs_dgoy >> e.segs_dgmx >> e.segs_dgmy >> e.segs_dgtm >> e.segs_dgo;
		}
		else if (type == "EFX_WAVY")
		{
			debugLog("loading wavy");
			efxType = EFX_WAVY;
			is >> e.segsy >> e.wavy_radius >> e.wavy_flip;

		}
		else if (type == "EFX_ALPHA")
		{
			efxType = EFX_ALPHA;
			float to_x, time, loop, pingPong, ease;
			is >> e.blendType >> e.alpha.x >> to_x >> time >> loop >> pingPong >> ease;
			e.alpha.interpolateTo(to_x, time, loop, pingPong, ease);
		}
		e.type = efxType;
		elementEffects.push_back(e);
	}
	inFile.close();
}

ElementEffect DSQ::getElementEffectByIndex(size_t e)
{
	if (e < elementEffects.size())
	{
		return elementEffects[e];
	}

	ElementEffect empty;
	empty.type = EFX_NONE;
	empty.alpha = 0;
	empty.blendType = 0;
	empty.color = 0;
	empty.segsx = empty.segsy = 0;
	empty.segs_dgmx = empty.segs_dgmy = 0;
	empty.segs_dgo = 0;
	empty.segs_dgox = empty.segs_dgoy = 0;
	empty.segs_dgtm = 0;
	empty.wavy_flip = false;
	empty.wavy_max = empty.wavy_min = 0;
	empty.wavy_radius = 0;

	return empty;
}

void DSQ::centerMessage(const std::string &text, float y, int type)
{
	Vector pos(400,y);
	float time = 2;

	BitmapText *t = 0;
	if (type == 1)
		t = new BitmapText(&smallFontRed);
	else
		t = new BitmapText(&smallFont);
	t->position = pos;
	t->alpha.ensureData();
	t->alpha.data->path.addPathNode(1, 0);
	t->alpha.data->path.addPathNode(1, 0.8f);
	t->alpha.data->path.addPathNode(0, 1);
	t->alpha.startPath(time);
	t->followCamera = 1;
	t->setLife(time + 0.5f);
	t->setDecayRate(1);
	t->setText(text);
	t->offset.interpolateTo(Vector(0, -40), 2, 0, 0, 1);
	getTopStateData()->addRenderObject(t, LR_OVERLAY);
}

void DSQ::centerText(const std::string &text)
{
	Vector pos(400,200);
	float time = 8;

	BitmapText *s = new BitmapText(&font);
	s->color = Vector(0,0,0);
	s->position = pos;
	s->offset = Vector(1,1);
	s->setText(text);
	s->setLife(time + 0.5f);
	s->setDecayRate(1);
	s->followCamera = 1;
	s->alpha.ensureData();
	s->alpha.data->path.addPathNode(0, 0);
	s->alpha.data->path.addPathNode(1, 0.1f);
	s->alpha.data->path.addPathNode(1, 0.8f);
	s->alpha.data->path.addPathNode(0, 1);
	s->alpha.startPath(time);
	getTopStateData()->addRenderObject(s, LR_HUD);


	BitmapText *t = new BitmapText(&font);


	t->position =pos;
	t->alpha.ensureData();
	t->alpha.data->path.addPathNode(0, 0);
	t->alpha.data->path.addPathNode(1, 0.1f);
	t->alpha.data->path.addPathNode(1, 0.8f);
	t->alpha.data->path.addPathNode(0, 1);
	t->alpha.startPath(time);

	t->followCamera = 1;
	t->setLife(time + 0.5f);
	t->setDecayRate(1);

	t->setText(text);
	getTopStateData()->addRenderObject(t, LR_HUD);
}

void DSQ::destroyFonts()
{

	debugLog("destroyFonts...");
	font.destroy();
	smallFont.destroy();
	subsFont.destroy();
	goldFont.destroy();
	smallFontRed.destroy();

	debugLog("ttf fonts...");
	fontArialBig.destroy();
	fontArialSmall.destroy();
	fontArialSmallest.destroy();
	delete[] arialFontData;
	arialFontData = 0;

	debugLog("done destroyFonts");
}

void DSQ::loadFonts()
{
	debugLog("loadFonts...");

	destroyFonts();

	std::string file = localisePath("data/font-small.glf");

	font.load(file, 1, false);
	font.fontTopColor = Vector(0.9f,0.9f,1);
	font.fontBtmColor = Vector(0.5f,0.8f,1);
	font.overrideTexture = core->addTexture("font");

	smallFont.load(file, 0.6f, false);
	smallFont.fontTopColor = Vector(0.9f,0.9f,1);
	smallFont.fontBtmColor = Vector(0.5f,0.8f,1);
	smallFont.overrideTexture = core->addTexture("font");

	smallFontRed.load(file, 0.6f, false);
	smallFontRed.fontTopColor = Vector(1,0.9f,0.9f);
	smallFontRed.fontBtmColor = Vector(1,0.8f,0.5f);
	smallFontRed.overrideTexture = core->addTexture("font");

	subsFont.load(file, 0.5f, false);
	subsFont.fontTopColor = Vector(1,1,1);
	subsFont.fontBtmColor = Vector(0.5f,0.8f,1);
	subsFont.overrideTexture = core->addTexture("font");

	goldFont.load(file, 1, false);
	goldFont.fontTopColor = Vector(1,0.9f,0.5f);
	goldFont.fontBtmColor = Vector(0.6f,0.5f,0.25f);
	goldFont.overrideTexture = core->addTexture("font");


	file = localisePath("data/font.ttf");

	debugLog("ttf...");
	if(arialFontData)
		delete [] arialFontData;
	arialFontData = (unsigned char *)readFile(file.c_str(), &arialFontDataSize);
	if (arialFontData)
	{
		fontArialSmall   .create(arialFontData, arialFontDataSize, 12);
		fontArialBig     .create(arialFontData, arialFontDataSize, 18);
		fontArialSmallest.create(arialFontData, arialFontDataSize, 10);
	}
	debugLog("done loadFonts");



}

void DSQ::onReloadResources()
{
	Core::onReloadResources();

	loadFonts();

	setTexturePointers();
}

void DSQ::debugMenu()
{
	if (isDeveloperKeys() || (mod.isActive() && mod.isDebugMenu()))
	{

		if (core->getShiftState())

		{
			core->frameOutputMode = false;
			dsq->game->togglePause(true);
			std::string s = dsq->getUserInputString(stringbank.get(2012), "");
			stringToUpper(s);



			if (!dsq->game->isSceneEditorActive())
				dsq->game->togglePause(false);
			if (!s.empty())
			{
				char c = s[0];
				int i = 0;
				if (c == '1')
				{
					v.load();

					core->particleManager->loadParticleBank(particleBank1, particleBank2);

					// important: kill all shots before reloading the shot bank
					// still might crash here
					Shot::killAllShots();
					Shot::loadShotBank(shotBank1, shotBank2);
					dsq->continuity.loadEatBank();

					dsq->game->loadEntityTypeList();
					if (core->afterEffectManager)
					{
						core->afterEffectManager->loadShaders();
					}
					dsq->user.load();
					dsq->continuity.loadIngredientData();
				}
				else if (c == '2')
				{
					if (dsq->game && dsq->game->avatar)
					{
						dsq->game->avatar->heal(999);
					}
				}
				else if (c == '3')
				{
					dsq->continuity.reset();
				}
				else if (c == 'B')
				{
					dsq->unloadResources();
				}
				else if (c == 'A')
				{
					dsq->reloadResources();
				}
				else if (c == 'J')
				{
					std::istringstream is(s);
					std::string state;
					char read = ' ';
					is >> read >> state;

					dsq->quitNestedMain();
					dsq->enqueueJumpState(state);
				}
				else if (c == 'Q')
				{
					dsq->quitNestedMain();
				}

				else if (c == '5')
				{

					dsq->game->invinciblity = !dsq->game->invinciblity;
				}
				else if (c == '6')
				{
					while (core->getKeyState(KEY_RETURN))
						core->run(0.1f);
					dsq->setStory();
				}
				else if (c == '8')
				{
					for (i = 0; i <= SONG_MAX; i++)
						dsq->continuity.learnSong(i);
				}
				else if (c == '9')
				{
					for (i = 0; i <= SONG_MAX; i++)
						dsq->continuity.learnSong(i);
					for (i = 0; i < FORMUPGRADE_MAX; i++)
					{
						dsq->continuity.learnFormUpgrade((FormUpgradeType)i);
					}
				}
				else if (c == '0')
				{
					dsq->continuity.learnSong(SONG_SHIELDAURA);
					dsq->continuity.learnSong(SONG_ENERGYFORM);
					dsq->continuity.learnSong(SONG_BIND);
				}
				else if (c == 'S')
				{
					std::istringstream is(s);
					int num = 0;
					char read=' ';
					is >> read >> num;
					dsq->continuity.learnSong(num);


				}
				else if (c == 'F')
				{
					std::istringstream is(s);
					char read = ' ';
					std::string entityName;
					is >> read >> entityName;
					Entity *e = dsq->getEntityByNameNoCase(entityName);
					if (e)
					{
						dsq->cameraPos = game->getCameraPositionFor(e->position);
					}
				}
				else if (c == 'C')
				{
					std::istringstream is(s);
					std::string nm;
					char read=' ';
					is >> read >> nm;
					dsq->continuity.setCostume(nm);
				}
				else if (c == 'R')
				{
					dsq->demo.toggleRecord(true);
				}
				else if (c == 'P')
				{
					dsq->demo.togglePlayback(true);
				}
				else if (c == 'T')
				{
					if (dsq->demo.frames.size()> 0)
					{
						dsq->game->avatar->position = dsq->demo.frames[0].avatarPos;
						dsq->game->avatar->rotation.z = dsq->demo.frames[0].rot;
					}
				}
				else if (c == 'U')
				{
					dsq->demo.renderFramesToDisk();


				}
				else if (c == 'K')
				{
					dsq->demo.clearRecordedFrames();
				}
				else if (c == 'H')
				{
					std::ostringstream os;
					os << dsq->game->avatar->health;
					std::istringstream is(dsq->getUserInputString(stringbank.get(2013), os.str()));
					float h = 0;
					is >> h;
					dsq->game->avatar->health = h;

				}
			}



		}
	}
}

void DSQ::takeScreenshotKey()
{
	if (getCtrlState() && getAltState())
		screenshot();
}

Quad *loading=0;

float loadingProgress = 0;
static const float loadingProgressTable[] = {
	#define LOAD_INITIAL	0  // Initial display (just so it's not empty)
	#define LOAD_PARTICLES	1  // After loading particles and shots
	#define LOAD_SOUNDCACHE	2  // After loading the sound cache
	#define LOAD_FONTS		3  // After loading fonts
	#define LOAD_GRAPHICS1	4  // After creating graphics resources
	#define LOAD_GRAPHICS2	5  // After creating more graphics resources
	#define LOAD_TEXTURES	6  // After loading textures to be precached
	#define LOAD_FINISHED	7  // All done!
	0.01f, 0.07f, 0.20f, 0.23f, 0.24f, 0.25f, 0.89f, 1.00f,
};

void loadBit(int index, float perc = 1)
{
	const float previous = index==0 ? 0 : loadingProgressTable[index-1];
	const float next = loadingProgressTable[index];
	loadingProgress = MIN(1, previous + ((next - previous) * MIN(1, perc)));

	loading->setWidthHeight(loadingProgress*600, 23);

	core->render();
	core->showBuffer();
}

unsigned int soundsLoaded = 0;
const unsigned int soundsExpected = 195;
void loadBitForSoundCache()
{
	if (soundsLoaded > 0 && soundsLoaded < soundsExpected)
	{
		// Only update every 4 sounds so we don't waste too much
		// time waiting for vsync.
		if (soundsLoaded % 4 == 0)
		{
			loadBit(LOAD_SOUNDCACHE,
					(float)soundsLoaded / soundsExpected);
		}
	}
	soundsLoaded++;
}

unsigned int texturesLoaded = 0;
const unsigned int texturesExpected = 652;
void loadBitForTexPrecache()
{
	if (texturesLoaded > 0 && texturesLoaded < texturesExpected)
	{
		if (texturesLoaded % 16 == 0)
		{
			loadBit(LOAD_TEXTURES,
					(float)texturesLoaded / texturesExpected);
		}
	}
	texturesLoaded++;
}


void DSQ::setVersionLabelText()
{
#ifdef AQUARIA_OVERRIDE_VERSION_STRING
	std::string overrideText = AQUARIA_OVERRIDE_VERSION_STRING;
	if(user.system.allowDangerousScriptFunctions)
		overrideText += stringbank.get(2050);
	versionLabel->setText(overrideText);
	return;
#endif

	std::ostringstream os;
	os << "Aquaria";

#ifdef AQUARIA_DEMO
	os << " Demo";
#endif

	os << " v" << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION;

	if (!isFinalCandidate && !isGoldMaster && VERSION_BETA)
	{
		os << "b" << VERSION_BETA;
	}

	if (isFinalCandidate)
	{
		os << "fc" << VERSION_FC;
	}
	else if (isReleaseCandidate)
	{
		os << "RC";
	}
	else if (isGoldMaster)
	{

	}

	#ifdef AQUARIA_CUSTOM_BUILD_ID
	os << AQUARIA_CUSTOM_BUILD_ID;
	#endif

	if(user.system.allowDangerousScriptFunctions)
		os << stringbank.get(2050);

	versionLabel->setText(os.str());
}

static bool sdlVideoModeOK(int disp, const int w, const int h, const int bpp)
{
	if(!w && !h)
		return true;
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_DisplayMode mode;
	const int modecount = SDL_GetNumDisplayModes(disp);
	for (int i = 0; i < modecount; i++) {
		SDL_GetDisplayMode(disp, i, &mode);
		if (!mode.w || !mode.h || (w >= mode.w && h >= mode.h)) {
			return true;
		}
	}
	return false;
#else
	return SDL_VideoModeOK(w, h, bpp, SDL_OPENGL | SDL_FULLSCREEN);
#endif
}

void DSQ::init()
{
	core->settings.runInBackground = true;

#ifdef BBGE_BUILD_WINDOWS
	/*
	const std::string welcomeMessage = \
"Thank you for reviewing Aquaria!\n\
This build is not yet final, and as such there are a couple things lacking. They include:\n\
  * documentation (not 100% complete)\n\
  * mod editing (some known issues)\n\
  * gamepad support/config (some known issues)\n\
\nFor the best experience, we recommend playing using the mouse or mouse and keyboard.\n\
			Have fun!";

	if (!welcomeMessage.empty())
	{
		MessageBox(core->hWnd, welcomeMessage.c_str(), "Welcome to Aquaria", MB_OK);
	}
	*/
#endif

	disableMiniMapOnNoInput = true;
	fpsText = 0;
	cmDebug = 0;

	debugLog("DSQ init");

	useFrameBuffer = false;
	gameSpeed = 1;

	// steam gets inited in here
	Core::init();

	loadStringBank();

	vars = &v;
	v.load();

	// steam callbacks are inited here
	continuity.init();

	// do copy stuff
#ifdef BBGE_BUILD_UNIX
	std::string fn;
	fn = getPreferencesFolder() + "/" + userSettingsFilename;
	if (!exists(fn))
		Linux_CopyTree(adjustFilenameCase(userSettingsFilename).c_str(), adjustFilenameCase(fn).c_str());

	fn = getUserDataFolder() + "/_mods";
	if (!exists(fn))
		Linux_CopyTree(adjustFilenameCase("_mods").c_str(), adjustFilenameCase(fn).c_str());
#endif

	createDir(getUserDataFolder());
	createDir(getUserDataFolder() + "/save");
	createDir(getUserDataFolder() + "/_mods");
	createDir(getUserDataFolder() + "/screenshots");

	addStateInstance(game = new Game);
	addStateInstance(new GameOver);
	addStateInstance(new AnimationEditor);
	addStateInstance(new Intro2);
	addStateInstance(new BitBlotLogo);
	addStateInstance(new ParticleEditor);
	addStateInstance(new Credits);
	addStateInstance(new Intro);
	addStateInstance(new Nag);



	this->setBaseTextureDirectory("gfx/");

	voiceOversEnabled = true;



	user.load(false);

	particleManager->setSize(user.video.numParticles);

	bool fullscreen = user.video.full;
	useFrameBuffer = user.video.fbuffer;

	if (isDeveloperKeys())
	{
		maxPages = 600/saveSlotPageSize;
	}

	if (isDeveloperKeys())
		debugLog("DeveloperKeys Enabled");
	else
		debugLog("DeveloperKeys Disabled");

	if (voiceOversEnabled)
		debugLog("VoiceOvers Enabled");
	else
		debugLog("VoiceOvers Disabled");

	SDL_Init(SDL_INIT_VIDEO);
	if (fullscreen && !sdlVideoModeOK(user.video.displayindex, user.video.resx, user.video.resy, user.video.bits))
	{
#if SDL_VERSION_ATLEAST(2,0,0)
		SDL_DisplayMode mode, closest;
		mode.format = 0;
		mode.driverdata = 0;
		mode.w = user.video.resx;
		mode.h = user.video.resy;
		mode.refresh_rate = user.video.hz;
		if(SDL_GetClosestDisplayMode(user.video.displayindex, &mode, &closest))
		{
			user.video.resx = closest.w;
			user.video.resy = closest.h;
			user.video.hz = closest.refresh_rate;
		}
		else
#endif
		{
			// maybe we can force a sane resolution if SetVideoMode is going to fail...
			user.video.resx = 800;
			user.video.resy = 600;
			user.video.hz = 60;
			user.video.bits = 32;
			user.video.displayindex = 0;
			if (!sdlVideoModeOK(0, user.video.resx, user.video.resy, user.video.bits))
				fullscreen = false;  // last chance.
		}
	}

	debugLog("Init Graphics Library...");
		initGraphicsLibrary(user.video.resx, user.video.resy, fullscreen, user.video.vsync, user.video.bits, user.video.displayindex, user.video.hz);
	debugLog("OK");

	debugLog("Init Sound Library...");
		initSoundLibrary(user.audio.deviceName);
		debugLog("Set Voice Fader");
		sound->setVoiceFader(0.5);
		sound->event_playVoice.set(MakeFunctionEvent(DSQ, onPlayVoice));
		sound->event_stopVoice.set(MakeFunctionEvent(DSQ, onStopVoice));
	debugLog("OK");

	debugLog("Init Input Library...");
		initInputLibrary();
	debugLog("OK");

	if (user.control.joystickEnabled)
	{
		debugLog("Init Joystick Library...");
			initJoystickLibrary();

		debugLog("OK");
	}

	user.apply();

	applyPatches();

	loading = new Quad("loading/juice", Vector(400,300));
	loading->alpha = 1.0;
	loading->followCamera = 1;
	loading->setWidthHeight(0,0);
	addRenderObject(loading, LR_HUD);

	Vector loadShift(2, 0);


	Vector sz(800.0f/1024.0f, 600.0f/768.0f);



	Quad *tube = new Quad("loading/tube", Vector(400, 300));
	tube->followCamera = 1;
	tube->scale = sz;
	addRenderObject(tube, LR_HUD);

	Quad *label = new Quad("loading/label", Vector(400, 300));
	label->followCamera = 1;
	label->scale = sz;
	addRenderObject(label, LR_HUD);

	int sideOut = 300, sideDown = 8;

	Quad *sidel = new Quad("loading/side", Vector(400-sideOut, 300+sideDown));
	sidel->followCamera = 1;
	sidel->scale = sz;
	addRenderObject(sidel, LR_HUD);

	Quad *sider = new Quad("loading/side", Vector(400+sideOut, 300+sideDown));
	sider->flipHorizontal();
	sider->followCamera = 1;
	sider->scale = sz;
	addRenderObject(sider, LR_HUD);



	core->render();
	core->showBuffer();



	loadBit(LOAD_INITIAL);

	debugLog("Loading Particle Bank...");
	{
		core->particleManager->loadParticleBank(particleBank1, particleBank2);
		Shot::loadShotBank(shotBank1, shotBank2);
	}
	debugLog("OK");

	loadBit(LOAD_PARTICLES);



	debugLog("Loading Sound Cache...");
		sound->loadSoundCache("sfx/cache/", ".ogg", loadBitForSoundCache);
	debugLog("OK");

	loadBit(LOAD_SOUNDCACHE);


	debugLog("Init Script Interface...");
		scriptInterface.init();
	debugLog("OK");

	loadFonts();

	loadBit(LOAD_FONTS);

	setTexturePointers();

	cursor = new Quad;
	{
		cursor->alphaMod = 0.5;
		cursor->toggleCull(false);
		cursor->followCamera = 1;
		cursor->setWidthHeight(24, 24);
		cursor->alpha = 0;
	}
	addRenderObject (cursor, LR_CURSOR);

	user.video.darkbuffersize = MAX(user.video.darkbuffersize,128);

	dsq->darkLayer.setLayers(LR_ELEMENTS13, LR_AFTER_EFFECTS);
	debugLog("dark layer init");
	dsq->darkLayer.init(user.video.darkbuffersize, user.video.darkfbuffer);
	debugLog("dark layer togle...");
	dsq->darkLayer.toggle(0);
	debugLog("done");

	debugLog("post FX init");
	dsq->postProcessingFx.init();
	debugLog("done");


	debugLog("Creating console");
	console = new DebugFont;
	{
		console->followCamera = 1;
		console->alpha = 0;
		console->setFontSize(6);
	}
	addRenderObject(console, LR_DEBUG_TEXT);

	debugLog("1");

	if (isDeveloperKeys())
	{

		cmDebug = new DebugFont();
		{
			cmDebug->followCamera = 1;
			cmDebug->alpha = 0;
			cmDebug->setFontSize(6);
		}
		addRenderObject(cmDebug, LR_DEBUG_TEXT);
	}

	debugLog("2");

	versionLabel = new BitmapText(&smallFont);
	{
	setVersionLabelText();

	versionLabel->followCamera = 1;
	versionLabel->setAlign(ALIGN_LEFT);
	versionLabel->scale = Vector(0.7f, 0.7f);
	versionLabel->alphaMod = 0.75f;
	versionLabel->alpha = 0;
	}
	addRenderObject(versionLabel, LR_REGISTER_TEXT);


	subbox = new Quad();
	subbox->position = Vector(400,580);
	subbox->alpha = 0;
	subbox->alphaMod = 0.7f;
	subbox->followCamera = 1;
	subbox->autoWidth = AUTO_VIRTUALWIDTH;
	subbox->setHeight(40);
	subbox->color = 0;
	addRenderObject(subbox, LR_SUBTITLES);

	subtext = new BitmapText(&dsq->subsFont);

	subtext->position = Vector(400,570);
	subtext->followCamera = 1;
	subtext->alpha = 0;
	subtext->setFontSize(14);
	subtext->setWidth(800);
	subtext->setAlign(ALIGN_CENTER);

	addRenderObject(subtext, LR_SUBTITLES);

	achievement_box = new Quad();
	achievement_box->position = Vector(800,0);
	achievement_box->alpha = 0;
	achievement_box->alphaMod = 0.7f;
	achievement_box->followCamera = 1;
	achievement_box->setWidthHeight(400, 87);
	achievement_box->color = 0;
	addRenderObject(achievement_box, LR_SUBTITLES);

	achievement_text = new BitmapText(&dsq->subsFont);
	achievement_text->position = Vector(603, 5);
	achievement_text->followCamera = 1;
	achievement_text->alpha = 0;
	achievement_text->setFontSize(6);
	achievement_text->setWidth(280);
	achievement_text->setAlign(ALIGN_LEFT);
	addRenderObject(achievement_text, LR_SUBTITLES);

	cutscene_bg = new Quad();
	cutscene_bg->autoWidth = AUTO_VIRTUALWIDTH;
	cutscene_bg->color = 0;
	cutscene_bg->alphaMod = 0.75;
	cutscene_bg->setWidthHeight(0, 80);
	cutscene_bg->position = Vector(400,300);
	cutscene_bg->alpha.x = 0;
	cutscene_bg->followCamera = 1;
	addRenderObject(cutscene_bg, LR_SUBTITLES);

	cutscene_text = new BitmapText(&dsq->font);
	cutscene_text->setText(stringbank.get(2004));
	cutscene_text->position = Vector(400,300-16);
	cutscene_text->alpha.x = 0;
	cutscene_text->followCamera = 1;
	addRenderObject(cutscene_text, LR_SUBTITLES);

	cutscene_text2 = new BitmapText(&dsq->smallFont);
	cutscene_text2->setText(stringbank.get(2005));
	cutscene_text2->position = Vector(400,300+10);
	cutscene_text2->alpha.x = 0;
	cutscene_text2->followCamera = 1;
	addRenderObject(cutscene_text2, LR_SUBTITLES);

	debugLog("3");

	loadBit(LOAD_GRAPHICS1);

	debugLog("4");

	cursorGlow = new Quad;
	{
		cursorGlow->setTexture("glow");
		cursorGlow->setWidthHeight(48, 48);
		cursorGlow->alpha = 0;
		cursorGlow->setBlendType(RenderObject::BLEND_ADD);
	}
	cursor->addChild(cursorGlow, PM_NONE, RBP_OFF);
	addRenderObject(cursorGlow, LR_CURSOR);

	cursorBlinker = new Quad;
	{
		cursorBlinker->setTexture("cursor");
		cursorBlinker->scale = Vector(1.5f, 1.5f);
		cursorBlinker->scale.interpolateTo(Vector(2,2), 0.2f, -1, 1, 1);
		cursorBlinker->alpha = 0;
		cursorBlinker->alphaMod = 0.5f;
	}
	cursor->addChild(cursorBlinker, PM_NONE, RBP_OFF);
	addRenderObject(cursorBlinker, LR_CURSOR);

	debugLog("5");

	recreateBlackBars();

	debugLog("6");

	overlay = new Quad;
	{
		overlay->position = Vector(400,300,3);
		overlay->color = 0;
		overlay->autoWidth = AUTO_VIRTUALWIDTH;
		overlay->autoHeight = AUTO_VIRTUALHEIGHT;
		overlay->alpha = 0;
		overlay->followCamera = 1;
	}
	addRenderObject(overlay, LR_OVERLAY);

	overlay2 = new Quad;
	{
		overlay2->position = Vector(400,300);
		overlay2->color = 0;
		overlay2->autoWidth = AUTO_VIRTUALWIDTH;
		overlay2->autoHeight = AUTO_VIRTUALHEIGHT;
		overlay2->alpha = 0;
		overlay2->followCamera = 1;
	}
	addRenderObject(overlay2, LR_OVERLAY);

	overlay3 = new Quad;
	{
		overlay3->position = Vector(400,300);
		overlay3->color = 0;
		overlay3->autoWidth = AUTO_VIRTUALWIDTH;
		overlay3->autoHeight = AUTO_VIRTUALHEIGHT;
		overlay3->alpha = 0;
		overlay3->followCamera = 1;
	}
	addRenderObject(overlay3, LR_OVERLAY);

	overlayRed = new Quad;
	{
		overlayRed->position = Vector(400,300);
		overlayRed->color = Vector(1,0,0);
		overlayRed->alphaMod = 0.5;
		overlayRed->autoWidth = AUTO_VIRTUALWIDTH;
		overlayRed->autoHeight = AUTO_VIRTUALHEIGHT;
		overlayRed->alpha = 0;
		overlayRed->followCamera = 1;
	}
	addRenderObject(overlayRed, LR_OVERLAY);

	sceneColorOverlay = new Quad;
	{
		sceneColorOverlay->position = Vector(400,300);
		sceneColorOverlay->color = Vector(1,1,1);
		sceneColorOverlay->alpha = 1;
		sceneColorOverlay->setBlendType(RenderObject::BLEND_MULT);
		sceneColorOverlay->autoWidth = AUTO_VIRTUALWIDTH;
		sceneColorOverlay->autoHeight = AUTO_VIRTUALHEIGHT;
		sceneColorOverlay->followCamera = 1;
	}
	addRenderObject(sceneColorOverlay, LR_SCENE_COLOR);

	tfader = new Quad;
	{
		tfader->position = Vector(400,300,3);
		tfader->color = 0;
		tfader->autoWidth = AUTO_VIRTUALWIDTH;
		tfader->autoHeight = AUTO_VIRTUALHEIGHT;
		tfader->alpha = 0;
		tfader->followCamera = 1;
	}
	addRenderObject(tfader, LR_TRANSITION);

	screenTransition = new AquariaScreenTransition();
	{
		screenTransition->position = Vector(400,300);
	}
	addRenderObject(screenTransition, LR_TRANSITION);

	debugLog("8");

	loadBit(LOAD_GRAPHICS2);

	debugLog("9");

	fpsText = new DebugFont;
	{
		fpsText->color = Vector(1,1,1);
		fpsText->setFontSize(6);
		fpsText->setText("FPS");
		fpsText->alpha= 0;

	}
	addRenderObject(fpsText, LR_DEBUG_TEXT);

	precacher.precacheList("data/precache.txt", loadBitForTexPrecache);

	setTexturePointers();

	loadBit(LOAD_TEXTURES);

	resetLayerPasses();

	renderObjectLayerOrder[LR_BACKGROUND_ELEMENTS1] = LR_ELEMENTS1;
	renderObjectLayerOrder[LR_BACKGROUND_ELEMENTS2] = LR_ELEMENTS2;
	renderObjectLayerOrder[LR_BACKGROUND_ELEMENTS3] = LR_ELEMENTS3;

	renderObjectLayerOrder[LR_FOREGROUND_ELEMENTS1] = LR_ELEMENTS8;
	renderObjectLayerOrder[LR_FOREGROUND_ELEMENTS2] = LR_ELEMENTS9;

	renderObjectLayerOrder[LR_BACKDROP_ELEMENTS1] = LR_ELEMENTS10;
	renderObjectLayerOrder[LR_BACKDROP_ELEMENTS2] = LR_ELEMENTS11;
	renderObjectLayerOrder[LR_BACKDROP_ELEMENTS3] = LR_ELEMENTS12;

	renderObjectLayerOrder[LR_ENTITIES_MINUS4_PLACEHOLDER] = LR_ENTITIES_MINUS4;
	renderObjectLayerOrder[LR_ENTITIES_MINUS3_PLACEHOLDER] = LR_ENTITIES_MINUS3;
	renderObjectLayerOrder[LR_ENTITIES_MINUS2_PLACEHOLDER] = LR_ENTITIES_MINUS2;

	renderObjectLayerOrder[LR_DARK_LAYER] = LR_ELEMENTS13;

	renderObjectLayerOrder[LR_BACKDROP_ELEMENTS4] = LR_ELEMENTS14;
	renderObjectLayerOrder[LR_BACKDROP_ELEMENTS5] = LR_ELEMENTS15;
	renderObjectLayerOrder[LR_BACKDROP_ELEMENTS6] = LR_ELEMENTS16;

	renderObjectLayerOrder[LR_ELEMENTS1] = -1;
	renderObjectLayerOrder[LR_ELEMENTS2] = -1;
	renderObjectLayerOrder[LR_ELEMENTS3] = -1;

	renderObjectLayerOrder[LR_ELEMENTS8] = -1;
	renderObjectLayerOrder[LR_ELEMENTS9] = -1;

	renderObjectLayerOrder[LR_ELEMENTS10] = -1;
	renderObjectLayerOrder[LR_ELEMENTS11] = -1;
	renderObjectLayerOrder[LR_ELEMENTS12] = -1;

	renderObjectLayerOrder[LR_ELEMENTS14] = -1;
	renderObjectLayerOrder[LR_ELEMENTS15] = -1;
	renderObjectLayerOrder[LR_ELEMENTS16] = -1;

	renderObjectLayerOrder[LR_ELEMENTS13] = -1;

	renderObjectLayerOrder[LR_ENTITIES_MINUS4] = -1;
	renderObjectLayerOrder[LR_ENTITIES_MINUS3] = -1;
	renderObjectLayerOrder[LR_ENTITIES_MINUS2] = -1;



	setMousePosition(core->center);



	loadBit(LOAD_FINISHED);



	// Don't do transitions for a faster start up in dev mode
	if (!isDeveloperKeys())
	{
		float trans = 0.5;
		overlay->alpha.interpolateTo(1, trans);
		core->run(trans);
	}

	removeRenderObject(loading);
	loading = 0;
	removeRenderObject(sidel);
	removeRenderObject(sider);
	removeRenderObject(label);
	removeRenderObject(tube);

	if (useFrameBuffer && core->frameBuffer.isInited())
		core->afterEffectManager = new AfterEffectManager(vars->afterEffectsXDivs,vars->afterEffectsYDivs);
	else
		core->afterEffectManager = 0;

	bindInput();
	setInputGrab(user.system.grabInput);

	// Go directly to the title in dev mode
	if(isDeveloperKeys())
		title();
	else
		enqueueJumpState("BitBlotLogo");
}

void DSQ::recreateBlackBars()
{
	bool useOldAlpha=false;
	InterpolatedVector abar_left, abar_right, abarFade_left, abarFade_right;

	if (bar_left)
	{
		useOldAlpha = true;
		abar_left = bar_left->alpha;
		removeRenderObject(bar_left);
	}
	if (bar_right)
	{
		abar_right = bar_right->alpha;
		removeRenderObject(bar_right);
	}
	if (barFade_left)
	{
		abarFade_left = barFade_left->alpha;
		removeRenderObject(barFade_left);
	}
	if (barFade_right)
	{
		abarFade_right = barFade_right->alpha;
		removeRenderObject(barFade_right);
	}
	if (bar_up)
		removeRenderObject(bar_up);
	if (bar_down)
		removeRenderObject(bar_down);

	bar_left = bar_right = bar_up = bar_down = barFade_left = barFade_right = 0;

	if (getVirtualWidth() > 800)
	{
		int sz2 = (core->getVirtualWidth() - baseVirtualWidth)/2.0f;

		bar_left = new Quad;
		{
			bar_left->position = Vector(-sz2, 300);
			bar_left->setWidthHeight(sz2*2, 600);
			bar_left->color = 0;
			bar_left->followCamera = 1;
			bar_left->cull = 0;
		}
		addRenderObject(bar_left, LR_BLACKBARS);

		bar_right = new Quad;
		{
			bar_right->position = Vector(800 + sz2, 300);
			bar_right->setWidthHeight(sz2*2, 600);
			bar_right->color = 0;
			bar_right->followCamera = 1;
			bar_right->cull = 0;
		}
		addRenderObject(bar_right, LR_BLACKBARS);

		barFade_left = new Quad;
		{
			barFade_left->setTexture("gui/edge");
			barFade_left->position = Vector(16, 300);
			barFade_left->setWidthHeight(34, 620);
			barFade_left->color = 0;
			barFade_left->followCamera = 1;
			barFade_left->cull = 0;
		}
		addRenderObject(barFade_left, LR_BLACKBARS);

		barFade_right = new Quad;
		{
			barFade_right->setTexture("gui/edge");
			barFade_right->flipHorizontal();
			barFade_right->position = Vector(800-16, 300);
			barFade_right->setWidthHeight(34, 620);
			barFade_right->color = 0;
			barFade_right->followCamera = 1;
			barFade_right->cull = 0;
		}
		addRenderObject(barFade_right, LR_BLACKBARS);

		if (useOldAlpha)
		{
			bar_left->alpha = abar_left;
			bar_right->alpha = abar_right;
			barFade_left->alpha = abarFade_left;
			barFade_right->alpha = abarFade_right;
		}
		else
		{
			bar_right->alpha = 0;
			bar_left->alpha = 0;
			barFade_right->alpha = 0;
			barFade_left->alpha = 0;
		}
	}

	// top and bottom bars are not toggle-able, they will always be on if they are needed
	if (getVirtualHeight() > 600)
	{
		int sz2 = (core->getVirtualHeight() - baseVirtualHeight)/2.0f;
		bar_up = new Quad;
		{
			bar_up->position = Vector(400, -sz2);
			bar_up->setWidthHeight(800, sz2*2);
			bar_up->color = 0;
			bar_up->followCamera = 1;
			bar_up->cull = 0;
		}
		addRenderObject(bar_up, LR_BLACKBARS);

		bar_down = new Quad;
		{
			bar_down->position = Vector(400, 600 + sz2);
			bar_down->setWidthHeight(800, sz2*2);
			bar_down->color = 0;
			bar_down->followCamera = 1;
			bar_down->cull = 0;
		}
		addRenderObject(bar_down, LR_BLACKBARS);
	}
}

void DSQ::setBlackBarsColor(Vector color)
{
	if (bar_left && bar_right)
	{
		bar_left->color = bar_right->color = barFade_left->color = barFade_right->color = color;
	}
}

void DSQ::toggleBlackBars(bool on, float t)
{
	if (bar_left && bar_right)
	{
		if (on)
		{
			bar_left->alpha = bar_right->alpha = 1;
			barFade_right->alpha = barFade_left->alpha = 1;
		}
		else
		{
			if (t != 0)
			{
				bar_left->alpha.interpolateTo(0, t);
				bar_right->alpha.interpolateTo(0, t);
				barFade_left->alpha.interpolateTo(0, t);
				barFade_right->alpha.interpolateTo(0, t);
			}
			else
			{
				bar_left->alpha = bar_right->alpha = 0;
				barFade_right->alpha = barFade_left->alpha = 0;
			}
		}
	}
}

int DSQ::getEntityLayerToLayer(int lcode)
{
	if (lcode == -4)
		return LR_ENTITIES_MINUS4; // in front of elements11
	else if (lcode == -3)
		return LR_ENTITIES_MINUS3; // in front of elements2
	else if (lcode == -2)
		return LR_ENTITIES_MINUS2; // in front of elements3
	else if (lcode == -1)
		return LR_ENTITIES0;
	else if (lcode == -100)
		return LR_ENTITIES00;
	else if (lcode == 0)
		return LR_ENTITIES;
	else if (lcode == 1)
		return LR_ENTITIES2;
	else if (lcode == 2)
		return LR_DARK_LAYER;
	std::ostringstream os;
	os << "Invalid entity layer code: " << lcode;
	debugLog(os.str());
	return 0;
}

void DSQ::setStory()
{
	std::string flagString = getUserInputString(stringbank.get(2014), "0");
	int flag = 0;
	std::istringstream is(flagString);
	is >> flag;

	core->run(0.2f);
	std::ostringstream os;
	os << dsq->continuity.getFlag(flag);
	flagString = getUserInputString(stringbank.get(2015), os.str());
	int value = 0;
	std::istringstream is2(flagString);
	is2 >> value;
	dsq->continuity.setFlag(flag, value);
}

Vector DSQ::getNoteVector(int note, float mag)
{
	Vector vec;
	switch(note)
	{
	case 0:
		vec = Vector(0,1);
	break;
	case 1:
		vec = Vector(0.5, 0.5);
	break;
	case 2:
		vec = Vector(1, 0);
	break;
	case 3:
		vec = Vector(0.5, -0.5);
	break;
	case 4:
		vec = Vector(0, -1);
	break;
	case 5:
		vec = Vector(-0.5, -0.5);
	break;
	case 6:
		vec = Vector(-1, 0);
	break;
	case 7:
		vec = Vector(-0.5, 0.5);
	break;
	}
	return vec*mag;
}

int DSQ::getRandNote()
{
	static int lastRand = -1;

	int r = rand()%8;

	int c = 0;
	while (r == lastRand)
	{
		r = rand()%8;
		c++;
		if (c > 8) break;
	}
	lastRand = r;


	return r;
}

Vector DSQ::getNoteColor(int note)
{
	Vector noteColor;
	switch(note)
	{
	case 0:
		// light green
		noteColor = Vector(0.5f, 1, 0.5f);
	break;
	case 1:
		// blue/green
		noteColor = Vector(0.5f, 1, 0.75f);
	break;
	case 2:
		// blue
		noteColor = Vector(0.5f, 0.5f, 1);
	break;
	case 3:
		// purple
		noteColor = Vector(1, 0.5f, 1);
	break;
	case 4:
		// red
		noteColor = Vector(1, 0.5f, 0.5f);
	break;
	case 5:
		// red/orange
		noteColor = Vector(1, 0.6f, 0.5f);
	break;
	case 6:
		// orange
		noteColor = Vector(1, 0.75f, 0.5f);
	break;
	case 7:
		// orange
		noteColor = Vector(1, 1, 0.5f);
	break;
	}
	return noteColor;
}

void DSQ::toggleVersionLabel(bool on)
{
	float a = 0;
	if (on)
		a = 1;

	versionLabel->alpha.interpolateTo(a, 1);
}

void DSQ::setInputMode(InputDevice mode)
{
	lastInputMode = mode;
	switch(mode)
	{
	case INPUT_JOYSTICK:
		core->joystickAsMouse = true;
	break;
	case INPUT_MOUSE:
		setMousePosition(core->mouse.position);
		core->joystickAsMouse = false;
	break;
	case INPUT_KEYBOARD:
	break;
	case INPUT_NODEVICE:
		break;
	}
}

void DSQ::toggleRenderCollisionShapes()
{
	if (core->getCtrlState() && core->getShiftState())
		RenderObject::renderCollisionShape = !RenderObject::renderCollisionShape;
}

void DSQ::unloadDevice()
{
	destroyFonts();

	Core::unloadDevice();
	darkLayer.unloadDevice();
}

void DSQ::reloadDevice()
{
	Core::reloadDevice();
	darkLayer.reloadDevice();

	recreateBlackBars();
}

void DSQ::toggleConsole()
{
	if (console && isDeveloperKeys())
	{
		if (console->alpha == 0)
		{
			console->alpha.interpolateTo(1, 0.1f);
			cmDebug->alpha.interpolateTo(1, 0.1f);
			fpsText->alpha.interpolateTo(1, 0.1f);
			RenderObject::renderPaths = true;
		}
		else if (console->alpha == 1)
		{
			console->alpha.interpolateTo(0, 0.1f);
			cmDebug->alpha.interpolateTo(0, 0.1f);
			fpsText->alpha.interpolateTo(0, 0.1f);
			RenderObject::renderPaths = false;
		}
	}
}

void DSQ::debugLog(const std::string &s)
{
	consoleLines.push_back(s);
	if (consoleLines.size() > MAX_CONSOLELINES)
	{

		for (size_t i = 0; i < consoleLines.size()-1; i++)
		{
			consoleLines[i] = consoleLines[i+1];
		}
		consoleLines.resize(MAX_CONSOLELINES);
	}
	if (console)
	{
		std::string text;
		for (size_t i = 0; i < consoleLines.size(); i++)
		{
			text += consoleLines[i] + '\n';
		}
		console->setText(text);
	}
	Core::debugLog(s);
}

int DSQ::getEntityTypeIndexByName(std::string s)
{
	for (size_t i = 0; i < game->entityTypeList.size(); i++)
	{
		EntityClass *t = &game->entityTypeList[i];
		if (t->name == s)
			return t->idx;
	}
	return -1;
}

void DSQ::loadModsCallback(const std::string &filename, void *param)
{

	int pos = filename.find_last_of('/')+1;
	int pos2 = filename.find_last_of('.');
	std::string name = filename.substr(pos, pos2-pos);
	ModEntry m;
	m.path = name;
	m.id = dsq->modEntries.size();

	XMLDocument d;
	if(!Mod::loadModXML(&d, name))
	{
		const char *err = d.GetErrorStr1();
		if(!err)
			err = "<unknown error>";
		std::ostringstream os;
		os << "Failed to load mod xml: " << filename << " -- Error: " << err;
		dsq->debugLog(os.str());
		return;
	}

	m.type = Mod::getTypeFromXML(d.FirstChildElement("AquariaMod"));

	dsq->modEntries.push_back(m);

	std::ostringstream ss;
	ss << "Loaded ModEntry [" << m.path << "] -> " << m.id << "  | type " << m.type;

	dsq->debugLog(ss.str());
}

void DSQ::loadModPackagesCallback(const std::string &filename, void *param)
{
	bool ok = dsq->mountModPackage(filename);

	std::ostringstream ss;
	ss << "Mount Mod Package '" << filename << "' : " << (ok ? "ok" : "FAIL");
	dsq->debugLog(ss.str());

	// they will be enumerated by the following loadModsCallback round
}

void DSQ::startSelectedMod()
{
	ModEntry *e = getSelectedModEntry();
	if (e)
	{
		clearModSelector();
		mod.load(e->path);
		mod.start();


	}
}

ModEntry* DSQ::getSelectedModEntry()
{
	if (!modEntries.empty() && selectedMod < modEntries.size())
		return &modEntries[selectedMod];
	return 0;
}

void DSQ::loadMods()
{
	modEntries.clear();

	std::string modpath = mod.getBaseModPath();
	debugLog("loadMods: " + modpath);

#ifdef BBGE_BUILD_VFS
	// first load the packages, then enumerate XMLs
	forEachFile(modpath, ".aqmod", loadModPackagesCallback);
#endif

	forEachFile(modpath, ".xml", loadModsCallback);
	selectedMod = 0;

	std::ostringstream os;
	os << "loadMods done, " << modEntries.size() << " entries";
	debugLog(os.str());
}

void DSQ::applyPatches()
{
#ifndef AQUARIA_DEMO
#ifdef BBGE_BUILD_VFS

	// This is to allow files in patches to override files in mods on non-win32 systems
	vfs.Mount(mod.getBaseModPath().c_str(), "_mods");

	loadMods();

	for (std::vector<std::string>::iterator it = activePatches.begin(); it != activePatches.end(); ++it)
		for(size_t i = 0; i < modEntries.size(); ++i)
			if(modEntries[i].type == MODTYPE_PATCH)
				if(!nocasecmp(modEntries[i].path.c_str(), it->c_str()))
					_applyPatch(modEntries[i].path);
#endif
#endif
}


#ifdef BBGE_BUILD_VFS

static void refr_pushback(ttvfs::DirBase *vd, void *user)
{
	std::list<std::string> *li = (std::list<std::string>*)user;
	li->push_back(vd->fullname());
}

static void refr_insert(VFILE *vf, void *user)
{
	// texture names are like: "naija/naija2-frontleg3" - no .png extension, and no gfx/ path
	std::vector<std::string>*files = (std::vector<std::string>*)user;
	std::string t = vf->fullname();
	size_t dotpos = t.rfind('.');
	size_t pathstart = t.find("gfx/");
	if(dotpos == std::string::npos || pathstart == std::string::npos || dotpos < pathstart)
		return; // whoops

	files->push_back(t.substr(pathstart + 4, dotpos - (pathstart + 4)));
}


// this thing is rather heuristic... but works for normal mod paths
// there is apparently nothing else except Textures that is a subclass of Resource,
// thus directly using "gfx" subdir should be fine...
void DSQ::refreshResourcesForPatch(const std::string& name)
{
	std::list<std::string> left;
	std::vector<std::string> files;
	left.push_back(mod.getBaseModPath() + name + "/gfx");
	ttvfs::DirView view;
	do
	{
		std::string dirname = left.front();
		left.pop_front();
		if(vfs.FillDirView(dirname.c_str(), view))
		{
			view.forEachDir(refr_pushback, &left);
			view.forEachFile(refr_insert, &files);
		}
	}
	while(left.size());

	std::sort(files.begin(), files.end());
	std::vector<std::string>::iterator newend = unique(files.begin(), files.end());
	files.erase(newend, files.end());

	std::ostringstream os;
	os << "refreshResourcesForPatch - " << files.size() << " to refresh";
	debugLog(os.str());

	int reloaded = 0;
	if(files.size())
	{
		for(size_t i = 0; i < dsq->resources.size(); ++i)
		{
			Texture *r = dsq->resources[i];
			for(size_t i = 0; i < files.size(); ++i)
				if(files[i] == r->name)
				{
					++reloaded;
					r->reload();
					break;
				}
		}
	}
	os.str("");
	os << "refreshResourcesForPatch - " << reloaded << " textures reloaded";
	debugLog(os.str());
}
#else
void DSQ::refreshResourcesForPatch(const std::string& name) {}
#endif

void DSQ::activatePatch(const std::string& name)
{
	_applyPatch(name);
	activePatches.push_back(name);
}

void DSQ::_applyPatch(const std::string& name)
{
#ifdef BBGE_BUILD_VFS
#ifdef AQUARIA_DEMO
	return;
#endif

	std::string src = mod.getBaseModPath();
	src += name;
	debugLog("Apply patch: " + src);
	vfs.Mount(src.c_str(), "");
	refreshResourcesForPatch(name);
#endif
}

void DSQ::disablePatch(const std::string& name)
{
#ifdef BBGE_BUILD_VFS
	std::string src = mod.getBaseModPath();
	src += name;
	debugLog("Unapply patch: " + src);
	vfs.Unmount(src.c_str(), "");

	// preserve order
	std::vector<std::string>::iterator it = std::remove(activePatches.begin(), activePatches.end(), name);
	activePatches.erase(it, activePatches.end());

	refreshResourcesForPatch(name);
#endif
}

bool DSQ::isPatchActive(const std::string& name)
{
	for(size_t i = 0; i < activePatches.size(); ++i)
		if(activePatches[i] == name)
			return true;
	return false;
}

void DSQ::playMenuSelectSfx()
{
	core->sound->playSfx("MenuSelect");
}

void DSQ::playPositionalSfx(const std::string &name, const Vector &position, float f, float fadeOut, SoundHolder *holder)
{
	PlaySfx sfx;
	sfx.freq = f;
	sfx.name = name;
	sfx.relative = false;
	sfx.positional = true;
	sfx.x = position.x;
	sfx.y = position.y;

	void *c = sound->playSfx(sfx);

	if (fadeOut != 0)
		sound->fadeSfx(c, SFT_OUT, fadeOut);

	if (holder)
		holder->linkSound(c);
}

void DSQ::shutdown()
{
	mod.stop();

	Network::shutdown();

	scriptInterface.shutdown();
	precacher.clean();


	core->particleManager->clearParticleBank();
	Shot::clearShotBank();
	SkeletalSprite::clearCache();


	cursor->setTexturePointer(0);

	UNREFTEX(texCursor);
	UNREFTEX(texCursorSwim);
	UNREFTEX(texCursorBurst);
	UNREFTEX(texCursorSing);
	UNREFTEX(texCursorLook);

	removeRenderObject(console);
	console = 0;
	removeRenderObject(cmDebug);
	cmDebug = 0;
	removeRenderObject(subtext);
	subtext = 0;
	removeRenderObject(subbox);
	subbox = 0;

	removeRenderObject(achievement_text);
	achievement_text = 0;
	removeRenderObject(achievement_box);
	achievement_box = 0;

	removeRenderObject(cursor);
	removeRenderObject(cursorGlow); // is this necessary? probably
	removeRenderObject(cursorBlinker);
	removeRenderObject(overlay);
	removeRenderObject(overlay2);
	removeRenderObject(overlay3);
	removeRenderObject(overlayRed);
	removeRenderObject(tfader);

	removeRenderObject(fpsText);

	if (bar_left)
		removeRenderObject(bar_left);
	if (bar_right)
		removeRenderObject(bar_right);
	if (barFade_left)
		removeRenderObject(barFade_left);
	if (barFade_right)
		removeRenderObject(barFade_right);
	if (bar_up)
		removeRenderObject(bar_up);
	if (bar_down)
		removeRenderObject(bar_down);

	if (cutscene_bg)
		removeRenderObject(cutscene_bg);
	if (cutscene_text)
		removeRenderObject(cutscene_text);
	if (cutscene_text2)
		removeRenderObject(cutscene_text2);


	removeRenderObject(versionLabel);
	versionLabel = 0;

	if (screenTransition)
	{
		screenTransition->destroy();
		removeRenderObject(screenTransition);
	}

	destroyFonts();


	screenTransition = 0;

	core->run(0.1f);
	overlay = 0;
	overlay2 = 0;
	overlay3 = 0;
	cursor = 0;
	tfader = 0;

	continuity.shutdown();


	Core::shutdown();
}

void DSQ::setTexturePointers()
{
	texCursor = core->addTexture("cursor");
	texCursorLook = core->addTexture("cursor-look");
	texCursorBurst = core->addTexture("cursor-burst");
	texCursorSwim = core->addTexture("cursor-swim");
	texCursorSing = core->addTexture("cursor-sing");

	if (cursor)
		cursor->setTexturePointer(texCursor);
}

void DSQ::setCursor(CursorType type)
{
	switch(type)
	{
	case CURSOR_NORMAL:
		cursor->setTexturePointer(texCursor);
	break;
	case CURSOR_LOOK:
		cursor->setTexturePointer(texCursorLook);
	break;
	case CURSOR_BURST:
		cursor->setTexturePointer(texCursorBurst);
	break;
	case CURSOR_SWIM:
		cursor->setTexturePointer(texCursorSwim);
	break;
	case CURSOR_SING:
		cursor->setTexturePointer(texCursorSing);
	break;
	default:
		cursor->setTexturePointer(texCursor);
	break;
	}
}

void DSQ::toggleEffects()
{


}

void DSQ::clickRingEffect(Vector pos, int type, Vector color, float ut)
{
	switch(type)
	{
	case 0:
		{
			float t = 0.5;
			if (ut != 0) t = ut;
			Quad *q = new Quad;
			q->setTexture("Particles/SoftRing");
			q->setWidthHeight(100, 100);
			q->scale = Vector(1,1);
			q->scale.interpolateTo(Vector(5,5), t);

			q->color = color;

			q->setBlendType(RenderObject::BLEND_ADD);

			q->alpha.ensureData();
			q->alpha.data->path.addPathNode(0, 0);
			q->alpha.data->path.addPathNode(0.5f, 0.1f);
			q->alpha.data->path.addPathNode(0.5f, 0.5f);
			q->alpha.data->path.addPathNode(0, 1);
			q->alpha.startPath(t);

			q->position = pos;

			q->followCamera = 1;

			q->setLife(t);

			getTopStateData()->addRenderObject(q, LR_WORLDMAPHUD);
		}
		break;
	case 1:
		{
			float t = 0.2f;
			if (ut != 0) t = ut;
			Quad *q = new Quad;
			q->setTexture("Particles/SoftRing");
			q->setWidthHeight(100, 100);
			q->scale = Vector(5,5);
			q->scale.interpolateTo(Vector(1,1), t);

			q->setBlendType(RenderObject::BLEND_ADD);

			q->color = color;

			q->alpha.ensureData();
			q->alpha.data->path.addPathNode(0, 0);
			q->alpha.data->path.addPathNode(0.5f, 0.1f);
			q->alpha.data->path.addPathNode(0.5f, 0.5f);
			q->alpha.data->path.addPathNode(0, 1);
			q->alpha.startPath(t);

			q->position = pos;

			q->followCamera = 1;

			q->setLife(t);

			getTopStateData()->addRenderObject(q, LR_WORLDMAPHUD);
		}
		break;
	}
}

Entity *DSQ::getEntityByName(const std::string& name)
{
	Entity *e = 0;
	FOR_ENTITIES(i)
	{
		e = (*i);

		if (e->life == 1 && (nocasecmp(e->name, name)==0))
		{
			break;
		}
		e = 0;
	}
	return e;
}

// HACK: warning SLOW!
// really?
Entity *DSQ::getEntityByNameNoCase(std::string name)
{
	stringToUpper(name);
	Entity *e = 0;
	FOR_ENTITIES(i)
	{
		e = (*i);

		std::string check =  e->name;
		stringToUpper(check);
		if (e->life == 1 && check == name)
		{
			break;
		}
		e = 0;
	}
	return e;
}

void DSQ::doLoadMenu()
{
	doSaveSlotMenu(SSM_LOAD);
	if (selectedSaveSlot != 0)
	{

		dsq->doScreenTrans = true;
	}
	else
	{

		clearSaveSlots(true);
	}
}

void DSQ::doSavePoint(const Vector &position)
{


	dsq->game->avatar->setv(EV_LOOKAT, 0);
	core->sound->playSfx("MemoryCrystalActivate");

	Quad *glow = new Quad;
	{
		glow->setTexture("save-point-glow");
		glow->alpha = 0;
		glow->alpha.interpolateTo(0.5f, 1, 1, true, true);
		glow->setBlendType(RenderObject::BLEND_ADD);
		glow->position = position;
		glow->scale = Vector(1,1)*1.25f;
		glow->setLife(3);
		glow->setDecayRate(1);
	}
	addRenderObject(glow, LR_LIGHTING);

	dsq->game->avatar->idle();
	dsq->game->avatar->vel=0;
	dsq->game->avatar->disableInput();
	dsq->game->avatar->fhTo(false);
	dsq->game->avatar->position.interpolateTo(position, 1, 0, 0, 1);
	dsq->game->avatar->myZoom.interpolateTo(Vector(1,1),0.5f);

	dsq->game->avatar->skeletalSprite.animate("save", 0, 3);
	dsq->game->clearControlHint();
	dsq->run(2);
	dsq->game->avatar->enableInput();
	dsq->game->avatar->revive();
	dsq->game->togglePause(1);
	dsq->doSaveSlotMenu(SSM_SAVE, position);
	dsq->game->togglePause(0);
	core->resetTimer();
	dsq->game->avatar->setv(EV_LOOKAT, 1);


}

void DSQ::playNoEffect()
{
	if (noEffectTimer <= 0)
	{
		sound->playSfx("noeffect", 0.9f);
		noEffectTimer = 0.2f;
	}
}

void DSQ::clearMenu(float t)
{
	for (size_t i = 0; i < menu.size(); i++)
	{
		menu[i]->setLife(1);
		menu[i]->setDecayRate(1/t);
		menu[i]->fadeAlphaWithLife = 1;

	}
	menu.clear();
}

void DSQ::screenMessage(const std::string &msg)
{
	debugLog(msg);

	DebugFont *b = new DebugFont();
	b->position = Vector(16,300);
	b->setFontSize(10);
	b->setText(msg);
	b->alpha = 0;
	b->alpha.interpolateTo(1, 0.75, 1, 1);
	b->followCamera= 1;
	b->setLife(2);
	b->setDecayRate(1);
	core->getTopStateData()->addRenderObject(b, LR_DEBUG_TEXT);


}

void DSQ::onExitSaveSlotMenu()
{
	onPickedSaveSlot(0);
}

bool DSQ::onPickedSaveSlot(AquariaSaveSlot *slot)
{
	if (slot == 0)
	{
		selectedSaveSlot = 0;
		quitNestedMain();
		return true;
	}
	else if ((saveSlotMode == SSM_LOAD && !slot->isEmpty()) || saveSlotMode == SSM_SAVE)
	{
		bool doit = false;

		if (saveSlotMode == SSM_SAVE && !slot->isEmpty())
		{
			selectedSaveSlot = 0;
			if (confirm("", "save"))
				doit = true;
		}
		else
		{
			doit = true;
		}

		if (doit)
		{
			selectedSaveSlot = slot;
			quitNestedMain();
			return true;
		}
		else
		{
			slot->setFocus(true);
		}
	}

	for (size_t i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i]->mbDown = false;
	}

	return false;
}

void DSQ::nag(NagType type)
{
	nagType = type;
	core->enqueueJumpState("nag");
}

void DSQ::doModSelect()
{
	modIsSelected = false;

	dsq->loadMods();

	createModSelector();

	resetTimer();

	inModSelector = true;

	run(-1);

	clearModSelector();

	if (modIsSelected)
	{
#ifdef AQUARIA_DEMO
		nag(NAG_TOTITLE);
#else
		dsq->startSelectedMod();
#endif
	}

	inModSelector = false;

	modIsSelected = false;

	resetTimer();
}

void DSQ::createModSelector()
{
	blackout = new Quad;
	blackout->color = 0;
	blackout->autoWidth = AUTO_VIRTUALWIDTH;
	blackout->autoHeight = AUTO_VIRTUALHEIGHT;
	blackout->followCamera = 1;
	blackout->position = Vector(400,300);
	blackout->alphaMod = 0.75f;
	blackout->alpha = 0;
	blackout->alpha.interpolateTo(1, 0.2f);
	addRenderObject(blackout, LR_MENU);

	modSelectorScr = new ModSelectorScreen();
	modSelectorScr->position = Vector(400,300);
	modSelectorScr->setWidth(getVirtualWidth()); // just to be sure
	modSelectorScr->setHeight(getVirtualHeight());
	modSelectorScr->autoWidth = AUTO_VIRTUALWIDTH;
	modSelectorScr->autoHeight = AUTO_VIRTUALHEIGHT;
	modSelectorScr->init();
	addRenderObject(modSelectorScr, LR_MENU);
}

bool DSQ::modIsKnown(const std::string& name)
{
	std::string nlower = name;
	stringToLower(nlower);

	for(size_t i = 0; i < modEntries.size(); ++i)
	{
		std::string elower = modEntries[i].path;
		stringToLower(elower);
		if(nlower == elower)
			return true;
	}
	return false;
}

bool DSQ::mountModPackage(const std::string& pkg)
{
#ifdef BBGE_BUILD_VFS
	ttvfs::DirBase *vd = vfs.GetDir(pkg.c_str());
	if (!vd)
	{
		// Load archive only if no such directory exists already (prevent loading multiple times)
		vd = vfs.AddArchive(pkg.c_str());
		if(!vd)
		{
			debugLog("Package: Unable to load " + pkg);
			return false;
		}
	}
	vfs.Mount(pkg.c_str(), mod.getBaseModPath().c_str());
	debugLog("Package: Mounted " + pkg + " as archive in _mods");
	return true;
#else
	debugLog("Package: Can't mount " + pkg + ", VFS support disabled");
	return false;
#endif
}

#ifdef BBGE_BUILD_VFS
static void _CloseSubdirCallback(ttvfs::DirBase *vd, void*)
{
	vd->close();
}
#endif

// This just closes some file handles, nothing fancy
void DSQ::unloadMods()
{
#ifdef BBGE_BUILD_VFS
	ttvfs::DirView view;
	if(vfs.FillDirView(mod.getBaseModPath().c_str(), view))
		view.forEachDir(_CloseSubdirCallback);
#endif
}

void DSQ::applyParallaxUserSettings()
{
	dsq->getRenderObjectLayer(LR_ELEMENTS10)->visible = dsq->user.video.parallaxOn0;
	dsq->getRenderObjectLayer(LR_ELEMENTS11)->visible = dsq->user.video.parallaxOn1;
	dsq->getRenderObjectLayer(LR_ELEMENTS12)->visible = dsq->user.video.parallaxOn2;
}

void DSQ::clearModSelector()
{
	if (blackout)
	{
		blackout->setLife(1);
		blackout->setDecayRate(2);
		blackout->fadeAlphaWithLife = 1;
		blackout = 0;
	}

	if(modSelectorScr)
	{
		modSelectorScr->close();
		modSelectorScr->setLife(1);
		modSelectorScr->setDecayRate(2);
		modSelectorScr->fadeAlphaWithLife = 1;
		modSelectorScr = 0;
	}

	// This just closes some file handles, nothing fancy
	unloadMods();

	clearMenu();
}



void DSQ::updateSaveSlotPageCount()
{
	std::ostringstream os;
	os << stringbank.get(2006) << " " << user.data.savePage+1 << "/" << maxPages+1;
	saveSlotPageCount->setText(os.str());
}


void DSQ::createSaveSlots(SaveSlotMode ssm)
{
	if (!saveSlots.empty())
	{
		errorLog("save slots weren't cleared");
	}
	if (!menu.empty())
	{
		errorLog ("menu wasn't cleared");
	}
	menu.resize(5);

	float t = 0.3f;


	blackout = new Quad;
	blackout->color = 0;
	blackout->autoWidth = AUTO_VIRTUALWIDTH;
	blackout->autoHeight = AUTO_VIRTUALHEIGHT;
	blackout->followCamera = 1;
	blackout->position = Vector(400,300);
	blackout->alphaMod = 0.75f;
	blackout->alpha = 0;
	blackout->alpha.interpolateTo(1, 0.5f);
	addRenderObject(blackout, LR_MENU);


	menu[1] = new Quad("gui/save-menu", Vector(400,300));
	menu[1]->alpha = 0;
	menu[1]->alpha.interpolateTo(1, t);
	menu[1]->scale = savesz * 0.5f;
	menu[1]->scale.interpolateTo(savesz, t);
	menu[1]->followCamera = 1;
	addRenderObject(menu[1], LR_MENU);

	core->sound->playSfx("menu-open");

	watch(t);

	saveSlotPageCount = new BitmapText(&dsq->smallFont);
	saveSlotPageCount->followCamera = 1;
	saveSlotPageCount->setAlign(ALIGN_LEFT);
	saveSlotPageCount->position = Vector(590, 300);
	addRenderObject(saveSlotPageCount, LR_MENU);
	updateSaveSlotPageCount();


	cancel = new AquariaMenuItem();

	cancel->useGlow("glow", 200, 50);
	cancel->event.set(MakeFunctionEvent(DSQ,onExitSaveSlotMenu));
	cancel->position = Vector(665, 545);
	addRenderObject(cancel, LR_MENU);

	menu[0] = cancel;



	arrowUp = new AquariaMenuItem();
	arrowUp->useQuad("gui/arrow-left");
	arrowUp->useGlow("glow", 100, 50);
	arrowUp->useSound("click");
	arrowUp->event.set(MakeFunctionEvent(DSQ, prevSaveSlotPage));
	arrowUp->rotation.z = 90;
	arrowUp->scale = Vector(0.75, 0.75);
	arrowUp->position = Vector(620, 200);
	addRenderObject(arrowUp, LR_MENU);

	menu[2] = arrowUp;

	arrowDown = new AquariaMenuItem();
	arrowDown->useQuad("gui/arrow-right");
	arrowDown->useSound("click");
	arrowDown->useGlow("glow", 100, 50);
	arrowDown->scale = Vector(0.75, 0.75);
	arrowDown->event.set(MakeFunctionEvent(DSQ, nextSaveSlotPage));
	arrowDown->rotation.z = 90;
	arrowDown->position = Vector(620, 400);
	addRenderObject(arrowDown, LR_MENU);

	if (dsq->game->miniMapRender)
		dsq->game->miniMapRender->slide(1);

	menu[3] = arrowDown;

	BitmapText *txt = new BitmapText(&dsq->font);
	if (ssm == SSM_LOAD)
		txt->setText(stringbank.get(2001));
	else
		txt->setText(stringbank.get(2000));
	txt->position = Vector(230, 68);
	txt->followCamera = 1;
	addRenderObject(txt, LR_MENU);

	menu[4] = txt;

	createSaveSlotPage();

	saveSlots[0]->setFocus(true);
}

void DSQ::title(bool fade)
{
	core->settings.runInBackground = false;
	recentSaveSlot = -1;

	dsq->overlay->color = 0;
	dsq->overlay->alpha.interpolateTo(1, 1);

	if (fade)
	{
		dsq->sound->fadeMusic(SFT_OUT, 1);
	}

	run(1);

	resetTimer();

	if (fade)
		dsq->sound->stopMusic();

	user.save();

	if (mod.isActive())
	{
		mod.shutdown();
	}

	// Will be re-loaded on demand
	unloadMods();

	// VERY important
	dsq->continuity.reset();

	dsq->game->transitionToScene("Title");
}

void DSQ::createSaveSlotPage()
{
	for (size_t i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i]->safeKill();
	}

	saveSlots.resize(saveSlotPageSize);
	for (size_t i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i] = new AquariaSaveSlot(i + user.data.savePage *  saveSlotPageSize);
		saveSlots[i]->followCamera = 1;
		saveSlots[i]->position = Vector(409,193+i*90);
		if (i != 1)
			saveSlots[i]->position.y ++;

		if (i == 1 || i == 3)
			saveSlots[i]->position.y -= 0.5f;

		addRenderObject(saveSlots[i], LR_FILEMENU);
	}

	saveSlots[0]->setDirMove(DIR_RIGHT, arrowUp);
	saveSlots[1]->setDirMove(DIR_RIGHT, arrowUp);
	saveSlots[2]->setDirMove(DIR_RIGHT, arrowDown);
	saveSlots[3]->setDirMove(DIR_RIGHT, cancel);
	arrowDown->setDirMove(DIR_DOWN, cancel);
	cancel->setDirMove(DIR_UP, arrowDown);
	cancel->setDirMove(DIR_LEFT, saveSlots[3]);
}

void DSQ::nextSaveSlotPage()
{
	if (saveSlots.empty()) return;

	user.data.savePage++;
	if (user.data.savePage > maxPages)
		user.data.savePage = 0;
	createSaveSlotPage();

	updateSaveSlotPageCount();
}

void DSQ::prevSaveSlotPage()
{
	if (saveSlots.empty()) return;

	user.data.savePage--;
	if (user.data.savePage > maxPages)
		user.data.savePage = maxPages;
	createSaveSlotPage();

	updateSaveSlotPageCount();
}

void DSQ::hideSaveSlotCrap()
{
	clearMenu();

	if (blackout)
		blackout->alpha = 0;

	if (saveSlotPageCount)
		saveSlotPageCount->alpha = 0;
}

void DSQ::clearSaveSlots(bool trans)
{
	if (trans)
	{
		core->sound->playSfx("menu-close");
	}
	float t = 0.3f;
	if (blackout)
	{
		if (!trans)
		{
			blackout->setLife(1);
			blackout->setDecayRate(10);
			if (blackout->alpha.x > 0)
				blackout->fadeAlphaWithLife = 1;
		}
		else
		{
			blackout->setLife(1);
			blackout->setDecayRate(1);
			if (blackout->alpha.x > 0)
				blackout->fadeAlphaWithLife = 1;
		}
	}
	if (saveSlotPageCount)
	{
		saveSlotPageCount->setLife(1);
		saveSlotPageCount->setDecayRate(10);
		if (saveSlotPageCount->alpha.x > 0)
			saveSlotPageCount->fadeAlphaWithLife = 1;
	}

	for (size_t i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i]->close(trans);
	}

	saveSlots.clear();



	if (trans)
	{
		disableMiniMapOnNoInput = false;

		for (size_t i = 0; i < menu.size(); i++)
		{
			if (i != 1)
			{
				menu[i]->alpha = 0;

			}
		}
		if (menu.size() >= 2)
		{
			menu[1]->scale.interpolateTo(savesz*0.5f, t);
			menu[1]->alpha.interpolateTo(0, t);
			watch(t);
		}

		disableMiniMapOnNoInput = true;
	}
	clearMenu();


	if (dsq->game->miniMapRender)
		dsq->game->miniMapRender->slide(0);
}

void DSQ::hideSaveSlots()
{
	for (size_t i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i]->hide();
	}
}

void DSQ::transitionSaveSlots()
{
	hideSaveSlotCrap();

	for (size_t i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i]->transition();
	}
}

void DSQ::doSaveSlotMenu(SaveSlotMode ssm, const Vector &position)
{
	int scrShotWidth = 0, scrShotHeight = 0;
	unsigned char *scrShotData = 0;

	if (ssm == SSM_SAVE && user.video.saveSlotScreens)
	{
		prepScreen(1);

		int renderWidth = getWindowWidth(), renderHeight = getWindowHeight();
		int i = 2;
		while (1 << i < renderHeight)
		{
			i++;
		}
		scrShotWidth = scrShotHeight = 1 << (i-1);
		int x = renderWidth/2  - scrShotWidth/2;
		int y = renderHeight/2 - scrShotHeight/2;

		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0, 0, renderWidth, renderHeight);
		clearBuffers();
		render();
		scrShotData = core->grabScreenshot(x, y, scrShotWidth, scrShotHeight);
		glPopAttrib();
		showBuffer();

		prepScreen(0);
	}

	saveSlotMode = SSM_NONE;

	createSaveSlots(ssm);
	const size_t firstSaveSlot = user.data.savePage * saveSlotPageSize;
	if (user.data.saveSlot >= firstSaveSlot && user.data.saveSlot < firstSaveSlot + saveSlots.size())
	{
		selectedSaveSlot = saveSlots[user.data.saveSlot - firstSaveSlot];
		selectedSaveSlot->setFocus(true);
	}
	else
	{
		selectedSaveSlot = 0;
	}

	saveSlotMode = ssm;



	resetTimer();
	core->run(-1);



	if (selectedSaveSlot == 0)
	{
		if (saveSlotMode == SSM_SAVE)
		{
			clearSaveSlots(true);
		}
	}
	else
	{
		// Drop focus early so it doesn't see joystick movement and
		// try to select a destroyed save slot (and thus crash).
		selectedSaveSlot->setFocus(false);
		recentSaveSlot = selectedSaveSlot->getSlotIndex();
		user.data.saveSlot = recentSaveSlot;
		if (saveSlotMode == SSM_SAVE)
		{
			continuity.saveFile(selectedSaveSlot->getSlotIndex(), position, scrShotData, scrShotWidth, scrShotHeight);
			if (user.video.saveSlotScreens && scrShotData != 0)
			{
				std::ostringstream os;
				os << dsq->getSaveDirectory() << "/screen-" << numToZeroString(selectedSaveSlot->getSlotIndex(), 4) << ".zga";

				// Cut off top and bottom to get a 4:3 aspect ratio.
				/*int adjHeight = (scrShotWidth * 3.0f) / 4.0f;
				int imageDataSize = scrShotWidth * scrShotHeight * 4;
				int adjImageSize = scrShotWidth * adjHeight * 4;
				int adjOffset = scrShotWidth * ((scrShotHeight-adjHeight)/2) * 4;
				memmove(scrShotData, scrShotData + adjOffset, adjImageSize);
				memset(scrShotData + adjImageSize, 0, imageDataSize - adjImageSize);*/
				zgaSaveRGBA(os.str().c_str(), scrShotWidth, scrShotHeight, scrShotData);
			}

			PlaySfx sfx;
			sfx.name = "saved";
			sfx.vol = 0.55f;
			dsq->sound->playSfx(sfx);
			confirm("", "saved", 1);

			clearSaveSlots(true);
		}
		else if (saveSlotMode == SSM_LOAD)
		{
			if(continuity.loadFile(selectedSaveSlot->getSlotIndex()))
				dsq->game->transitionToScene(dsq->game->sceneToLoad);
		}
		// when gameover hits, load up this instead of that.
	}

	resetTimer();

	delete[] scrShotData;
	saveSlotMode = SSM_NONE;

}

std::string DSQ::getEntityFlagName(Entity *e)
{
	if (!dsq->game) return "";
	std::ostringstream os;
	os << dsq->game->sceneName << e->startPos.x << e->startPos.y;
	return os.str();
}

void doAlphabetInputKey(int d, char c, char map[], std::string *text, char upper=0)
{
	if (core->getKeyState(d) && !map[d])
	{
		char usec = c;
		if (upper != 0 && (core->getKeyState(KEY_LSHIFT) || core->getKeyState(KEY_RSHIFT)))
		{
			usec = upper;
		}
		*text += usec;
		map[d] = 1;
	}
	else if (!core->getKeyState(d) && map[d])
	{
		map[d] = 0;
	}
}

void DSQ::generateCollisionMask(RenderObject *r)
{
	Quad *q = dynamic_cast<Quad*>(r);
	if (q)
		game->generateCollisionMask(q);
}

void DSQ::onConfirmYes()
{
	dsq->confirmDone = 1;
}

void DSQ::onConfirmNo()
{
	dsq->confirmDone = 2;
}

bool DSQ::confirm(const std::string &text, const std::string &image, bool ok, float countdown)
{
	const float t = 0.3f;

	sound->playSfx("menu-open");

	confirmDone = 0;

	std::string imageName = "gui/confirm-bg";
	if (!image.empty())
	{
		imageName += "-" + image;
	}

	Quad *bgLabel = new Quad(imageName, Vector(400,300));
	bgLabel->followCamera = 1;
	bgLabel->alpha = 0;
	bgLabel->alpha.interpolateTo(1, t);

	bgLabel->scale = Vector(0.5, 0.5);
	bgLabel->scale.interpolateTo(Vector(1,1), t);
	addRenderObject(bgLabel, LR_CONFIRM);

	const int GUILEVEL_CONFIRM = 200;

	AquariaGuiElement::currentGuiInputLevel = GUILEVEL_CONFIRM;

	dsq->run(t, true);

	float t2 = 0.05f;



	AquariaMenuItem *yes=0;
	AquariaMenuItem *no=0;

	if (ok)
	{
		yes = new AquariaMenuItem();
		yes->useQuad("gui/ok");
		yes->useGlow("glow", 64, 50);
		yes->event.set(MakeFunctionEvent(DSQ,onConfirmYes));

		yes->position = Vector(400, 340);
		addRenderObject(yes, LR_CONFIRM);

		yes->setFocus(true);

		yes->setCanDirMove(false);

		yes->guiInputLevel = GUILEVEL_CONFIRM;
	}
	else
	{
		yes = new AquariaMenuItem();
		yes->useQuad("yes");
		yes->useGlow("glow", 64, 50);
		yes->event.set(MakeFunctionEvent(DSQ,onConfirmYes));

		yes->position = Vector(330, 340);
		addRenderObject(yes, LR_CONFIRM);

		yes->guiInputLevel = GUILEVEL_CONFIRM;

		no = new AquariaMenuItem();
		no->useQuad("no");
		no->useGlow("glow", 64, 50);
		no->event.set(MakeFunctionEvent(DSQ,onConfirmNo));

		no->position = Vector(470, 340);
		addRenderObject(no, LR_CONFIRM);

		no->guiInputLevel = GUILEVEL_CONFIRM;

		no->setFocus(true);

		no->setDirMove(DIR_LEFT, yes);

		no->setDirMove(DIR_UP, no);
		no->setDirMove(DIR_DOWN, no);
		no->setDirMove(DIR_RIGHT, no);

		yes->setDirMove(DIR_RIGHT, no);

		yes->setDirMove(DIR_UP, yes);
		yes->setDirMove(DIR_DOWN, yes);
		yes->setDirMove(DIR_LEFT, yes);
	}

	BitmapText *txt = new BitmapText(&dsq->smallFont);
	txt->followCamera = 1;
	txt->position = Vector(400,250);
	txt->setText(text);
	txt->alpha = 0;
	txt->scale = Vector(0.9f, 0.9f);
	txt->alpha.interpolateTo(1, t2);
	addRenderObject(txt, LR_CONFIRM);

	dsq->run(t2, true);

	while (!confirmDone)
	{
		dsq->run(FRAME_TIME, true);
		if (countdown > 0) {
			countdown -= FRAME_TIME;
			if (countdown < 0)
				break;
		}
	}

	sound->playSfx("menu-close");

	txt->alpha.interpolateTo(0, t2);
	if (yes)	yes->alpha.interpolateTo(0, t2);
	if (no)		no->alpha.interpolateTo(0, t2);
	dsq->run(t2, true);

	bgLabel->alpha.interpolateTo(0, t);
	bgLabel->scale.interpolateTo(Vector(0.5, 0.5), t);
	dsq->run(t, true);

	bgLabel->safeKill();
	txt->safeKill();
	if (yes)
	{
		yes->setFocus(false);
		yes->safeKill();
	}
	if (no)
	{
		no->setFocus(false);
		no->safeKill();
	}

	bool ret = (confirmDone == 1);

	if (countdown < 0)
		ret = false;

	AquariaGuiElement::currentGuiInputLevel = 0;

	return ret;
}

std::string DSQ::getUserInputString(std::string labelText, std::string t, bool allowNonLowerCase)
{
	float trans = 0.1f;

	bool pauseState = dsq->game->isPaused();

	dsq->game->togglePause(true);

	sound->playSfx("Menu-Open");

	RoundedRect *bg = new RoundedRect;
	bg->setWidthHeight(790, 64, 10);
	bg->position = Vector(400,300);
	bg->followCamera = 1;
	bg->alpha = 0;
	addRenderObject(bg, LR_DEBUG_TEXT);



	TTFText *label = new TTFText(&dsq->fontArialSmall);
	label->setText(labelText);
	label->position = Vector(-400 + 20, -12);
	bg->addChild(label, PM_POINTER);

	TTFText *inputText = new TTFText(&dsq->fontArialBig);

	inputText->position = Vector(-400 + 20,8+8);
	bg->addChild(inputText, PM_POINTER);


	bg->show();
	run(trans);


	std::string text = t;
	char map[256];
	for (int i = 0; i < 256; i++)
		map[i] = 0;
	bool delDown = false;
	bool escDown = false;

	float dt = 1.0f/60.0f;
	float blinkTimer = 0;
	bool blink = false;

	while (1)
	{
		if (blink)
		{
			text.resize(text.size()-1);
			inputText->setText(text);
		}
		if (inputText->getActualWidth() < 800-60)
		{
			doAlphabetInputKey(KEY_A, 'a', (char*)&map, &text, 'A');
			doAlphabetInputKey(KEY_B, 'b', (char*)&map, &text, 'B');
			doAlphabetInputKey(KEY_C, 'c', (char*)&map, &text, 'C');
			doAlphabetInputKey(KEY_D, 'd', (char*)&map, &text, 'D');
			doAlphabetInputKey(KEY_E, 'e', (char*)&map, &text, 'E');
			doAlphabetInputKey(KEY_F, 'f', (char*)&map, &text, 'F');
			doAlphabetInputKey(KEY_G, 'g', (char*)&map, &text, 'G');
			doAlphabetInputKey(KEY_H, 'h', (char*)&map, &text, 'H');
			doAlphabetInputKey(KEY_I, 'i', (char*)&map, &text, 'I');
			doAlphabetInputKey(KEY_J, 'j', (char*)&map, &text, 'J');
			doAlphabetInputKey(KEY_K, 'k', (char*)&map, &text, 'K');
			doAlphabetInputKey(KEY_L, 'l', (char*)&map, &text, 'L');
			doAlphabetInputKey(KEY_M, 'm', (char*)&map, &text, 'M');
			doAlphabetInputKey(KEY_N, 'n', (char*)&map, &text, 'N');
			doAlphabetInputKey(KEY_O, 'o', (char*)&map, &text, 'O');
			doAlphabetInputKey(KEY_P, 'p', (char*)&map, &text, 'P');
			doAlphabetInputKey(KEY_Q, 'q', (char*)&map, &text, 'Q');
			doAlphabetInputKey(KEY_R, 'r', (char*)&map, &text, 'R');
			doAlphabetInputKey(KEY_S, 's', (char*)&map, &text, 'S');
			doAlphabetInputKey(KEY_T, 't', (char*)&map, &text, 'T');
			doAlphabetInputKey(KEY_U, 'u', (char*)&map, &text, 'U');
			doAlphabetInputKey(KEY_V, 'v', (char*)&map, &text, 'V');
			doAlphabetInputKey(KEY_W, 'w', (char*)&map, &text, 'W');
			doAlphabetInputKey(KEY_X, 'x', (char*)&map, &text, 'X');
			doAlphabetInputKey(KEY_Y, 'y', (char*)&map, &text, 'Y');
			doAlphabetInputKey(KEY_Z, 'z', (char*)&map, &text, 'Z');
			doAlphabetInputKey(KEY_1, '1', (char*)&map, &text);
			doAlphabetInputKey(KEY_2, '2', (char*)&map, &text);
			doAlphabetInputKey(KEY_3, '3', (char*)&map, &text);
			doAlphabetInputKey(KEY_4, '4', (char*)&map, &text);
			doAlphabetInputKey(KEY_5, '5', (char*)&map, &text);
			doAlphabetInputKey(KEY_6, '6', (char*)&map, &text);
			doAlphabetInputKey(KEY_7, '7', (char*)&map, &text);
			doAlphabetInputKey(KEY_8, '8', (char*)&map, &text);
			doAlphabetInputKey(KEY_9, '9', (char*)&map, &text);
			doAlphabetInputKey(KEY_0, '0', (char*)&map, &text);

			doAlphabetInputKey(KEY_PERIOD, '.', (char*)&map, &text);
			doAlphabetInputKey(KEY_SPACE, ' ', (char*)&map, &text);
			doAlphabetInputKey(KEY_MINUS, '-', (char*)&map, &text, '_');

			doAlphabetInputKey(KEY_TILDE, '~', (char*)&map, &text, '~');
		}

		if (core->getKeyState(KEY_BACKSPACE))
		{
			if (!delDown)
			{
				if (!text.empty())
				{
					text.resize(text.size()-1);
				}
			}
			delDown = true;
		}
		else
		{
			delDown = false;
		}

		blinkTimer += dt;
		if (blinkTimer > 0.2f)
		{
			blink = !blink;
			blinkTimer = 0;
		}

		if (blink)
		{
			text += "|";
		}



		if (core->getKeyState(KEY_RETURN))
			break;

		if (!escDown && core->getKeyState(KEY_ESCAPE))
			escDown = true;
		else if (escDown && !core->getKeyState(KEY_ESCAPE))
		{
			escDown = false;
			text = t;
			break;
		}
		inputText->setText(text);
		core->run(dt);
	}

	if (blink && !text.empty() && (text[text.size()-1] == '|'))
		text.resize(text.size()-1);

	sound->playSfx("Menu-Close");


	bg->hide();

	run(0.2f);

	inputText->alpha = 0;
	label->alpha = 0;
	inputText->safeKill();
	label->safeKill();
	bg->alpha = 0;
	bg->safeKill();

	dsq->game->togglePause(pauseState);

	if (!allowNonLowerCase)
		stringToLower(text);

	debugLog("getUserInputString returned: " + text);

	return text;
}

void DSQ::stopVoice()
{
	sound->stopVoice();
	subtitlePlayer.end();
}

bool DSQ::playedVoice(const std::string &file)
{
	std::string f = file;
	stringToUpper(f);
	for (size_t i = 0; i < dsq->continuity.voiceOversPlayed.size(); i++)
	{
		if (f == dsq->continuity.voiceOversPlayed[i])
		{
			return true;
		}
	}
	return false;
}

void DSQ::voiceOnce(const std::string &file)
{
	std::string f = file;
	stringToUpper(f);
	if (!playedVoice(f))
	{
		voice(file);
	}
}

// This is a pretty "dangerous" function
// it will kill the current voice over and all pending voice overs
// recommended only in situations where overriding the voice is acceptable
// i.e. song cave door (1st songdoor)
void DSQ::voiceInterupt(const std::string &f)
{
	sound->playVoice(f, SVT_INTERRUPT);

}



void DSQ::onPlayVoice()
{

}

void DSQ::onStopVoice()
{
	subtitlePlayer.end();

}

void DSQ::voice(const std::string &f, float volMod)
{
	debugLog("Voice: " + f);
	std::string file = f;
	stringToUpper(file);

	if (!playedVoice(file))
		dsq->continuity.voiceOversPlayed.push_back(file);

	sound->playVoice(file, SVT_QUEUE, volMod);
}

void DSQ::onPlayedVoice(const std::string &name)
{
	Core::onPlayedVoice(name);

	if (user.audio.subtitles)
		subtitlePlayer.go(name);
}

Entity *DSQ::getFirstEntity()
{
	iter = &entities[0];
	return getNextEntity();
}

Entity *DSQ::getNextEntity()
{
	if (*iter == 0)
		return 0;
	return *(iter++);
}

bool DSQ::runScript(const std::string &name, const std::string &function, bool ignoremissing /* = false */)
{
	if (!scriptInterface.runScript(name, function, ignoremissing))
	{
		debugLog("Could not find script file [" + name + "]");
	}
	else
	{
		return true;
	}
	return false;
}

bool DSQ::runScriptNum(const std::string &name, const std::string &func, float num)
{
	if (!scriptInterface.runScriptNum(name, func, num))
	{
		debugLog("Could not find script file [" + name + "]");
	}
	else
	{
		return true;
	}
	return false;
}

void DSQ::collectScriptGarbage()
{
	scriptInterface.collectGarbage();
}

void DSQ::onMouseInput()
{
	if (dsq->game && dsq->game->avatar)
	{
		if (!dsq->game->isInGameMenu() && !dsq->game->isSceneEditorActive() && !dsq->game->isPaused())
		{
			bool limitRange = true;
			int range = 300;
			if (dsq->game->avatar->singing)
				range = 100;
			else
				limitRange = false;

			if (limitRange)
			{
				Vector diff = core->mouse.position - core->center;
				if (diff.getSquaredLength2D() > sqr(range))
				{
					diff.setLength2D(range);
					core->mouse.position = core->center + diff;
				}
			}
		}
	}
}

//prepare for screenshot or unprepare
void DSQ::prepScreen(bool t)
{
	if (t)
	{
		cursor->offset = Vector(2000, 0);

		if (game->miniMapRender)
			game->miniMapRender->offset = Vector(2000,0);
		if (fpsText)
			fpsText->offset = Vector(2000,0);
	}
	else
	{

		cursor->offset = Vector(0,0);
		if (game->miniMapRender)
			game->miniMapRender->offset = Vector(0,0);
		if (fpsText)
			fpsText->offset = Vector(0,0);
	}
}

void DSQ::onRender()
{
	if (cursor)
	{
		// HACK: not so pretty :D
		if (core->getTopStateObject() == (StateObject*)game)
		{
			if (doScreenshot)
				prepScreen(1);
		}
		cursor->position = mouse.position;
		cursor->position.z = 0;
	}
}

void DSQ::vision(std::string folder, int num, bool ignoreMusic)
{
	toggleBlackBars(1);

	dsq->toggleCursor(false);
	if (game)
		game->togglePause(true);

	dsq->overlay->color = Vector(1,1,1);

	float t = 0.1f;

	dsq->game->miniMapRender->toggle(0);

	fade(1, t);
	run(t);

	// load images
	typedef std::list<Quad*> QuadList;
	QuadList images;

	for (int i = num-1; i >= 0; i--)
	{
		Quad *q = new Quad;
		std::string label = "visions/"+folder+"/"+numToZeroString(i, 2)+".png";

		q->setTexture(label);

		q->setWidthHeight(800,600);
		q->followCamera = 1;
		q->position = Vector(400,300);
		images.push_front(q);
		addRenderObject(q, LR_HUD);
	}

	if (!ignoreMusic)
		sound->setMusicFader(0, t);

	for (QuadList::iterator i = images.begin(); i != images.end(); i++)
	{
		sound->playSfx("memory-flash");

		(*i)->scale.interpolateTo(Vector(1.1f,1.1f), 0.4f);
		fade(0, t);
		run(t);

		run(0.1f);

		fade(1, t);
		run(t);

		(*i)->alpha = 0;
	}

	if (game)
		game->togglePause(false);
	dsq->toggleCursor(true);

	sound->playSfx("memory-flash");
	fade(0, t);
	run(t);

	for (QuadList::iterator i = images.begin(); i != images.end(); i++)
	{
		(*i)->safeKill();
	}
	images.clear();

	if (!ignoreMusic)
		sound->setMusicFader(1, t);

	dsq->overlay->color = Vector(0,0,0);

	dsq->game->miniMapRender->toggle(1);

	toggleBlackBars(0);
}

bool DSQ::isDeveloperKeys()
{
#ifdef AQUARIA_DEMO
	return false;
#endif

	return user.system.devModeOn;
}

bool DSQ::canOpenEditor() const
{
	return dsq->isDeveloperKeys() || (dsq->mod.isActive() && !dsq->mod.isEditorBlocked());
}

bool DSQ::isQuitFlag()
{
	return watchQuitFlag;
}

void DSQ::run(float runTime /* = -1 */, bool skipRecurseCheck)
{
	if(isDeveloperKeys() && isNested() && !skipRecurseCheck)
		errorLog("Warning: Nesting recursive main()");

	Core::run(runTime);
}

void DSQ::watch(float t, int canQuit)
{
	watchQuitFlag = false;
	watchForQuit = false;

	bool wasInputEnabled = false;

	if (dsq->game && dsq->game->avatar)
	{
		wasInputEnabled = dsq->game->avatar->isInputEnabled();

		if (wasInputEnabled)
		{
			dsq->game->avatar->disableInput();
		}
	}

	core->quitNestedMain();

	if (canQuit)
	{
		watchForQuit = true;
	}

	if (t != 0.0f)
		core->run(t);
	else
		errorLog("Called Watch with time == 0");

	if (canQuit && watchQuitFlag)
	{
		// did it!
	}

	watchForQuit = false;

	if (dsq->game && dsq->game->avatar)
	{
		if (wasInputEnabled)
			dsq->game->avatar->enableInput();
	}
}

void DSQ::action(int id, int state, int source, InputDevice device)
{
	Core::action(id, state, source, device);

	if (id == ACTION_ESC && !state)
	{
		if (isInCutscene())
		{
			if (isCutscenePaused())
			{
				pauseCutscene(false);
			}
			else
			{
				pauseCutscene(true);
			}
		}
	}

	if(id == ACTION_SCREENSHOT && state)
	{
		screenshot();
	}
}

void DSQ::bindInput()
{
	clearActions();
	almb.clear();
	armb.clear();

	addAction(ACTION_ESC, KEY_ESCAPE, -1);
	addAction(MakeFunctionEvent(DSQ, onSwitchScreenMode), KEY_RETURN, 1);

	if (isDeveloperKeys())
	{
		addAction(MakeFunctionEvent(DSQ, toggleConsole), KEY_TILDE, 0);
		addAction(MakeFunctionEvent(DSQ, toggleRenderCollisionShapes), KEY_RETURN, 0);
	}
	addAction(MakeFunctionEvent(DSQ, debugMenu), KEY_BACKSPACE, 0);
	//addAction(MakeFunctionEvent(DSQ, takeScreenshotKey	),		KEY_P,				0);

	for(size_t i = 0; i < dsq->user.control.actionSets.size(); ++i)
	{
		ActionSet& as = dsq->user.control.actionSets[i];
		int sourceID = (int)i;

		as.importAction(this, "Escape",		ACTION_ESC, sourceID);
		as.importAction(this, "Screenshot",		ACTION_SCREENSHOT, sourceID);

		if(ActionInput *a = as.getActionInputByName("PrimaryAction"))
			almb.push_back(a);
		if(ActionInput *a = as.getActionInputByName("SecondaryAction"))
			armb.push_back(a);
	}
}

void DSQ::jiggleCursor()
{
	// hacky
	SDL_ShowCursor(SDL_ENABLE);
	SDL_ShowCursor(SDL_DISABLE);
}

void DSQ::updateActionButtons()
{
	// HACK: not optimal

	// This must be done *before* Core::updateActionButtons()
	// for LMB/RMB emulation to work properly -- fg
	if (/*inputMode != INPUT_KEYBOARD &&*/ game->isActive())
	{
		for(size_t i = 0; i < almb.size(); ++i)
			if (ActionMapper::getKeyState(almb[i]->data.single.key[0]) || ActionMapper::getKeyState(almb[i]->data.single.key[1]))
			{
				mouse.buttons.left = DOWN;
				break;
			}
			for(size_t i = 0; i < armb.size(); ++i)
				if (ActionMapper::getKeyState(armb[i]->data.single.key[0]) || ActionMapper::getKeyState(armb[i]->data.single.key[1]))
				{
					mouse.buttons.right = DOWN;
					break;
				}
	}

	if (joystickAsMouse)
	{
		for(size_t i = 0; i < almb.size(); ++i)
			if (ActionMapper::getKeyState(almb[i]->data.single.joy[0]))
			{
				mouse.buttons.left = DOWN;
				break;
			}
			for(size_t i = 0; i < armb.size(); ++i)
				if (ActionMapper::getKeyState(armb[i]->data.single.joy[0]))
				{
					mouse.buttons.right = DOWN;
					break;
				}
	}

	Core::updateActionButtons();
}

static float skipSfxVol = 1.0;
void DSQ::onUpdate(float dt)
{
	if (isSkippingCutscene())
	{
		if (!isInCutscene())
		{
			pauseCutscene(false);
			skippingCutscene = false;
			settings.renderOn = true;
			sound->setSfxVolume(skipSfxVol);
		}
		else
		{
			sound->stopVoice();
		}
	}
	else
	{
		if (isCutscenePaused())
		{
			sound->pause();
			float sec = 1.0f/60.0f;
			while (isCutscenePaused())
			{
				pollEvents(sec);
				ActionMapper::onUpdate(sec);
				SDL_Delay(int(sec*1000));
				render();
				showBuffer();
				resetTimer();

				if (_canSkipCutscene && core->getKeyState(KEY_S))
				{
					skippingCutscene = true;
					settings.renderOn = false;
					skipSfxVol = sound->getSfxVol();
					sound->setSfxVolume(0.0);
					resetTimer();
					sound->resume();
					return;
				}
			}
			dsq->resetTimer();
			dsq->sound->resume();
		}
	}

	// This queries pressed keys and updates ActionMapper
	Core::onUpdate(dt);


	mod.update(dt);

	if (dsq->game && watchForQuit && isNested())
	{
		if (dsq->game->isActing(ACTION_ESC, -1))
		{
			watchQuitFlag = true;
			quitNestedMain();
		}
	}

	// messy
	if (versionLabel && versionLabel->alpha.x > 0)
	{
		versionLabel->position = Vector(10 - core->getVirtualOffX(), 575);
	}

	if (noEffectTimer > 0)
	{
		noEffectTimer -=dt;
		if (noEffectTimer < 0)
			noEffectTimer = 0;
	}


	subtitlePlayer.update(dt);

	demo.update(dt);

	if (joystickEnabled)
	{
		if (dsq->getInputMode() != INPUT_JOYSTICK)
		{
			const float thresh = JOY_AXIS_THRESHOLD;
			for(size_t i = 0; i < getNumJoysticks(); ++i)
				if(Joystick *j = getJoystick(i))
					if(j && j->isEnabled())
						if (j->anyButton() || !j->position.isLength2DIn(thresh) || !j->rightStick.isLength2DIn(thresh))
						{
							//debugLog("setting joystick input mode");
							dsq->setInputMode(INPUT_JOYSTICK);
						}
		}
		else if (dsq->getInputMode() != INPUT_MOUSE)
		{
			if ((!core->mouse.change.isLength2DIn(5) || (core->getMouseButtonState(0) || core->getMouseButtonState(1))) /*&& !core->joystick.anyButton()*/)
			{
				//debugLog("setting mouse input mode");
				dsq->setInputMode(INPUT_MOUSE);
			}
		}
	}
	if (dsq->game->avatar)
	{
		if (dsq->game->avatar->isActing(ACTION_SWIMUP, -1) ||
			dsq->game->avatar->isActing(ACTION_SWIMDOWN, -1) ||
			dsq->game->avatar->isActing(ACTION_SWIMLEFT, -1) ||
			dsq->game->avatar->isActing(ACTION_SWIMRIGHT, -1))
		{
			dsq->setInputMode(INPUT_KEYBOARD);
		}
	}

	// check the actual values, since mouse.buttons.left might be overwritten by keys
	int cb = 0;
	if (user.control.flipInputButtons)
		cb = 1;

	if (dsq->getInputMode() == INPUT_KEYBOARD && (core->getMouseButtonState(cb)))
	{
		dsq->setInputMode(INPUT_MOUSE);
	}



	if (isDeveloperKeys() && cmDebug && cmDebug->alpha == 1 && fpsText)
	{
		std::ostringstream os;


		if (dsq->game->avatar)
		{
			Avatar *avatar = dsq->game->avatar;
			os << "rolling: " << dsq->game->avatar->isRolling() << " rollDelay: " << dsq->game->avatar->rollDelay << std::endl;
			os << "canChangeForm: " << dsq->game->avatar->canChangeForm << " gamespeed: " << gameSpeed.x << std::endl;
			os << "h: " << dsq->game->avatar->health << " / " << dsq->game->avatar->maxHealth << std::endl;
			os << "biteTimer: " << dsq->game->avatar->biteTimer << " flourTimer: " << dsq->game->avatar->flourishTimer.getValue() << std::endl;
			os << "stillTimer: " << dsq->game->avatar->stillTimer.getValue() << std::endl;
			os << "hp: " << dsq->game->avatar->getHealthPerc() << " flourishPowerTimer: " << dsq->game->avatar->flourishPowerTimer.getValue() << std::endl;
			os << "maxSpeed: " << dsq->game->avatar->currentMaxSpeed << " - ";
			os << "lockedToWall: " << dsq->game->avatar->state.lockedToWall;
			os << std::endl;
			os << "swmng: " << avatar->isSwimming();
			os << " dualFormCharge: " << continuity.dualFormCharge;
			os << std::endl;
			os << "vel(" << avatar->vel.x << ", " << avatar->vel.y << ") ";
			os << "vel2(" << avatar->vel2.x << ", " << avatar->vel2.y << ")";
			os << std::endl;
			os << "rot: " << avatar->rotation.z << " rotoff: " << avatar->rotationOffset.z << std::endl;
			os << "p(" << int(avatar->position.x) << ", " << int(avatar->position.y) << ")" << std::endl;
			os << "inp: " << avatar->isInputEnabled() << std::endl;
			os << "wallNormal(" << avatar->wallNormal.x << ", " << avatar->wallNormal.y << ") collradius: " << avatar->collideRadius << std::endl;
			os << "burst: " << avatar->burst << " burstTimer: " << avatar->burstTimer << std::endl;
			os << "inCurrent: " << avatar->isInCurrent() << std::endl;
			os << "qsongCastDelay: " << avatar->quickSongCastDelay << std::endl;
			os << "singing: " << dsq->game->avatar->singing << " blockSinging: " << dsq->game->avatar->isBlockSinging();
			os << " look: " << dsq->game->avatar->state.updateLookAtTime << " ";

			os << "inputMode: ";
			switch(dsq->getInputMode())
			{
			case INPUT_MOUSE:
				os << "mouse";
			break;
			case INPUT_JOYSTICK:
				os << "joystick";
			break;
			case INPUT_KEYBOARD:
			break;
			case INPUT_NODEVICE:
				break;
			}
			os << std::endl;
			Bone *b = dsq->game->avatar->skeletalSprite.getBoneByIdx(1);
			if (b)
				os << " headRot: " << b->rotation.z;
			os << std::endl;
			os << "fh: " << dsq->game->avatar->isfh() << " fv: " << dsq->game->avatar->isfv() << std::endl;
			os << "canActivate: " << dsq->game->avatar->canActivateStuff();
			os << " canBurst: " << dsq->game->avatar->canBurst();
			os << " canLTW: " << dsq->game->avatar->canLockToWall();
			os << " canSAC: " << dsq->game->avatar->canSwimAgainstCurrents() << std::endl;
		}

		// DO NOT CALL AVATAR-> beyond this point
		os << "story: " << continuity.getStory() << std::endl;
		os << "invGlobalScale: " << core->invGlobalScale;
		os << std::endl;
		os << "globalScale: " << core->globalScale.x << std::endl;
		os << "mousePos:(" << core->mouse.position.x << ", " << core->mouse.position.y << ") mouseChange:(" << core->mouse.change.x << ", " << core->mouse.change.y << ")\n";
		for(size_t i = 0; i < getNumJoysticks(); ++i)
			if(Joystick *j = getJoystick(i))
			{
				os << "J[" << i << "," << (j->isEnabled() ? " on" : "off") << "]:[";
				for(unsigned ii = 0; ii < MAX_JOYSTICK_BTN; ++ii)
					if(j->getButton(ii))
						os << (ii % 10);
					else
						os << '-';
				os << "], (" << j->position.x << ", " << j->position.y << "), ("<< j->rightStick.x << ", " << j->rightStick.y << ")\n";
			}
		os << "altState: " << core->getKeyState(KEY_LALT) << " | " << core->getKeyState(KEY_RALT) << " mb: " << mouse.buttons.left << mouse.buttons.middle << mouse.buttons.right << std::endl;
		os << "PMFree: " << particleManager->getFree() << " Active: " << particleManager->getNumActive() << std::endl;
		os << "cameraPos: (" << dsq->cameraPos.x << ", " << dsq->cameraPos.y << ")" << std::endl;
		os << "worldType: " << continuity.getWorldType() << " worldPaused: " << game->isWorldPaused() << std::endl;
		os << "voiceTime: " << dsq->sound->getVoiceTime() << " bNat: " << dsq->game->bNatural;
		int ca, ma;
		dsq->sound->getStats(&ca, &ma);
		os << " ca: " << ca << " ma: " << ma << std::endl;
		os << dsq->sound->getVolumeString() << std::endl;
		os << "runInBG: " << core->settings.runInBackground << " nested: " << core->getNestedMains() << std::endl;
		os << core->globalResolutionScale.x << ", " << core->globalResolutionScale.y << std::endl;
		os << "elemu: " << game->elementUpdateList.size() << " elemi: " << game->elementInteractionList.size() << std::endl;
		os << "Lua mem: " << scriptInterface.gcGetStats() << " KB" << std::endl;

		cmDebug->position = Vector(20 - virtualOffX,50);
		cmDebug->setText(os.str());
	}

	if (isDeveloperKeys() && fpsText && cmDebug && cmDebug->alpha == 1)
	{
		std::ostringstream os;
		os << "FPS: " << core->fps << " | ROC: " << core->renderObjectCount << " | RC: " << g_dbg_numRenderCalls << " | RES: " << core->resources.size();
		os << " | p: " << core->processedRenderObjectCount << " | t: " << core->totalRenderObjectCount;
		os << " | s: " << dsq->continuity.seconds;
		os << " | sndQ: " << core->dbg_numThreadDecoders;
		os << " | dt: " << core->get_current_dt();

		fpsText->position = Vector(10 - virtualOffX,580);
		fpsText->setText(os.str());
	}

	if(console && console->alpha == 1)
		console->position = Vector(10 - virtualOffX,400);

	if (shakeCameraTimer > 0)
	{
		shakeCameraTimer -= dt;
		if (shakeCameraTimer <= 0)
		{
			shakeCameraTimer = 0;
			cameraOffset = Vector(0,0);
		}
		else
		{
			cameraOffset = Vector((rand()%int(shakeCameraMag))-shakeCameraMag/2.0f, (rand()%int(shakeCameraMag))-shakeCameraMag/2.0f);
		}
	}

	updatepecue(dt);

	Network::update();

	Shot::clearShotGarbage();

	AquariaGuiElement::UpdateGlobalFocus(dt);
}


void DSQ::shakeCamera(float mag, float time)
{
	cameraOffset = Vector(0,0);
	shakeCameraMag = mag;
	shakeCameraTimer = time;
}

void DSQ::delay(float dt)
{
	core->run(dt);
}

void DSQ::fade(float alpha, float time)
{
	if (overlay)
		overlay->alpha.interpolateTo(alpha, time,0);
}

void DSQ::toggleCursor(bool v, float time)
{
	if (!cursor) return;
	float t = time;
	if (time == -1)
		t = 0.1f;
	if (!v)
		cursor->alpha.interpolateTo(0, t);
	else
		cursor->alpha.interpolateTo(1, t);
}

void DSQ::playVisualEffect(int vfx, Vector position, Entity *target)
{
	switch(vfx)
	{
	case VFX_SHOCK:
	{



		core->sound->playSfx("ShockWave");

		float t =1.0;

		Quad *q = new Quad;
		q->position = position;
		q->scale = Vector(0,0);
		q->scale.interpolateTo(Vector(5,5),t);

		q->alpha.ensureData();
		q->alpha.data->path.addPathNode(0, 0);
		q->alpha.data->path.addPathNode(0.75f, 0.25f);
		q->alpha.data->path.addPathNode(0.75f, 0.75f);
		q->alpha.data->path.addPathNode(0, 1);
		q->alpha.startPath(t);
		q->setBlendType(RenderObject::BLEND_ADD);
		q->setTexture("particles/EnergyRing");
		if (target)
			q->positionSnapTo = &target->position;

		game->addRenderObject(q, LR_PARTICLES);

		if (target && target->getEntityType() == ET_AVATAR)
			if (core->afterEffectManager)
				core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08f,0.05f,22,0.2f, 1.2f));

		t = 0.75f;
		{
			Quad *q = new Quad;
			q->position = position;
			q->scale = Vector(0.5,0.5);
			q->scale.interpolateTo(Vector(2,2),t);
			q->alpha.ensureData();
			q->alpha.data->path.addPathNode(0, 0);
			q->alpha.data->path.addPathNode(0.75f, 0.25f);
			q->alpha.data->path.addPathNode(0.75f, 0.75f);
			q->alpha.data->path.addPathNode(0, 1);
			q->alpha.startPath(t);
			q->setBlendType(RenderObject::BLEND_ADD);
			q->setTexture("particles/EnergyPart");
			if (target)
				q->positionSnapTo = &target->position;
			q->rotation.z = rand()%360;
			game->addRenderObject(q, LR_PARTICLES);
		}
	}
	break;
	case VFX_SHOCKHIT:
	{
		float t = 1.0;
		{
		Quad *q = new Quad;
		q->position = position;
		q->scale = Vector(1,1);
		q->scale.interpolateTo(Vector(3,3),t);
		q->alpha.ensureData();
		q->alpha.data->path.addPathNode(0, 0);
		q->alpha.data->path.addPathNode(1, 0.3f);

		q->alpha.data->path.addPathNode(0, 1);
		q->alpha.startPath(t);
		q->setBlendType(RenderObject::BLEND_ADD);
		q->rotation.z = rand()%360;
		q->setTexture("particles/EnergyRing");

		q->rotation.interpolateTo(Vector(0,0,q->rotation.z + 360), t+0.1f);
		game->addRenderObject(q, LR_PARTICLES);
		}

		t = 0.75;
		{
			Quad *q = new Quad;
			q->position = position;
			q->scale = Vector(1,1);
			q->scale.interpolateTo(Vector(3,3),t);
			q->alpha.ensureData();
			q->alpha.data->path.addPathNode(0, 0);
			q->alpha.data->path.addPathNode(0.8f, 0.25f);

			q->alpha.data->path.addPathNode(0, 1);
			q->alpha.startPath(t);
			q->setBlendType(RenderObject::BLEND_ADD);

			q->setTexture("particles/EnergyDeltas");
			q->rotation.z = rand()%360;

			game->addRenderObject(q, LR_PARTICLES);
		}
	}
	break;
	case VFX_RIPPLE:
		if (core->afterEffectManager)
			core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.04f,0.06f,15,0.2f));
	break;

	}
}

void DSQ::addElement(Element *e)
{
	elements.push_back(e);
	if (e->bgLayer >= 0 && e->bgLayer < 16)
	{
		e->bgLayerNext = firstElementOnLayer[e->bgLayer];
		firstElementOnLayer[e->bgLayer] = e;
	}
	else
	{
		e->bgLayerNext = 0;
	}
}

void DSQ::modifyDt(float &dt)
{
	if (isDeveloperKeys())
	{
		if (core->getKeyState(KEY_G))
		{
			if(core->getShiftState())
				dt *= 10;
			else
				dt *= 4;
		}
		else if (core->getKeyState(KEY_F))
		{
			if (core->getShiftState())
				dt *= 0.1f;
			else
				dt *= 0.6f;
		}
		else if (core->getKeyState(KEY_H))
			dt = FRAME_TIME;
		else
		{
			// frame cap
			if (dt > FRAME_TIME)
				dt = FRAME_TIME;
		}
		if (core->getKeyState(KEY_H))
			stopVoice();
	}
	else
	{
		if (dt > FRAME_TIME)
			dt = FRAME_TIME;
	}

	if (skippingCutscene)
		dt = 0.07f;

	gameSpeed.update(dt);
	dt *= gameSpeed.x;

	if (frameOutputMode)
	{
		dt = 1.0f/60.0f;
		doScreenshot = true;
	}

	if (dsq->demo.mode == Demo::DEMOMODE_RECORD)
	{
		dt = 1.0f/60.0f;
	}
}

void DSQ::removeElement(Element *element)
{
	for (size_t i = 0; i < dsq->elements.size(); i++)
	{
		if (dsq->elements[i] == element)
		{
			removeElement(i);
			break;
		}
	}

}
// only happens in editor, no need to optimize
void DSQ::removeElement(size_t idx)
{
	ElementContainer copy = elements;
	clearElements();
	size_t i = 0;
	for (i = 0; i < idx; i++)
	{
		addElement(copy[i]);
	}
	for (i = idx+1; i < copy.size(); i++)
	{
		addElement(copy[i]);
	}
	copy.clear();

	if (!dsq->game->elementUpdateList.empty())
		dsq->game->rebuildElementUpdateList();
}

void DSQ::clearElements()
{
	elements.clear();
	for (int i = 0; i < 16; i++)
		firstElementOnLayer[i] = 0;
}


void DSQ::addEntity(Entity *entity)
{
	size_t i;
	for (i = 0; entities[i] != 0; i++) {}
	if (i+1 >= entities.size())
		entities.resize(entities.size()*2, 0);
	entities[i] = entity;
	entities[i+1] = 0;
}

void DSQ::removeEntity(Entity *entity)
{
	int i;
	for (i = 0; entities[i] != 0; i++)
	{
		if (entities[i] == entity)
			break;
	}
	for (; entities[i] != 0; i++)
	{
		entities[i] = entities[i+1];
	}
}

void DSQ::clearEntities()
{
	const int size = entities.size();
	int i;
	for (i = 0; i < size; i++)
	{
		entities[i] = 0;
	}
}


std::string DSQ::getSaveDirectory()
{
	return getUserDataFolder() + "/save";
}

ParticleEffect *DSQ::spawnParticleEffect(const std::string &name, Vector position, float rotz, float t, int layer, float follow)
{
	if (name.empty())
		return NULL;
	if (t!=0)
	{
		PECue p(name, position, rotz, t);
		pecue.push_back(p);
		return NULL;
	}

	ParticleEffect *e = core->createParticleEffect(name, position, layer, rotz);
	e->followCamera = follow;
	return e;
}

void DSQ::spawnAllIngredients(const Vector &position)
{
	continuity.spawnAllIngredients(position);
}

void DSQ::updatepecue(float dt)
{
	if (!core->particlesPaused)
	{

		int nz = 0;
		for (size_t i = 0; i < pecue.size(); i++)
		{
			PECue *p = &pecue[i];
			if (p->t > 0)
			{
				p->t -= dt;
				if (p->t < 0)
				{
					p->t = 0;
					spawnParticleEffect(p->name, p->pos, p->rot, 0);
				}
				nz ++;
			}
		}
		// lazy ass delete
		if (nz == 0)
		{
			pecue.clear();
		}
	}
}

void AquariaScreenTransition::capture()
{
	assert(screen_texture);
	this->alpha = 0;
	InterpolatedVector oldAlpha = dsq->cursor->alpha;
	dsq->cursor->alpha.x = 0;
	int width=0, height=0;
	core->render();

	width = core->getWindowWidth();
	height = core->getWindowHeight();

	glBindTexture(GL_TEXTURE_2D,screen_texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);


	dsq->cursor->alpha = oldAlpha;
	core->render();
	core->showBuffer();

	this->alpha = 1;
	//ScreenTransition::capture();
}

void DSQ::setCutscene(bool on, bool canSkip)
{
	inCutscene = on;
	_canSkipCutscene = canSkip;
}

bool DSQ::canSkipCutscene()
{
	return _canSkipCutscene;
}

bool DSQ::isSkippingCutscene()
{
	return skippingCutscene;
}

bool DSQ::isInCutscene()
{
	return inCutscene;
}

bool DSQ::isCutscenePaused()
{
	return cutscenePaused;
}

void DSQ::pauseCutscene(bool on)
{
	cutscenePaused = on;

	cutsceneEffects(on);
}

void DSQ::cutsceneEffects(bool on)
{
	if (cutscene_bg && cutscene_text && cutscene_text2)
	{
		if (canSkipCutscene())
		{
			cutscene_text->offset = Vector(0, -10);
			cutscene_text2->offset = Vector(0, -10);
		}
		else
		{
			cutscene_text->offset = Vector(0,0);
			cutscene_text2->offset = Vector(0,0);
		}
		cutscene_bg->alpha.x = on?1:0;
		cutscene_text->alpha.x = on?1:0;
		cutscene_text2->alpha.x = (on&&dsq->canSkipCutscene())?1:0;
	}
}

void DSQ::onBackgroundUpdate()
{
	Network::update();
	Core::onBackgroundUpdate();
}

void DSQ::resetLayerPasses()
{
	for(size_t i = 0; i < renderObjectLayers.size(); ++i)
	{
		renderObjectLayers[i].startPass = 0;
		renderObjectLayers[i].endPass = 0;
	}
	renderObjectLayers[LR_ENTITIES].startPass = -2;
	renderObjectLayers[LR_ENTITIES].endPass = 5;
}

bool DSQ::isMiniMapCursorOkay()
{
	return (!useMouseInput() ||  (!game->miniMapRender || !game->miniMapRender->isCursorIn()));
}

void DSQ::onJoystickAdded(int deviceID)
{
	Core::onJoystickAdded(deviceID);
	fixupJoysticks();
}

void DSQ::onJoystickRemoved(int instanceID)
{
	Core::onJoystickRemoved(instanceID);
	fixupJoysticks();
}

void DSQ::fixupJoysticks()
{
	for(size_t i = 0; i < getNumJoysticks(); ++i)
		if(Joystick *j = getJoystick(i))
			j->setEnabled(false);

	for(size_t i = 0; i < user.control.actionSets.size(); ++i)
	{
		ActionSet& as = user.control.actionSets[i];
		as.updateJoystick();
	}

	// HACK: why here? kinda dirty, but the joystick ID needs to be propagated
	importActionButtons();
}

void DSQ::initActionButtons()
{
	clearActionButtons();

	std::vector<int> allkeys;
	for(int i = 0; i < INTERNALLY_USED_ACTION_BUTTONS_END; ++i)
		allkeys.push_back(i);

	// create sentinel
	ActionButtonStatus *allbtn = new ActionButtonStatus;
	allbtn->importQuery(&allkeys[0], allkeys.size());
	actionStatus.push_back(allbtn);

	// create the rest
	for(size_t i = 0; i < user.control.actionSets.size(); ++i)
		actionStatus.push_back(new ActionButtonStatus);

	importActionButtons();
}

void DSQ::importActionButtons()
{
	assert(user.control.actionSets.size()+1 == actionStatus.size());

	// ignore sentinel
	for(size_t i = 1; i < actionStatus.size(); ++i)
	{
		const ActionSet& as = user.control.actionSets[i-1];
		ActionButtonStatus *abs = actionStatus[i];
		abs->import(as);
	}
}

void DSQ::loadStringBank()
{
	stringbank.clear();

#define BANKSTRING(id, str) stringbank.set(id, str);
#include "StringBank_gen.h"
#undef BANKSTRING

	// First, load the default string banks
	stringbank.load("data/stringbank.txt");
	if (mod.isActive())
		stringbank.load(mod.getPath() + "stringbank.txt");

	// Then, load localized ones. If some entries in these are missing, the default for each is taken.
	std::string fname = localisePath("data/stringbank.txt");
	stringbank.load(fname);

	if (mod.isActive()) {
		fname = localisePath(mod.getPath() + "stringbank.txt", mod.getPath());
		stringbank.load(fname);
	}
}

InputDevice DSQ::getInputMode() const
{
	return lastInputMode;
}

InputDevice DSQ::getInputMode(int source) const
{
	assert(source >= 0 && size_t(source) < _inputModes.size());
	return _inputModes[source];
}

InputDevice DSQ::getInputModeSafe(int source) const
{
	return source < 0 ? lastInputMode :
		(size_t(source) < _inputModes.size() ? _inputModes[source] : INPUT_NODEVICE);
}

bool DSQ::useMouseInput() const
{
	return lastInputMode == INPUT_MOUSE;
}

bool DSQ::useJoystickInput() const
{
	return lastInputMode == INPUT_JOYSTICK;
}
