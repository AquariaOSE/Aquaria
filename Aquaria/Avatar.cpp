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
#include "../BBGE/MathFunctions.h"

#include "Avatar.h"
#include "Game.h"
#include "Shot.h"
#include "GridRender.h"
#include "Web.h"
#include "Hair.h"


const float MULT_DMG_CRABCOSTUME = 0.75f;
const float MULT_DMG_FISHFORM = 1.5f;
const float MULT_DMG_SEAHORSEARMOR = 0.6f;

const float MULT_MAXSPEED_BEASTFORM = 1.2f;
const float MULT_MAXSPEED_FISHFORM = 1.5f;

const float MULT_DMG_EASY	= 0.5f;

const float JELLYCOSTUME_HEALTHPERC		= 0.5f;
const float JELLYCOSTUME_HEALDELAY		= 2.0f;
const float	JELLYCOSTUME_HEALAMOUNT		= 0.5f;

const float biteTimerBiteRange = 0.6f;
const float biteTimerMax = 3;
const float biteDelayPeriod = 0.08f;
const size_t normalTendrilHits = 3;
const size_t rollTendrilHits = 4;
const size_t maxTendrilHits = 6;

const float fireDelayTime = 0.2f;
const int maxShieldPoints = 8;
const int minMouse = 60;
int SongIcon::notesOpen = 0;
Avatar *avatar = 0;
const Vector BLIND_COLOR = Vector(0.1f, 0.1f, 0.1f);
const float ANIM_TRANSITION	= 0.2f;
//const float MANA_RECHARGE_RATE = 1.0;
const int AURA_SHIELD_RADIUS = 64;
//const int TARGET_RANGE = 1024;
const int TARGET_RANGE = 1024; // 650
const int TARGET_GRACE_RANGE = 200;
//const int TARGET_RANGE = 700;
//const int TARGET_RANGE = 64;
const float NOTE_SCALE = 0.75f;
const int singingInterfaceRadius = 100;
const int openSingingInterfaceRadius = 128;
//164
const int BURST_DISTANCE = 200;
const int STOP_DISTANCE = 48;
const int maxMouse = BURST_DISTANCE;
//const int SHOCK_RANGE	= 700;
//const int SHOCK_RANGE	= 1000;
const int SPIRIT_RANGE	= 2000;

const float QUICK_SONG_CAST_DELAY = 0.4f;

const float BURST_RECOVER_RATE = 1.2f; // 3.0 // 0.75
const float BURST_USE_RATE = 1.5f; //0.9 //1.5;
const float BURST_ACCEL = 4000; //2000 // 1000

// Minimum time between two splash effects (seconds).
const float SPLASH_INTERVAL = 0.2f;

//const float TUMMY_TIME = 6.0;

//const float chargeMax = 2.0;

// Axis input distance (0.0-1.0) at which we start moving.
const float JOYSTICK_LOW_THRESHOLD = 0.2f;
// Axis input distance at which we move full speed.
const float JOYSTICK_HIGH_THRESHOLD = 0.6f;
// Axis input distance at which we accept a note.
const float JOYSTICK_NOTE_THRESHOLD = 0.6f;

// Mouse cursor distance (from note icon, in virtual pixels) below which
// we accept a note.
const float NOTE_ACCEPT_DISTANCE = 25;
// Joystick input angle offset (from note icon, in degrees) below which
// we accept a note.
const float NOTE_ACCEPT_ANGLE_OFFSET = 15;

const int COLLIDE_RADIUS_NORMAL = 10;
const int COLLIDE_RADIUS_FISH = 8;

const int COLLIDE_RANGE_NORMAL = 2;
const int COLLIDE_RANGE_FISH = 1;

const float COLLIDE_MOD_NORMAL = 1.0f;
const float COLLIDE_MOD_FISH = 0.1f;

const int requiredDualFormCharge = 3;


Vector Target::getWorldPosition()
{
	Vector ret;
	if (e)
	{
		ret = e->getTargetPoint(targetPt);
	}
	return ret;
}

void Avatar::bindInput()
{
	ActionMapper::clearActions();
	ActionMapper::clearCreatedEvents();

	for(size_t i = 0; i < dsq->user.control.actionSets.size(); ++i)
	{
		const ActionSet& as = dsq->user.control.actionSets[i];
		int sourceID = (int)i;

		as.importAction(this, "PrimaryAction", ACTION_PRIMARY, sourceID);
		as.importAction(this, "SecondaryAction", ACTION_SECONDARY, sourceID);

		as.importAction(this, "SwimUp",		ACTION_SWIMUP, sourceID);
		as.importAction(this, "SwimDown",		ACTION_SWIMDOWN, sourceID);
		as.importAction(this, "SwimLeft",		ACTION_SWIMLEFT, sourceID);
		as.importAction(this, "SwimRight",		ACTION_SWIMRIGHT, sourceID);

		as.importAction(this, "SongSlot1",		ACTION_SONGSLOT1, sourceID);
		as.importAction(this, "SongSlot2",		ACTION_SONGSLOT2, sourceID);
		as.importAction(this, "SongSlot3",		ACTION_SONGSLOT3, sourceID);
		as.importAction(this, "SongSlot4",		ACTION_SONGSLOT4, sourceID);
		as.importAction(this, "SongSlot5",		ACTION_SONGSLOT5, sourceID);
		as.importAction(this, "SongSlot6",		ACTION_SONGSLOT6, sourceID);
		as.importAction(this, "SongSlot7",		ACTION_SONGSLOT7, sourceID);
		as.importAction(this, "SongSlot8",		ACTION_SONGSLOT8, sourceID);
		as.importAction(this, "SongSlot9",		ACTION_SONGSLOT9, sourceID);
		as.importAction(this, "SongSlot10",	ACTION_SONGSLOT10, sourceID);

		as.importAction(this, "Revert",		ACTION_REVERT, sourceID);
		as.importAction(this, "Look",			ACTION_LOOK, sourceID);
		as.importAction(this, "Roll",			ACTION_ROLL, sourceID);
	}
}

// note: z is set to 1.0 when we want the aim to be used as the shot direction
// otherwise the shot will head straight to the target
Vector Avatar::getAim()
{
	Vector d;
	if (dsq->getInputMode() == INPUT_JOYSTICK)
	{
		for(size_t i = 0; i < core->getNumJoysticks(); ++i)
			if(Joystick *j = core->getJoystick(i))
				if(j->isEnabled() && !j->rightStick.isZero())
				{
					d = j->rightStick * 300;
					d.z = 1;
					break;
				}

		if(d.isZero())
			for(size_t i = 0; i < core->getNumJoysticks(); ++i)
				if(Joystick *j = core->getJoystick(i))
					if(j->isEnabled() && !j->position.isZero())
					{
						d = j->position * 300;
						break;
					}
	}
	else
	{
		d = dsq->getGameCursorPosition() - position;
		d.z = 1;
	}

	if (d.isZero())
		d = getForwardAim();

	return d;
}

Vector Avatar::getForwardAim()
{
	Vector aim = getForward();
	// getForward() points toward Naija's head, but it makes more sense
	// to shoot in the direction she's facing, so rotate the aim vector.
	aim = getRotatedVector(aim, isfh() ? 90 : -90);
	return aim;
}

void Avatar::onAnimationKeyPassed(int key)
{
	Entity::onAnimationKeyPassed(key);
}

Vector randCirclePos(Vector position, int radius)
{
	float a = ((rand()%360)*(2*PI))/360.0f;
	return position + Vector(sinf(a), cosf(a))*radius;
}

SongIconParticle::SongIconParticle(Vector color, Vector pos, size_t note)
								:  note(note)
{
	cull = false;
	//fastTransform = true;
	setTexture("particles/glow");

	setWidthHeight(32);

	float life = 1.0;

	toIcon = 0;
	this->color = color;
	position = pos;

	alpha.ensureData();
	alpha.data->path.addPathNode(0, 0);
	alpha.data->path.addPathNode(0.4f, 0.2f); // .8
	alpha.data->path.addPathNode(0.2f, 0.8f); // .4
	alpha.data->path.addPathNode(0, 1);
	alpha.startPath(life);

	scale.ensureData();
	scale.data->path.addPathNode(Vector(0.5f,0.5f), 0);
	scale.data->path.addPathNode(Vector(1,1), 0.5f);
	scale.data->path.addPathNode(Vector(0.5f,0.5f), 1);
	scale.startPath(life);

	setLife(life);
	setDecayRate(1);

	//if (rand()%6 <= 2)
	setBlendType(BLEND_ADD);

	float smallestDist = HUGE_VALF;
	SongIcon *closest = 0;
	for (size_t i = 0; i < avatar->songIcons.size(); i++)
	{
		if (i != note)
		{
			Vector diff = (position - avatar->songIcons[i]->position);
			float dist = diff.getSquaredLength2D();
			if (dist < smallestDist)
			{
				smallestDist = dist;
				closest = avatar->songIcons[i];
			}
		}
	}
	// find nearest song icon
	if (closest)
	{
		toIcon = closest;
	}
}

void SongIconParticle::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	if (toIcon)
	{
		Vector add = (toIcon->position - position);
		add.setLength2D(200*dt);
		velocity += add;
		velocity.capLength2D(50);
	}
}

SongIcon::SongIcon(size_t note) : Quad(), note(note)
{
	open = false;
	alphaMod = 0.9f;
	/*
	std::ostringstream os;
	os << "SongIcon" << note;
	setTexture(os.str());
	*/
	//setTexture("Cursor-Sing");
	std::ostringstream os;
	os << "Song/NoteSymbol" << note;
	os.str();
	setTexture(os.str());

	scale = Vector(NOTE_SCALE, NOTE_SCALE);
	cursorIsIn = false;
	delay = 0;
	counter = 0;
	width = 40;
	height = 40;

	minTime = 0;
	ptimer = 0;
	noteColor = dsq->getNoteColor(note);
	//color = dsq->getNoteColor(note)*0.75f + Vector(1,1,1)*0.25f;
	color = dsq->getNoteColor(note);
	len = 0;

	channel = BBGE_AUDIO_NOCHANNEL;

	rippleTimer = 0;

	glow = new Quad;
	glow->setTexture("particles/bigglow");
	glow->followCamera = 1;
	glow->rotation.interpolateTo(Vector(0,0,360), 10, -1);
	glow->alpha = 0;
	glow->setBlendType(BLEND_ADD);
	glow->scale = Vector(0.5, 0.5);
	glow->color = dsq->getNoteColor(note);
	game->addRenderObject(glow, LR_PARTICLES2);
}

void SongIcon::destroy()
{
	Quad::destroy();
}

void SongIcon::spawnParticles(float dt)
{
	float intv = 0.1f;
	// do stuff!
	ptimer += dt;
	while (ptimer > intv)
	{
		ptimer -= intv;
		SongIconParticle *s = new SongIconParticle(noteColor, randCirclePos(position, 16), note);
		s->followCamera = true;
		game->addRenderObject(s, LR_HUD);
	}
}

void SongIcon::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	if (!avatar->singing)
		return;

	if (alpha.x == 0 && !alpha.isInterpolating())
		alpha.interpolateTo(0.3f, 0.1f);
	if (delay > 0)
	{
		delay -= dt;
		if (delay < 0)
		{
			delay = 0;
			//channel = BBGE_AUDIO_NOCHANNEL;
		}
	}
	if (counter > 0)
	{
		counter -= dt;
		if (counter < 0)
		{
			counter = 0;
			closeNote();
		}
	}
	if (alpha.x > 0.5f)
	{
		spawnParticles(dt);
	}
	if (open)
	{
		len += dt;
		avatar->setHeadTexture("Singing", 0.1f);
	}
	if (alpha.x == 1)
	{
		if (isCoordinateInRadius(core->mouse.position, NOTE_ACCEPT_DISTANCE))
		{
			//if (delay == 0)
			if (true)
			{
				if (!cursorIsIn)
				// highlighted for the first time
				{
					cursorIsIn = true;
					openNote();
				}
				else
				{
					if (minTime > 0)
					{
						minTime -= dt;
						if (minTime < 0)
						{
							minTime = 0;
						}
					}

				}
			}
		}
		else if (!isCoordinateInRadius(core->mouse.position, NOTE_ACCEPT_DISTANCE*1.25f))
		{
			if (cursorIsIn)
			{
				cursorIsIn = false;
				closeNote();
			}
		}
	}
	if (alpha.x <= 0 && delay == 0) //  && channel != BBGE_AUDIO_NOCHANNEL
	{
		closeNote();
	}

	if (open)
	{
		if (dsq->user.video.noteEffects)
		{
			rippleTimer -= dt;
			if (rippleTimer <= 0)
			{
				//rippleTimer = 1.0f - ((7 - note)/7.0f)*0.7f;
				rippleTimer = 0.5f - (note/7.0f)*0.4f;

				if (core->afterEffectManager)
				{
					core->afterEffectManager->addEffect(new ShockEffect(position - Vector(400, 300) + Vector(core->width/2, core->height/2),
					core->screenCenter,0.009f,0.015f,18,0.2f, 0.9f + (note*0.08f) ));
				}
			}
		}
	}

	if (glow)
	{
		glow->position = position;
	}
}

void SongIcon::openNote()
{
	//if (delay > 0) return;
	scale.interpolateTo(Vector(1.2f, 1.2f), 0.1f);

	if (dsq->user.video.noteEffects)
	{
		glow->scale = Vector(0.5f,0.5f);
		glow->scale.interpolateTo(Vector(1.0f, 1.0f), 2, -1, 1, 1);

		glow->alpha.interpolateTo(0.6f, 0.2f, 0, 0, 1);
	}

	/*
	std::ostringstream os;
	os << "Note"
	*/

	std::string sfx = game->getNoteName(note);

	open = true;

	internalOffset = Vector(-5, 0);
	internalOffset.interpolateTo(Vector(5, 0), 0.08f, -1, 1);

	avatar->singNote(this->note);

	// this should never get called:
	if (channel != BBGE_AUDIO_NOCHANNEL)
	{
		dsq->sound->fadeSfx(channel, SFT_OUT, 0.2f);
		//dsq->sound->fadeSfx(channel, SFT_OUT, 0.2);
		channel = BBGE_AUDIO_NOCHANNEL;
	}
	//dsq->sound->stopSfx(channel);


	PlaySfx play;
	play.name = sfx;
	channel = dsq->sound->playSfx(play);


	rippleTimer = 0;

	minTime = 0.05f;
	counter = 3.2f;

	float glowLife = 0.5;

	{
	Quad *q = new Quad("particles/glow", position);
	q->scale.interpolateTo(Vector(10, 10), glowLife+0.1f);
	q->alpha.ensureData();
	q->alpha.data->path.addPathNode(0,0);
	q->alpha.data->path.addPathNode(0.75f,0.2f);
	q->alpha.data->path.addPathNode(0,1);
	q->alpha.startPath(glowLife);
	q->color = dsq->getNoteColor(note); //*0.5f + Vector(0.5, 0.5, 0.5)
	q->setBlendType(BLEND_ADD);
	q->followCamera = 1;
	game->addRenderObject(q, LR_HUD);
	q->setDecayRate(1/(glowLife+0.1f));
	}

	{
	std::ostringstream os2;
	os2 << "Song/NoteSymbol" << note;

	Quad *q = new Quad(os2.str(), position);
	q->color = 0;
	q->scale = Vector(0.5,0.5);
	q->scale.interpolateTo(Vector(2, 2), glowLife+0.1f);
	//q->scale.interpolateTo(Vector(10, 10), glowLife+0.1f);
	q->alpha.ensureData();
	q->alpha.data->path.addPathNode(0,0);
	q->alpha.data->path.addPathNode(0.5f,0.2f);
	q->alpha.data->path.addPathNode(0,1);
	q->alpha.startPath(glowLife);
	//q->setBlendType(BLEND_ADD);
	q->followCamera = 1;
	game->addRenderObject(q, LR_HUD);
	q->setDecayRate(1/(glowLife+0.1f));
	}

	avatar->songInterfaceTimer = 1.0f;

	notesOpen++;
	/*
	std::ostringstream os2;
	os2 << "notesOpen: " << notesOpen;
	debugLog(os2.str());
	*/
	if (notesOpen > 0)
	{
		len = 0;

		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if ((e->position - game->avatar->position).getSquaredLength2D() < sqr(1000))
			{
				e->songNote(note);
			}
		}
		for (size_t i = 0; i < game->getNumPaths(); i++)
		{
			Path *p = game->getPath(i);
			if (!p->nodes.empty())
			{
				if ((p->nodes[0].position - game->avatar->position).getSquaredLength2D() < sqr(1000))
				{
					p->songNote(note);
				}
			}
		}
	}
}

void SongIcon::closeNote()
{
	//if (delay > 0) return;
	scale.interpolateTo(Vector(NOTE_SCALE, NOTE_SCALE), 0.1f);

	if (game->avatar->isSinging() && dsq->user.video.noteEffects)
		glow->alpha.interpolateTo(0.3f, 1.5f, 0, 0, 1);
	else
		glow->alpha.interpolateTo(0, 1.5f, 0, 0, 1);
	glow->scale.interpolateTo(Vector(0.5f, 0.5f), 0.5f);


	cursorIsIn = false;

	if (channel != BBGE_AUDIO_NOCHANNEL)
	{
		dsq->sound->fadeSfx(channel, SFT_OUT, 1.0);
		channel = BBGE_AUDIO_NOCHANNEL;
		//delay = 0.5;
	}

	if (open)
	{
		internalOffset.stop();
		internalOffset = Vector(0,0);
		notesOpen--;
		open = false;

		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			int dist = (e->position - game->avatar->position).getSquaredLength2D();
			if (e != game->avatar && dist < sqr(1000))
			{
				e->songNoteDone(note, len);
			}
		}
		for (size_t i = 0; i < game->getNumPaths(); i++)
		{
			Path *p = game->getPath(i);
			if (!p->nodes.empty())
			{
				if ((p->nodes[0].position - game->avatar->position).getSquaredLength2D() < sqr(1000))
				{
					p->songNoteDone(note, len);
				}
			}
		}
	}

	/*
	std::ostringstream os;
	os << "notesOpen: " << notesOpen;
	debugLog(os.str());
	*/

	if (notesOpen <= 0)
	{
		notesOpen = 0;
		if (dsq->continuity.form == FORM_NORMAL)
			avatar->setHeadTexture("");
	}
}

void SongIcon::openInterface()
{
	delay = 0;
	alpha.interpolateTo(1, 0.1f);
}

void SongIcon::closeInterface()
{
	closeNote();
	delay = 0;
	alpha.interpolateTo(0, 0.1f);
}

AvatarState::AvatarState()
{
	abilityDelay = 0;
	outOfWaterTimer = 0;
	backFlip = false;
	nearWall = false;
	wasUnderWater = true;
	blind = false;
	lockedToWall = false;
	shotDelay = 0;
	spellCharge = 0;
	leachTimer = 0;
	swimTimer = 0;
	rollTimer = 0;
	updateLookAtTime = 0;
	lookAtEntity = 0;
	blinkTimer = 0;
}

void Avatar::toggleMovement(bool on)
{
	canMove = on;
}

bool Avatar::isLockable()
{
	return (bursting || !_isUnderWater) && (boneLockDelay == 0) && canLockToWall();
}

bool Avatar::isSinging()
{
	return singing;
}

void Avatar::applyWorldEffects(WorldType type)
{
	static bool oldfh=false;

	if (type == WT_SPIRIT)
	{
		//skeletalSprite.transitionAnimate("ball", 0.1, -1);
		//skeletalSprite.alpha.interpolateTo(0, 1);
		//skeletalSprite.alpha = 0;
		//game->addRenderObject(&skeletalSprite, LR_ENTITIES);

		removeChild(&skeletalSprite);
		skeletalSprite.position = position;
		skeletalSprite.setFreeze(true);
		skeletalSprite.scale = scale;
		skeletalSprite.alpha.interpolateTo(0.5, 1);
		//game->addRenderObject(&skeletalSprite, LR_ENTITIES);
		skeletalSprite.rotation.z = rotation.z;
		skeletalSprite.rotationOffset.z = rotationOffset.z;


		oldfh = skeletalSprite.isfh();
		skeletalSprite.fhTo(isfh());

		renderQuad = true;
		setTexture("glow");
		width = 256;
		height = 256;
		setBlendType(BLEND_ADD);
		fader->alpha.interpolateTo(0.75, 1);

		dsq->sound->toggleEffectMusic(SFX_FLANGE, true);
	}
	else
	{
		//skeletalSprite.transitionAnimate("idle", 1, -1);
		//skeletalSprite.alpha.interpolateTo(1, 1);
		//skeletalSprite.alpha = 1;
		//game->removeRenderObject(&skeletalSprite);

		skeletalSprite.setFreeze(false);
		if (!skeletalSprite.getParent())
		{
			addChild(&skeletalSprite, PM_NONE);
		}
		skeletalSprite.position = Vector(0,0,0);
		skeletalSprite.scale = Vector(1,1,1);
		renderQuad = false;
		setBlendType(BLEND_DEFAULT);
		fader->alpha.interpolateTo(0, 1);

		bool newfh = skeletalSprite.isfh();
		skeletalSprite.fhTo(oldfh);
		skeletalSprite.rotation.z = 0;
		skeletalSprite.rotationOffset.z = 0;

		fhTo(newfh);

		dsq->sound->toggleEffectMusic(SFX_FLANGE, false);
	}
}

void Avatar::startFlourish()
{
	std::string anim = dsq->continuity.getInternalFormName() + "-flourish";
	//if (skeletalSprite.getAnimation(anim))
	Animation *fanim = skeletalSprite.getAnimation(anim);
	if (fanim)
	{
		flourishTimer.start(fanim->getAnimationLength()-0.2f);
		flourishPowerTimer.start(fanim->getAnimationLength()*0.5f);
	}
	skeletalSprite.transitionAnimate(anim, 0.1f, 0, ANIMLAYER_FLOURISH);
	flourish = true;

	float rotz = rotationOffset.z;
	if (this->isfh())
		rotationOffset = Vector(0,0,rotz+360);
	else
		rotationOffset = Vector(0,0,rotz-360);

	FormType f = dsq->continuity.form;
	if (f != FORM_NORMAL && f != FORM_BEAST && f != FORM_FISH && f != FORM_SUN && f != FORM_NATURE)
	{
		rotationOffset.z *= -1;
	}
	if (f == FORM_ENERGY || f == FORM_DUAL)
	{
		rotationOffset.z *= 2;
	}

	if (f == FORM_BEAST)
	{
		Vector v = getNormal();
		if (!v.isZero())
		{
			v *= 400;
			vel += v;
		}
	}

	rotationOffset.interpolateTo(Vector(0,0,rotz), 0.8f, 0, 0, 1);
}

