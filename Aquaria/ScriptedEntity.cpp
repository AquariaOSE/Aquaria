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
#include "ScriptedEntity.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "Shot.h"
#include "ScriptInterface.h"

bool ScriptedEntity::runningActivation = false;

ScriptedEntity::ScriptedEntity(const std::string &scriptName, Vector position, EntityType et) : CollideEntity(), Segmented(2, 26)
{
	addType(SCO_SCRIPTED_ENTITY);
	crushDelay = 0;
	songNoteFunction = songNoteDoneFunction = true;
	addChild(&pullEmitter, PM_STATIC);

	hair = 0;
	becomeSolidDelay = false;
	strandSpacing = 10;
	animKeyFunc = true;
	canShotHitFunc = true;
	postInitDone = false;


	setEntityType(et);
	layer = LR_ENTITIES;
	surfaceMoveDir = 1;
	this->position = position;
	numSegments = 0;
	reverseSegments = false;
	manaBallAmount = 1;
	this->name = scriptName;
	if(scriptName.length() && scriptName[0] == '@')
		this->name = this->name.substr(1, this->name.size());

	std::string file = ScriptInterface::MakeScriptFileName(scriptName, "entities");
	script = dsq->scriptInterface.openScript(file);
	if (!script)
	{
		debugLog("Could not load script [" + file + "]");
	}
}

ScriptedEntity::~ScriptedEntity()
{
}

void ScriptedEntity::setAutoSkeletalUpdate(bool v)
{
	skeletalSprite.ignoreUpdate = !v;
}

void ScriptedEntity::message(const std::string &msg, int v)
{
	if (script)
	{
		if (!script->call("msg", this, msg.c_str(), v))
			debugLog(name + " : msg : " + script->getLastError());
	}
	Entity::message(msg, v);
}

int ScriptedEntity::callVariadic(const char* func, lua_State* L, int nparams)
{
	if (script)
	{
		int res = script->callVariadic(func, L, nparams, this);
		if (res < 0)
			luaDebugMsg(func, script->getLastError());
		else
			return res;
	}
	return 0;
}

int ScriptedEntity::messageVariadic(lua_State *L, int nparams)
{
	return callVariadic("msg", L, nparams);
}

int ScriptedEntity::activateVariadic(lua_State* L, int nparams)
{
	return callVariadic("activate", L, nparams);
}

void ScriptedEntity::warpSegments()
{
	Segmented::warpSegments(position);
}

void ScriptedEntity::init()
{
	if (script)
	{
		if (!script->call("init", this))
			luaDebugMsg("init", script->getLastError());
	}

	Entity::init();
}

void ScriptedEntity::postInit()
{
	if(postInitDone)
		return;
	postInitDone = true;
	if (script)
	{
		if (!script->call("postInit", this))
			luaDebugMsg("postInit", script->getLastError());
	}

	Entity::postInit();
}

void ScriptedEntity::initEmitter(size_t emit, const std::string &file)
{
	if (emitters.size() <= emit)
	{
		emitters.resize(emit+1);
	}
	if (emitters[emit] != 0)
	{
		errorLog("Trying to init emitter being used");
		return;
	}
	emitters[emit] = new ParticleEffect;
	addChild(emitters[emit], PM_POINTER);
	emitters[emit]->load(file);
}

void ScriptedEntity::startEmitter(size_t emit)
{
	if(emit >= emitters.size())
		return;

	if (emitters[emit])
	{
		emitters[emit]->start();
	}
}

void ScriptedEntity::stopEmitter(size_t emit)
{
	if(emit >= emitters.size())
		return;

	if (emitters[emit])
	{
		emitters[emit]->stop();
	}
}

ParticleEffect *ScriptedEntity::getEmitter(size_t emit)
{
	return (size_t(emit) < emitters.size()) ? emitters[emit] : NULL;
}

size_t ScriptedEntity::getNumEmitters() const
{
	return emitters.size();
}

void ScriptedEntity::registerNewPart(RenderObject *r, const std::string &name)
{
	partMap[name] = r;
}

void ScriptedEntity::initSegments(int numSegments, int minDist, int maxDist, std::string bodyTex, std::string tailTex, int w, int h, float taper, bool reverseSegments)
{
	this->reverseSegments = reverseSegments;
	this->numSegments = numSegments;
	this->minDist = minDist;
	this->maxDist = maxDist;
	segments.resize(numSegments);
	for (size_t i = segments.size(); i-- > 0 ; )
	{
		Quad *q = new Quad;
		if (i == segments.size()-1)
			q->setTexture(tailTex);
		else
			q->setTexture(bodyTex);
		q->setWidthHeight(w, h);

		if (i > 0 && i < segments.size()-1 && taper !=0)
			q->scale = Vector(1.0f-(i*taper), 1-(i*taper));
		game->addRenderObject(q, LR_ENTITIES);
		segments[i] = q;
	}
	Segmented::initSegments(position);
}

