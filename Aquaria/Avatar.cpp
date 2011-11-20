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

//#include "CommonEvents.h"

//#include <float.h>

//#define AQ_TEST_QUADTRAIL

#ifdef AQ_TEST_QUADTRAIL
	#include "QuadTrail.h"

	QuadTrail *quadTrail = 0;
#endif

Path *lastWaterBubble = 0;
bool lastJumpOutFromWaterBubble = false;

bool useSpiritDistance = true;
bool inSpiritWorld = false;

const int LAYER_FLOURISH = 3;

const float MULT_DMG_CRABCOSTUME = 0.75;
const float MULT_DMG_FISHFORM = 1.5;
const float MULT_DMG_SEAHORSEARMOR = 0.6;

const float MULT_MAXSPEED_BEASTFORM = 1.2;
const float MULT_MAXSPEED_FISHFORM = 1.5;

const float MULT_DMG_EASY	= 0.5;

const float JELLYCOSTUME_HEALTHPERC		= 0.5;
const float JELLYCOSTUME_HEALDELAY		= 2.0;
const float	JELLYCOSTUME_HEALAMOUNT		= 0.5;

const float biteTimerBiteRange = 0.6;
const float biteTimerMax = 3;
const float biteDelayPeriod = 0.08;
const int normalTendrilHits = 3;
const int rollTendrilHits = 4;
const int maxTendrilHits = 6;

const float fireDelayTime = 0.2;
const int maxShieldPoints = 8;
const int minMouse = 60;
int SongIcon::notesOpen = 0;
Avatar *avatar = 0;
const Vector BLIND_COLOR = Vector(0.1, 0.1, 0.1);
const float ANIM_TRANSITION	= 0.2;
const float MANA_RECHARGE_RATE = 1.0;
const int AURA_SHIELD_RADIUS = 64;
//const int TARGET_RANGE = 1024;
const int TARGET_RANGE = 1024; // 650
const int TARGET_GRACE_RANGE = 200;
//const int TARGET_RANGE = 700;
//const int TARGET_RANGE = 64;
const float NOTE_SCALE = 0.75;
const int singingInterfaceRadius = 100;
const int openSingingInterfaceRadius = 128;
//164
const int BURST_DISTANCE = 200;
const int STOP_DISTANCE = 48;
const int maxMouse = BURST_DISTANCE;
//const int SHOCK_RANGE	= 700;
const int SHOCK_RANGE	= 1000;
const int SPIRIT_RANGE	= 2000;

const float QUICK_SONG_CAST_DELAY = 0.4;

const float BURST_RECOVER_RATE = 1.2; // 3.0 // 0.75
const float BURST_USE_RATE = 1.5; //0.9 //1.5;
const float BURST_DELAY = 0.1;
const float BURST_ACCEL = 4000; //2000 // 1000

// Minimum time between two splash effects (seconds).
const float SPLASH_INTERVAL = 0.2;

const float TUMMY_TIME = 6.0;

const float chargeMax = 2.0;

// Axis input distance (0.0-1.0) at which we start moving.
const float JOYSTICK_LOW_THRESHOLD = 0.2;
// Axis input distance at which we move full speed.
const float JOYSTICK_HIGH_THRESHOLD = 0.6;
// Axis input distance at which we accept a note.
const float JOYSTICK_NOTE_THRESHOLD = 0.6;

// Mouse cursor distance (from note icon, in virtual pixels) below which
// we accept a note.
const float NOTE_ACCEPT_DISTANCE = 25;
// Joystick input angle offset (from note icon, in degrees) below which
// we accept a note.
const float NOTE_ACCEPT_ANGLE_OFFSET = 15;

volatile int micNote = -1;
bool openedFromMicInput = false;

const int requiredDualFormCharge = 3;

bool usingDigital = false;

Bone *bone_head = 0;
Bone *bone_dualFormGlow = 0;


bool _isUnderWater;

//HRECORD avatarRecord = 0;

#define ANIMLAYER_OVERRIDE			4
#define ANIMLAYER_UPPERBODYIDLE		6
#define ANIMLAYER_HEADOVERRIDE		7

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


	dsq->user.control.actionSet.importAction(this, "PrimaryAction", ACTION_PRIMARY);
	dsq->user.control.actionSet.importAction(this, "SecondaryAction", ACTION_SECONDARY);

	dsq->user.control.actionSet.importAction(this, "Revert", MakeFunctionEvent(Avatar, revert), 0);

	dsq->user.control.actionSet.importAction(this, "SwimUp",		ACTION_SWIMUP);
	dsq->user.control.actionSet.importAction(this, "SwimDown",		ACTION_SWIMDOWN);
	dsq->user.control.actionSet.importAction(this, "SwimLeft",		ACTION_SWIMLEFT);
	dsq->user.control.actionSet.importAction(this, "SwimRight",		ACTION_SWIMRIGHT);

	/*
	dsq->user.control.actionSet.importAction(this, "SingUp",		ACTION_SINGUP);
	dsq->user.control.actionSet.importAction(this, "SingDown",		ACTION_SINGDOWN);
	dsq->user.control.actionSet.importAction(this, "SingLeft",		ACTION_SINGLEFT);
	dsq->user.control.actionSet.importAction(this, "SingRight",		ACTION_SINGRIGHT);
	*/

	dsq->user.control.actionSet.importAction(this, "SongSlot1",		ACTION_SONGSLOT1);
	dsq->user.control.actionSet.importAction(this, "SongSlot2",		ACTION_SONGSLOT2);
	dsq->user.control.actionSet.importAction(this, "SongSlot3",		ACTION_SONGSLOT3);
	dsq->user.control.actionSet.importAction(this, "SongSlot4",		ACTION_SONGSLOT4);
	dsq->user.control.actionSet.importAction(this, "SongSlot5",		ACTION_SONGSLOT5);
	dsq->user.control.actionSet.importAction(this, "SongSlot6",		ACTION_SONGSLOT6);
	dsq->user.control.actionSet.importAction(this, "SongSlot7",		ACTION_SONGSLOT7);
	dsq->user.control.actionSet.importAction(this, "SongSlot8",		ACTION_SONGSLOT8);
	dsq->user.control.actionSet.importAction(this, "SongSlot9",		ACTION_SONGSLOT9);
	dsq->user.control.actionSet.importAction(this, "SongSlot10",	ACTION_SONGSLOT10);
	
	dsq->user.control.actionSet.importAction(this, "Look",			ACTION_LOOK);

	/*
	dsq->user.control.actionSet.importAction(this, "SongSlot5", "f5");
	dsq->user.control.actionSet.importAction(this, "SongSlot6", "f6");
	dsq->user.control.actionSet.importAction(this, "SongSlot7", "f7");
	dsq->user.control.actionSet.importAction(this, "SongSlot8", "f8");
	*/
	
	dsq->user.control.actionSet.importAction(this, "Roll",			ACTION_ROLL);

	/*
	// song note keys
	addAction("s1", KEY_1);
	addAction("s2", KEY_2);
	addAction("s3", KEY_3);
	addAction("s4", KEY_4);
	addAction("s5", KEY_5);
	addAction("s6", KEY_6);
	addAction("s7", KEY_7);
	addAction("s8", KEY_8);
	*/

}

int Avatar::getNotesOpen()
{
	return SongIcon::notesOpen;
}

