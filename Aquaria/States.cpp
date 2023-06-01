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
#include "Game.h"
#include "AquariaMenuItem.h"

#include "../BBGE/DebugFont.h"
#include "../BBGE/AfterEffect.h"


Intro2::Intro2() : StateObject()
{
	registerState(this, "Intro2");
}

void Intro2::skipIntro()
{
	dsq->newGame();
}

void Intro2::applyState()
{
	StateObject::applyState();


	ActionMapper::clearActions();



	dsq->subtitlePlayer.show(0.5f);

	core->resetCamera();

	dsq->jiggleCursor();

	dsq->setCutscene(1,1);

	core->run(1);
	dsq->overlay->alpha.interpolateTo(0, 40);
	dsq->toggleCursor(0);



	Quad *frame4 = new Quad;
	{
		frame4->setTexture("gameover-0004");
		frame4->position = Vector(400,310);
		frame4->setWidthHeight(600, 600);
		frame4->setSegs(2, 32, 0.1f, 0.1f, 0.002f, 0.003f, 2.0f, 1);
	}
	addRenderObject(frame4, LR_BACKGROUND);

	ParticleEffect *emitter = new ParticleEffect;
	emitter->load("NaijaIntro_Idle");

	emitter->position = Vector(400,300);
	addRenderObject(emitter, LR_PARTICLES);
	emitter->start();

	ParticleEffect *emitter3 = new ParticleEffect;
	emitter3->load("VerseEnergy2");
	emitter3->position = Vector(400,300);
	addRenderObject(emitter3, LR_PARTICLES);
	emitter3->start();

	ParticleEffect *emitter2 = new ParticleEffect;
	emitter2->load("NaijaIntro_Transition");
	emitter2->position = Vector(400,300);
	addRenderObject(emitter2, LR_PARTICLES);

	dsq->voice("Naija_Intro-music");


	while (dsq->sound->isPlayingVoice())
		dsq->run(FRAME_TIME);



	emitter->stop();
	emitter2->start();
	core->run(0.5);
	core->sound->playSfx("NormalForm");
	dsq->overlay->color = Vector(1,1,1);
	dsq->overlay->alpha = 0;
	dsq->fade(1, 1);
	core->run(1);

	frame4->alpha = 0;
	dsq->overlay->color.interpolateTo(0, 1);
	core->run(1);

	dsq->overlay->color = 0;
	dsq->overlay->alpha = 1;

	dsq->setCutscene(0);

	dsq->newGame();
}

void Intro2::removeState()
{
	StateObject::removeState();
}

void Intro2::update(float dt)
{
	StateObject::update(dt);
}

const float GO_ANIM_TIME = 1.0;
GameOver::GameOver() : StateObject()
{
	registerState(this, "GameOver");

	frame3 = frame2 = frame1 = 0;
}

void GameOver::applyState()
{
	core->sound->fadeMusic(SFT_OUT, 1);


	StateObject::applyState();
	core->globalScale = Vector(1,1);
	core->globalScaleChanged();
	core->cameraPos = Vector(0,0,0);

	core->sound->playSfx("Death");

	Quad *q = new Quad;
	{
		q->color = 0;
		q->setWidthHeight(800, 600);
		q->position = Vector(400,300,-0.1f);
	}
	addRenderObject(q, LR_ZERO);



	frame1 = new Quad;
	{
		frame1->setTexture("gameover-0004");
		frame1->position = Vector(400,300);
		frame1->setWidthHeight(600, 600);
	}
	addRenderObject(frame1, LR_BACKGROUND);

	frame2 = new Quad;
	{
		frame2->setTexture("gameover-0003");
		frame2->position = Vector(400,300);
		frame2->setWidthHeight(600, 600);
	}
	addRenderObject(frame2, LR_BACKGROUND);

	frame3 = new Quad;
	{
		frame3->setTexture("gameover-0002");
		frame3->position = Vector(400,300);
		frame3->setWidthHeight(600, 600);
	}
	addRenderObject(frame3, LR_BACKGROUND);

	Quad *frame4 = new Quad;
	{
		frame4->setTexture("gameover-0001");
		frame4->position = Vector(400,300);
		frame4->setWidthHeight(600, 600);
	}
	addRenderObject(frame4, LR_BACKGROUND);

	Quad *shockLines = new Quad;
	{
		shockLines->setTexture("shock-lines");
		shockLines->position = Vector(400,300);
		shockLines->setWidthHeight(800, 600);
		shockLines->setBlendType(BLEND_ADD);
		shockLines->scale.interpolateTo(Vector(4,4), 1);
	}
	addRenderObject(shockLines, LR_BACKGROUND);

	core->run(0.033f);
	if (core->afterEffectManager)
	{
		core->afterEffectManager->clear();
		core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter, 0.07f,0.03f,30,0.2f, 1.1f));
	}


	dsq->overlay->alpha = 0;



	core->run(GO_ANIM_TIME);
	frame4->alpha.interpolateTo(0, GO_ANIM_TIME);
	core->run(GO_ANIM_TIME);

	frame3->alpha.interpolateTo(0, GO_ANIM_TIME);
	core->run(GO_ANIM_TIME);

	frame2->alpha.interpolateTo(0, GO_ANIM_TIME);
	core->run(GO_ANIM_TIME);

	frame1->alpha.interpolateTo(0, GO_ANIM_TIME);
	core->run(GO_ANIM_TIME);

	core->run(1.5);



	if (dsq->recentSaveSlot != -1)
	{

		dsq->sound->stopMusic();
		float transferSeconds = dsq->continuity.seconds;
		dsq->continuity.loadFile(dsq->recentSaveSlot);



		dsq->continuity.seconds = transferSeconds;
		game->transitionToScene(game->sceneToLoad);
	}
	else
		dsq->title(false);
}