void ScriptedEntity::setupEntity(const std::string &tex, int lcode)
{
	setEntityType(ET_NEUTRAL);
	if (!tex.empty())
		setTexture(tex);

	updateCull = -1;
	manaBallAmount = 0;
	setState(STATE_IDLE);

	this->layer = dsq->getEntityLayerToLayer(lcode);
}

void ScriptedEntity::setupBasicEntity(const std::string& texture, int health, int manaBall, int exp, int money, float collideRadius, int state, int w, int h, int expType, bool hitEntity, int updateCull, int layer)
{

	updateCull = -1;

	if (texture.empty())
		renderQuad = false;
	else
		setTexture(texture);
	this->health = maxHealth = health;
	this->collideRadius = collideRadius;
	setState(state);
	this->manaBallAmount = manaBall;
	width = w;
	height = h;

	setEntityLayer(layer);
}

void ScriptedEntity::setEntityLayer(int lcode)
{
	this->layer = dsq->getEntityLayerToLayer(lcode);
}

void ScriptedEntity::initStrands(int num, int segs, int dist, int strandSpacing, Vector color)
{
	this->strandSpacing = strandSpacing;
	strands.resize(num);
	for (size_t i = 0; i < strands.size(); i++)
	{
		strands[i] = new Strand(position, segs, dist);
		strands[i]->color = color;
		game->addRenderObject(strands[i], this->layer);
	}
	updateStrands(0);
}

void ScriptedEntity::onAlwaysUpdate(float dt)
{
	Entity::onAlwaysUpdate(dt);

	updateStrands(dt);

	if (!isEntityDead() && getState() != STATE_DEAD && getState() != STATE_DEATHSCENE && isPresent())
	{
		if (frozenTimer > 0)
		{
			pullEmitter.update(dt);

			doFriction(dt, 50);
			updateCurrents(dt);
			updateMovement(dt);

			if (hair)
			{
				setHairHeadPosition(position);
				updateHair(dt);
			}

			if (skeletalSprite.isLoaded())
				game->handleShotCollisionsSkeletal(this);
			else
				game->handleShotCollisions(this);
		}

		if (isPullable() && !fillGridFromQuad)
		{
			bool doCrush = false;
			crushDelay -= dt;
			if (crushDelay < 0)
			{
				crushDelay = 0.2f;
				doCrush = true;
			}
			FOR_ENTITIES(i)
			{
				Entity *e = *i;
				if (e && e != this && e->life == 1 && e->ridingOnEntity != this)
				{
					if ((e->position - this->position).isLength2DIn(collideRadius + e->collideRadius))
					{
						if (this->isEntityProperty(EP_BLOCKER) && doCrush)
						{
							bool doit = !vel.isLength2DIn(64) || (e->position.y > position.y && vel.y > 0);
							if (doit)
							{
								if (e->getEntityType() == ET_ENEMY && e->isDamageTarget(DT_CRUSH))
								{
									DamageData d;
									d.damageType = DT_CRUSH;
									d.attacker = this;
									d.damage = 1;
									if (e->damage(d))
									{
										e->sound("RockHit");
										dsq->spawnParticleEffect("rockhit", e->position, 0, 0);
									}

									Vector add = vel;
									add.setLength2D(5000*dt);
									e->vel += add;
								}
							}
						}
						Vector add = e->position - this->position;
						add.capLength2D(10000 * dt);
						e->vel += add;
						e->doCollisionAvoidance(dt, 3, 1);
					}
				}
			}
		}

		if (isPullable())
		{
			Entity *followEntity = game->avatar;
			if (followEntity && game->avatar->pullTarget == this)
			{
				Vector dist = followEntity->position - this->position;
				if (dist.isLength2DIn(followEntity->collideRadius + collideRadius + 16))
				{
					vel = 0;
				}
				else if (!dist.isLength2DIn(800))
				{

					vel.setZero();
					game->avatar->pullTarget->stopPull();
					game->avatar->pullTarget = 0;
				}
				else if (!dist.isLength2DIn(128))
				{
					Vector v = dist;
					int moveSpeed = 1000;
					moveSpeed = 4000;
					v.setLength2D(moveSpeed);
					vel += v*dt;
					setMaxSpeed(game->avatar->getMaxSpeed());
				}
				else
				{
					if (!vel.isZero())
					{
						Vector sub = vel;
						sub.setLength2D(getMaxSpeed()*maxSpeedLerp.x*dt);
						vel -= sub;
						if (vel.isLength2DIn(100))
							vel = 0;
					}
				}
				doCollisionAvoidance(dt, 2, 0.5);
			}
		}
	}
}

