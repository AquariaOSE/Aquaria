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
#include "../ExternalLibs/glpng.h"
#include "../BBGE/AfterEffect.h"
#include "../BBGE/ProfRender.h"

#include "DSQ.h"
#include "States.h"
#include "Game.h"
#include "Logo.h"
#include "Avatar.h"
#include "Entity.h"
#include "Avatar.h"
#include "Shot.h"
#include "GridRender.h"
#include "AutoMap.h"
#include "PackRead.h"

#include "RoundedRect.h"
#include "TTFFont.h"

#ifdef BBGE_BUILD_OPENGL
	#include <sys/stat.h>
#endif

#ifdef BBGE_BUILD_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

static void Linux_CopyTree(const char *src, const char *dst)
{
    //printf("Linux_CopyTree('%s', '%s')...\n", src, dst);

    struct stat statbuf;
    if (stat(src, &statbuf) == -1)
        return;

    if (S_ISDIR(statbuf.st_mode))
    {
        mkdir(dst, 0700);  // don't care if this fails.
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


float titTimer = 0;

const int saveSlotPageSize = 4;
int maxPages = 7;
#ifdef AQUARIA_BUILD_CONSOLE
const int MAX_CONSOLELINES	= 14;
#endif

DSQ *dsq = 0;

const bool isReleaseCandidate	= false;
const bool isFinalCandidate		= false;
const bool isGoldMaster			= true;

int setInpGrab = -1;

Vector savesz;

/// WARNING: this is just to init, the actual value is set from user settings!
#define PARTICLE_AMOUNT_DEFAULT			2048	// 64 // 1024 // 2048

#ifdef AQUARIA_DEMO
	#define APPNAME "Aquaria Demo"
#else
	#define APPNAME "Aquaria"
#endif

DSQ::DSQ(std::string fileSystem) : Core(fileSystem, LR_MAX, APPNAME, PARTICLE_AMOUNT_DEFAULT, "Aquaria")
{
	// 2048
	//createDirectory(getSaveDirectory());
	dsq = this;

	cutscene_bg = 0;
	cutscene_text = 0;
	cutscene_text2 = 0;

	doScreenTrans = false;

	cutscenePaused = false;
	inCutscene = false;
	_canSkipCutscene = false;
	skippingCutscene = false;

	almb = armb = 0;
	bar_left = bar_right = bar_up = bar_down = barFade_left = barFade_right = 0;
	
	// do copy stuff
#ifdef BBGE_BUILD_UNIX
	std::string fn;
	fn = getPreferencesFolder() + "/" + userSettingsFilename;
	if (!exists(fn))
		Linux_CopyTree(core->adjustFilenameCase(userSettingsFilename).c_str(), core->adjustFilenameCase(fn).c_str());

	fn = getUserDataFolder() + "/_mods";
	if (!exists(fn))
		Linux_CopyTree(core->adjustFilenameCase("_mods").c_str(), core->adjustFilenameCase(fn).c_str());
#endif

	std::string p1 = getUserDataFolder();
	std::string p2 = getUserDataFolder() + "/save";
#if defined(BBGE_BUILD_UNIX)
	mkdir(p1.c_str(), S_IRWXU);
	mkdir(p2.c_str(), S_IRWXU);
#elif defined(BBGE_BUILD_WINDOWS)
	CreateDirectoryA(p2.c_str(), NULL);
#endif
	
	difficulty = DIFF_NORMAL;

	/*
	if (exists("easy"))
		difficulty = DIFF_EASY;//DIFF_NORMAL;
	*/

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
	menuSelectDelay = 0;
	modSelector = 0;
	blackout = 0;
	useMic = false;
	autoSingMenuOpen = false;
	inputMode = INPUT_MOUSE;
	overlay = 0;
	recentSaveSlot = -1;
	arialFontData = 0;

#ifdef BBGE_BUILD_ACHIEVEMENTS_INTERNAL
	achievement_text = 0;
	achievement_box = 0;
#endif

	vars = &v;
	v.load();

#ifdef AQUARIA_BUILD_CONSOLE
	console = 0;
#endif
	cmDebug = 0;
	languagePack = "english";
	saveSlotMode = SSM_NONE;
	afterEffectManagerLayer = LR_AFTER_EFFECTS; // LR_AFTER_EFFECTS
	renderObjectLayers.resize(LR_MAX);

	entities.resize(64, 0);
	
	//Emitter::particleLayer = LR_PARTICLES;
	sortEnabled = false;
	shakeCameraTimer = shakeCameraMag = 0;
	avgFPS.resize(dsq->user.video.fpsSmoothing);

	cursor = cursorGlow = 0;

	for (int i = 0; i < 16; i++)
		firstElementOnLayer[i] = 0;

	addStateInstance(game = new Game);
	addStateInstance(new GameOver);
#ifdef AQUARIA_BUILD_SCENEEDITOR
	addStateInstance(new AnimationEditor);
#endif
	addStateInstance(new Intro2);
	addStateInstance(new BitBlotLogo);
#ifdef AQUARIA_BUILD_SCENEEDITOR
	addStateInstance(new ParticleEditor);
#endif
	addStateInstance(new Credits);
	addStateInstance(new Intro);
	addStateInstance(new Nag);

	//addStateInstance(new Logo);
	//addStateInstance(new SCLogo);
	//addStateInstance(new IntroText);
	//addStateInstance(new Intro);

	//stream = 0;
}

DSQ::~DSQ()
{
	dsq = 0;
}

void DSQ::onAltTab()
{
	if (getAltState())
	{
		if (!core->isNested())
		{
			if (_fullscreen)
			{
				core->toggleScreenMode(false);
			}
		}
	}
}

// actually toggle
void DSQ::toggleFullscreen()
{
	//if (!core->isNested() && !core->sound->isPlayingVoice()) {
	core->toggleScreenMode(!_fullscreen);
	user.video.full = _fullscreen;
	//}
}

// for handling the input, not the actual switch functionality
void DSQ::onSwitchScreenMode()
{
	//if (!core->isNested() && !core->sound->isPlayingVoice())
	{
		
#if defined(BBGE_BUILD_WINDOWS) || defined(BBGE_BUILD_UNIX)
		if (getAltState()) {
			toggleFullscreen();
		}
#endif
	}
}

void DSQ::forceInputGrabOff()
{
	toggleInputGrabPlat(false);
	setInpGrab = 0;
#ifdef BBGE_BUILD_SDL
	SDL_ShowCursor(SDL_DISABLE);
#endif
}

void DSQ::rumble(float leftMotor, float rightMotor, float time)
{
	if (this->inputMode == INPUT_JOYSTICK)
		core->joystick.rumble(leftMotor, rightMotor, time);
}

void DSQ::newGame()
{
	dsq->game->resetFromTitle();
	dsq->initScene = "NaijaCave";
	dsq->game->transitionToScene(dsq->initScene);
}

void DSQ::loadElementEffects()
{
 	std::ifstream inFile("data/elementeffects.txt");
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
			// >> e.wavy_min >> e.wavy_max >> e.wavy_flip;
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

ElementEffect DSQ::getElementEffectByIndex(int e)
{
	if (e < elementEffects.size() && e >= 0)
	{
		return elementEffects[e];
	}

	ElementEffect empty;
	empty.type = EFX_NONE;

	return empty;
}

Element *DSQ::getSolidElementNear(Vector pos, int rad)
{
	Element *closestE = 0;
	int closestDist = -1;
	for (int i = 0; i < elements.size(); i++)
	{
		Element *e = elements[i];
		int dist = (e->position - pos).getSquaredLength2D();
		if (e->isElementActive() && e->elementFlag == EF_SOLID && dist < sqr(rad) && (dist < closestDist || closestDist==-1))
		{
			closestDist = dist;
			closestE = e;
		}
	}
	return closestE;
}

Vector DSQ::getCameraCenter()
{
	return cameraPos; //+ Vector(400*(1.0f/core->globalScale.x),300*(1.0f/core->globalScale.x));
}

void DSQ::doScript(const std::string &script)
{
	/*
	this->script.loadScriptFile(script);
	this->script.run("void main()");
	*/
}

void DSQ::print(int x, int y, const std::string &text)
{
//	CTextDrawer::GetSingleton().PrintText(x, y, text.c_str());
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
	t->alpha.data->path.addPathNode(1, 0.8);
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
	s->alpha.data->path.addPathNode(1, 0.1);
	s->alpha.data->path.addPathNode(1, 0.8);
	s->alpha.data->path.addPathNode(0, 1);
	s->alpha.startPath(time);
	getTopStateData()->addRenderObject(s, LR_HUD);


	BitmapText *t = new BitmapText(&font);
	

	t->position =pos;
	t->alpha.ensureData();
	t->alpha.data->path.addPathNode(0, 0);
	t->alpha.data->path.addPathNode(1, 0.1);
	t->alpha.data->path.addPathNode(1, 0.8);
	t->alpha.data->path.addPathNode(0, 1);
	t->alpha.startPath(time);
	/*
	t->scale = Vector(0.7, 0.7);
	t->scale.interpolateTo(Vector(1, 1), 6);
	*/
	t->followCamera = 1;
	t->setLife(time + 0.5f);
	t->setDecayRate(1);
	//t->scrollText(text, 0.1);
	t->setText(text);
	getTopStateData()->addRenderObject(t, LR_HUD);
}

void DSQ::destroyFonts()
{
	/*
	if (font) { delete font; font = 0; }
	if (smallFont) { delete smallFont; smallFont = 0; }
	if (subsFont) { delete subsFont; subsFont = 0; }
	if (goldFont) { delete goldFont; goldFont = 0; }
	*/
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

	font.load("data/font-small.glf", 1, false);
	font.fontTopColor = Vector(0.9,0.9,1);
	font.fontBtmColor = Vector(0.5,0.8,1);
	font.overrideTexture = core->addTexture("font");

	smallFont.load("data/font-small.glf", 0.6, false);
	smallFont.fontTopColor = Vector(0.9,0.9,1);
	smallFont.fontBtmColor = Vector(0.5,0.8,1);
	smallFont.overrideTexture = core->addTexture("font");

	smallFontRed.load("data/font-small.glf", 0.6, false);
	smallFontRed.fontTopColor = Vector(1,0.9,0.9);
	smallFontRed.fontBtmColor = Vector(1,0.8,0.5);
	smallFontRed.overrideTexture = core->addTexture("font");

	subsFont.load("data/font-small.glf", 0.5, false);
	subsFont.fontTopColor = Vector(1,1,1);
	subsFont.fontBtmColor = Vector(0.5,0.8,1);
	subsFont.overrideTexture = core->addTexture("font");

	goldFont.load("data/font-small.glf", 1, false);
	goldFont.fontTopColor = Vector(1,0.9,0.5);
	goldFont.fontBtmColor = Vector(0.6,0.5,0.25);
	goldFont.overrideTexture = core->addTexture("font");

	debugLog("ttf...");
	arialFontData = (unsigned char *)readFile("data/font.ttf", &arialFontDataSize);
	if (arialFontData)
	{
		fontArialSmall   .create(arialFontData, arialFontDataSize, 12);
		fontArialBig     .create(arialFontData, arialFontDataSize, 18);
		fontArialSmallest.create(arialFontData, arialFontDataSize, 10);
	}
	debugLog("done loadFonts");

	/*
	//gold
	smallFont.fontTopColor = Vector(1,0.9,0.5);
	smallFont.fontBtmColor = Vector(0.6,0.5,0.25);
	*/

	//Texture::format = GL_LUMINANCE_ALPHA;
	
	//Texture::format = 0;
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
/*
#if defined(BBGE_BUILD_MACOSX)
		if (core->getCtrlState())
#elif defined(BBGE_BUILD_WINDOWS) || defined(BBGE_BUILD_UNIX)
*/
		if (core->getShiftState())
//#endif
		{
			core->frameOutputMode = false;
			dsq->game->togglePause(true);
			std::string s = dsq->getUserInputString("1: Refresh\n2: Heal\n3: Reset Cont.\n5: Set Invincible\n6: Set Flag\n8: All Songs\n9: All Ups\nS: learn song #\nF: Find Entity\nC: Set Costume\n0: Learn MArea Songs\nR: Record Demo\nP: Playback Demo\nT: Rewind Demo\nU: Ouput Demo Frames\nB: Unload Resources\nA: Reload Resources\nM: AutoMap\nJ: JumpState\nQ: QuitNestedMain", "");
			stringToUpper(s);

			/*
			char c=0;
			is >> str;
			*/

			/*
			if (dsq->postProcessingFx.isEnabled(FXT_RADIALBLUR))
			{
			dsq->postProcessingFx.disable(FXT_RADIALBLUR);
			}
			else
			{
			dsq->postProcessingFx.enable(FXT_RADIALBLUR);
			dsq->postProcessingFx.radialBlurColor = Vector(1,1,1);
			dsq->postProcessingFx.intensity = 0.1;
			}
			*/


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
				/*
				else if (c == '4')
				{
				dsq->continuity.learnSong(SONG_FISHFORM);
				}
				else if (c == '5')
				{
				dsq->continuity.learnSong(SONG_NATUREFORM);
				}
				*/
				else if (c == '5')
				{
					/*
					if (dsq->game->avatar->isInvincible())
					dsq->game->avatar->setInvincible(false);
					else
					dsq->game->avatar->setInvincible(true);
					*/
					dsq->game->invinciblity = !dsq->game->invinciblity;
				}
				else if (c == '6')
				{
					while (core->getKeyState(KEY_RETURN))
						core->main(0.1);
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

					/*
					std::ostringstream os;
					os << "debug: Learned Song: " << num << " from s: " << s;
					debugLog(os.str());
					*/
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
						dsq->cameraPos = e->position;
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
					//dsq->demo.togglePlayback(true);
					//core->frameOutputMode = false;
				}
				else if (c == 'K')
				{
					dsq->demo.clearRecordedFrames();
				}
				else if (c == 'M')
				{
					dsq->game->autoMap->toggle(!dsq->game->autoMap->isOn());
				}
				else if (c == 'H')
				{
					std::ostringstream os;
					os << dsq->game->avatar->health;
					std::istringstream is(dsq->getUserInputString("health", os.str()));
					float h = 0;
					is >> h;
					dsq->game->avatar->health = h;

				}
			}


			//dsq->game->togglePause(false);
		}
	}
}

void DSQ::takeScreenshotKey()
{
	if (core->getCtrlState() && core->getAltState())
		takeScreenshot();
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
	0.01, 0.07, 0.20, 0.23, 0.24, 0.25, 0.89, 1.00,
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


void DSQ::setVersionLabelText() {
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
		//os << "gm" << VERSION_GM;
	}

	#ifdef AQUARIA_CUSTOM_BUILD_ID
	os << AQUARIA_CUSTOM_BUILD_ID;
	#endif

	versionLabel->setText(os.str());
}

#ifdef BBGE_BUILD_SDL
static bool sdlVideoModeOK(const int w, const int h, const int bpp)
{
	return SDL_VideoModeOK(w, h, bpp, SDL_OPENGL | SDL_FULLSCREEN);
}
#endif

void DSQ::init()
{
	core->settings.runInBackground = true;

	weird = 0;

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

	routeShoulder = true;
	debugLog("DSQ init");

	useFrameBuffer = false;
	gameSpeed = 1;
	
	// steam gets inited in here
	Core::init();

	// steam callbacks are inited here
	dsq->continuity.init();

	//packReadInfo("mus.dat");

	this->setBaseTextureDirectory("gfx/");
	//sound->setMusicVolume(0);

//	PHYSFS_addToSearchPath("gfx",1);
//	PHYSFS_addToSearchPath("gfx.zpk",0 );

	bool mipmapsEnabled=true;
	bool fullscreen = true;
	int joystickMode = 0;
	int dsq_filter = 0;
	developerKeys = false;
	voiceOversEnabled = true;


	//core->messageBox("info", "loading user settings");
	user.load(false);
	
	particleManager->setSize(user.video.numParticles);

	fullscreen = user.video.full;

	float asp = float(user.video.resx)/float(user.video.resy);

	
	if (asp >= 1.0f && asp < 1.8f)
	{
	}
	else
	{
		std::ostringstream os;
		os << "Aspect ratio for resolution [" << user.video.resx << ", " << user.video.resy << "] not supported.";
		errorLog(os.str());
		exit(0);
	}

	setFilter(dsq_filter);

	useFrameBuffer = user.video.fbuffer;

#ifdef AQUARIA_DEMO
	developerKeys = 0;
#endif

	if (exists("unlockdeveloperkeys"))
		developerKeys = 1;

	if (isDeveloperKeys())
	{
		maxPages = 600/saveSlotPageSize;
	}

	if (mipmapsEnabled)
		debugLog("Mipmaps Enabled");
	else
		debugLog("Mipmaps Disabled");

	Texture::useMipMaps = mipmapsEnabled;

	if (isDeveloperKeys())
		debugLog("DeveloperKeys Enabled");
	else
		debugLog("DeveloperKeys Disabled");

	if (voiceOversEnabled)
		debugLog("VoiceOvers Enabled");
	else
		debugLog("VoiceOvers Disabled");


#ifdef _DEBUG
	if (!createWindow(800, 600, user.video.bits, false, "Aquaria"))
#else
	if (!createWindow(user.video.resx, user.video.resy, user.video.bits, user.video.full, "Aquaria"))
#endif
	{
		msg("Failed to create window");
		return;
	}

	/*
	debugLog("Setting Music Volume...");
		if (musicVolume != 0)
		{
			musicVolume = 0.9;
		}
		sound->setMusicVolume(musicVolume);
	debugLog("OK");
	*/

#ifdef BBGE_BUILD_SDL
	SDL_Init(SDL_INIT_VIDEO);
	if (fullscreen && !sdlVideoModeOK(user.video.resx, user.video.resy, user.video.bits))
	{
		// maybe we can force a sane resolution if SetVideoMode is going to fail...
		user.video.resx = 800;
		user.video.resy = 600;
		if (!sdlVideoModeOK(user.video.resx, user.video.resy, user.video.bits))
			fullscreen = false;  // last chance.
	}
#endif

	debugLog("Init Graphics Library...");
		initGraphicsLibrary(user.video.resx, user.video.resy, fullscreen, user.video.vsync, user.video.bits);
#ifdef BBGE_BUILD_WIDESCREEN
		core->enable2DWide(user.video.resx, user.video.resy);
#else
		core->enable2D(800, 600, 1);
#endif
		core->initFrameBuffer();
	debugLog("OK");

	setInputGrab(0);
	dsq->forceInputGrabOff();

	debugLog("Init Sound Library...");
		initSoundLibrary(user.audio.deviceName);
		debugLog("Set Voice Fader");
		sound->setVoiceFader(0.5);
		sound->event_playVoice.set(MakeFunctionEvent(DSQ, onPlayVoice));
		sound->event_stopVoice.set(MakeFunctionEvent(DSQ, onStopVoice));
		//sound->reverbKeyword = "naija_";
	debugLog("OK");

	debugLog("Init Input Library...");
		initInputLibrary();
	debugLog("OK");


	joystickMode = user.control.joystickEnabled;
	if (joystickMode)
	{
		debugLog("Init Joystick Library...");
			initJoystickLibrary();
			/*
			if (joystickEnabled)
				joystickOverrideMouse = true;
			*/
		debugLog("OK");
	}

	user.apply();

	/*

	sound->loadLocalSound("bbfloppy");

	Quad *disk = new Quad("bitblot/disk", Vector(400, 300));
	disk->alpha = 0;
	disk->alpha.interpolateTo(1, 0.5);
	disk->scale = Vector(0.6, 0.6);
	addRenderObject(disk, LR_HUD);

	debugLog("core->main");
	core->main(0.5);
	debugLog("end of core->main");

	disk->position.interpolateTo(Vector(400,560), 0.5);
	
	core->main(0.4);

	sound->playSfx("bbfloppy");

	core->main(0.1);

	*/
	

	/*
	loading = new Quad("loading", Vector(400,300));
	loading->followCamera = 1;
	loading->alpha = 0.01;
	addRenderObject(loading, LR_HUD);
	*/

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
	/*
	DebugFont *loadingFont = new DebugFont(6, "LOADING");
	loadingFont->alpha = 1;
	loadingFont->setAlign(DebugFont::ALIGN_CENTER);
	loadingFont->position = Vector(400,300) + loadShift;
	loadingFont->followCamera = 1;
	addRenderObject(loadingFont, LR_HUD);

	Vector shift(-12, 0);

	DebugFont *lbit1 = new DebugFont(32, "[");
	//lbit1->setAlign(DebugFont::ALIGN_CENTER);
	lbit1->position = Vector(100,300) + shift;
	lbit1->followCamera = 1;
	addRenderObject(lbit1, LR_HUD);

	DebugFont *lbit2 = new DebugFont(32, "]");
	//lbit2->setAlign(DebugFont::ALIGN_CENTER);
	lbit2->position = Vector(700,300) + shift;
	lbit2->followCamera = 1;
	addRenderObject(lbit2, LR_HUD);
	*/

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


#ifdef AQUARIA_BUILD_CONSOLE
	debugLog("Creating console");
	console = new DebugFont;
	//(&dsq->smallFont);
	{
		console->position = Vector(10 - virtualOffX,400);
		console->followCamera = 1;
		console->alpha = 0;
		//console->setAlign(ALIGN_LEFT);
		//console->setWidth(12);
		//console->setFontSize(12);
		console->setFontSize(6);
	}
	addRenderObject(console, LR_DEBUG_TEXT);
#else
	debugLog("NOT creating console (disabled in this build)");
#endif

	debugLog("1");

	if (isDeveloperKeys())
	{
		//cmDebug = new BitmapText(&dsq->font);
		cmDebug = new DebugFont();
		{
			//cmDebug->position = Vector(450,250);
			//cmDebug->position = Vector(500,350);
			cmDebug->position = Vector(20 - virtualOffX,50);
			cmDebug->followCamera = 1;
			cmDebug->alpha = 0;
			//cmDebug->setAlign(ALIGN_LEFT);
			//cmDebug->setWidth(12);
			//cmDebug->setFontSize(18);
			cmDebug->setFontSize(6);
		}
		addRenderObject(cmDebug, LR_DEBUG_TEXT);
	}

	debugLog("2");

	versionLabel = new BitmapText(&smallFont);
	{
	setVersionLabelText();
	//versionLabel->position = Vector(10 - core->getVirtualOffX(), 575);
	versionLabel->followCamera = 1;
	versionLabel->setAlign(ALIGN_LEFT);
	versionLabel->scale = Vector(0.7, 0.7);
	versionLabel->alphaMod = 0.75;
	versionLabel->alpha = 0;
	}
	addRenderObject(versionLabel, LR_REGISTER_TEXT);
	

	subbox = new Quad();
	subbox->position = Vector(400,580);
	subbox->alpha = 0;
	subbox->alphaMod = 0.7;
	subbox->followCamera = 1;
	subbox->autoWidth = AUTO_VIRTUALWIDTH;
	subbox->setHeight(40);
	subbox->color = 0;
	addRenderObject(subbox, LR_SUBTITLES);

	subtext = new BitmapText(&dsq->subsFont);
	//subtext->position = Vector(400,570);
	subtext->position = Vector(400,570);
	subtext->followCamera = 1;
	subtext->alpha = 0;
	subtext->setFontSize(14);
	subtext->setWidth(800);
	subtext->setAlign(ALIGN_CENTER);
	//subtext->setFontSize(12);
	addRenderObject(subtext, LR_SUBTITLES);

#ifdef BBGE_BUILD_ACHIEVEMENTS_INTERNAL
	achievement_box = new Quad();
	achievement_box->position = Vector(800,0);
	achievement_box->alpha = 0;
	achievement_box->alphaMod = 0.7;
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
#endif

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
	cutscene_text->setText("~Paused~");
	cutscene_text->position = Vector(400,300-16);
	cutscene_text->alpha.x = 0;
	cutscene_text->followCamera = 1;
	addRenderObject(cutscene_text, LR_SUBTITLES);

	cutscene_text2 = new BitmapText(&dsq->smallFont);
	cutscene_text2->setText("Press 'S' to Skip");
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
		cursorBlinker->scale = Vector(1.5, 1.5);
		cursorBlinker->scale.interpolateTo(Vector(2,2), 0.2, -1, 1, 1);
		cursorBlinker->alpha = 0;
		cursorBlinker->alphaMod = 0.5;
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

	screenTransition = 0;

	screenTransition = new AquariaScreenTransition();
	{
		screenTransition->position = Vector(400,300);
	}
	addRenderObject(screenTransition, LR_TRANSITION);

	debugLog("8");

	loadBit(LOAD_GRAPHICS2);

	debugLog("9");


	profRender = 0;
#ifdef Prof_ENABLED
	profRender = new ProfRender();
	profRender->alpha = 0;
	addRenderObject(profRender, LR_DEBUG_TEXT);
#endif

	//fpsText = new BitmapText(&dsq->smallFont);
	fpsText = new DebugFont;
	{
		fpsText->color = Vector(1,1,1);
		fpsText->setFontSize(6);
		//fpsText->position = Vector(10,15+200);
		fpsText->position = Vector(10 - virtualOffX,580);
		fpsText->position.z = 5;
		//fpsText->position += Vector(0,-200);
		//fpsText->setAlign(ALIGN_LEFT);
		fpsText->setText("FPS");
		fpsText->alpha= 0;
		//fpsText->followCamera = 1;
	}
	addRenderObject(fpsText, LR_DEBUG_TEXT);

	precacher.precacheList("data/precache.txt", loadBitForTexPrecache);

	setTexturePointers();

	loadBit(LOAD_TEXTURES);

	renderObjectLayers[LR_ENTITIES].startPass = -2;
	renderObjectLayers[LR_ENTITIES].endPass = 5;

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

	if (!Entity::blurShader.isLoaded())
	{
		//Entity::blurShader.load("data/shaders/stan.vert", "data/shaders/hoblur.frag");
	}

	setMousePosition(core->center);
	
	dsq->continuity.reset();

	loadBit(LOAD_FINISHED);

	/*
	sound->playSfx("defense", 0.5);
	sound->playSfx("visionwakeup");
	*/
#if defined(AQUARIA_FULL) || defined(AQUARIA_DEMO)
	float trans = 0.5;
	overlay->alpha.interpolateTo(1, trans);
	core->main(trans);
#endif
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

	setInputGrab(1);

	/*
	removeRenderObject(disk);
	disk = 0;
	*/

	bindInput();

#if defined(AQUARIA_FULL) || defined(AQUARIA_DEMO)
	enqueueJumpState("BitBlotLogo");
#else
	title();
#endif
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

void DSQ::toggleInputGrabPlat(bool on)
{
}

void DSQ::instantQuit()
{
	if (core->getCtrlState() && core->getAltState())
		Core::instantQuit();
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

void DSQ::setFilter(int ds)
{
	dsq_filter = ds;
	if (dsq_filter == 0)
	{
		Texture::filter = GL_LINEAR;
	}
	else if (dsq_filter == 2)
	{
		Texture::filter = GL_NEAREST;
	}
}

void DSQ::setStory()
{
	std::string flagString = getUserInputString("Enter Flag to Set", "0");
	int flag = 0;
	std::istringstream is(flagString);
	is >> flag;

	core->main(0.2);
	std::ostringstream os;
	os << dsq->continuity.getFlag(flag);
	flagString = getUserInputString("Enter Value to Set Flag To", os.str());
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
	/*
	static std::vector<int> lastRand;

	int r = -1;
	int c = 0;
	
	while (r == -1 && c < 8)
	{
		r = rand()%8;
		for (int i = 0; i < lastRand.size(); i++)
		{
			if (lastRand[i] == r)
			{
				r = -1;
				break;
			}
		}
		c++;
	}

	if (r == -1)
		r = rand()%8;

	std::ostringstream os;
	os << "r: " << r;
	debugLog(os.str());

	lastRand.push_back(r);

	if (lastRand.size() > 4)
		lastRand.resize(4);
	*/

	return r;
}

Vector DSQ::getNoteColor(int note)
{
	Vector noteColor;
	switch(note)
	{
	case 0:
		// light green
		noteColor = Vector(0.5, 1, 0.5);
	break;
	case 1:
		// blue/green
		noteColor = Vector(0.5, 1, 0.75);
	break;
	case 2:
		// blue
		noteColor = Vector(0.5, 0.5, 1);
	break;
	case 3:
		// purple
		noteColor = Vector(1, 0.5, 1);
	break;
	case 4:
		// red
		noteColor = Vector(1, 0.5, 0.5);
	break;
	case 5:
		// red/orange
		noteColor = Vector(1, 0.6, 0.5);
	break;
	case 6:
		// orange
		noteColor = Vector(1, 0.75, 0.5);
	break;
	case 7:
		// orange
		noteColor = Vector(1, 1, 0.5);
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

void DSQ::toggleInputMode()
{
	switch(inputMode)
	{
	case INPUT_MOUSE:
		setInputMode(INPUT_JOYSTICK);
	break;
	case INPUT_JOYSTICK:
		setInputMode(INPUT_MOUSE);
	break;
	}
}

void DSQ::setInputMode(InputMode mode)
{
	inputMode = mode;
	switch(inputMode)
	{
	case INPUT_JOYSTICK:
		core->joystickAsMouse = true;
		updateCursorFromMouse = false;
	break;
	case INPUT_MOUSE:
		setMousePosition(core->mouse.position);
		core->joystickAsMouse = false;
		updateCursorFromMouse = true;
	break;
	}
}

void DSQ::toggleRenderCollisionShapes()
{
	RenderObject::renderCollisionShape = !RenderObject::renderCollisionShape;
}

void DSQ::takeScreenshot()
{
	//takeScreenShot();
	doScreenshot = true;
	//saveScreenshotTGA("screen.tga");
}

void DSQ::unloadDevice()
{
	destroyFonts();

	Core::unloadDevice();
	darkLayer.unloadDevice();
}

void DSQ::reloadDevice()
{
	loadFonts();

	Core::reloadDevice();
	darkLayer.reloadDevice();

	recreateBlackBars();
}

#ifdef AQUARIA_BUILD_CONSOLE
void DSQ::toggleConsole()
{
	if (console)
	{
		if (console->alpha == 0)
		{
			console->alpha.interpolateTo(1, 0.1);
			cmDebug->alpha.interpolateTo(1, 0.1);
			fpsText->alpha.interpolateTo(1, 0.1);
			if (profRender)
				profRender->alpha.interpolateTo(1, 0.1);
			RenderObject::renderPaths = true;
		}
		else if (console->alpha == 1)
		{
			console->alpha.interpolateTo(0, 0.1);
			cmDebug->alpha.interpolateTo(0, 0.1);
			fpsText->alpha.interpolateTo(0, 0.1);
			if (profRender)
				profRender->alpha.interpolateTo(0, 0.1);
			RenderObject::renderPaths = false;
		}
	}
}

void DSQ::debugLog(const std::string &s)
{
	consoleLines.push_back(s);
	if (consoleLines.size() > MAX_CONSOLELINES)
	{
		//consoleLines.size()-MAX_CONSOLELINES
		for (int i = 0; i < consoleLines.size()-1; i++)
		{
			consoleLines[i] = consoleLines[i+1];
		}
		consoleLines.resize(MAX_CONSOLELINES);
	}
	if (console)
	{
		std::string text;
		for (int i = 0; i < consoleLines.size(); i++)
		{
			text += consoleLines[i] + '\n';
		}
		console->setText(text);
	}
	Core::debugLog(s);
}
#endif  // AQUARIA_BUILD_CONSOLE

int DSQ::getEntityTypeIndexByName(std::string s)
{
	for (int i = 0; i < game->entityTypeList.size(); i++)
	{
		EntityClass *t = &game->entityTypeList[i];
		if (t->name == s)
			return t->idx;
	}
	return -1;
}

void DSQ::toggleMuffleSound(bool toggle)
{
	/*
	if (sound->isPlayingMusic())
	{
	#ifdef BBGE_BUILD_WINDOWS
		if (toggle)
		{
			BASS_ChannelSetFX(sound->getMusicStream(), 1, BASS_FX_FLANGER);
			BASS_FXREVERB rev;
			rev.fHighFreqRTRatio = 0;
			rev.fInGain = 0;
			rev.fReverbMix = 0.001;
			rev.fReverbTime = 1000;
			BASS_FXSetParameters(sound->getMusicStream(), &rev);
		}
		else
			BASS_ChannelRemoveFX(sound->getMusicStream(), BASS_FX_DISTORTION);
	#endif

	}
	*/
}

void loadModsCallback(const std::string &filename, intptr_t param)
{
	//errorLog(filename);
	int pos = filename.find_last_of('/')+1;
	int pos2 = filename.find_last_of('.');
	std::string name = filename.substr(pos, pos2-pos);
	ModEntry m;
	m.path = name;
	dsq->modEntries.push_back(m);

	debugLog("Loaded ModEntry [" + m.path + "]");
}

void DSQ::startSelectedMod()
{
	ModEntry *e = getSelectedModEntry();
	if (e)
	{
		clearModSelector();
		mod.load(e->path);
		mod.start();

		/*
		if (dsq->game->sceneToLoad.empty())
		{
			mod.setActive(false);
		}
		*/
	}
}

void DSQ::selectNextMod()
{
	selectedMod ++;

	if (selectedMod >= modEntries.size())
		selectedMod = 0;

	if (modSelector)
		modSelector->refreshTexture();
}

void DSQ::selectPrevMod()
{
	selectedMod --;

	if (selectedMod < 0)
		selectedMod = modEntries.size()-1;

	if (modSelector)
		modSelector->refreshTexture();
}

ModEntry* DSQ::getSelectedModEntry()
{
	if (!modEntries.empty() && selectedMod >= 0 && selectedMod < modEntries.size())
		return &modEntries[selectedMod];
	return 0;
}

void DSQ::loadMods()
{
	modEntries.clear();
	
	forEachFile(mod.getBaseModPath(), ".xml", loadModsCallback, 0);
	selectedMod = 0;
}

void DSQ::playMenuSelectSfx()
{
	core->sound->playSfx("MenuSelect");
}

PlaySfx DSQ::calcPositionalSfx(const Vector &position, float maxdist)
{
	PlaySfx sfx;
	sfx.vol = 0;
	if (dsq->game && dsq->game->avatar)
	{
		Vector diff = position - dsq->game->avatar->position;

		// Aspect-ratio-adjustment:
		// Just multiplying the cut-off distance with aspect increases it too much on widescreen,
		// so only a part of it is aspect-corrected to make it sound better.
		// Aspect is most likely >= 5/4 here, which results in a higher value than
		// the default of 1024; this is intended. -- FG
		if (maxdist <= 0)
			maxdist = 724 + (300 * aspect);

		float dist = diff.getLength2D();
		if (dist < maxdist)
		{
			sfx.vol = 1.0f - (dist / maxdist);
			sfx.pan = (diff.x / maxdist) * 2.0f;
			if (sfx.pan < -1)
				sfx.pan = -1;
			if (sfx.pan > 1)
				sfx.pan = 1;
		}
	}
	return sfx;
}

void DSQ::playPositionalSfx(const std::string &name, const Vector &position, float f, float fadeOut)
{
	PlaySfx sfx = calcPositionalSfx(position);

	// FIXME: Right now, positional sound effects never update their relative position to the
	// listener, which means that if they are spawned too far away to be audible, it is not possible
	// that they ever get audible at all. Additionally, the current scripting API only provides
	// functions to fade sounds OUT, not to set their volume arbitrarily.
	// Because audio thread creation is costly, drop sounds that can not be heard.
	// This needs to be removed once proper audio source/listener positioning is implemented,
	// or the scripting interface gets additional functions to mess with sound. -- FG
	if (sfx.vol <= 0)
		return;

	sfx.freq = f;
	sfx.name = name;

	void *c = sound->playSfx(sfx);
	if (fadeOut != 0)
	{
		sound->fadeSfx(c, SFT_OUT, fadeOut);
	}
}

void DSQ::shutdown()
{
	scriptInterface.shutdown();
	precacher.clean();
	/*
	if (title)delete title;
	if (game) delete game;
	if (logo) delete logo;
	if (gameOver) delete gameOver;
	if (scLogo)	delete scLogo;
	if (introText)	delete introText;
	if (animationEditor) delete animationEditor;
	if (intro)		delete intro;
	*/

	core->particleManager->clearParticleBank();
	Shot::clearShotBank();


	cursor->setTexturePointer(0, RenderObject::NO_ADD_REF);

	UNREFTEX(texCursor);
	UNREFTEX(texCursorSwim);
	UNREFTEX(texCursorBurst);
	UNREFTEX(texCursorSing);
	UNREFTEX(texCursorLook);

#ifdef AQUARIA_BUILD_CONSOLE
	removeRenderObject(console);
	console = 0;
#endif
	removeRenderObject(cmDebug);
	cmDebug = 0;
	removeRenderObject(subtext);
	subtext = 0;
	removeRenderObject(subbox);
	subbox = 0;

#ifdef BBGE_BUILD_ACHIEVEMENTS_INTERNAL
	removeRenderObject(achievement_text);
	achievement_text = 0;
	removeRenderObject(achievement_box);
	achievement_box = 0;
#endif

	removeRenderObject(cursor);
	removeRenderObject(cursorGlow); // is this necessary? probably
	removeRenderObject(cursorBlinker);
	removeRenderObject(overlay);
	removeRenderObject(overlay2);
	removeRenderObject(overlay3);
	removeRenderObject(overlayRed);
	removeRenderObject(tfader);
	//removeRenderObject(messageLabelBG);
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

	if (profRender)
		removeRenderObject(profRender);

	if (screenTransition)
	{
		screenTransition->destroy();
		removeRenderObject(screenTransition);
	}

	destroyFonts();


	screenTransition = 0;

	core->main(0.1);
	overlay = 0;
	overlay2 = 0;
	overlay3 = 0;
	cursor = 0;
	tfader = 0;

	continuity.shutdown();

	//script.shutdown();
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
		cursor->setTexturePointer(texCursor, RenderObject::NO_ADD_REF);
}

void DSQ::setCursor(CursorType type)
{
	switch(type)
	{
	case CURSOR_NORMAL:
		cursor->setTexturePointer(texCursor, RenderObject::NO_ADD_REF);
	break;
	case CURSOR_LOOK:
		cursor->setTexturePointer(texCursorLook, RenderObject::NO_ADD_REF);
	break;
	case CURSOR_BURST:
		cursor->setTexturePointer(texCursorBurst, RenderObject::NO_ADD_REF);
	break;
	case CURSOR_SWIM:
		cursor->setTexturePointer(texCursorSwim, RenderObject::NO_ADD_REF);
	break;
	case CURSOR_SING:
		cursor->setTexturePointer(texCursorSing, RenderObject::NO_ADD_REF);
	break;
	default:
		cursor->setTexturePointer(texCursor, RenderObject::NO_ADD_REF);
	break;
	}
}

void DSQ::toggleEffects()
{
	/*
	static int tester = 0;
	float t = 3;
	if (!tester)
		sound->crossover("battletest-town", t);
	else
		sound->crossover("battletest-battle", t);
	tester = !tester;
	*/
	/*
	static int effectToggler = 0;
	effectToggler ++;
	switch (effectToggler)
	{
	case 1:
		postProcessingFx.enable(FXT_RADIALBLUR);
		postProcessingFx.radialBlurColor = Vector(1,1,1);
		postProcessingFx.intensity = 0.1;
	break;
	case 2:
		postProcessingFx.intensity = 0.05;
	break;
	case 3:
		postProcessingFx.intensity = 0.2;
	break;
	case 4:
		postProcessingFx.intensity = 0.4;
	break;
	default:
	case 0:
		effectToggler = 0;
		postProcessingFx.disable(FXT_RADIALBLUR);

	break;
	}
	*/
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
			q->alpha.data->path.addPathNode(0.5, 0.1);
			q->alpha.data->path.addPathNode(0.5, 0.5);
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
			float t = 0.2;
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
			q->alpha.data->path.addPathNode(0.5, 0.1);
			q->alpha.data->path.addPathNode(0.5, 0.5);
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

Entity *DSQ::getEntityByName(std::string name)
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
		//loaded = true;
		dsq->doScreenTrans = true;
	}
	else
	{
		//loaded = false;
		clearSaveSlots(true);
	}
}

void DSQ::doSavePoint(const Vector &position)
{
//	if (!e) return;

	dsq->game->avatar->setv(EV_LOOKAT, 0);
	core->sound->playSfx("MemoryCrystalActivate");

	Quad *glow = new Quad;
	{
		glow->setTexture("save-point-glow");
		glow->alpha = 0;
		glow->alpha.interpolateTo(0.5, 1, 1, -1, 1);
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
	dsq->game->avatar->myZoom.interpolateTo(Vector(1,1),0.5);
	// override =
	dsq->game->avatar->skeletalSprite.animate("save", 0, 3);
	dsq->game->clearControlHint();
	dsq->main(2);
	dsq->game->avatar->enableInput();
	dsq->game->avatar->revive();
	dsq->game->togglePause(1);
	dsq->doSaveSlotMenu(SSM_SAVE, position);
	dsq->game->togglePause(0);
	core->resetTimer();
	dsq->game->avatar->setv(EV_LOOKAT, 1);
	//dsq->game->avatar->skeletalSprite.animate("unsave", 0, 3);
	//dsq->continuity.saveFile(0);
}

void DSQ::playNoEffect()
{
	if (noEffectTimer <= 0)
	{
		sound->playSfx("noeffect", 0.9);
		noEffectTimer = 0.2;
	}
}

void DSQ::clearMenu(float t)
{
	for (int i = 0; i < menu.size(); i++)
	{
		menu[i]->setLife(1);
		menu[i]->setDecayRate(1/t);
		menu[i]->fadeAlphaWithLife = 1;
		//menu[i]->alpha.interpolateTo(0, 0.5);
	}
	menu.clear();
}

void DSQ::screenMessage(const std::string &msg)
{
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

	/*
	DebugFont *d = new DebugFont;
	d->position = Vector(400,300);
	d->setFontSize(16);
	d->setText(msg);
	d->alpha = 0;
	d->alpha.interpolateTo(1, 0.75);
	d->setLife(2);
	d->setDecayRate(1);
	d->followCamera = 1;
	core->getTopStateData()->addRenderObject(d, LR_DEBUG_TEXT);
	debugLog(msg);
	*/
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

	for (int i = 0; i < saveSlots.size(); i++)
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
#ifdef AQUARIA_DEMO
	nag(NAG_TOTITLE);
	return;
#endif

	modIsSelected = false;

	dsq->loadMods();
	
	selectedMod = user.data.lastSelectedMod;
	
	if (selectedMod >= modEntries.size() || selectedMod < 0)
		selectedMod = 0;

	createModSelector();

	resetTimer();

	inModSelector = true;

	main(-1);

	clearModSelector();
		
	if (modIsSelected)
	{
		user.data.lastSelectedMod = selectedMod;
		dsq->startSelectedMod();
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
	blackout->alphaMod = 0.75;
	blackout->alpha = 0;
	blackout->alpha.interpolateTo(1, 0.2);
	addRenderObject(blackout, LR_MENU);

	menu.resize(4);

	menu[0] = new Quad("Cancel", Vector(750,580));
	menu[0]->followCamera = 1;
	addRenderObject(menu[0], LR_MENU);


	AquariaMenuItem *a = new AquariaMenuItem();
	//menu[0]->setLabel("Cancel");
	a->useGlow("glow", 200, 50);
	a->event.set(MakeFunctionEvent(DSQ,onExitSaveSlotMenu));
	a->position = Vector(750, 580);
	addRenderObject(a, LR_MENU);
	menu[1] = a;
	AquariaMenuItem *m1 = a;


	a = new AquariaMenuItem();
	a->useQuad("gui/arrow-left");
	a->useGlow("glow", 100, 50);
	a->useSound("Click");
	a->event.set(MakeFunctionEvent(DSQ, selectPrevMod));
	a->position = Vector(150, 300);
	addRenderObject(a, LR_MENU);

	menu[2] = a;

	AquariaMenuItem *m2 = a;

	a = new AquariaMenuItem();
	a->useQuad("gui/arrow-right");
	a->useGlow("glow", 100, 50);
	a->useSound("Click");
	a->event.set(MakeFunctionEvent(DSQ, selectNextMod));
	a->position = Vector(650, 300);
	addRenderObject(a, LR_MENU);

	menu[3] = a;

	AquariaMenuItem *m3 = a;

	modSelector = new ModSelector();
	modSelector->position = Vector(400,300);
	modSelector->alpha = 0;
	modSelector->alpha.interpolateTo(1, 0.4);
	modSelector->followCamera = 1;
	addRenderObject(modSelector, LR_MENU);

	modSelector->setFocus(true);

	m2->setDirMove(DIR_RIGHT, modSelector);
	modSelector->setDirMove(DIR_RIGHT, m3);
	modSelector->setDirMove(DIR_LEFT, m2);
	m2->setDirMove(DIR_LEFT, modSelector);

	modSelector->setDirMove(DIR_DOWN, m1);
	m2->setDirMove(DIR_DOWN, m1);
	m3->setDirMove(DIR_DOWN, m1);

	m1->setDirMove(DIR_UP, modSelector);
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

	if (modSelector)
	{
		modSelector->setLife(1);
		modSelector->setDecayRate(2);
		modSelector->fadeAlphaWithLife = 1;
		modSelector = 0;
	}

	clearMenu();
}



void DSQ::updateSaveSlotPageCount()
{
	std::ostringstream os;
	os << "Page " << user.data.savePage+1 << "/" << maxPages+1;
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

	float t = 0.3;


	blackout = new Quad;
	blackout->color = 0;
	blackout->autoWidth = AUTO_VIRTUALWIDTH;
	blackout->autoHeight = AUTO_VIRTUALHEIGHT;
	blackout->followCamera = 1;
	blackout->position = Vector(400,300);
	blackout->alphaMod = 0.75;
	blackout->alpha = 0;
	blackout->alpha.interpolateTo(1, 0.5);
	addRenderObject(blackout, LR_MENU);


	menu[1] = new Quad("gui/save-menu", Vector(400,300));
	savesz = Vector(750.0f/1024.0f, 750.0f/1024.0f);
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
	//menu[0]->setLabel("Cancel");
	cancel->useGlow("glow", 200, 50);
	cancel->event.set(MakeFunctionEvent(DSQ,onExitSaveSlotMenu));
	cancel->position = Vector(665, 545); // 670
	addRenderObject(cancel, LR_MENU);

	menu[0] = cancel;

	/*
	menu[1] = new Quad("Cancel", Vector(750,580));
	menu[1]->followCamera = 1;
	addRenderObject(menu[1], LR_MENU);
	*/

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
		txt->setText(continuity.stringBank.get(2001));
	else
		txt->setText(continuity.stringBank.get(2000));
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

	main(1);
	
	resetTimer();
	
	if (fade)
		dsq->sound->stopMusic();

	user.save();
	
	if (mod.isActive())
	{
		mod.shutdown();
	}
	
	// VERY important
	dsq->continuity.reset();

	dsq->game->transitionToScene("Title");
}

void DSQ::createSaveSlotPage()
{
	for (int i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i]->safeKill();
	}

	saveSlots.resize(saveSlotPageSize);
	for (int i = 0; i < saveSlots.size(); i++)
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


	if (inputMode == INPUT_JOYSTICK)
	{
		saveSlots[0]->setDirMove(DIR_RIGHT, arrowUp);
		saveSlots[1]->setDirMove(DIR_RIGHT, arrowUp);
		saveSlots[2]->setDirMove(DIR_RIGHT, arrowDown);
		saveSlots[3]->setDirMove(DIR_RIGHT, cancel);
		arrowDown->setDirMove(DIR_DOWN, cancel);
		cancel->setDirMove(DIR_UP, arrowDown);
		cancel->setDirMove(DIR_LEFT, saveSlots[3]);
	}
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
	if (user.data.savePage < 0)
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
	float t = 0.3;
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

	for (int i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i]->close(trans);
	}

	saveSlots.clear();

	//watch(0.25);

	if (trans)
	{
		disableMiniMapOnNoInput = false;

		for (int i = 0; i < menu.size(); i++)
		{
			if (i != 1)
			{
				menu[i]->alpha = 0;
				//menu[i]->alpha.interpolateTo(0, 0.01);
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
	//watch(0.5);

	if (dsq->game->miniMapRender)
		dsq->game->miniMapRender->slide(0);
}

void DSQ::hideSaveSlots()
{
	for (int i = 0; i < saveSlots.size(); i++)
	{
		saveSlots[i]->hide();
	}
}

void DSQ::transitionSaveSlots()
{
	hideSaveSlotCrap();

	for (int i = 0; i < saveSlots.size(); i++)
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
	const int firstSaveSlot = user.data.savePage * saveSlotPageSize;
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

	//core->globalScale.interpolateTo(Vector(1, 1), 0.5);

	resetTimer();
	core->main(-1);



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
				std::string tempfile = dsq->getSaveDirectory() + "/poot-s.tmp";

				//saveCenteredScreenshotTGA(tempfile, scrShotWidth);
				//saveSizedScreenshotTGA(tempfile,512,1);

				// Cut off top and bottom to get a 4:3 aspect ratio.
				int adjHeight = (scrShotWidth * 3.0f) / 4.0f;
				int imageDataSize = scrShotWidth * scrShotHeight * 4;
				int adjImageSize = scrShotWidth * adjHeight * 4;
				int adjOffset = scrShotWidth * ((scrShotHeight-adjHeight)/2) * 4;
				memmove(scrShotData, scrShotData + adjOffset, adjImageSize);
				memset(scrShotData + adjImageSize, 0, imageDataSize - adjImageSize);
				tgaSave(tempfile.c_str(), scrShotWidth, scrShotHeight, 32, scrShotData);
				scrShotData = 0;  // deleted by tgaSave()

				packFile(dsq->getSaveDirectory() + "/poot-s.tmp", os.str(),9);
				remove((dsq->getSaveDirectory() + "/poot-s.tmp").c_str());
			}

			PlaySfx sfx;
			sfx.name = "saved";
			sfx.vol = 0.55;
			dsq->sound->playSfx(sfx);
			confirm("", "saved", 1);

			clearSaveSlots(true);
		}
		else if (saveSlotMode == SSM_LOAD)
		{
			continuity.loadFile(selectedSaveSlot->getSlotIndex());
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
	const float t = 0.3;

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
	//bgLabel->setWidthHeight(512*0.9f, 256*0.9f);
	bgLabel->scale = Vector(0.5, 0.5);
	bgLabel->scale.interpolateTo(Vector(1,1), t);
	addRenderObject(bgLabel, LR_CONFIRM);

	dsq->main(t);

	float t2 = 0.05;

	/*
	Quad *yes = new Quad("gui/yes", Vector(350, 400));
	yes->followCamera = 1;
	yes->alpha = 0;
	yes->alpha.interpolateTo(1, t2);
	addRenderObject(yes, LR_CONFIRM);

	Quad *no = new Quad("gui/no", Vector(450, 400));
	no->followCamera = 1;
	no->alpha = 0;
	no->alpha.interpolateTo(1, t2);
	addRenderObject(no, LR_CONFIRM);
	*/

	const int GUILEVEL_CONFIRM = 200;

	AquariaGuiElement::currentGuiInputLevel = GUILEVEL_CONFIRM;

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
	txt->scale = Vector(0.9, 0.9);
	txt->alpha.interpolateTo(1, t2);
	addRenderObject(txt, LR_CONFIRM);

	dsq->main(t2);

	while (!confirmDone)
	{
		dsq->main(FRAME_TIME);
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
	dsq->main(t2);

	bgLabel->alpha.interpolateTo(0, t);
	bgLabel->scale.interpolateTo(Vector(0.5, 0.5), t);
	dsq->main(t);

	bgLabel->safeKill();
	txt->safeKill();
	if (yes)	yes->safeKill();
	if (no)		no->safeKill();

	bool ret = (confirmDone == 1);

	if (countdown < 0)
		ret = false;

	AquariaGuiElement::currentGuiInputLevel = 0;

	return ret;
}

std::string DSQ::getUserInputString(std::string labelText, std::string t, bool allowNonLowerCase)
{
	float trans = 0.1;

	bool pauseState = dsq->game->isPaused();

	dsq->game->togglePause(true);

	sound->playSfx("Menu-Open");

	RoundedRect *bg = new RoundedRect;
	bg->setWidthHeight(790, 64, 10);
	bg->position = Vector(400,300);
	bg->followCamera = 1;
	bg->alpha = 0;
	addRenderObject(bg, LR_DEBUG_TEXT);

	/*
	DebugFont *label = new DebugFont();
	label->setFontSize(10);
	label->setText(labelText);
	label->position = Vector(20,250 - label->getNumLines()*10);
	label->followCamera = 1;
	label->alpha = 0;
	label->alpha.interpolateTo(1, trans);
	*/

	TTFText *label = new TTFText(&dsq->fontArialSmall);
	label->setText(labelText);
	label->position = Vector(-400 + 20, -12); //- label->getNumLines()*10
	bg->addChild(label, PM_POINTER);

	TTFText *inputText = new TTFText(&dsq->fontArialBig);
	//inputText->setFontSize(14);
	inputText->position = Vector(-400 + 20,8+8);
	bg->addChild(inputText, PM_POINTER);
	//addRenderObject(inputText, LR_DEBUG_TEXT);

	bg->show();
	main(trans);


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
		if (inputText->getWidth() < 800-60)
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
		core->main(dt);
	}

	if (blink && !text.empty() && (text[text.size()-1] == '|'))
		text.resize(text.size()-1);

	sound->playSfx("Menu-Close");

	/*
	inputText->offset.interpolateTo(Vector(800, 0), 0.2);
	label->offset.interpolateTo(Vector(800, 0), 0.2);
	bg->offset.interpolateTo(Vector(800, 0), 0.2);
	*/
	bg->hide();

	main(0.2);

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
	for (int i = 0; i < dsq->continuity.voiceOversPlayed.size(); i++)
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
	/*
	voxQueue.clear();
	if (streamingVoice)
	{
		streamingVoice = false;
		if (stream)
		{
			BASS_ChannelSlideAttributes(stream, -1, -2, -101, 500);
			stream = 0;
		}
	}
	voice(f);
	*/
}

/*
void DSQ::updateVoiceVolume()
{
	if (streamingVoice)
	{
		BASS_ChannelSlideAttributes(stream, -1, user.audio.voxvol*100.0f, -101, 100);
	}
}
*/

void DSQ::onPlayVoice()
{
	/*
	if (user.audio.subtitles)
	{
		std::string fn = "scripts/vox/" + sound->lastVoice + ".txt";
		std::ifstream inf(fn.c_str());
		if (inf.is_open())
		{
			std::string dia;
			std::getline(inf, dia);

			if (!dia.empty())
				hint(dia);
		}
		inf.close();
	}
	*/
}

void DSQ::onStopVoice()
{
	subtitlePlayer.end();
	/*
	if (user.audio.subtitles)
	{
		hint("");
	}
	*/
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

Vector DSQ::getUserInputDirection(std::string labelText)
{
	BitmapText *label = new BitmapText(&dsq->font);
	label->setFontSize(16);
	label->position = Vector(400,200);
	label->followCamera = 1;
	label->setText(labelText);
	addRenderObject(label, LR_HUD);

	while (core->getKeyState(KEY_RETURN))
	{
		core->main(1.0f/30.0f);
	}
	Vector v;
	while (1)
	{
		v.x = v.y = 0;
		if (core->getKeyState(KEY_LEFT))		v.x = -1;
		if (core->getKeyState(KEY_RIGHT))		v.x = 1;
		if (core->getKeyState(KEY_UP))			v.y = -1;
		if (core->getKeyState(KEY_DOWN))		v.y = 1;
		if (core->getKeyState(KEY_RETURN))
			break;
		core->main(1.0f/30.0f);
	}
	label->alpha = 0;
	label->safeKill();
	return v;
}

std::string DSQ::getDialogueFilename(const std::string &f)
{
	return "dialogue/" + languagePack + "/" + f + ".txt";
}

void DSQ::jumpToSection(std::ifstream &inFile, const std::string &section)
{
	if (section.empty()) return;
	std::string file = dsq->getDialogueFilename(dialogueFile);
	if (!exists(file))
	{
		debugLog("Could not find dialogue [" + file + "]");
		return;
	}
	inFile.open(core->adjustFilenameCase(file).c_str());
	std::string s;
	while (std::getline(inFile, s))
	{
		if (!s.empty())
		{
			if (s.find("[")!=std::string::npos && s.find(section) != std::string::npos)
			{
				return;
			}
		}
	}
	debugLog("could not find section [" + section + "]");
}


void DSQ::runGesture(const std::string &line)
{
	std::istringstream is(line);
	std::string target;
	is >> target;
	debugLog("Gesture: " + line);
	if (target == "entity")
	{
		std::string entName;
		is >> entName;
		Entity *e = getEntityByName(entName);
		if (e)
		{
			std::string cmd;
			is >> cmd;
			if (cmd=="anim" || cmd=="animate")
			{
				std::string anim;
				is >> anim;
				int loop = 0;
				int group = 0;
				if (anim == "idle")
				{
					e->skeletalSprite.stopAllAnimations();
					loop = -1;
				}
				if (line.find("upperBody")!=std::string::npos)
				{
					group = 1;
				}
				if (line.find("loop")!=std::string::npos)
				{
					loop = -1;
				}
				if (line.find("stopAll")!=std::string::npos)
				{
					e->skeletalSprite.stopAllAnimations();
				}
				e->skeletalSprite.transitionAnimate(anim, 0.2, loop, group);
			}
			else if (cmd == "moveToNode")
			{
				std::string node;
				is >> node;
				Path *p = dsq->game->getPathByName(node);
				e->moveToNode(p, 0);
			}
		}
	}
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
				//limitRange = core->mouse.buttons.left;
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
		//cursor->renderQuad = false;
		if (game->miniMapRender)
			game->miniMapRender->offset = Vector(2000,0);
		if (fpsText)
			fpsText->offset = Vector(2000,0);
	}
	else
	{
		//cursor->renderQuad = true;
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

	float t = 0.1;

	dsq->game->miniMapRender->toggle(0);

	fade(1, t);
	main(t);

	// load images
	typedef std::list<Quad*> QuadList;
	QuadList images;

	for (int i = num-1; i >= 0; i--)
	{
		Quad *q = new Quad;
		std::string label = "visions/"+folder+"/"+numToZeroString(i, 2)+".png";
		//debugLog(label);
		q->setTexture(label);
		/*
		if (q->getWidth() == q->getHeight())
			q->setWidthHeight(800,800);
		else
			q->setWidthHeight(800,600);
		*/
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

		(*i)->scale.interpolateTo(Vector(1.1,1.1), 0.4);
		fade(0, t);
		main(t);

		main(0.1);

		fade(1, t);
		main(t);

		(*i)->alpha = 0;
	}

	if (game)
		game->togglePause(false);
	dsq->toggleCursor(true);

	sound->playSfx("memory-flash");
	fade(0, t);
	main(t);

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
	///HACK TEMPORARY
	//return true;

#if !defined(AQUARIA_FULL) && !defined(AQUARIA_DEMO)
	return true;
#endif

#ifdef AQUARIA_DEMO
	return false;
#endif
#ifdef AQUARIA_FULL
	return false;
#endif

	return developerKeys;
}

bool DSQ::isQuitFlag()
{
	return watchQuitFlag;
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
		core->main(t);
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

void DSQ::action(int id, int state)
{
	Core::action(id, state);

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
}

void DSQ::bindInput()
{
	clearActions();

	almb = user.control.actionSet.getActionInputByName("lmb");
	armb = user.control.actionSet.getActionInputByName("rmb");

	user.control.actionSet.importAction(this, "Escape",		ACTION_ESC);

#if defined(BBGE_BUILD_MACOSX)
	addAction(MakeFunctionEvent(DSQ, instantQuit), KEY_Q, 1);
#endif
#if defined(BBGE_BUILD_WINDOWS) || defined(BBGE_BUILD_UNIX)
	addAction(MakeFunctionEvent(DSQ, onSwitchScreenMode), KEY_RETURN, 1);
	//addAction(MakeFunctionEvent(DSQ, onAltTab), KEY_TAB, 0);
#endif
	if (isDeveloperKeys())
	{
#if defined(BBGE_BUILD_WINDOWS) || defined(BBGE_BUILD_UNIX)
		addAction(MakeFunctionEvent(DSQ, instantQuit), KEY_Q, 1);
#ifdef AQUARIA_BUILD_CONSOLE
		addAction(MakeFunctionEvent(DSQ, toggleConsole), KEY_TILDE, 0);
#endif
#endif
		addAction(MakeFunctionEvent(DSQ, toggleRenderCollisionShapes), KEY_CAPSLOCK, 0);
	}
	addAction(MakeFunctionEvent(DSQ, debugMenu), KEY_BACKSPACE, 0);
#if BBGE_BUILD_SDL
	addAction(MakeFunctionEvent(DSQ, takeScreenshot		),		KEY_PRINTSCREEN,	0);
#endif
	addAction(MakeFunctionEvent(DSQ, takeScreenshotKey	),		KEY_P,				0);
}

void DSQ::jiggleCursor()
{
#ifdef BBGE_BUILD_SDL
	// hacky
	SDL_ShowCursor(SDL_ENABLE);
	SDL_ShowCursor(SDL_DISABLE);
#endif
}

float skipSfxVol = 1.0;
void DSQ::onUpdate(float dt)
{
	/*
	if (hintTimer > 0)
	{
		hintTimer -= dt;
		if (hintTimer <= 0)
			closeHint();
	}
	*/

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
				pollEvents();
				ActionMapper::onUpdate(sec);
#ifdef BBGE_BUILD_SDL
				SDL_Delay(int(sec*1000));
#endif
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


	mod.update(dt);

	lockMouse();

	if (dsq->game && watchForQuit && isNested())
	{
		if (dsq->game->isActing(ACTION_ESC))
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

	if (menuSelectDelay > 0)
	{
		menuSelectDelay -= dt;
		if (menuSelectDelay <= 0)
		{
			menuSelectDelay = 0;
		}
	}

	if (noEffectTimer > 0)
	{
		noEffectTimer -=dt;
		if (noEffectTimer < 0)
			noEffectTimer = 0;
	}


	subtitlePlayer.update(dt);

	
	Core::onUpdate(dt);

	demo.update(dt);

	// HACK: not optimal
	
	if (inputMode != INPUT_KEYBOARD && game->isActive())
	{
		if (almb && (ActionMapper::getKeyState(almb->key[0]) || ActionMapper::getKeyState(almb->key[1])))
			mouse.buttons.left = DOWN;

		if (armb && (ActionMapper::getKeyState(armb->key[0]) || ActionMapper::getKeyState(armb->key[1])))
			mouse.buttons.right = DOWN;
	}

	if (joystickAsMouse)
	{
		if (almb && ActionMapper::getKeyState(almb->joy[0]))
			mouse.buttons.left = DOWN;

		if (armb && ActionMapper::getKeyState(armb->joy[0]))
			mouse.buttons.right = DOWN;

		/*
		if (routeShoulder)
		{
		if (joystick.leftTrigger > 0.7)
		mouse.buttons.left = DOWN;
		else if (joystick.leftShoulder)
		mouse.buttons.left = DOWN;
		}
		*/

		/*
		if (joystick.buttons[1])
		mouse.buttons.right = DOWN;
		if (routeShoulder)
		{
		if (joystick.rightTrigger > 0.7)
		mouse.buttons.right = DOWN;
		else if (joystick.rightShoulder)
		mouse.buttons.right = DOWN;
		}
		*/

		// not going to happen anymore!
		// bye, bye xbox360 controller
		if (!mouse.buttons.middle)
		{
			if (joystick.rightThumb)
				mouse.buttons.middle = DOWN;
			else if (joystick.leftThumb)
				mouse.buttons.middle = DOWN;
		}

		/*
		if (!mouse.buttons.middle)
			mouse.buttons.middle = joystick.buttons[4];
		*/
		//|| (game->avatar && game->avatar->isSinging())
		/*
		if (core->getTopStateObject() != game || game->isPaused()  || (game->avatar && game->avatar->getState() == Entity::STATE_TITLE))
			core->updateCursorFromJoystick(dt, user.control.joyCursorSpeed);
		else
		{
			core->mouse.position = Vector(400,300);
		}
		*/

		//core->mouse.position = Vector(400,300);
	}

	if (joystickEnabled)
	{
		//if (!dsq->game->isInGameMenu())
		{
			if (dsq->inputMode != INPUT_JOYSTICK)
			{
				//if (!core->joystick.position.isZero() || !core->joystick.rightStick.isZero())
				const float thresh = 0.6;
				if (core->joystick.anyButton() || !core->joystick.position.isLength2DIn(thresh) || !core->joystick.rightStick.isLength2DIn(thresh))
				{
					//debugLog("setting joystick input mode");
					dsq->setInputMode(INPUT_JOYSTICK);
				}
			}
			else if (dsq->inputMode != INPUT_MOUSE)
			{
				///if (core->mouse.change.getLength2D() > core->joystick.position.getLength2D())
				//if (!core->mouse.change.isZero())
				if ((!core->mouse.change.isLength2DIn(5) || (core->getMouseButtonState(0) || core->getMouseButtonState(1))) && !core->joystick.anyButton())
				{
					//debugLog("setting mouse input mode");
					dsq->setInputMode(INPUT_MOUSE);
				}
			}
		}
	}
	if (dsq->game->avatar)
	{
		if (dsq->game->avatar->isActing(ACTION_SWIMUP) ||
			dsq->game->avatar->isActing(ACTION_SWIMDOWN) ||
			dsq->game->avatar->isActing(ACTION_SWIMLEFT) ||
			dsq->game->avatar->isActing(ACTION_SWIMRIGHT))
		{
			dsq->setInputMode(INPUT_KEYBOARD);
		}
	}
	
	// check the actual values, since mouse.buttons.left might be overwritten by keys
	int cb = 0;
	if (user.control.flipInputButtons)
		cb = 1;
	
	if (dsq->inputMode == INPUT_KEYBOARD && (core->getMouseButtonState(cb)))
	{
		dsq->setInputMode(INPUT_MOUSE);
	}

	if (isDeveloperKeys())
	{
		if (core->getCtrlState())
		{
			if (core->getKeyState(KEY_LEFT))
				core->adjustWindowPosition(-5, 0);
			if (core->getKeyState(KEY_RIGHT))
				core->adjustWindowPosition(5, 0);
			if (core->getKeyState(KEY_UP))
				core->adjustWindowPosition(0, -5);
			if (core->getKeyState(KEY_DOWN))
				core->adjustWindowPosition(0, 5);
		}
	}

	if (isDeveloperKeys() && cmDebug && cmDebug->alpha == 1 && fpsText)
	{
		std::ostringstream os;
		/*
		os << "id: " << continuity.cm.id << " - ";
		os << "ego: " << continuity.cm.ego << " - ";
		os << "sEgo: " << continuity.cm.superEgo << " - ";
		os << "cd: " << continuity.cm.getCommonDemoninator();
		os << std::endl;
		*/
		/*
		os << "id.p: " << continuity.cm.getPercentID() << std::endl;
		os << "ego.p: " << continuity.cm.getPercentEGO() << std::endl;
		os << "sEgo.p: " << continuity.cm.getPercentSEGO() << std::endl;
		*/
		if (dsq->game->avatar)
		{
			Avatar *avatar = dsq->game->avatar;
			os << "rolling: " << dsq->game->avatar->isRolling() << " rollDelay: " << dsq->game->avatar->rollDelay << std::endl;
			os << "canChangeForm: " << dsq->game->avatar->canChangeForm << std::endl;
			os << "h: " << dsq->game->avatar->health << " / " << dsq->game->avatar->maxHealth << std::endl;
			os << "biteTimer: " << dsq->game->avatar->biteTimer << " flourTimer: " << dsq->game->avatar->flourishTimer.getValue() << std::endl;
			os << "stillTimer: " << dsq->game->avatar->stillTimer.getValue() << std::endl;
			os << "hp: " << dsq->game->avatar->getHealthPerc() << " flourishPowerTimer: " << dsq->game->avatar->flourishPowerTimer.getValue() << std::endl;
			os << "maxSpeed: " << dsq->game->avatar->currentMaxSpeed << " - ";
			os << "lockedToWall: " << dsq->game->avatar->state.lockedToWall;
			os << std::endl;
			os << "crwlng: " << avatar->state.crawlingOnWall;
			os << " swmng: " << avatar->isSwimming();
			os << " dualFormCharge: " << continuity.dualFormCharge;
			os << std::endl;
			os << "vel(" << avatar->vel.x << ", " << avatar->vel.y << ") ";
			os << "vel2(" << avatar->vel2.x << ", " << avatar->vel2.y << ")";
			os << std::endl;
			os << "rot: " << avatar->rotation.z << " rotoff: " << avatar->rotationOffset.z << std::endl;
			os << "p(" << int(avatar->position.x) << ", " << int(avatar->position.y) << ")" << std::endl;
			os << "inp: " << avatar->isInputEnabled() << std::endl;
			os << "wallNormal(" << avatar->wallNormal.x << ", " << avatar->wallNormal.y << ")" << std::endl;
			os << "burst: " << avatar->burst << " burstTimer: " << avatar->burstTimer << std::endl;
			os << "inTummy: " << avatar->inTummy << " tummyAmount: " << avatar->tummyAmount << std::endl;
			os << "inCurrent: " << avatar->isInCurrent() << std::endl;
			os << "qsongCastDelay: " << avatar->quickSongCastDelay << std::endl;
			os << "singing: " << dsq->game->avatar->singing << " blockSinging: " << dsq->game->avatar->isBlockSinging();
			os << " look: " << dsq->game->avatar->state.updateLookAtTime << " ";

			os << "inputMode: ";
			switch(dsq->inputMode)
			{
			case INPUT_MOUSE:
				os << "mouse";
			break;
			case INPUT_JOYSTICK:
				os << "joystick";
			break;
			}
			os << std::endl;
			Bone *b = dsq->game->avatar->skeletalSprite.getBoneByIdx(1);
			if (b)
				os << " headRot: " << b->rotation.z;
			os << std::endl;
			os << "fh: " << dsq->game->avatar->isfh() << " fv: " << dsq->game->avatar->isfv() << std::endl;
		}

		// DO NOT CALL AVATAR-> beyond this point
		os << "story: " << continuity.getStory() << std::endl;
		os << "invGlobalScale: " << core->invGlobalScale;
		os << std::endl;
		os << "globalScale: " << core->globalScale.x << std::endl;
		os << "mousePos:(" << core->mouse.position.x << ", " << core->mouse.position.y << ") mouseChange:(" << core->mouse.change.x << ", " << core->mouse.change.y << ")\n";
		os << "joyStick:(" << core->joystick.position.x << ", " << core->joystick.position.y << ")\n";
		os << "altState: " << core->getKeyState(KEY_LALT) << " | " << core->getKeyState(KEY_RALT) << std::endl;
		os << "PMFree: " << particleManager->getFree() << " Active: " << particleManager->getNumActive() << std::endl;
		os << "cameraPos: (" << dsq->cameraPos.x << ", " << dsq->cameraPos.y << ")" << std::endl;
		os << "voiceTime: " << dsq->sound->getVoiceTime() << " bNat: " << dsq->game->bNatural;
		int ca, ma;
		dsq->sound->getStats(&ca, &ma);
		os << " ca: " << ca << " ma: " << ma << std::endl;
		os << dsq->sound->getVolumeString() << std::endl;
		os << core->globalResolutionScale.x << ", " << core->globalResolutionScale.y << std::endl;
		os << "Lua mem: " << scriptInterface.gcGetStats() << " KB" << std::endl;

		cmDebug->setText(os.str());
	}

	if (isDeveloperKeys() && fpsText && cmDebug && cmDebug->alpha == 1)
	{
		std::ostringstream os;
		os << "FPS: " << core->fps << " | ROC: " << core->renderObjectCount;
		os << " | p: " << core->processedRenderObjectCount << " | t: " << core->totalRenderObjectCount;
		os << " | s: " << dsq->continuity.seconds;
		os << " | evQ: " << core->eventQueue.getSize();
		/*
		os << " | s: " << dsq->continuity.seconds;
		os << " cr: " << core->cullRadius;
		os << " r: " << core->redBits << " g: " << core->greenBits << " b: " << core->blueBits;
		os << " a: " << core->alphaBits;
		*/
		//<< "(" << core->mouse.position.x << ", " << core->mouse.position.y << ")";
		// << "|"
		//os << float(1/dt) << " Instant";
		fpsText->setText(os.str());
	}

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
	
	static int lastWidth = 0;
	static int lastHeight = 0;
	if (lastWidth != width || lastHeight != height) {
		setInpGrab = -1;
	}
	lastWidth = width;
	lastHeight = height;
	
	static bool lastfullscreen = false;
	
	if (lastfullscreen != _fullscreen)
	{
		setInpGrab = -1;
	}
	lastfullscreen = _fullscreen;
	
	if (game && game->avatar && game->avatar->isInputEnabled() && !game->isPaused() && !game->isInGameMenu())
	{
		//debugLog("enabled");
		if (setInpGrab != 1)
		{
			toggleInputGrabPlat(true);
			setInpGrab = 1;
		}
	}
	else
	{
		//debugLog("not enabled");
		if (setInpGrab != 0)
		{
			toggleInputGrabPlat(false);
			setInpGrab = 0;
		}
	}
	
	


	updatepecue(dt);


	lockMouse();
}

void DSQ::lockMouse()
{
	/*
	if (core->getVirtualWidth() > 800 && core->mouse.position.x >= 800)
	{
		core->mouse.position.x = 800;
		core->setMousePosition(core->mouse.position);
	}
	*/
}

void DSQ::shakeCamera(float mag, float time)
{
	cameraOffset = Vector(0,0);
	shakeCameraMag = mag;
	shakeCameraTimer = time;
}

bool DSQ::isShakingCamera()
{
	return (shakeCameraTimer > 0);
}

bool DSQ::isScriptRunning()
{
	if (nestedMains>1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void DSQ::delay(float dt)
{
	core->main(dt);
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
		t = 0.1;
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
		/*
		dsq->game->sceneColor2 = Vector(1,1,1);
		dsq->game->sceneColor2.interpolateTo(Vector(1,0.7,0.7), 1.2, 1, 1);
		*/

		//dsq->game->avatar->damage(1);
		core->sound->playSfx("ShockWave");
		//dsq->spawnParticleEffect("ShockWave", position);
		float t =1.0;

		Quad *q = new Quad;
		q->position = position;
		q->scale = Vector(0,0);
		q->scale.interpolateTo(Vector(5,5),t);
		/*
		q->color = Vector(1,1,1);
		q->color.interpolateTo(Vector(1,0,0),t-t*0.05f);
		*/
		q->alpha.ensureData();
		q->alpha.data->path.addPathNode(0, 0);
		q->alpha.data->path.addPathNode(0.75, 0.25);
		q->alpha.data->path.addPathNode(0.75, 0.75);
		q->alpha.data->path.addPathNode(0, 1);
		q->alpha.startPath(t);
		q->setBlendType(RenderObject::BLEND_ADD);
		q->setTexture("particles/EnergyRing");
		if (target)
			q->positionSnapTo = &target->position;
		//q->rotation.interpolateTo(Vector(0,0,360), t+0.1f);
		game->addRenderObject(q, LR_PARTICLES);

		if (target && target->getEntityType() == ET_AVATAR)
			if (core->afterEffectManager)
				core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08,0.05,22,0.2f, 1.2));

		t = 0.75;
		{
			Quad *q = new Quad;
			q->position = position;
			q->scale = Vector(0.5,0.5);
			q->scale.interpolateTo(Vector(2,2),t);
			q->alpha.ensureData();
			q->alpha.data->path.addPathNode(0, 0);
			q->alpha.data->path.addPathNode(0.75, 0.25);
			q->alpha.data->path.addPathNode(0.75, 0.75);
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
		q->alpha.data->path.addPathNode(1, 0.3);
		//q->alpha.data->path.addPathNode(0.75, 0.75);
		q->alpha.data->path.addPathNode(0, 1);
		q->alpha.startPath(t);
		q->setBlendType(RenderObject::BLEND_ADD);
		q->rotation.z = rand()%360;
		q->setTexture("particles/EnergyRing");
		/*
		if (target)
			q->positionSnapTo = &target->position;
		*/
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
			q->alpha.data->path.addPathNode(0.8, 0.25);
			//q->alpha.data->path.addPathNode(0.75, 0.75);
			q->alpha.data->path.addPathNode(0, 1);
			q->alpha.startPath(t);
			q->setBlendType(RenderObject::BLEND_ADD);
			/*
			if (target)
				q->positionSnapTo = &target->position;
			*/
			q->setTexture("particles/EnergyDeltas");
			q->rotation.z = rand()%360;
			//q->rotation.interpolateTo(Vector(0,0,-360), t+0.1f);
			game->addRenderObject(q, LR_PARTICLES);
		}
	}
	break;
	case VFX_RIPPLE:
		if (core->afterEffectManager)
			core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.04,0.06,15,0.2f));
	break;
	/*
	case VFX_BIGRIPPLE:
	break;
	*/
	}
}

// get the closest, active, in range element to the vector
/*
Element *DSQ::getElementAtVector(const Vector &vec)
{
	Element *returnElement = 0;
	int smallestDistance = 9999;
	for (int i = 0; i < elements.size(); i++)
	{
		if (elements[i]->isActive() && elements[i]->getTotalInteractions() > 0)
		{
			int distanceToElementI = elements[i]->getFakeDistanceFromCenterToVector(vec);
			if (elements[i]->isVectorInActivationRange(vec) && distanceToElementI < smallestDistance)
			{
				smallestDistance = distanceToElementI;
				returnElement = elements[i];
			}
		}
	}
	return returnElement;
}
*/

Element *DSQ::getElementWithType(Element::Type type)
{
	for (int i = 0; i < elements.size(); i++)
	{
		if (elements[i]->getElementType() == type)
		{
			return elements[i];
		}
	}
	return 0;
}

/*
Element *DSQ::getClosestElementWithType(Element::Type type, Element *e)
{
	Element *returnElement = 0;
	long smallestDistance = 999999;
	for (int i = 0; i < elements.size(); i++)
	{
		if (elements[i]->isActive() && elements[i]->getElementType() == type)
		{
			long distanceToElementI = elements[i]->getFakeDistanceFromCenterToVector(e->position);
			if (distanceToElementI < smallestDistance)
			{
				smallestDistance = distanceToElementI;
				returnElement = elements[i];
			}
		}
	}
	return returnElement;
}
*/

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
			dt *= 4;
		else if (core->getKeyState(KEY_F))
		{
			if (core->getShiftState())
				dt *= 0.1f;
			else
				dt *= 0.6;
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
	for (int i = 0; i < dsq->elements.size(); i++)
	{
		if (dsq->elements[i] == element)
		{
			removeElement(i);
			break;
		}
	}

}
// only happens in editor, no need to optimize
void DSQ::removeElement(int idx)
{
	ElementContainer copy = elements;
	clearElements();
	int i = 0;
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
	int i;
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
		//for (std::vector<PECue>::iterator i = pecue.begin(); i != pecue.end(); i++)
		int nz = 0;
		for (int i = 0; i < pecue.size(); i++)
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
	this->alpha = 0;
	InterpolatedVector oldAlpha = dsq->cursor->alpha;
	dsq->cursor->alpha.x = 0;
	int width=0, height=0;
	core->render();

	width = core->getWindowWidth();
	height = core->getWindowHeight();

#ifdef BBGE_BUILD_OPENGL
	glBindTexture(GL_TEXTURE_2D,screen_texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
#endif


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

void pauseSound()
{
	if (dsq && dsq->sound) {
		dsq->sound->pause();
	}
}

void resumeSound()
{
	if (dsq && dsq->sound) {
		dsq->sound->resume();
	}
}