// note: z is set to 1.0 when we want the aim to be used as the shot direction
// otherwise the shot will head straight to the target
Vector Avatar::getAim()
{
	Vector d;
	if (dsq->inputMode == INPUT_JOYSTICK)
	{
		if (!core->joystick.rightStick.isZero())
		{
			d = core->joystick.rightStick * 300;
			d.z = 1;
		}
		else
		{
			d = core->joystick.position * 300;
			d.z = 0;
		}
	}
	else if (dsq->inputMode == INPUT_KEYBOARD)
	{
		d = dsq->getGameCursorPosition() - position;
		d.z = 1;
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

void Avatar::postInit()
{
	// post init isn't early enough
	/*
	Entity::postInit();
	*/
}

void Avatar::onAnimationKeyPassed(int key)
{
	if (swimming && !isRolling() && !bursting && _isUnderWater)
	{
		if (key == 0 || key == 2)
		{
			//core->sound->playSfx("SwimKick", 255, 0, 1000+getMaxSpeed()/10.0f);
		}
	}
	Entity::onAnimationKeyPassed(key);
}

void Avatar::doBounce()
{
	float ba = 0.75;
	if (isRolling())
		ba = 1.0;
	float len = vel.getLength2D();
	Vector I = vel/len;
	Vector N = dsq->game->getWallNormal(position);

	if (!N.isZero())
	{
		//2*(-I dot N)*N + I
		vel = 2*(-I.dot(N))*N + I;
		vel.setLength2D(len*ba);
	}
}

Vector randCirclePos(Vector position, int radius)
{
	float a = ((rand()%360)*(2*PI))/360.0f;
	return position + Vector(sinf(a), cosf(a))*radius;
}

SongIconParticle::SongIconParticle(Vector color, Vector pos, int note)
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
	alpha.data->path.addPathNode(0.4, 0.2); // .8
	alpha.data->path.addPathNode(0.2, 0.8); // .4
	alpha.data->path.addPathNode(0, 1);
	alpha.startPath(life);

	scale.ensureData();
	scale.data->path.addPathNode(Vector(0.5,0.5), 0);
	scale.data->path.addPathNode(Vector(1,1), 0.5);
	scale.data->path.addPathNode(Vector(0.5,0.5), 1);
	scale.startPath(life);

	setLife(life);
	setDecayRate(1);

	//if (rand()%6 <= 2)
	setBlendType(RenderObject::BLEND_ADD);

	float smallestDist = HUGE_VALF;
	SongIcon *closest = 0;
	for (int i = 0; i < avatar->songIcons.size(); i++)
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

SongIcon::SongIcon(int note) : Quad(), note(note)
{
	open = false;
	alphaMod = 0.9;
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
	glow->setBlendType(RenderObject::BLEND_ADD);
	glow->scale = Vector(0.5, 0.5);
	glow->color = dsq->getNoteColor(note);
	dsq->game->addRenderObject(glow, LR_PARTICLES2);
}

void SongIcon::destroy()
{
	Quad::destroy();
}

void SongIcon::spawnParticles(float dt)
{
	float intv = 0.1;
	// do stuff!
	ptimer += dt;
	while (ptimer > intv)
	{
		ptimer -= intv;
		SongIconParticle *s = new SongIconParticle(noteColor, randCirclePos(position, 16), note);
		s->followCamera = true;
		dsq->game->addRenderObject(s, LR_HUD);
	}
}

void SongIcon::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	if (!avatar->singing)
		return;

	if (alpha.x == 0 && !alpha.isInterpolating())
		alpha.interpolateTo(0.3, 0.1);
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
		avatar->setHeadTexture("Singing", 0.1);
	}
	if (alpha.x == 1)
	{
		if ((!openedFromMicInput && isCoordinateInRadius(core->mouse.position, NOTE_ACCEPT_DISTANCE)) || micNote == note)
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
		else if (openedFromMicInput || !isCoordinateInRadius(core->mouse.position, NOTE_ACCEPT_DISTANCE*1.25f))
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
	scale.interpolateTo(Vector(1.2, 1.2), 0.1);

	if (dsq->user.video.noteEffects)
	{
		glow->scale = Vector(0.5,0.5);
		glow->scale.interpolateTo(Vector(1.0, 1.0), 2, -1, 1, 1);

		glow->alpha.interpolateTo(0.6, 0.2, 0, 0, 1);
	}

	/*
	std::ostringstream os;
	os << "Note"
	*/

	std::string sfx = dsq->game->getNoteName(note);

	open = true;

	internalOffset = Vector(-5, 0);
	internalOffset.interpolateTo(Vector(5, 0), 0.08, -1, 1);

	avatar->singNote(this->note);

	// this should never get called:
	if (channel != BBGE_AUDIO_NOCHANNEL)
	{
		dsq->sound->fadeSfx(channel, SFT_OUT, 0.2);
		//dsq->sound->fadeSfx(channel, SFT_OUT, 0.2);
		channel = BBGE_AUDIO_NOCHANNEL;
	}
	//dsq->sound->stopSfx(channel);


	PlaySfx play;
	play.name = sfx;
	play.channel = 1 + note;
	channel = dsq->sound->playSfx(play);


	rippleTimer = 0;

	minTime = 0.05;
	counter = 3.2;

	float glowLife = 0.5;

	Quad *q = new Quad("particles/glow", position);
	q->scale.interpolateTo(Vector(10, 10), glowLife+0.1f);
	q->alpha.ensureData();
	q->alpha.data->path.addPathNode(0,0);
	q->alpha.data->path.addPathNode(0.75,0.2);
	q->alpha.data->path.addPathNode(0,1);
	q->alpha.startPath(glowLife);
	q->color = dsq->getNoteColor(note); //*0.5f + Vector(0.5, 0.5, 0.5)
	q->setBlendType(RenderObject::BLEND_ADD);
	q->followCamera = 1;
	dsq->game->addRenderObject(q, LR_HUD);

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
	q->alpha.data->path.addPathNode(0.5,0.2);
	q->alpha.data->path.addPathNode(0,1);
	q->alpha.startPath(glowLife);
	//q->setBlendType(RenderObject::BLEND_ADD);
	q->followCamera = 1;
	dsq->game->addRenderObject(q, LR_HUD);
	}

	avatar->songInterfaceTimer = 1.0;

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
			if ((e->position - dsq->game->avatar->position).getSquaredLength2D() < sqr(1000))
			{
				e->songNote(note);
			}
		}
		for (int i = 0; i < dsq->game->getNumPaths(); i++)
		{
			Path *p = dsq->game->getPath(i);
			if (!p->nodes.empty())
			{
				if ((p->nodes[0].position - dsq->game->avatar->position).getSquaredLength2D() < sqr(1000))
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
	scale.interpolateTo(Vector(NOTE_SCALE, NOTE_SCALE), 0.1);

	if (dsq->game->avatar->isSinging() && dsq->user.video.noteEffects)
		glow->alpha.interpolateTo(0.3, 1.5, 0, 0, 1);
	else
		glow->alpha.interpolateTo(0, 1.5, 0, 0, 1);
	glow->scale.interpolateTo(Vector(0.5, 0.5), 0.5);


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
			int dist = (e->position - dsq->game->avatar->position).getSquaredLength2D();
			if (e != dsq->game->avatar && dist < sqr(1000))
			{
				e->songNoteDone(note, len);
			}
		}
		for (int i = 0; i < dsq->game->getNumPaths(); i++)
		{
			Path *p = dsq->game->getPath(i);
			if (!p->nodes.empty())
			{
				if ((p->nodes[0].position - dsq->game->avatar->position).getSquaredLength2D() < sqr(1000))
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
	alpha.interpolateTo(1, 0.1);
}

void SongIcon::closeInterface()
{
	closeNote();
	delay = 0;
	alpha.interpolateTo(0, 0.1);
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
	crawlingOnWall = false;
	shotDelay = 0;
	spellCharge = 0;
	leachTimer = 0;
	swimTimer = 0;
	rollTimer = 0;
	updateLookAtTime = 0;
	lookAtEntity = 0;
	blinkTimer = 0;
}

//    0  1  2  3  4  5  6  7  8  9
const int	spellManaCost	[]	= { 0,    3,    1,    1,   2,    1,    4,    1,    1,    2};
const float spellChargeMins	[]	= { 0,    1.25, 0,    0,   0.75, 0,    0.5,  0,    0,    3};
//const float spellChargeMaxs	[]	= { 0.75, 1.5,  1.9,  1,   1,    1,    0.75, 1,    1,    3};
const float spellCastDelays	[]	= { 0.05, 0.2,  0.4, 0.1, 0.1,  0.1,  0.2,  0.1,  0.1,  0.1 };

bool avatarDebugEnabled = false;

void Avatar::toggleMovement(bool on)
{
	canMove = on;
}

bool Avatar::isLockable()
{
	return (bursting || !_isUnderWater) && (boneLockDelay == 0) && (dsq->continuity.form != FORM_FISH);
}

bool Avatar::isSinging()
{
	return singing;
}

void Avatar::shift()
{
	dsq->continuity.shiftWorlds();
}

void Avatar::applyWorldEffects(WorldType type)
{
	static bool oldfh=false;

	if (type == WT_SPIRIT)
	{
		//skeletalSprite.transitionAnimate("ball", 0.1, -1);
		//skeletalSprite.alpha.interpolateTo(0, 1);
		//skeletalSprite.alpha = 0;
		//dsq->game->addRenderObject(&skeletalSprite, LR_ENTITIES);

		removeChild(&skeletalSprite);
		skeletalSprite.position = position;
		skeletalSprite.setFreeze(true);
		skeletalSprite.scale = scale;
		skeletalSprite.alpha.interpolateTo(0.5, 1);
		//dsq->game->addRenderObject(&skeletalSprite, LR_ENTITIES);
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
		//dsq->game->removeRenderObject(&skeletalSprite);

		skeletalSprite.setFreeze(false);
		if (!skeletalSprite.getParent())
		{
			addChild(&skeletalSprite, PM_STATIC);
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
	skeletalSprite.transitionAnimate(anim, 0.1, 0, LAYER_FLOURISH);
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

	rotationOffset.interpolateTo(Vector(0,0,rotz), 0.8, 0, 0, 1);
}

void Avatar::onIdle()
{
	if (dsq->game->li)
	{
		if (dsq->game->li->getState() == STATE_HUG && riding)
		{
			dsq->game->li->setState(STATE_IDLE);
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

void Avatar::debugMsg(const std::string &msg)
{
	if (avatarDebugEnabled)
	{
		BitmapText *txt = new BitmapText(&dsq->font);
		txt->setLife(2);
		txt->setDecayRate(1);
		txt->position = this->position;
		txt->setText(msg);
		txt->fadeAlphaWithLife = true;
		core->getTopStateData()->addRenderObject(txt, LR_DEBUG_TEXT);
	}
}

void Avatar::onBlindTest()
{
	setBlind(5);
}

void Avatar::onToggleDebugMessages()
{
	avatarDebugEnabled = !avatarDebugEnabled;
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
		float frc = 0.333333;
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
	if (health <= damageThreshold)
	{
		//dsq->game->damageSprite->alpha.interpolateTo(0.9, 0.5);
		float a = ((damageThreshold - health)/float(damageThreshold))*1.0f;
		dsq->game->damageSprite->alpha.interpolateTo(a, 0.3);

		/*
		std::ostringstream os;
		os << "damageSprite alpha: " << a;
		debugLog(os.str());
		*/

		dsq->game->damageSprite->scale = Vector(1,1);
		dsq->game->damageSprite->scale.interpolateTo(Vector(1.2, 1.2), 0.5, -1, 1);

		/*
		if (health <= 0)
		{
			dsq->game->sceneColor.interpolateTo(Vector(1,0.5,0.5), 0.75);
		}
		*/
	}
	else
	{
		dsq->game->damageSprite->alpha.interpolateTo(0, 0.3);
	}
}

void Avatar::checkUpgradeForShot(Shot *s)
{
	if (dsq->continuity.energyMult <= 1)
		s->extraDamage = dsq->continuity.energyMult * 1;
	else
		s->extraDamage = dsq->continuity.energyMult * 0.75f;

	if (s->extraDamage > 0)
	{
		Quad *glow = new Quad("particles/glow", Vector(0,0));
		glow->color = Vector(1,0,0);
		glow->color.interpolateTo(Vector(1,0.5,0.5), 0.1, -1, 1);
		glow->setBlendType(BLEND_ADD);
		glow->scale = Vector(4, 4) + (s->extraDamage*Vector(2,2));
		glow->scale.interpolateTo(Vector(16,16)+ (s->extraDamage*Vector(2,2)), 0.5, -1, 1);
		s->addChild(glow, PM_POINTER);
	}
}

void Avatar::onDamage(DamageData &d)
{
	Entity::onDamage(d);


	if (dsq->difficulty == DSQ::DIFF_EASY)
	{
		if (d.damage > 0)
			d.damage *= MULT_DMG_EASY;
	}

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

	if ((core->isNested() && dsq->game->invincibleOnNested) || dsq->game->invinciblity)
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

	if ((!invincible || !dsq->game->invincibleOnNested) && !(invincibleBreak && damageTimer.isActive() && d.useTimer) && !dsq->continuity.invincibleTimer.isActive())
	{
		if (d.damageType == DT_ENEMY_ACTIVEPOISON)
			core->sound->playSfx("Poison");
		else
			core->sound->playSfx("Pain");
			

		setHeadTexture("Pain", 1);

		int r = (rand()%2)+1;
		std::ostringstream os;
		os << "basicHit" << r;
		skeletalSprite.transitionAnimate(os.str(), 0.05, 0, ANIMLAYER_OVERRIDE);

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
			float t = 0.5;
			if (healthWillBe<=0)
				t = 2;

			dsq->rumble(d.damage, d.damage, 0.4);
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
						dsq->overlayRed->alpha.data->path.addPathNode(1.0, 0.1);
						dsq->overlayRed->alpha.data->path.addPathNode(0, 1.0);
						dsq->overlayRed->alpha.startPath(1);

						dsq->sound->playSfx("heartbeat");

						if (healthWillBe < 2 && healthWillBe >= 1 && !dsq->game->hasPlayedLow)
						{
							dsq->emote.playSfx(EMOTE_NAIJALOW);
							dsq->game->hasPlayedLow = 1;
						}
						

						dsq->gameSpeed.ensureData();
						dsq->gameSpeed.data->path.clear();
						dsq->gameSpeed.data->path.addPathNode(1, 0);
						dsq->gameSpeed.data->path.addPathNode(0.25, 0.1);
						dsq->gameSpeed.data->path.addPathNode(0.25, 0.4);
						dsq->gameSpeed.data->path.addPathNode(1.0, 1.0);

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

const int beatHealth = 3;
void Avatar::updateHeartbeatSfx(float t)
{
	/*
	if (heartbeat)
	{
		BASS_CHANNELINFO info;
		BASS_ChannelGetInfo(heartbeat, &info);
		int num = (beatHealth - health);
		float wantFreq = 1000 + num*300;
		float useFreq = ((wantFreq*info.freq)/1000.0f);
		float vol = 75 + (num*25)*0.5f;
		vol *= (core->sound->getUseSfxVol()/100.0f);
		//int vol = 100;
		BASS_ChannelSlideAttributes(heartbeat, useFreq, vol, -101, 1000.0f*t);
	}
	*/
}

void Avatar::onHealthChange(float change)
{
	updateDamageVisualEffects();

	if (health <= beatHealth && health > 0)
	{
		/*
		if (!heartbeat)
		{
			//debugLog("starting heartbeat");
			heartbeat = core->sound->playSfx("Heartbeat", 255, 0, 1000, 1);
			//core->sound->playSfx("Heartbeat");
		}
		*/
		updateHeartbeatSfx(0.5);
	}
	if (health > beatHealth)
	{
		/*
		if (heartbeat)
		{
			//debugLog("stopping heartbeat");
			BASS_CHANNELINFO info;
			BASS_ChannelGetInfo(heartbeat, &info);
			BASS_ChannelSlideAttributes(heartbeat, info.freq, -2, -101, 1000*2);
			heartbeat = 0;
		}
		*/
	}
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
	for (int i = 0; i < targets.size(); i++)
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
	dsq->game->toggleMiniMapRender(1);

	if (!dsq->game->isApplyingState())
		dsq->toggleCursor(true);

	if (movingOn)
	{
		dsq->setMousePosition(Vector(400,300));
	}

	if (dsq->continuity.form == FORM_ENERGY)
	{
		for (int i = 0; i < targetQuads.size(); i++)
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
	dsq->game->toggleMiniMapRender(0);
	dsq->toggleCursor(false);
	endCharge();
	clearTargets();
	if (movingOn)
	{
		dsq->setMousePosition(Vector(400,300));
	}

	for (int i = 0; i < targetQuads.size(); i++)
	{
		targetQuads[i]->stop();
	}

	setInvincible(true);
}

void Avatar::clearTargets()
{
	for (int i = 0; i < targets.size(); i++)
	{
		if (targets[i].e)
		{
			lostTarget(i, 0);
		}
		targets[i].e = 0;
	}
}

void Avatar::slowToRest()
{
	vel.capLength2D(50);
	/*
	if (vel.getSquaredLength2D() > sqr(50))
	{
		vel.setLength2D(50);
	}
	*/
	bursting = swimming = false;
	skeletalSprite.stopAnimation(1);
	rotation.interpolateTo(Vector(0,0,0), 0.2, 0, 0, 1);
}

/*
#define SPECWIDTH 368
#define SPECHEIGHT 127
BYTE *specbuf = 0;
*/

volatile int curMicNote = -1, lastMicNote=-1;

#ifdef BBGE_BUILD_RECORD
	volatile float timeFromLastNote=0;
	volatile float timerFreq;
	volatile float lastLargest=0;
	volatile bool inMe = false;
	volatile __int64 lastTick = 0;
#endif

	/*
BOOL CALLBACK recordCallback(HRECORD handle, const void *buf, DWORD len, DWORD user)
{

#ifdef BBGE_BUILD_RECORD
	if (inMe) return TRUE;
	inMe = true;

	//if (!dsq->game->avatar->isSinging()) return FALSE;
	//int x,y;

	//timerFreq =
	__int64 freq=0;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	__int64 curTime=0;
	QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
	if (lastTick == 0)
	{
		lastTick = curTime;
	}

	float fft[4096]; // get the FFT data
	BASS_ChannelGetData(handle,fft,BASS_DATA_FFT4096);

	int v = 0;
	int c = 0;
	float largest = -1;

	//128
	//for (int i = 5; i < 13; i++)
	for (int i = 12; i < 64; i++)
	{
		if (fft[i] > 0.01f && (fft[i] > largest || largest == -1))
		{
			largest = fft[i];
			v = i;
		}
	}

	int v2=0;
	largest = -1;
	// find the next largest

	float dt = (float(curTime-lastTick)/float(freq));
	lastTick = curTime;
	int posMicNote=-1;
	if (c != 0)
		v /= float(c);
	int ov = v;
	float factor=1.0;
	int octave = dsq->user.audio.octave;

	int minNote = 12;///6;
	int maxNote = minNote + 25; // 8
	int octRange = 11;

	minNote += (octRange)*octave;
	maxNote += (octRange)*octave;

	posMicNote = dsq->fftnotes.getNoteFromFFT(v, octave);

	//if (lastLargest<largest || fabsf(largest-lastLargest) < 0.05f)
	if (true)
	{
		lastLargest = largest;

		float closeRange = 0.05;
		// check for our e natural

		if (posMicNote != -1)
		{

			curMicNote = posMicNote;
		}
		else
		{
			curMicNote = -1;
		}
	}
	else
	{
		lastLargest -= dt;
	}


	timeFromLastNote += dt;
	if (curMicNote != lastMicNote)
		timeFromLastNote = 0;
	if (timeFromLastNote > 0.0001f)
	{
		micNote = curMicNote;
		timeFromLastNote = 0;
	}
	if (curMicNote == -1)
		micNote = -1;
	lastMicNote = curMicNote;

	inMe = false;
#endif
	return TRUE;
}
*/

/*

class FoodIcon2 : public Quad
{
};

class FoodIcon : public Quad
{
public:
	FoodIcon(IngredientEffectType iet);

	IngredientEffectType type;

	void 
};

void Avatar::openFoodInterface()
{
	if (!singing && !pickingPullTarget && health > 0 && !isEntityDead() && !blockSinging)
	{
		// build it
		foodIcons.clear();

		foodIcons.resize(8);
	}
}
*/

void Avatar::openSingingInterface()
{
	if (!singing && !pickingPullTarget && health > 0 && !isEntityDead() && !blockSinging)
	{
		//core->mouse.position = Vector(400,300);
		if (dsq->inputMode != INPUT_MOUSE)
		{
			core->centerMouse();
			//core->setMousePosition(Vector(400,300));
		}

		core->setMouseConstraintCircle(singingInterfaceRadius);
		stopRoll();
		singing = true;
		currentSongIdx = SONG_NONE;

		// make the singing icons appear
		for (int i = 0; i < songIcons.size(); i++)
		{
			songIcons[i]->openInterface();
		}
		currentSong.notes.clear();

		//
		songInterfaceTimer = 0;

		dsq->game->songLineRender->clear();

		//if (avatarRecord)
		//{
		//	if (dsq->useMic && !dsq->autoSingMenuOpen && dsq->user.audio.micOn)
		//	{
		//		//avatarRecord=BASS_RecordStart(44100,1,0,&recordCallback,0);
		//		BASS_ChannelPlay(avatarRecord, false);
		//	}
		//}

		if (dsq->inputMode == INPUT_JOYSTICK)
		{
			core->setMousePosition(core->center);
		}
	}
}

void Avatar::closeSingingInterface()
{

	if (dsq->game->songLineRender)
		dsq->game->songLineRender->clear();
	if (singing)
	{
		core->setMouseConstraint(false);
		usingDigital = false;
		quickSongCastDelay = 1;

		// HACK: this prevents being "locked" away from the seahorse... so naija can
		// be in singing range of the seahorse
		applyRidingPosition();
		singing = false;

		for (int i = 0; i < songIcons.size(); i++)
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

		/*
		if (avatarRecord)
		{
			if (dsq->useMic && !dsq->autoSingMenuOpen && dsq->user.audio.micOn)
			{
				//BASS_ChannelStop(avatarRecord);
				BASS_ChannelPause(avatarRecord);
				//BASS_RecordFree();
				//avatarRecord = 0;

			}
		}
		*/

		lastMicNote = curMicNote = micNote = -1;
	}
}

void Avatar::openFormInterface()
{
	if (!inFormInterface)
	{
		inFormInterface = true;

		for(int i = 0; i < formIcons.size(); i++)
		{
			formIcons[i]->alpha.interpolateTo(1, 0.1);
		}
	}
}

void Avatar::closeFormInterface()
{
	if (inFormInterface)
	{
		inFormInterface = false;

		for (int i = 0; i < formIcons.size(); i++)
		{
			formIcons[i]->alpha.interpolateTo(0, 0.1);
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
		bone_dualFormGlow->scale.interpolateTo(Vector(perc, perc), 0.2);
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
		dsq->game->clearControlHint();
	*/

	if (lastForm == FORM_NONE)
		lastForm = dsq->continuity.form;

	endCharge();

	std::ostringstream os2;
	os2 << "lastForm: " << lastForm;
	debugLog(os2.str());

	for (int i = 0; i < targetQuads.size(); i++)
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
			Vector n = dsq->game->getWallNormal(position);
			if (!n.isZero())
			{
				n *= 400;
				vel += n;
			}

			return;
		}
		//rotationOffset.interpolateTo(Vector(0,0,0), 0.5);
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
	break;
	case FORM_BEAST:
		//dsq->game->sceneColor3.interpolateTo(Vector(1, 1, 1), 0.5);
	break;
	case FORM_DUAL:
		if (dsq->continuity.hasLi())
		{
			dsq->game->li->alpha = 1;
			dsq->game->li->position = position;
			dsq->game->li->setState(STATE_IDLE);
		}
	break;
	default:
		if (leftHandEmitter && rightHandEmitter)
		{
			leftHandEmitter->stop();
			rightHandEmitter->stop();
		}
	break;
	}


	state.abilityDelay = 0;
	formAbilityDelay = 0;
	dsq->continuity.form = form;
	ropeState = 0;
	formTimer = 0;
	if (effects)
	{
		if (core->afterEffectManager)
			core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08,0.05,22,0.2f, 1.2));

		switch(form)
		{
		case FORM_ENERGY:
			core->sound->playSfx("EnergyForm");
			/*
			dsq->game->tintColor.path.addPathNode(Vector(1,1,1),0);
			dsq->game->tintColor.path.addPathNode(Vector(1.5,1.5,4),0.25);
			dsq->game->tintColor.path.addPathNode(Vector(4,1.5,1),0.5);
			dsq->game->tintColor.path.addPathNode(Vector(1,1,1),0.5);
			dsq->game->tintColor.startPath(2);
			*/

			/*
			dsq->game->tintColor = Vector(1,1,3);
			dsq->game->tintColor.interpolateTo(Vector(1,1,1), 1);
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
		dsq->game->sceneColor3.interpolateTo(Vector(1,1,1), 0.2);
	}
	*/
	float lastHairAlphaMod = 0;
	if (hair)
	{
		hair->alphaMod = 0;
		lastHairAlphaMod = hair->alphaMod;
	}
	switch (form)
	{
	case FORM_ENERGY:
		refreshModel("Naija", "EnergyForm");
		for (int i = 0; i < targetQuads.size(); i++)
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
	}
	break;
	case FORM_SUN:
	{
		refreshModel("Naija", "SunForm");
		lightFormGlow->moveToFront();
		lightFormGlow->alpha.interpolateTo(0.75, 1);
		lightFormGlowCone->alpha.interpolateTo(0.4, 1);

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
		/*
		skeletalSprite.loadSkin("ChildTeira");
		refreshModel();
		*/
		/*
		if (dsq->game->sceneNatureForm == "forest")
		{
			debugLog("Forest Form");
			dsq->continuity.form = FORM_NATURE_FOREST;
		}
		else if (dsq->game->sceneNatureForm == "sun")
		{
			dsq->continuity.form = FORM_NATURE_SUN;
			debugLog("Sun Form");
		}
		else if (dsq->game->sceneNatureForm == "fire")
		{
			dsq->continuity.form = FORM_NATURE_FIRE;
			debugLog("Fire Form");
		}
		else if (dsq->game->sceneNatureForm == "dark")
		{
			dsq->continuity.form = FORM_NATURE_DARK;
			debugLog("Dark Form");
		}
		else if (dsq->game->sceneNatureForm == "rock" || dsq->game->sceneNatureForm.empty())
		{
			dsq->continuity.form = FORM_NATURE_ROCK;
			debugLog("Rock Form");
		}
		*/

	break;
	case FORM_BEAST:
	{
		refreshModel("Naija", "BeastForm");
	}
	break;
	case FORM_SPIRIT:
		bodyPosition = position;
		bodyOffset = offset;
		fallOffWall();
		dsq->continuity.shiftWorlds();

		if (onInit)
		{
			skeletalSprite.alphaMod = 0;
			canChangeForm = false;
			useSpiritDistance = false;
			inSpiritWorld = true;
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
			dsq->game->li->setState(STATE_WAIT);
			dsq->game->li->alpha = 0;
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

int Avatar::getLastNote()
{
	return lastNote;
}

void Avatar::singNote(int note)
{
	currentSong.notes.push_back(note);
	lastNote = note;
	//int song = dsq->continuity.checkSong(currentSong);
	//int song = dsq->continuity.checkSongAssisted(currentSong);
	//int song = dsq->continuity.checkSongAssisted(currentSong);
	/*
	std::ostringstream os;
	os << "sung note: " << note;
	debugLog(os.str());
	*/
	//currentSongIdx = song;
	/*
	if (song != -1)
	{
	*/
		/*
		std::ostringstream os;
		os << "Sung Song: " << song;
		debugLog(os.str());
		*/
		// close in a few seconds
		//closeSingingInterface();
	//}
}

void Avatar::updateSingingInterface(float dt)
{

	//if (singing)
	if (songIcons.size()>0 && songIcons[0]->alpha.x > 0)
	{
		if (dsq->inputMode != INPUT_JOYSTICK && !core->mouse.change.isZero())
		{
			if (dsq->game->songLineRender && songIcons[0]->alpha.x == 1)
			{
				float smallestDist = HUGE_VALF;
				int closest = -1;
				for (int i = 0; i < songIcons.size(); i++)
				{
					float dist = (songIcons[i]->position - core->mouse.position).getSquaredLength2D();
					if (dist < smallestDist)
					{
						smallestDist = dist;
						closest = i;
					}
				}

				dsq->game->songLineRender->newPoint(core->mouse.position, songIcons[closest]->noteColor);
			}
		}

		if (health <= 0 || isEntityDead())
		{
			closeSingingInterface();
		}
		else
		{
			if (dsq->inputMode == INPUT_JOYSTICK)
			{
				Vector d = dsq->joystick.position;

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
						for (int i = 0; i < songIcons.size(); i++)
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
	for (int i = 0; i < songIcons.size(); i++)
	{
		songIcons[i]->position = Vector(400,300)+/*this->position + */Vector(sinf(rad)*singingInterfaceRadius, cosf(rad)*singingInterfaceRadius);
		rad += radIncr;
	}
}

const int chkDist = 2500*2500;

Target Avatar::getNearestTarget(const Vector &checkPos, const Vector &distPos, Entity *source, DamageType dt, bool override, std::vector<Target> *ignore, /*FIXME:unused*/ EntityList *entityList)
{
	BBGE_PROF(Avatar_getNearestTarget);
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
		if (e != this && e->targetPriority >= highestPriority && this->pullTarget != e && e->isDamageTarget(dt) && dsq->game->isValidTarget(e, this))
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
							int j = 0;
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
								int j = 0;
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
	for (int i = 0; i < targets.size(); i++)
	{
		if (!targets[i].e
		|| !targets[i].e->isPresent()
		|| targets[i].e->getState() == STATE_DEATHSCENE
		|| !dsq->game->isValidTarget(targets[i].e, this))
		{
			targets.clear();
			break;
		}
	}
	if ((dsq->inputMode == INPUT_MOUSE || dsq->inputMode == INPUT_KEYBOARD) && !(wasDown && core->mouse.buttons.right))
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
				for (int i = 0; i < oldTargets.size(); i++)
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
		for (int i = 0; i < targets.size(); i++)
		{
			Entity *e = targets[i].e;
			if (e)
			{
				if (!(position - e->position).isLength2DIn(e->getTargetRange() + TARGET_RANGE + TARGET_GRACE_RANGE) || !dsq->game->isValidTarget(e, this) || !e->isDamageTarget(damageType))
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

void Avatar::loseTargets()
{
	for (int i = 0; i < targets.size(); i++)
	{
		Entity *e = targets[i].e;
		if (e)
		{
			lostTarget(i, targets[i].e);
			targets[i].e = 0;
			targetUpdateDelay = maxTargetDelay;
		}
	}
}

void Avatar::updateTargetQuads(float dt)
{

	particleManager->setSuckPosition(1, dsq->getGameCursorPosition());

	/*
	for (int i = 0; i < targetQuads.size(); i++)
	{
		
	}
	*/

	static Entity *lastTargetE = 0;
	const float tt = 0.02;
	for (int i = 0; i < targets.size(); i++)
	{
		if (targets[i].e)
		{

			targetQuads[i]->alpha.interpolateTo(1, 0.1);
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
				particleManager->setSuckPosition(1, targets[i].pos);
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
			targetQuads[i]->position = dsq->getGameCursorPosition();
			//targetQuads[i]->alpha.interpolateTo(0, 0.1);
		}
	}

	if (targets.empty())
	{
		for (int i = 0; i < targetQuads.size(); i++)
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

			targetQuads[i]->position = dsq->getGameCursorPosition();
			if (dsq->continuity.form == FORM_ENERGY && isInputEnabled())
			{
				if (dsq->inputMode == INPUT_JOYSTICK && targetQuads[i]->isRunning())
				{
					targetQuads[i]->stop();
				}
				else if (dsq->inputMode != INPUT_JOYSTICK && !targetQuads[i]->isRunning())
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
	p = boneLeftArm->getWorldPosition();
	//&& !dsq->game->isObstructed(TileVector(position))
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
	if ((dsq->inputMode == INPUT_JOYSTICK && !aimAt) || dsq->user.control.autoAim)
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
		for (int i = 0; i < targets.size(); i++)
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

			s = dsq->game->fireShot(shot, this, targets[i].e);
			s->setAimVector(dir);
			s->setTargetPoint(targets[i].targetPt);

			/*
			if (dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY2))
			{
				s = dsq->game->fireShot("EnergyBlast2", this, targets[i].e);
				s->setAimVector(dir);
				s->setTargetPoint(targets[i].targetPt);
			}
			else
			{
				s = dsq->game->fireShot("EnergyBlast", this, targets[i].e);
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
			s = dsq->game->fireShot(shot, this);

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
				s = dsq->game->fireShot("EnergyBlast2", this);
				s->setAimVector(dir);
			}
			else
			{
				s = dsq->game->fireShot("EnergyBlast", this);
				s->setAimVector(dir);
			}
			*/
		}
	}

	if (s)
	{
		checkUpgradeForShot(s);

		

		skeletalSprite.transitionAnimate("fireBlast", 0.1, 0, 5);
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

void Avatar::spawnSeed()
{
	// max spore children/seeds = 50
	if (dsq->game->getNumberOfEntitiesNamed("SporeChild") < 4)
	{
		if (!dsq->game->isObstructed(TileVector(position)))
		{
			dsq->game->createEntity("SporeChild", 0, position, 0, 0, "");
		}
	}
	else
	{
		// visual effect and/or sound effect
	}
}

Vector Avatar::getFacing()
{
	if (vel.isLength2DIn(2) && rotation.z == 0)
	{
		if (isfh())
			return Vector(1,0);
		else
			return Vector(-1,0);
	}
	return getForward();
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

void Avatar::formAbility(int ability)
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
									core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08,0.05,22,0.2f, 1.2));

								dsq->continuity.dualFormCharge = 0;
								dsq->shakeCamera(25, 2);

								core->globalScale = Vector(0.4, 0.4);
								myZoom = Vector(0.4, 0.4);

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
								Shot *s = dsq->game->fireShot("DualForm", this, 0, position, 0);
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
			/*
			else if (ability == 1)
			{

			}
			*/
		}
	break;
	case FORM_ENERGY:
		{
			if (ability == 0)
			{
				if (chargeLevelAttained == 2)
				{
					/*
					shockTimer = 1;
					damageDelay = 0.2;
					didShockDamage = false;
					*/
					didShockDamage = false;
					if (dsq->continuity.hasFormUpgrade(FORMUPGRADE_ENERGY2))
						doShock("EnergyTendril2");
					else
						doShock("EnergyTendril");
					if (!state.lockedToWall)
						skeletalSprite.animate("energyChargeAttack", 0, 6);

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
		}
		break;
	case FORM_NATURE:
		// no abilities
		{
			//debugLog("rock ability");
			if (ability == 0)
			{
				if (formAbilityDelay == 0)
				{
					formAbilityDelay = 0.2;
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

					dsq->game->fireShot(seedName, this, 0, pos, getAim());


					/*
					Vector pos = getAim();
					if (!pos.isZero())
						pos.setLength2D(64);
					pos += position;


					//dsq->spawnParticleEffect("Fertilizer", pos);

					Entity *e = 0;
					std::string seedName;
					if (chargeLevelAttained == 0)
						seedName = "SeedFlower";
					else if (chargeLevelAttained == 2)
						seedName = "SeedUberVine";

					e = dsq->game->createEntity(seedName, 0, pos, 0, false, "");

					Vector add = pos - position;
					add.setLength2D(800);
					e->vel += add;
					*/

					/*
					if (chargeLevelAttained == 0)
					{
					}
					else if (chargeLevelAttained == 1)
					{
						e->setState(STATE_CHARGE1);
					}
					else if (chargeLevelAttained == 2)
					{
						e->setState(STATE_CHARGE2);
					}

					e->update(0);
					*/
					// idle = charge 0
					// attack = charge1
					// something = charge2
					/*
					FOR_ENTITIES (i)
					{
						Entity *e = *i;
						if (e && e->getEntityType() == ET_ENEMY && e->isDamageTarget(DT_AVATAR_NATURE))
						{
							if ((e->position - pos).isLength2DIn(128))
							{
								DamageData d;
								d.damageType = DT_AVATAR_NATURE;
								d.damage = 1;
								d.attacker = this;
								e->damage(d);
							}
						}
					}
					*/
				}
			}
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
						Shot *s = dsq->game->fireShot(d->shot, this, 0, Vector(0,0,0), Vector(0,0,0), playSfx);
						if (s->shotData && s->shotData->damage > 0)
						{
							s->extraDamage = 1;
						}

						Entity *target = 0;
						if (s->shotData->homing > 0)
						{
							Vector p = dsq->getGameCursorPosition();
							target = dsq->game->getNearestEntity(p, 800, this, ET_ENEMY, s->shotData->damageType);
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
								effect = 0.4;
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
			q->setDecayRate(0.05);
			q->fadeAlphaWithLife = 1;
			q->scale = Vector(0,0);
			q->scale.interpolateTo(Vector(2,2), 0.1);
			dsq->game->addRenderObject(q, LR_ELEMENTS13);
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
				if (s->shotData && !s->shotData->invisible)
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
				dsq->game->spawnManaBall(position, 1);
				spiritEnergyAbsorbed = 0;
			}
			spiritBeaconEmitter.start();
			formAbilityDelay = 1.0;

			Path *p = dsq->game->getNearestPath(position, "SPIRITBEACON");
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
				Path *p = dsq->game->getNearestPath(position, PATH_SPIRITPORTAL);
				if (p && p->isCoordinateInside(position))
				{
					if (inSpiritWorld)
						changeForm(FORM_NORMAL);
					dsq->game->warpToSceneFromNode(p);
				}
			}
		}
	break;
	case FORM_FISH:
	{

	}
	break;
	}
}

Vector Avatar::getTendrilAimVector(int i, int max)
{
	float a = float(float(i)/float(max))*PI*2;
	Vector aim(sinf(a), cosf(a));
	if (state.lockedToWall)
	{
		Vector n = dsq->game->getWallNormal(position);
		if (!n.isZero())
		{
			aim = aim*0.4f + n*0.6f;
		}
	}
	return aim;
}

int Avatar::getNumShots()
{
	int thits = normalTendrilHits;
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

	

	int c = 0;
	//int maxHit = 2 + dsq->continuity.getSpellLevel(SPELL_SHOCK)*2;
	//int maxHit = 4;
	std::vector <Entity*> entitiesToHit;
	std::vector <Target> localTargets;
	bool clearTargets = true;

	int thits = getNumShots();

	/*
	if (skeletalSprite.getAnimationLayer(LAYER_FLOURISH)->getCurrentAnimation())
	{
		thits = maxTendrilHits;
	}
	*/

	if (!targets.empty() && targets[0].e != 0)
	{
		clearTargets = false;
		for (int i = 0; i < thits; i++)
		{
			entitiesToHit.push_back(targets[0].e);
		}
	}
	else
	{
		//std::vector <Target> localTargets;

		localTargets.clear();

		int range = 800;
		EntityList entityList;
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e != this && e->isPresent() && e->isDamageTarget(DT_AVATAR_SHOCK) && (e->position - position).isLength2DIn(range))
			{
				entityList.push_back(e);
			}
		}

		while (c < thits)
		{
			Target t = getNearestTarget(position, position, this, DT_AVATAR_SHOCK, true, &localTargets, &entityList);
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
				for (int i = 0; i < localTargets.size(); i++)
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
		for (int i = 0; i < thits; i++)
		{
			Shot *s = dsq->game->fireShot(shotName, this, 0);

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
				Shot *s = dsq->game->fireShot(shotName, this, e);
				if (!targets.empty())
				{
					for (int j = 0; j < targets.size(); j++)
					{
						if (targets[j].e == e)
							s->targetPt = targets[j].targetPt;
					}
				}
				/*
				else if (!localTargets.empty())
				{
					for (int j = 0; j < localTargets.size(); j++)
					{
						if (localTargets[j].e == e)
							s->targetPt = localTargets[j].targetPt;
					}
				}
				*/
				Vector d = e->position - position;
				/*
				float a = float(float(i)/float(sz))*PI*2;
				Vector aim(sinf(a), cosf(a));

				swizzleTendrilAimVector(aim);
				*/
				s->setAimVector(getTendrilAimVector(i, thits));
				checkUpgradeForShot(s);
				/*
				float ang = 0;
				MathFunctions::calculateAngleBetweenVectorsInRadians(Vector(0,-1), d, ang);
				float a = i-(sz/2);
				Vector adjust = a*spread + ang;
				d.normalize2D();
				s->setAimVector((d + adjust)/2);
				*/
			}
		}
	}

	if (clearTargets)
	{
		targets.clear();
	}


	/*
	// old method
	for (int i = 0; i < entitiesToHit.size(); i++)
	{
		Entity *e = entitiesToHit[i];
		DamageData d;
		d.attacker = this;
		d.damageType = DT_AVATAR_SHOCK;
		d.damage = 3;
		e->damage(d);

		dsq->playVisualEffect(VFX_SHOCKHIT, e->position, e);

		EnergyTendril *t = new EnergyTendril(this, e);
		core->addRenderObject(t, LR_PARTICLES);

		e->shock();
	}
	*/

	//HACK: WHAT DOES THIS VARIABLE DO EXACTLY?
	didShockDamage = true;


	//loseTargets();
}

void Avatar::updateShock(float dt)
{
	/*
	if (shockTimer > 0)
	{
		float shockTime = 0.75;
		castShockTimer += dt;
		std::vector<Entity*> closestEntities;
		unsigned int c=0;
		const float shotDelayTime = 0.05;


		if (damageDelay > 0)
		{
			damageDelay -= dt;
			if (damageDelay <= 0)
				damageDelay = 0;
		}

		if (damageDelay == 0)
		{
		}

		if (damageDelay == 0)
		{
			damageDelay = 999;
		}

		shockTimer -= dt;
		if (shockTimer < 0)
		{
			shockTimer = 0;
		}
	}
	*/
}

void Avatar::formAbilityUpdate(float dt)
{
	switch(dsq->continuity.form)
	{
	case FORM_FISH:
	{
		if (core->mouse.buttons.right)
		{
			const float bubbleRate = 0.2;

			state.abilityDelay -= dt;
			if (state.abilityDelay < 0)
				state.abilityDelay = 0;

			if (state.abilityDelay == 0)
			{
				state.abilityDelay = bubbleRate;
				//state.abilityDelay -= bubbleRate;
				// spawn bubble
				//Entity *bubble = dsq->game->createEntity("FishFormBubble", 0, position, 0, false, "");
				Vector dir = getAim();
				dir.normalize2D();

				dsq->game->fireShot("FishFormBubble", this, 0, position+dir*16, dir);
			}
		}
	}
	break;
	case FORM_ENERGY:
	{
		/*
		if (core->mouse.buttons.right && mana > 0 && !core->mouse.buttons.left)
		{
			float shockTime = 0.75;
			castShockTimer += dt;
			std::vector<Entity*> closestEntities;
			unsigned int c=0;
			const float shotDelayTime = 0.05;


			int maxHit = 2 + dsq->continuity.getSpellLevel(SPELL_SHOCK)*2;
			FOR_ENTITIES (i)
			{
				Entity *e = *i;
				Vector d = e->position - this->position;
				if (e != this && !e->isEntityDead() && e->isAffectedBySpell(SPELL_SHOCK) && d.getSquaredLength2D() < sqr(400+(dsq->continuity.getSpellLevel(SPELL_SHOCK)-1)*100))
				{
					state.shotDelay += dt;
					if (state.shotDelay > shotDelayTime)
					{
						state.shotDelay -= shotDelayTime;

						EnergyTendril *t = new EnergyTendril(avatar->position, e->position);
						core->addRenderObject(t, LR_PARTICLES);
					}
					e->offset.x = rand()%5;
					e->shock();
					DamageData d;
					d.attacker = this;
					d.spellType = SPELL_SHOCK;
					d.damage = 1+(dsq->continuity.getSpellLevel(SPELL_SHOCK)-1)*1;
					e->hit(d);
					c ++;
				}
				if (c >= maxHit)
					break;
			}
			//std::ostringstream os;
			//os << "castShockTimer: " << castShockTimer << " - shockTime: " << shockTime;
			//debugLog(os.str());
			if (castShockTimer > shockTime)
			{
				castShockTimer -= shockTime;
				mana --;
			}
		}
		else
		{
			state.shotDelay = 0;
			endShock();
		}
		*/
	}
	break;
	}
}

bool Avatar::isMouseInputEnabled()
{
	if (!inputEnabled) return false;
	//if (dsq->continuity.getWorldType() != WT_NORMAL) return false;
	//if (getState() != STATE_IDLE) return false;
	if (dsq->game->isPaused()) return false;
	return true;
}

int rmb_flag = 0;
void Avatar::rmbd2()
{
	rmb_flag = 1;
	rmbd();
	rmb_flag = 0;
}

void Avatar::rmbd()
{
	//core->setDockIcon("BitBlot");
	if (!isMouseInputEnabled() || isEntityDead()) return;
	if (dsq->continuity.form == FORM_NORMAL )
	{
		//if (isCoordinateInRadius(dsq->getGameCursorPosition(), 96))
		///Vector diff = core->mouse.position - c;
		if (dsq->inputMode == INPUT_MOUSE && !rmb_flag)
		{
			Vector diff = getVectorToCursorFromScreenCentre();
			if (diff.getSquaredLength2D() < sqr(openSingingInterfaceRadius))
				openSingingInterface();
		}
		else
		{
			openSingingInterface();
		}
	}
	else
	{
		if (spellCastDelay == 0)
			startCharge(0);
	}
	/*
	if (spellCastDelay == 0)
		startCharging(1);
	*/
}

void Avatar::rmbu()
{
	if (!isMouseInputEnabled() || isEntityDead()) return;

	if (charging)
	{
		if (!entityToActivate && !pathToActivate)
			formAbility(0);

		endCharge();
	}


	dsq->cursorGlow->alpha.interpolateTo(0, 0.2);
	dsq->cursorBlinker->alpha.interpolateTo(0, 0.2);

	if (pickingPullTarget)
	{
		if (potentialPullTarget)
		{
			pullTarget = potentialPullTarget;
			debugLog("Calling start pull");
			pullTarget->startPull();
		}
		closePullTargetInterface();
	}

	if (singing)
	{
		closeSingingInterface();
	}


	if (entityToActivate)
	{
		activateEntity = entityToActivate;
		entityToActivate = 0;
	}
	if (pathToActivate)
	{
		pathToActivate->activate();
		pathToActivate = 0;
	}



	/*
	if (charging)
	{
		formAbility(1);
		endCharge();
	}
	*/
}

bool Avatar::canCharge(int ability)
{
	switch(dsq->continuity.form)
	{
	case FORM_ENERGY:
		if (ability == 0) return true;
	break;
	case FORM_BEAST:
		//if (inTummy) return true;
	break;
	case FORM_DUAL:
		/*
		if (dualFormMode == DUALFORM_NAIJA)
		{
			if (dualFormCharge >= requiredDualFormCharge)
			{
				return true;
			}
		}
		else
		{
			return true;
		}
		*/
		return true;
	break;
	case FORM_NATURE:
		if (ability == 0)
			return true;
	break;
	case FORM_SUN:
		return true;
	break;
	}
	return false;
}

void Avatar::startCharge(int ability)
{
	if (!isCharging() && canCharge(ability))
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
		spellChargeMin = 0;
		chargeLevelAttained = 0;

		/*
		chargeGraphic->alpha = 0;
		chargeGraphic->scale = Vector(0,0);
		chargeGraphic->alpha.interpolateTo(0.6, chargeMax, 0);
		float sz = 1.5;
		chargeGraphic->scale.interpolateTo(Vector(sz,sz), chargeMax, 0);
		*/

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
		abilityCharging = ability;

	}
	if (!canCharge(ability))
	{
		formAbility(ability);
	}
}

void Avatar::setBlockSinging(bool v)
{
	blockSinging = v;
}

bool Avatar::canSetBoneLock()
{
	/*
	if (dsq->continuity.form == FORM_FISH || dsq->continuity.form == FORM_SPIRIT)
		return false;
	*/

	return true;
}

void Avatar::onSetBoneLock()
{
	Entity::onSetBoneLock();

	if (boneLock.on)
	{
		skeletalSprite.transitionAnimate("wallLookUp", 0.2, -1);
		lockToWallCommon();
		state.lockedToWall = 1;
		wallNormal = boneLock.localOffset;
		wallNormal.normalize2D();
		rotateToVec(wallNormal, 0.1);
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
	rotateToVec(wallNormal, 0.01);
}

void Avatar::lmbd()
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
	/*
	else
	{
		if (spellCastDelay == 0)
			startCharging(0);
	}
	*/
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
		offset.interpolateTo(Vector(0,0), 0.1);
		if (!wallNormal.isZero())
		{
			Vector velSet = wallNormal;
			velSet.setLength2D(200);
			vel += velSet;
		}
		//doCollisionAvoidance(dt, 5, 1);
	}
}

void Avatar::lmbu()
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


		/*
		std::ostringstream os;
		os << "spellCharge: " << spellCharge;
		debugLog(os.str());
		*/
		/*
		chargeGraphic->alpha.interpolateTo(0, 0.5, 0);
		chargeGraphic->scale.interpolateTo(Vector(0,0), 1.0, 0);
		*/

		chargingEmitter->stop();

		charging = false;
		state.spellCharge = 0;
	}
}

Vector Avatar::getWallNormal(TileVector t)
{
	return dsq->game->getWallNormal(t.worldVector(), 5)*-1;

	/*
	Vector accum;
	int c = 0;
	for (int x = -2; x <= 2; x++)
	{
		for (int y = -2; y <= 2; y++)
		{
			TileVector check = t;
			check.x+=x;
			check.y+=y;
			if (!dsq->game->isObstructed(check))
			{
				Vector v(check.x, check.y);
				c++;
				accum+= v;
			}
			//check.x+x, check.y+y
		}
	}
	if (c > 0)
	{
		accum /= c;
		//accum.normalize2D();
		accum.setLength2D(-1);
		return accum;
	}
	Vector v = vel;
	v.setLength2D(-1);
	return v;
	*/
}

int Avatar::getSingingInterfaceRadius()
{
	return singingInterfaceRadius;
}

int Avatar::getOpenSingingInterfaceRadius()
{
	return openSingingInterfaceRadius;
}

bool Avatar::isSwimming()
{
	return swimming;
}

void Avatar::lockToWallCommon()
{
	swimEmitter.stop();

	skeletalSprite.stopAllAnimations();
	rotationOffset.interpolateTo(0, 0.01);

	fallGravityTimer = 0;

	dsq->spawnParticleEffect("LockToWall", position);
	stopBurst();
	stopRoll();
	disableOverideMaxSpeed();
	core->sound->playSfx("LockToWall", 1.0, 0);//, (1000+rand()%100)/1000.0f);
	//bursting = false;
	animatedBurst = false;
	this->burst = 1;
	//lastLockToWallPos = position;

	state.lockToWallDelay.start(0.2);
	state.lockedToWall = true;

	lockToWallFallTimer = -1;

	// move this to its own function?
	state.backFlip = false;
	skeletalSprite.getAnimationLayer(ANIMLAYER_OVERRIDE)->stopAnimation();
}

void Avatar::lockToWall()
{
	if (riding) return;
	if (inCurrent && dsq->continuity.form != FORM_BEAST) return;
	if (dsq->continuity.form == FORM_FISH || dsq->continuity.form == FORM_SPIRIT) return;
	if (state.lockedToWall) return;
	if (vel.x == 0 && vel.y == 0) return;
	if (dsq->game->isPaused()) return;

	/*
	Vector opos = position;
	position = lastPosition;
	*/

	TileVector t(position);
	TileVector myTile = t;
	// 3 + 4
	// 4 + 5
	Vector m = vel;
	m.setLength2D(3);
	t.x += int(m.x);
	t.y += int(m.y);
	/*
	TileVector t2 = t;
	m.setLength2D(4);
	t2.x += int(m.x);
	t2.y += int(m.y);
	*/
	m.setLength2D(2);
	TileVector tback = myTile;
	tback.x += int(m.x);
	tback.y += int(m.y);

	Vector add = m;
	add.setLength2D(1);
	TileVector tnext = myTile;
	tnext.x += int(add.x);
	tnext.y += int(add.y);

	// find the fraking wall
	/*
	TileVector actualWall = myTile;
	Vector lastWall;
	Vector getWall(myTile.x, myTile.y);
	for (int i = -1; i < 5; i++)
	{
		getWall.x += add.x*i;
		getWall.y += add.y*i;
		if (lastWall.isZero())
			lastWall = getWall;
		TileVector test(getWall.x, getWall.y);
		if (dsq->game->isObstructed(test))
			break;
		lastWall = getWall;
	}
	actualWall = TileVector(lastWall.x, lastWall.y);
	*/

	Vector diff = lastLockToWallPos - position;

	bool good = true;
	if (!dsq->game->isObstructed(t))
	{
		int tried = 0;
//tryAgain:
		while(1)
		{
			TileVector test;

			test = TileVector(t.x, t.y+1);
			if (dsq->game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x, t.y-1);
			if (dsq->game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x-1, t.y);
			if (dsq->game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x+1, t.y);
			if (dsq->game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x+1, t.y+1);
			if (dsq->game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x-1, t.y+1);
			if (dsq->game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x+1, t.y-1);
			if (dsq->game->isObstructed(test))
			{
				t = test;
				break;
			}
			test = TileVector(t.x-1, t.y-1);
			if (dsq->game->isObstructed(test))
			{
				t = test;
				break;
			}
			tried++;
			//if (tried >= 2)
			if (true)
			{
				good = false;
				break;
			}
			else
			{
				//debugLog("trying other");
				//t = myTile;
				//goto tryAgain;
			}
		}
	}

	if (dsq->game->getGrid(t)==OT_HURT && dsq->continuity.form != FORM_NATURE)
	{
		good = false;
	}
	if (good /*&& dsq->game->)isObstructed(t2, OT_BLACK)*/ /*&& diff.getSquaredLength2D() > sqr(40)*/)
	{
		wallNormal = dsq->game->getWallNormal(position);
		bool outOfWaterHit = (!_isUnderWater && !(wallNormal.y < -0.1f));
		if (wallNormal.isZero() ) //|| outOfWaterHit
		{
			debugLog("COULD NOT FIND NORMAL, GOING TO BOUNCE");
			if (outOfWaterHit)
			{
				/*
				Animation *anim = skeletalSprite.getCurrentAnimation();
				if (anim && anim->name == "hitGround")
				{
				}
				else
				{
					skeletalSprite.animate("hitGround");
				}
				*/
			}
			return;
		}
		else
		{

			/*
			position = TileVector(position).worldVector();
			lastPosition = position;
			*/

			if (!dsq->mod.isActive() && !dsq->continuity.getFlag("lockedToWall"))
			{
				
				if (!dsq->game->isControlHint()){
					dsq->continuity.setFlag("lockedToWall", 1);
					dsq->game->setControlHint(dsq->continuity.stringBank.get(13), 1, 0, 0, 6, "", true);
				}
			}

			lockToWallCommon();

			if (outOfWaterHit)
				lockToWallFallTimer = 0.4;
			else
				lockToWallFallTimer = -1;

			//wallPushVec = getWallNormal(t);


			wallPushVec = wallNormal;
			wallPushVec *= 2000;
			wallPushVec.z = 0;
			skeletalSprite.stopAllAnimations();
			if (wallPushVec.y < 0 && (fabsf(wallPushVec.y) > fabsf(wallPushVec.x)))
			{
				skeletalSprite.transitionAnimate("wallLookUp", 0.2, -1);
			}
			else
			{
				skeletalSprite.transitionAnimate("wall", 0.2, -1);
			}
			rotateToVec(wallPushVec, 0.1);

			offset.stop();

			Vector goIn;

			TileVector uset;
			if (!dsq->game->isObstructed(tnext))
			{
				uset = tnext;
			}
			else
				uset = tback;

			int tileType = dsq->game->getGrid(t);
			Vector offdiff = t.worldVector() - position;
			if (!offdiff.isZero())
			{
				if (tileType != OT_INVISIBLEIN)
				{
					Vector adjust = offdiff;
					adjust.setLength2D(TILE_SIZE*2);
					offdiff -= adjust;
				}
				else
				{
					Vector adjust = offdiff;
					adjust.setLength2D(TILE_SIZE/2);
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
			/*
			std::ostringstream os;
			os << "time: " << time;
			debugLog(os.str());
			*/
			offset.interpolateTo(offdiff, time);
			/*
			if (tileType == OT_INVISIBLEIN)
			{
				goIn = wallNormal;
				goIn.setLength2D(-28);
			}
			else
			{
				Vector diff = uset.worldVector()-position;
				goIn = diff;
			}
			offset.interpolateTo(goIn, 0.05);
			*/

			wallLockTile = t;

			vel = Vector(0,0,0);
			vel2 = 0;

			/*
			Vector oldPos = position;

			while (true)
			{
				Vector m = vel;
				m |= 1;
				position += m;
				TileVector t(position);
				if (dsq->game->isObstructed(t, OT_BLACK))
				{
					position = oldPos;
					break;
				}
			}
			*/
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
	color.interpolateTo(BLIND_COLOR, 0.5);
	currentColor = BLIND_COLOR;

	tripper->alpha.interpolateTo(1, 8);

	tripper->color = Vector(1, 1, 1);
	tripper->rotation.z = 0;
	tripper->rotation.interpolateTo(Vector(0, 0, 360), 10, -1);
	tripper->scale = Vector(1.25, 1.25, 1.25);
	tripper->scale.interpolateTo(Vector(1.3, 1.3, 1.3), 2, -1, 1, 1);

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
	currentColor = Vector(1,1,1);
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
	color.interpolateTo(BLIND_COLOR, 0.5);
	currentColor = BLIND_COLOR;
	blinder->alpha.interpolateTo(1, 0.5);

	blinder->rotation.z = 0;
	blinder->rotation.interpolateTo(Vector(0, 0, 360), 10, -1);
	blinder->scale = Vector(1.25, 1.25, 1.25);
	blinder->scale.interpolateTo(Vector(1.3, 1.3, 1.3), 2, -1, 1, 1);

	//dsq->toggleMuffleSound(1);
}

void Avatar::removeBlindEffects()
{
	color.interpolateTo(Vector(1,1,1),0.5);
	currentColor = Vector(1,1,1);
	blinder->alpha.interpolateTo(0, 0.5);
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

void Avatar::openPullTargetInterface()
{
	debugLog("Open pull target");
	if (pullTarget)
	{
		pullTarget->stopPull();
	}
	pullTarget = 0;
	potentialPullTarget = 0;
	pickingPullTarget = true;
	// change the cursor
	dsq->cursor->color = Vector(0.5,0.5,1);
}

void Avatar::closePullTargetInterface()
{
	debugLog("close pull target");
	pickingPullTarget = false;
	potentialPullTarget = 0;
	dsq->cursor->color = Vector(1,1,1);
}

void Avatar::createWeb()
{
	web = new Web;
	web->setParentEntity(this);
	dsq->game->addRenderObject(web, LR_ENTITIES);
	curWebPoint = web->addPoint(dsq->game->avatar->position);
	curWebPoint = web->addPoint(dsq->game->avatar->position);
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
	warpIn = false;
#ifdef AQ_TEST_QUADTRAIL
	quadTrail = new QuadTrail(100, 32);
	quadTrail->setTexture("Particles/QuadTrail");
	quadTrail->setBlendType(BLEND_ADD);
	dsq->game->addRenderObject(quadTrail, LR_PARTICLES);
#endif

	curWebPoint = 0;

	web = 0;

	lastBurstType = BURST_NONE;
	dsq->loops.shield = BBGE_AUDIO_NOCHANNEL;
	leftHandEmitter = rightHandEmitter = 0;
	boneLeftHand = boneRightHand = 0;
	canChangeForm = true;
	biteTimer = 0;
	dsq->loops.charge = BBGE_AUDIO_NOCHANNEL;
	//heartbeat = 0;

	lastNote = -1;
	headTextureTimer = 0;
	bone_dualFormGlow = 0;
	//dsq->continuity.dualFormCharge = 0;
	//dsq->continuity.dualFormMode = Continuity::DUALFORM_NAIJA;
	debugLog("Avatar 1");

	//registerEntityDied = true;
	setv(EV_ENTITYDIED, 1);
	wallJumps = 0;
	wallBurstTimer = 0;
	beautyFlip = false;
	invincibleBreak = true;
	targetUpdateDelay = 0;
	biteDelay = 0;
	songInterfaceTimer = 0;
	quickSongCastDelay = 0;
	flourish = false;
	tummyAmount=0;

	blockSinging = false;
	singing = false;

	spiritEnergyAbsorbed = 0;
	joystickMove = false;

	debugLog("setCanLeaveWater");

	setCanLeaveWater(true);

	debugLog("setOverrideRenderPass");

	setOverrideRenderPass(1);

	debugLog("Done those");
	/*
	setRenderPass(2);
	*/
	rippleDelay = 0;
	ripples = false;
	fallGravityTimer = 0;
	lastOutOfWaterMaxSpeed = 0;
	//chargeGraphic = 0;
	tummyTimer = 0;
	inTummy = EAT_NONE;
	ropeTimer = shieldPoints = auraTimer = 0;
	glow = 0;

	fireDelay = 0;
	inFormInterface = false;
	looking = false;
	canWarpDelay = 0.5;
	canWarp = false;
	rollDidOne = 0;
	lastQuad = lastQuadDir = rollDelay = rolling = 0;
	stopTimer = 0;
	doubleClickDelay = 0;
	damageDelay = 0;
	didShockDamage = false;
	chargeLevelAttained = 0;
	shockTimer = 0;
	activeAura = AURA_NONE;
	ropeState = 0;
	movingOn = false;
	currentMaxSpeed = 0;
	abilityCharging = -1;
	pickingPullTarget = false;
	potentialPullTarget = 0;
	pullTarget = 0;
	revertTimer = 0;
	currentSongIdx = -1;
	leaches = 0;


	debugLog("Avatar vars->");

	damageTime = vars->avatarDamageTime;

	activateEntity = 0;
	canMove = true;
	castShockTimer = 0;
	//scale = Vector(0.5, 0.5);
	scale = Vector(0.5, 0.5);

	debugLog("Avatar 2");
	//scale = Vector(1.0, 1.0);
	//setTexture("Naija-sprite2");
	renderQuad = false;
	name = "Naija";
	setEntityType(ET_AVATAR);

	targets.resize(1);


	lastEntityActivation = 0;

	entityToActivate = 0;
	pathToActivate = 0;
	zoomOverriden = false;
	canWarp = true;
	blinder = 0;
	zoomVel = 0;

	myZoom = Vector(1,1);
	spellChargeMin = spellCastDelay = 0;
	this->pushingOffWallEffect = 0;
	lockToWallFallTimer  = 0;
	swimming = false;
	dodgeDelay = 0;
	charging = false;
	bursting = false;
	animatedBurst = false;
	burst = 1;
	burstDelay = 0;
	ignoreInputDelay = 0;
	idleAnimDelay = 2;
	splashDelay = 0;
	avatar = this;

	frame = 0;

	particleDelay = 0;

	swimming = false;

	debugLog("Avatar 3");
	hair = new Hair();
	hair->setTexture("Naija/Cape");
	hair->setOverrideRenderPass(1);
	hair->setRenderPass(1);
	dsq->game->addRenderObject(hair, LR_ENTITIES);

	debugLog("Avatar 4");

	bindInput();

	debugLog("Avatar 5");

	blinder = new PauseQuad;
	blinder->position = Vector(400, 300, 4.5);
	blinder->setTexture("particles/blinder");
	//blinder->width = blinder->height = 810;
	blinder->autoWidth = AUTO_VIRTUALWIDTH;
	blinder->autoHeight = AUTO_VIRTUALWIDTH;
	blinder->scale = Vector(1.0125,1.0125);
	blinder->followCamera = 1;

	blinder->alpha = 0;
	dsq->game->addRenderObject(blinder, LR_AFTER_EFFECTS);

	tripper = new PauseQuad;
	tripper->position = Vector(400,300);
	tripper->setTexture("particles/tripper");
	//tripper->setWidthHeight(810, 810);
	tripper->autoWidth = AUTO_VIRTUALWIDTH;
	tripper->autoHeight = AUTO_VIRTUALWIDTH;
	tripper->scale = Vector(1.0125, 1.0125);
	tripper->followCamera = 1;
	tripper->alpha = 0;
	dsq->game->addRenderObject(tripper, LR_AFTER_EFFECTS);

	songIcons.resize(8);
	int i = 0;
	for (i = 0; i < songIcons.size(); i++)
	{
		songIcons[i] = new SongIcon(i);
		songIcons[i]->alpha = 0;
		songIcons[i]->followCamera = 1;
		dsq->game->addRenderObject(songIcons[i], LR_HUD);
	}

	setSongIconPositions();

	fader = new Quad;
	fader->position = Vector(400,300);
	fader->setTexture("fader");
	fader->setWidthHeight(core->getVirtualWidth()+10);
	fader->followCamera = 1;
	fader->alpha = 0;
	dsq->game->addRenderObject(fader, LR_AFTER_EFFECTS);

	text = 0;

	burstBar = 0;

	/*
	chargeGraphic = new Particle;
	{
		chargeGraphic->setBlendType(RenderObject::BLEND_ADD);
		chargeGraphic->setTexture("glow");
		chargeGraphic->alpha = 0;
		//chargeGraphic->color = Vector(1,,0);
		chargeGraphic->width = 128;
		chargeGraphic->height = 128;
		chargeGraphic->scale = Vector(0,0);
		chargeGraphic->parentManagedPointer = 1;
		//chargeGraphic->positionSnapTo = &this->position;
		chargeGraphic->rotation.interpolateTo(Vector(0,0,360), 1, -1, 1);
		chargeGraphic->position = Vector(16, 58);
		//chargeGraphic->color = Vector(1,0.2,0.1);
	}
	//skeletalSprite.getBoneByIdx(3)->addChild(chargeGraphic);
	*/

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
		dsq->game->addRenderObject(targetQuads[i], LR_PARTICLES);
	} 

	lightFormGlow = new Quad("Naija/LightFormGlow", 0);
	lightFormGlow->alpha = 0;
	
	lightFormGlow->scale.interpolateTo(Vector(5.5, 5.5), 0.4, -1, 1);
	//lightFormGlow->positionSnapTo = &position;
	dsq->game->addRenderObject(lightFormGlow, LR_ELEMENTS13);

	lightFormGlowCone = new Quad("Naija/LightFormGlowCone", 0);
	lightFormGlowCone->alpha = 0;
	lightFormGlowCone->scale = Vector(1, 6); // 4.5
	dsq->game->addRenderObject(lightFormGlowCone, LR_ELEMENTS13);


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
	/*
	addChild(&chargingEmitter);
	chargingEmitter.parentManagedStatic = true;
	*/

	chargeEmitter = new ParticleEffect;
	dsq->getTopStateData()->addRenderObject(chargeEmitter, LR_PARTICLES_TOP);

	leftHandEmitter = new ParticleEffect;
	dsq->getTopStateData()->addRenderObject(leftHandEmitter, LR_PARTICLES);

	rightHandEmitter = new ParticleEffect;
	dsq->getTopStateData()->addRenderObject(rightHandEmitter, LR_PARTICLES);

	/*
	leftHandEmitter = new ParticleEffect;
	dsq->getTopStateData()->addRenderObject(`, LR_PARTICLES);

	rightHandEmitter = new ParticlesEffect;
	dsq->getTopStateData()->addRenderObject(rightHandEmitter, LR_PARTICLES);
	*/

	/*
	addChild(&chargeEmitter);
	chargeEmitter.parentManagedStatic = true;
	*/

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

	if (dsq->useMic)
	{
		debugLog("useMic...initing recording");

		/*
		debugLog("RecordStart...");
		avatarRecord = BASS_RecordStart(44100,1,0,&recordCallback,0);

		if (avatarRecord)
		{
			if (dsq->autoSingMenuOpen)
			{
			}
			else
			{
				debugLog("ChannelPause...");
				BASS_ChannelPause(avatarRecord);
			}
		}
		debugLog("...done");
		*/
	}

	debugLog("Avatar 10");
	setDamageTarget(DT_AVATAR_LANCE, false);

	//changeForm(FORM_NORMAL, false);

	refreshNormalForm();

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
	if (true)
	{
		if (hair)
			hair->alphaMod = 1.0;
		if (!c.empty() && c!="Naija")
		{
			if (exists(core->getBaseTextureDirectory() + "naija/cape-"+c+".png"))
			{
				if (hair)
					hair->setTexture("naija/cape-"+c);
			}
			else
			{
				if (hair)
					hair->alphaMod = 0;
			}
		}
		else
		{
			if (hair)
				hair->setTexture("naija/cape");
		}
	}
	else
	{
		if (hair)
			hair->alphaMod = 0.0;
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
		boneRightFoot = skeletalSprite.getBoneByName("RightFoot");
		boneLeftFoot = skeletalSprite.getBoneByName("LeftFoot");
		boneRightArm = skeletalSprite.getBoneByName("RightArm");
		boneLeftArm = skeletalSprite.getBoneByName("LeftArm");
		boneFish2 = skeletalSprite.getBoneByName("Fish2");
		boneFish2->alpha = 0;
		bone_dualFormGlow = skeletalSprite.getBoneByName("DualFormGlow");
		bone_dualFormGlow->scale = 0;
		bone_dualFormGlow->setBlendType(BLEND_ADD);

		boneLeftHand = skeletalSprite.getBoneByName("LeftArm");
		boneRightHand = skeletalSprite.getBoneByName("RightArm");
	}
	else
	{
		bone_dualFormGlow = 0;
		bone_head = 0;
		boneRightFoot = boneLeftFoot = boneRightArm = boneLeftArm = boneFish2 = skeletalSprite.getBoneByIdx(0);
		boneLeftHand = boneRightHand = 0;
	}

	core->resetTimer();

	skeletalSprite.getAnimationLayer(ANIMLAYER_UPPERBODYIDLE)->stopAnimation();
}

Avatar::~Avatar()
{
	songIcons.clear();
}

void Avatar::destroy()
{
	Entity::destroy();

	text = 0;

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

void Avatar::fireRope()
{
	ropeVel = getAim();
	ropeVel.z = 0;
	if (!ropeVel.isLength2DIn(1))
	{
		ropeTimer = 0.2;
		ropeState = 1;
		ropePos = position;
		//ropeVel = core->mouse.position - Vector(400,300);

		ropeVel.setLength2D(7000);
		//ropeVel |= 500;
	}
	else
		ropeVel = Vector(0,0,0);
}

void Avatar::toggleZoom()
{
	if (core->globalScale.isInterpolating()) return;
	if (core->globalScale.x == 1)
		core->globalScale.interpolateTo(Vector(0.75,0.75),0.2);
	else if (core->globalScale.x == 0.75)
		core->globalScale.interpolateTo(Vector(0.5,0.5),0.2);
	else if (core->globalScale.x == 0.5)
		core->globalScale.interpolateTo(Vector(0.25,0.25),0.2);
	else if (core->globalScale.x == 0.25)
		core->globalScale.interpolateTo(Vector(1,1),0.2);

	/*
	else if (core->globalScale.x == 1.5)
		core->globalScale.interpolateTo(Vector(1,1),0.2);
	*/

}

/*
void Avatar::setActiveSpell(Spells spell)
{
	activeSpell = spell;
}
*/

void Avatar::dodge(std::string dir)
{
	if (bursting) return;
	if (!canMove) return;
	if (dodgeDelay == 0)
	{
		Vector mov;

		if (dir == "right")
			mov = Vector(1,0);
		else if (dir == "left")
			mov = Vector(-1, 0);
		else if (dir == "down")
			mov = Vector(0, 1);
		else if (dir == "up")
			mov = Vector(0, -1);

		Vector lastPosition = position;
		//position += mov * 80;

		dodgeVec = mov * 8000;
		vel += mov * vars->maxDodgeSpeed;
		//dodgeEffectTimer = 0.125;
		state.dodgeEffectTimer.start(/*0.125*/vars->dodgeTime);
		/*
		float vlen = vel.getLength2D();
		mov |= 500;
		vel += mov;
		*/
		/*
		if (dsq->game->collideCircleWithGrid(position, 24))
		{
			position = lastPosition;
		}
		*/
	}
}

void Avatar::startBackFlip()
{
	if (boneLock.on) return;
	if (riding) return;

	skeletalSprite.getAnimationLayer(ANIMLAYER_OVERRIDE)->transitionAnimate("backflip", 0.2, 0);
	vel.x = -vel.x*0.25f;
	state.backFlip = true;
}

void Avatar::stopBackFlip()
{
	if (state.backFlip)
	{
		//skeletalSprite.getAnimationLayer(ANIMLAYER_OVERRIDE)->stopAnimation();
		skeletalSprite.getAnimationLayer(ANIMLAYER_OVERRIDE)->transitionAnimate("backflip2", 0.2, 0);
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
	//getVectorToCursorFromScreenCentre()
	//!bursting && burst == 1
	//&&
	/*
	bool nearWallProblem = false;
	if (vel.isLength2DIn(200))
	{
		if ()
		{
			nearWallProblem = false
		}
		else
		{
			nearWallProblem = true;
		}
	}
	*/
	//&& !vel.isLength2DIn(32)
	if (!riding && dsq->continuity.form != FORM_SPIRIT && (joystickMove || getVectorToCursor().getSquaredLength2D() > sqr(BURST_DISTANCE))
		&& getState() != STATE_PUSH && (!skeletalSprite.getCurrentAnimation() || (skeletalSprite.getCurrentAnimation()->name != "spin"))
		&& _isUnderWater && !isActing(ACTION_ROLL))
	{
		if (!bursting && burst == 1)
		{
			dsq->rumble(0.2, 0.2, 0.2);
			if (dsq->continuity.form != FORM_BEAST)
				wakeEmitter.start();
			dsq->game->playBurstSound(pushingOffWallEffect>0);
			skeletalSprite.animate(getBurstAnimName(), 0);
			bursting = true;
			burst = 1.0;
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

		offset.interpolateTo(Vector(0,0), 0.05);

		dsq->spawnParticleEffect("WallBoost", position+offset, rotation.z);
		if (goDir.x != 0 || goDir.y != 0)
		{
			lastBurstType = BURST_WALL;
			/*
			if (wallBurstTimer > 0)
			{
				Vector wallJumpDir = position - lastWallJumpPos;
				if (lastWallJumpPos.isZero() || wallJumpDir.dot2D(lastWallJumpPos) > 0.2f)
				{
					std::ostringstream os;
					os << "wallJumps: " << wallJumps;
					debugLog(os.str());
					wallJumps++;
				}
				else
				{
					debugLog("failed angle");
					stopWallJump();
				}
			}
			wallBurstTimer = 0.8f - 0.1f * wallJumps;
			if (wallBurstTimer <= 0)
			{
				// super boost!
				debugLog("super boost!");
				wallJumps = 0;
			}


			lastWallJumpPos = position;
			lastWallJumpDir = position - lastWallJumpPos;
			*/

			dsq->rumble(0.22, 0.22, 0.2);
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
			if (wallJumps > 0)
			{
				dsq->sound->playSfx("WallJump", 255, 0, 1000+wallJumps*100);
			}
			dsq->game->playBurstSound(pushingOffWallEffect>0);
			skeletalSprite.animate(getBurstAnimName(), 0);
			bursting = true;
			burst = 1.5;
			ripples = true;

			startBurstCommon();
			/*
			if (core->afterEffectManager)
				core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.04,0.06,15,0.2f));
			*/


			/*
			float len = wallPushVec.getLength2D();
			goDir |= len;
			goDir.z = 0;

			Vector test = goDir;
			test |= TILE_SIZE*2;
			if (dsq->game->isObstructed(TileVector(position + test)))
			{
			}
			else
				wallPushVec = goDir;
			*/
			/*
			wallPushVec = Vector((wallPushVec.x+goDir.x)/2, (wallPushVec.y+goDir.y)/2);
			wallPushVec |= len;
			*/
		}
		else
		{
			//wallPushVec |= 10;
		}
	}
}

void Avatar::doDodgeInput(const std::string &action, int s)
{
	if (s)
	{
		if (tapped.empty())
		{
			tapped = action;
			state.tapTimer.start(0.25);
		}
		else if (tapped == action)
		{
			if (state.tapTimer.isActive())
				dodge(action);
			tapped = "";
			dodgeDelay = 1.0;
		}
		else
		{
			tapped = "";
		}
	}
}

Vector Avatar::getKeyDir()
{
	Vector dir;
	if (isActing(ACTION_SWIMLEFT))
		dir += Vector(-1,0);
	if (isActing(ACTION_SWIMRIGHT))
		dir += Vector(1,0);
	if (isActing(ACTION_SWIMUP))
		dir += Vector(0,-1);
	if (isActing(ACTION_SWIMDOWN))
		dir += Vector(0,1);

	if (dir.x != 0 && dir.y != 0)
		dir/=2;

	return dir;
}

Vector Avatar::getFakeCursorPosition()
{
	if (dsq->inputMode == INPUT_KEYBOARD)
	{
		return getKeyDir() * 350;
	}
	if (dsq->inputMode == INPUT_JOYSTICK)
	{
		const float axisInput = core->joystick.position.getLength2D();
		if (axisInput < JOYSTICK_LOW_THRESHOLD)
		{
			return Vector(0,0,0);
		}
		else
		{
			const float axisMult = (maxMouse - minMouse) / (JOYSTICK_HIGH_THRESHOLD - JOYSTICK_LOW_THRESHOLD);
			const float distance = minMouse + ((axisInput - JOYSTICK_LOW_THRESHOLD) * axisMult);
			return (core->joystick.position * (distance / axisInput));
		}
	}
	return Vector(0,0,0);
}

Vector Avatar::getVectorToCursorFromScreenCentre()
{
	/*
	if (core->joystickEnabled)
	{
		Vector joy(core->joystate.lX-(65536/2), core->joystate.lY-(65536/2));
		float len = (joy.getLength2D() * 600) / 65536;
		joy.setLength2D(len);
		std::ostringstream os;
		os << "joy (" << joy.x << ", " << joy.y << ")";
		debugLog(os.str());
		return joy;
	}
	*/
	if (game->cameraOffBounds)
		return getVectorToCursor();
	else
	{
		if (dsq->inputMode != INPUT_MOUSE)
			return getFakeCursorPosition();
		return (core->mouse.position+offset) - Vector(400,300);
	}
}

Vector Avatar::getVectorToCursor(bool trueMouse)
{
	//return getVectorToCursorFromScreenCentre();
	Vector pos = dsq->getGameCursorPosition();
	

	if (!trueMouse && dsq->inputMode != INPUT_MOUSE)
		return getFakeCursorPosition();

	return pos - (position+offset);
	//return core->mouse.position - Vector(400,300);
}

void Avatar::startWallCrawl()
{
	lastWallNormal = wallNormal;
	state.crawlingOnWall = true;
	skeletalSprite.transitionAnimate("crawl", 0.1, -1);
}

void Avatar::stopWallCrawl()
{
	state.crawlingOnWall = false;
	state.lockedToWall = false;
	idle();
}

void Avatar::action(int id, int state)
{
	if (id == ACTION_PRIMARY)	{ if (state) lmbd(); else lmbu(); }
	if (id == ACTION_SECONDARY) { if (state) rmbd(); else rmbu(); }

	if (id == ACTION_PRIMARY && state)// !state
	{
		if (isMiniMapCursorOkay())
		{
			if (this->state.lockedToWall && !this->state.crawlingOnWall)
			{
				Vector test = getVectorToCursor();
				if (test.isLength2DIn(minMouse))
				{
					fallOffWall();
					// previously didn't fall off wall with mouse.... why?
					/*
					//fallOffWall();
					if (dsq->inputMode == INPUT_JOYSTICK || dsq->inputMode == INPUT_KEYBOARD)
						fallOffWall();
					*/
				}
				/*
				else if (test.isLength2DIn(maxMouse))
				{
					this->state.lockedToWall = false;
					idle();
				}
				*/
				else
				{
					//if (dsq->continuity.setFlag("lockedToWall", 1)

					//!boneLock.entity && 
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

				/*
				else
				{
					wallNormal = getWallNormal(TileVector(position));
					Vector left = wallNormal.getPerpendicularLeft();
					Vector right = wallNormal.getPerpendicularRight();
					position += left*0.1f;
				}
				*/
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
				if (dsq->inputMode != INPUT_MOUSE)
					skeletalSprite.transitionAnimate("swim", ANIM_TRANSITION, -1);
			}
		}
	}
	/*
	// song note keys
	else if (!action.empty() && action[0] == 's')
	{
		if (isSinging())
		{
			int count=0;
			std::istringstream is(action.substr(1, action.size()));
			is >> count;

			count--;
			if (count >= 0 && count <= 7)
			{
				core->setMousePosition(songIcons[count]->position);
			}
		}
	}
	*/
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

void Avatar::doRangePull(float dt)
{
	int range = 4;
	Vector total;

	Vector dest = position + vel*dt;
	TileVector t(dest);
	std::vector<Vector> vectors;
	for (int x = t.x-range; x <= t.x+range; x++)
	{
		for (int y = t.y-range; y <= t.y+range; y++)
		{
			TileVector tile(x,y);
			if (!(tile.x == t.x &&  tile.y == t.y) && !dsq->game->isObstructed(tile)
				&& !dsq->game->isObstructed(TileVector(tile.x+1, tile.y))
				&& !dsq->game->isObstructed(TileVector(tile.x, tile.y+1))
				&& !dsq->game->isObstructed(TileVector(tile.x-1, tile.y))
				&& !dsq->game->isObstructed(TileVector(tile.x, tile.y+1))
				)
			{
				vectors.push_back(tile.worldVector());
				/*
				Vector obs = tile.worldVector();
				Vector mov = position - obs;

				int len = range*TILE_SIZE - mov.getLength2D();
				if (len < 0) len = 1;
				mov |= len;
				total += mov;
				*/
			}
		}
	}

	unsigned int amount = 5;
	if (amount > vectors.size())
		amount = vectors.size();
	std::vector<int>smallestDists;
	smallestDists.resize(amount);
	int i = 0;
	for (i = 0; i < smallestDists.size(); i++)
	{
		smallestDists[i] = 0x7FFFFFFF;
	}
	std::vector<Vector> closestVectors;
	closestVectors.resize(amount);
	for (i = 0; i < vectors.size(); i++)
	{
		Vector diff = dest - vectors[i];
		int dist = diff.getSquaredLength2D();
		for (int j = 0; j < smallestDists.size(); j++)
		{
			if (dist < smallestDists[j])
			{
				for (int k = smallestDists.size()-1; k > j; k--)
				{
					smallestDists[k] = smallestDists[k-1];
					closestVectors[k] = closestVectors[k-1];
				}
				smallestDists[j] = dist;
				closestVectors[j] = vectors[i];
			}
		}
	}

	for (i = 0; i < closestVectors.size(); i++)
	{
		Vector obs = closestVectors[i];
		if (obs.x == 0 && obs.y == 0) continue;
		Vector mov = obs - position;

		int len = range*TILE_SIZE - mov.getLength2D();
		if (len < 0) len = 0;
		else
		{
			mov.setLength2D(len);
			total += mov;
		}
	}

	if (total.x != 0 || total.y != 0)
	{
		//float vlen = vel.getLength2D();
		//float len = (range*TILE_SIZE - avgDist)/range*TILE_SIZE;
		//if (len > 0)
		{
			if (bursting && swimming)
			{
				total.setLength2D(dt*4000);
			}
			else if (swimming)
			{
				//vel = Vector(0,0,0);
				//vel |= vlen;
				total.setLength2D(dt*1000);
			}
			else
			{
				total.setLength2D(dt*200);
			}
			vel += total;
		}
		/*
		if (vlen < 250)
		{
			total |= 200*dt;
		}
		else
		*/
		/*
		{
			total |= 500*dt;
		}
		*/

	}
}

void Avatar::doRangePush(float dt)
{
	// current not used
	/*
	if (vel.getSquaredLength2D() < sqr(1)) return;
	int range = 4;
	Vector total;
	TileVector t(position);
	std::vector<Vector> vectors;
	for (int x = t.x-range; x <= t.x+range; x++)
	{
		for (int y = t.y-range; y <= t.y+range; y++)
		{
			TileVector tile(x,y);
			if (dsq->game->isObstructed(tile))
			{
				vectors.push_back(tile.worldVector());
			}
		}
	}

	int amount = 5;
	if (amount > vectors.size())
		amount = vectors.size();
	std::vector<int>smallestDists;
	smallestDists.resize(amount);
	for (int i = 0; i < smallestDists.size(); i++)
	{
		smallestDists[i] = 0x7FFFFFFF;
	}
	std::vector<Vector> closestVectors;
	closestVectors.resize(amount);
	for (int i = 0; i < vectors.size(); i++)
	{
		Vector diff = position - vectors[i];
		int dist = diff.getSquaredLength2D();
		for (int j = 0; j < smallestDists.size(); j++)
		{
			if (dist < smallestDists[j])
			{
				for (int k = smallestDists.size()-1; k > j; k--)
				{
					smallestDists[k] = smallestDists[k-1];
					closestVectors[k] = closestVectors[k-1];
				}
				smallestDists[j] = dist;
				closestVectors[j] = vectors[i];
			}
		}
	}

	float tot=0;
	int c = 0;
	for (int i = 0; i < smallestDists.size(); i++)
	{
		if (smallestDists[i] < HUGE_VALF)
		{
			tot += smallestDists[i];
			c++;
		}
	}
	float avgDist = range*TILE_SIZE;
	if (c > 0)
	{
		avgDist = tot / c;
	}



	for (int i = 0; i < closestVectors.size(); i++)
	{
		Vector obs = closestVectors[i];
		if (obs.x == 0 && obs.y == 0) continue;
		Vector mov = position - obs;

		int len = range*range*TILE_SIZE - mov.getLength2D();
		if (len < 0) len = 0;
		else
		{
			mov |= len;
			total += mov;
		}
	}

	if (total.x != 0 || total.y != 0)
	{
		float vlen = vel.getLength2D();
		//float len = (range*TILE_SIZE - avgDist)/range*TILE_SIZE;
		//if (len > 0)
		{
			float totLen = 0;
			if (swimming)
			{
				totLen = dt*600;
			}
			else
			{
				totLen = dt*50;
			}
			total |= totLen;

			Vector perp = vel;
			Vector n = vel;
			n.normalize2D();
			float d = total.dot2D(n);
			perp |= d;
			total -= perp;
			total |= totLen;
			lastPush = total;
			vel += total;
		}
	}
	*/

}

void Avatar::render()
{

	if (dsq->continuity.form == FORM_SPIRIT && !skeletalSprite.getParent())
	{
		skeletalSprite.position = bodyPosition+bodyOffset;
		skeletalSprite.color = Vector(0.2, 0.3, 0.6);
		skeletalSprite.render();
		skeletalSprite.color = Vector(1,1,1);
	}

	Entity::render();

	if (ropeState != 0)
	{
#ifdef BBGE_BUILD_OPENGL
		glPushMatrix();
			glColor4f(1.0f,0.0f,0.0f,1.0f);
			glBegin(GL_LINES);
				glVertex3f(position.x, position.y, 0);
				glVertex3f(ropePos.x, ropePos.y, 0);
			glEnd();
		glPopMatrix();
#endif
	}

	/*
	if (activeAura == AURA_SHIELD)
	{
		glPushMatrix();
		glColor4f(0,0.5,1,1);
		glTranslatef(shieldPosition.x, shieldPosition.y, 0);
		drawCircle(AURA_SHIELD_RADIUS, 8);
		glPopMatrix();
	}
	*/

}

void Avatar::onRender()
{
	Entity::onRender();

	/*
	// HACK
	if (RenderObject::renderPaths)
	{
		glPushMatrix();
			glLoadIdentity();
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1, 0, 0, 0.5);
			glTranslatef(400, 300, 0);
			drawCircle(64);
		glPopMatrix();
	}
	*/

	//dsq->print(20, 600-100, "Naija: Hello there. My name is fred.");
	/*
	std::ostringstream os;
	os << lastPush.x << ", " << lastPush.y;
	debugLog(os.str());
	*/
	/*
	glPopMatrix();
	glPushMatrix();
	glTranslatef(position.x, position.y, position.z);
	//glRotatef(0, 0, 1, -rotation.z);
	glDisable(GL_BLEND);
	glPointSize(12);
	glDisable(GL_LIGHTING);
	glColor3f(1,0,0);
	glBegin(GL_LINES);
		//glColor3f(1, 0, 0);
		glVertex3f(0,0,0);
		//glColor3f(1, 0, 0);
		glVertex3f(lastPush.x*50, lastPush.y*50, 0);
	glEnd();
	glPopMatrix();
	*/
}

int Avatar::getBeamWidth()
{
	const int MAX_BEAM_LEN = 50;
	Vector mov = dsq->getGameCursorPosition() - this->position;
	mov.setLength2D(1);
	TileVector t(position);
	Vector tile(t.x, t.y);
	int c = 0;
	while (c < MAX_BEAM_LEN)
	{
		bool hit = false;
		tile += mov;
		TileVector t;
		t.x = int(tile.x);
		t.y = int(tile.y);
		if (dsq->game->isObstructed(t))
		{
			hit = true;
		}

		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e != this)
			{
				TileVector et(e->position);
				Vector t1(et.x, et.y);
				Vector t2(tile.x, tile.y);
				Vector diff = t1-t2;
				if (diff.getSquaredLength2D() <= 1)
				{
					// HACK: replace damage function
					//e->damage(1, 0, this);
					hit = true;
				}
			}
		}
		if (hit)
			break;
		c++;
	}
	return c * TILE_SIZE;
}

void Avatar::onEnterState(int action)
{
	Entity::onEnterState(action);
	if (action == STATE_TRANSFORM)
	{
		animator.stop();
		frame = 0;
		animate(anim_fish);
	}
	else if (action == STATE_PUSH)
	{
		state.lockedToWall = false;
		state.crawlingOnWall = false;
		Animation *a = skeletalSprite.getCurrentAnimation();
		if (!a || (a && a->name != "pushed"))
			skeletalSprite.animate("pushed", 0);
	}
	else if (action == STATE_EATING)
	{
		/*
		// the problems with this:
		// 1. what happens out of water?
		// 2. is the delay too long?
		// could just play a sound and then spawn some kind of particle effect
		disableInput();
		idle();
		vel=vel2=Vector(0,0);
		skeletalSprite.animate("eat", 0, ANIM_OVERRIDE);
		core->main(1);
		enableInput();
		*/
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
		skeletalSprite.transitionAnimate("spin", 0.1);
		/*
		rotation.z = rotation.z+360;
		rotation.interpolateTo(Vector(0,0,rotation.z-360-90), 0.5);
		*/
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
		int freq = 1000;
		if (rolling)
			freq = 900;
		sound("splash-into", freq);
		//dsq->postProcessingFx.disable(FXT_RADIALBLUR);
		if (_isUnderWater && core->afterEffectManager)
			core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08,0.05,22,0.2f, 1.2));
		dsq->rumble(0.7, 0.7, 0.2);
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
	hsplash.y = dsq->game->waterLevel.x;
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


	//Vector(position.x, dsq->game->waterLevel.x));
	/*
	Quad *splash = new Quad;
	splash->setTexture("splash");
	splash->position = this->position;
	splash->position.y = dsq->game->waterLevel-splash->width.x/2;
	float t = 0.5;
	splash->alpha.path.addPathNode(0, 0);
	splash->alpha.path.addPathNode(1, 0.25);
	splash->alpha.path.addPathNode(0, 1);
	splash->alpha.startPath(t);
	splash->scale = Vector(1.5, 0.9);
	splash->scale.interpolateTo(Vector(0.5,1.2),t);
	splash->setLife(1);
	splash->setDecayRate(0.9);
	core->getTopStateData()->addRenderObject(splash, LR_PARTICLES);
	*/
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


	setMaxSpeed(currentMaxSpeed * useSpeedMult);
	//float cheatLen = vel.getSquaredLength2D();
	vel.capLength2D(getMaxSpeed());
	/*
	if (cheatLen > sqr(getMaxSpeed()))
		vel.setLength2D(getMaxSpeed());
	*/
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
				//&& (*i)->life > 0.2f
				if ((*i) && dsq->game->isDamageTypeEnemy((*i)->getDamageType()) && (*i)->firer != this
					&& (!(*i)->shotData || !(*i)->shotData->ignoreShield))
				{

					Vector diff = (*i)->position - shieldPosition;
					if (diff.getSquaredLength2D() < sqr(AURA_SHIELD_RADIUS))
					{
						shieldPoints -= (*i)->getDamage();
						auraHitEmitter.start();
						dsq->spawnParticleEffect("ReflectShot", (*i)->position);
						core->sound->playSfx("Shield-Hit");
						(*i)->position += diff;
						//(*i)->target = 0;
						diff.setLength2D((*i)->maxSpeed);
						(*i)->velocity = diff;
						(*i)->reflectFromEntity(this);
					}
				}
			}
		}
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

void Avatar::chargeVisualEffect(const std::string &tex)
{
	float time = 0.4;
	Quad *chargeEffect = new Quad;
	chargeEffect->setBlendType(BLEND_ADD);
	chargeEffect->alpha.ensureData();
	chargeEffect->alpha.data->path.addPathNode(0, 0);
	chargeEffect->alpha.data->path.addPathNode(0.6, 0.1);
	chargeEffect->alpha.data->path.addPathNode(0.6, 0.9);
	chargeEffect->alpha.data->path.addPathNode(0, 1.0);
	chargeEffect->alpha.startPath(time);
	chargeEffect->setTexture(tex);
	//chargeEffect->positionSnapTo = &this->position;
	chargeEffect->position = this->position;
	chargeEffect->setPositionSnapTo(&position);
	chargeEffect->setLife(1);
	chargeEffect->setDecayRate(1.0f/time);
	chargeEffect->scale = Vector(0.1, 0.1);
	chargeEffect->scale.interpolateTo(Vector(1,1),time);
	//chargeEffect->rotation.interpolateTo(Vector(0,0,360), time);
	dsq->game->addRenderObject(chargeEffect, LR_PARTICLES);
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
		skeletalSprite.position = bodyPosition;
	break;
	}
}

void Avatar::stopBurst()
{
	burst = 0;
	//burstDelay = BURST_DELAY;
	burstDelay = 0;
	bursting = false;
	animatedBurst = false;
	wakeEmitter.stop();
	ripples = false;

	biteLeftEmitter.stop();
	biteRightEmitter.stop();
//	lastWallJumpPos = Vector(0,0,0);
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
	//if (lastQuad==0) return 0;
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
	//debugLog("start roll!");
	if (!a || a->name != getRollAnimName())
	{
		skeletalSprite.transitionAnimate(getRollAnimName(), 0.2, -1);
	}
	/*
	rollRightEmitter.load("RollRight");
	rollLeftEmitter.load("RollLeft");

	rollRightEmitter.start();
	rollLeftEmitter.start();
	*/
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
		core->sound->fadeSfx(dsq->loops.roll, SFT_OUT, 0.5);
	}
	/*
	//HACK: make this dt based
	static int rollBits = 0;
	rollBits = rollBits + 1;
	if (rollBits > 6)
	{
		if (_isUnderWater)
			core->sound->playSfx("Roll2");
		rollBits = 0;
	}
	*/
	rollDir = dir;

	/*
	if (core->afterEffectManager)
		core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.04,0.06,15,0.2f));
	*/
	if (_isUnderWater && core->afterEffectManager)
		core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.08,0.05,22,0.2f, 1.2));


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
	wallJumps = 0;
	lastWallJumpPos = Vector(0,0,0);
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
	if (!inputEnabled || dsq->continuity.getWorldType() == WT_SPIRIT)
	{
		if (rolling)
			stopRoll();
		return;
	}
	if (state.lockedToWall || isSinging()) return;

	if (rollDelay > 0)
	{
		/*
		std::ostringstream os;
		os << "rollDelay: " << rollDelay;
		debugLog(os.str());
		*/
		rollDelay -= dt;
		if (rollDelay <= 0)
		{
			// stop the animation
			stopRoll();
		}
	}
	
	if (!_isUnderWater && isActing(ACTION_ROLL))
	{
		stopRoll();
	}

	if (!core->mouse.buttons.left && dsq->inputMode == INPUT_MOUSE && !isActing(ACTION_ROLL))
	{
		if (rolling)
			stopRoll();
		return;
	}

	if (rolling)
	{
		/*
		FOR_ENTITIES (i)
		{
			Entity *e = *i;
			if (e->getEntityType() == ET_ENEMY && (e->position - this->position).isLength2DIn(350))
			{
				//e->move(dt, 500, 1, this);
				Vector diff = (position - e->position);
				diff.setLength2D(1000*dt);
				e->vel2 += diff;
			}
		}
		*/
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

		/*
		//HACK: -ish, fixes low frame rate roll stuck problem?
		// nope
		if (rollDelay == 0)
		{
			rollDelay = 0.01;
		}
		*/
		// NOTE: does this fix the roll problem?
		if (rollDelay <= 0)
			stopRoll();
	}
	
	if (isActing(ACTION_ROLL))
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
			/*
			std::ostringstream os;
			os << "quad: " << quad << " lastQuad: " << lastQuad << " lastQuadDir: " << lastQuadDir;
			debugLog(os.str());
			*/


			/*
			if (lastQuad != 0)
			{
				lastQuadDir = quadDir;
			}
			*/

			lastQuadDir = quadDir;

			lastQuad = quad;

			rollDelay = 0.2;
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

void Avatar::updateTummy(float dt)
{
	if (dsq->continuity.form == FORM_BEAST)
	{
		//dsq->shakeCamera(5, 0.1);

		/*
		if (inTummy > 0)
		{
			tummyTimer += dt;
			if (tummyTimer > TUMMY_TIME)
			{
				//core->sound->playSfx("Digest");
				//heal(inTummy);
				inTummy = 0;
			}
		}
		*/

	}
}

void Avatar::setWasUnderWater()
{
	state.wasUnderWater = isUnderWater();
}

bool Avatar::canActivateStuff()
{
	return dsq->continuity.form != FORM_SPIRIT;
}

bool Avatar::canQuickSong()
{
	return !isSinging() && !isEntityDead() && isInputEnabled() && quickSongCastDelay <= 0;
}

void Avatar::updateJoystick(float dt)
{
	if (canQuickSong())
	{

		if (core->joystick.dpadUp)
		{
			if (dsq->continuity.hasSong(SONG_ENERGYFORM) && dsq->continuity.form != FORM_ENERGY)
			{
				quickSongCastDelay = QUICK_SONG_CAST_DELAY;
				dsq->continuity.castSong(SONG_ENERGYFORM);
			}
		}
		else if (core->joystick.dpadDown && dsq->continuity.hasSong(SONG_BEASTFORM) && dsq->continuity.form != FORM_BEAST)
		{
			quickSongCastDelay = QUICK_SONG_CAST_DELAY;
			dsq->continuity.castSong(SONG_BEASTFORM);
		}
		else if (core->joystick.dpadLeft && dsq->continuity.hasSong(SONG_SUNFORM) && dsq->continuity.form != FORM_SUN)
		{
			quickSongCastDelay = QUICK_SONG_CAST_DELAY;
			dsq->continuity.castSong(SONG_SUNFORM);
		}
		else if (core->joystick.dpadRight && dsq->continuity.hasSong(SONG_NATUREFORM) && dsq->continuity.form != FORM_NATURE)
		{
			quickSongCastDelay = QUICK_SONG_CAST_DELAY;
			dsq->continuity.castSong(SONG_NATUREFORM);
		}

	}
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
	if (dsq->game->isShuttingDownGameState()) return;
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
			//if (dsq->continuity.form == FORM_NORMAL)
			setHeadTexture("blink", 0.1);
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
		const float lookAtTime = 0.8;
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
					/*
					std::ostringstream os;
					os << "rotationOffset lerp " << bone_head->rotationOffset.z;
					debugLog(os.str());
					*/
				}

				/*
				bone_head->rotationOffset = bone_head->rotation.z;
				bone_head->rotation.z = 0;
				bone_head->rotationOffset.interpolateTo(0, 0.2);
				*/
				//state.updateLookAtTime += dt*10;
				//state.updateLookAtTime += dt;
				state.updateLookAtTime += dt*4*2;
				bone_head->internalOffset.interpolateTo(Vector(0,0), 0.2);
			}

			if (state.updateLookAtTime > 1.5f)
			{
				state.lookAtEntity = dsq->game->getNearestEntity(position, 800, this, ET_NOTYPE, DT_NONE, LR_ENTITIES0, LR_ENTITIES2);
				if (state.lookAtEntity && state.lookAtEntity->isv(EV_LOOKAT, 1))
				{
					/*
					std::ostringstream os;
					os << "Nearest: " << state.lookAtEntity->name;
					debugLog(os.str());
					*/

					state.updateLookAtTime = 0;
					//if (dsq->continuity.form == FORM_NORMAL)
					//setHeadTexture("blink", 0.1);

					/*
					if (state.lookAtEntity->getEntityType() == ET_NEUTRAL)
					{
						//if (dsq->continuity.form == FORM_NORMAL)
						//setHeadTexture("smile", 1);
					}
					*/

					if (!state.lookAtEntity->naijaReaction.empty())
					{
						setHeadTexture(state.lookAtEntity->naijaReaction, 1.5);
					}
				}
				else
				{
					state.lookAtEntity = 0;
					/*
					std::ostringstream os;
					os << state.updateLookAtTime << " : found no entities";
					debugLog(os.str());
					*/
					//state.updateLookAtTime -= 0.3f;
				}

				//skeletalSprite.animate("blink", 2, ANIMLAYER_HEADOVERRIDE);
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

bool Avatar::isMiniMapCursorOkay()
{
//!(dsq->getMouseButtonState(0) || dsq->getMouseButtonState(1))
	return ((dsq->inputMode != INPUT_MOUSE) ||  (!dsq->game->miniMapRender || !dsq->game->miniMapRender->isCursorIn()));
}

void Avatar::updateCursorFromKeyboard()
{
	/*
	// why return when singing??
	//if (isSinging()) return;
	if (!isInputEnabled()) return;

	Vector diff;
	int dist = 200;
	if (isActing(ACTION_SINGLEFT))
		diff.x = -dist;
	if (isActing(ACTION_SINGRIGHT))
		diff.x = dist;
	if (isActing(ACTION_SINGUP))
		diff.y = -dist;
	if (isActing(ACTION_SINGDOWN))
		diff.y = dist;
	if (!diff.isZero())
	{
		diff.setLength2D(dist);
		core->mouse.position = Vector(400,300) + diff;
		lastCursorKeyboard = true;
	}
	else if (lastCursorKeyboard)
	{
		debugLog("HEY!: lastCursorKeyboard mouse position reset");
		core->mouse.position = Vector(400,300);
		lastCursorKeyboard = false;
		dsq->toggleCursor(false, 0.2);
	}

	//!diff.isZero()  || 
	if (isInputEnabled() && (!core->mouse.change.isZero()))
	{
		dsq->toggleCursor(true, 0.2);
	}
	*/
}

void Avatar::onUpdate(float dt)
{
	BBGE_PROF(Avatar_onUpdate);

	// animation debug code
	/*
	for (int i = 0; i < 8; i++)
	{
		if (skeletalSprite.getAnimationLayer(i))
		{
			if (skeletalSprite.getAnimationLayer(i)->isAnimating())
			{
				std::ostringstream os;
				os << "anim layer: " << i << " - " << skeletalSprite.getAnimationLayer(i)->getCurrentAnimation()->name;
				debugLog(os.str());
				//debugLog("anim layer 0: " + skeletalSprite.getAnimationLayer(0)->getCurrentAnimation()->name);
			}
		}
	}
	*/

	looking = 0;

#ifdef AQ_TEST_QUADTRAIL
	quadTrail->addPoint(position);
#endif

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
	if (activateEntity)
	{
		activateEntity->activate();
		activateEntity = 0;
	}
	if (bone_head)
		headPosition = bone_head->getWorldPosition();

	//vel /= 0;
	if (vel.isNan())
	{
		debugLog("detected velocity NaN");
		vel = Vector(0,0);
	}

	if (canWarpDelay > 0)
	{
		canWarpDelay = canWarpDelay - dt;
		if (canWarpDelay < 0)
		{
			canWarp = true;
			canWarpDelay = 0;
		}
	}

	if (fireDelay > 0)
	{
		fireDelay -= dt;
		if (fireDelay < 0)
		{
			fireDelay = 0;
		}
	}

	if (doubleClickDelay > 0)
	{
		doubleClickDelay = doubleClickDelay - dt;
		if (doubleClickDelay < 0) doubleClickDelay = 0;
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
		
		if (!dsq->game->isPaused() && isActing(ACTION_LOOK) && !dsq->game->avatar->isSinging() && dsq->game->avatar->isInputEnabled() && !dsq->game->isInGameMenu())
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



	// setup shader
	if (core->afterEffectManager)
	{

		/*
		if (!_isUnderWater)
		{
			core->afterEffectManager->setActiveShader(AS_WASHOUT);
			//core->afterEffectManager->setActiveShader(AS_NONE);
		}
		else
		*/
		if (dsq->user.video.shader != AS_NONE)
		{
			core->afterEffectManager->setActiveShader((ActiveShader)dsq->user.video.shader);
		}
		else
		{
			if (damageTimer.isActive() && dsq->isShakingCamera())
			{
				if (dsq->user.video.blur)
					core->afterEffectManager->setActiveShader(AS_BLUR);
			}
			else
			{
				core->afterEffectManager->setActiveShader(AS_NONE);
			}

		}
	}

	/*
	if (!targets.empty())
	{
		if (targets[0] && (targets[0]->position - this->position).getSquaredLength2D() > sqr(TARGET_RANGE))
		{
			clearTargets();
		}
	}
	*/
	//spawnChildClone(4);
	if (!core->cameraRot.isInterpolating())
		// 10
		core->cameraRot.interpolateTo(Vector(0,0,360), 30, -1);
	/*
	for (int i = 0; i < targets.size(); i++)
	{
		if (targets[i] && !this->isEntityDead())
		{
			targetQuads[i]->alpha.interpolateTo(1,0.1);
			targetQuads[i]->position = targets[i]->position;
		}
		else
		{
			if (targetQuads[i]->alpha.getValue()>0)
				targetQuads[i]->alpha.interpolateTo(0,0.1);
		}
	}
	*/

	Entity::onUpdate(dt);

	if (isEntityDead() && skeletalSprite.getCurrentAnimation()->name != "dead")
	{
		fallOffWall();
		biteLeftEmitter.stop();
		biteRightEmitter.stop();
		wakeEmitter.stop();
		rollLeftEmitter.stop();
		rollRightEmitter.stop();
		dsq->game->toggleOverrideZoom(false);
		if (dsq->continuity.form != FORM_NORMAL)
			changeForm(FORM_NORMAL);
		setHeadTexture("Pain");
		core->globalScale.interpolateTo(Vector(5,5),3);
		rotation.interpolateTo(Vector(0,0,0), 0.1);
		skeletalSprite.animate("dead");
	}
	if (isEntityDead())
	{
		dsq->game->toggleOverrideZoom(false);
	}

	if (dsq->user.control.targeting)
		updateTargets(dt, false);
	else
		targets.clear();

	updateTargetQuads(dt);

	updateDualFormGlow(dt);
	updateLookAt(dt);

	updateFoodParticleEffects();

	if (!dsq->game->isPaused())
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
				if (!dsq->game->waterLevel.isInterpolating())
				{
					if (vel.y < 0)
						vel.y = -vel.y*0.5f;
					position.y = dsq->game->waterLevel.x + collideRadius;
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
			BBGE_PROF(Avatar_splashOut);
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
			if (!dsq->mod.isActive() && dsq->continuity.getFlag("leftWater")==0 && dsq->game->sceneName.find("veil")!=std::string::npos)
			{
				setInvincible(true);
				setv(EV_NOINPUTNOVEL, 0);

				setWasUnderWater();

				if (vel.y > -500)
					vel.y = -500;
				dsq->continuity.setFlag("leftWater", 1);

				core->sound->fadeMusic(SFT_OUT, 2);
				//("Veil");
				dsq->game->avatar->disableInput();
				dsq->gameSpeed.interpolateTo(0.1, 0.5);

				//dsq->sound->setMusicFader(0.5, 0.5);
				core->sound->playSfx("NaijaGasp");
				core->main(0.75);

				

				dsq->voiceOnce("Naija_VeilCrossing");
				core->main(10*0.1f);

				dsq->gameSpeed.interpolateTo(1, 0.2);

				dsq->sound->playMusic("Veil", SLT_LOOP, SFT_CROSS, 20);

				//dsq->sound->setMusicFader(1.0, 1);

				dsq->game->avatar->enableInput();

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

	//skeletalSprite.getBoneByIdx(3)->getWorldPosition();

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

	/*
	if (core->afterEffectManager && _isUnderWater)
	{
		if (swimming && vel.getSquaredLength2D() > sqr(200))
		{
			rippleTimer += dt;
			while (rippleTimer > RIPPLE_INTERVAL)
			{
				// 0.01   20
				core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),core->screenCenter,0.05,0.08,15,0.2f, 1.2));

				//core->afterEffectManager->addEffect(new ShockEffect(Vector(400,300),0.01,0.002f,15,0.1f));
				rippleTimer = 0;
			}
		}
	}
	*/


	/*
	bobTimer += dt*2;
	offset.y = sinf(bobTimer)*5 - 2.5f;
	*/

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
	updateJoystick(dt);

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
				core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),position+offset,0.04,0.06,15,0.2f));
			rippleDelay = 0.15;
		}
	}

	if (dsq->continuity.tripTimer.isActive())
	{
		static int tripCount = 0;
		tripDelay -= dt;
		if (tripDelay < 0)
		{
;
			tripDelay = 0.15;
			tripCount ++;
			if (tripCount > 10)
			{
				/*
				// hacktastic
				EMOTE_NAIJAEVILLAUGH	= 0
				EMOTE_NAIJAGIGGLE		= 1
				EMOTE_NAIJALAUGH		= 2
				EMOTE_NAIJASADSIGH		= 3
				EMOTE_NAIJASIGH			= 4
				EMOTE_NAIJAWOW			= 5
				EMOTE_NAIJAUGH			= 6
				*/
				float p = dsq->continuity.tripTimer.getPerc();
				if (p > 0.6f)
				{
					if (core->afterEffectManager)
						core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),position+offset,0.04,0.06,15,0.2f));
				}
				else
				{
					if (core->afterEffectManager)
						core->afterEffectManager->addEffect(new ShockEffect(Vector(core->width/2, core->height/2),position+offset,0.4,0.6,15,0.2f));
				}
				if (p > 0.75f){}
				else if (p > 0.5f)
				{
					dsq->shakeCamera(2, 4);
					if (chance(80))
					{
						if (chance(60))
							dsq->emote.playSfx(2);
						else
							dsq->emote.playSfx(0);
					}
				}
				else
				{
					if (p < 0.2f)
						dsq->shakeCamera(10, 4);
					else
						dsq->shakeCamera(5, 4);
					tripper->color.interpolateTo(Vector(1, 0.2, 0.2), 3);
					if (chance(75))
						dsq->emote.playSfx(6);
				}

				tripCount = 0;
			}
		}
	}

	if (position.isInterpolating())
	{
		lastPosition = position;
	}


	updateCursorFromKeyboard();
	updateFormVisualEffects(dt);
	updateShock(dt);
	updateRoll(dt);
	updateTummy(dt);
	updateWallJump(dt);

	if (dsq->autoSingMenuOpen)
	{
		if (micNote != -1 && !this->isSinging())
		{
			openedFromMicInput = true;
			openSingingInterface();
		}
		if (micNote == -1 && isSinging() && openedFromMicInput)
		{
			openedFromMicInput = false;
			closeSingingInterface();
		}
	}

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
	switch(ropeState)
	{
	case 1:
	{
		if (ropeTimer > 0)
		{
			ropeTimer -= dt;
			if (ropeTimer < 0)
			{
				ropeState = 0;
				ropeTimer = 0;
			}
		}
		ropePos += ropeVel*dt;
		if (dsq->game->isObstructed(TileVector(ropePos)))
		{
			ropeState = 2;
			ropeTimer = 2;
		}
		std::ostringstream os;
		os << "ropePos (" << ropePos.x << ", " << ropePos.y << ")";
		debugLog(os.str());
	}
	break;
	case 2:
		if (ropeTimer > 0)
		{
			ropeTimer -= dt;
			if (ropeTimer < 0)
			{
				ropeState = 0;
				ropeTimer = 0;
			}
		}
		Vector add = (ropePos - position);
		if (add.getSquaredLength2D() > sqr(200))
		{
			add.setLength2D(4000);
			vel += add*dt;
		}
	break;
	}


	updateSingingInterface(dt);

	if (pullTarget)
	{
		if (pullTarget->life < 1 || !pullTarget->isPullable())
		{
			pullTarget->stopPull();
			pullTarget = 0;
		}
		else
		{
			/*
			static float c = 0;
			c += dt;
			if (c > 0.2f)
			{
				EnergyTendril *t = new EnergyTendril(avatar->position, pullTarget->position);
				core->addRenderObject(t, LR_PARTICLES);
				c -= dt;
			}
			*/
		}
	}

	formTimer += dt;

	/*
	if (formTimer > 2.0f && dsq->continuity.form == FORM_SPIRIT)
	{
		changeForm(FORM_NORMAL, true);
	}
	*/
	if (pickingPullTarget)
	{
		//debugLog("picking pull target");
		Entity *closest = 0;
		float smallestDist = HUGE_VALF;
		FOR_ENTITIES(i)
		{
			Entity *e = *i;

			if (e->isPullable() && e->life == 1)
			{
				if (e->isCoordinateInside(dsq->getGameCursorPosition()))
				{
					float dist = (e->position - dsq->getGameCursorPosition()).getSquaredLength2D();
					if (dist < smallestDist)
					{
						smallestDist = dist;
						closest = e;
					}
				}
			}
		}
		potentialPullTarget = closest;
	}

	if (dsq->continuity.form == FORM_SPIRIT)
	{
		if (useSpiritDistance)
		{
			if (formTimer > 1)
			{
				if (!(bodyPosition - position).isLength2DIn(SPIRIT_RANGE))
				{
					changeForm(FORM_NORMAL);
				}
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
	if (inputEnabled && (dsq->inputMode == INPUT_KEYBOARD || dsq->inputMode == INPUT_MOUSE) && (!pathToActivate && !entityToActivate))
	{
		//debugLog("update stuff");
		///*&& dsq->continuity.form != FORM_SPIRIT*/
		//|| isActing(ACTION_PRIMARY)
		//|| isActing(ACTION_SECONDARY)
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

	/*
	if (this->state.crawlingOnWall)
	{
		if (isActing("a1"))
		{
			wallNormal = dsq->game->getWallNormal(position);
			if (wallNormal.dot2D(lastWallNormal)<=0.3f)
			{
				stopWallCrawl();
			}
			else
			{
				Vector left = wallNormal.getPerpendicularLeft();
				Vector right = wallNormal.getPerpendicularRight();


				Vector test = getVectorToCursor();
				if (!test.isLength2DIn(64))
				{
					test.normalize2D();

					Vector move;
					if (test.dot2D(left)>0)
						move = left;
					else
						move = right;

					move.setLength2D(800);

					if (move.x > 0 && !isfh())
						flipHorizontal();
					if (move.x < 0 && isfh())
						flipHorizontal();

					position += move*dt;
					rotateToVec(wallNormal, 0.1);
				}
				else
				{
					stopWallCrawl();
				}
			}
		}
		else
		{
			stopWallCrawl();
		}
	}
	*/

	//if (core->getNestedMains() == 1)
	{
		if (leaches > 3)
		{
			/*
			const float leachHurtInterval = 3;
			state.leachTimer += dt;
			if (state.leachTimer > leachHurtInterval)
			{
				state.leachTimer -= leachHurtInterval;
				DamageData d;
				d.damage = int(leaches/3);
				damage(d);
				//hit(0, 0, SPELL_NONE, int(leaches/3));
			}
			*/
		}


		if (getState() != STATE_TRANSFORM && dsq->continuity.getWorldType() == WT_NORMAL)
		//if (dsq->continuity.form == FORM_ENERGY)
		{
			//if (dsq->continuity.selectedSpell == SPELL_SHOCK)

			formAbilityUpdate(dt);

			// is this really necessary??
			// YES!
			// this allows the player to start charging quickly after firing
			/*
			if (isActing("charge") && !charging && spellCastDelay == 0 && inputEnabled)
			{
				this->rmbd();
			}
			*/
			// maybe not useful anymore
			/*
			if (!isActing("charge") && charging && spellChargeDelay == 0 && inputEnabled)
			{
			}
			*/
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

			 /*&& this->getSelectedSpell() == SPELL_ENERGYBLAST*/
			/*
			// HACK: hacked out for now
			// FINDTARGET

			if (charging && !targets.empty() && targets[0] == 0 && state.spellCharge > 0.2f
				&& dsq->continuity.form == FORM_ENERGY)
			{
				FOR_ENTITIES (i)
				{
					Entity *e = *i;
					if (e && e != this && e->isAvatarAttackTarget() && !e->isEntityDead() && e->isAffectedBySpell(SPELL_ENERGYBLAST))
					{
						ScriptedEntity *se = (ScriptedEntity*)e;
						if ((e->position - dsq->getGameCursorPosition()).getSquaredLength2D() < sqr(64))
						{
							targets[0] = e;
						}
					}
				}
			}
			*/
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
					lockToWallFallTimer = 0.4;
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

		if (spellCastDelay > 0)
		{
			spellCastDelay -= dt;
			if (spellCastDelay <= 0)
			{
				spellCastDelay = 0;
			}
		}

		if (state.lockToWallDelay.updateCheck(dt))
		{
		}

		if (state.tapTimer.updateCheck(dt))
		{
			tapped = "";
		}
		if (pushingOffWallEffect > 0)
		{
			pushingOffWallEffect -= dt;
			if (pushingOffWallEffect <= 0)
			{
				lastLockToWallPos = Vector(0,0);
				pushingOffWallEffect = 0;
				if (vel.getSquaredLength2D() > sqr(1200))
				{
					vel.setLength2D(1200);
				}
			}
		}
		/*
		if (beamFiring)
		{
			// collides enemies with beam as well
			beam->width = getBeamWidth();
			Vector diff = dsq->getGameCursorPosition() - this->position;
			diff |= beam->width.getValue()/2.0f;
			beam->position = this->position + diff;
			float angle=0;
			MathFunctions::calculateAngleBetweenVectorsInDegrees(this->position, dsq->getGameCursorPosition(), angle);
			beam->rotation.z = angle+90;
			beam->position.z = 3;
			//collideBeamWithEntities();
		}
		*/
		if (state.dodgeEffectTimer.updateCheck(dt))
		{
			vel.capLength2D(vars->maxSwimSpeed);
			/*
			if (vel.getSquaredLength2D() > sqr(vars->maxSwimSpeed))
				vel.setLength2D(vars->maxSwimSpeed);
			*/
		}
		/*
		if (dodgeEffectTimer > 0)
		{
			dodgeEffectTimer -= dt;
			if (dodgeEffectTimer <= 0)
			{
				dodgeEffectTimer = 0;
				if (vel.getSquaredLength2D() > sqr(vars->maxSwimSpeed))
					vel |= vars->maxSwimSpeed;
			}
		}
		*/
		if (dodgeDelay > 0)
		{
			dodgeDelay -= dt;
			if (dodgeDelay <= 0)
			{
				dodgeDelay = 0;
			}
		}

		if (text)
		{
			text->position = position + Vector(100);
		}

		if (charging)
		{
			/*
			chargeGraphic->position = this->position;
			chargeGraphic->position.z = position.z + 0.05f;
			*/
			state.spellCharge += dt;
			switch (dsq->continuity.form)
			{
			case FORM_SUN:
			{
				if (state.spellCharge > 1.5f && chargeLevelAttained <1)
				{
					chargeLevelAttained = 1.5;
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



					//chargeVisualEffect("particles/energy-charge-2");
				}
				/*
				if (state.spellCharge >= 1.5f && chargeLevelAttained<2)
				{
					chargeLevelAttained = 2;

					core->sound->playSfx("PowerUp");
					//debugLog("charge visual effect 2");
					chargeEmitter->load("EnergyCharge");
					chargeEmitter->start();

					//chargeVisualEffect("particles/energy-charge-2");
				}
				*/
			}
			break;
			case FORM_ENERGY:
			{
				/*
				if (state.spellCharge >= 0.99f && chargeLevelAttained<1)
				{
					chargeLevelAttained = 1;
					debugLog("charge visual effect 1");
					chargeVisualEffect("energy-charge-1");

				}
				*/
				if (state.spellCharge >= 1.5f && chargeLevelAttained<2)
				{
					chargeLevelAttained = 2;
					core->sound->playSfx("PowerUp");
					//debugLog("charge visual effect 2");
					chargeEmitter->load("ChargeEnergy");
					chargeEmitter->start();


					chargingEmitter->load("ChargingEnergy2");
					//chargeVisualEffect("particles/energy-charge-2");
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
				/*
				if (state.spellCharge >= 0.5f && chargeLevelAttained<1)
				{
					chargeLevelAttained = 1;
					core->sound->playSfx("PowerUp");
					chargeEmitter->load("ChargeNature");
					chargeEmitter->start();
				}

				if (state.spellCharge >= 2.0f && chargeLevelAttained<2)
				{
					chargeLevelAttained = 2;
					core->sound->playSfx("PowerUp");
					chargeEmitter->load("ChargeNature2");
					chargeEmitter->start();

					chargingEmitter->load("ChargingNature2");
					chargingEmitter->start();
				}
				*/
			}
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
						urchinDelay = 0.1;

						dsq->game->fireShot("urchin", this, 0, position + offset);
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

							dsq->game->spawnManaBall(position + offset + d, JELLYCOSTUME_HEALAMOUNT);

							//Shot *s = dsq->game->fireShot("urchin", this, 0, getWorldPosition());
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
				dsq->game->fireShot(shot, this, 0, p);
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
					dsq->game->fireShot("FishPoison", this, 0, position);
				}
			}
		}

		if (!(state.lockedToWall || state.dodgeEffectTimer.isActive()) && _isUnderWater && dsq->continuity.getWorldType() == WT_NORMAL && canMove)
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
			if (bursting)
			{
				// disable check to stop burst
				/*
				if (!isActing("a1"))
				{
					stopBurst();
					//bursting = false;
					//burstDelay = BURST_DELAY;
					//animatedBurst = false;
				}
				*/
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
		if (state.lockedToWall && !state.crawlingOnWall)
		{
			rotateToVec(wallPushVec, dt*2);
			if (!boneLock.on && !dsq->game->isObstructed(wallLockTile))
			{
				//debugLog("Dropping from wall");
				fallOffWall();
			}
		}

		if (getState() != STATE_PUSH && !state.lockedToWall && inputEnabled && !ignoreInputDelay && _isUnderWater && canMove)
		{
			float a = 800*dt;
			Vector lastVel = vel;
			Vector addVec;

			bool isMovingSlow = false;
			static Vector lastMousePos;
			Vector pos = lastMousePos - dsq->getGameCursorPosition();
			static bool lastDown;

			float len = 0;
			//dsq->continuity.toggleMoveMode &&
			//!dsq->continuity.toggleMoveMode &&
			
			if (isMiniMapCursorOkay() && !isActing(ACTION_ROLL) &&
				_isUnderWater && !riding && !boneLock.on &&
				(movingOn || ((dsq->inputMode == INPUT_JOYSTICK || dsq->inputMode== INPUT_KEYBOARD) || (core->mouse.buttons.left || bursting))))
			{
				//addVec = getVectorToCursorFr
				//(dsq->inputMode != INPUT_JOYSTICK && dsq->inputMode != INPUT_KEYBOARD)
				if (dsq->inputMode == INPUT_MOUSE || !this->singing)
				{
					addVec = getVectorToCursorFromScreenCentre();//getVectorToCursor();

					if (dsq->inputMode == INPUT_MOUSE)
					{
						static Vector lastAddVec;
						if (!isActing(ACTION_PRIMARY) && bursting)
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
						if (dsq->inputMode == INPUT_JOYSTICK)
							addVec = Vector(0,0,0);
						/*
						if (dsq->inputMode == INPUT_JOYSTICK && !core->mouse.buttons.left)
						{
							addVec = Vector(0,0,0);
						}
						*/
					}



					/*
					if (!core->mouse.buttons.left && bursting)
					{
						addVec = vel;
					}
					*/

					if (!addVec.isLength2DIn(minMouse))
					{
						//if (core->mouse.buttons.left)
						{
							len = addVec.getLength2D();
							// addVec is always overwritten below; I assume this is old code?  --achurch
							//if (len > 200)
							//	addVec.setLength2D(a *10);
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
						if ((dsq->inputMode == INPUT_MOUSE || isActing(ACTION_PRIMARY))
							&& addVec.isLength2DIn(STOP_DISTANCE))
						{
							vel *= 0.9f;
							if (!rolling)
								rotation.interpolateTo(Vector(0,0,0), 0.1);
							if (vel.isLength2DIn(50))
							{
								if (bursting)
								{
									stopBurst();
								}
							}
							//vel = Vector(0,0,0);
						}
						addVec = Vector(0,0,0);
					}
				}

				//addVec |= a;
				/*
				if (pos.getSquaredLength2D() > 10000)
				{
					startBurst();
				}
				*/
			}
			else
			{
			}
			lastDown = core->mouse.buttons.left;

			/*
			std::ostringstream os;
			os << "addVec(" << addVec.x << ", " << addVec.y << ")";
			debugLog(os.str());
			*/
			lastMousePos = dsq->getGameCursorPosition();

			if (!rolling && !state.backFlip && !flourish)
			{
				bool swimOnBack = false;
				if (swimOnBack)
				{
					if (addVec.x > 0)
					{
						if (isfh())
							flipHorizontal();
					}
					if (addVec.x < 0)
					{
						if (!isfh())
							flipHorizontal();
					}
				}
				else
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
			}


			/*
			// HACK: joystick code / slow
			if (addVec.x == 0 && addVec.y == 0)
			{
				float jpos[2];
				glfwGetJoystickPos(GLFW_JOYSTICK_1, jpos, 2);
				const float deadZone = 0.1;
				if (fabsf(jpos[0]) > deadZone || fabsf(jpos[1]) > deadZone)
					addVec = Vector(jpos[0]*a, -jpos[1]*a);
			}
			*/


			// will not get here if not underwater
			if (isLockable())
				lockToWall();
			if ((addVec.x != 0 || addVec.y != 0))
			{
				currentMaxSpeed=0;
				vel += addVec;
				//addVec |= a;
				//float cheatLen = vel.getSquaredLength2D();
				if (bursting)
				{
					Vector add = addVec;
					/*
					// HACK: this will let the player boost in one direction while turning to face another
					if (!core->mouse.buttons.left)
					{
						add = vel;
					}
					*/
					add.setLength2D(BURST_ACCEL*dt);
					vel += add;

					if (pushingOffWallEffect > 0 || wallJumps > 0)
						currentMaxSpeed = vars->maxWallJumpBurstSpeed + 50*wallJumps;
					else
						currentMaxSpeed = vars->maxBurstSpeed;

				}
				else
				{
					if (pushingOffWallEffect > 0)
						currentMaxSpeed = vars->maxWallJumpSpeed;
					else if (state.dodgeEffectTimer.isActive())
						currentMaxSpeed = vars->maxDodgeSpeed;
					else
					{
						if (isActing(ACTION_SLOW) || isMovingSlow)
						{
							/*
							int spdRange = maxMouse - minMouse;
							float p = (len - minMouse) / spdRange;
							int spd = p * vars->maxSwimSpeed;// + minMouse
							currentMaxSpeed = spd;
							*/
							currentMaxSpeed = vars->maxSlowSwimSpeed;
						}
						//else if (dsq->continuity.getWorldType() == WT_NORMAL)
						else
							currentMaxSpeed = vars->maxSwimSpeed;
						/*
						else
							currentMaxSpeed = vars->maxDreamWorldSpeed;
						*/
					}
				}

				/*
				if (dsq->continuity.form == FORM_SPIRIT)
					currentMaxSpeed *= 0.5f;
				*/

				if (leaches > 0)
				{
					currentMaxSpeed -= leaches*60;
				//	vel |= vel.getLength2D()-1*leaches;
				}

				if (state.blind)
					currentMaxSpeed -= 100;

				if (currentMaxSpeed < 0)
					currentMaxSpeed = 1;

				if (ropeState == 2 && currentMaxSpeed < vars->maxWallJumpBurstSpeed)
					currentMaxSpeed = vars->maxWallJumpBurstSpeed;

				/*
				if (inCurrent)
				{
					ropeState = 0;
					currentMaxSpeed = 1200;
				}
				*/

				//clampVelocity();


				//float angle;

				if (getState() == STATE_TRANSFORM)
					rotateToVec(addVec, 0.1, 90);
				else
				{
					if (rolling)
					{
						// here for roll key?
						// seems like this isn't reached
						//if (isActing("roll"))
						if (isActing(ACTION_ROLL))
						{
							//debugLog("here");
						}
						else
						{
							float t = 0;
							if (dsq->inputMode == INPUT_KEYBOARD)
								t = 0.1;
							rotateToVec(addVec, t);
						}
					}
					else if (bursting && flourish)
					{

					}
					else
					{
						/*
						if (bursting && !core->mouse.buttons.left)
						{
						}
						else
							rotateToVec(addVec, 0.1);
						*/
						if (!state.nearWall && !flourish)
							rotateToVec(addVec, 0.1);
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
							//animate(anim_burst);
						animatedBurst = true;
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
					rotateToVec(vel, 0.1);

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
				rotateToVec(v, 0.01);
			}
			if (isLockable())
				lockToWall();
			/*
			if (isActing("left"))
				addVec += Vector(-a, 0);
			if (isActing("right"))
				addVec += Vector(a, 0);
			*/
			/*
			if (isActing("up"))
				addVec += Vector(0, -a);
			if (isActing("down"))
				addVec += Vector(0, a);
			*/
			//vel += addVec;
		}

		if (!moved)
		{
			if (swimming)
			{
				swimming = false;
				idleAnimDelay = 0;
				if (dsq->continuity.form == FORM_FISH)
					rotation.interpolateTo(0, 0.2);
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

		if (ropeState == 2 && currentMaxSpeed < vars->maxWallJumpBurstSpeed)
			currentMaxSpeed = vars->maxWallJumpBurstSpeed;

		if (!state.lockedToWall && !bursting)
		{
			if (getState() == STATE_IDLE && inputEnabled)
			{
				Animation *a = skeletalSprite.getCurrentAnimation(0);
				if (a && a->name != getIdleAnimName() && a->name != "pushed" && a->name != "spin" && !rolling)
					skeletalSprite.transitionAnimate(getIdleAnimName(), ANIM_TRANSITION, -1);
			}
			/*
			idleAnimDelay -= dt;
			if (idleAnimDelay <= 0)
			{
				idleAnimDelay = 1.5;//anim_idle.time*2;

				//if (currentAction == IDLE && (!skeletalSprite.isAnimating() || skeletalSprite.getCurrentAnimation()->name=="swim"
				//	|| skeletalSprite.getCurrentAnimation()->name=="a1"))
				if (currentAction == STATE_IDLE)
				{
					skeletalSprite.transitionAnimate("idle", ANIM_TRANSITION);
				}

					//animate(anim_idle);
			}
			*/
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
		//static bool lastSpreadUp = false;
		if (!rolling && !internalOffset.isInterpolating())
		{
			int spread = 8;
			//int rotSpread = 45;
			float t = 1;

			internalOffset = Vector(-spread, 0);
			internalOffset.interpolateTo(Vector(spread, 0), t, -1, 1, 1);

			/*
			rotationOffset = Vector(-rotSpread, 0);
			rotationOffset.interpolateTo(Vector(rotSpread, 0), t, -1, 1, 1);
			*/

			for (int i = 0; i < int((t*0.5f)/0.01f); i++)
			{
				internalOffset.update(0.01);
				//rotationOffset.update(0.01);
			}
			/*
			if (lastSpreadUp)
				internalOffset.interpolateTo(Vector(spread, 0), t, 1, 1, 1);
			else
				internalOffset.interpolateTo(Vector(-spread, 0), t, 1, 1, 1);
			*/

			//lastSpreadUp = !lastSpreadUp;
			//internalOffset.update(t*0.5f);
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
					skeletalSprite.transitionAnimate(os.str(), 0.5, 0, 6);
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
		internalOffset.interpolateTo(Vector(0,0),0.5);
	}

	checkNearWall();
	//if (core->getNestedMains()==1)
	{
		Vector zoomSurface(0.55, 0.55);
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
			if (dsq->game->waterLevel.x > 0 && fabsf(avatar->position.y - dsq->game->waterLevel.x) < 800)
			{
				float time = 0.5;
				if (!myZoom.isInterpolating() || ((!core->globalScale.data || core->globalScale.data->target != zoomSurface) && myZoom.data->timePeriod != time))
				{
					myZoom.interpolateTo(zoomSurface, time, 0, 0, 1);
				}
			}
			else if (avatar->looking == 2)
			{
				float time = 1.0;
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
			else if ((cheatLen > sqr(250) && cheatLen < sqr(1000)) || attachedTo || avatar->looking==1)
			{
				float time = 3;
				if (avatar->looking)
				{
					time = 1.0;
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
				float time = 1.6;
				if (!myZoom.isInterpolating() || (myZoom.data->target != zoomMove && myZoom.data->timePeriod != time))
					myZoom.interpolateTo(zoomMove, time, 0, 0, 1);
			}

			if (myZoom.x < game->maxZoom)
			{
				core->globalScale.x = game->maxZoom;
				core->globalScale.y = game->maxZoom;
			}
			else
			{
				core->globalScale.x = myZoom.x;
				core->globalScale.y = myZoom.y;
			}

		}

		if (state.dodgeEffectTimer.isActive())
		{
			vel += dodgeVec*dt;
		}

		if (!state.lockedToWall && !bursting && _isUnderWater && swimming && !isFollowingPath())
		{
			//debugLog("collision avoidance");
			if (dsq->continuity.form == FORM_FISH)
				doCollisionAvoidance(dt, 1, 0.1, 0, 800, OT_HURT);
			else
				doCollisionAvoidance(dt, 2, 1.0, 0, 800, OT_HURT);
		}

		// friction for extraVel
		if (!extraVel.isZero())
		{
			Vector d = extraVel;
			d.setLength2D(100);
			extraVel -= d*dt;
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

		//int collideCircle = 24;//24; // 48
		int collideCircle = 10;
		if (dsq->continuity.form == FORM_FISH)
			collideCircle = 8;
		// just for external access
		// HACK: should always be using collide radius :| ?

		//updateMovement
		collideRadius = collideCircle;
		if (!state.lockedToWall && !isFollowingPath() && !riding)
		{
/*collideCheck:*/

			if (vel.getLength2D() < sqr(2))
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
				Vector mov = (moveVel * dt) + (extraVel * dt);
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

						lastLastPosition = position;
						lastPosition = position;
						Vector newPosition = position + mov;
						//Vector testPosition = position + (vel *dt)*2;
						position = newPosition;


						int hw = collideCircle;
						Vector fix;

						if (dsq->game->collideCircleWithGrid(position, hw, &fix))
						{
							if (dsq->game->lastCollideTileType == OT_HURT
								&& dsq->continuity.getWorldType() != WT_SPIRIT
								&& dsq->continuity.form != FORM_NATURE)
							{
								DamageData d;
								d.damage = 1;
								damage(d);
								vel2 = Vector(0,0,0);
								//doCollisionAvoidance(1, 3, 1);
								/*
								Vector v = dsq->game->getWallNormal(position);
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
								if (dsq->game->isObstructed(TileVector(position)))
								{
									//vel = -vel*0.5f;
									vel = 0;
								}
								else
								{
									// && dsq->game->getPercObsInArea(position, 4) < 0.75f
									if (!inCurrent)
									{
										/*
										int px=int(position.x);
										int py=int(position.y);
										int llpx=int(lastLastPosition.x);
										int llpy=int(lastLastPosition.y);
										if (px == llpx && py == llpy)
										{

										}
										else
										{
										*/
											//doBounce();
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
												Vector n = dsq->game->getWallNormal(position, 5);
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
										//}
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

		/*
		if (swimming)
		{
			int px=int(position.x);
			int py=int(position.y);
			int llpx=int(lastLastPosition.x);
			int llpy=int(lastLastPosition.y);
			if (px == llpx && py == llpy)
			{
				if (isNearObstruction(4))
				{
					vel = 0;
					Vector n = dsq->game->getWallNormal(position, 6);
					if (!n.isZero())
					{
						Vector add = n * 100;
						Vector f = getForward();
						n = (n + f * 100);
						n *= 0.5f;
						vel += n;
					}
				}
			}
		}
		*/

		if (ignoreInputDelay>0)
		{
			ignoreInputDelay -= dt;
			if (ignoreInputDelay < 0)
				ignoreInputDelay = 0;
		}

		if (burstBar && burstBar->alpha == 1)
		{
			float amount = burst;
			if (amount > 1) amount = 1;
			if (amount < 0) amount = 0;
			burstBar->frame = (19-(amount*19));
		}
		//checkSpecial();
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

	dsq->game->handleShotCollisions(this, (activeAura == AURA_SHIELD));
}


void Avatar::checkNearWall()
{
	state.nearWall = false;

	if (!inCurrent && bursting && !state.lockedToWall && !vel.isZero() && !riding && _isUnderWater)
	{
		/*
		int mult = 1;
		if (isfh())
			mult = -1;
		*/
		/*
		Vector n = dsq->game->getWallNormal(position, 8);
		if (!n.isZero())
		{
			state.nearWall = true;
			float t=0.2;
			rotateToVec(n, t, 0);
			skeletalSprite.transitionAnimate("wall", t);
		}
		else
			state.nearWall = false;
		*/
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
			if (dsq->game->isObstructed(t) && dsq->game->getGrid(t) != OT_HURT)
			{
				obs = true;
				break;
			}
			lastT = t;
		}
		if (obs)
		{
			Vector n = dsq->game->getWallNormal(t.worldVector());
			if (!n.isZero())
			{
				state.nearWall = true;
				float t=0.2;
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

void Avatar::checkSpecial()
{
	/*
	if (dsq->continuity.getFlag("VedhaFollow1") == 7)
	{
		int total = 0, c = 0;
		FOR_ENTITIES (i)
		{
			Entity *e = *i;
			if (e->name == "PracticeEnemy")
			{
				total++;
				if (e->isEntityDead())
					c++;
			}
		}
		if (total == c)
		{
			health = maxHealth;
			dsq->continuity.setFlag("VedhaFollow1", 8);
			dsq->getEntityByName("Vedha")->activate();
		}
	}
	*/
}

void Avatar::onWarp()
{
	avatar->setv(EV_NOINPUTNOVEL, 0);
	closeSingingInterface();
}

bool Avatar::checkWarpAreas()
{
	int i = 0;
	for (i = 0; i < dsq->game->getNumPaths(); i++)
	{
		bool warp = false;
		Path *p = dsq->game->getPath(i);
		if (!p->nodes.empty())
		{
			PathNode *n = &p->nodes[0];
			if (p && n)
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
						dsq->game->warpToSceneFromNode(p);
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
							Path *p2 = dsq->game->getPathByName(p->warpNode);
							if (p2)
							{
								dsq->game->preLocalWarp(p->localWarpType);
								dsq->game->avatar->position = p2->getPathNode(0)->position;
								dsq->game->postLocalWarp();
								//dsq->fade(0, t);
								return true;
							}

						}
					}
				}
			}
		}

	}
	for (i = 0; i < dsq->game->warpAreas.size(); i++)
	{
		WarpArea *a = &dsq->game->warpAreas[i];
		if (a->radius)
		{
			Vector diff = a->position - this->position;
			if (diff.getSquaredLength2D() < sqr(a->radius))
			{
				if (canWarp)
				{
					dsq->game->warpToArea(a);
					return false;
				}
				else
				{
					position = lastPosition;
					vel = -diff;
					return true;
				}
			}
		}
		else
		{
			if (position.x > a->position.x - a->w && position.x < a->position.x + a->w)
			{
				if (position.y > a->position.y - a->h && position.y < a->position.y + a->h)
				{
					if (canWarp)
					{
						dsq->game->warpToArea(a);
						return false;
					}
					else
					{
						position = lastPosition;
						vel = -vel*1.1f;
					}
				}
			}
		}
	}
	return false;
}
