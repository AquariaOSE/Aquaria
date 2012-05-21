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

Bubble::Bubble() : Quad()
{
	setTexture("particles/bubble");
	/*
	this->life = 10;
	this->decayRate = 1;
	*/
	this->alpha = 0.5f;
	this->speed = rand()%10+20;
	this->width = 32;
	this->height = 32;
	//this->scale = Vector(0.5f, 0.5f, 0.5f);
}

void Bubble::onUpdate(float dt)
{
	Quad::onUpdate(dt);
	this->position -= Vector(0, speed)*dt;
	if (position.y < -32)
		position.y = position.y + 600+64;
}

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

	//addAction(MakeFunctionEvent(Intro2, skipIntro), KEY_ESCAPE, 0);

	dsq->subtitlePlayer.show(0.5f);

	core->resetCamera();
	
	dsq->jiggleCursor();

	dsq->setCutscene(1,1);

	core->main(1);
	dsq->overlay->alpha.interpolateTo(0, 40);
	dsq->toggleCursor(0);

	// OLD WAY of skipping
	//dsq->user.control.actionSet.importAction(this, "Escape",		ACTION_ESC);

	Quad *frame4 = new Quad;
	{
		frame4->setTexture("gameover-0004");
		frame4->position = Vector(400,310);
		frame4->setWidthHeight(600, 600);
		frame4->setSegs(2, 32, 0.1, 0.1, 0.002, 0.003, 2.0, 1);
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
		dsq->main(FRAME_TIME);

	/*
	while (dsq->sound->isPlayingVoice())
	{
		if (isActing(ACTION_ESC))
		{
			while (isActing(ACTION_ESC) && dsq->sound->isPlayingVoice())
			{
				core->main(FRAME_TIME);
			}
			dsq->overlay->alpha.interpolateTo(1, 1);
			dsq->stopVoice();
			dsq->main(1);
			dsq->newGame();
			return;
		}
		core->main(FRAME_TIME);
	}
	*/

	/*
	Quad *frame3 = new Quad;
	{
		frame3->setTexture("gameover-0002");
		frame3->position = Vector(400,300);
		frame3->width = 600;
		frame3->height = 600;
	}
	addRenderObject(frame3, LR_BACKGROUND);

	Quad *frame2 = new Quad;
	{
		frame2->setTexture("gameover-0003");
		frame2->position = Vector(400,300);
		frame2->width = 600;
		frame2->height = 600;
	}
	addRenderObject(frame2, LR_BACKGROUND);

	Quad *frame1 = new Quad;
	{
		frame1->setTexture("gameover-0004");
		frame1->position = Vector(400,300);
		frame1->width = 600;
		frame1->height = 600;
	}
	addRenderObject(frame1, LR_BACKGROUND);
	*/

	/*
	dsq->voice("Naija_Intro1");
	while (dsq->isStreamingVoice())		core->main(1);
	*/

	/*
	frame1->alpha.interpolateTo(0, 1);
	core->main(1);
	*/

	/*
	dsq->voice("Naija_Intro2");
	while (dsq->isStreamingVoice())		core->main(1);
	*/

	//frame2->alpha.interpolateTo(0, 1);

	/*
	dsq->voice("Naija_Intro3");
	while (dsq->isStreamingVoice())		core->main(1);
	*/

	/*
	frame3->alpha.interpolateTo(0, 1);
	core->main(1);
	*/

	/*
	dsq->voice("Naija_Intro3");
	while (dsq->isStreamingVoice())		core->main(1);
	*/

	//frame4->alpha.interpolateTo(0, 1);

	/*
	dsq->voice("Naija_Intro4");
	while (dsq->isStreamingVoice())		core->main(1);
	*/

	emitter->stop();
	emitter2->start();
	core->main(0.5);
	core->sound->playSfx("NormalForm");
	dsq->overlay->color = Vector(1,1,1);
	dsq->overlay->alpha = 0;
	dsq->fade(1, 1);
	core->main(1);

	frame4->alpha = 0;
	dsq->overlay->color.interpolateTo(0, 1);
	core->main(1);

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
	const bool frameOutputGameOver = false;

	core->sound->fadeMusic(SFT_OUT, 1);
	//float transTime = 0.01;
	//core->sound->fadeOut(transTime);
	StateObject::applyState();
	core->globalScale = Vector(1,1);
	core->cameraPos.stop();
	core->cameraPos = Vector(0,0,0);

	core->sound->playSfx("Death");

	if (frameOutputGameOver)
	{
		dsq->fpsText->alpha = 0;
		core->frameOutputMode = true;
	}

	Quad *q = new Quad;
	{
		q->color = 0;
		q->setWidthHeight(800, 600);
		q->position = Vector(400,300,-0.1);
	}
	addRenderObject(q);

	/*
	BitmapText *b = new BitmapText(&dsq->font);
	{
		int sz = 64;
		b->setFontSize(sz);
		b->setText("GAME OVER");
		b->position = Vector(400+sz/2,300-sz/2);
	}
	addRenderObject(b);
	*/
	//core->main(transTime);



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
		shockLines->setBlendType(RenderObject::BLEND_ADD);
		shockLines->scale.interpolateTo(Vector(4,4), 1);
	}
	addRenderObject(shockLines, LR_BACKGROUND);

	core->main(0.033);
	if (core->afterEffectManager)
	{
		core->afterEffectManager->clear();
		core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter, 0.07,0.03,30,0.2f, 1.1));
	}

	//dsq->screenTransition->transition(0);
	dsq->overlay->alpha = 0;

	//core->main(0.1);
	//frame3->alpha.interpolateTo(0, GO_ANIM_TIME);

	core->main(GO_ANIM_TIME);
	frame4->alpha.interpolateTo(0, GO_ANIM_TIME);
	core->main(GO_ANIM_TIME);

	frame3->alpha.interpolateTo(0, GO_ANIM_TIME);
	core->main(GO_ANIM_TIME);

	frame2->alpha.interpolateTo(0, GO_ANIM_TIME);
	core->main(GO_ANIM_TIME);

	frame1->alpha.interpolateTo(0, GO_ANIM_TIME);
	core->main(GO_ANIM_TIME);

	core->main(1.5);
	//core->sound->streamMusic("Requiem", 0);


	if (dsq->recentSaveSlot != -1)
	{
		// game over recent save load
		dsq->sound->stopMusic();
		float transferSeconds = dsq->continuity.seconds;
		dsq->continuity.loadFile(dsq->recentSaveSlot);
		
		/*
		//float lastLoadSeconds = dsq->continuity.seconds;
		// time spent on a session that ended with death is the 
		// difference between the current total time and the last save time?
		// nope.
		// ignore doing the above for now!
		*/
		
		dsq->continuity.seconds = transferSeconds;
		dsq->game->transitionToScene(dsq->game->sceneToLoad);
	}
	else
		dsq->title();


	//core->main(transTime);

	if (frameOutputGameOver)
		core->frameOutputMode = false;

	/*
	addAction(MakeFunctionEvent(GameOver, onClick), ActionMapper::MOUSE_BUTTON_LEFT, 0);
	timer = 1;
	*/
}

