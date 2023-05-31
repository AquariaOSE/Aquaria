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
#include "Particles.h"
#include "Game.h"

namespace AQCredits
{
	Quad *bg1=0, *bg2=0;

	Quad *alec=0, *derek=0, *jenna=0;

	std::vector<Quad*> slides;

	void watchSlide(size_t slide)
	{
		float t = 10;

		if (slide >= slides.size()) return;

		Quad *q = slides[slide];

		q->alpha.ensureData();
		q->alpha.data->path.addPathNode(0, 0);
		q->alpha.data->path.addPathNode(1, 0.5);
		q->alpha.data->path.addPathNode(0, 1);
		q->alpha.startPath(t);

		q->scale.ensureData();
		q->scale.data->path.addPathNode(Vector(0.8f, 0.8f), 0);
		q->scale.data->path.addPathNode(Vector(1.4f, 1.4f), 1);
		q->scale.startPath(t);

		core->run(t);
	}

	void cred(Quad *cred, bool show)
	{
		float t= 3;
		cred->scale = Vector(0.7f, 0.7f);
		if (show)
		{
			cred->alpha.interpolateTo(0.7f, t, 0, 0, 1);
			cred->offset.interpolateTo(Vector(100, 0), 60);
		}
		else
			cred->alpha.interpolateTo(0, t, 0, 0, 1);
	}
}

using namespace AQCredits;

Credits::Credits() : StateObject()
{
	registerState(this, "Credits");
}

void Credits::applyState()
{
	StateObject::applyState();

#ifndef AQUARIA_DEMO

	dsq->setCutscene(1,0);
	core->resetCamera();

	core->sound->stopMusic();

	// load everything here:

	bg1 = new Quad("particles/gas", Vector(400,300));
	bg1->setWidthHeight(1424, 1424);
	bg1->color = Vector(0,0,1);
	bg1->followCamera = 1;
	bg1->rotation.interpolateTo(Vector(0, 0, 360), 20, -1);
	bg1->alpha = 0.5;
	bg1->setSegs(32, 32, 0.5f, 0.5f, 0.008f, 0.008f, 2.0f, 1);
	addRenderObject(bg1, LR_BACKGROUND);

	bg2 = new Quad("particles/gas", Vector(400,300));
	bg2->setWidthHeight(1424, 1424);
	bg2->color = Vector(0,0,1);
	bg2->followCamera = 1;
	bg2->rotation.interpolateTo(Vector(0, 0, -360), 20, -1);
	bg2->alpha = 0.5;
	bg2->setSegs(32, 32, 0.5f, 0.5f, 0.008f, 0.008f, 2.0f, 1);
	addRenderObject(bg2, LR_BACKGROUND);

	alec = new Quad("credits/alec", Vector(200, 500));
	alec->alpha = 0;
	addRenderObject(alec, LR_HUD);

	derek = new Quad("credits/derek", Vector(200, 500));
	derek->alpha = 0;
	addRenderObject(derek, LR_HUD);

	jenna = new Quad("credits/jenna", Vector(200, 500));
	jenna->alpha = 0;
	addRenderObject(jenna, LR_HUD);

	ParticleEffect *bubbles = new ParticleEffect();
	bubbles->load("credits_idle");
	bubbles->position = Vector(400,300);
	bubbles->followCamera = 1;
	addRenderObject(bubbles, LR_PARTICLES);
	bubbles->start();

	int numSlides = 16;

	slides.resize(numSlides);

	for (size_t i = 0; i < slides.size(); i++)
	{
		slides[i] = new Quad("credits/slide-" + numToZeroString(i, 4), Vector(400, 300));
		slides[i]->alpha = 0;
		slides[i]->followCamera = 1;
		slides[i]->setBlendType(BLEND_ADD);
		addRenderObject(slides[i], LR_ENTITIES);
	}

	// start:
	core->run(1);

	dsq->sound->playMusic("losttothewaves", SLT_NONE, SFT_CROSS, 1);

	dsq->overlay->alpha.interpolateTo(0, 12);
	dsq->overlay2->alpha.interpolateTo(0, 12);

	core->run(12);

	core->run(6);

	cred(derek, true);

	watchSlide(0);
	watchSlide(1);
	watchSlide(2);

	cred(derek, false);
	cred(alec, true);

	watchSlide(3);
	watchSlide(4);
	watchSlide(5);

	cred(alec, false);
	cred(jenna, true);

	watchSlide(6);
	watchSlide(7);
	watchSlide(8);

	cred(jenna, false);

	watchSlide(9);
	watchSlide(10);
	watchSlide(11);
	watchSlide(12);
	watchSlide(13);
	watchSlide(14);
	watchSlide(15);

	dsq->overlay->alpha.interpolateTo(1, 6);
	core->run(6);

	while (dsq->sound->isPlayingMusic() && !dsq->isSkippingCutscene())
	{
		core->run(1);
	}

	dsq->setCutscene(0);

	game->transitionToScene("thirteenlair");
#endif

}

void Credits::removeState()
{
	StateObject::removeState();
}

void Credits::update(float dt)
{
	StateObject::update(dt);
}
