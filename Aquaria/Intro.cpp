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
#include "States.h"
#include "../BBGE/AfterEffect.h"
#include "Gradient.h"


//class Mappy;

namespace IntroStuff
{
	//Mappy *m;
	bool quitFlag;
};

using namespace IntroStuff;

/*
class Mappy : public RenderObject, public ActionMapper
{
public:
	Mappy() : RenderObject(), ActionMapper() {}
	void render() {}
	//bool isOnScreen() { return false; }
	void action(int id, int state)
	{
		ActionMapper::action(id, state);
		if (!quitFlag)
		{
			if (state)
			{
				quitFlag = true;
				dsq->quitNestedMain();
			}
		}
	}
protected:
	void onUpdate(float dt)
	{
		ActionMapper::onUpdate(dt);
		RenderObject::onUpdate(dt);
	}
};
*/

Intro::Intro() : StateObject()
{
	registerState(this, "Intro");
}

void Intro::applyState()
{
	quitFlag = false;

	dsq->jiggleCursor();
	dsq->cursor->alpha = 0;
	//core->sound->streamOgg("sc/theme", 0);
	StateObject::applyState();

	core->sound->fadeMusic(SFT_OUT, 1);

	dsq->user.load(true);

	ActionMapper::clearActions();

	/*
	m = new Mappy;
	dsq->user.control.actionSet.importAction(m, "Escape", ACTION_ESC);
	addRenderObject(m, LR_BACKGROUND);
	*/

	done = false;
}

void Intro::removeState()
{
	core->setClearColor(Vector(0,0,0));
	//glClearColor(0, 0, 0, 0);
	dsq->cursor->alpha = 1;
	StateObject::removeState();
}

void Intro::endIntro()
{
	quitFlag = true;

	dsq->overlay->color.stop();
	dsq->overlay->alpha.stop();

	dsq->overlay2->alpha.stop();
	dsq->overlay2->color.stop();

	dsq->overlay2->color = Vector(1,1,1);
	dsq->overlay2->alpha.interpolateTo(1, 1);

	dsq->main(1);

	dsq->overlay->color = Vector(0,0,0);
	dsq->overlay->alpha = 0;

	dsq->sound->stopAllSfx();

	dsq->sound->clearLocalSounds();

	cachy.clean();

	dsq->toggleBlackBars(0);

	dsq->setCutscene(0);

	dsq->title();
}

bool Intro::waitQuit(float t)
{
	dsq->main(t);
	return false;

	/*
	dsq->main(t);
	if (quitFlag)
	{
		//errorLog("quitFlag!");
		endIntro();
		return true;
	}
	return false;
	*/

	/*
	float frameTime = 0.01;
	float c = 0;
	bool tminus=false;
	while (c < t)
	{
		float dt = frameTime;
		if ((c + dt) > t)
		{
			dt = (t-c);
			tminus = true;
		}

		std::ostringstream os;
		os << " tminus: " << tminus << " dt: " << dt << " c: " << c;
		debugLog(os.str());

		dsq->main(dt);
		c += dt;

		if (m->isActing(ACTION_ESC))
		//if (core->getKeyState(KEY_ESCAPE))
		{
			endIntro();
			return true;
		}
	}

	return false;
	*/
}

void Intro::createMeteor(int layer, Vector pos, Vector off, Vector sz)
{
	Quad *m = new Quad("intro/meteor", pos);
	m->offset.interpolateTo(off, 5);
	m->rotationOffset.interpolateTo(Vector(0,0,-360), 2.5, -1);
	m->rotation.z = rand()%360;
	Quad *g = new Quad("particles/glow", Vector(0,0));
	g->setBlendType(RenderObject::BLEND_ADD);
	g->scale = Vector(24, 24);
	g->alpha = 0.5;
	g->color = Vector(1, 0.5, 0.5);

	m->addChild(g, PM_POINTER);
	m->scale = sz;
	addRenderObject(m, LR_ENTITIES);

	meteors.push_back(m);
}

void Intro::clearMeteors()
{
	for (int i = 0; i < meteors.size(); i++)
	{
		meteors[i]->setLife(1);
		meteors[i]->setDecayRate(100);
	}

	meteors.clear();
}