void GameOver::removeState()
{

	StateObject::removeState();
	frame3 = frame2 = frame1 = 0;
}

void GameOver::onClick()
{
	if (timer == 0)
		dsq->title(true);
}

void GameOver::update(float dt)
{
	StateObject::update(dt);
	if (timer > 0)
	{
		timer -= dt;
		if (timer <= 0)
			timer = 0;
	}


}

namespace NagStuff
{
	std::vector<Quad*> irot;
	int ic=0;
	const int numScreens = 11;
	float screenTimer = 0;

	const float screenTime = 3;
	const float nagFadeTime = 1;
}

using namespace NagStuff;

Nag::Nag() : StateObject()
{
	registerState(this, "Nag");

	hitBuy = false;
}

void Nag::applyState()
{
	StateObject::applyState();

	click = 0;

	dsq->cursor->offset.stop();
	dsq->cursor->internalOffset.stop();
	dsq->cursor->offset = Vector(0,0,0);
	dsq->cursor->internalOffset = Vector(0,0,0);
	dsq->cursor->color = Vector(0,0,0);

	dsq->sound->stopMusic();

	hitBuy = false;

	core->setInputGrab(false);
	grab = false;


	dsq->overlay2->alpha = 0;
	dsq->stopVoice();

	Quad *bg = new Quad("nag/nag", Vector(400,400));
	bg->setWidthHeight(800, 800);
	bg->followCamera = 1;
	addRenderObject(bg, LR_BACKGROUND);

	AquariaMenuItem *buy = new AquariaMenuItem();
	buy->followCamera = 1;
	buy->position = Vector(630, 400);

	buy->useGlow("particles/glow", 480, 128);
	buy->event.set(MakeFunctionEvent(Nag, onBuy));
	buy->setDirMove(DIR_LEFT, buy);
	buy->setDirMove(DIR_RIGHT, buy);
	buy->setDirMove(DIR_UP, buy);
	addRenderObject(buy, LR_BACKGROUND);

	AquariaMenuItem *exit = new AquariaMenuItem();
	exit->followCamera = 1;
	exit->position = Vector(732, 543);

	exit->useGlow("particles/glow", 128, 64);
	exit->event.set(MakeFunctionEvent(Nag, onExit));
	exit->setDirMove(DIR_LEFT, exit);
	exit->setDirMove(DIR_RIGHT, exit);
	exit->setDirMove(DIR_DOWN, exit);
	addRenderObject(exit, LR_BACKGROUND);


	buy->setDirMove(DIR_DOWN, exit);
	exit->setDirMove(DIR_UP, buy);

	buy->setFocus(true);

	ic = 0;
	irot.clear();


	Vector framePos(240, 400);
	Vector frameScale(0.98f, 0.98f);

	for (int i = 0; i < numScreens; i++)
	{
		std::ostringstream os;
		os << "nag/s-" << numToZeroString(i, 4);
		Quad *q = new Quad(os.str(), framePos);
		q->setTexture(os.str());
		if (i != 0)
			q->alpha = 0;
		else
		{
			q->alpha = 0;
			q->alpha.interpolateTo(1, nagFadeTime);
		}
		q->scale = Vector(0.75, 0.75) * frameScale;
		addRenderObject(q, LR_BACKGROUND);
		irot.push_back(q);
	}

	Quad *frame = new Quad("nag/nag-frame", framePos);
	frame->scale = frameScale;
	addRenderObject(frame, LR_BACKGROUND);

	dsq->toggleCursor(1, 0.5);

	dsq->fade(0, 1);
	dsq->run(1);

	dsq->sound->playMusic("openwaters3", SLT_NORMAL, SFT_IN, 2);
}

void Nag::removeState()
{
	dsq->toggleCursor(0);

	dsq->fade(1, 1);
	dsq->run(1);

	dsq->cursor->color = Vector(1,1,1);

	core->setInputGrab(true);
	grab = true;

	StateObject::removeState();
}

void Nag::update(float dt)
{
	StateObject::update(dt);

	core->setInputGrab(grab);

	screenTimer += dt;
	if (screenTimer > screenTime)
	{
		screenTimer = 0;
		ic ++;
		if (ic >= numScreens)
			ic = 0;
		for (size_t i = 0; i < irot.size(); i++) irot[i]->alpha.interpolateTo(0, nagFadeTime);
		irot[ic]->alpha.interpolateTo(1, nagFadeTime);
	}
}

void Nag::onExit()
{
	if (core->isNested()) return;

	if (!hitBuy)
	{
		dsq->sound->playSfx("naijalow1");
	}

	switch (dsq->nagType)
	{
	case NAG_TOTITLE:
		dsq->title(true);
	break;
	default:
	case NAG_QUIT:
		dsq->quit();
	break;
	}
}

void Nag::onBuy()
{
	if (core->isNested()) return;

	if (click == 1)
	{
		click = 0;
		return;
	}

	if (core->isFullscreen())
	{
		core->setFullscreen(false);
		dsq->run(0.6f);
	}

	std::ostringstream os;
	os << "naijagiggle" << randRange(1, 5);
	dsq->sound->playSfx(os.str());

	dsq->run(0.5f);

	click = 1;

	openURL("http://www.bit-blot.com/aquaria/buy.html");

	hitBuy = true;
}