void GameOver::removeState()
{
	//dsq->screenTransition->capture();
	StateObject::removeState();
	frame3 = frame2 = frame1 = 0;
}

void GameOver::onClick()
{
	if (timer == 0)
		dsq->title();
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

	/*
	if (frame1 && frame2 && frame3)
	{
		if (frame3->alpha.x == 0 && !frame3->alpha.isInterpolating())
		{
			if (frame2->alpha.x == 0 && !frame2->alpha.isInterpolating())
			{
				if (frame1->alpha.x == 0 && !frame1->alpha.isInterpolating())
				{
					frame1=frame2=frame3=0;
				}
				else if (!frame1->alpha.isInterpolating())
				{
					frame1->alpha.interpolateTo(0, 0.8);

				}
			}
			else if (!frame2->alpha.isInterpolating())
			{
				frame2->alpha.interpolateTo(0, GO_ANIM_TIME);
			}
		}
	}
	*/
}

namespace NagStuff
{
	std::vector<Quad*> irot;
	int ic=0;
	const int numScreens = 11;
	float screenTimer = 0;
	/*
	const float screenTime = 7;
	const float nagFadeTime = 3;
	*/
	const float screenTime = 3;
	const float nagFadeTime = 1;
};

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
	buy->position = Vector(630, 400); //300, 540);
	//buy->setLabel("Buy");
	buy->useGlow("particles/glow", 480, 128);
	buy->event.set(MakeFunctionEvent(Nag, onBuy));
	buy->setDirMove(DIR_LEFT, buy);
	buy->setDirMove(DIR_RIGHT, buy);
	buy->setDirMove(DIR_UP, buy);
	addRenderObject(buy, LR_BACKGROUND);

	AquariaMenuItem *exit = new AquariaMenuItem();
	exit->followCamera = 1;
	exit->position = Vector(732, 543);
	//exit->setLabel("Exit");
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

	//Vector framePos(235, 405);
	Vector framePos(240, 400);
	Vector frameScale(0.98, 0.98);

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
	dsq->main(1);

	dsq->sound->playMusic("openwaters3", SLT_NORMAL, SFT_IN, 2);
}

void Nag::removeState()
{
	dsq->toggleCursor(0);

	dsq->fade(1, 1);
	dsq->main(1);

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
		for (int i = 0; i < irot.size(); i++) irot[i]->alpha.interpolateTo(0, nagFadeTime);
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
		dsq->title();
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
		core->toggleScreenMode(0);
		dsq->main(0.6);
	}

	std::ostringstream os;
	os << "naijagiggle" << randRange(1, 5);
	dsq->sound->playSfx(os.str());

	dsq->main(0.5);

	click = 1;

	openURL("http://www.bit-blot.com/aquaria/buy.html");

	hitBuy = true;
}