void Intro::update(float dt)
{
	if (!done)
	{
		done = true;

#ifndef AQUARIA_DEMO
		bool doit=true;
		if (doit){
			if (dsq->user.demo.intro > 0)
				dsq->user.demo.intro = 0;
		}
#endif

		dsq->setCutscene(1, 1);

		//errorLog("Here!");

		dsq->toggleBlackBars(1);
		dsq->setBlackBarsColor(Vector(0,0,0));
		
		cachy.precacheTex("intro/*.png");

		meteors.clear();

		dsq->sound->loadLocalSound("thunder");
		dsq->sound->loadLocalSound("drone");
		dsq->sound->loadLocalSound("windloop");
		dsq->sound->loadLocalSound("lululu");
		dsq->sound->loadLocalSound("proploop");
		dsq->sound->loadLocalSound("bridgebreak");
		dsq->sound->loadLocalSound("erictodoor");
		dsq->sound->loadLocalSound("ericdrowns");
		dsq->sound->loadLocalSound("mother");
		dsq->sound->loadLocalSound("bgloop-interior");
		dsq->sound->loadLocalSound("aqfocus");
		dsq->sound->loadLocalSound("thewave");
		dsq->sound->loadLocalSound("screaming");
		

		SkeletalSprite *citybg = new SkeletalSprite();
		citybg->loadSkeletal("citybg");
		citybg->animate("idle", -1);
		citybg->color = Vector(0.1, 0.08, 0.08);
		citybg->position = Vector(400,300);
		citybg->offset = Vector(-100, 0);
		citybg->scale = Vector(0.6, 0.6);
		citybg->alpha = 0;
		for (int i = 0; i < citybg->bones.size(); i++)
		{
			if (citybg->bones[i]->name != "meteor")
			{
				citybg->bones[i]->color = citybg->color;
			}
		}
		addRenderObject(citybg, LR_ENTITIES);

		SkeletalSprite *eric = new SkeletalSprite();
		eric->loadSkeletal("cc");
		eric->animate("runLow", -1);
		eric->color = Vector(0.08,0.08,0.08);
		eric->flipHorizontal();
		eric->position = Vector(50, 400);
		eric->alpha = 0;
		eric->scale = Vector(0.4, 0.4);
		for (int i = 0; i < eric->bones.size(); i++)
		{
			eric->bones[i]->color = eric->color;
		}
		addRenderObject(eric, LR_ENTITIES);

	

		dsq->overlay->alpha = 0;
		dsq->overlay2->alpha = 0;
		dsq->overlay3->alpha = 0;

		dsq->overlay->color = Vector(1,1,1);
		dsq->overlay2->color.interpolateTo(Vector(0,0,0), 1);

		dsq->overlay2->alpha = 1;
		
		core->cacheRender();


		//----------- set up the stuff


		// -- bit blot presents

		PlaySfx play;
		play.name = "drone";
		play.fade = SFT_IN;
		play.loops = -1;
		play.time = 40;
		play.vol = 0.7;
		void *drone = dsq->sound->playSfx(play);


		PlaySfx play2;
		play2.name = "windloop";
		play2.fade = SFT_IN;
		play2.loops = -1;
		play2.time = 20;
		play2.vol = 0.9;
		void *windLoop = dsq->sound->playSfx(play2);

		dsq->main(3);

		dsq->setClearColor(Vector(0.2,0.2,0.21));

		float bt = 11;

		/*
		Quad *bg = new Quad("intro/sky", Vector(400,300));
		bg->setWidthHeight(800, 600);
		addRenderObject(bg, LR_HUD);
		*/

		Quad *cloud_bg = new Quad("intro/cloud-bg", Vector(400,300));
		cloud_bg->setWidthHeight(800,600);
		cloud_bg->followCamera = 1;
		cloud_bg->alpha = 0.2;
		cloud_bg->flipVertical();
		cloud_bg->scale.interpolateTo(Vector(1.2, 1.5), bt*2);
		addRenderObject(cloud_bg, LR_BACKGROUND);

		Quad *mc1 = new Quad("intro/intro-big-cloud", Vector(200, 300));
		mc1->scale = Vector(2, 2);
		mc1->followCamera = 1;
		mc1->alpha = 1;
		mc1->alpha.interpolateTo(0, bt*2);
		mc1->offset = Vector(200, 0);
		mc1->offset.interpolateTo(Vector(0,0), bt*2);
		mc1->cull = false;
		addRenderObject(mc1, LR_HUD2);

		/*
		// FIXME: In the original code, the addRenderObject() call below
		// referenced mc1 instead of mc2, causing mc1 to get added to the
		// RenderObject list twice (potentially resulting in a crash).
		// Fixing the addRenderObject() call will naturally change the
		// appearance of the intro, so I've left this disabled.  --achurch
		Quad *mc2 = new Quad("intro/intro-big-cloud", Vector(500, 300));
		mc2->scale = Vector(2, 2);
		mc2->followCamera = 1;
		mc2->alpha = 1;
		mc2->alpha.interpolateTo(0.2, bt*2);
		mc2->offset = Vector(-200, 0);
		mc2->offset.interpolateTo(Vector(0,0), bt*1.5f);
		mc2->cull = false;
		addRenderObject(mc2, LR_HUD2);
		*/

		Quad *bbp = new Quad("intro/bbp", Vector(400,300));
		bbp->alpha = 0;
		bbp->alpha.interpolateTo(1, 8);
		bbp->followCamera = 1;
		bbp->scale = Vector(0.5, 0.5);
		bbp->scale.interpolateTo(Vector(0.8, 0.8), bt);
		bbp->color.ensureData();
		bbp->color.data->path.addPathNode(Vector(1,1,1), 0);
		bbp->color.data->path.addPathNode(Vector(0.5,0.5,0.5), 0.1);
		bbp->color.data->path.addPathNode(Vector(1,1,1), 0.2);
		bbp->color.data->path.addPathNode(Vector(0.5,0.5,0.5), 0.9);
		bbp->color.data->path.addPathNode(Vector(1,1,1), 0.95);
		bbp->color.data->path.addPathNode(Vector(0.5,0.5,0.5), 1.0);
		bbp->color.startPath(2.5);
		bbp->color.data->loopType = -1;
		addRenderObject(bbp, LR_HUD);


		dsq->overlay2->alpha.interpolateTo(0, 2);

		if (waitQuit(1)) return;

		dsq->sound->playSfx("lululu", 0.6);

		if (waitQuit(9)) return;

		dsq->sound->playSfx("thunder");
		dsq->overlay->alpha.interpolateTo(1, 0.2);

		if (waitQuit(0.5)) return;

		bbp->alpha = 0;
		cloud_bg->alpha = 0;
		cloud_bg->scale.stop();
		cloud_bg->scale = Vector(1,1);
		mc1->alpha.stop();
		//mc2->alpha.stop();
		mc1->alpha = 0;
		//mc2->alpha = 0;


		// -- floating city in clouds

		
		dsq->setClearColor(Vector(0.4,0.4,0.4));

		/*
		Gradient *grad = new Gradient();
		grad->position = Vector(400,300);
		grad->scale = Vector(800,600);
		grad->makeVertical(Vector(0,0,0), Vector(0.2, 0.2, 0.3));
		addRenderObject(grad, LR_BACKGROUND);
		*/

		cloud_bg->alpha = 1;
		cloud_bg->flipVertical();

		Quad *grad = new Quad("intro/intro-gradient-bg", Vector(400,300));
		grad->setWidthHeight(800,600);
		//grad->alpha = 0.5;
		grad->alpha = 0;
		addRenderObject(grad, LR_BACKGROUND);

		SkeletalSprite *city = new SkeletalSprite();
		city->loadSkeletal("floating-city");
		city->position = Vector(400,300);
		city->shareColorWithChildren = true;
		city->shareAlphaWithChildren = true;
		city->alpha = 0;
		for (int i = 1; i <=2; i++)
		{
			Bone *b = city->getBoneByIdx(i);
			b->rotationOffset.interpolateTo(Vector(0,0,360), 0.5, -1);
		}
		for (int i = 3; i <=5; i++)
		{
			Bone *b = city->getBoneByIdx(i);
			b->scale.interpolateTo(Vector(0.2, 1), 0.2, -1, 1);
		}
		city->alpha.interpolateTo(1.0, bt*0.75f);
		city->internalOffset = Vector(0, 50);
		city->scale = Vector(0.6, 0.6);
		city->scale.interpolateTo(Vector(0.75, 0.75), bt);
		city->animate("idle");
		addRenderObject(city, LR_HUD);

		PlaySfx play3;
		play3.name = "proploop";
		play3.fade = SFT_IN;
		play3.loops = -1;
		play3.time = 10;
		play3.vol = 0.7;
		void *propLoop = dsq->sound->playSfx(play3);

		dsq->overlay->alpha.interpolateTo(0, 1);

		if (waitQuit(8)) return;


		dsq->sound->playSfx("heartbeat");

		dsq->sound->fadeSfx(propLoop, SFT_OUT, 2);

		dsq->overlay->color = Vector(0,0,0);
		dsq->fade(1, 0.5);
		if (waitQuit(0.5)) return;

		city->alpha = 0;
		grad->alpha = 0;
		cloud_bg->alpha = 0;

		if (waitQuit(2)) return;

		

		// -- window

		float wt = 7;
		dsq->setClearColor(Vector(0,0,0));

		Quad *window = new Quad("intro/window", Vector(400,300));
		window->offset.interpolateTo(Vector(0, 0),0);
		window->offset.interpolateTo(Vector(50, 0), wt, 0, 0, 1);
		window->followCamera = 1;
		//window->setWidthHeight(200, 400);
		addRenderObject(window, LR_BACKGROUND);

		Quad *windowGlow = new Quad("intro/window-glow", Vector(0,0));
		window->addChild(windowGlow, PM_POINTER);

		window->shareColorWithChildren = 1;
		window->shareAlphaWithChildren = 1;

		Quad *ericHead = new Quad("intro/eric-head", Vector(350,600-256));
		ericHead->offset.interpolateTo(Vector(-50, 0),0);
		ericHead->offset.interpolateTo(Vector(-140, 50), wt, 0, 0, 1);
		ericHead->followCamera = 1;
		addRenderObject(ericHead, LR_ENTITIES);

		dsq->fade(0, 0.5);
		if (waitQuit(0.5)) return;

		if (waitQuit(5)) return;

		dsq->sound->playSfx("heartbeat");

		dsq->fade(1, 0.5);
		if (waitQuit(0.5)) return;

		window->alpha = 0;
		ericHead->alpha = 0;


		
		// -- brush

		float brusht=7;
		Quad *painting = new Quad("intro/painting", Vector(400,300));
		painting->scale = Vector(0.9, 0.9);
		// 1.2
		painting->scale.interpolateTo(Vector(2,2), brusht, 0, 0, 1);
		painting->followCamera = 1;
		addRenderObject(painting, LR_HUD);

		Quad *ericHandBrush = new Quad("intro/eric-hand-brush", Vector(800-256 + 20,600-256 + 20));
		ericHandBrush->scale = Vector(0.99, 0.99);
		ericHandBrush->scale.interpolateTo(Vector(1.7,1.7), brusht, 0, 0, 1);
		ericHandBrush->followCamera = 1;
		ericHandBrush->rotation.interpolateTo(Vector(0,0,-20), brusht);
		ericHandBrush->offset.interpolateTo(Vector(550, 550), brusht*0.75f, 0, 0, 1);
		addRenderObject(ericHandBrush, LR_HUD);


		dsq->fade(0, 0.5);
		if (waitQuit(0.5)) return;

		if (waitQuit(2)) return;

		// music bit

		dsq->sound->playSfx("aqfocus", 0.8);

		if (waitQuit(1.5)) return;
		

		if (waitQuit(2)) return;

		// aquaria... could go here

		if (waitQuit(2)) return;

		dsq->sound->playSfx("thewave");

		if (waitQuit(0.5)) return;

		dsq->sound->playSfx("heartbeat");
		dsq->fade(1, 0.5);
		if (waitQuit(0.5)) return;

		painting->alpha = 0;
		ericHandBrush->alpha = 0;


		// -- window turning red

		float wrt = 4;

		dsq->setClearColor(Vector(0,0,0));
		window->alpha = 1;
		ericHead->alpha = 1;

		window->color.interpolateTo(Vector(1, 0.5, 0.5), wrt);
		ericHead->color.interpolateTo(Vector(1, 0.7, 0.7), wrt);
		ericHead->offset = Vector(-200,0);
		ericHead->offset.interpolateTo(Vector(-800, 200), wrt);

		window->scale.interpolateTo(Vector(1.3, 1.3), wrt);

		dsq->fade(0, 0.5);
		if (waitQuit(0.5)) return;

		if (waitQuit(wrt-1)) return;

		dsq->sound->playSfx("heartbeat");
		dsq->fade(1, 0.5);
		if (waitQuit(0.5)) return;

		ericHead->alpha = 0;
		window->alpha = 0;

		// -- city under attack by meteors

		dsq->setClearColor(Vector(0.6,0.1,0.1));

		createMeteor(LR_ENTITIES, Vector(600, -50), Vector(-600, 600), Vector(0.05, 0.05));


		cloud_bg->alpha = 0.99;
		cloud_bg->alpha.interpolateTo(0.8, 4);
		//grad->alpha = 0.5;
		city->alpha = 1;

		city->stopAllAnimations();

		city->animate("idle");

		dsq->fade(0, 0.5);
		if (waitQuit(0.5)) return;

		if (waitQuit(0.5)) return;

		createMeteor(LR_ENTITIES, Vector(300, -50), Vector(-500, 900), Vector(0.2, 0.2));
		createMeteor(LR_HUD, Vector(400, -100), Vector(-400, 1000), Vector(0.2, 0.2));

		if (waitQuit(2)) return;

		dsq->shakeCamera(2, 10);

		for (int i = 0; i < 16; i++)
		{
			int l = LR_ENTITIES;
			Vector sz(0.1, 0.1);
			if (rand()%5 < 2)
			{
				l = LR_HUD;
				sz = Vector(0.15, 0.15);
			}
			createMeteor(l, Vector(200+rand()%600, -50 - rand()%100), Vector(-200 - rand()%400, 900 + rand()%400), sz);
		}

		if (waitQuit(1)) return;


		for (int i = 0; i < 8; i++)
		{
			int l = LR_ENTITIES;
			Vector sz(0.1, 0.1);
			if (rand()%5 < 2)
			{
				l = LR_HUD;
				sz = Vector(0.15, 0.15);
			}
			createMeteor(l, Vector(200+rand()%600, -50 - rand()%100), Vector(-200 - rand()%400, 900 + rand()%400), sz);
		}

		if (waitQuit(0.4)) return;

		dsq->sound->playSfx("heartbeat");
		dsq->sound->fadeSfx(drone, SFT_OUT, 0.1);
		dsq->sound->fadeSfx(windLoop, SFT_OUT, 0.1);

		dsq->overlay->color = Vector(1, 0.5, 0.5);
		dsq->fade(1, 0.1);
		if (waitQuit(1)) return;

		city->alpha = 0;

		cloud_bg->alpha.stop();
		cloud_bg->alpha = 0;

		clearMeteors();

		dsq->shakeCamera(0, 0);


		// -- walking out

		dsq->sound->playSfx("bridgebreak");

		if (waitQuit(2)) return;

		dsq->setClearColor(Vector(0,0,0));
		dsq->fade(0, 10);

		//dsq->sound->playSfx("heartbeat");

		if (waitQuit(2)) return;

		if (waitQuit(2)) return;

		dsq->sound->playSfx("erictodoor");

		if (waitQuit(8)) return;

		windLoop = dsq->sound->playSfx(play2);
		drone = dsq->sound->playSfx(play);

		dsq->overlay->color = Vector(0,0,0);

		dsq->fade(1, 0.1);

		if (waitQuit(3.5)) return;


		// -- outside scene


		dsq->setClearColor(Vector(0.3,0.1,0.1));

		cloud_bg->alpha = 0.4;
		cloud_bg->scale = Vector(1.5, 1.5);
		//cloud_bg->setSegs(32, 32, 0.5, 0.5, 0.008, 0.008, 4, 1);
		//cloud_bg->setSegs(10, 10, 0.1, 0.1, 1, 1, 0.1, 1);
		cloud_bg->rotation.z = 180;

		citybg->alpha = 1;

		PlaySfx pScreaming;
		pScreaming.name = "screaming";
		pScreaming.fade = SFT_IN;
		pScreaming.loops = -1;
		pScreaming.time = 8;
		pScreaming.vol = 1.0;
		void *screaming = dsq->sound->playSfx(pScreaming);

		eric->alpha = 1;
		eric->offset.interpolateTo(Vector(150, 0), 1);

		dsq->fade(0, 0.2);

		if (waitQuit(1)) return;
		eric->animate("idle", -1);

		if (waitQuit(1)) return;

		eric->animate("runLow", -1);

		eric->offset.interpolateTo(Vector(150+150, 0), 1);
		if (waitQuit(0.5)) return;

		dsq->sound->playSfx("thunder", 0.1);

		if (waitQuit(0.5)) return;

		eric->offset.interpolateTo(Vector(150+150+150, 0), 1);
		if (waitQuit(1)) return;

		eric->offset.interpolateTo(Vector(150+150+150+150, 0), 1);
		if (waitQuit(0.5)) return;
		dsq->sound->playSfx("thunder", 0.7);
		if (waitQuit(0.5)) return;

		eric->animate("idle", -1);



		eric->flipHorizontal();

		citybg->animate("crash");
		if (waitQuit(1.9)) return;

		dsq->sound->playSfx("bridgebreak");

		dsq->sound->stopSfx(windLoop);
		dsq->sound->stopSfx(drone);
		dsq->sound->stopSfx(screaming);

		eric->rotation.interpolateTo(Vector(0,0,96), 1.5);
		eric->offset.interpolateTo(Vector(eric->offset.x, 800), 1);

		if (waitQuit(1.9)) return;


		


		dsq->overlay->color = Vector(0,0,0);
		dsq->fade(1, 0.1);
		if (waitQuit(0.1)) return;

		citybg->alpha = 0;
		eric->alpha = 0;
		cloud_bg->alpha = 0;

		if (waitQuit(2)) return;


		// -- drown


		dsq->sound->playSfx("ericdrowns");

		if (waitQuit(8)) return;

		float st = 10;

		Quad *underwaterBG = new Quad("intro/underwater-bg", Vector(400,400));
		underwaterBG->setWidthHeight(800, 800);
		underwaterBG->followCamera = 1;
		addRenderObject(underwaterBG, LR_ENTITIES);

		Quad *ericHandSink = new Quad("intro/eric-hand-sink", Vector(550,610));
		ericHandSink->scale = Vector(0.99, 0.99);
		ericHandSink->scale.interpolateTo(Vector(1.5,1.5), st, 0, 0, 1);
		ericHandSink->internalOffset = Vector(0, -256);
		ericHandSink->rotation = Vector(0,0,-10);
		ericHandSink->rotation.interpolateTo(Vector(0,0,10), st, 0, 0, 1);
		ericHandSink->offset.interpolateTo(Vector(360, 100), st, 0, 0, 1);
		ericHandSink->followCamera = 1;
		addRenderObject(ericHandSink, LR_ENTITIES);

		dsq->setClearColor(Vector(0, 0.05, 0.1));


		if (core->afterEffectManager)
			core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08,0.05,22,0.2f, 1.2));


		dsq->fade(0, 3);

		if (waitQuit(3)) return;

		PlaySfx play4;
		play4.name = "bgloop-interior";
		play4.loops = -1;
		play4.time = 1;
		play4.fade = SFT_IN; 
		void *bgLoop = dsq->sound->playSfx(play4);

		if (waitQuit(3)) return;


		// -- dying



		dsq->sound->playSfx("heartbeat", 0.5);

		dsq->fade(1, 5);
		if (waitQuit(2.5)) return;

		core->sound->playVoice("titleb");

		dsq->sound->playSfx("heartbeat", 0.2);
		
		if (waitQuit(2.5)) return;

		underwaterBG->alphaMod = 0;

		core->sound->playSfx("mother", 0.6);

		dsq->fade(0, 2);

		Quad *mom = new Quad("gameover-0004", Vector(400,300));
		mom->setWidthHeight(600,600);
		mom->alphaMod = 0.035;
		mom->setBlendType(RenderObject::BLEND_ADD);
		mom->alpha = 0;
		mom->alpha.interpolateTo(1, 5);
		mom->followCamera = 1;
		addRenderObject(mom, LR_HUD);


		// -- music backwards

		dsq->overlay2->color = Vector(1,1,1);
		dsq->overlay2->alpha.interpolateTo(1, 9);

		dsq->sound->fadeSfx(bgLoop, SFT_OUT, 5);

		if (waitQuit(8)) return; // 11.5

		// -- end

		endIntro();
	}
}

