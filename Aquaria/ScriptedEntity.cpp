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

bool ScriptedEntity::runningActivation = false;

ScriptedEntity::ScriptedEntity(const std::string &scriptName, Vector position, EntityType et) : CollideEntity(), Segmented(2, 26)
{	
	crushDelay = 0;
	autoSkeletalSpriteUpdate = true;
	script = 0;
	songNoteFunction = songNoteDoneFunction = true;
	addChild(&pullEmitter, PM_STATIC);

	hair = 0;
	becomeSolidDelay = false;
	strandSpacing = 10;
	animKeyFunc = true;
	preUpdateFunc = true;
	//runningActivation = false;

	setEntityType(et);
	myTimer = 0;
	layer = LR_ENTITIES;
	surfaceMoveDir = 1;
	this->position = position;
	numSegments = 0;
	reverseSegments = false;
	manaBallAmount = 1;
	this->name = scriptName;

	std::string file;
	if (!scriptName.empty())
	{
		if (scriptName[0]=='@' && dsq->mod.isActive())
		{
			file = dsq->mod.getPath() + "scripts/" + scriptName.substr(1, scriptName.size()) + ".lua";
			this->name = scriptName.substr(1, scriptName.size());
		}
		else if (dsq->mod.isActive())
		{
			file = dsq->mod.getPath() + "scripts/" + scriptName + ".lua";

			if (!exists(file))
			{
				file = "scripts/entities/" + scriptName + ".lua";
			}
		}
		else
		{
			file = "scripts/entities/" + scriptName + ".lua";
		}
	}
	script = dsq->scriptInterface.openScript(file);
	if (!script)
	{
		debugLog("Could not load script [" + file + "]");
	}
}