void Avatar::onIdle()
{
	if (game->li)
	{
		if (game->li->getState() == STATE_HUG && riding)
		{
			game->li->setState(STATE_IDLE);
		}
	}
	//stillTimer.stop();
	stopBurst();
	if (movingOn)
	{
		dsq->setMousePosition(Vector(400,300));
	}
	skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->stopAnimation();
	stopRoll();
	closeSingingInterface();
	fallOffWall();

	dsq->gameSpeed.stopPath();
	dsq->gameSpeed.interpolateTo(1,0);
}

std::string Avatar::getBurstAnimName()
{
	std::string ret;
	switch(dsq->continuity.form)
	{
	case FORM_ENERGY:
		ret = "energyburst";
	break;
	default:
		ret = "burst";
	break;
	}
	return ret;
}

std::string Avatar::getRollAnimName()
{
	std::string ret;
	switch(dsq->continuity.form)
	{
	case FORM_ENERGY:
		ret = "energyroll";
	break;
	default:
		ret = "roll";
	break;
	}
	return ret;
}

std::string Avatar::getIdleAnimName()
{
	std::string ret="idle";
	switch(dsq->continuity.form)
	{
	case FORM_ENERGY:
		ret="energyidle";
	break;
	case FORM_NORMAL:
	case FORM_BEAST:
	case FORM_NATURE:
	case FORM_SPIRIT:
	case FORM_DUAL:
	case FORM_FISH:
	case FORM_SUN:
	case FORM_MAX:
	case FORM_NONE:
		break;
	}
	return ret;
}

void Avatar::clampPosition()
{
	lastPosition = position;
}

void Avatar::updatePosition()
{
	updateHair(0);
}

void Avatar::updateHair(float dt)
{
	static float hairTimer = 0;
	Bone *b = skeletalSprite.getBoneByIdx(0);
	if (hair && b)
	{
		hair->alpha.x = alpha.x;
		hair->color.x = color.x * multColor.x;
		hair->color.y = color.y * multColor.y;
		hair->color.z = color.z * multColor.z;
		Vector headPos = b->getWorldCollidePosition(Vector(12,-32,0));

		hair->setHeadPosition(headPos);
		Vector diff = headPos - position;

		Vector diff2;
		if (!isfh())
			diff2 = diff.getPerpendicularLeft();
		else
			diff2 = diff.getPerpendicularRight();

		Vector diff3 = position - headPos;

		if (state.lockedToWall && wallPushVec.y < 0 && (fabsf(wallPushVec.y) > fabsf(wallPushVec.x)))
		{
			if (isfh())
			{
				diff3 = Vector(-50, -25);
			}
			else
				diff3 = Vector(50,-25);
		}

		float len =diff2.getLength2D();
		diff3.setLength2D(len);
		/*
		diff.y = -diff.y;
		diff = (diff + diff2)/2.0f;
		*/

		hairTimer += dt;
		while (hairTimer > 2.0f)
		{
			hairTimer -= 2.0f;
		}
		float useTimer = hairTimer;
		if (useTimer > 1.0f)
			useTimer = 1.0f - (hairTimer-1);
		float frc = 0.333333f;
		diff = (diff2*(frc*(1.0f-(useTimer*0.5f))) + diff3*(frc) + Vector(0,len)*(frc*(0.5f+useTimer*0.5f)));



		if (_isUnderWater)
		{
			diff.setLength2D(400);
			//if (!vel.isLength2DIn(10))
			hair->exertForce(diff, dt);
		}
		else
		{
			diff.setLength2D(400);
			hair->exertForce(diff, dt);
		}
		if (!vel2.isZero())
			hair->exertForce(vel2, dt);
		hair->updatePositions();
	}
}

void Avatar::updateDamageVisualEffects()
{
   	int damageThreshold = float(maxHealth/5.0f)*3.0f;
	Quad *damageSprite = game->damageSprite;
	if (health <= damageThreshold)
	{
		//game->damageSprite->alpha.interpolateTo(0.9, 0.5);
		float a = ((damageThreshold - health)/float(damageThreshold))*1.0f;
		damageSprite->alpha.interpolateTo(a, 0.3f);

		/*
		std::ostringstream os;
		os << "damageSprite alpha: " << a;
		debugLog(os.str());
		*/

		if(!damageSprite->scale.isInterpolating())
		{
			damageSprite->scale = Vector(1,1);
			damageSprite->scale.interpolateTo(Vector(1.2f, 1.2f), 0.5f, -1, 1);
		}

		/*
		if (health <= 0)
		{
			game->sceneColor.interpolateTo(Vector(1,0.5,0.5), 0.75);
		}
		*/
	}
	else
	{
		damageSprite->alpha.interpolateTo(0, 0.3f);
	}
}

void Avatar::checkUpgradeForShot(Shot *s)
{
	if (dsq->continuity.energyMult <= 1)
		s->extraDamage = dsq->continuity.energyMult;
	else
		s->extraDamage = dsq->continuity.energyMult * 0.75f;

	if (s->extraDamage > 0)
	{
		Quad *glow = new Quad("particles/glow", Vector(0,0));
		glow->color = Vector(1,0,0);
		glow->color.interpolateTo(Vector(1,0.5f,0.5f), 0.1f, -1, 1);
		glow->setBlendType(BLEND_ADD);
		glow->scale = Vector(4, 4) + (s->extraDamage*Vector(2,2));
		glow->scale.interpolateTo(Vector(16,16)+ (s->extraDamage*Vector(2,2)), 0.5f, -1, 1);
		s->addChild(glow, PM_POINTER);
	}
}

void Avatar::onDamage(DamageData &d)
{
	Entity::onDamage(d);

	skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->stopAnimation();
	if (dsq->continuity.form == FORM_NORMAL)
	{
		if (nocasecmp(dsq->continuity.costume, "CC")==0)
		{
			d.damage *= MULT_DMG_CRABCOSTUME;
		}
	}

	if (riding != 0)
	{
		if (nocasecmp(dsq->continuity.costume, "seahorse")==0)
		{
			d.damage *= MULT_DMG_SEAHORSEARMOR;
		}
	}

	if (dsq->continuity.form == FORM_FISH)
	{
		d.damage *= MULT_DMG_FISHFORM;
	}

	if ((core->isNested() && game->invincibleOnNested) || game->invinciblity)
	{
		d.damage = 0;
		d.damageType = DT_NONE;
		return;
	}

	if (d.damageType == DT_ENEMY_INK)
	{
		setBlind(d.effectTime);
		return;
	}

	if (d.damageType == DT_ENEMY_POISON)
	{
		dsq->continuity.setPoison(1, d.effectTime);
	}

	if (dsq->continuity.defenseMultTimer.isActive())
	{
		d.damage *= dsq->continuity.defenseMult;
	}

	if (dsq->continuity.invincibleTimer.isActive())
		d.damage = 0;

	if (!canDie)
	{
		if ((health - d.damage) <= 0)
		{
			float i = d.damage;
			while (i >= 0)
			{
				if ((health - i) > 0)
				{
					d.damage = i;
					break;
				}

				i -= 0.5f;
			}
		}
	}

	if ((!invincible || !game->invincibleOnNested) && !(invincibleBreak && damageTimer.isActive() && d.useTimer) && !dsq->continuity.invincibleTimer.isActive())
	{
		if (d.damageType == DT_ENEMY_ACTIVEPOISON)
			core->sound->playSfx("Poison");
		else
			core->sound->playSfx("Pain");


		setHeadTexture("Pain", 1);

		int r = (rand()%2)+1;
		std::ostringstream os;
		os << "basicHit" << r;
		skeletalSprite.transitionAnimate(os.str(), 0.05f, 0, ANIMLAYER_OVERRIDE);

		/*
		if (d.attacker)
		{
			// this will probably cause a crash!
			state.lookAtEntity = d.attacker;
		}
		*/

		if (d.damage > 0)
		{
			float healthWillBe = health-d.damage;
			// determines length of shader blur as well
			float t = 0.5f;
			if (healthWillBe<=0)
				t = 2;

			dsq->rumble(d.damage, d.damage, 0.4f, _lastActionSourceID, _lastActionInputDevice);
			if (d.damage > 0)
			{
				//dsq->shakeCamera(5, t);
				if (d.damage >= 1)
				{
					float shake = d.damage*2;
					if (shake > 10)
						shake = 10;
					dsq->shakeCamera(shake, t);
				}

				if (healthWillBe <= 2 && d.damageType != DT_ENEMY_ACTIVEPOISON)
				{
					//if (!dsq->gameSpeed.isInterpolating() && dsq->gameSpeed.x==1)

					{
						dsq->gameSpeed.stop();
						dsq->gameSpeed.stopPath();
						dsq->gameSpeed.x = 1;

						dsq->overlayRed->alpha.ensureData();
						dsq->overlayRed->alpha.data->path.clear();
						dsq->overlayRed->alpha.data->path.addPathNode(0, 0);
						dsq->overlayRed->alpha.data->path.addPathNode(1, 0);
						dsq->overlayRed->alpha.data->path.addPathNode(0, 1);
						dsq->overlayRed->alpha.startPath(1);

						dsq->sound->playSfx("heartbeat");

						if (healthWillBe < 2 && healthWillBe >= 1 && !game->hasPlayedLow)
						{
							dsq->emote.playSfx(EMOTE_NAIJALOW);
							game->hasPlayedLow = 1;
						}


						dsq->gameSpeed.ensureData();
						dsq->gameSpeed.data->path.clear();
						dsq->gameSpeed.data->path.addPathNode(1, 0);
						dsq->gameSpeed.data->path.addPathNode(0.25f, 0.1f);
						dsq->gameSpeed.data->path.addPathNode(0.25f, 0.4f);
						dsq->gameSpeed.data->path.addPathNode(1, 1);

						dsq->gameSpeed.startPath(2);

						//dsq->gameSpeed.interpolateTo(0.7, 3);
					}
					//dsq->emote();
				}
			}
			hitEmitter.load("NaijaHit");
			hitEmitter.start();

			playHitSound();
		}
	}
}

void Avatar::playHitSound()
{
	int hitSound = (rand()%8)+1;
	static int lastHitSound = 0;
	if (lastHitSound == hitSound)
	{
		hitSound ++;
		if (hitSound > 8)
			hitSound = 1;
	}
	std::ostringstream os;
	os << "hit" << hitSound;
	core->sound->playSfx(os.str());
}

void Avatar::onHealthChange(float change)
{
	updateDamageVisualEffects();
}

void Avatar::revive()
{
	entityDead	= false;
	health		= 0;
	heal(maxHealth);
}

void Avatar::updateDualFormChargeEffects()
{
}

void Avatar::lostTarget(int i, Entity *e)
{
	dsq->sound->playSfx("target-unlock");
}

void Avatar::entityDied(Entity *e)
{
	Entity::entityDied(e);
	for (size_t i = 0; i < targets.size(); i++)
	{
		if (targets[i].e == e)
		{
			lostTarget(i, 0);
			targets[i].e = 0;
			targetUpdateDelay = 100;
			targets.clear();
			break;
		}
	}

	if (state.lookAtEntity==e)
		state.lookAtEntity = 0;

	// eating
	if (e->isGoingToBeEaten())
	{
		EatType et = e->getEatType();
		switch(et)
		{
			case EAT_FILE:
			{
				dsq->continuity.eatBeast(e->eatData);
			}
			break;
		case EAT_DEFAULT:
		case EAT_MAX:
		case EAT_NONE:
			break;
		}
	}

	//debugLog("Entity died");
	//e->lastDamage.damageType == DT_AVATAR_ENERGYBLAST &&
	//debugLog("Entity died");
	if (e->lastDamage.form == FORM_DUAL && e->lastDamage.damageType == DT_AVATAR_SHOCK)
	{
		dsq->continuity.dualFormCharge ++;
		updateDualFormChargeEffects();
		dsq->spawnParticleEffect("SpiritSteal", e->position);
		//dsq->spawnParticleEffect("SpiritBeacon", position);
		core->sound->playSfx("DualForm-Absorb");
		if (dsq->continuity.dualFormCharge == requiredDualFormCharge)
			core->sound->playSfx("DualForm-Charge");
	}
	/*
	std::ostringstream os;
	os << "lastDamage.form = " << e->lastDamage.form;
	debugLog(os.str());
	*/
}

void Avatar::enableInput()
{
	ActionMapper::enableInput();
	game->toggleMiniMapRender(1);

	if (!game->isApplyingState())
		dsq->toggleCursor(true);

	if (movingOn)
	{
		dsq->setMousePosition(Vector(400,300));
	}

	if (dsq->continuity.form == FORM_ENERGY)
	{
		for (size_t i = 0; i < targetQuads.size(); i++)
			targetQuads[i]->start();
	}

	setInvincible(false);
	// can't do that here, cause it'll break the hug
	//stillTimer.stop();
}

void Avatar::disableInput()
{
	ActionMapper::disableInput();

	// can't do that here, cause it'll break the hug
	//stillTimer.stop();

	closeSingingInterface();
	game->toggleMiniMapRender(0);
	dsq->toggleCursor(false);
	endCharge();
	clearTargets();
	if (movingOn)
	{
		dsq->setMousePosition(Vector(400,300));
	}

	for (size_t i = 0; i < targetQuads.size(); i++)
	{
		targetQuads[i]->stop();
	}

	setInvincible(true);
}

void Avatar::clearTargets()
{
	for (size_t i = 0; i < targets.size(); i++)
	{
		if (targets[i].e)
		{
			lostTarget(i, 0);
		}
		targets[i].e = 0;
	}
}

void Avatar::openSingingInterface(InputDevice device)
{
	if (!singing && health > 0 && !isEntityDead() && !blockSinging)
	{
		//core->mouse.position = Vector(400,300);
		if (device != INPUT_MOUSE)
		{
			core->centerMouse();
			//core->setMousePosition(Vector(400,300));
		}

		core->setMouseConstraintCircle(core->center, singingInterfaceRadius);
		stopRoll();
		singing = true;
		currentSongIdx = SONG_NONE;

		// make the singing icons appear
		for (size_t i = 0; i < songIcons.size(); i++)
		{
			songIcons[i]->openInterface();
		}
		currentSong.notes.clear();

		songInterfaceTimer = 0;

		game->songLineRender->clear();


		if (device == INPUT_JOYSTICK)
		{
			core->setMousePosition(core->center);
		}
	}
}

void Avatar::closeSingingInterface()
{

	if (game->songLineRender)
		game->songLineRender->clear();
	if (singing)
	{
		core->setMouseConstraint(false);
		quickSongCastDelay = 1;

		// HACK: this prevents being "locked" away from the seahorse... so naija can
		// be in singing range of the seahorse
		applyRidingPosition();
		singing = false;

		for (size_t i = 0; i < songIcons.size(); i++)
		{
			songIcons[i]->closeInterface();
		}

		if (dsq->continuity.form == FORM_NORMAL)
			setHeadTexture("");

		currentSongIdx = dsq->continuity.checkSongAssisted(currentSong);
		if (currentSongIdx != SONG_NONE)
		{
			dsq->continuity.castSong(currentSongIdx);
			currentSongIdx = SONG_NONE;
		}
	}
}

void Avatar::toggleCape(bool on)
{
	if (!hair) return;

	if (!on)
		hair->alphaMod = 0;
	else
		hair->alphaMod = 1;
}

void Avatar::refreshDualFormModel()
{
	//charging = 0;
	if (dsq->continuity.dualFormMode == Continuity::DUALFORM_NAIJA)
		refreshModel("Naija", "DualForm_Naija");
	else if (dsq->continuity.dualFormMode == Continuity::DUALFORM_LI)
		refreshModel("Naija", "DualForm_Li");
}

void Avatar::updateDualFormGlow(float dt)
{
	if (dsq->continuity.form == FORM_DUAL && bone_dualFormGlow)
	{

		float perc = 1;
		if (requiredDualFormCharge != 0)
			perc = float(dsq->continuity.dualFormCharge)/float(requiredDualFormCharge);
		if (perc > 1)
			perc = 1;
		bone_dualFormGlow->alpha = perc*0.5f + 0.1f;
		bone_dualFormGlow->scale.interpolateTo(Vector(perc, perc), 0.2f);
	}
}

void Avatar::changeForm(FormType form, bool effects, bool onInit, FormType lastForm)
{
	/*
	if (core->afterEffectManager)
		core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter, 0.1,0.03,15,0.2f, 0.5));
	*/


	/*
	if (pullTarget)
	{
		pullTarget->stopPull();
		pullTarget = 0;
	}
	*/

	if (form == FORM_DUAL && !dsq->continuity.hasLi())
		return;

	if (!canChangeForm) return;

	std::ostringstream os;
	os << "changeForm: " << form;
	debugLog(os.str());

	/*
	if (dsq->game)
		game->clearControlHint();
	*/

	if (lastForm == FORM_NONE)
		lastForm = dsq->continuity.form;

	endCharge();

	std::ostringstream os2;
	os2 << "lastForm: " << lastForm;
	debugLog(os2.str());

	for (size_t i = 0; i < targetQuads.size(); i++)
	{
		if (targetQuads[i])
			targetQuads[i]->stop();
	}


	if (bone_dualFormGlow)
		bone_dualFormGlow->scale = 0;

	clearTargets();

	if (form != FORM_NORMAL)
		stopAura();

	switch (lastForm)
	{
	case FORM_FISH:
	{
		// check nearby area
		//bool isTooCloseToWall=false;

		if (isNearObstruction(3))
		{
			Vector n = game->getWallNormal(position);
			if (!n.isZero())
			{
				n *= 400;
				vel += n;
			}

			return;
		}
		//rotationOffset.interpolateTo(Vector(0,0,0), 0.5);

		collideRadius = COLLIDE_RADIUS_NORMAL;
		setCanLockToWall(true);
		setCollisionAvoidanceData(COLLIDE_RANGE_NORMAL, COLLIDE_MOD_NORMAL);
	}
	break;
	case FORM_SUN:
		lightFormGlow->alpha.interpolateTo(0, 0.5);
		lightFormGlowCone->alpha.interpolateTo(0, 0.5);
	break;
	case FORM_SPIRIT:
		//position.interpolateTo(bodyPosition, 2, 0);
		position = bodyPosition;
		dsq->continuity.warpLiToAvatar();
		spiritBeaconEmitter.start();
		setCanActivateStuff(true);
		setCanLockToWall(true);
		setCanBurst(true);
		setDamageTarget(DT_WALLHURT, true);
	break;
	case FORM_BEAST:
		setCanSwimAgainstCurrents(false);
	break;
	case FORM_DUAL:
		if (dsq->continuity.hasLi())
		{
			game->li->alpha = 1;
			game->li->position = position;
			game->li->setState(STATE_IDLE);
		}
	break;
	case FORM_NATURE:
		setDamageTarget(DT_WALLHURT, true);
		break;
	default:
		if (leftHandEmitter && rightHandEmitter)
		{
			leftHandEmitter->stop();
			rightHandEmitter->stop();
		}
	break;
	}

	elementEffectMult = 1;
	state.abilityDelay = 0;
	formAbilityDelay = 0;
	dsq->continuity.form = form;
	formTimer = 0;
	if (effects)
	{
		if (core->afterEffectManager)
			core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08f,0.05f,22,0.2f, 1.2f));

		switch(form)
		{
		case FORM_ENERGY:
			core->sound->playSfx("EnergyForm");
			/*
			game->tintColor.path.addPathNode(Vector(1,1,1),0);
			game->tintColor.path.addPathNode(Vector(1.5,1.5,4),0.25);
			game->tintColor.path.addPathNode(Vector(4,1.5,1),0.5);
			game->tintColor.path.addPathNode(Vector(1,1,1),0.5);
			game->tintColor.startPath(2);
			*/

			/*
			game->tintColor = Vector(1,1,3);
			game->tintColor.interpolateTo(Vector(1,1,1), 1);
			*/

		break;
		case FORM_NORMAL:
			core->sound->playSfx("NormalForm");
		break;
		case FORM_BEAST:
			core->sound->playSfx("BeastForm");
		break;
		case FORM_FISH:
			core->sound->playSfx("FishForm");
		break;
		case FORM_SUN:
			core->sound->playSfx("SunForm");
		break;
		case FORM_NATURE:
			core->sound->playSfx("NatureForm");
		break;
		case FORM_SPIRIT:
			spiritBeaconEmitter.start();
		break;
		case FORM_DUAL:
			core->sound->playSfx("DualForm");
		break;
		case FORM_NONE:
		case FORM_MAX:
		break;
		}

		/*
		dsq->overlay->color = Vector(1,1,1);
		dsq->overlay->alpha.interpolateTo(1.0, 0.2);
		avatar->disableInput();
		setv(EV_NOINPUTNOVEL, 0);
		core->main(0.2);
		setv(EV_NOINPUTNOVEL, 1);
		dsq->overlay->alpha.interpolateTo(0, 0.2);
		dsq->overlay->color.interpolateTo(0, 0.4);
		*/
		dsq->overlay->color = Vector(1,1,1);
		dsq->overlay->alpha = 1;
		dsq->overlay->alpha.interpolateTo(0, 0.5);
		dsq->overlay->color.interpolateTo(0, 1.0);
	}
	/*
	if (form != FORM_ENERGY)
	{
		game->sceneColor3.interpolateTo(Vector(1,1,1), 0.2);
	}
	*/
	if (hair)
	{
		hair->alphaMod = 0;
	}
	switch (form)
	{
	case FORM_ENERGY:
		refreshModel("Naija", "EnergyForm");
		for (size_t i = 0; i < targetQuads.size(); i++)
			targetQuads[i]->start();
		leftHandEmitter->load("EnergyFormHandGlow");
		leftHandEmitter->start();
		rightHandEmitter->load("EnergyFormHandGlow");
		rightHandEmitter->start();
	break;
	case FORM_FISH:
	{
		fallOffWall();

		setBoneLock(BoneLock());

		refreshModel("FishForm", "");
		//rotationOffset.interpolateTo(Vector(0,0,-90), 0.5);
		//refreshModel("NaijaFish", "");

		collideRadius = COLLIDE_RADIUS_FISH;
		setCanLockToWall(false);
		setCollisionAvoidanceData(COLLIDE_RANGE_FISH, COLLIDE_MOD_FISH);
		elementEffectMult = 0.4f;
	}
	break;
	case FORM_SUN:
	{
		refreshModel("Naija", "SunForm");
		lightFormGlow->moveToFront();
		lightFormGlow->alpha.interpolateTo(0.75f, 1);
		lightFormGlowCone->alpha.interpolateTo(0.4f, 1);

		lightFormGlow->alphaMod = 0;
		lightFormGlowCone->alphaMod = 0;
	}
	break;
	case FORM_NORMAL:
	{
		if (lastForm == FORM_SPIRIT)
		{
			dsq->continuity.shiftWorlds();
			fallOffWall();
		}
		refreshNormalForm();
		//skeletalSprite.loadSkeletal("child");
	}
	break;
	case FORM_NATURE:
		refreshModel("Naija", "NatureForm");
		if (hair)
		{
			hair->setTexture("Naija/Cape-NatureForm");
			hair->alphaMod = 1.0;
		}
		setDamageTarget(DT_WALLHURT, false);

	break;
	case FORM_BEAST:
	{
		refreshModel("Naija", "BeastForm");
		setCanSwimAgainstCurrents(true);
	}
	break;
	case FORM_SPIRIT:
		bodyPosition = position;
		bodyOffset = offset;
		fallOffWall();
		dsq->continuity.shiftWorlds();
		setCanActivateStuff(false);
		setCanLockToWall(false);
		setCanBurst(false);
		setDamageTarget(DT_WALLHURT, false);
		elementEffectMult = 0;

		if (onInit)
		{
			skeletalSprite.alphaMod = 0;
			canChangeForm = false;
		}
		/*
		if (hair)
			hair->alphaMod = lastHairAlphaMod;
		*/
	break;
	case FORM_DUAL:
	{
		if (dsq->continuity.hasLi())
		{
			game->li->setState(STATE_WAIT);
			game->li->alpha = 0;
		}
		//dualFormMode = DUALFORM_LI;
		refreshDualFormModel();
		/*
		for (int i = 0; i < targetQuads.size(); i++)
			targetQuads[i]->start();
		*/
	}
	break;
	default:
	break;
	}
	setHeadTexture("");
	if (effects)
		avatar->enableInput();

	//if (onInit) {
		//idle();//skeletalSprite.animate("idle", -1, 0);
	//}
}