void ScriptedEntity::updateStrands(float dt)
{
	if (strands.empty()) return;
	float angle = rotation.z;
	angle = (PI*(360-(angle-90)))/180.0;

	float sz = (strands.size()/2);
	for (size_t i = 0; i < strands.size(); i++)
	{
		float diff = (i-sz)*strandSpacing;
		if (diff < 0)
			strands[i]->position = position - Vector(sinf(angle)*fabsf(diff), cosf(angle)*fabsf(diff));
		else
			strands[i]->position = position + Vector(sinf(angle)*diff, cosf(angle)*diff);
		if (dt > 0)
			strands[i]->update(dt);
	}
}

void ScriptedEntity::destroy()
{
	CollideEntity::destroy();

	closeScript();
}

void ScriptedEntity::song(SongType songType)
{
	if (script)
	{
		if (!script->call("song", this, int(songType)))
			debugLog(name + " : " + script->getLastError());
	}
}

void ScriptedEntity::shiftWorlds(WorldType lastWorld, WorldType worldType)
{
	if (script)
	{
		if (!script->call("shiftWorlds", this, int(lastWorld), int(worldType)))
			debugLog(name + " : " + script->getLastError() + " shiftWorlds");
	}
}

void ScriptedEntity::startPull()
{
	Entity::startPull();
	beforePullMaxSpeed = getMaxSpeed();
	becomeSolidDelay = false;
	debugLog("HERE!");
	if (isEntityProperty(EP_BLOCKER))
	{
		fillGridFromQuad = false;
		game->reconstructEntityGrid();
	}
	pullEmitter.load("Pulled");
	pullEmitter.start();

	// HACK: move this to the lower level at some point

	if (isEntityProperty(EP_BLOCKER))
	{
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e != this && e->getEntityType() != ET_AVATAR && e->isv(EV_CRAWLING, 1))
			{
				if ((e->position - position).isLength2DIn(collideRadius+e->collideRadius+32))
				{
					debugLog(e->name + ": is now riding on : " + name);
					e->ridingOnEntity = this;
					e->ridingOnEntityOffset = e->position - position;
					e->ridingOnEntityOffset.setLength2D(collideRadius);
				}
			}
		}
	}
}

void ScriptedEntity::sporesDropped(const Vector &pos, int type)
{
	if (script)
	{
		script->call("sporesDropped", this, pos.x, pos.y, type);
	}
}

void ScriptedEntity::stopPull()
{
	Entity::stopPull();
	pullEmitter.stop();
	setMaxSpeed(beforePullMaxSpeed);
}

void ScriptedEntity::onUpdate(float dt)
{
	CollideEntity::onUpdate(dt);

	if (becomeSolidDelay)
	{
		if (vel.isLength2DIn(5))
		{
			if (!isEntityInside())
			{
				becomeSolid();
				becomeSolidDelay = false;
			}
		}
	}


	if (life != 1 || isEntityDead()) return;

	if (this->isEntityDead() || this->getState() == STATE_DEATHSCENE || this->getState() == STATE_DEAD)
	{
		return;
	}
	if (script)
	{
		if (!script->call("update", this, dt))
			debugLog(name + " : update : " + script->getLastError());
	}

	if (numSegments > 0)
	{
		updateSegments(position, reverseSegments);
		updateAlpha(alpha.x);
	}
}

void ScriptedEntity::onAnimationKeyPassed(int key)
{
	if (script && animKeyFunc)
	{
		if (!script->call("animationKey", this, key))
		{
			debugLog(this->name + " : " + script->getLastError() + " animationKey");
			animKeyFunc = false;
		}
	}

	Entity::onAnimationKeyPassed(key);
}

void ScriptedEntity::lightFlare()
{
	if (script && !isEntityDead())
	{
		script->call("lightFlare", this);
	}
}

bool ScriptedEntity::canShotHit(const DamageData &d)
{
	bool doDefault = true;
	if (script && canShotHitFunc)
	{
		if (!script->call("canShotHit", this, d.attacker, d.bone, int(d.damageType), d.damage, d.hitPos.x, d.hitPos.y, d.shot, &doDefault))
		{
			debugLog(name + ": canShotHit function failed");
			canShotHitFunc = false;
		}
	}

	if (doDefault)
	{
		return Entity::canShotHit(d);
	}

	return false;
}

bool ScriptedEntity::damage(const DamageData &d)
{
	if (d.damageType == DT_NONE)	return false;
	bool doDefault = true;
	if (script)
	{
		if (!script->call("damage", this, d.attacker, d.bone, int(d.damageType), d.damage, d.hitPos.x, d.hitPos.y, d.shot, &doDefault))
		{
			debugLog(name + ": damage function failed");
		}
	}

	if (doDefault)
	{
		return Entity::damage(d);
	}

	return false;
}

