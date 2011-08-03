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
#include "../BBGE/AfterEffect.h"

#include "States.h"
#include "Game.h"

BitBlotLogo::BitBlotLogo() : StateObject()
{
	registerState(this, "BitBlotLogo");
}

bool BitBlotLogo::watchQuit(float time)
{
	core->main(time);
	return false;
/*
	dsq->watch(time);
	if (quitFlag > 0)
	{
		skipLogo();
		return true;
	}
	return false;
*/
}

void BitBlotLogo::doShortBitBlot()
{
	dsq->overlay->color = Vector(0,0,0);
	dsq->overlay->alpha = 0;
	
	dsq->overlay2->color = Vector(1,1,1);
	dsq->overlay2->alpha = 1;
	dsq->overlay2->alpha.interpolateTo(0, 0.25);
	
	Quad *bg = new Quad;
	bg->setWidthHeight(800, 600);
	bg->position = Vector(400,300);
	bg->followCamera = 1;
	addRenderObject(bg, LR_HUD);
	
	Quad *logo = new Quad("BitBlot/Logo.png", Vector(400,300));
	logo->followCamera = 1;
	logo->scale = Vector(0.6,0.6);
	addRenderObject(logo, LR_HUD);
	
	core->main(1.5);
	
	dsq->overlay2->alpha.interpolateTo(1, 0.5);
	core->main(0.5);
}

void BitBlotLogo::skipLogo()
{
	quitFlag = 2;
	doShortBitBlot();
	getOut();
}

void BitBlotLogo::getOut()
{
	dsq->continuity.reset();

#ifdef AQUARIA_DEMO
	dsq->title();
#else
	if (dsq->user.demo.intro != 0)
		dsq->enqueueJumpState("Intro");
	else
		dsq->title();
#endif
}