void Avatar::singNote(int note)
{
	currentSong.notes.push_back(note);
}

void Avatar::updateSingingInterface(float dt)
{
	if (songIcons.size()>0 && songIcons[0]->alpha.x > 0)
	{
		if (dsq->getInputMode() != INPUT_JOYSTICK && !core->mouse.change.isZero())
		{
			if (game->songLineRender && songIcons[0]->alpha.x == 1)
			{
				float smallestDist = HUGE_VALF;
				int closest = -1;
				for (size_t i = 0; i < songIcons.size(); i++)
				{
					float dist = (songIcons[i]->position - core->mouse.position).getSquaredLength2D();
					if (dist < smallestDist)
					{
						smallestDist = dist;
						closest = i;
					}
				}

				game->songLineRender->newPoint(core->mouse.position, songIcons[closest]->noteColor);
			}
		}

		if (health <= 0 || isEntityDead())
		{
			closeSingingInterface();
		}
		else
		{
			if (dsq->getInputMode() == INPUT_JOYSTICK)
			{
				Vector d;
				for(size_t i = 0; i < core->getNumJoysticks(); ++i)
					if(Joystick *j = core->getJoystick(i))
						if(j->isEnabled())
						{
							d = j->position;
							if(!d.isZero())
								break;
						}

				if (d.isLength2DIn(JOYSTICK_NOTE_THRESHOLD))
				{
					core->setMousePosition(core->center);
				}
				else
				{
					// Choose the closest note based on the joystick input
					// angle (rather than the resultant cursor position).
					// But if we already have an active note and we're not
					// within the note-accept threshold, maintain the
					// current note instead.
					float angle = (atan2f(-d.y, d.x) * 180 / PI) + 90;
					if (angle < 0)
						angle += 360;
					int closestNote = (int)floorf(angle/45 + 0.5f);
					float angleOffset = fabsf(angle - closestNote*45);
					if (closestNote == 8)
						closestNote = 0;

					bool setNote = (angleOffset <= NOTE_ACCEPT_ANGLE_OFFSET);
					if (!setNote)
					{
						bool alreadyAtNote = false;
						for (size_t i = 0; i < songIcons.size(); i++)
						{
							const float dist = (songIcons[i]->position - core->mouse.position).getSquaredLength2D();
							if (dist <= sqr(NOTE_ACCEPT_DISTANCE))
							{
								alreadyAtNote = true;
								break;
							}
						}
						if (!alreadyAtNote)
							setNote = true;
					}

					if (setNote)
						core->setMousePosition(songIcons[closestNote]->position);
				}
			}

			setSongIconPositions();
		}
	}
}

void Avatar::setSongIconPositions()
{
	float radIncr = (2*PI)/float(songIcons.size());
	float rad = 0;
	for (size_t i = 0; i < songIcons.size(); i++)
	{
		songIcons[i]->position = Vector(400,300)+/*this->position + */Vector(sinf(rad)*singingInterfaceRadius, cosf(rad)*singingInterfaceRadius);
		rad += radIncr;
	}
}

const int chkDist = 2500*2500;

Target Avatar::getNearestTarget(const Vector &checkPos, const Vector &distPos, Entity *source, DamageType dt, bool override, std::vector<Target> *ignore)
{
	Target t;

	Vector targetPosition;
	int targetPt = -1;
	Entity *closest = 0;
	int highestPriority = -999;
	float smallestDist = HUGE_VALF;
	Entity *e = 0;
	FOR_ENTITIES(i)
	{
		e = *i;
		/*
		int j;
		for (j = 0; j < targets.size(); j++)
		{
			if (targets[j].e == e) break;
		}
		if (j != targets.size()) continue;
		*/

		//e &&
		if (e != this && e->targetPriority >= highestPriority && this->pullTarget != e && e->isDamageTarget(dt) && game->isValidTarget(e, this))
		{



			if (e->position.isNan())
			//if (false)
			{
				std::ostringstream os;
				os << "NAN position entity name: " << e->name << " type: " << e->getEntityType();
				debugLog(os.str());
				continue;
			}
			else
			{
				int dist = (e->position - position).getSquaredLength2D();
				if (dist < chkDist)
				{
					int numTargetPoints = e->getNumTargetPoints();
					bool clearAfter = false;
					if (numTargetPoints == 0)
					{
						if (ignore)
						{
							size_t j = 0;
							for (; j < ignore->size(); j++)
							{
								if ((*ignore)[j].e == e)
									break;
							}
							if (j != ignore->size()) continue;
						}
						e->addTargetPoint(e->getEnergyShotTargetPosition());
						clearAfter = true;
						numTargetPoints = 1;
					}
					if (numTargetPoints > 0)
					{
						for (int i = 0; i < numTargetPoints; i++)
						{
							if (ignore)
							{
								size_t j = 0;
								for (; j < ignore->size(); j++)
								{
									if ((*ignore)[j].e == e && (*ignore)[j].targetPt == i)
										break;
								}
								if (j != ignore->size()) continue;
							}
							float dist = (e->getTargetPoint(i) - distPos).getSquaredLength2D();
							//float dist = (e->getTargetPoint(i) - distPos).getLength2D();
							if (dist < sqr(TARGET_RANGE+e->getTargetRange()))
							{
								if (override || (checkPos - e->getTargetPoint(i)).isLength2DIn(64))
								{
									dist = (e->getTargetPoint(i) - checkPos).getSquaredLength2D();
									if (dist < smallestDist)
									{
										highestPriority = e->targetPriority;
										targetPosition = e->getTargetPoint(i);
										closest = e;
										smallestDist = dist;
										targetPt = i;
									}
								}
							}
						}
					}
					if (clearAfter)
						e->clearTargetPoints();
				}
			}
		}
	}
	t.e = closest;
	t.pos = targetPosition;
	t.targetPt = targetPt;
	return t;
}

float maxTargetDelay = 0.5;
bool wasDown = false;
void Avatar::updateTargets(float dt, bool override)
{
	DamageType damageType = DT_AVATAR_ENERGYBLAST;
	for (size_t i = 0; i < targets.size(); i++)
	{
		if (!targets[i].e
		|| !targets[i].e->isPresent()
		|| targets[i].e->getState() == STATE_DEATHSCENE
		|| !game->isValidTarget(targets[i].e, this))
		{
			targets.clear();
			break;
		}
	}
	if ((dsq->getInputMode() == INPUT_MOUSE || dsq->getInputMode() == INPUT_KEYBOARD) && !(wasDown && core->mouse.buttons.right))
	{
		wasDown = false;
		float mod = 1;
		if (isCharging())
			mod = maxTargetDelay*10;
		targetUpdateDelay += dt*mod;
	}

	if (targetUpdateDelay > maxTargetDelay || override)
	{
		maxTargetDelay = 0;
		std::vector<Target> oldTargets = targets;
		if ((dsq->continuity.form == FORM_ENERGY) && ((core->mouse.buttons.right && state.spellCharge > 0.3f) || override))
			//&& state.spellCharge > 0.2f /*&& state.spellCharge < 0.5f*/
		{
			// crappy hack for now, assuming one target:
			targets.clear();


			Vector dir = getAim();
			Vector checkPos = position + dir;
			Vector distPos = position;
			if (!(dsq->getGameCursorPosition() - distPos).isLength2DIn(4))
			{
				Target t;
				t = getNearestTarget(checkPos, distPos, this, damageType, override, &targets);
				if (t.e)
				{
					//if ((t.getWorldPosition() - dsq->getGameCursorPosition()).isLength2DIn(64))
					{


						// found a target?
						targets.push_back(t);

						targetUpdateDelay = 0;
						if (!override && core->mouse.buttons.right)
						{
							maxTargetDelay = 90;

							dsq->spawnParticleEffect("TargetAquired", t.pos);
							wasDown = true;
						}
					}
				}
			}
			if (targets.empty())
			{
				for (size_t i = 0; i < oldTargets.size(); i++)
				{
					Entity *e = oldTargets[i].e;
					if (e)
					{
						int dist = (e->getTargetPoint(oldTargets[i].targetPt) - distPos).getSquaredLength2D();
						if (dist < sqr(TARGET_RANGE+e->getTargetRange()))
						{
							targets.push_back(oldTargets[i]);
						}
					}
					else
					{
						targets.clear();
						break;
					}
				}
			}
		}
	}
	else
	{
		for (size_t i = 0; i < targets.size(); i++)
		{
			Entity *e = targets[i].e;
			if (e)
			{
				if (!(position - e->position).isLength2DIn(e->getTargetRange() + TARGET_RANGE + TARGET_GRACE_RANGE) || !game->isValidTarget(e, this) || !e->isDamageTarget(damageType))
				{
					lostTarget(i, targets[i].e);
					targets[i].e = 0;
					targetUpdateDelay = maxTargetDelay;
					wasDown = false;
				}
			}
		}
	}
}

void Avatar::updateTargetQuads(float dt)
{

	const Vector cursorpos = dsq->getGameCursorPosition();
	particleManager->setSuckPosition(1, cursorpos);

	/*
	for (int i = 0; i < targetQuads.size(); i++)
	{

	}
	*/

	static Entity *lastTargetE = 0;
	const float tt = 0.02f;
	for (size_t i = 0; i < targets.size(); i++)
	{
		if (targets[i].e)
		{

			targetQuads[i]->alpha.interpolateTo(1, 0.1f);
			Entity *e = targets[i].e;
			if (lastTargetE != e)
			{
				dsq->sound->playSfx("target-lock");
				lastTargetE = e;
			}
			else
			{
				//targetQuads[i]->position.interpolateTo(targets[i].pos, 0.01);
			}
			targetQuads[i]->position.interpolateTo(targets[i].pos, tt);
			targets[i].pos = e->getTargetPoint(targets[i].targetPt);
			if (i == 0)
			{
				particleManager->setSuckPosition(1, targets[i].pos); // suckpos 1 is overridden elsewhere later
				particleManager->setSuckPosition(2, targets[i].pos);
			}

			/*
			Emitter *em = targetQuads[i];
			if (!em->isRunning())
			{
				em->start();
			}
			*/
		}
		else
		{
			targetQuads[i]->position = cursorpos;
			//targetQuads[i]->alpha.interpolateTo(0, 0.1);
		}
	}

	if (targets.empty())
	{
		for (size_t i = 0; i < targetQuads.size(); i++)
		{
			if (lastTargetE != 0)
			{
				lastTargetE = 0;
			}
			//targetQuads[i]->position.interpolateTo(dsq->getGameCursorPosition(),tt);
			/*
			std::ostringstream os;
			os << "setting targetQuads[i] to game cursor, is running = " << targetQuads[i]->isRunning();
			debugLog(os.str());
			*/

			targetQuads[i]->position = cursorpos;
			if (dsq->continuity.form == FORM_ENERGY && isInputEnabled())
			{
				if (dsq->getInputMode() == INPUT_JOYSTICK && targetQuads[i]->isRunning())
				{
					targetQuads[i]->stop();
				}
				else if (dsq->getInputMode() != INPUT_JOYSTICK && !targetQuads[i]->isRunning())
				{
					targetQuads[i]->start();
				}
			}

			/*
			if (targetQuads[i]->isRunning())
			{
				targetQuads[i]->stop();
			}
			*/
		}
	}
}

//fireAtNearestValidEntity("Fire", DT_AVATAR_ENERGYBLAST);
bool Avatar::fireAtNearestValidEntity(const std::string &shot)
{
	if (state.swimTimer > 0)
		state.swimTimer -= 0.5f;

	skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->stopAnimation();

	targetUpdateDelay = 0;
	if (targetUpdateDelay < 0)
		targetUpdateDelay = 0;
	//bool big = false;

	Vector dir;
	Vector p = position;
	if(boneLeftArm)
		p = boneLeftArm->getWorldPosition();
	//&& !game->isObstructed(TileVector(position))
	/*
	if (dsq->inputMode == INPUT_MOUSE && state.lockedToWall )
		dir = dsq->getGameCursorPosition() - p;
	else
	*/
	dir = getAim();

	ShotData *shotData = Shot::getShotData(shot);


	bool aimAt = (dir.z == 1.0f);
	//bool aimAt = true;
	dir.z = 0;
	Vector targetPosition;

	//std::vector<Target>targets;

	bool firedShot = false;
	//int homing = 0;
	/*
	if (target)
	{
		if (dsq->inputMode != INPUT_JOYSTICK && vel.isLength2DIn(50))
		{
		}
		else
		{

		}
		homing = home;
	}
	else
		homing = 0;
	*/

	/*
	if (!dir.isLength2DIn(2))
	{
	*/
	Shot *s = 0;
	bool clearTargets = false;

	// allow autoAim if desired
	if ((dsq->getInputMode() == INPUT_JOYSTICK && !aimAt) || dsq->user.control.autoAim)
	{
		if (targets.empty())
		{
			// force a grab of the nearest targets
			updateTargets(shotData->damageType, true);
			// clear the targets after
			clearTargets = true;
		}
	}

	if (!targets.empty())
	{
		//homing = home;
		for (size_t i = 0; i < targets.size(); i++)
		{
			/*
			if (!aimAt)
			{
				dir = targets[i].pos - p;
			}
			*/
				/*
				std::ostringstream os;
				os << "shotdir(" << dir.x << ", " << dir.y << ")";
				debugLog(os.str());
				*/


				/*
				Vector oldDir = dir;

				dir.normalize2D();
				dir = (dir + oldDir)/2.0f;
				*/

			if (!aimAt)
			{
				dir = (targets[i].e->getTargetPoint(targets[i].targetPt) - p);
			}

			s = game->fireShot(shot, this, targets[i].e);
			s->setAimVector(dir);
			s->setTargetPoint(targets[i].targetPt);

			/*
			if (dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY2))
			{
				s = game->fireShot("EnergyBlast2", this, targets[i].e);
				s->setAimVector(dir);
				s->setTargetPoint(targets[i].targetPt);
			}
			else
			{
				s = game->fireShot("EnergyBlast", this, targets[i].e);
				s->setAimVector(dir);
				s->setTargetPoint(targets[i].targetPt);
			}
			*/
		}
	}
	else
	{
		//if (!dir.isLength2DIn(2) || dsq->inputMode == INPUT_JOYSTICK)
		if (true)
		{
			s = game->fireShot(shot, this);

			if (dir.isLength2DIn(2))
			{
				if (!vel.isLength2DIn(2))
					s->setAimVector(vel);
				else // standing still
					s->setAimVector(getForwardAim());
			}
			else
			{
				s->setAimVector(dir);
			}
			/*
			if (dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY2))
			{
				s = game->fireShot("EnergyBlast2", this);
				s->setAimVector(dir);
			}
			else
			{
				s = game->fireShot("EnergyBlast", this);
				s->setAimVector(dir);
			}
			*/
		}
	}

	if (s)
	{
		checkUpgradeForShot(s);



		skeletalSprite.transitionAnimate("fireBlast", 0.1f, 0, ANIMLAYER_ARMOVERRIDE);
		s->position = p;
		//s->damageType = dt;
		/*
		if (!targets.empty())
			s->damage = float(damage)/float(targets.size());
		*/
		firedShot = true;
	}

	if (clearTargets)
	{
		targets.clear();
		// try to avoid targets sticking
		updateTargetQuads(shotData->damageType);
	}

	return firedShot;
}

void Avatar::switchDualFormMode()
{
	//debugLog("dualForm: changing");

	dsq->sound->playSfx("dualform-switch");

	dsq->overlay->color = Vector(1,1,1);
	dsq->fade(1, 0);
	dsq->fade(0, 0.5);

	if (dsq->continuity.dualFormMode == Continuity::DUALFORM_NAIJA)
		dsq->continuity.dualFormMode = Continuity::DUALFORM_LI;
	else
		dsq->continuity.dualFormMode = Continuity::DUALFORM_NAIJA;

	refreshDualFormModel();
}

bool Avatar::hasThingToActivate()
{
	return ((pathToActivate != 0) || (entityToActivate != 0));
}

