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
	return dsq->run(time, false, true);
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
	logo->scale = Vector(0.6f,0.6f);
	addRenderObject(logo, LR_HUD);

	if(watchQuit(1.5f))
		return;

	dsq->overlay2->alpha.interpolateTo(1, 0.5f);
	watchQuit(0.5f);
}

void BitBlotLogo::getOut()
{
	dsq->title(false);
}

void BitBlotLogo::applyState()
{
	showSequence();
	getOut();
}

void BitBlotLogo::showSequence()
{
	StateObject::applyState();
	dsq->toggleCursor(0);
	dsq->toggleBlackBars(1);

	dsq->jiggleCursor();

	if (dsq->user.demo.shortLogos)
	{
		doShortBitBlot();
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
		bird->getBoneByIdx(0)->alphaMod = 0.3f;
		bird->getBoneByIdx(1)->alphaMod = 0.3f;
		landscape->addChild(bird, PM_POINTER, RBP_OFF);
		bird->update((rand()%100)*0.1f);
	}


	if (rand()%100 < 40)
	{
		SkeletalSprite *dragon = new SkeletalSprite();
		dragon->scale = Vector(0.9f*0.6f, 0.9f*0.6f);
		dragon->loadSkeletal("bb-dragon");
		dragon->animate("idle", -1);
		dragon->position = Vector(300 , -100);
		for (size_t i = 0; i < dragon->bones.size(); i++)
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
		windmill->scale = Vector(0.7f, 0.7f);
		for (size_t i = 0; i < windmill->bones.size(); i++)
		{
			windmill->getBoneByIdx(i)->alpha = 0.7f;
		}
		windmills.push_back(windmill);
	}
	addRenderObject(landscape, LR_BACKDROP);

	if (windmills.size() >= 2)
	{
		for (size_t i = 0; i < windmills.size()-1; i++)
		{
			if (windmills[i]->position.y < 4000)
			{
				for (size_t j = i+1; j < windmills.size(); j++)
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
	logo->scale = Vector(0.6f, 0.6f);
	addRenderObject(logo, LR_ENTITIES);

	Quad *logob = new Quad("BitBlot/Logo-blur.png", Vector(400,300));
	logob->followCamera = 1;
	logob->scale = Vector(0.6f, 0.6f);
	logob->offset = Vector(-4, 0);
	logob->offset.interpolateTo(Vector(4,0), 0.05f, -1, 1);

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
		lin->alphaMod = 0.2f;
		lin->setWidthHeight(800, 3);
		lines->addChild(lin, PM_POINTER);
	}


	Quad *scanline = new Quad("", Vector(400,-100));
	scanline->followCamera = 1;
	scanline->setWidthHeight(800, 200);
	scanline->alpha = 0.5;
	scanline->position.interpolateTo(Vector(400,700), 0.4f, -1);
	addRenderObject(scanline, LR_ENTITIES2);

	dsq->overlay->alpha = 1;

	core->cacheRender();
	core->resetTimer();


	sound->playSfx("BBPowerOn");

	if (watchQuit(1.0)) return;

	dsq->overlay->alpha.interpolateTo(0, 1);


	sound->playMusic("bblogo", SLT_NONE);

	if (watchQuit(1.0)) return;



	dsq->overlay2->color = Vector(1,1,1);


	logob->alpha.interpolateTo(0, 2.0);

	if (watchQuit(1.0)) return;
	if (watchQuit(1.0)) return;


	sound->playSfx("normalform");


	white->alpha.interpolateTo(0, 2);

	landscape->position.interpolateTo(Vector(400,400), 5);



	landscape->scale.interpolateTo(Vector(1.1f, 1.1f), 5);

	scanline->alpha.interpolateTo(0, 1);


	logo->scale.interpolateTo(Vector(2, 2), 1);

	logo->alpha.interpolateTo(0, 1);

	lines->alpha.interpolateTo(0, 4);
	lines->scale.interpolateTo(Vector(8, 8), 4);

	if (watchQuit(2.0)) return;

	if (watchQuit(1.0)) return;

	dsq->overlay2->alpha.interpolateTo(1, 2);
	if (watchQuit(2.0)) return;
}

void BitBlotLogo::removeState()
{
	StateObject::removeState();
	core->setClearColor(Vector(0,0,0));
}

void BitBlotLogo::update(float dt)
{
	StateObject::update(dt);


}