void ScriptedEntity::songNote(int note)
{
	Entity::songNote(note);

	if (script && songNoteFunction)
	{
		if (!script->call("songNote", this, note))
		{
			songNoteFunction = false;
			debugLog(this->name + " : " + script->getLastError() + " songNote");
		}
	}
}

void ScriptedEntity::songNoteDone(int note, float len)
{
	Entity::songNoteDone(note, len);
	if (script && songNoteDoneFunction)
	{
		if (!script->call("songNoteDone", this, note, len))
		{
			songNoteDoneFunction = false;
			debugLog(this->name + " : " + script->getLastError() + " songNoteDone");
		}
	}
}

void ScriptedEntity::becomeSolid()
{

	float oldRot = 0;
	bool doRot=false;
	Vector n = game->getWallNormal(position);
	if (!n.isZero())
	{
		oldRot = rotation.z;
		rotateToVec(n, 0);
		doRot = true;
	}
	fillGridFromQuad = true;
	game->reconstructEntityGrid();

	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->ridingOnEntity == this)
		{
			e->ridingOnEntity = 0;
			e->moveOutOfWall();
			// if can't get the rider out of the wall, kill it
			if (game->isObstructed(TileVector(e->position)))
			{
				e->setState(STATE_DEAD);
			}
		}
	}

	if (doRot)
	{
		rotation.z = oldRot;
		rotateToVec(n, 0.01f);
	}
}

void ScriptedEntity::onHitWall()
{
	if (isEntityProperty(EP_BLOCKER) && !fillGridFromQuad && game->avatar->pullTarget != this)
	{
		becomeSolidDelay = true;
	}

	if (isEntityProperty(EP_BLOCKER) && !fillGridFromQuad)
	{
		Vector n = game->getWallNormal(position);
		if (!n.isZero())
		{
			rotateToVec(n, 0.2f);
		}
	}

	CollideEntity::onHitWall();

	if (script)
	{
		if (!script->call("hitSurface", this))
			debugLog(this->name + " : " + script->getLastError() + " hitSurface");
	}
}

void ScriptedEntity::activate(Entity *by, int source)
{
	if (runningActivation) return;
	Entity::activate(by, source);

	runningActivation = true;
	if (script)
	{
		if (!script->call("activate", this, by, source))
			luaDebugMsg("activate", script->getLastError());
	}
	runningActivation = false;
}

void ScriptedEntity::shotHitEntity(Entity *hit, Shot *shot, Bone *bone)
{
	Entity::shotHitEntity(hit, shot, bone);

	if (script)
	{
		script->call("shotHitEntity", this, hit, shot, bone);
	}
}

void ScriptedEntity::entityDied(Entity *e)
{
	CollideEntity::entityDied(e);

	if (script)
	{
		script->call("entityDied", this, e);
	}
}

void ScriptedEntity::luaDebugMsg(const std::string &func, const std::string &msg)
{
	debugLog("luaScriptError: " + name + " : " + func + " : " + msg);
}

void ScriptedEntity::onDieNormal()
{
	Entity::onDieNormal();
	if (script)
	{
		script->call("dieNormal", this);
	}
}

void ScriptedEntity::onDieEaten()
{
	Entity::onDieEaten();
	if (script)
	{
		script->call("dieEaten", this);
	}
}

void ScriptedEntity::onEnterState(int action)
{
	CollideEntity::onEnterState(action);

	if (script)
	{
		if (!script->call("enterState", this))
			luaDebugMsg("enterState", script->getLastError());
	}
	switch(action)
	{
	case STATE_DEAD:
		if (!isGoingToBeEaten())
		{
			doDeathEffects(manaBallAmount);
			dsq->spawnParticleEffect(deathParticleEffect, position);
			onDieNormal();
		}
		else
		{
			// eaten
			doDeathEffects(0);
			onDieEaten();
		}
		destroySegments(1);


		for (size_t i = 0; i < strands.size(); i++)
		{
			strands[i]->safeKill();
		}
		strands.clear();

		// BASE ENTITY CLASS WILL HANDLE CLEANING UP HAIR
	break;
	}
}

void ScriptedEntity::onExitState(int action)
{

	if (script)
	{
		if (!script->call("exitState", this))
			luaDebugMsg("exitState", script->getLastError());
	}

	CollideEntity::onExitState(action);
}

void ScriptedEntity::deathNotify(RenderObject *r)
{
	if (script)
	{
		if (!script->call("deathNotify", this, r))
			luaDebugMsg("deathNotify", script->getLastError());
	}
	CollideEntity::deathNotify(r);
}