void Avatar::formAbility()
{
	if (hasThingToActivate()) return;
	//debugLog("form ability function");
	switch(dsq->continuity.form)
	{
	case FORM_DUAL:
		{
			debugLog("dual form ability");
			/*
			if (this->getVectorToCursorFromScreenCentre().isLength2DIn(minMouse))
			{
				debugLog("in and changing");
				if (dualFormMode == DUALFORM_NAIJA)
					dualFormMode = DUALFORM_LI;
				else
					dualFormMode = DUALFORM_NAIJA;
				refreshDualFormModel();
			}
			else
			*/
			{
				/*
				if (chargeLevelAttained == 2)
				{
					if (dualFormMode == DUALFORM_NAIJA)
						dualFormMode = DUALFORM_LI;
					else
						dualFormMode = DUALFORM_NAIJA;
					refreshDualFormModel();
				}
				else
				*/
				{
					if (dsq->continuity.dualFormMode == Continuity::DUALFORM_NAIJA)
					{

						// ~~~~~~~~~~ SOUL SCREAM

						if (chargeLevelAttained == 1)
						{
							if (dsq->continuity.dualFormCharge >= requiredDualFormCharge)
							{
								core->sound->playSfx("DualForm-Scream");

								if (core->afterEffectManager)
									core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08f,0.05f,22,0.2f, 1.2f));

								dsq->continuity.dualFormCharge = 0;
								dsq->shakeCamera(25, 2);

								core->globalScale = Vector(0.4f, 0.4f);
								core->globalScaleChanged();
								myZoom = Vector(0.4f, 0.4f);

								/*
								setv(EV_NOINPUTNOVEL, 0);
								core->globalScale = Vector(1.5, 1.5);
								core->main(0.5);
								setv(EV_NOINPUTNOVEL, 1);
								*/


								FOR_ENTITIES(i)
								{
									Entity *e = *i;
									if (e->getEntityType() == ET_ENEMY && e != this)
									{
										if (e->isv(EV_SOULSCREAMRADIUS, -1) || (e->position - position).isLength2DIn(1000 + e->getv(EV_SOULSCREAMRADIUS)))
										{
											DamageData d;
											d.damage = 20;
											d.damageType = DT_AVATAR_DUALFORMNAIJA;
											d.attacker = this;
											d.form = dsq->continuity.form;
											e->damage(d);
										}
									}
								}

								/*
								setv(EV_NOINPUTNOVEL, 0);
								core->main(0.5);
								dsq->screenTransition->capture();
								dsq->screenTransition->go(0.5);
								myZoom = Vector(1,1);
								setv(EV_NOINPUTNOVEL, 1);
								*/
							}
							else
							{
								core->sound->playSfx("Denied");
							}
						}
					}
					else if (dsq->continuity.dualFormMode == Continuity::DUALFORM_LI)
					{
						if (chargeLevelAttained == 1)
						{
							int i = 0;
							int num = 5;
							for (; i < num; i++)
							{
								Shot *s = game->fireShot("DualForm", this, 0, position, 0);
								//*0.5f + getAim()*0.5f
								Vector v1 = this->getTendrilAimVector(i, num);
								Vector v2 = getAim();
								v1.normalize2D();
								v2.normalize2D();
								s->setAimVector(v1*0.1f + v2*0.9f);
							}
							core->sound->playSfx("DualForm-Shot");
							dsq->spawnParticleEffect("DualFormFire", position);

							/*
							didShockDamage = false;
							doShock("DualFormLiTendril");
							*/
						}
						else
						{
							core->sound->playSfx("Denied");
							/*
							if (!fireDelay)
							{
								if (fireAtNearestValidEntity("DualFormLi"))
								{
									fireDelay = fireDelayTime;
								}
							}
							*/
						}
					}
				}
			}
		}
	break;
	case FORM_ENERGY:
		{
			if (chargeLevelAttained == 2)
			{
				if (dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY2))
					doShock("EnergyTendril2");
				else
					doShock("EnergyTendril");
				if (!state.lockedToWall)
					skeletalSprite.animate("energyChargeAttack", 0, ANIMLAYER_UPPERBODYIDLE);

				/*
				if (core->afterEffectManager)
					core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter, 0.1,0.03,30,0.2f, 1.5));
				*/
				dsq->playVisualEffect(VFX_SHOCK, position, this);
			}
			else
			{
				if (!fireDelay)
				{
					std::string shotName;
					if (dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY2))
						shotName = "EnergyBlast2";
					else
						shotName = "EnergyBlast";

					if (fireAtNearestValidEntity(shotName))
					{
						fireDelay = fireDelayTime;
					}
				}
			}
		}
		break;
	case FORM_NATURE:
		if (formAbilityDelay == 0)
		{
			formAbilityDelay = 0.2f;
			//Vector pos = dsq->getGameCursorPosition() - position;

			Vector pos = getAim();
			if (!pos.isZero())
				pos.setLength2D(16);
			pos += position;

			std::string seedName;
			if (chargeLevelAttained == 0)
				seedName = "SeedFlower";
			else if (chargeLevelAttained == 2)
				seedName = "SeedUberVine";

			game->fireShot(seedName, this, 0, pos, getAim());
		}
	break;
	case FORM_BEAST:
		{

			if (!dsq->continuity.isNaijaEatsEmpty())
			{
				EatData *d = dsq->continuity.getLastNaijaEat();
				if (!d->shot.empty())
				{
					int num = getNumShots()-2;

					for (int i = 0; i < num; i++)
					{
						bool playSfx = true;
						if (i > 0)
							playSfx = false;
						Shot *s = game->fireShot(d->shot, this, 0, Vector(0,0,0), Vector(0,0,0), playSfx);
						if (s->shotData && s->shotData->damage > 0)
						{
							s->extraDamage = 1;
						}

						Entity *target = 0;
						if (s->shotData->homing > 0)
						{
							Vector p = dsq->getGameCursorPosition();
							target = game->getNearestEntity(p, 800, this, ET_ENEMY, s->getDamageType());
						}
						if (target)
						{
							s->target = target;
						}

						if (bone_head)
						{
							s->position = bone_head->getWorldPosition();
						}
						else
						{
							s->position = this->position;
						}

						Vector aim = getVectorToCursor();
						if (aim.isZero())
							aim = getForwardAim();
						if (num == 1)
							s->setAimVector(aim);
						else
						{
							aim.normalize2D();
							s->setAimVector(getTendrilAimVector(i, num)*0.1f + aim*0.9f);
						}

						if (s->shotData && s->shotData->avatarKickBack)
						{
							Vector d = s->velocity;
							d.setLength2D(-s->shotData->avatarKickBack);
							float effect = 1;
							if (!isUnderWater())
								effect = 0.4f;
							push(d, s->shotData->avatarKickBackTime * effect, s->shotData->avatarKickBack * effect, 0);
						}
					}

					debugLog("firing: " + d->shot);
					d->ammo--;
				}
				if (d->ammo <= 0)
					dsq->continuity.removeLastNaijaEat();
					//dsq->continuity.removeEatData(eats.size()-1);
			}

			/*
			switch(inTummy)
			{
			case EAT_BASICSHOT:
			{
				tummyAmount --;
				// FIRE!
				if (tummyAmount <= 0)
				{

					//fireAtNearestValidEntity("Vomit", inTummy, DT_AVATAR_VOMIT, 6000);
					inTummy = EAT_NONE;
				}
			}
			break;
			}
			*/
			/*
			if (inTummy > 0)
			{
				if (inTummy > 3)
					inTummy = 3;
				if (fireAtNearestValidEntity("Vomit", inTummy, DT_AVATAR_VOMIT, 6000))
				{
					inTummy = 0;
				}
			}
			*/
			/*
			if (ability == 0)
			{
				Vector bitePos = position;
				Vector offset = vel;
				offset.setLength2D(128);
				bitePos += offset;

				FOR_ENTITIES (i)
				{
					Entity *e = *i;
					if (e && (e->position - bitePos).getSquaredLength2D() < sqr(64))
					{
						DamageData d;
						d.attacker = this;
						d.damage = 2;
						e->damage(d);
						heal(1);
					}
				}
			}
			else
			{
			}
			*/
		}
	break;
	case FORM_SUN:
	{
		if (formAbilityDelay == 0 && chargeLevelAttained==1)
		{
			core->sound->playSfx("SunForm");
			//dsq->spawnParticleEffect("LightFlare", position);

			chargeEmitter->load("SunFlare");
			chargeEmitter->start();

			PauseQuad *q = new PauseQuad;
			q->setTexture("Naija/LightFormGlow");
			q->position = position;
			q->setWidthHeight(1024, 1024);
			q->setLife(1);
			q->setDecayRate(0.05f);
			q->fadeAlphaWithLife = 1;
			q->scale = Vector(0,0);
			q->scale.interpolateTo(Vector(2,2), 0.1f);
			game->addRenderObject(q, LR_ELEMENTS13);
			q->moveToFront();

			FOR_ENTITIES(i)
			{
				Entity *e = *i;
				if (e != this && (e->position - position).isLength2DIn(2048))
				{
					e->lightFlare();
				}
			}
			//formAbilityDelay = 0.1;
		}
	}
	break;
	case FORM_SPIRIT:
		// spirit beacon
		// absorbs nearby shots, and respawns the player if in a "SPIRITBEACON" node
		if (formAbilityDelay == 0)
		{
			core->sound->playSfx("Spirit-Beacon");
			//dsq->spawnParticleEffect("SpiritBeacon", position);
			std::list<Shot*> delShots;
			Shot::Shots::iterator i;
			for (i = Shot::shots.begin(); i != Shot::shots.end(); i++)
			{
				Shot *s = (*i);
				if (s->isActive() && s->shotData && !s->shotData->invisible)
				{
					if (!s->firer || s->firer->getEntityType()==ET_ENEMY)
					{
						if ((s->position - position).isLength2DIn(256))
						{
							//s->safeKill();
							delShots.push_back(s);
							spiritEnergyAbsorbed++;
						}
					}
				}
			}
			for (std::list<Shot*>::iterator j = delShots.begin(); j != delShots.end(); j++)
			{
				Shot *s = (*j);
				s->safeKill();
			}
			if (spiritEnergyAbsorbed > 4)
			{
				game->spawnManaBall(position, 1);
				spiritEnergyAbsorbed = 0;
			}
			spiritBeaconEmitter.start();
			formAbilityDelay = 1.0;

			Path *p = game->getNearestPath(position, "SPIRITBEACON");
			if (p && p->isCoordinateInside(position))
			{
				bodyPosition = position;
				if (pullTarget)
				{
					pullTarget->position = position;
				}
				revert();
			}
			else
			{
				Path *p = game->getNearestPath(position, PATH_SPIRITPORTAL);
				if (p && p->isCoordinateInside(position))
				{
					changeForm(FORM_NORMAL);
					game->warpToSceneFromNode(p);
				}
			}
		}
	break;
	case FORM_FISH:
	{

	}
	break;
	case FORM_NORMAL:
	case FORM_NONE:
	case FORM_MAX:
	break;
	}
}

Vector Avatar::getTendrilAimVector(int i, int max)
{
	float a = float(float(i)/float(max))*PI*2;
	Vector aim(sinf(a), cosf(a));
	if (state.lockedToWall)
	{
		Vector n = game->getWallNormal(position);
		if (!n.isZero())
		{
			aim = aim*0.4f + n*0.6f;
		}
	}
	return aim;
}

size_t Avatar::getNumShots()
{
	size_t thits = normalTendrilHits;
	if (flourishPowerTimer.isActive())
	{
		if (lastBurstType == BURST_WALL)
			thits = maxTendrilHits;
		else
			thits = rollTendrilHits;
	}
	else
	{
		if (bursting)
		{
			if (lastBurstType == BURST_WALL)
				thits = rollTendrilHits;
		}
	}
	return thits;
}

void Avatar::doShock(const std::string &shotName)
{
	size_t c = 0;
	std::vector <Entity*> entitiesToHit;
	std::vector <Target> localTargets;
	bool clearTargets = true;

	size_t thits = getNumShots();

	if (!targets.empty() && targets[0].e != 0)
	{
		clearTargets = false;
		for (size_t i = 0; i < thits; i++)
		{
			entitiesToHit.push_back(targets[0].e);
		}
	}
	else
	{
		localTargets.clear();

		while (c < thits)
		{
			Target t = getNearestTarget(position, position, this, DT_AVATAR_SHOCK, true, &localTargets);
			if (t.e)
			{
				localTargets.push_back(t);
				entitiesToHit.push_back(t.e);
				targets.push_back(t);
				c ++;
			}
			else
			{
				break;
			}
		}

		if (!localTargets.empty())
		{
			while (entitiesToHit.size()<thits)
			{
				for (size_t i = 0; i < localTargets.size(); i++)
				{
					if (!(entitiesToHit.size()<thits))
						break;
					entitiesToHit.push_back(localTargets[i].e);
					targets.push_back(localTargets[i]);
				}
			}
		}
		localTargets.clear();
	}

	Vector aim = getAim();
	aim.normalize2D();
	int sz = entitiesToHit.size();


	if (sz == 0)
	{
		for (size_t i = 0; i < thits; i++)
		{
			Shot *s = game->fireShot(shotName, this, 0);

			s->setAimVector(getTendrilAimVector(i, thits));

			checkUpgradeForShot(s);
		}
	}
	else
	{
		for (int i = 0; i < sz; i++)
		{
			Entity *e = entitiesToHit[i];
			if (e)
			{
				Shot *s = game->fireShot(shotName, this, e);
				if (!targets.empty())
				{
					for (size_t j = 0; j < targets.size(); j++)
					{
						if (targets[j].e == e)
							s->targetPt = targets[j].targetPt;
					}
				}
				s->setAimVector(getTendrilAimVector(i, thits));
				checkUpgradeForShot(s);
			}
		}
	}

	if (clearTargets)
	{
		targets.clear();
	}
}

void Avatar::formAbilityUpdate(float dt)
{
	switch(dsq->continuity.form)
	{
	case FORM_FISH:
	{
		if (core->mouse.buttons.right)
		{
			const float bubbleRate = 0.2f;

			state.abilityDelay -= dt;
			if (state.abilityDelay < 0)
				state.abilityDelay = 0;

			if (state.abilityDelay == 0)
			{
				state.abilityDelay = bubbleRate;
				// spawn bubble
				Vector dir = getAim();
				dir.normalize2D();

				game->fireShot("FishFormBubble", this, 0, position+dir*16, dir);
			}
		}
	}
	break;
	case FORM_ENERGY:
	case FORM_NORMAL:
	case FORM_BEAST:
	case FORM_NATURE:
	case FORM_SPIRIT:
	case FORM_DUAL:
	case FORM_SUN:
	case FORM_MAX:
	case FORM_NONE:
		break;
	}
}

bool Avatar::isMouseInputEnabled()
{
	if (!inputEnabled) return false;
	//if (dsq->continuity.getWorldType() != WT_NORMAL) return false;
	//if (getState() != STATE_IDLE) return false;
	if (game->isPaused()) return false;
	return true;
}

void Avatar::rmbd(int source, InputDevice device)
{
	if (!isMouseInputEnabled() || isEntityDead()) return;
	if (dsq->continuity.form == FORM_NORMAL )
	{
		if (device == INPUT_MOUSE)
		{
			Vector diff = getVectorToCursorFromScreenCentre();
			if (diff.getSquaredLength2D() < sqr(openSingingInterfaceRadius))
				openSingingInterface(device);
		}
		else
		{
			openSingingInterface(device);
		}
	}
	else
	{
		startCharge();
	}
}

void Avatar::rmbu(int source, InputDevice device)
{
	if (!isMouseInputEnabled() || isEntityDead()) return;

	if (charging)
	{
		if (!entityToActivate && !pathToActivate)
			formAbility();

		endCharge();
	}


	dsq->cursorGlow->alpha.interpolateTo(0, 0.2f);
	dsq->cursorBlinker->alpha.interpolateTo(0, 0.2f);

	if (singing)
	{
		closeSingingInterface();
	}


	if (entityToActivate)
	{
		entityToActivate->activate(this, source);
		entityToActivate = 0;
	}
	if (pathToActivate)
	{
		pathToActivate->activate(this, source);
		pathToActivate = 0;
	}
}

bool Avatar::canCharge()
{
	switch(dsq->continuity.form)
	{
	case FORM_ENERGY:
	case FORM_DUAL:
	case FORM_NATURE:
	case FORM_SUN:
		return true;
	break;
	case FORM_BEAST:
	case FORM_NORMAL:
	case FORM_SPIRIT:
	case FORM_FISH:
	case FORM_MAX:
	case FORM_NONE:
		break;
	}
	return false;
}

void Avatar::startCharge()
{
	if (!isCharging() && canCharge())
	{
		if (dsq->loops.charge != BBGE_AUDIO_NOCHANNEL)
		{
			core->sound->stopSfx(dsq->loops.charge);
			dsq->loops.charge = BBGE_AUDIO_NOCHANNEL;
		}

		PlaySfx sfx;
		sfx.name = "ChargeLoop";
		sfx.loops = -1;
		dsq->loops.charge = core->sound->playSfx(sfx);

		state.spellCharge = 0;
		chargeLevelAttained = 0;

		switch(dsq->continuity.form)
		{
		case FORM_ENERGY:
			chargingEmitter->load("ChargingEnergy");
		break;
		case FORM_NATURE:
			chargingEmitter->load("ChargingNature");
		break;
		case FORM_SUN:
			chargingEmitter->load("ChargingEnergy");
		break;
		case FORM_DUAL:
			chargingEmitter->load("ChargingDualForm");
		break;
		default:
			chargingEmitter->load("ChargingGeneric");
		break;
		}

		chargingEmitter->start();

		charging = true;

	}
	if (!canCharge())
	{
		formAbility();
	}
}

void Avatar::setBlockSinging(bool v)
{
	blockSinging = v;
	if (v)
	{
		currentSong.notes.clear(); // abort singing without triggering a song, if queued
		closeSingingInterface();
	}
}

bool Avatar::canSetBoneLock()
{
	return true;
}

void Avatar::onSetBoneLock()
{
	Entity::onSetBoneLock();

	if (boneLock.on)
	{
		skeletalSprite.transitionAnimate("wallLookUp", 0.2f, -1);
		lockToWallCommon();
		state.lockedToWall = 1;
	}
	else
	{
		if (state.lockedToWall)
		{
			fallOffWall();
		}
	}
}

void Avatar::onUpdateBoneLock()
{
	Entity::onUpdateBoneLock();

	wallNormal = boneLock.wallNormal;
	rotateToVec(wallNormal, 0.01f);
}

void Avatar::lmbd(int source, InputDevice device)
{
	if (!isMouseInputEnabled()) return;

	// getstopdistance
	if (_isUnderWater)
	{
		Vector v = getVectorToCursor();
		if (v.isLength2DIn(getStopDistance()) && !v.isLength2DIn(minMouse))
		{
			if (state.lockedToWall)
			{
				fallOffWall();
			}
		}
	}
}

void Avatar::fallOffWall()
{
	//stillTimer.stop();
	if (state.lockedToWall)
	{
		lockToWallFallTimer = 0;
		state.nearWall = false;
		state.lockedToWall = false;

		setBoneLock(BoneLock());


		idle();
		offset.interpolateTo(Vector(0,0), 0.1f);
		if (!wallNormal.isZero())
		{
			Vector velSet = wallNormal;
			velSet.setLength2D(200);
			vel += velSet;
		}
		//doCollisionAvoidance(dt, 5, 1);
	}
}

void Avatar::lmbu(int source, InputDevice device)
{
	if (!isMouseInputEnabled()) return;

	if (dsq->continuity.toggleMoveMode)
		movingOn = !movingOn;

	if (isSinging())
	{
		// switch menu
	}
}

bool Avatar::isCharging()
{
	return charging;
}

void Avatar::endCharge()
{
	if (charging)
	{
		if (dsq->loops.charge != BBGE_AUDIO_NOCHANNEL)
		{
			core->sound->stopSfx(dsq->loops.charge);
			dsq->loops.charge = BBGE_AUDIO_NOCHANNEL;
		}

		chargingEmitter->stop();

		charging = false;
		state.spellCharge = 0;
	}
}

Vector Avatar::getWallNormal(TileVector t)
{
	return game->getWallNormal(t.worldVector(), 5)*-1;
}


bool Avatar::isSwimming()
{
	return swimming;
}

void Avatar::lockToWallCommon()
{
	swimEmitter.stop();

	skeletalSprite.stopAllAnimations();
	rotationOffset.interpolateTo(0, 0.01f);

	fallGravityTimer = 0;

	dsq->spawnParticleEffect("LockToWall", position);
	stopBurst();
	stopRoll();
	core->sound->playSfx("LockToWall");
	//bursting = false;
	this->burst = 1;
	//lastLockToWallPos = position;

	state.lockToWallDelay.start(0.2f);
	state.lockedToWall = true;

	lockToWallFallTimer = -1;

	// move this to its own function?
	state.backFlip = false;
	skeletalSprite.getAnimationLayer(ANIMLAYER_OVERRIDE)->stopAnimation();
}

void Avatar::lockToWall()
{
	if (riding) return;
	if (inCurrent && !canSwimAgainstCurrents()) return;
	if (!canLockToWall()) return;
	if (state.lockedToWall) return;
	if (vel.x == 0 && vel.y == 0) return;
	if (game->isPaused()) return;

	TileVector t(position);
	Vector m = vel;
	m.setLength2D(3);
	t.x += int(m.x);
	t.y += int(m.y);

	bool good = true;
	if (!game->isObstructed(t))
	{
		do
		{
			TileVector test;

			test = TileVector(t.x, t.y+1);
			if (game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x, t.y-1);
			if (game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x-1, t.y);
			if (game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x+1, t.y);
			if (game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x+1, t.y+1);
			if (game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x-1, t.y+1);
			if (game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x+1, t.y-1);
			if (game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x-1, t.y-1);
			if (game->isObstructed(test))
			{
				t = test;
				break;
			}

			good = false;
		}
		while(0);
	}

	if (game->isObstructed(t, OT_HURT) && isDamageTarget(DT_WALLHURT))
	{
		good = false;
	}
	if (good)
	{
		wallNormal = game->getWallNormal(position);
		bool outOfWaterHit = (!_isUnderWater && !(wallNormal.y < -0.1f));
		if (wallNormal.isZero() )
		{
			debugLog("COULD NOT FIND NORMAL, GOING TO BOUNCE");
			return;
		}
		else
		{
			if (!dsq->mod.isActive() && !dsq->continuity.getFlag("lockedToWall"))
			{

				if (!game->isControlHint()){
					dsq->continuity.setFlag("lockedToWall", 1);
					game->setControlHint(stringbank.get(13), 1, 0, 0, 6, "", true);
				}
			}

			lockToWallCommon();

			if (outOfWaterHit)
				lockToWallFallTimer = 0.4f;
			else
				lockToWallFallTimer = -1;

			wallPushVec = wallNormal;
			wallPushVec *= 2000;
			wallPushVec.z = 0;
			skeletalSprite.stopAllAnimations();
			if (wallPushVec.y < 0 && (fabsf(wallPushVec.y) > fabsf(wallPushVec.x)))
			{
				skeletalSprite.transitionAnimate("wallLookUp", 0.2f, -1);
			}
			else
			{
				skeletalSprite.transitionAnimate("wall", 0.2f, -1);
			}
			rotateToVec(wallPushVec, 0.1f);

			offset.stop();

			int tileType = game->getGrid(t);
			Vector offdiff = t.worldVector() - position;
			if (!offdiff.isZero())
			{
				if (tileType & OT_INVISIBLEIN)
				{
					Vector adjust = offdiff;
					adjust.setLength2D(TILE_SIZE/2);
					offdiff -= adjust;
				}
				else
				{
					Vector adjust = offdiff;
					adjust.setLength2D(TILE_SIZE*2);
					offdiff -= adjust;
				}
			}

			float spd = vel.getLength2D();
			if (spd < 1000)
				spd = 1000;

			Vector diff = offset - offdiff;
			float len = diff.getLength2D();
			float time = 0;
			if (len > 0)
			{
				time = len/spd;
			}
			offset.interpolateTo(offdiff, time);

			wallLockTile = t;

			vel = Vector(0,0,0);
			vel2 = 0;
		}
	}
	else
	{
		//debugLog("COULD NOT FIND TILE TO GRAB ONTO");
		//position = opos;
	}
}

void Avatar::applyTripEffects()
{
	color.interpolateTo(BLIND_COLOR, 0.5f);

	tripper->alpha.interpolateTo(1, 8);

	tripper->color = Vector(1, 1, 1);
	tripper->rotation.z = 0;
	tripper->rotation.interpolateTo(Vector(0, 0, 360), 10, -1);
	tripper->scale = Vector(1.25f, 1.25f, 1.25f);
	tripper->scale.interpolateTo(Vector(1.3f, 1.3f, 1.3f), 2, -1, 1, 1);

	if (dsq->loops.trip != BBGE_AUDIO_NOCHANNEL)
	{
		dsq->sound->stopSfx(dsq->loops.trip);
		dsq->loops.trip = BBGE_AUDIO_NOCHANNEL;
	}

	PlaySfx play;
	play.name = "TripLoop";
	play.loops = -1;
	play.fade = SFT_IN;
	play.time = 1;
	dsq->loops.trip = dsq->sound->playSfx(play);
}

void Avatar::removeTripEffects()
{
	color.interpolateTo(Vector(1,1,1),0.5);
	tripper->alpha.interpolateTo(0, 4);

	if (dsq->loops.trip != BBGE_AUDIO_NOCHANNEL)
	{
		dsq->sound->fadeSfx(dsq->loops.trip, SFT_OUT, 3);
		dsq->loops.trip = BBGE_AUDIO_NOCHANNEL;
	}
}

void Avatar::applyBlindEffects()
{
	// screen black

	// character black
	color.interpolateTo(BLIND_COLOR, 0.5f);
	blinder->alpha.interpolateTo(1, 0.5f);

	blinder->rotation.z = 0;
	blinder->rotation.interpolateTo(Vector(0, 0, 360), 10, -1);
	blinder->scale = Vector(1.25f, 1.25f, 1.25f);
	blinder->scale.interpolateTo(Vector(1.3f, 1.3f, 1.3f), 2, -1, 1, 1);

	//dsq->toggleMuffleSound(1);
}