void ScriptedEntity::setAutoSkeletalUpdate(bool v)
{
	autoSkeletalSpriteUpdate = v;
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

void ScriptedEntity::message(const std::string &msg, void *v)
{
	if (script)
	{
		if (!script->call("msg", this, msg.c_str(), v))
			luaDebugMsg("msg", script->getLastError());
	}
	Entity::message(msg, v);
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
	//update(0);

	Entity::init();
	/*
	if (script)
	{
		bool fail=false;
		//update(0);
	}
	*/
}

void ScriptedEntity::postInit()
{	
	if (script)
	{
		if (!script->call("postInit", this))
			luaDebugMsg("postInit", script->getLastError());
	}

	Entity::postInit();
}

void ScriptedEntity::initEmitter(int emit, const std::string &file)
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

void ScriptedEntity::startEmitter(int emit)
{
	if (emitters[emit])
	{
		emitters[emit]->start();
	}
}

void ScriptedEntity::stopEmitter(int emit)
{
	if (emitters[emit])
	{
		emitters[emit]->stop();
	}
}

void ScriptedEntity::registerNewPart(RenderObject *r, const std::string &name)
{
	partMap[name] = r;
}

void ScriptedEntity::initHair(int numSegments, int segmentLength, int width, const std::string &tex)
{
	if (hair)
	{
		errorLog("Trying to init hair when hair is already present");
	}
	hair = new Hair(numSegments, segmentLength, width);
	hair->setTexture(tex);
	dsq->game->addRenderObject(hair, layer);
}

void ScriptedEntity::setHairHeadPosition(const Vector &pos)
{
	if (hair)
	{
		hair->setHeadPosition(pos);
	}
}

void ScriptedEntity::updateHair(float dt)
{
	if (hair)
	{
		hair->updatePositions();
	}
}

void ScriptedEntity::exertHairForce(const Vector &force, float dt)
{
	if (hair)
	{
		hair->exertForce(force, dt);
	}
}

void ScriptedEntity::initSegments(int numSegments, int minDist, int maxDist, std::string bodyTex, std::string tailTex, int w, int h, float taper, bool reverseSegments)
{
	this->reverseSegments = reverseSegments;
	this->numSegments = numSegments;
	this->minDist = minDist;
	this->maxDist = maxDist;
	segments.resize(numSegments);
	for (int i = segments.size()-1; i >= 0 ; i--)
	{
		Quad *q = new Quad;
		if (i == segments.size()-1)
			q->setTexture(tailTex);
		else
			q->setTexture(bodyTex);
		q->setWidthHeight(w, h);
		
		if (i > 0 && i < segments.size()-1 && taper !=0)
			q->scale = Vector(1.0f-(i*taper), 1-(i*taper));
		dsq->game->addRenderObject(q, LR_ENTITIES);
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

void ScriptedEntity::setupBasicEntity(std::string texture, int health, int manaBall, int exp, int money, int collideRadius, int state, int w, int h, int expType, bool hitEntity, int updateCull, int layer)
{
	//this->updateCull = updateCull;
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
	for (int i = 0; i < strands.size(); i++)
	{
		/*
		int sz = 5;
		if (i == 0 || i == strands.size()-1)
			sz = 4;
		*/
		strands[i] = new Strand(position, segs, dist);
		strands[i]->color = color;
		dsq->game->addRenderObject(strands[i], this->layer);
	}
	updateStrands(0);
}

/*
// write this if/when needed, set all strands to color (with lerp)
void ScriptedEntity::setStrandsColor(const Vector &color, float time)
{

}
*/

void ScriptedEntity::onAlwaysUpdate(float dt)
{
	Entity::onAlwaysUpdate(dt);
//	debugLog("calling updateStrands");
	updateStrands(dt);

	//HACK: this would be better in base Entity

	/*
	if (frozenTimer)
	{
	}
	*/

	if (!isEntityDead() && getState() != STATE_DEAD && getState() != STATE_DEATHSCENE && isPresent())
	{
		const bool useEV=false;

		if (useEV)
		{
			int mov = getv(EV_MOVEMENT);
			if (mov && frozenTimer)
			{
				doFriction(dt, 50);
			}
			else
			{
				// don't update friction if we're in a bubble.
				int fric = getv(EV_FRICTION);
				if (fric)
				{
					doFriction(dt, fric);
				}
			}

			switch (mov)
			{
			case 1:
				updateMovement(dt);
			break;
			case 2:
				updateCurrents(dt);
				updateMovement(dt);
			break;
			}

			if (mov)
			{
				if (hair)
				{
					setHairHeadPosition(position);
					updateHair(dt);
				}
			}


			switch (getv(EV_COLLIDE))
			{		
			case 1:
				if (skeletalSprite.isLoaded())
					dsq->game->handleShotCollisionsSkeletal(this);
				else
					dsq->game->handleShotCollisions(this);
			break;
			case 2:
				if (skeletalSprite.isLoaded())
					dsq->game->handleShotCollisionsSkeletal(this);
				else
					dsq->game->handleShotCollisions(this);

				int dmg = getv(EV_TOUCHDMG);
				if (frozenTimer > 0)
					dmg = 0;
				touchAvatarDamage(collideRadius, dmg);
			break;
			}
		}

		if (frozenTimer > 0)
		{
			pullEmitter.update(dt);

			if (!useEV)
			{
				doFriction(dt, 50);
				updateCurrents(dt);
				updateMovement(dt);

				if (hair)
				{
					setHairHeadPosition(position);
					updateHair(dt);
				}

				if (skeletalSprite.isLoaded())
					dsq->game->handleShotCollisionsSkeletal(this);
				else
					dsq->game->handleShotCollisions(this);
			}
		}

		if (isPullable() && !fillGridFromQuad)
		{
			bool doCrush = false;
			crushDelay -= dt;
			if (crushDelay < 0)
			{
				crushDelay = 0.2;
				doCrush = true;
			}
			//if ((dsq->game->avatar->position - this->position).getSquaredLength2D() < sqr(collideRadius + dsq->game->avatar->collideRadius))
			FOR_ENTITIES(i)
			{
				Entity *e = *i;
				if (e && e != this && e->life == 1 && e->ridingOnEntity != this)
				{
					if ((e->position - this->position).isLength2DIn(collideRadius + e->collideRadius))
					{
						if (this->isEntityProperty(EP_BLOCKER) && doCrush)
						{
							//bool doit = !vel.isLength2DIn(200) || (e->position.y > position.y && vel.y > 0);
							/*dsq->game->avatar->pullTarget != this ||*/ 
							/*&& */
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
									//e->push(vel, 0.2, 500, 0);
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
			//debugLog("movable!");
			Entity *followEntity = dsq->game->avatar;
			if (followEntity && dsq->game->avatar->pullTarget == this)
			{
				collideWithAvatar = false;
				//debugLog("followentity!");
				Vector dist = followEntity->position - this->position;
				if (dist.isLength2DIn(followEntity->collideRadius + collideRadius + 16))
				{
					vel = 0;
				}
				else if (!dist.isLength2DIn(800))
				{
					// break;
					vel.setZero();
					dsq->game->avatar->pullTarget->stopPull();
					dsq->game->avatar->pullTarget = 0;
				}
				else if (!dist.isLength2DIn(128))
				{				
					Vector v = dist;
					int moveSpeed = 1000;
					moveSpeed = 4000;
					v.setLength2D(moveSpeed);
					vel += v*dt;
					setMaxSpeed(dsq->game->avatar->getMaxSpeed());
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
					//vel = 0;
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
	//angle = (180*angle)/PI;
	float sz = (strands.size()/2);
	for (int i = 0; i < strands.size(); i++)
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
	//debugLog("calling target died");	

	CollideEntity::destroy();
	/*
	// spring plant might already be destroyed at this point (end of state)
	// could add as child?
	if (springPlant)
	{
		//springPlant->life = 0.1;
		springPlant->alpha = 0;
	}
	*/
	/*
	if (hair)
	{
		//dsq->removeRenderObject(hair, DESTROY_RENDER_OBJECT);
		dsq->game->removeRenderObject(hair);
		hair->destroy();
		delete hair;
		hair = 0;
	}
	*/
	if (script)
	{
		dsq->scriptInterface.closeScript(script);
		script = 0;
	}
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
		//debugLog("property set!");
		fillGridFromQuad = false;
		dsq->game->reconstructEntityGrid();
	}
	else
	{
		//debugLog("property not set!");
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
	BBGE_PROF(ScriptedEntity_onUpdate);

	/*
	if (script && preUpdateFunc)
	{
		if (!script->call("preUpdate", this, dt))
		{
			debugLog(name + " : preUpdate : " + script->getLastError());
			preUpdateFunc = false;
		}
	}
	*/

	if (!autoSkeletalSpriteUpdate)
		skeletalSprite.ignoreUpdate = true;

	CollideEntity::onUpdate(dt);

	if (!autoSkeletalSpriteUpdate)
		skeletalSprite.ignoreUpdate = false;
	//updateStrands(dt);

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


	if (myTimer > 0)
	{
		myTimer -= dt;
		if (myTimer <= 0)
		{
			myTimer = 0;
			onExitTimer();
		}
	}

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

	/*
	//HACK: if this is wanted (to support moving placed entities), then 
	// springPlant has to notify ScriptedEntity when it is deleted / pulled out
	if (springPlant)
	{
		springPlant->position = this->position;
	}
	*/
}

void ScriptedEntity::resetTimer(float t)
{
	myTimer = t;
}

void ScriptedEntity::stopTimer()
{
	myTimer = 0;
}

void ScriptedEntity::onExitTimer()
{
	if (script)
	{
		if (!script->call("exitTimer", this))
			debugLog(this->name + " : " + script->getLastError() + " exitTimer");
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

bool ScriptedEntity::damage(const DamageData &d)
{
	if (d.damageType == DT_NONE)	return false;
	bool doDefault = true;
	if (script)
	{
		if (!script->call("damage", this, d.attacker, d.bone, int(d.damageType), d.damage, d.hitPos.x, d.hitPos.y, d.shot, &doDefault))
		{
			debugLog(name + ": damage function failed");
			//debugLog(this->name + " : " + script->getLastError() + " hit");
		}
		else
		{
			/*
			std::ostringstream os;
			os << "doDefault: " << doDefault;
			debugLog(os.str());
			*/
		}
	}

	if (doDefault)
	{
		//debugLog("doing default damage");
		return Entity::damage(d);
	}

	//debugLog("not doing default damage");
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

bool ScriptedEntity::isEntityInside()
{
	bool v = false;
	int avatars = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->getEntityType() == ET_AVATAR)
			avatars ++;
		if (e && e->life == 1 && e != this && e->ridingOnEntity != this)
		{			
			if (isCoordinateInside(e->position))
			{				
				/*
				Vector diff = (e->position - position);
				diff.setLength2D(100);
				e->vel += diff;
				*/
				v = true;
			}
		}
		
	}
	return v;
}

void ScriptedEntity::becomeSolid()
{
	//vel = 0;
	float oldRot = 0;
	bool doRot=false;
	Vector n = dsq->game->getWallNormal(position);
	if (!n.isZero())
	{
		oldRot = rotation.z;
		rotateToVec(n, 0);
		doRot = true;
	}
	fillGridFromQuad = true;
	dsq->game->reconstructEntityGrid();

	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->ridingOnEntity == this)
		{
			e->ridingOnEntity = 0;
			e->moveOutOfWall();
			// if can't get the rider out of the wall, kill it
			if (dsq->game->isObstructed(TileVector(e->position)))
			{
				e->setState(STATE_DEAD);
			}
		}
	}

	if (doRot)
	{
		rotation.z = oldRot;
		rotateToVec(n, 0.01);
	}
}

void ScriptedEntity::onHitWall()
{
	if (isEntityProperty(EP_BLOCKER) && !fillGridFromQuad && dsq->game->avatar->pullTarget != this)
	{
		becomeSolidDelay = true;
	}
	
	if (isEntityProperty(EP_BLOCKER) && !fillGridFromQuad)
	{
		Vector n = dsq->game->getWallNormal(position);
		if (!n.isZero())
		{
			rotateToVec(n, 0.2);
		}
	}

	CollideEntity::onHitWall();

	if (script)
	{
		if (!script->call("hitSurface", this))
			debugLog(this->name + " : " + script->getLastError() + " hitSurface");
	}
}

void ScriptedEntity::activate()
{	
	if (runningActivation) return;
	Entity::activate();
	/*
	if (dsq->game->avatar)
	{
		Avatar *a = dsq->game->avatar;
		if (a->position.x < this->position.x)
		{
			if (!a->isFlippedHorizontal())
				a->flipHorizontal();
		}
		else
		{
			if (a->isFlippedHorizontal())
				a->flipHorizontal();
		}
		if (getEntityType() == ET_NEUTRAL)
			flipToTarget(dsq->game->avatar->position);
	}
	*/
	
	runningActivation = true;
	if (script)
	{
		if (!script->call("activate", this))
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


		for (int i = 0; i < strands.size(); i++)
		{
			strands[i]->safeKill();
		}
		strands.clear();

		// BASE ENTITY CLASS WILL HANDLE CLEANING UP HAIR
		/*
		if (hair)
		{
			hair->setLife(1.0);
			hair->setDecayRate(10);
			hair->fadeAlphaWithLife = true;
			hair = 0;
		}
		*/
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

