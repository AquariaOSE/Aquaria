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
#include "AquariaMenuItem.h"
#include "../BBGE/Gradient.h"
#include "../BBGE/Particles.h"
#include "Hair.h"
#include "Game.h"


ParticleEditor *pe = 0;


ParticleEditor::ParticleEditor() : StateObject()
{
	registerState(this, "ParticleEditor");
}

void ParticleEditor::applyState()
{
	StateObject::applyState();

	ActionMapper::clearActions();
	ActionMapper::clearCreatedEvents();

	addAction(MakeFunctionEvent(ParticleEditor, load), KEY_F1, 0);
	addAction(MakeFunctionEvent(ParticleEditor, reload), KEY_F5, 0);


	addAction(MakeFunctionEvent(ParticleEditor, start), MOUSE_BUTTON_LEFT, 0);
	addAction(MakeFunctionEvent(ParticleEditor, stop), MOUSE_BUTTON_RIGHT, 0);

	addAction(MakeFunctionEvent(ParticleEditor, goToTitle), KEY_ESCAPE, 0);

	addAction(MakeFunctionEvent(ParticleEditor, toggleHair), KEY_F9, 0);

	Gradient *grad = new Gradient;
	grad->scale = Vector(800, 600);
	grad->position = Vector(400,300);
	grad->makeVertical(Vector(0.4f, 0.4f, 0.4f), Vector(0.8f, 0.8f, 0.8f));
	addRenderObject(grad, LR_BACKDROP);

	core->cameraPos = Vector(0,0);

	emitter = new ParticleEffect;


	addRenderObject(emitter, LR_ENTITIES);

	dsq->overlay->alpha.interpolateTo(0, 0.5);

	hair = new Hair(20,12,32);
	hair->setTexture("eel-0001");
	hair->followCamera = 1;
	addRenderObject(hair, LR_PARTICLES);
	hair->alpha = 0;

	test = new Quad;
	test->color = 0;
	test->scale = Vector(64,64);
	test->position = Vector(400,300);
	test->offset = Vector(0,-80);
	addRenderObject(test, LR_PARTICLES);
	test->alpha = 0;
}

void ParticleEditor::toggleHair()
{
	if (hair)
	{
		hair->toggleAlpha();
	}
	if (test)
		test->toggleAlpha();
}

void ParticleEditor::removeState()
{
	StateObject::removeState();
}

void ParticleEditor::update(float dt)
{
	StateObject::update(dt);
	if (emitter)
	{
		emitter->position = core->mouse.position;
	}

	if (hair)
	{
		hair->setHeadPosition(core->mouse.position);
		hair->updatePositions();
	}

	if (test)
	{
		test->offset.rotate2DRad(PI*dt);
	}
}

void ParticleEditor::goToTitle()
{
	if (!dsq->returnToScene.empty())
		game->transitionToScene(dsq->returnToScene);
	else
		dsq->title(false);
}

void ParticleEditor::load()
{
	particleManager->loadParticleBank(dsq->particleBank1, dsq->particleBank2);
	emitter->stop();
	std::string pname = dsq->getUserInputString(stringbank.get(2018));
	lastLoadedParticle = pname;
	emitter->load(pname);
	emitter->start();
}

void ParticleEditor::reload()
{
	particleManager->loadParticleBank(dsq->particleBank1, dsq->particleBank2);
	emitter->stop();
	emitter->load(lastLoadedParticle);
	emitter->start();
}

void ParticleEditor::start()
{
	emitter->load(lastLoadedParticle);
	emitter->start();
}

void ParticleEditor::stop()
{
	emitter->stop();
}