void Avatar::removeBlindEffects()
{
	color.interpolateTo(Vector(1,1,1),0.5f);
	blinder->alpha.interpolateTo(0, 0.5f);
	//dsq->toggleMuffleSound(0);
}

void Avatar::setBlind(float time)
{
	if (time == 0)
	{
		removeBlindEffects();
		return;
	}

	if (!state.blind)
	{
		applyBlindEffects();
	}
	state.blind = true;
	if (time > state.blindTimer.getValue())
		///*state.blindTimer.getValue() + */
		state.blindTimer.start(time);
}

void Avatar::setNearestPullTarget()
{
	const float maxDistSqr = sqr(800);
	float smallestDist = maxDistSqr;
	Entity *closest = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e)
		{
			if ((e->isPullable()) && e->life == 1)
			{
				float dist = (e->position - position).getSquaredLength2D();
				if (dist < smallestDist)
				{
					closest = e;
					smallestDist = dist;
				}
			}
		}
	}
	if (closest)
	{
		pullTarget = closest;
		pullTarget->startPull();
	}
}

void Avatar::createWeb()
{
	web = new Web;
	web->setParentEntity(this);
	game->addRenderObject(web, LR_ENTITIES);
	curWebPoint = web->addPoint(game->avatar->position);
	curWebPoint = web->addPoint(game->avatar->position);
}

void Avatar::clearWeb()
{
	if (web)
	{
		web->setExistence(25);
		//web->setLife(1);
		//web->setDecayRate(1.0f/30.0f);
		//web->fadeAlphaWithLife = 1;

		web = 0;
	}
}

Avatar::Avatar() : Entity(), ActionMapper()
{
	canDie = true;

	urchinDelay = 0;
	jellyDelay = 0;

	curWebPoint = 0;

	web = 0;

	bone_dualFormGlow = 0;
	bone_head = 0;
	boneLeftHand = 0;
	boneRightHand = 0;
	boneLeftArm = 0;
	boneFish2 = 0;

	lastWaterBubble = 0;
	lastJumpOutFromWaterBubble = false;

	lastBurstType = BURST_NONE;
	dsq->loops.shield = BBGE_AUDIO_NOCHANNEL;
	leftHandEmitter = rightHandEmitter = 0;
	boneLeftHand = boneRightHand = 0;
	canChangeForm = true;
	biteTimer = 0;
	dsq->loops.charge = BBGE_AUDIO_NOCHANNEL;
	//heartbeat = 0;

	headTextureTimer = 0;
	bone_dualFormGlow = 0;
	//dsq->continuity.dualFormCharge = 0;
	//dsq->continuity.dualFormMode = Continuity::DUALFORM_NAIJA;
	debugLog("Avatar 1");

	//registerEntityDied = true;
	setv(EV_ENTITYDIED, 1);
	wallBurstTimer = 0;
	beautyFlip = false;
	invincibleBreak = true;
	targetUpdateDelay = 0;
	biteDelay = 0;
	songInterfaceTimer = 0;
	quickSongCastDelay = 0;
	flourish = false;
	_isUnderWater = false;

	blockSinging = false;
	singing = false;

	spiritEnergyAbsorbed = 0;
	joystickMove = false;

	debugLog("setCanLeaveWater");

	setCanLeaveWater(true);

	debugLog("setRenderPass");

	setRenderPass(1);

	debugLog("Done those");

	rippleDelay = 0;
	ripples = false;
	fallGravityTimer = 0;
	lastOutOfWaterMaxSpeed = 0;
	//chargeGraphic = 0;
	shieldPoints = auraTimer = 0;
	glow = 0;

	fireDelay = 0;
	looking = false;
	rollDidOne = 0;
	lastQuad = lastQuadDir = rollDelay = rolling = 0;
	chargeLevelAttained = 0;
	activeAura = AURA_NONE;
	movingOn = false;
	currentMaxSpeed = 0;
	pullTarget = 0;
	revertTimer = 0;
	currentSongIdx = -1;
	leaches = 0;


	debugLog("Avatar vars->");

	damageTime = vars->avatarDamageTime;

	canMove = true;
	//scale = Vector(0.5, 0.5);
	scale = Vector(0.5, 0.5);

	debugLog("Avatar 2");
	//scale = Vector(1.0, 1.0);
	//setTexture("Naija-sprite2");
	renderQuad = false;
	name = "Naija";
	setEntityType(ET_AVATAR);

	targets.resize(1);

	entityToActivate = 0;
	pathToActivate = 0;
	zoomOverriden = false;
	canWarp = true;
	blinder = 0;
	zoomVel = 0;

	myZoom = Vector(1,1);
	this->pushingOffWallEffect = 0;
	lockToWallFallTimer  = 0;
	swimming = false;
	charging = false;
	bursting = false;
	burst = 1;
	burstDelay = 0;
	splashDelay = 0;
	avatar = this;

	swimming = false;

	debugLog("Avatar 3");
	hair = new Hair();
	hair->setTexture("Naija/Cape");
	hair->setRenderPass(1);
	game->addRenderObject(hair, LR_ENTITIES);

	debugLog("Avatar 4");

	bindInput();

	debugLog("Avatar 5");

	blinder = new PauseQuad;
	blinder->position = Vector(400, 300, 4.5);
	blinder->setTexture("particles/blinder");
	//blinder->width = blinder->height = 810;
	blinder->autoWidth = AUTO_VIRTUALWIDTH;
	blinder->autoHeight = AUTO_VIRTUALWIDTH;
	blinder->scale = Vector(1.0125f,1.0125f);
	blinder->followCamera = 1;

	blinder->alpha = 0;
	game->addRenderObject(blinder, LR_AFTER_EFFECTS);

	tripper = new PauseQuad;
	tripper->position = Vector(400,300);
	tripper->setTexture("particles/tripper");
	//tripper->setWidthHeight(810, 810);
	tripper->autoWidth = AUTO_VIRTUALWIDTH;
	tripper->autoHeight = AUTO_VIRTUALWIDTH;
	tripper->scale = Vector(1.0125f, 1.0125f);
	tripper->followCamera = 1;
	tripper->alpha = 0;
	game->addRenderObject(tripper, LR_AFTER_EFFECTS);

	songIcons.resize(8);
	size_t i = 0;
	for (i = 0; i < songIcons.size(); i++)
	{
		songIcons[i] = new SongIcon(i);
		songIcons[i]->alpha = 0;
		songIcons[i]->followCamera = 1;
		game->addRenderObject(songIcons[i], LR_HUD);
	}

	setSongIconPositions();

	fader = new Quad;
	fader->position = Vector(400,300);
	fader->setTexture("fader");
	fader->setWidthHeight(core->getVirtualWidth()+10);
	fader->followCamera = 1;
	fader->alpha = 0;
	game->addRenderObject(fader, LR_AFTER_EFFECTS);

	debugLog("Avatar 6");

	targetQuads.resize(targets.size());

	for (i = 0; i < targets.size(); i++)
	{
		targetQuads[i] = new ParticleEffect;
		/*
		targetQuads[i]->setTexture("missingImage");
		targetQuads[i]->alpha = 0;
		*/
		targetQuads[i]->load("EnergyBlastTarget");

		// HACK: should have its own layer?
		game->addRenderObject(targetQuads[i], LR_PARTICLES);
	}

	lightFormGlow = new Quad("Naija/LightFormGlow", 0);
	lightFormGlow->alpha = 0;

	lightFormGlow->scale.interpolateTo(Vector(5.5f, 5.5f), 0.4f, -1, 1);
	//lightFormGlow->positionSnapTo = &position;
	game->addRenderObject(lightFormGlow, LR_ELEMENTS13);

	lightFormGlowCone = new Quad("Naija/LightFormGlowCone", 0);
	lightFormGlowCone->alpha = 0;
	lightFormGlowCone->scale = Vector(1, 6); // 4.5
	game->addRenderObject(lightFormGlowCone, LR_ELEMENTS13);


	debugLog("Avatar 7");

	addChild(&regenEmitter, PM_STATIC);
	regenEmitter.load("FoodEffectRegen");

	addChild(&speedEmitter, PM_STATIC);
	speedEmitter.load("FoodEffectSpeed");

	addChild(&defenseEmitter, PM_STATIC);
	defenseEmitter.load("FoodEffectDefense");

	addChild(&invincibleEmitter, PM_STATIC);
	invincibleEmitter.load("FoodEffectInvincible");

	addChild(&auraEmitter, PM_STATIC);

	addChild(&auraLowEmitter, PM_STATIC);

	addChild(&auraHitEmitter, PM_STATIC);
	auraHitEmitter.load("AuraShieldHit");

	chargingEmitter = new ParticleEffect;
	dsq->getTopStateData()->addRenderObject(chargingEmitter, LR_PARTICLES);

	chargeEmitter = new ParticleEffect;
	dsq->getTopStateData()->addRenderObject(chargeEmitter, LR_PARTICLES_TOP);

	leftHandEmitter = new ParticleEffect;
	dsq->getTopStateData()->addRenderObject(leftHandEmitter, LR_PARTICLES);

	rightHandEmitter = new ParticleEffect;
	dsq->getTopStateData()->addRenderObject(rightHandEmitter, LR_PARTICLES);

	addChild(&biteLeftEmitter, PM_STATIC);
	biteLeftEmitter.load("BiteLeft");

	addChild(&biteRightEmitter, PM_STATIC);
	biteRightEmitter.load("BiteRight");

	addChild(&wakeEmitter, PM_STATIC);
	wakeEmitter.load("Wake");

	addChild(&swimEmitter, PM_STATIC);
	swimEmitter.load("Swim");

	addChild(&plungeEmitter, PM_STATIC);
	plungeEmitter.load("Plunge");
	plungeEmitter.position = Vector(0,-100);

	addChild(&spiritBeaconEmitter, PM_STATIC);
	spiritBeaconEmitter.load("SpiritBeacon");

	addChild(&healEmitter, PM_STATIC);

	addChild(&hitEmitter, PM_STATIC);

	addChild(&rollLeftEmitter, PM_STATIC);

	addChild(&rollRightEmitter, PM_STATIC);

	rollRightEmitter.load("RollRight");
	rollLeftEmitter.load("RollLeft");

	debugLog("Avatar 8");

	perform(STATE_IDLE);
	skeletalSprite.animate(getIdleAnimName(),-1);

	debugLog("Avatar 9");

	setDamageTarget(DT_AVATAR_LANCE, false);

	//changeForm(FORM_NORMAL, false);

	refreshNormalForm();

	if(dsq->continuity.form == FORM_FISH)
		collideRadius = COLLIDE_RADIUS_FISH;
	else
		collideRadius = COLLIDE_RADIUS_NORMAL;

	// defaults for normal form
	_canActivateStuff = true;
	_canBurst = true;
	_canLockToWall = true;
	_canSwimAgainstCurrents = false;
	_canCollideWithShots = true;

	_collisionAvoidMod = COLLIDE_MOD_NORMAL;
	_collisionAvoidRange = COLLIDE_RANGE_NORMAL;

	_seeMapMode = SEE_MAP_DEFAULT;

	blockBackFlip = false;
	elementEffectMult = 1;
	_lastActionSourceID = 9999;
	_lastActionInputDevice = INPUT_NODEVICE;
}

void Avatar::revert()
{
	if (canChangeForm)
	{
		if (dsq->continuity.form != FORM_NORMAL)
			changeForm(FORM_NORMAL);
	}
}

void Avatar::onHeal(int type)
{
	if (type == 1)
	{
		healEmitter.load("Heal");
		healEmitter.start();
	}
}

void Avatar::refreshNormalForm()
{
	std::string c = dsq->continuity.costume;
	if (c.empty())
		c = "Naija";
	refreshModel("Naija", c);
	if(hair)
	{
		hair->alphaMod = 1.0;
		if (!c.empty() && c!="Naija")
		{
			if(!hair->setTexture("naija/cape-"+c))
				hair->alphaMod = 0;
		}
		else
			hair->setTexture("naija/cape");
	}
}

void Avatar::refreshModel(std::string file, const std::string &skin, bool forceIdle)
{
	stringToLower(file);

	bool loadedSkeletal = false;

	if (!skeletalSprite.isLoaded() || nocasecmp(skeletalSprite.filenameLoaded, file)!=0)
	{
		skeletalSprite.loadSkeletal(file);
		loadedSkeletal = true;
	}
	if (!skin.empty())
		skeletalSprite.loadSkin(skin);

	if (file == "beast")
	{
		skeletalSprite.scale = Vector(1.25,1.25);
	}
	else
		skeletalSprite.scale = Vector(1,1);

	Animation *anim = skeletalSprite.getCurrentAnimation(0);
	if (forceIdle || (!anim || loadedSkeletal))
	{
		idle();
	}

	if (file == "naija")
	{
		bone_head = skeletalSprite.getBoneByIdx(1);
		boneLeftArm = skeletalSprite.getBoneByName("LeftArm");
		boneFish2 = skeletalSprite.getBoneByName("Fish2");
		if(boneFish2)
			boneFish2->alpha = 0;
		bone_dualFormGlow = skeletalSprite.getBoneByName("DualFormGlow");
		if (bone_dualFormGlow)
		{
			bone_dualFormGlow->scale = 0;
			bone_dualFormGlow->setBlendType(BLEND_ADD);
		}

		boneLeftHand = skeletalSprite.getBoneByName("LeftArm");
		boneRightHand = skeletalSprite.getBoneByName("RightArm");
	}
	else
	{
		bone_dualFormGlow = 0;
		bone_head = 0;
		boneLeftArm = boneFish2 = 0;
		boneLeftHand = boneRightHand = 0;
	}

	core->resetTimer();

	skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->stopAnimation();
}

Avatar::~Avatar()
{
}

void Avatar::destroy()
{
	Entity::destroy();

	if (dsq->loops.shield != BBGE_AUDIO_NOCHANNEL)
	{
		core->sound->fadeSfx(dsq->loops.shield, SFT_OUT, 1);
		dsq->loops.shield = BBGE_AUDIO_NOCHANNEL;
	}
	if (dsq->loops.current != BBGE_AUDIO_NOCHANNEL)
	{
		core->sound->fadeSfx(dsq->loops.current, SFT_OUT, 1);
		dsq->loops.current = BBGE_AUDIO_NOCHANNEL;
	}

	avatar = 0;
}

void Avatar::startBackFlip()
{
	if (boneLock.on) return;
	if (riding) return;
	if (blockBackFlip) return;

	skeletalSprite.getAnimationLayer(ANIMLAYER_OVERRIDE)->transitionAnimate("backflip", 0.2f, 0);
	vel.x = -vel.x*0.25f;
	state.backFlip = true;
}

void Avatar::stopBackFlip()
{
	if (state.backFlip)
	{
		//skeletalSprite.getAnimationLayer(ANIMLAYER_OVERRIDE)->stopAnimation();
		skeletalSprite.getAnimationLayer(ANIMLAYER_OVERRIDE)->transitionAnimate("backflip2", 0.2f, 0);
		state.backFlip = false;
	}
}

void Avatar::startBurstCommon()
{
	skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->stopAnimation();

	bittenEntities.clear();
	flourish = false;
	if (dsq->continuity.form == FORM_BEAST)
	{
		setHeadTexture("Bite");
	}
	burstTimer = 0;

	setBoneLock(BoneLock());

	biteTimer = 0;

	if (dsq->continuity.form == FORM_BEAST)
	{
		if (isfh())
			biteRightEmitter.start();
		else
			biteLeftEmitter.start();
	}
}

void Avatar::startBurst()
{
	if (!riding && canBurst() && (joystickMove || getVectorToCursor().getSquaredLength2D() > sqr(BURST_DISTANCE))
		&& getState() != STATE_PUSH && (!skeletalSprite.getCurrentAnimation() || (skeletalSprite.getCurrentAnimation()->name != "spin"))
		&& _isUnderWater && !isActing(ACTION_ROLL, -1))
	{
		if (!bursting && burst == 1)
		{
			dsq->rumble(0.2f, 0.2f, 0.2f, _lastActionSourceID, _lastActionInputDevice);
			if (dsq->continuity.form != FORM_BEAST)
				wakeEmitter.start();
			game->playBurstSound(pushingOffWallEffect>0);
			skeletalSprite.animate(getBurstAnimName(), 0);
			bursting = true;
			burst = 1.0f;
			ripples = true;
			startBurstCommon();

			lastBurstType = BURST_NORMAL;
		}
		else if (bursting && burstTimer > 0.3f)
		{
			if (!flourish && !state.nearWall)
				//&& dsq->continuity.form == FORM_NORMAL)
			{
				//if (rand()%100 < 50)
				if (true)
				{
					startFlourish();
				}
				/*
				else
				{
					skeletalSprite.transitionAnimate("flourish2", 0.1, 0, 3);
					flourish = true;
					if (this->isfh())
						rotationOffset = Vector(0,0,-360);
					else
						rotationOffset = Vector(0,0,360);
					rotationOffset.interpolateTo(Vector(0,0,0), 0.8, 0, 0, 1);
				}
				*/
				//burst += 0.1;
				if (!vel.isZero())
				{
					Vector add = vel;
					add.setLength2D(50);
					vel2 += add;
				}
			}
		}
	}
}

void Avatar::startWallBurst(bool useCursor)
{
	//if (!bursting && burst == 1 )
	{
		Vector goDir;

		//goDir = getVectorToCursorFromScreenCentre();
		goDir = getVectorToCursor();

		if (goDir.isLength2DIn(BURST_DISTANCE))
		{
			if (!goDir.isLength2DIn(minMouse))
				fallOffWall();
			return;
		}
		goDir.normalize2D();

		if (_isUnderWater && dsq->continuity.form != FORM_BEAST)
			wakeEmitter.start();

		offset.interpolateTo(Vector(0,0), 0.05f);

		dsq->spawnParticleEffect("WallBoost", position+offset, rotation.z);
		if (goDir.x != 0 || goDir.y != 0)
		{
			lastBurstType = BURST_WALL;

			dsq->rumble(0.22f, 0.22f, 0.2f, _lastActionSourceID, _lastActionInputDevice);
			bittenEntities.clear();
			if (useCursor)
			{
				wallPushVec = (goDir*0.75f + wallNormal*0.25f);
				//wallPushVec = goDir;
			}
			else
			{
				float v = goDir.dot2D(wallNormal);
				if (v <= -0.8f)
					wallPushVec = wallNormal;
				else
				{
					wallPushVec = (goDir*0.5f + wallNormal*0.5f);
				}
			}
				//wallPushVec = (goDir*0.9f + wallNormal*0.1f);
			wallPushVec.setLength2D(vars->maxWallJumpBurstSpeed);

			position.stop();
			pushingOffWallEffect = 0.5;
			vel = wallPushVec;
			this->state.lockedToWall = false;
			skeletalSprite.stopAllAnimations();
			game->playBurstSound(pushingOffWallEffect>0);
			skeletalSprite.animate(getBurstAnimName(), 0);
			bursting = true;
			burst = 1.5;
			ripples = true;

			startBurstCommon();
		}
	}
}

bool Avatar::isActionAndGetDir(Vector& dir)
{
	bool dL = isActing(ACTION_SWIMLEFT, -1);
	bool dR = isActing(ACTION_SWIMRIGHT, -1);
	bool dU = isActing(ACTION_SWIMUP, -1);
	bool dD = isActing(ACTION_SWIMDOWN, -1);
	Vector a;
	if (dL)
		a += Vector(-1,0);
	if (dR)
		a += Vector(1,0);
	if (dU)
		a += Vector(0,-1);
	if (dD)
		a += Vector(0,1);

	if ((dL || dR) && (dU || dD))
		a *= 0.70710678f; // 1 / sqrt(2)

	dir += a;
	return dL || dR || dU || dD;
}

Vector Avatar::getFakeCursorPosition()
{
	Vector dir;
	if(isActionAndGetDir(dir))
		return dir * 350;

	for(size_t i = 0; i < core->getNumJoysticks(); ++i)
		if(Joystick *j = core->getJoystick(i))
			if(j->isEnabled())
			{
				float axisInput = j->position.getLength2D();
				if(axisInput >= JOYSTICK_LOW_THRESHOLD)
				{
					const float axisMult = (maxMouse - minMouse) / (JOYSTICK_HIGH_THRESHOLD - JOYSTICK_LOW_THRESHOLD);
					const float distance = minMouse + ((axisInput - JOYSTICK_LOW_THRESHOLD) * axisMult);
					return (j->position * (distance / axisInput));
				}
			}

	return dir;
}

Vector Avatar::getVectorToCursorFromScreenCentre()
{
	if (game->cameraOffBounds)
		return getVectorToCursor();
	else
	{
		if (dsq->getInputMode() != INPUT_MOUSE)
			return getFakeCursorPosition();
		return (core->mouse.position+offset) - Vector(400,300);
	}
}

Vector Avatar::getVectorToCursor(bool trueMouse)
{
	//return getVectorToCursorFromScreenCentre();
	Vector pos = dsq->getGameCursorPosition();


	if (!trueMouse && dsq->getInputMode() != INPUT_MOUSE)
		return getFakeCursorPosition();

	return pos - (position+offset);
	//return core->mouse.position - Vector(400,300);
}