void BitBlotLogo::applyState()
{	
	StateObject::applyState();
	quitFlag = 0;
	logo = 0;
	dsq->toggleCursor(0);
	dsq->toggleBlackBars(1);
	//dsq->setBlackBarsColor(Vector(1,1,1));
	dsq->jiggleCursor();
	
	dsq->forceInputGrabOff();
	
	if (dsq->user.demo.shortLogos)
	{
		skipLogo();
		return;
	}

	logo = 1;
	
	if (core->getKeyState(KEY_ESCAPE))
	{
		skipLogo();
		return;
	}

	Quad *bg = new Quad;
	bg->setWidthHeight(800, 600);
	bg->position = Vector(400,300);
	bg->followCamera = 1;
	addRenderObject(bg, LR_BACKDROP);

	Quad *landscape = new Quad("bitblot/logo-bg", Vector(400,200));
	landscape->setWidthHeight(800, 800);
	landscape->followCamera = 1;
	landscape->alpha = 1;
	landscape->shareAlphaWithChildren = 1;
	

	for (int i = 2; i < 5 + rand()%10; i++)
	{
		SkeletalSprite *bird = new SkeletalSprite();
		bird->loadSkeletal("bb-bird");
		bird->animate("idle", -1);
		bird->position = Vector(-300 + rand()%150, -200 + rand()%200);
		bird->offset.interpolateTo(Vector(200, -20 + rand()%100), 20);
		bird->shareAlphaWithChildren = 1;
		bird->getBoneByIdx(0)->alphaMod = 0.3;
		bird->getBoneByIdx(1)->alphaMod = 0.3;
		landscape->addChild(bird, PM_POINTER, RBP_OFF);
		bird->update((rand()%100)*0.1f);
	}

	//if (true)
	if (rand()%100 < 40)
	{
		SkeletalSprite *dragon = new SkeletalSprite();
		dragon->scale = Vector(0.9f*0.6f, 0.9f*0.6f);
		dragon->loadSkeletal("bb-dragon");
		dragon->animate("idle", -1);
		dragon->position = Vector(300 , -100);
		for (int i = 0; i < dragon->bones.size(); i++)
		{
			dragon->getBoneByIdx(i)->shareAlphaWithChildren = 1;
		}
		dragon->shareAlphaWithChildren = 1;
		landscape->addChild(dragon, PM_POINTER, RBP_OFF);
		dragon->update((rand()%100)*0.1f);
	}

	std::vector<SkeletalSprite*> windmills;
	int numWindmills = rand()%3; 
	for (int i = 0; i < numWindmills; i++)
	{
		SkeletalSprite *windmill = new SkeletalSprite();
		windmill->loadSkeletal("bb-windmill");
		windmill->animate("idle", -1);
		windmill->position = Vector(-200 + rand()%150, 100+rand()%300);
		windmill->shareAlphaWithChildren = 1;
		landscape->addChild(windmill, PM_POINTER, RBP_OFF);
		windmill->update((rand()%100)*0.1f);
		windmill->scale = Vector(0.7, 0.7);
		for (int i = 0; i < windmill->bones.size(); i++)
		{
			windmill->getBoneByIdx(i)->alpha = 0.7;
		}
		windmills.push_back(windmill);
	}
	addRenderObject(landscape, LR_BACKDROP);

	if (windmills.size() >= 2)
	{
		for (int i = 0; i < windmills.size()-1; i++)
		{
			if (windmills[i]->position.y < 4000)
			{
				for (int j = i+1; j < windmills.size(); j++)
				{
					if (windmills[j]->position.y < 4000)
					{
						if ((windmills[i]->getWorldPosition() - windmills[j]->getWorldPosition()).isLength2DIn(45))
						{
							windmills[i]->position.y += 4900;
						}
					}
				}
			}
		}
	}

	Quad *white = new Quad("", Vector(400,300));
	white->setWidthHeight(800, 600);
	white->alpha = 1;
	addRenderObject(white, LR_BACKDROP);

	Quad *logo = new Quad("BitBlot/Logo.png", Vector(400,300));
	logo->followCamera = 1;
	logo->scale = Vector(0.6, 0.6);
	addRenderObject(logo, LR_ENTITIES);

	Quad *logob = new Quad("BitBlot/Logo-blur.png", Vector(400,300));
	logob->followCamera = 1;
	logob->scale = Vector(0.6, 0.6);
	logob->offset = Vector(-4, 0);
	logob->offset.interpolateTo(Vector(4,0), 0.05, -1, 1);
	
	addRenderObject(logob, LR_ENTITIES);

	RenderObject *lines = new RenderObject();
	lines->followCamera = 1;
	lines->position = Vector(400,300);
	lines->shareAlphaWithChildren = 1;
	addRenderObject(lines, LR_ENTITIES);

	for (int y = 0; y < 600; y+=6)
	{
		Quad *lin = new Quad("", Vector(0,y-300));
		lin->followCamera = 1;
		lin->color = 0;
		lin->alphaMod = 0.2;
		lin->setWidthHeight(800, 3);
		lines->addChild(lin, PM_POINTER);
	}


	Quad *scanline = new Quad("", Vector(400,-100));
	scanline->followCamera = 1;
	scanline->setWidthHeight(800, 200);
	scanline->alpha = 0.5;
	scanline->position.interpolateTo(Vector(400,700), 0.4, -1);
	addRenderObject(scanline, LR_ENTITIES2);

	dsq->overlay->alpha = 1;

	core->cacheRender();
	core->resetTimer();


	sound->playSfx("BBPowerOn");

	if (watchQuit(1.0)) return;

	dsq->overlay->alpha.interpolateTo(0, 1);

	//sound->playSfx("BBSplash");
	sound->playMusic("bblogo", SLT_NONE);

	if (watchQuit(1.0)) return;

	//sound->playSfx("normalform");

	dsq->overlay2->color = Vector(1,1,1);


	logob->alpha.interpolateTo(0, 2.0);

	if (watchQuit(1.0)) return;
	if (watchQuit(1.0)) return;


	sound->playSfx("normalform");

	//landscape->alpha.interpolateTo(1, 2);
	white->alpha.interpolateTo(0, 2);

	landscape->position.interpolateTo(Vector(400,400), 5);

	/*
	if (core->afterEffectManager)
		core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter, 0.1,0.03,30,0.2f, 0.5));
	*/

	landscape->scale.interpolateTo(Vector(1.1, 1.1), 5);

	scanline->alpha.interpolateTo(0, 1);
	//scanline->scale.interpolateTo(Vector(5, 5), 1);

	logo->scale.interpolateTo(Vector(2, 2), 1);
	//logo->offset.interpolateTo(Vector(0, -300), 1);
	logo->alpha.interpolateTo(0, 1);

	lines->alpha.interpolateTo(0, 4);
	lines->scale.interpolateTo(Vector(8, 8), 4);

	if (watchQuit(2.0)) return;

	if (watchQuit(1.0)) return;

	dsq->overlay2->alpha.interpolateTo(1, 2);
	if (watchQuit(2.0)) return;
	
	
	getOut();

	/*
	// BOING
	dsq->toggleCursor(0);
	Quad *logo = new Quad("BitBlot/Logo.png", Vector(400,300));
	logo->followCamera = 1;
	addRenderObject(logo);
	//logo->scale = Vector(0.6, 0.6);
	logo->scale = Vector(0, 0);
	core->setClearColor(Vector(1,1,1));
	dsq->overlay->alpha = 1;
	dsq->overlay->alpha.interpolateTo(0, 1);
	core->main(0.5);

	logo->scale.path.addPathNode(Vector(0,0), 0);
	logo->scale.path.addPathNode(Vector(0,0), 0.4);
	logo->scale.path.addPathNode(Vector(1.2,1.2), 0.8);
	logo->scale.path.addPathNode(Vector(1.0, 1.0), 0.85);
	logo->scale.path.addPathNode(Vector(1.1, 1.1), 0.95);
	logo->scale.path.addPathNode(Vector(1,1), 1.0);
	logo->scale.startPath(1);

	core->main(2);

	dsq->overlay->alpha.interpolateTo(1, 1.5);
	core->main(1.5);
	*/
}

void BitBlotLogo::removeState()
{
	StateObject::removeState();
	core->setClearColor(Vector(0,0,0));
}

void BitBlotLogo::update(float dt)
{
	StateObject::update(dt);
	
	/*
	if (quitFlag == 0)
	{
		if (core->getKeyState(KEY_ESCAPE))
		{
			quitFlag = 1;
		}
	}
	*/
}