void Avatar::action(int id, int state, int source, InputDevice device)
{
	if(game->isIgnoreAction((AquariaActions)id))
		return;

	_lastActionSourceID = source;
	_lastActionInputDevice = device;

	if (id == ACTION_PRIMARY)	{ if (state) lmbd(source, device); else lmbu(source, device); }
	if (id == ACTION_SECONDARY) { if (state) rmbd(source, device); else rmbu(source, device); }

	if (id == ACTION_REVERT && !state)
		revert();

	if (id == ACTION_PRIMARY && state)// !state
	{
		if (dsq->isMiniMapCursorOkay())
		{
			if (this->state.lockedToWall)
			{
				Vector test = getVectorToCursor();
				if (test.isLength2DIn(minMouse))
				{
					fallOffWall();
				}
				else
				{
					if (boneLock.entity)
						wallNormal = boneLock.wallNormal;

					if (isUnderWater())
					{
						test.normalize2D();
						float dott = wallNormal.dot2D(test);
						burst = 1;
						bursting = false;
						// normal is 90 degrees within/on the right side
						if (dott > 0)
						{
							startWallBurst(true);
						}
						else
						{
							startWallBurst(false);
						}
					}
					else
					{
						test.normalize2D();
						float dott = wallNormal.dot2D(test);

						if (dott > -0.3f)
						{
							burst = 1;
							bursting = false;
							startWallBurst(true);
						}
						else
						{
							// nothing
						}
					}
				}

			}
			else
			{
				startBurst();
				// FIXME: This is a quick hack to make sure the burst
				// transition animation is played when using joystick or
				// keyboard control.  The same thing should probably be
				// done for wall bursts, but the movement there is fast
				// enough that people probably won't notice, so I skipped
				// that.  Sorry about the ugliness.  --achurch
				if (device != INPUT_MOUSE)
					skeletalSprite.transitionAnimate("swim", ANIM_TRANSITION, -1);
			}
		}
	}

	else if (id >= ACTION_SONGSLOT1 && id < ACTION_SONGSLOTEND)
	{
		if (canQuickSong())
		{
			int count = (id - ACTION_SONGSLOT1)+1;

			bool cast = false;

			if (dsq->continuity.form == FORM_SPIRIT)
			{
				revert();
				cast = true;
			}
			else
			{
				switch(count)
				{
				case 1:
				{
					if (dsq->continuity.form != FORM_NORMAL)
					{
						revert();
						cast = true;
					}
				}
				break;
				case 2:
					if (dsq->continuity.form != FORM_ENERGY)
					{
						dsq->continuity.castSong(SONG_ENERGYFORM);
						cast = true;
					}
				break;
				case 3:
					if (dsq->continuity.form != FORM_BEAST)
					{
						dsq->continuity.castSong(SONG_BEASTFORM);
						cast = true;
					}
				break;
				case 4:
					if (dsq->continuity.form != FORM_NATURE)
					{
						dsq->continuity.castSong(SONG_NATUREFORM);
						cast = true;
					}
				break;
				case 5:
					if (dsq->continuity.form != FORM_SUN)
					{
						dsq->continuity.castSong(SONG_SUNFORM);
						cast = true;
					}
				break;
				case 6:
					if (dsq->continuity.form != FORM_FISH)
					{
						dsq->continuity.castSong(SONG_FISHFORM);
						cast = true;
					}
				break;
				case 7:
					if (dsq->continuity.form != FORM_SPIRIT)
					{
						dsq->continuity.castSong(SONG_SPIRITFORM);
						cast = true;
					}
				break;
				case 8:
					if (dsq->continuity.form != FORM_DUAL)
					{
						dsq->continuity.castSong(SONG_DUALFORM);
						cast = true;
					}
				break;
				default:
					if (dsq->continuity.form == FORM_NORMAL)
					{
						switch(count)
						{
						case 9:
						{
							dsq->continuity.castSong(SONG_SHIELDAURA);
							cast = true;
						}
						break;
						case 10:
						{
							dsq->continuity.castSong(SONG_BIND);
							cast = true;
						}
						break;
						}
					}
				break;
				}
			}


			if (cast)
			{
				quickSongCastDelay = QUICK_SONG_CAST_DELAY;
			}
		}
	}
}

void Avatar::doBindSong()
{
	if (pullTarget)
	{
		pullTarget->stopPull();
		pullTarget = 0;
		core->sound->playSfx("Denied");
	}
	else
	{
		game->bindIngredients();
		setNearestPullTarget();
		if (!pullTarget)
		{
			core->sound->playSfx("Denied");
		}
		else
		{
			core->sound->playSfx("Bind");
		}
	}
}

void Avatar::doShieldSong()
{
	core->sound->playSfx("Shield-On");
	activateAura(AURA_SHIELD);
}

void Avatar::render(const RenderState& rs) const
{
	if (dsq->continuity.form == FORM_SPIRIT && !skeletalSprite.getParent())
		skeletalSprite.render(rs);

	Entity::render(rs);

}

void Avatar::onEnterState(int action)
{
	Entity::onEnterState(action);
	if (action == STATE_PUSH)
	{
		state.lockedToWall = false;
		Animation *a = skeletalSprite.getCurrentAnimation();
		if (!a || (a && a->name != "pushed"))
			skeletalSprite.animate("pushed", 0);
	}
}

void Avatar::onExitState(int action)
{
	Entity::onExitState(action);
	if (action == STATE_TRANSFORM)
	{
		setState(STATE_IDLE);
	}
	else if (action == STATE_PUSH)
	{
		skeletalSprite.transitionAnimate("spin", 0.1f);
	}
}

void Avatar::splash(bool down)
{
	if (splashDelay > 0)
	{
		lastJumpOutFromWaterBubble = false;
		return;
	}
	splashDelay = SPLASH_INTERVAL;

	if (down)
	{
		sound("splash-into", rolling ? 0.9f : 1.0f);
		//dsq->postProcessingFx.disable(FXT_RADIALBLUR);
		if (_isUnderWater && core->afterEffectManager)
			core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08f,0.05f,22,0.2f, 1.2f));
		dsq->rumble(0.7f, 0.7f, 0.2f, _lastActionSourceID, _lastActionInputDevice);
		plungeEmitter.start();

		core->sound->playSfx("GoUnder");
	}
	else
	{
		sound("splash-outof");
		/*
		dsq->postProcessingFx.enable(FXT_RADIALBLUR);
		dsq->postProcessingFx.radialBlurColor = Vector(1,1,1);
		dsq->postProcessingFx.intensity = 0.1;
		*/

		core->sound->playSfx("Emerge");
	}
	// make a splash effect @ current position

	// FIXME: This is broken for waterfalls/bubbles (e.g. the waterfalls
	// in the Turtle Cave).  Not sure how best to fix it in the current
	// code.  --achurch
	Vector hsplash = avatar->getHeadPosition();
	hsplash.y = game->waterLevel.x;
	core->createParticleEffect("HeadSplash", hsplash, LR_PARTICLES);

	float a = 0;

	if (waterBubble || lastJumpOutFromWaterBubble)
	{
		lastJumpOutFromWaterBubble = false;

		if (waterBubble)
		{
			Vector diff = position - waterBubble->nodes[0].position;
			a = MathFunctions::getAngleToVector(diff, 180);
		}
		else if (lastWaterBubble)
		{
			Vector diff = position - lastWaterBubble->nodes[0].position;
			a = MathFunctions::getAngleToVector(diff, 0);
		}
		else
			a = MathFunctions::getAngleToVector(vel+vel2, 0);
	}

	dsq->spawnParticleEffect("Splash", position, a);
}

void Avatar::clampVelocity()
{
	/*
	std::ostringstream os;
	os << "currentMaxSpeed: " << currentMaxSpeed;
	debugLog(os.str());
	*/
	float useSpeedMult = dsq->continuity.speedMult;
	bool inCurrent = isInCurrent();
	bool withCurrent = false;

	if (inCurrent)
	{
		// if vel2 and vel are pointing the same way
		if (vel2.dot2D(vel) > 0)
		{
			withCurrent = true;
		}
	}

	if (!inCurrent || (inCurrent && withCurrent))
	{
		if (dsq->continuity.form == FORM_FISH)
		{
			useSpeedMult *= MULT_MAXSPEED_FISHFORM;
		}
	}
	else
	{
		useSpeedMult = 1;
	}

	if (dsq->continuity.form == FORM_BEAST)
	{
		useSpeedMult *= MULT_MAXSPEED_BEASTFORM;
	}

	if (currentState == STATE_PUSH)
	{
		currentMaxSpeed = pushMaxSpeed;
	}


	setMaxSpeed(currentMaxSpeed * useSpeedMult * dsq->continuity.speedMult2);

	vel.capLength2D(getMaxSpeed() /* * maxSpeedLerp.x*/);
}

void Avatar::activateAura(AuraType aura)
{
	activeAura = aura;
	auraTimer = 30;
	if (aura == AURA_SHIELD)
	{
		shieldPoints = maxShieldPoints;
		if (auraLowEmitter.isRunning())
			auraLowEmitter.stop();
		auraEmitter.load("AuraShield");
		auraEmitter.start();
		if (dsq->loops.shield == BBGE_AUDIO_NOCHANNEL)
		{
			PlaySfx play;
			play.name = "Shield-Loop";
			play.fade = SFT_IN;
			play.time = 1;
			play.loops = -1;
			dsq->loops.shield = core->sound->playSfx(play);
		}
	}
}

void Avatar::updateAura(float dt)
{
	if (auraTimer > 0 && dsq->continuity.form != FORM_SPIRIT)
	{
		switch(activeAura)
		{
		case AURA_SHIELD:
		{
			//shieldPosition = position + Vector(cosf(auraTimer*4)*100, sinf(auraTimer*4)*100);
			shieldPosition = position;
			/*
			float a = ((rotation.z)*PI)/180.0f + PI*0.5f;
			shieldPosition = position + Vector(cosf(a)*100, sinf(a)*100);
			*/
			for (Shot::Shots::iterator i = Shot::shots.begin(); i != Shot::shots.end(); ++i)
			{
				Shot *s = *i;
				if (s->isActive() && game->isDamageTypeEnemy(s->getDamageType()) && s->firer != this
					&& (!s->shotData || !s->shotData->ignoreShield))
				{

					Vector diff = s->position - shieldPosition;
					if (diff.getSquaredLength2D() < sqr(AURA_SHIELD_RADIUS))
					{
						shieldPoints -= s->getDamage();
						auraHitEmitter.start();
						dsq->spawnParticleEffect("ReflectShot", s->position);
						core->sound->playSfx("Shield-Hit");
						s->position += diff;
						//s->target = 0;
						diff.setLength2D(s->maxSpeed);
						s->velocity = diff;
						s->reflectFromEntity(this);
					}
				}
			}
		}
		break;
		case AURA_THING:
		case AURA_HEAL:
		case AURA_NONE:
			break;
		}

		auraTimer -= dt;
		if (auraTimer < 5 || shieldPoints < 2)
		{
			if (auraEmitter.isRunning())
			{
				auraEmitter.stop();
				auraLowEmitter.load("AuraShieldLow");
				auraLowEmitter.start();
			}
		}

		if (auraTimer < 0 || shieldPoints < 0)
		{
			stopAura();
		}
	}
}

void Avatar::stopAura()
{
	auraTimer = 0;
	activeAura = AURA_NONE;
	auraEmitter.stop();
	auraLowEmitter.stop();
	if (dsq->loops.shield != BBGE_AUDIO_NOCHANNEL)
	{
		core->sound->fadeSfx(dsq->loops.shield, SFT_OUT, 1);
		dsq->loops.shield = BBGE_AUDIO_NOCHANNEL;
	}
}

void Avatar::setHeadTexture(const std::string &name, float time)
{
	if (!bone_head) return;
	if (dsq->continuity.form == FORM_NORMAL /*&& dsq->continuity.costume.empty()*/)
	{
		if (!name.empty() && (nocasecmp(lastHeadTexture, "singing")==0)) return;

		lastHeadTexture = name;
		stringToUpper(lastHeadTexture);
		std::string t = "Naija/";

		if (!dsq->continuity.costume.empty())
		{
			if (nocasecmp(dsq->continuity.costume, "end")==0)
				t += "Naija2";
			else
				t += dsq->continuity.costume;
		}
		else if (dsq->continuity.form == FORM_BEAST)
		{
			t += "Beast";
		}
		else
		{
			t += "Naija2";
		}
		t += "-Head";
		if (!name.empty())
		{
			t += "-" + name;
		}
		bone_head->setTexture(t);

		headTextureTimer = time;
	}
}

void Avatar::updateFormVisualEffects(float dt)
{
	switch (dsq->continuity.form)
	{
	case FORM_ENERGY:
	{
		Vector hairDir(96, -96);
		if (this->isfh())
		{
			hairDir.x = -hairDir.x;
		}
	}
	break;
	case FORM_SUN:
	{
		lightFormGlow->position = this->position;
		lightFormGlowCone->position = this->position;

		float angle=0;
		MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), getVectorToCursorFromScreenCentre(), angle);
		angle = 180-(360-angle);

		//lightFormGlowCone->rotation.interpolateTo(Vector(0,0,angle), 0.1);
		lightFormGlowCone->rotation = Vector(0,0,angle);

		static float lfgTimer = 0;
		lfgTimer += dt;
		if (lfgTimer > 0.5f)
		{
			//debugLog("lightFormGlow to front");
			lightFormGlow->moveToFront();
			lightFormGlowCone->moveToFront();
			lfgTimer = 0;
		}

		if (this->isInDarkness())
		{
			lightFormGlowCone->alphaMod = 1;
			lightFormGlow->alphaMod = 1;
		}
		else
		{
			lightFormGlow->alphaMod = 0;
			lightFormGlowCone->alphaMod = 0;
		}
	}
	break;
	case FORM_SPIRIT:
		skeletalSprite.update(dt);
		skeletalSprite.position = bodyPosition + bodyOffset;
	break;
	case FORM_NORMAL:
	case FORM_BEAST:
	case FORM_NATURE:
	case FORM_DUAL:
	case FORM_FISH:
	case FORM_MAX:
	case FORM_NONE:
		break;
	}
}

void Avatar::stopBurst()
{
	burst = 0;
	//burstDelay = BURST_DELAY;
	burstDelay = 0;
	bursting = false;
	wakeEmitter.stop();
	ripples = false;

	biteLeftEmitter.stop();
	biteRightEmitter.stop();
}

int Avatar::getCursorQuadrant()
{
	//Vector diff = getVectorToCursorFromScreenCentre();
	Vector diff = getVectorToCursor();
	if (diff.isLength2DIn(40))
	{
		stopRoll();
		return -999;
	}

	if (diff.y < 0)
		return diff.x < 0 ? 4 : 1;
	else
		return diff.x < 0 ? 3 : 2;
}

int Avatar::getQuadrantDirection(int lastQuad, int quad)
{
	int diff = quad - lastQuad;
	if ((lastQuad==4 && quad == 1))
	{
		diff = 1;
	}
	if (lastQuad==1 && quad==4)
	{
		diff = -1;
	}
	if (abs(diff) != 1)
		diff = 0;
	return diff;
}

void Avatar::startRoll(int dir)
{
	if (!rolling && !state.backFlip)
	{
		if (dsq->continuity.form == FORM_ENERGY && dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY1))
		{
			rollRightEmitter.load("EnergyRollRight");
			rollLeftEmitter.load("EnergyRollLeft");
		}
		else
		{
			rollRightEmitter.load("RollRight");
			rollLeftEmitter.load("RollLeft");
		}
	}

	Animation *a = skeletalSprite.getCurrentAnimation();
	if (!a || a->name != getRollAnimName())
	{
		skeletalSprite.transitionAnimate(getRollAnimName(), 0.2f, -1);
	}

	rollRightEmitter.stop();
	rollLeftEmitter.stop();

	if (_isUnderWater)
	{
		if (dir >= 1)
			rollRightEmitter.start();

		if (dir <= -1)
			rollLeftEmitter.start();
	}

	//dsq->playVisualEffect(VFX_RIPPLE, Vector());
	rolling = true;

	if (dsq->loops.roll == BBGE_AUDIO_NOCHANNEL && _isUnderWater)
	{
		PlaySfx play;
		play.name = "RollLoop";
		play.fade = SFT_IN;
		play.time = 1;
		play.vol = 1;
		play.loops = -1;
		dsq->loops.roll = core->sound->playSfx(play);
	}
	else if (dsq->loops.roll != BBGE_AUDIO_NOCHANNEL && !_isUnderWater)
	{
		core->sound->fadeSfx(dsq->loops.roll, SFT_OUT, 0.5f);
	}

	rollDir = dir;

	if (_isUnderWater && core->afterEffectManager)
		core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08f,0.05f,22,0.2f, 1.2f));


	//rollDelay = 0.3;
}

void Avatar::stopRoll()
{
	rolling = false;
	lastQuadDir = lastQuad = 0;
	rollDelay = 0;
	rollDidOne = 0;
	rollLeftEmitter.stop();
	rollRightEmitter.stop();
	state.rollTimer = 0;

	if (dsq->loops.roll != BBGE_AUDIO_NOCHANNEL)
	{
		core->sound->fadeSfx(dsq->loops.roll, SFT_OUT, 1);
		dsq->loops.roll = BBGE_AUDIO_NOCHANNEL;
	}
}


void Avatar::stopWallJump()
{
	wallBurstTimer = 0;
}

void Avatar::updateWallJump(float dt)
{
	if (wallBurstTimer > 0)
	{
		wallBurstTimer -= dt;
		if (wallBurstTimer < 0)
		{
			// wall jump failed!
			stopWallJump();
		}
	}
}

void Avatar::updateRoll(float dt)
{
	if (!inputEnabled || game->isWorldPaused() || riding)
	{
		if (rolling)
			stopRoll();
		return;
	}
	if (state.lockedToWall || isSinging()) return;

	if (rollDelay > 0)
	{
		rollDelay -= dt;
		if (rollDelay <= 0)
		{
			// stop the animation
			stopRoll();
		}
	}
	const bool rollact = isActing(ACTION_ROLL, -1);
	if (!_isUnderWater && rollact)
	{
		stopRoll();
	}

	if (!core->mouse.buttons.left && dsq->getInputMode() == INPUT_MOUSE && !rollact)
	{
		if (rolling)
			stopRoll();
		return;
	}

	if (rolling)
	{
		if (dsq->continuity.form == FORM_ENERGY && dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY1))
		{
			FOR_ENTITIES(i)
			{
				Entity *e = *i;
				if (e->getEntityType() == ET_ENEMY && (e->position - this->position).isLength2DIn(256) && e->isDamageTarget(DT_AVATAR_ENERGYROLL))
				{
					DamageData d;
					d.damage = dt*15;
					d.damageType = DT_AVATAR_ENERGYROLL;
					d.attacker = this;
					e->damage(d);
				}
			}
		}

		state.rollTimer += dt;
		if (state.rollTimer > 0.55f)
		{
			state.rollTimer = 0;
			if (dsq->continuity.form == FORM_DUAL)
			{
				switchDualFormMode();
				state.rollTimer = -1;
			}
		}

		// NOTE: does this fix the roll problem?
		if (rollDelay <= 0)
			stopRoll();
	}

	if (rollact)
	{
		if (_isUnderWater)
		{
			if (rollDelay < 0.5f)
			{
				startRoll(isfh()?1:-1);
			}
			float amt = dt * 1000;
			if (isfh())
			{
				rotation.z += amt;
			}
			else
			{
				rotation.z -= amt;
			}
			rotation.capRotZ360();

			rollDelay = 1.0;
		}
	}
	else
	{
		int quad = getCursorQuadrant();
		if (lastQuad != quad)
		{
			int quadDir = 0;
			if (lastQuad != 0)
			{
				quadDir = getQuadrantDirection(lastQuad, quad);
				if (quadDir != 0 && lastQuadDir == quadDir && rollDelay > 0)
				{
					if (rolling)
					{
						startRoll(quadDir);
					}
					else
					{
						if (rollDidOne==1)
							rollDidOne = 2;
						else if (rollDidOne == 2)
							startRoll(quadDir);
						else
							rollDidOne = 1;
					}
				}
			}

			lastQuadDir = quadDir;

			lastQuad = quad;

			rollDelay = 0.2f;
		}
	}
}

int Avatar::getStopDistance()
{
	return STOP_DISTANCE;
}

int Avatar::getBurstDistance()
{
	return BURST_DISTANCE;
}

void Avatar::setWasUnderWater()
{
	state.wasUnderWater = isUnderWater();
}

bool Avatar::canActivateStuff()
{
	return _canActivateStuff;
}

void Avatar::setCanActivateStuff(bool on)
{
	_canActivateStuff = on;
}

void Avatar::setCollisionAvoidanceData(int range, float mod)
{
	_collisionAvoidRange = range;
	_collisionAvoidMod = mod;
}

bool Avatar::canQuickSong()
{
	return !isSinging() && !isEntityDead() && isInputEnabled() && quickSongCastDelay <= 0;
}

void Avatar::applyRidingPosition()
{
	if (riding)
	{
		position = riding->getRidingPosition();
		lastPosition = position;
		rotation.z = riding->getRidingRotation();

		if (riding->getRidingFlip())
		{
			if (!isfh())
				flipHorizontal();
		}
		else
		{
			if (isfh())
				flipHorizontal();
		}
		//state.wasUnderWater = _isUnderWater;
	}
}

void Avatar::adjustHeadRot()
{
	if (bone_head)
	{
		// 0 to 30 range
		if (bone_head->rotation.z > 0)
		{
			bone_head->internalOffset.x = (bone_head->rotation.z/30.0f)*5;
			//bone_head->internalOffset.y = (bone_head->rotation.z/30.0f)*1;
		}
		// 0 to -10 range
		if (bone_head->rotation.z < 0)
		{
			bone_head->internalOffset.x = (bone_head->rotation.z/(-10.0f))*-4;
			bone_head->internalOffset.y = (bone_head->rotation.z/(-10.0f))*-2;
		}
	}
}

void Avatar::endOfGameState()
{
	state.lookAtEntity = 0;
	setInvincible(true);
}

bool didRotationFix = true;

void timerEffectStart(Timer *timer, ParticleEffect *effect)
{
	if (timer->isActive() && !effect->isRunning())
	{
		effect->start();
	}
	else if (!timer->isActive() && effect->isRunning())
	{
		effect->stop();
	}
}

void Avatar::updateFoodParticleEffects()
{
	timerEffectStart(&dsq->continuity.speedMultTimer, &speedEmitter);
	timerEffectStart(&dsq->continuity.defenseMultTimer, &defenseEmitter);
	timerEffectStart(&dsq->continuity.invincibleTimer, &invincibleEmitter);
	timerEffectStart(&dsq->continuity.regenTimer, &regenEmitter);
}

void Avatar::updateLookAt(float dt)
{
	//if (dsq->overlay->alpha != 0) return;
	if (game->isShuttingDownGameState()) return;
	if (headTextureTimer > 0)
	{
		headTextureTimer -= dt;
		if (headTextureTimer <= 0)
		{
			headTextureTimer = 0;
			setHeadTexture("");
		}
	}

	if (dsq->continuity.form == FORM_FISH)
	{
		Bone *b = skeletalSprite.getBoneByIdx(0);
		if (b)
			b->setAnimated(Bone::ANIM_ALL);
		return;
	}

	const float blinkTime = 5.0;
	state.blinkTimer += dt;
	if (state.blinkTimer > blinkTime)
	{
		if (lastHeadTexture.empty())
		{
			setHeadTexture("blink", 0.1f);
			if (chance(50))
			{
				state.blinkTimer = blinkTime-0.2f;
			}
			else
			{
				state.blinkTimer = rand()%2;
			}
		}
		else
		{
			state.blinkTimer -= dt;
		}
	}

	if (bone_head)
	{
		const float lookAtTime = 0.8f;
		if (core->mouse.buttons.middle && !state.lockedToWall && isInputEnabled())
		{
			didRotationFix = false;
			bone_head->setAnimated(Bone::ANIM_POS);
			bone_head->lookAt(dsq->getGameCursorPosition(), lookAtTime, -10, 30, -90);
			adjustHeadRot();
		}
		else
		{
			if (state.lookAtEntity && (state.lookAtEntity->isEntityDead() || state.lookAtEntity->isDead() || state.lookAtEntity->isv(EV_LOOKAT,0) || swimming))
			{
				state.lookAtEntity = 0;
			}
			// find an object of interest
			if (isv(EV_LOOKAT, 1) && state.lookAtEntity && !skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->isAnimating() && !state.lockedToWall && !swimming)
			{
				didRotationFix = false;
				bone_head->setAnimated(Bone::ANIM_POS);
				bone_head->lookAt(state.lookAtEntity->getLookAtPoint(), lookAtTime, -10, 30, -90);

				if (!((state.lookAtEntity->position - position).isLength2DIn(1000)))
				{
					state.lookAtEntity = 0;
				}
				state.updateLookAtTime += dt;
				adjustHeadRot();
			}
			else
			{
				bone_head->setAnimated(Bone::ANIM_ALL);

				if (!didRotationFix && !bone_head->rotationOffset.isInterpolating())
				{
					float t = 1;
					didRotationFix = true;
					float oldRot = bone_head->rotation.z;

					skeletalSprite.updateBones();

					bone_head->rotationOffset.z = oldRot - bone_head->rotation.z;
					bone_head->rotationOffset.interpolateTo(Vector(0,0,0), t);
					bone_head->internalOffset.interpolateTo(Vector(0,0,0), t);
				}

				state.updateLookAtTime += dt*4*2;
				bone_head->internalOffset.interpolateTo(Vector(0,0), 0.2f);
			}

			if (state.updateLookAtTime > 1.5f)
			{
				state.lookAtEntity = game->getNearestEntity(position, 800, this, ET_NOTYPE, DT_NONE, LR_ENTITIES0, LR_ENTITIES2);
				if (state.lookAtEntity && state.lookAtEntity->isv(EV_LOOKAT, 1))
				{
					state.updateLookAtTime = 0;

					if (!state.lookAtEntity->naijaReaction.empty())
					{
						setHeadTexture(state.lookAtEntity->naijaReaction, 1.5f);
					}
				}
				else
				{
					state.lookAtEntity = 0;

				}
			}
		}
	}
}

Vector Avatar::getHeadPosition()
{
	if (bone_head)
		return bone_head->getWorldPosition();
	return position;
}

bool lastCursorKeyboard = false;

void Avatar::onUpdate(float dt)
{
	looking = 0;


	if (lightFormGlow)
	{
		if (dsq->continuity.light)
		{
			lightFormGlow->scale = Vector(6,6) + Vector(4,4)*dsq->continuity.light;
		}
		else
		{
			lightFormGlow->scale = Vector(6,6);
		}
	}

	applyRidingPosition();

	if (bone_head)
		headPosition = bone_head->getWorldPosition();

	//vel /= 0;
	if (vel.isNan())
	{
		debugLog("detected velocity NaN");
		vel = Vector(0,0);
	}

	if (fireDelay > 0)
	{
		fireDelay -= dt;
		if (fireDelay < 0)
		{
			fireDelay = 0;
		}
	}

	if (isInputEnabled())
	{
		if (web)
		{
			if (!webBitTimer.isActive())
			{
				webBitTimer.start(0.5);
			}
			web->setPoint(curWebPoint, position);

			if (webBitTimer.updateCheck(dt))
			{
				webBitTimer.start(0.5);

				curWebPoint = web->addPoint(position);
			}
		}

		if (!game->isPaused() && isActing(ACTION_LOOK, -1) && !game->avatar->isSinging() && game->avatar->isInputEnabled() && !game->isInGameMenu())
		{
			looking = 1;
		}
		else
		{
			looking = 0;
		}
	}
	else
	{
		looking = 0;
	}


	Entity::onUpdate(dt);

	if (isEntityDead() && skeletalSprite.getCurrentAnimation()->name != "dead")
	{
		fallOffWall();
		biteLeftEmitter.stop();
		biteRightEmitter.stop();
		wakeEmitter.stop();
		rollLeftEmitter.stop();
		rollRightEmitter.stop();
		game->toggleOverrideZoom(false);
		if (dsq->continuity.form != FORM_NORMAL)
			changeForm(FORM_NORMAL);
		setHeadTexture("Pain");
		core->globalScale.interpolateTo(Vector(5,5),3);
		rotation.interpolateTo(Vector(0,0,0), 0.1f);
		skeletalSprite.animate("dead");
	}
	if (isEntityDead())
	{
		game->toggleOverrideZoom(false);
	}

	if (dsq->user.control.targeting)
		updateTargets(dt, false);
	else
		targets.clear();

	updateTargetQuads(dt);

	updateDualFormGlow(dt);
	updateLookAt(dt);

	updateFoodParticleEffects();

	if (!game->isPaused())
		myZoom.update(dt);

	_isUnderWater = isUnderWater();

	splashDelay -= dt;
	if (splashDelay < 0)
		splashDelay = 0;

	// JUMPING OUT
	if (!_isUnderWater && state.wasUnderWater)
	{

		// "falling" out, not bursting out
		int fallOutSpeed = 200;
		/*
		if (waterBubble)
			fallOutSpeed = 400;
		*/
		//bool waterBubbleRect = (waterBubble && waterBubble->pathShape == PATHSHAPE_RECT);

		//&& !waterBubbleRect
		if (!riding && (!bursting && vel.isLength2DIn(fallOutSpeed)))
		{

			if (waterBubble)
			{
				// prevent from falling out
				// if circle, clamp
				waterBubble->clampPosition(&position);
				vel *= 0.5f;
				startBurstCommon();
			}
			else
			{
				if (!game->waterLevel.isInterpolating())
				{
					if (vel.y < 0)
						vel.y = -vel.y*0.5f;
					position.y = game->waterLevel.x + collideRadius;
				}
			}
		}
		else
		{
			if (waterBubble)
				lastJumpOutFromWaterBubble = true;
			else
				lastJumpOutFromWaterBubble = false;

			lastWaterBubble = waterBubble;
			waterBubble = 0;
			splash(false);

			if (dsq->continuity.form != FORM_FISH)
			{
				vel *= vars->jumpVelocityMod; // 1.25f;
				vel.capLength2D(2000);
				currentMaxSpeed *= 2.0f;
			}
			else
			{
				vel *= 1.5f;
				vel.capLength2D(1500);
				currentMaxSpeed *= 1.5f;
			}

			// total max speed
			fallGravityTimer = 0.0;

			wakeEmitter.stop();
			biteTimer = 0;
			//stopBurst();

			// if first time
			if (!dsq->mod.isActive() && dsq->continuity.getFlag("leftWater")==0 && game->sceneName.find("veil")!=std::string::npos)
			{
				setInvincible(true);
				setv(EV_NOINPUTNOVEL, 0);

				setWasUnderWater();

				if (vel.y > -500)
					vel.y = -500;
				dsq->continuity.setFlag("leftWater", 1);

				core->sound->fadeMusic(SFT_OUT, 2);
				//("Veil");
				game->avatar->disableInput();
				dsq->gameSpeed.interpolateTo(0.1f, 0.5f);

				//dsq->sound->setMusicFader(0.5, 0.5);
				core->sound->playSfx("NaijaGasp");
				core->run(0.75);



				dsq->voiceOnce("Naija_VeilCrossing");
				core->run(10*0.1f);

				dsq->gameSpeed.interpolateTo(1, 0.2f);

				dsq->sound->playMusic("Veil", SLT_LOOP, SFT_CROSS, 20);

				//dsq->sound->setMusicFader(1.0, 1);

				game->avatar->enableInput();

				setv(EV_NOINPUTNOVEL, 1);

				setInvincible(false);

				//dsq->continuity.setFlag("leftWater", 0);

			}


			state.outOfWaterTimer = 0;
			state.outOfWaterVel = vel;
			//startBackFlip();
		}

		if (currentMaxSpeed > dsq->v.maxOutOfWaterSpeed)
		{
			currentMaxSpeed = dsq->v.maxOutOfWaterSpeed;
		}
		if (currentMaxSpeed < 1200)
		{
			currentMaxSpeed = 1200;
		}
	}
	// JUMPING IN
	else if (_isUnderWater && !state.wasUnderWater)
	{
		// falling in
		splash(true);

		lastOutOfWaterMaxSpeed = getMaxSpeed();
		lastOutOfWaterMaxSpeed *= 0.75f;

		if (lastOutOfWaterMaxSpeed > 1000)
			lastOutOfWaterMaxSpeed = 1000;

		fallGravityTimer = 0.5;
		if (rolling)
			fallGravityTimer *= 1.5f;
		stopBurst();

		if (state.backFlip)
		{
			stopBackFlip();
		}

	}

	state.wasUnderWater = _isUnderWater;

	if (!_isUnderWater)
	{
		state.outOfWaterTimer += dt;
		if (state.outOfWaterTimer > 100)
			state.outOfWaterTimer = 100;
	}


	if (!state.backFlip && !_isUnderWater && state.outOfWaterTimer < 0.1f && !riding && !boneLock.on)
	{
		const int check = 64;
		Vector m = getVectorToCursor();
		if (state.outOfWaterVel.x < 0 && m.x > check)
		{
			startBackFlip();
		}
		if (state.outOfWaterVel.x > 0 && m.x < -check)
		{
			startBackFlip();
		}
	}

	if (isEntityDead())
	{
		updateHair(dt);
	}

	if (isEntityDead()) return;

	if (flourishTimer.updateCheck(dt))
	{
		flourish = 0;
		rotationOffset.z = 0;
	}

	if (isInputEnabled())
		stillTimer.update(dt);

	if (vel.isZero()) //&& !isSinging())
	{
		if (!stillTimer.isActive())
		{
			stillTimer.startStopWatch();
			//debugLog("start stillTimer");
		}
	}
	else
	{
		stillTimer.stop();
	}

	flourishPowerTimer.updateCheck(dt);

	if (isSinging())
	{
		if (songInterfaceTimer < 1)
			songInterfaceTimer += dt;
	}

	if (quickSongCastDelay>0)
	{
		quickSongCastDelay -= dt;
		if (quickSongCastDelay < 0)
			quickSongCastDelay = 0;
	}
	if (ripples && _isUnderWater)
	{
		rippleDelay -= dt;
		if (rippleDelay < 0)
		{
			if (core->afterEffectManager)
				core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),position+offset,0.04f,0.06f,15,0.2f));
			rippleDelay = 0.15f;
		}
	}

	if (dsq->continuity.tripTimer.isActive())
	{
		static int tripCount = 0;
		tripDelay -= dt;
		if (tripDelay < 0)
		{
			tripDelay = 0.15f;
			tripCount ++;
			if (tripCount > 10)
			{
				float p = dsq->continuity.tripTimer.getPerc();
				if (p > 0.6f)
				{
					if (core->afterEffectManager)
						core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),position+offset,0.04f,0.06f,15,0.2f));
				}
				else
				{
					if (core->afterEffectManager)
						core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),position+offset,0.4f,0.6f,15,0.2f));
				}
				if (p > 0.75f){}
				else if (p > 0.5f)
				{
					dsq->shakeCamera(2, 4);
					if (chance(80))
					{
						if (chance(60))
							dsq->emote.playSfx(EMOTE_NAIJALAUGH);
						else
							dsq->emote.playSfx(EMOTE_NAIJAEVILLAUGH);
					}
				}
				else
				{
					if (p < 0.2f)
						dsq->shakeCamera(10, 4);
					else
						dsq->shakeCamera(5, 4);
					tripper->color.interpolateTo(Vector(1, 0.2f, 0.2f), 3);
					if (chance(75))
						dsq->emote.playSfx(EMOTE_NAIJAUGH);
				}

				tripCount = 0;
			}
		}
	}

	if (position.isInterpolating())
	{
		lastPosition = position;
	}

	updateFormVisualEffects(dt);
	updateRoll(dt);
	updateWallJump(dt);

	if (formAbilityDelay > 0)
	{
		formAbilityDelay -= dt;
		if (formAbilityDelay < 0)
			formAbilityDelay = 0;
	}
	//updateCursor(dt);

	if (getState() == STATE_PUSH)
	{
		/*
		if (rotation.z < 0)
			rotation.z += 360;
		if (rotation.z > 360)
			rotation.z -= 360;
		*/
		rotateToVec(vel, 0, -90);
		if (vel.x < 0&& !isfh())
			flipHorizontal();
		else if (vel.x > 0 && isfh())
			flipHorizontal();
	}

	updateAura(dt);

	updateSingingInterface(dt);

	if (pullTarget)
	{
		if (pullTarget->life < 1 || !pullTarget->isPullable())
		{
			pullTarget->stopPull();
			pullTarget = 0;
		}
	}

	formTimer += dt;


	if (dsq->continuity.form == FORM_SPIRIT)
	{
		if (formTimer > 1)
		{
			if (!(bodyPosition - position).isLength2DIn(SPIRIT_RANGE))
			{
				changeForm(FORM_NORMAL);
			}
		}
		// here
		if (!_isUnderWater)
		{
			changeForm(FORM_NORMAL);
		}
	}

	// revert stuff
	float revertGrace = 0.4f;
	static bool revertButtonsAreDown = false;
	if (inputEnabled && (dsq->getInputMode() == INPUT_KEYBOARD || dsq->getInputMode() == INPUT_MOUSE) && (!pathToActivate && !entityToActivate))
	{
		if (dsq->continuity.form != FORM_NORMAL && (core->mouse.pure_buttons.left && core->mouse.pure_buttons.right) && getVectorToCursor(true).isLength2DIn(minMouse))
		{
			if (!revertButtonsAreDown)
			{
				revertTimer = revertGrace;
				revertButtonsAreDown = true;
			}
			else if (revertButtonsAreDown)
			{
				if (revertTimer > 0)
				{
					revertTimer -= dt;
					if (revertTimer < 0)
					{
						revertTimer = 0;
					}
				}
			}
		}
		//&& !isActing(ACTION_PRIMARY) && !isActing(ACTION_SECONDARY)
		else if ((!core->mouse.pure_buttons.left && !core->mouse.pure_buttons.right))
		{
			if (revertTimer > 0 && getVectorToCursor(true).isLength2DIn(minMouse) && state.spellCharge < revertGrace+0.1f)
			{
				revert();
				//changeForm(FORM_NORMAL);
			}
			revertButtonsAreDown = false;
			revertTimer = 0;
		}
	}
	else
	{
		revertButtonsAreDown = false;
	}

	//if (core->getNestedMains() == 1)
	{
		if (getState() != STATE_TRANSFORM && !game->isWorldPaused())
		{
			formAbilityUpdate(dt);
		}

		if (state.useItemDelay.updateCheck(dt))
		{
		}

		ActionMapper::onUpdate(dt);

		if (inputEnabled)
		{
			if (state.blind)
			{
				if (state.blindTimer.updateCheck(dt))
				{
					state.blind = false;
					removeBlindEffects();
				}
			}
		}


		if (boneLock.entity != 0)
		{
			/*
			std::ostringstream os;
			os << "boneLock.wallNormal(" << boneLock.wallNormal.x << ", " << boneLock.wallNormal.y << ")";
			debugLog(os.str());
			*/
			if (!_isUnderWater && !(boneLock.wallNormal.y < -0.03f))
			{
				if (lockToWallFallTimer == -1)
					lockToWallFallTimer = 0.4f;
			}
			else
				lockToWallFallTimer = -1;
		}

		if (lockToWallFallTimer > 0)
		{
			lockToWallFallTimer -= dt;
			if (lockToWallFallTimer <= 0)
			{
				fallOffWall();
			}
		}

		if (state.lockToWallDelay.updateCheck(dt))
		{
		}

		if (pushingOffWallEffect > 0)
		{
			pushingOffWallEffect -= dt;
			if (pushingOffWallEffect <= 0)
			{
				pushingOffWallEffect = 0;
				if (vel.getSquaredLength2D() > sqr(1200))
				{
					vel.setLength2D(1200);
				}
			}
		}

		if (charging)
		{
			state.spellCharge += dt;
			switch (dsq->continuity.form)
			{
			case FORM_SUN:
			{
				if (state.spellCharge > 1.5f && chargeLevelAttained <1)
				{
					chargeLevelAttained = 1;
					core->sound->playSfx("PowerUp");
					chargingEmitter->load("ChargingEnergy2");
				}
			}
			break;
			case FORM_DUAL:
			{
				if (state.spellCharge >= 1.4f && chargeLevelAttained<1)
				{
					chargeLevelAttained = 1;

					core->sound->playSfx("PowerUp");
					//debugLog("charge visual effect 2");
					chargeEmitter->load("ChargeDualForm");
					chargeEmitter->start();

					chargingEmitter->load("ChargedDualForm");
					chargingEmitter->start();
				}
			}
			break;
			case FORM_ENERGY:
			{
				if (state.spellCharge >= 1.5f && chargeLevelAttained<2)
				{
					chargeLevelAttained = 2;
					core->sound->playSfx("PowerUp");
					//debugLog("charge visual effect 2");
					chargeEmitter->load("ChargeEnergy");
					chargeEmitter->start();


					chargingEmitter->load("ChargingEnergy2");
				}
			}
			break;
			case FORM_NATURE:
			{
				if (state.spellCharge >= 0.9f && chargeLevelAttained<2)
				{
					chargeLevelAttained = 2;
					core->sound->playSfx("PowerUp");
					chargeEmitter->load("ChargeNature2");
					chargeEmitter->start();

					chargingEmitter->load("ChargingNature2");
					chargingEmitter->start();
				}
			}
			break;
			case FORM_NORMAL:
			case FORM_BEAST:
			case FORM_SPIRIT:
			case FORM_FISH:
			case FORM_MAX:
			case FORM_NONE:
				break;
			}
		}
		/*
		float angle = PI - ((rotation.z/180)*PI);
		int height = 25;
		*/
		//hair->hairNodes[0].position = position + Vector(sinf(angle)*height, cosf(angle)*height);


		if (biteTimer < biteTimerMax)
		{
			biteTimer += dt;
		}
		else
		{
			biteLeftEmitter.stop();
			biteRightEmitter.stop();
			biteTimer = biteTimerMax;
		}

		if (biteTimer > biteTimerBiteRange)
		{
			biteLeftEmitter.stop();
			biteRightEmitter.stop();
		}

		/*
		std::ostringstream os;
		os << "biteTimer: " << biteTimer;
		debugLog(os.str());
		*/

		if (isInputEnabled())
		{
			if (dsq->continuity.form == FORM_NORMAL && nocasecmp(dsq->continuity.costume, "urchin") == 0)
			{
				if (!isEntityDead() && health > 0)
				{
					urchinDelay -= dt;
					if (urchinDelay < 0)
					{
						urchinDelay = 0.1f;

						game->fireShot("urchin", this, 0, position + offset);
					}
				}
			}

			if (dsq->continuity.form == FORM_NORMAL && nocasecmp(dsq->continuity.costume, "jelly")==0)
			{
				if (!isEntityDead() && health > 0)
				{
					if (health < (maxHealth*JELLYCOSTUME_HEALTHPERC))
					{
						jellyDelay -= dt;
						if (jellyDelay < 0)
						{
							jellyDelay = JELLYCOSTUME_HEALDELAY;

							Vector d;
							if (!vel.isZero())
							{
								d = vel;
								d.setLength2D(16);
							}

							game->spawnManaBall(position + offset + d, JELLYCOSTUME_HEALAMOUNT);

							//Shot *s = game->fireShot("urchin", this, 0, getWorldPosition());
						}
					}
				}
			}
		}

		if (dsq->continuity.form == FORM_BEAST && bone_head && biteTimer < biteTimerBiteRange && biteTimer > 0)
		{
			biteDelay -= dt;
			if (biteDelay < 0)
			{
				biteDelay = biteDelayPeriod;

				Vector p = bone_head->getWorldPosition();
				std::string shot = "Bite";
				if (dsq->continuity.biteMult > 1)
				{
					shot = "SuperBite";
				}
				game->fireShot(shot, this, 0, p);
				//s->setAimVector(getNormal());
			}
		}

		if (dsq->continuity.form == FORM_FISH && dsq->continuity.fishPoisonTimer.isActive())
		{
			if (!vel.isLength2DIn(16))
			{
				static float fishPoison = 0;
				fishPoison += dt;
				if (fishPoison > 0.2f)
				{
					fishPoison = 0;
					game->fireShot("FishPoison", this, 0, position);
				}
			}
		}

		if (!state.lockedToWall && _isUnderWater && !game->isWorldPaused() && canMove)
		{
			if (bursting)
			{
				//debugLog("bursting~!");
				burst -= dt * BURST_USE_RATE;
				burstTimer += dt;

				/*
				std::ostringstream os;
				os << "burst: " << burst;
				debugLog(os.str());
				*/
				if (burst <= 0)
				{
					stopBurst();
				}
			}


		}
		if (inputEnabled && _isUnderWater)
		{
			if(bursting)
			{
			}
			else if (burstDelay > 0)
			{
				burstDelay -= dt;
				if (burstDelay <= 0)
					burstDelay = 0;
			}
			else if (burst < 1)
			{
				burst += BURST_RECOVER_RATE * dt;
				if (burst >= 1)
					burst = 1;
			}
		}

		bool moved = false;

		//check to make sure there's still a wall there, if not fall off
		if (state.lockedToWall)
		{
			rotateToVec(wallPushVec, dt*2);
			if (!boneLock.on && !game->isObstructed(wallLockTile))
			{
				//debugLog("Dropping from wall");
				fallOffWall();
			}
		}

		if (getState() != STATE_PUSH && !state.lockedToWall && inputEnabled && _isUnderWater && canMove)
		{
			float a = 800*dt;
			Vector addVec;

			bool isMovingSlow = false;

			float len = 0;

			if (dsq->isMiniMapCursorOkay() && !isActing(ACTION_ROLL, -1) &&
				_isUnderWater && !riding && !boneLock.on &&
				(movingOn || ((dsq->getInputMode() == INPUT_JOYSTICK || dsq->getInputMode()== INPUT_KEYBOARD) || (core->mouse.buttons.left || bursting))))
			{
				const bool isMouse = dsq->getInputMode() == INPUT_MOUSE;
				if (isMouse || !this->singing)
				{
					addVec = getVectorToCursorFromScreenCentre();//getVectorToCursor();

					if (isMouse)
					{
						static Vector lastAddVec;
						if (!isActing(ACTION_PRIMARY, -1) && bursting)
						{
							addVec = lastAddVec;
						}

						if (bursting)
						{
							lastAddVec = addVec;
						}
					}

					if (addVec.isLength2DIn(minMouse))
					{
						if (dsq->getInputMode() == INPUT_JOYSTICK)
							addVec = Vector(0,0,0);
					}

					if (!addVec.isLength2DIn(minMouse))
					{
						//if (core->mouse.buttons.left)
						{
							len = addVec.getLength2D();
							if (len > 100)
								addVec.setLength2D(a *2);
							else
								addVec.setLength2D(a);

							addVec *= dsq->continuity.speedMult;

							//128
							if (len < maxMouse && !bursting)
							{
								isMovingSlow = true;
							}
						}
					}
					else
					{
						// stop movement
						// For joystick/keyboard control, don't stop unless
						// the Swim (primary action) button is pressed with
						// no movement input.  --achurch
						if ((isMouse || isActing(ACTION_PRIMARY, -1))
							&& addVec.isLength2DIn(STOP_DISTANCE))
						{
							vel *= 0.9f;
							if (!rolling)
								rotation.interpolateTo(Vector(0,0,0), 0.1f);
							if (vel.isLength2DIn(50))
							{
								if (bursting)
								{
									stopBurst();
								}
							}
						}
						addVec = Vector(0,0,0);
					}
				}
			}

			if (!rolling && !state.backFlip && !flourish)
			{
				if (addVec.x > 0)
				{
					if (!isfh())
						flipHorizontal();
				}
				if (addVec.x < 0)
				{
					if (isfh())
						flipHorizontal();
				}
			}

			// will not get here if not underwater
			if (isLockable())
				lockToWall();
			if ((addVec.x != 0 || addVec.y != 0))
			{
				currentMaxSpeed=0;
				vel += addVec;

				if (bursting)
				{
					Vector add = addVec;
					add.setLength2D(BURST_ACCEL*dt);
					vel += add;

					if (pushingOffWallEffect > 0)
						currentMaxSpeed = vars->maxWallJumpBurstSpeed;
					else
						currentMaxSpeed = vars->maxBurstSpeed;

				}
				else
				{
					if (pushingOffWallEffect > 0)
						currentMaxSpeed = vars->maxWallJumpSpeed;
					else
					{
						if (isMovingSlow)
						{
							currentMaxSpeed = vars->maxSlowSwimSpeed;
						}
						else
							currentMaxSpeed = vars->maxSwimSpeed;
					}
				}

				if (leaches > 0)
				{
					currentMaxSpeed -= leaches*60;
				}

				if (state.blind)
					currentMaxSpeed -= 100;

				if (currentMaxSpeed < 0)
					currentMaxSpeed = 1;

				if (getState() == STATE_TRANSFORM)
					rotateToVec(addVec, 0.1f, 90);
				else
				{
					if (rolling)
					{
						// here for roll key?
						// seems like this isn't reached
						//if (isActing("roll"))
						if (isActing(ACTION_ROLL, -1))
						{
							//debugLog("here");
						}
						else
						{
							float t = 0;
							if (dsq->getInputMode() == INPUT_KEYBOARD)
								t = 0.1f;
							rotateToVec(addVec, t);
						}
					}
					else if (bursting && flourish)
					{

					}
					else
					{
						if (!state.nearWall && !flourish)
							rotateToVec(addVec, 0.1f);
					}
				}

				moved = true;
				if ((!swimming || (swimming && !bursting && skeletalSprite.getCurrentAnimation()->name != "swim")) && !state.lockedToWall)
				{
					swimming = true;
					//Animation *a = skeletalSprite.getCurrentAnimation();

					if (getState() == STATE_IDLE && !rolling)
					{
						skeletalSprite.transitionAnimate("swim", ANIM_TRANSITION, -1);
					}
						//animate(anim_swim);
				}
				skeletalSprite.setTimeMultiplier(1);
				Animation *anim=skeletalSprite.getCurrentAnimation();
				if (!bursting && (anim && anim->name == "swim"))
				{
					float velLen = vel.getLength2D();
					float time = velLen / 1200.0f;
					if (velLen > 1200)
						time = 1;
					//skeletalSprite.setTimeMultiplier(time*3);// 5
					//skeletalSprite.setTimeMultiplier(time*3.5f);
					//skeletalSprite.setTimeMultiplier(time*4);
					//animator.timePeriod = 1.5f*(1.0f-time);
					skeletalSprite.setTimeMultiplier(time*4.5f);
				}
				else
				{
					if (currentAnim != getBurstAnimName() && skeletalSprite.getCurrentAnimation()->name != getBurstAnimName() && !state.lockedToWall)
					{
						if (getState() == STATE_IDLE && !rolling)
							skeletalSprite.transitionAnimate(getBurstAnimName(), ANIM_TRANSITION);
					}
				}
			}
		}

		//int currentSwimSpeed = 400;
		//if (dsq->continuity.getWorldType() == WT_SPIRIT)
		/*
		if (dsq->continuity.form == FORM_SPIRIT)
		{
			currentSwimSpeed *= 0.3f;
		}
		*/
		if (!_isUnderWater && !state.lockedToWall)
		{
			//currentSwimSpeed *= 1.5f;
			//float a = *dt;
			// base on where the mouse is
			/*
			Vector addVec;
			addVec = getVectorToCursorFromScreenCentre();
			addVec.setLength2D(a);
			*/

			// gravity
			float fallMod = 1.5;
			if (dsq->continuity.form == FORM_SPIRIT)
			{
				fallMod = 1.0;
			}
			vel += Vector(0,980)*dt*fallMod;

			if (!rolling && !state.backFlip && !flourish)
			{
				if (vel.x != 0 || vel.y != 0)
					rotateToVec(vel, 0.1f);

				if (vel.x > 0)
				{
					if (!isfh())
						flipHorizontal();
				}
				if (vel.x < 0)
				{
					if (isfh())
						flipHorizontal();
				}
			}
			if (rolling && !state.backFlip)
			{
				Vector v = getVectorToCursorFromScreenCentre();
				rotateToVec(v, 0.01f);
			}
			if (isLockable())
				lockToWall();
		}

		if (!moved)
		{
			if (swimming)
			{
				swimming = false;
				if (dsq->continuity.form == FORM_FISH)
					rotation.interpolateTo(0, 0.2f);
			}
			// "friction"
			//vel += -vel*0.999f*dt;
			if (_isUnderWater)
			{
				/*
				std::ostringstream os;
				os << "fric(" << vel.x << ", " << vel.y;
				debugLog(os.str());
				*/
				if (isInCurrent())
					doFriction(dt*5);
				else
					doFriction(dt);
			}
		}

	}

	if (_isUnderWater && isInCurrent())
	{
		if (dsq->loops.current == BBGE_AUDIO_NOCHANNEL)
		{
			PlaySfx play;
			play.name = "CurrentLoop";
			play.vol = 1;
			play.time = 1;
			play.fade = SFT_IN;
			play.loops = -1;
			dsq->loops.current = core->sound->playSfx(play);
		}
	}
	else
	{
		if (dsq->loops.current != BBGE_AUDIO_NOCHANNEL)
		{
			core->sound->fadeSfx(dsq->loops.current, SFT_OUT, 1);
			dsq->loops.current = BBGE_AUDIO_NOCHANNEL;
		}
	}

	if (!swimming && _isUnderWater)
	{
		if (!inCurrent)
			currentMaxSpeed = vars->maxSwimSpeed;

		if (!state.lockedToWall && !bursting)
		{
			if (getState() == STATE_IDLE && inputEnabled)
			{
				Animation *a = skeletalSprite.getCurrentAnimation(0);
				if (a && a->name != getIdleAnimName() && a->name != "pushed" && a->name != "spin" && !rolling)
					skeletalSprite.transitionAnimate(getIdleAnimName(), ANIM_TRANSITION, -1);
			}
		}
	}

	if (_isUnderWater && fallGravityTimer)
	{
		fallGravityTimer -= dt;
		currentMaxSpeed = lastOutOfWaterMaxSpeed;
		if (fallGravityTimer < 0)
			fallGravityTimer = 0;
	}



	clampVelocity();

	if (swimming)
	{
		if (!rolling && !internalOffset.isInterpolating())
		{
			int spread = 8;
			//int rotSpread = 45;
			float t = 1;

			internalOffset = Vector(-spread, 0);
			internalOffset.interpolateTo(Vector(spread, 0), t, -1, 1, 1);

			for (int i = 0; i < int((t*0.5f)/0.01f); i++)
			{
				internalOffset.update(0.01f);
			}
		}

		if (dsq->continuity.form != FORM_ENERGY && dsq->continuity.form != FORM_DUAL && dsq->continuity.form != FORM_FISH)
		{
			if (leaches <= 0 && !bursting && !skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->animating)
			{
				state.swimTimer += dt;

				if (state.swimTimer > 5)
				{
					state.swimTimer = 0 - rand()%3;
					static int lastSwimExtra = -1;
					int maxAnim = 4;
					int anim = rand()%maxAnim;
					if (anim == lastSwimExtra)
						anim++;
					if (anim >= maxAnim)
						anim = 0;
					lastSwimExtra = anim;
					anim ++;

					std::ostringstream os;
					os << "swimExtra-" << anim;
					skeletalSprite.transitionAnimate(os.str(), 0.5, 0, ANIMLAYER_UPPERBODYIDLE);
				}
			}
		}
	}
	else
	{

	}

	if (!swimming || rolling)
	{
		//state.swimTimer = 0;
		if (skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->animating)
		{
			skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->stopAnimation();
		}
		internalOffset.interpolateTo(Vector(0,0),0.5f);
	}

	checkNearWall();
	//if (core->getNestedMains()==1)
	{
		Vector zoomSurface(0.55f, 0.55f);
		Vector zoomMove(vars->zoomMove, vars->zoomMove), zoomStop(vars->zoomStop, vars->zoomStop), zoomNaija(vars->zoomNaija, vars->zoomNaija);
		float cheatLen = getMoveVel().getSquaredLength2D();

		if (cheatLen > sqr(250) && _isUnderWater && !state.lockedToWall)
		{
			if (!swimEmitter.isRunning())
				swimEmitter.start();
		}
		else
		{
			swimEmitter.stop();
		}

		Vector targetScale(1,1);
		if (zoomOverriden || isEntityDead() || core->globalScale.isInterpolating())
		{

		}
		else
		{
			if (game->waterLevel.x > 0 && fabsf(avatar->position.y - game->waterLevel.x) < 800)
			{
				float time = 0.5f;
				if (!myZoom.isInterpolating() || ((!core->globalScale.data || core->globalScale.data->target != zoomSurface) && myZoom.data->timePeriod != time))
				{
					myZoom.interpolateTo(zoomSurface, time, 0, 0, 1);
				}
			}
			else if (avatar->looking == 2)
			{
				float time = 1.0f;
				if (!myZoom.isInterpolating() || ((!core->globalScale.data || core->globalScale.data->target != zoomNaija) && myZoom.data->timePeriod != time))
				{
					/*
					std::ostringstream os;
					os << "zooming in on Naija: " << zoomNaija.x;
					debugLog(os.str());
					*/
					myZoom.interpolateTo(zoomNaija, time, 0, 0, 1);
				}
			}
			else if ((cheatLen > sqr(250) && cheatLen < sqr(1000)) || avatar->looking==1)
			{
				float time = 3;
				if (avatar->looking)
				{
					time = 1.0f;
				}
				if (!myZoom.isInterpolating() || ((!core->globalScale.data || core->globalScale.data->target != zoomMove) && myZoom.data->timePeriod != time))
					myZoom.interpolateTo(zoomMove, time, 0, 0, 1);
			}
			else if (cheatLen < sqr(210) && !state.lockedToWall && stillTimer.getValue() > 4 && !isSinging())
			{
				float time = 10;
				if (!myZoom.isInterpolating() || (myZoom.data->target != zoomStop && myZoom.data->timePeriod != time))
					myZoom.interpolateTo(zoomStop, time, 0, 0, 1);
			}
			else if (cheatLen >= sqr(1000))
			{
				float time = 1.6f;
				if (!myZoom.isInterpolating() || (myZoom.data->target != zoomMove && myZoom.data->timePeriod != time))
					myZoom.interpolateTo(zoomMove, time, 0, 0, 1);
			}

			core->globalScale.x = myZoom.x;
			core->globalScale.y = myZoom.y;
			core->globalScaleChanged();

		}

		if (!state.lockedToWall && !bursting && _isUnderWater && swimming && !isFollowingPath() && _collisionAvoidRange > 0)
		{
			doCollisionAvoidance(dt, _collisionAvoidRange, _collisionAvoidMod, 0, 800, OT_HURT);
		}

		if (!game->isShuttingDownGameState())
		{
			updateCurrents(dt);
			updateVel2(dt);
		}
		else
		{
			vel2 = Vector(0,0,0);
		}

		if (!state.lockedToWall && !isFollowingPath() && !riding)
		{
/*collideCheck:*/

			// Beware: This code may cause clamping vel to zero if the framerate is very high.
			// Starting with zero vel, low difftimes will cause an addVec small enough that this
			// check will always trigger, and vel will never get larger than zero.
			// Under water and swimming check should hopefully prevent this from happening. -- FG
			if (_isUnderWater && !isSwimming() && vel.getLength2D() < sqr(2))
			{
				vel = Vector(0,0,0);
			}
			Vector moveVel;
			if (!isInputEnabled() && isv(EV_NOINPUTNOVEL, 1))
			{
				vel2=vel=Vector(0,0);
			}
			else
			{
				moveVel = getMoveVel();
			}
			if (!moveVel.isZero())
			{
				bool collided = false;

				/*
				std::ostringstream os;
				os << "vel (" << vel.x << ", " << vel.y << ")";
				debugLog(os.str());
				*/
				Vector mov = (moveVel * dt);
				Vector omov = mov;
				mov.capLength2D(TILE_SIZE);
				/*
				if (mov.getSquaredLength2D() > sqr(TILE_SIZE))
					mov.setLength2D(TILE_SIZE);
				*/
				if (omov.getSquaredLength2D() > 0)
				{
					while (omov.getSquaredLength2D() > 0)
					{
						if (omov.getSquaredLength2D() < sqr(TILE_SIZE))
						{
							mov = omov;
							omov = Vector(0,0);
						}
						else
							omov -= mov;

						lastPosition = position;
						Vector newPosition = position + mov;
						//Vector testPosition = position + (vel *dt)*2;
						position = newPosition;

						if (game->collideCircleWithGrid(position, collideRadius))
						{
							if (game->lastCollideTileType == OT_HURT
								&& isDamageTarget(DT_WALLHURT))
							{
								DamageData d;
								d.damage = 1;
								d.damageType = DT_WALLHURT;
								damage(d);
								vel2 = Vector(0,0,0);
								//doCollisionAvoidance(1, 3, 1);
								/*
								Vector v = game->getWallNormal(position);
								if (!v.isZero())
								{
									vel += v * 500;
								}
								*/
							}
							collided = true;

							if (currentState == STATE_PUSH)
							{
								dsq->sound->playSfx("rockhit");
								dsq->spawnParticleEffect("rockhit", position);
								if (pushDamage)
								{
									DamageData d;
									d.damage = pushDamage;
									damage(d);
								}
								setState(STATE_IDLE);
							}

							if (!_isUnderWater)
							{
								/*
								doBounce();
								position = lastPosition;
								*/

								// this is the out of water bounce...

								//debugLog("above water bounce");



								Vector n = getWallNormal(TileVector(lastPosition));
								n *= vel.getLength2D();

								/*
								std::ostringstream os;
								os << "vel(" << vel.x << ", " << vel.y << ") n(" << n.x << ", " << n.y << ")";
								debugLog(os.str());
								*/

								//flopping = true;

								vel = -vel;

								//vel = (vel*0.5f + n*0.5f);
								if (vel.y < 0)
								{
									//debugLog("Vel less than 0");

								}
								if (vel.y == 0)
								{
									//debugLog("Vel was 0");
									vel = getWallNormal(TileVector(position));
									vel.setLength2D(500);
								}

								if (vel.isLength2DIn(500))
									vel.setLength2D(500);


								vel.capLength2D(800);

								position = lastPosition;
								this->doCollisionAvoidance(1, 4, 0.5, 0, 500);


								/*
								vel = -vel;
								if (vel.y < 0)
									vel.y *= 2;
								position = lastPosition;
								*/
							}
							else
							{
								position = lastPosition;

								// works as long as not buried in wall ... yep. =(
								if (game->isObstructed(TileVector(position)))
								{
									//vel = -vel*0.5f;
									vel = 0;
								}
								else
								{
									// && game->getPercObsInArea(position, 4) < 0.75f
									if (!inCurrent)
									{
										if (bursting)
										{
											//vel = 0;
										}
										else
										{
											if (!_isUnderWater && dsq->continuity.form == FORM_FISH)
											{
												vel = 0;
											}
											else
											{
												// this bounce is what causes naija to get wedged
												float len = vel.getLength2D();
												Vector n = game->getWallNormal(position, 5);
												if (n.x == 0 && n.y == 0)
												{
													vel = 0;
												}
												else
												{
													//n.setLength2D(len/2);
													//vel += n;
													n.setLength2D(len);
													vel = (n+vel)*0.5f;
													//vel = (n + vel)*0.5f;
												}
											}
										}
									}
								}

							}
						}


						if (!collided && checkWarpAreas()) collided = true;
						if (collided) break;

						// DO NOT COLLIDE WITH LR_ENTITIES
						// ENTITY SCRIPTS WILL HANDLE Entity V. Entity COLLISIONS

					}
				}
			}
		}
	}

	//fuuugly
	// you said it!
	if (riding || boneLock.on)
	{
		checkWarpAreas();
	}
	applyRidingPosition();
	updateHair(dt);
	chargeEmitter->position = chargingEmitter->position = position + offset;

	// particle sets
	if (leftHandEmitter && rightHandEmitter && boneLeftHand && boneRightHand)
	{
		leftHandEmitter->position = boneLeftHand->getWorldCollidePosition(Vector(0, 16));
		rightHandEmitter->position = boneRightHand->getWorldCollidePosition(Vector(0,16));
	}

	if(canCollideWithShots())
		game->handleShotCollisions(this, (activeAura == AURA_SHIELD));


	if(!core->particlesPaused && elementEffectMult > 0)
	{
		ElementUpdateList& elems = game->elementInteractionList;
		for (ElementUpdateList::iterator it = elems.begin(); it != elems.end(); ++it)
		{
			(*it)->doInteraction(this, elementEffectMult, 16);
		}
	}
}


void Avatar::checkNearWall()
{
	state.nearWall = false;

	if (!inCurrent && bursting && !state.lockedToWall && !vel.isZero() && !riding && _isUnderWater)
	{
		int checkRange = 11;
		Vector v = vel;
		v.normalize2D();
		TileVector oT(position);
		TileVector t=oT,lastT=oT;
		bool obs = false;
		for (int i = 1; i < checkRange; i++)
		{
			t.x = oT.x + v.x*i;
			t.y = oT.y + v.y*i;
			if (game->isObstructed(t, ~OT_HURT))
			{
				obs = true;
				break;
			}
			lastT = t;
		}
		if (obs)
		{
			Vector n = game->getWallNormal(t.worldVector());
			if (!n.isZero())
			{
				state.nearWall = true;
				float t=0.2f;
				rotateToVec(n, t, 0);
				skeletalSprite.transitionAnimate("wall", t);
			}
			else
				state.nearWall = false;
		}
		else
		{
			state.nearWall = false;
		}
	}
}

void Avatar::onWarp()
{
	avatar->setv(EV_NOINPUTNOVEL, 0);
	closeSingingInterface();
}

bool Avatar::checkWarpAreas()
{
	size_t i = 0;
	for (i = 0; i < game->getNumPaths(); i++)
	{
		bool warp = false;
		Path *p = game->getPath(i);
		if (p && p->active && !p->nodes.empty())
		{
			PathNode *n = &p->nodes[0];
			if (n)
			{
				Vector backPos;
				if (!p->vox.empty())
				{
					if (p->isCoordinateInside(position))
					{
						if (p->replayVox == 1)
						{
							dsq->voice(p->vox);
							p->replayVox = 2;
						}
						else
						{
							dsq->voiceOnce(p->vox);
						}
					}
				}
				if (!p->warpMap.empty() && p->pathType == PATH_WARP)
				{
					int range = 512;
					switch(p->warpType)
					{
					case 'C':
						if ((position - n->position).getSquaredLength2D() < sqr(range))
						{
							warp = true;
						}
					break;
					default:
					{
						if (p->isCoordinateInside(position))
						{
							warp = true;
							backPos = p->getBackPos(position);
						}
					}
					break;
					}
				}
				if (warp)
				{
					if (avatar->canWarp)
						game->warpToSceneFromNode(p);
					else
					{
						avatar->position = backPos;
						//avatar->vel = -avatar->vel * 0.5f;
						Vector n = p->getEnterNormal();
						n.setLength2D(avatar->vel.getLength2D());
						avatar->vel += n;
						avatar->vel *= 0.5f;
					}
					return true;
				}
				if (core->getNestedMains() == 1 && !riding)
				{
					if (p->warpMap.empty() && !p->warpNode.empty())
					{
						if (p->isCoordinateInside(position))
						{
							Path *p2 = game->getPathByName(p->warpNode);
							if (p2)
							{
								game->preLocalWarp(p->localWarpType);
								game->avatar->position = p2->getPathNode(0)->position;
								game->postLocalWarp();
								//dsq->fade(0, t);
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}
