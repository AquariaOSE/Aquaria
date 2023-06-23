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
#include "../BBGE/MathFunctions.h"

#include "Entity.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "ScriptedEntity.h"
#include "Shot.h"
#include "PathFinding.h"
#include "Hair.h"

LanceData::LanceData()
	: delay(0), timer(0), gfx(NULL), bone(NULL)
{
}

LanceData::~LanceData()
{
	if(gfx)
	{
		gfx->setLife(1.0);
		gfx->setDecayRate(2);
		gfx->fadeAlphaWithLife = 1;
	}
}


void Entity::stopPull()
{
	if (game->avatar->pullTarget == this)
	{
		game->avatar->pullTarget = 0;
	}
}

void Entity::setIngredientData(const std::string &name)
{
	ingredientData = dsq->continuity.getIngredientDataByName(name);
}

void Entity::entityDied(Entity *e)
{
	for (size_t i = 0; i < targets.size(); i++)
	{
		targets[i] = 0;
	}

	if (boneLock.on)
	{
		if (boneLock.entity == e)
		{
			setBoneLock(BoneLock());
		}
	}
}

void Entity::setBounceType(BounceType bt)
{
	bounceType = bt;
}

BounceType Entity::getBounceType()
{
	return bounceType;
}

void Entity::generateCollisionMask(int ovrCollideRadius)
{
	if (this->skeletalSprite.isLoaded())
	{
		for (size_t i = 0; i < skeletalSprite.bones.size(); i++)
		{
			if (skeletalSprite.bones[i]->generateCollisionMask)
			{
				game->generateCollisionMask(skeletalSprite.bones[i], ovrCollideRadius);
			}
		}
	}
}



bool Entity::setBoneLock(const BoneLock &bl)
{
	if (!canSetBoneLock()) return false;

	if (bl.on && boneLockDelay > 0) return false;
	if (boneLock.on && bl.on) return false;

	if (boneLock.on && !bl.on)
	{
		boneLockDelay = 0.1f;
		boneLock = bl;
	}
	else
	{
		if (!bl.entity)
			return false;

		boneLock = bl;
		Quad *lockObj = bl.bone;
		if(!lockObj)
			lockObj = bl.entity;

		Vector posAndRot = lockObj->getWorldPositionAndRotation();
		boneLock.origRot = posAndRot.z;
		posAndRot.z = 0;
		Vector offs = position - posAndRot;
		boneLock.circleOffset = offs;
	}

	setv(EV_BONELOCKED, bl.on);

	onSetBoneLock();

	updateBoneLock();

	return true;
}

bool Entity::canSetBoneLock()
{
	return true;
}

Entity::Entity()
{
	addType(SCO_ENTITY);
	poison = 0.0f;
	calledEntityDied = false;
	wasUnderWater = true;
	waterBubble = 0;
	ridingFlip = false;
	ridingRotation = 0;
	boneLockDelay = 0;
	ingredientData = 0;
	for (int i = 0; i < EV_MAX; i++)
	{
		vs[i] = 0;
	}
	setv(EV_COLLIDELEVEL, 1);
	setv(EV_LOOKAT, 1);
	setv(EV_SWITCHCLAMP, 1);
	setvf(EV_CLAMPTRANSF, 0.2f);
	setv(EV_FLIPTOPATH, 1);
	setv(EV_NOINPUTNOVEL, 1);
	setv(EV_VINEPUSH, 1);
	setv(EV_BEASTBURST, 1);
	setv(EV_WEBSLOW, 100);



	invincible = false;
	lancedata = NULL;
	beautyFlip = true;
	fhScale = false;
	flipScale = Vector(1,1);
	wasUnderWater = true;
	deathScene = false;
	dieTimer = 0;
	bounceType = BOUNCE_SIMPLE;
	riding = 0;
	eatType = EAT_DEFAULT;
	stickToNaijasHead = false;
	spiritFreeze = true;
	pauseFreeze = true;
	canLeaveWater = false;
	targetPriority = 0;


	ridingOnEntity = 0;
	targetRange = 32;

	deathSound = "GenericDeath";
	entityID = 0;
	hair = 0;
	maxSpeedLerp = 1;
	fillGridFromQuad = false;
	dropChance = 0;
	inCurrent = false;
	for (size_t i = 0; i < EP_MAX; i++)
	{
		entityProperties[i] = false;
	}
	damageTime = vars->entityDamageTime;
	slowingToStopPathTimer = 0;
	slowingToStopPath = 0;
	swimPath = false;
	currentEntityTarget = 0;
	deleteOnPathEnd = false;
	multColor = Vector(1,1,1);
	collideRadius = 24;
	entityType = EntityType(0);
	targets.resize(10);

	frozenTimer = 0;
	canBeTargetedByAvatar = false;
	activationRange = 0;
	activationType = ACT_NONE;
	pushDamage = 0;



	dsq->addEntity(this);
	maxSpeed = 300;
	entityDead = false;
	health = maxHealth = 5;
	invincibleBreak = false;
	activationRadius = 40;
	activationRange = 600;

	bubble = 0;



	skeletalSprite.setAnimationKeyNotify(this);
	addChild(&skeletalSprite, PM_NONE);



	setDamageTarget(DT_AVATAR_NATURE, false);
	setDamageTarget(DT_AVATAR_LIZAP, true);

	setDamageTarget(DT_AVATAR_BUBBLE, false);
	setDamageTarget(DT_AVATAR_SEED, false);

	stopSoundsOnDeath = false;
	minimapIcon = 0;

	renderPass = RENDER_ALL;



}

Entity::~Entity()
{
	delete minimapIcon;
	delete lancedata;
}

void Entity::setDeathScene(bool v)
{
	deathScene = v;
}

void Entity::setCanLeaveWater(bool v)
{
	canLeaveWater = v;
}

bool Entity::checkSplash(const Vector &o)
{
	Path *lastWaterBubble = waterBubble;

	Vector check = position;
	if (!o.isZero())
		check = o;
	bool changed = false;
	bool uw = isUnderWater(o);
	if (wasUnderWater && !uw)
	{
		sound("splash-outof");
		changed = true;
		wasUnderWater = false;
	}
	else if (!wasUnderWater && uw)
	{
		sound("splash-into");
		changed = true;
		wasUnderWater = true;
	}

	if (changed)
	{
		float angle;

		if (!wasUnderWater && waterBubble)
		{
			Vector diff = position - waterBubble->nodes[0].position;
			angle = MathFunctions::getAngleToVector(diff, 0) + 180;
		}
		else if (wasUnderWater && lastWaterBubble)
		{
			Vector diff = position - lastWaterBubble->nodes[0].position;
			angle = MathFunctions::getAngleToVector(diff, 0);
		}
		else
		{
			angle = MathFunctions::getAngleToVector(vel+vel2, 0);
		}
		dsq->spawnParticleEffect("Splash", check, angle);
	}

	return changed;
}

void Entity::setSpiritFreeze(bool v)
{
	spiritFreeze = v;
}

void Entity::setPauseFreeze(bool v)
{
	pauseFreeze = v;
}

void Entity::setEntityProperty(EntityProperty ep, bool value)
{
	entityProperties[ep] = value;
}

bool Entity::isEntityProperty(EntityProperty ep)
{
	return entityProperties[ep];
}

Vector Entity::getRidingPosition()
{
	if (ridingPosition.isZero())
		return position;
	return ridingPosition;
}

float Entity::getRidingRotation()
{
	if (ridingRotation == 0)
		return rotation.z;
	return ridingRotation;
}


void Entity::setRidingFlip(bool on)
{
	ridingFlip = on;
}

bool Entity::getRidingFlip()
{
	return ridingFlip;
}

void Entity::setRidingData(const Vector &pos, float rot, bool fh)
{
	setRidingPosition(pos);
	setRidingRotation(rot);
	setRidingFlip(fh);
}

void Entity::setRidingPosition(const Vector &pos)
{
	ridingPosition = pos;
}

void Entity::setRidingRotation(float rot)
{
	ridingRotation = rot;
}

void Entity::doFriction(float dt)
{
	if (vel.getSquaredLength2D() > 0)
	{
		const float velStopLen = 10;
		bool wasIn = vel.isLength2DIn(velStopLen);

		Vector d = vel;
		d.setLength2D(vars->frictionForce);
		vel -= d * dt;

		if (!wasIn && vel.isLength2DIn(velStopLen))
			vel = 0;
	}
}

void Entity::doFriction(float dt, float len)
{
	Vector v = vel;
	if (!v.isZero())
	{
		v.setLength2D(dt * len);
		vel -= v;
	}
}

void Entity::setName(const std::string &name)
{
	this->name = name;
}

float Entity::followPath(Path *p, float speed, int dir, bool deleteOnEnd)
{
	if(!speed)
		speed = getMaxSpeed();
	deleteOnPathEnd = deleteOnEnd;

	position.stopPath();
	position.ensureData();
	position.data->path.clear();
	if (dir)
	{
		for (int i = p->nodes.size()-1; i >=0; i--)
		{
			PathNode pn = p->nodes[i];
			position.data->path.addPathNode(pn.position, 1.0f-(float(i/float(p->nodes.size()))));
		}
	}
	else
	{
		for (size_t i = 0; i < p->nodes.size(); i++)
		{
			PathNode pn = p->nodes[i];
			position.data->path.addPathNode(pn.position, float(i/float(p->nodes.size())));
		}
	}
	//debugLog("Calculating Time");
	float time = position.data->path.getLength()/speed;

	position.data->path.getPathNode(0)->value = position;
	position.startPath(time);
	return time;
}

float Entity::moveToPos(Vector dest, float speed, int dieOnPathEnd, bool swim)
{
	if(!speed)
		speed = getMaxSpeed();

	Vector start = position;

	position.ensureData();
	position.data->path.clear();
	position.stop();

	swimPath = swim;

	PathFinding::generatePath(this, TileVector(start), TileVector(dest));



	//debugLog("Regenerating section");



	this->vel = 0;

	//debugLog("Molesting Path");

	PathFinding::molestPath(position.data->path);



	//debugLog("forcing path to minimum 2 nodes");
	PathFinding::forceMinimumPath(position.data->path, start, dest);



	float time = position.data->path.getLength()/speed;

	position.data->path.getPathNode(0)->value = position;
	position.startPath(time);



	deleteOnPathEnd = dieOnPathEnd;



	return time;
}

void Entity::stopFollowingPath()
{
	position.stopPath();
}

void Entity::flipToTarget(Vector pos)
{



	if (pos.x > position.x)
	{
		if (!isfh())
			flipHorizontal();
	}
	else if (pos.x < position.x)
	{
		if (isfh())
			flipHorizontal();
	}
}

Entity* Entity::getTargetEntity(int t)
{
	return targets[t];
}

void Entity::setTargetEntity(Entity *e, int t)
{
	targets[t] = e;
}

bool Entity::hasTarget(int t)
{
	return (targets[t]!=0);
}

void Entity::destroy()
{
	if (stopSoundsOnDeath)
		this->stopAllSounds();
	this->unlinkAllSounds();


	if (hair)
	{

		hair = 0;
	}
	Shot::targetDied(this);
	dsq->removeEntity(this);
	Quad::destroy();
}

bool Entity::checkSurface(int tcheck, int state, float statet)
{
	TileVector hitTile;
	if (isNearObstruction(tcheck, OBSCHECK_8DIR, &hitTile))
	{
		if (clampToSurface(tcheck, Vector(0,0), hitTile))
		{
			setState(state, statet);
			return true;
		}
	}
	return false;

}

void Entity::rotateToSurfaceNormal(float t, int n, int rot)
{
	Vector v;
	if (ridingOnEntity)
	{
		v = position - ridingOnEntity->position;
	}
	else
	{
		if (n == 0)
			v = game->getWallNormal(position);
		else
			v = game->getWallNormal(position, n);
	}

	if (!v.isZero())
	{
		rotateToVec(v, t, rot);
	}
}

int Entity::getv(EV ev)
{
	return vs[ev];
}

float Entity::getvf(EV ev)
{
	return float(vs[ev])*0.01f;
}

bool Entity::isv(EV ev, int v)
{
	return vs[ev] == v;
}

void Entity::setv(EV ev, int v)
{
	vs[ev] = v;
}

void Entity::setvf(EV ev, float v)
{
	vs[ev] = int(v * 100.0f);
}

bool Entity::clampToSurface(int tcheck, Vector usePos, TileVector hitTile)
{
	float t = getvf(EV_CLAMPTRANSF);
	if (usePos.isZero())
		usePos = position;
	if (tcheck == 0)
		tcheck = 40;
	bool clamped = false;
	// HACK: ensure entity gets to location
	setv(EV_CRAWLING, 1);
	burstTimer.stop();
	// do stuff
	Vector pos = TileVector(usePos).worldVector();
	if (!hitTile.isZero())
	{

		pos = hitTile.worldVector();
		clamped = true;
	}
	else
	{

		if (vel.getSquaredLength2D() < 1)
		{
	longCheck:

			for (int i = 0; i < tcheck; i++)
			{
				int bit = i*TILE_SIZE;
				int backBit = (i-1)*TILE_SIZE;
				if (game->isObstructed(TileVector(pos - Vector(bit,0))))
				{
					TileVector t(pos - Vector(backBit,0));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (game->isObstructed(TileVector(pos - Vector(0,bit))))
				{
					TileVector t(pos - Vector(0,backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (game->isObstructed(TileVector(pos + Vector(bit,0))))
				{
					TileVector t(pos + Vector(backBit,0));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (game->isObstructed(TileVector(pos + Vector(0,bit))))
				{
					TileVector t(pos + Vector(0,backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (game->isObstructed(TileVector(pos + Vector(-bit,-bit))))
				{
					TileVector t(pos + Vector(-backBit,-backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (game->isObstructed(TileVector(pos + Vector(-bit,bit))))
				{
					TileVector t(pos + Vector(-backBit,backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (game->isObstructed(TileVector(pos + Vector(bit,-bit))))
				{
					TileVector t(pos + Vector(backBit,-backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (game->isObstructed(TileVector(pos + Vector(bit,bit))))
				{
					TileVector t(pos + Vector(backBit,backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
			}
		}
		else
		{

			Vector v = vel;
			v.normalize2D();
			for (int i = 0; i < tcheck; i++)
			{
				if (game->isObstructed(TileVector(pos + v*TILE_SIZE*i)))
				{
					TileVector t(pos + v*TILE_SIZE*(i-1));
					pos = t.worldVector();
					clamped = true;
					break;
				}
			}
			if (!clamped)
				goto longCheck;
		}
	}
	if (t > 0)
	{
		Vector n = game->getWallNormal(pos);
		n *= getv(EV_WALLOUT);

		Vector diff = getWorldPosition() - pos;

		offset = diff;
		offset.interpolateTo(n, t);
		position = pos;


		rotateToSurfaceNormal(0);

		setv(EV_CLAMPING, 1);



	}
	else
		position = pos;

	if (clamped)
	{
		vel = Vector(0,0,0);
		vel2 = Vector(0,0,0);
	}

	return clamped;
}

void Entity::heal(float a, int type)
{
	if (!entityDead)
	{
		health += a;
		if (health > maxHealth) health = maxHealth;
		onHealthChange(a);
		onHeal(type);
	}
}

void Entity::revive(float a)
{
	entityDead = false;
	health = 0;
	heal(a);
	if (getState() != STATE_IDLE)
		perform(STATE_IDLE);
	onHealthChange(a);

}

bool Entity::isGoingToBeEaten()
{
	return (eatType != EAT_NONE && (lastDamage.damageType == DT_AVATAR_BITE || lastDamage.damageType == DT_AVATAR_PETBITE));
}

void Entity::doDeathEffects(float manaBallEnergy, bool die)
{
	if (deathScene || !isGoingToBeEaten())
	{
		if (manaBallEnergy)
		{
			if (chance(dropChance))
			{
				game->spawnManaBall(position, manaBallEnergy);
			}
		}
		if (die)
		{
			setLife(1);
			setDecayRate(4);
			fadeAlphaWithLife = true;
		}
		else
		{
			alpha.interpolateTo(0.01f,1);
		}
	}
	else
	{
		if (die)
		{
			setLife(1);
			setDecayRate(1);
			fadeAlphaWithLife = true;
		}
		else
		{
			alpha.interpolateTo(0.01f,1);
		}
		scale.interpolateTo(Vector(0,0), 1);
		stickToNaijasHead = true;
	}
	activationType = ACT_NONE;

	if (ingredientData)
	{
		game->spawnIngredientFromEntity(this, ingredientData);
	}
}

bool Entity::isNearObstruction(int sz, int type, TileVector *hitTile)
{
	bool v=false;
	TileVector t(position);
	TileVector test;
	switch(type)
	{
	case OBSCHECK_RANGE:
	{
		for (int x = -sz; x <= sz; x++)
		{
			for (int y = -sz; y <= sz; y++)
			{
				test = TileVector(t.x+x, t.y+y);
				if (game->isObstructed(test))
				{
					v = true;
					break;
				}
			}
		}
	}
	break;
	case OBSCHECK_4DIR:
	{
		for (int x = -sz; x <= sz; x++)
		{
			test = TileVector(t.x+x, t.y);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}
		}
		for (int y = -sz; y <= sz; y++)
		{
			test = TileVector(t.x, t.y+y);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}
		}
	}
	break;
	case OBSCHECK_DOWN:
	{
		for (int y = 0; y <= sz; y++)
		{
			test = TileVector(t.x, t.y+y);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}
		}
	}
	break;
	case OBSCHECK_8DIR:
	{

		for (int d = 0; d <= sz; d++)
		{

			test = TileVector(t.x+d, t.y);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}
			test = TileVector(t.x-d, t.y);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}

			test = TileVector(t.x, t.y+d);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}
			test = TileVector(t.x, t.y-d);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}

			test = TileVector(t.x-d, t.y-d);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}
			test = TileVector(t.x-d, t.y+d);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}

			test = TileVector(t.x+d, t.y-d);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}
			test = TileVector(t.x+d, t.y+d);
			if (game->isObstructed(test))
			{
				v = true;
				break;
			}
		}
	}
	break;
	}
	if (hitTile)
		*hitTile = test;
	return v;
}

bool Entity::touchAvatarDamage(int radius, float dmg, const Vector &override, float speed, float pushTime, Vector collidePos)
{
	if (isv(EV_BEASTBURST, 1) && isDamageTarget(DT_AVATAR_BITE) && dsq->continuity.form == FORM_BEAST && game->avatar->bursting)
	{
		return false;
	}
	// this whole function has to be rewritten
	Vector usePosition = position;
	if (override.x != -1)
	{
		usePosition = override;
	}
	if (!collidePos.isZero())
	{
		usePosition = getWorldCollidePosition(collidePos);



	}
	if (radius == 0 || (game->avatar->getWorldPosition() - usePosition).getSquaredLength2D() < sqr(radius+game->avatar->collideRadius))
	{
		if (dmg > 0)
		{
			DamageData d;
			d.damage = dmg;
			d.attacker = this;
			game->avatar->damage(d);
		}
		if (pushTime > 0 && speed > 0)
		{
			Vector diff = game->avatar->position - position;
			diff.setLength2D(speed);


			game->avatar->push(diff, pushTime, speed, dmg);
		}
		else if (speed > 0)
		{
			game->avatar->fallOffWall();
			Vector diff = game->avatar->position - position;
			diff.setLength2D(speed);
			game->avatar->vel += diff;
		}


		return true;
	}
	return false;
}

const float sct = 0.15f;

void Entity::onFHScale()
{
	flipScale.interpolateTo(Vector(1, 1), sct);
	_fh = !_fh;
	fhScale = false;
}

void Entity::onFH()
{
	if (!beautyFlip) return;
	_fh = !_fh;
	if (!fhScale)
	{
		flipScale = Vector(1,1);
		flipScale.interpolateTo(Vector(0.6f, 1), sct);
		fhScale = true;
	}
}

void Entity::flipToVel()
{
	if ((vel.x < -5 && isfh()) || (vel.x > 5 && !isfh()))
		flipHorizontal();
}

void Entity::frozenUpdate(float dt)
{
}

void Entity::update(float dt)
{
	Vector backupPos = position;
	Vector backupVel = vel;

	bool doUpdate = (updateCull < 0 || (position - core->screenCenter).isLength2DIn(updateCull));
	if (doUpdate && !(pauseFreeze && game->isPaused()))
	{

		if (!(getEntityType() == ET_AVATAR || getEntityType() == ET_INGREDIENT))
		{
			if (spiritFreeze && game->isWorldPaused())
			{
				// possible bug here because of return
				return;
				//dt *= 0.5f;
			}
			//if (dsq->continuity.getWorldType() != WT_NORMAL)
			//	return;
		}

		//skeletalSprite.setFreeze(true);

		if (frozenTimer == 0 || getState() == STATE_PUSH)
			Quad::update(dt);
		onAlwaysUpdate(dt);

		// always, always update:
		if (damageTimer.updateCheck(dt))
		{
			multColor.stop();
			multColor = Vector(1,1,1);
		}

		if (!multColor.isInterpolating())
		{
			multColor = Vector(1,1,1);
		}

		vineDamageTimer.update(dt);

		if (dieTimer > 0)
		{
			dieTimer -= dt;
			if (dieTimer <0)
			{
				dieTimer = 0;

				setLife(1);
				setDecayRate(1);
				fadeAlphaWithLife = 1;
			}
		}

		StateMachine::onUpdate(dt);

		if (frozenTimer > 0)
		{
			onUpdateFrozen(dt);
			if (bubble)
				bubble->position = this->position;

			frozenTimer -= dt;
			if (frozenTimer <= 0)
			{
				frozenTimer = 0;
				popBubble();
			}
		}



	}

	updateBoneLock();

	if (position.isNan())
		position = backupPos;
	if (vel.isNan())
		vel = backupVel;

	updateSoundPosition();

	if(minimapIcon)
		minimapIcon->update(dt);
}

void Entity::postUpdate(float dt)
{
	updateBoneLock();
}

bool Entity::isAvatarAttackTarget()
{
	return getEntityType() == ET_ENEMY && canBeTargetedByAvatar;
}

bool Entity::pathBurst(bool wallJump)
{
	if (skeletalSprite.isLoaded() && (wallJump || (!wallJump && !burstTimer.isActive())))
	{
		game->playBurstSound(wallJump);
		skeletalSprite.animate("burst");
		position.ensureData();
		if (wallJump)
			position.data->pathTimeMultiplier = 2;
		else
			position.data->pathTimeMultiplier = 1.5;
		burstTimer.start(1);

		return true;
	}
	return false;
}

bool Entity::isFollowingPath()
{
	return position.isFollowingPath();
}

void Entity::onPathEnd()
{
	if (deleteOnPathEnd)
	{
		safeKill();
	}
	else
	{
		if (swimPath)
		{
			offset.interpolateTo(Vector(0, 0), 0.4f);
			rotateToVec(Vector(0,-1), 0.1f, 0);
			if (skeletalSprite.isLoaded())
			{
				skeletalSprite.animate("idle", -1);
			}
			position.ensureData();
			int num = position.data->path.getNumPathNodes();
			if (num >= 2)
			{
				Vector v2 = position.data->path.getPathNode(num-1)->value;
				Vector v1 = position.data->path.getPathNode(num-2)->value;
				Vector v = v2 - v1;

				if (isv(EV_FLIPTOPATH, 1))
				{
					if (v.x > 0)
					{
						if (!isfh())
							flipHorizontal();
					}
					if (v.x < 0)
					{
						if (isfh())
							flipHorizontal();
					}
				}
			}
		}
	}
}

void Entity::movementDetails(Vector v)
{
	v.normalize2D();

	if (isv(EV_FLIPTOPATH, 1))
	{
		if (v.x < 0)
		{
			if (isfh())
				flipHorizontal();
		}
		else if (v.x > 0)
		{
			if (!isfh())
				flipHorizontal();
		}
	}

	if (skeletalSprite.isLoaded())
	{
		if (burstTimer.isActive())
			rotateToVec(v, 0.05f);
		else
			rotateToVec(v, 0.2f);
		Animation *anim = skeletalSprite.getCurrentAnimation();
		if (!burstTimer.isActive())
		{
			if (!anim || anim->name != "swim")
				skeletalSprite.transitionAnimate("swim", 0.1f, -1);
		}
	}
}

void Entity::slowToStopPath(float t)
{
	slowingToStopPath = t;
	std::ostringstream os;
	os << "slowingToStopPath: " << slowingToStopPath;
	debugLog(os.str());
	slowingToStopPathTimer = 0;
}

bool Entity::isSlowingToStopPath()
{
	bool v = (slowingToStopPath > 0);

	return v;
}



bool Entity::updateCurrents(float dt)
{
	inCurrent = false;
	int c = 0;
	Vector accum;

	//if (isUnderWater())
	// why?
	{

		if (!game->isWorldPaused())
		{
			for (Path *p = game->getFirstPathOfType(PATH_CURRENT); p; p = p->nextOfType)
			{
				if (p->active)
				{
					for (size_t n = 1; n < p->nodes.size(); n++)
					{
						PathNode *node2 = &p->nodes[n];
						PathNode *node1 = &p->nodes[n-1];
						Vector dir = node2->position - node1->position;
						if (isTouchingLine(node1->position, node2->position, position, collideRadius + p->rect.getWidth()/2))
						{
							inCurrent = true;

							dir.setLength2D(p->currentMod);
							accum += dir;
							c++;


						}
					}
				}
			}
		}
		if (inCurrent)
		{
			accum /= c;
			vel2 = accum;
			float len = vel2.getLength2D();
			float useLen = len;
			if (useLen < 500)
				useLen = 500;
			if (!(this->getEntityType() == ET_AVATAR && game->avatar->canSwimAgainstCurrents() && game->avatar->bursting))
			{
				doCollisionAvoidance(1, 4, 1, &vel2, useLen);
			}

			doCollisionAvoidance(dt, 3, 1, 0, useLen);

			Vector dist = -vel2;
			dist.normalize2D();
			float v = dist.x;
			float scale = 0.2f;
			if (getEntityType() == ET_AVATAR)
			{
				Avatar *a = game->avatar;
				if (v < 0)
					dsq->rumble((-v)*scale, (1.0f+v)*scale, 0.2f, a->getLastActionSourceID(), a->getLastActionInputDevice());
				else
					dsq->rumble((1.0f-v)*scale, (v)*scale, 0.1f, a->getLastActionSourceID(), a->getLastActionInputDevice());
			}
		}
	}
	if (this->getEntityType() == ET_AVATAR && game->avatar->canSwimAgainstCurrents())
	{
		int cap = 100;
		if (!vel.isZero())
		{
			if (vel.dot2D(vel2) < 0 ) // greater than 90 degrees
			{
				// against current
				if (game->avatar->bursting)
					vel2 = 0;
				else if (game->avatar->isSwimming())
					vel2.capLength2D(cap);
			}
		}
		else
		{

		}
	}

	return inCurrent;
}

const float minVel2Len = 10;

void Entity::updateVel2(float dt, bool override)
{
	if ((override || !inCurrent) && !vel2.isZero())
	{
		bool wasUnder = vel.isLength2DIn(minVel2Len);
		// PATCH_KEY: vel2 doesn't build up if dt is low
		// 1000 * dt
		Vector d = vel2;
		d.setLength2D(1000*dt);
		bool xg = (vel2.x > 0), yg = (vel2.y > 0);
		vel2 -= d;

		// PATCH_KEY: could this be what breaks it? never gets out of 10?
		//|| vel2.isLength2DIn(10)
		if ((xg != (vel2.x > 0)) || (yg != (vel2.y >0)) || (!wasUnder && vel2.isLength2DIn(minVel2Len)))
			vel2 = 0;
	}
}

bool Entity::isSittingOnInvisibleIn()
{
	TileVector t(position);
	for (int x = 0; x < 4; x++)
	{
		for (int y = 0; y < 4; y++)
		{
			if (game->isObstructed(TileVector(t.x+x, t.y+y), OT_INVISIBLEIN))
				return true;
			if (game->isObstructed(TileVector(t.x-x, t.y+y), OT_INVISIBLEIN))
				return true;
			if (game->isObstructed(TileVector(t.x+x, t.y-y), OT_INVISIBLEIN))
				return true;
			if (game->isObstructed(TileVector(t.x-x, t.y-y), OT_INVISIBLEIN))
				return true;
		}
	}

	return false;



}

void Entity::moveOutOfWall()
{


	Vector n = getNormal();
	TileVector t(position);
	int c = 0;
	bool useValue = true;
	while (game->isObstructed(t))
	{
		c++;
		if (c > 6)
		{

			useValue = false;
			break;
		}
		t.x += n.x;
		t.y += n.y;
	}
	if (useValue)
	{
		position = t.worldVector();
	}
}

void Entity::setDamageTarget(DamageType dt, bool v)
{
	DisabledDamageTypes::iterator it = std::lower_bound(disabledDamageTypes.begin(), disabledDamageTypes.end(), dt);
	if (v)
	{
		if(it != disabledDamageTypes.end() && *it == dt)
			disabledDamageTypes.erase(it);
	}
	else
	{
		if(it == disabledDamageTypes.end())
			disabledDamageTypes.push_back(dt);
		else if(*it != dt)
			disabledDamageTypes.insert(it, dt);
	}
}

void Entity::setEatType(EatType et, const std::string &file)
{
	eatType = et;
	if (eatType == EAT_FILE)
	{
		EatData *e = dsq->continuity.getEatData(file);
		if (e)
			eatData = *e;
	}
}

void Entity::setAllDamageTargets(bool v)
{
	disabledDamageTypes.clear();
	if(!v)
	{
		for (int i = DT_ENEMY; i < DT_ENEMY_REALMAX; i++)
			disabledDamageTypes.push_back(DamageType(i));
		for (int i = DT_AVATAR; i < DT_AVATAR_REALMAX; i++)
			disabledDamageTypes.push_back(DamageType(i));
		for (int i = DT_AVATAR_MAX; i < DT_REALMAX; i++)
			disabledDamageTypes.push_back(DamageType(i));
	}
}

bool Entity::isDamageTarget(DamageType dt)
{
	return !std::binary_search(disabledDamageTypes.begin(), disabledDamageTypes.end(), dt);
}

float Entity::getHealthPerc()
{
	return float(health) / float(maxHealth);
}

Vector Entity::getMoveVel()
{
	return vel + vel2;
}

void Entity::onEndOfLife()
{
	if (!calledEntityDied)
	{
		game->entityDied(this);
		calledEntityDied = true;
	}

	skeletalSprite.safeKill();
}

void Entity::setPoison(float m, float t)
{
	poison = m;
	poisonTimer.start(t);
	if (poison)
		poisonBitTimer.start(dsq->continuity.poisonBitTime);
}

void Entity::onUpdate(float dt)
{
	if (isv(EV_CLAMPING, 1))
	{
		if (!offset.isInterpolating())
		{
			setv(EV_CLAMPING, 0);
		}
	}
	vel2.update(dt);

	if (boneLockDelay > 0)
	{
		boneLockDelay -= dt;
		if (boneLockDelay < 0)
			 boneLockDelay = 0;
	}


	if (beautyFlip)
	{
		flipScale.update(dt);
		if (fhScale && !flipScale.isInterpolating())
			onFHScale();
	}



	Vector lastPos = position;

	if (ridingOnEntity)
	{
		position = ridingOnEntity->position + ridingOnEntityOffset;
	}

	if (hair)
	{
		hair->color.x = color.x * multColor.x;
		hair->color.y = color.y * multColor.y;
		hair->color.z = color.z * multColor.z;

	}

	if (slowingToStopPath > 0)
	{


		slowingToStopPathTimer += dt;
		position.ensureData();
		if (slowingToStopPathTimer >= slowingToStopPath)
		{

			position.data->pathTimeMultiplier = 1;

			idle();
			slowingToStopPath = 0;
			slowingToStopPathTimer = 0;
		}
		else
		{
			position.data->pathTimeMultiplier = 1.0f - (slowingToStopPathTimer / slowingToStopPath);
		}
	}

	maxSpeedLerp.update(dt);
	velocity.z = 0;
	vel.z = 0;

	bool wasFollowing = false;
	if (isFollowingPath())
		wasFollowing = true;

	if (burstTimer.updateCheck(dt))
	{
		position.ensureData();
		position.data->pathTimeMultiplier = 1;
	}

	if (poisonTimer.updateCheck(dt))
	{
		poison = 0;
	}

	if (currentState != STATE_DEATHSCENE && currentState != STATE_DEAD)
	{
		if (poison)
		{
			if (poisonBitTimer.updateCheck(dt))
			{
				poisonBitTimer.start(dsq->continuity.poisonBitTime);

				DamageData d;
				d.damageType = DT_ENEMY_ACTIVEPOISON;
				d.useTimer = false;
				d.damage = 0.5f*poison;
				damage(d);

				dsq->spawnParticleEffect("PoisonBubbles", position);
			}
		}
	}

	Quad::onUpdate(dt);

	Vector v = position - lastPos;
	lastMove = v;
	if (position.isFollowingPath() && swimPath)
	{
		movementDetails(v);
	}

	if (wasFollowing && !isFollowingPath())
	{
		onPathEnd();
	}
	multColor.update(dt);

	if (bubble)
		bubble->position = this->position;



	if (getState() == STATE_PUSH)
	{
		vel = pushVec;
	}

	if (stickToNaijasHead)
	{
		position = game->avatar->headPosition;

	}

	updateLance(dt);

	skeletalSprite.update(dt);
}

void Entity::updateBoneLock()
{
	if (boneLock.on)
	{
		Vector lastPosition = position;
		const Quad *lockObj = boneLock.bone;
		if(!lockObj)
			lockObj = boneLock.entity;

		Vector posAndRot = lockObj->getWorldPositionAndRotation();
		Vector currentOffset = getRotatedVector(boneLock.circleOffset, posAndRot.z - boneLock.origRot);
		posAndRot.z = 0;
		position = posAndRot + currentOffset;
		currentOffset.normalize2D();
		boneLock.wallNormal = currentOffset;
		rotateToVec(currentOffset, 0.01f);

		if (game->collideCircleWithGrid(position, collideRadius))
		{
			position = lastPosition;
			setBoneLock(BoneLock());
			return;
		}

		onUpdateBoneLock();
	}
}

std::string Entity::getIdleAnimName()
{
	return "idle";
}

void Entity::idle()
{
	if (isFollowingPath())
		stopFollowingPath();
	position.stopPath();
	perform(Entity::STATE_IDLE);
	skeletalSprite.stopAllAnimations();

	onIdle();

	skeletalSprite.transitionAnimate(getIdleAnimName(), 0.3f, -1);
	rotateToVec(Vector(0,-1),0.1f);
	vel.capLength2D(50);

	setRiding(0);
}

void Entity::updateLance(float dt)
{
	if(!lancedata)
		return;

	lancedata->timer -= dt;
	if (lancedata->timer < 0)
	{
		delete lancedata;
		lancedata = NULL;
	}
	else
	{
		lancedata->gfx->fhTo(_fh);
		lancedata->delay += dt;
		if (lancedata->delay > 0.1f)
		{
			lancedata->delay = 0;
			game->fireShot("Lance", this, 0, lancedata->gfx->getWorldCollidePosition(Vector(-64, 0)));
		}

		if (lancedata->bone)
		{
			Vector pr = lancedata->bone->getWorldPositionAndRotation();
			lancedata->gfx->position.x = pr.x;
			lancedata->gfx->position.y = pr.y;
			lancedata->gfx->rotation = pr.z;
		}
		else
		{
			lancedata->gfx->position = getWorldPosition();
			lancedata->gfx->rotation = rotation;
		}
	}
}

void Entity::attachLance()
{
	std::ostringstream os;
	os << "attaching lance to " << this->name;
	debugLog(os.str());
	lancedata = new LanceData();
	lancedata->timer = 8;
	lancedata->bone = skeletalSprite.getBoneByName("Lance");

	PauseQuad *q = new PauseQuad();
	q = new PauseQuad();
	q->setTexture("Particles/Lance");
	q->alpha = 0;
	q->alpha.interpolateTo(1, 0.5);
	game->addRenderObject(q, LR_PARTICLES);
	lancedata->gfx = q;
}

void Entity::setRiding(Entity *e)
{
	riding = e;
}

Entity* Entity::getRiding()
{
	return riding;
}

void Entity::rotateToVec(Vector addVec, float time, float offsetAngle)
{
	// HACK: this mucks up wall normals for some reason
//	if (vel.getSquaredLength2D() <= 0) return;
	if (addVec.x == 0 && addVec.y == 0)
	{
		rotation.interpolateTo(Vector(0,0,0), time, 0);
	}
	else
	{
		float angle = MathFunctions::getAngleToVector(addVec, offsetAngle);

		if (rotation.z <= -90 && angle >= 90)
		{
			rotation.z = 360 + rotation.z;
		}
		if (rotation.z >= 90 && angle <= -90)
			rotation.z = rotation.z - 360;



		if (time == 0)
			rotation = Vector(0,0,angle);
		else
			rotation.interpolateTo(Vector(0,0,angle), time, 0);
	}
}

void Entity::clearTargetPoints()
{
	targetPoints.clear();
}

void Entity::addTargetPoint(const Vector &point)
{
	targetPoints.push_back(point);
}

int Entity::getNumTargetPoints()
{
	return targetPoints.size();
}

Vector Entity::getTargetPoint(size_t i)
{
	if (i >= targetPoints.size())
		return getEnergyShotTargetPosition();
	return targetPoints[i];
}

int Entity::getRandomTargetPoint()
{
	if (targetPoints.empty())
		return 0;
	return rand()%targetPoints.size();
}

bool Entity::isUnderWater()
{
	return _isUnderWaterPos(position);
}

bool Entity::isUnderWater(const Vector& overridePos)
{
	return _isUnderWaterPos(overridePos.isZero() ? position : overridePos);
}

bool Entity::_isUnderWaterPos(const Vector& pos)
{
	UnderWaterResult res = game->isUnderWater(pos, collideRadius);
	waterBubble = res.waterbubble;
	return res.uw;
}

void Entity::push(const Vector &vec, float time, float maxSpeed, float dmg)
{
	if (!this->isEntityDead())
	{
		pushDamage = dmg;
		if (maxSpeed == 0)
		{
			maxSpeed = this->maxSpeed;
		}
		this->pushMaxSpeed = maxSpeed;

		setState(STATE_PUSH, time);
		pushVec = vec;
		pushVec.z = 0;
	}

}

void Entity::setMaxSpeed(float ms)
{
	maxSpeed = ms;
}

int Entity::getMaxSpeed()
{
	return maxSpeed;
}

void Entity::songNote(int note)
{
}

void Entity::songNoteDone(int note, float len)
{
}

void Entity::sound(const std::string &sound, float freq, float fadeOut)
{
	dsq->playPositionalSfx(sound, position, freq, fadeOut, this);
	updateSoundPosition();
}

Vector Entity::getEnergyShotTargetPosition()
{
	return getWorldPosition();
}

bool Entity::isTargetInRange(int range, size_t t)
{
	if (t >= targets.size())
	{
		std::ostringstream os;
		os << "isTargetInRange: invalid target index: " << t;
		debugLog(os.str());
		return false;
	}
	if (!targets[t])
	{
		debugLog ("null target");
		return false;
	}
	return ((targets[t]->position - this->position).getSquaredLength2D() < sqr(range));
}

void Entity::setEntityType(EntityType et)
{
	entityType = et;
}

EntityType Entity::getEntityType()
{
	return entityType;
}

/* types:

*/
Entity *Entity::findTarget(int dist, int type, int t)
{
	targets[t] = 0;

	if (type == ET_AVATAR)
	{
		Vector d = game->avatar->position - this->position;
		if (d.getSquaredLength2D() < sqr(dist))
		{
			targets[t] = game->avatar;
		}
	}
	else
	{
		int closestDist = -1;
		Entity *target = 0;
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e != this && e->getEntityType() == type && e->health > 0)
			{
				int d = (e->position - this->position).getSquaredLength2D();
				if (d < sqr(dist) && (d < closestDist || closestDist == -1))
				{
					closestDist = d;
					target = e;
				}
			}
		}
		if (target)
		{
			targets[t] = target;
		}
	}
	return targets[t];
}

void Entity::moveTowards(Vector p, float dt, int spd)
{
	Vector d = p - this->position;
	d.setLength2D(spd*dt);
	vel += d;
}

void Entity::moveAround(Vector p, float dt, int spd, int dir)
{
	Vector d = p - this->position;
	if (!dir)
		d = Vector(-d.y, d.x);
	else
		d = Vector(d.y, -d.x);
	d.setLength2D(spd*dt);
	vel += d;
}

void Entity::moveTowardsAngle(int angle, float dt, int spd)
{
	Vector p(sinf(MathFunctions::toRadians(angle))*16+position.x, cosf(MathFunctions::toRadians(angle))*16+position.y);
	moveTowards(p, dt, spd);
}

void Entity::moveAroundAngle(int angle, float dt, int spd, int dir)
{
	Vector p(sinf(MathFunctions::toRadians(angle))*16+position.x, cosf(MathFunctions::toRadians(angle))*16+position.y);
	moveAround(p, dt, spd, dir);
}

void Entity::moveTowardsTarget(float dt, int spd, int t)
{
	if (!targets[t]) return;
	moveTowards(targets[t]->position, dt, spd);
}

void Entity::moveAroundTarget(float dt, int spd, int dir, int t)
{
	if (!targets[t]) return;
	moveAround(targets[t]->position, dt, spd, dir);
}

Vector Entity::getLookAtPoint()
{
	if (lookAtPoint.isZero())
		return position;
	return lookAtPoint;
}


void Entity::moveAroundEntity(float dt, int spd, int dir, Entity *e)
{
	Vector d = e->position - this->position;
	if (!dir)
		d = Vector(-d.y, d.x);
	else
		d = Vector(d.y, -d.x);
	d.setLength2D(spd*dt);
	vel += d;
}

void Entity::onEnterState(int action)
{
	switch (action)
	{
	case STATE_DEAD:
	{
		if (!isGoingToBeEaten())
		{
			if (!deathSound.empty())
				sound(deathSound, (800 + rand()%400)/1000.0f);
		}
		else
		{
			sound("Gulp");
		}
		popBubble();

		Shot::targetDied(this);
		if (!calledEntityDied)
		{
			game->entityDied(this);
			calledEntityDied = true;
		}
		if (hair)
		{
			hair->setLife(1);
			hair->setDecayRate(10);
			hair->fadeAlphaWithLife = 1;
			hair = 0;
		}
		if(minimapIcon)
		{
			minimapIcon->alpha.interpolateTo(0, 0.1f);
		}

	}
	break;
	case STATE_PUSH:
	{
		setMaxSpeed(this->pushMaxSpeed);
	}
	break;
	}
}

bool Entity::isPullable()
{
	return ((isEntityProperty(EP_MOVABLE)) || (frozenTimer > 0));
}

void Entity::freeze(float time)
{
	//HACK: prevent freeze from freezing if the enemy won't deal with it properly
	//if (isv(EV_MOVEMENT, 0)) return;

	vel = 0;
	frozenTimer = time;

	disableMotionBlur();

	vel.capLength2D(100);

	onFreeze();
	if (!bubble)
	{
		bubble = new Quad;
		bubble->setTexture("spell-bubble");
		bubble->position = this->position;
		bubble->scale = Vector(0.2f,0.2f);
		bubble->scale.interpolateTo(Vector(2,2), 0.5f, 0, 0, 1);
		bubble->alpha.ensureData();
		bubble->alpha.data->path.addPathNode(0.5f, 0);
		bubble->alpha.data->path.addPathNode(0.5f, 0.75f);
		bubble->alpha.data->path.addPathNode(0, 1);
		bubble->alpha.startPath(time+time*0.25f);
		core->getTopStateData()->addRenderObject(bubble, LR_PARTICLES);

	}
}

void Entity::onExitState(int action)
{
	switch(action)
	{
	case STATE_PUSH:
	{
		setState(STATE_IDLE);
	}
	break;
	case STATE_PUSHDELAY:
	{
		setState(STATE_IDLE);
	}
	break;
	case STATE_DEATHSCENE:
	{
		setState(STATE_DEAD);
	}
	break;
	}



}

void Entity::popBubble()
{


	if (bubble)
	{
		frozenTimer = 0;
		sound ("Pop");
		bubble->setLife(1);
		bubble->setDecayRate(4);
		bubble->fadeAlphaWithLife = bubble->alpha.x;
		bubble = 0;
		dsq->spawnParticleEffect("PopEnemyBubble", position);
	}
}



bool Entity::isHit()
{
	return (damageTimer.isActive());
}

bool Entity::isInvincible()
{
	return (invincible);

}

void Entity::setInvincible(bool inv)
{
	invincible = inv;
}

bool Entity::isInDarkness()
{
	for (Element *e = dsq->getFirstElementOnLayer(12); e; e = e->bgLayerNext)
	{
		if (e->isCoordinateInside(position))
			return true;
	}
	return false;
}

bool Entity::canSetState(int state)
{
	if (enqueuedState == STATE_DEAD || currentState == STATE_DEAD || nextState == STATE_DEAD)
	{
		std::ostringstream os;
		os << "entity: " << name << " tried to set state to: " << state << " when in/entering dead";
		debugLog(os.str());

		return false;
	}
	else if (enqueuedState == STATE_DEATHSCENE || currentState == STATE_DEATHSCENE || nextState == STATE_DEATHSCENE)
	{
		if (state == STATE_DEAD)
			return true;
		else
		{
			std::ostringstream os;
			os << "entity: " << name << " tried to set state to: " << state << " when in/entering deathscene";
			debugLog(os.str());
			return false;
		}
	}
	return true;
}

bool Entity::updateLocalWarpAreas(bool affectAvatar)
{
	for (size_t i = 0; i < game->getNumPaths(); i++)
	{
		Path *p = game->getPath(i);
		if (!p->nodes.empty())
		{
			PathNode *n = &p->nodes[0];
			if (p && n)
			{
				if (p->warpMap.empty() && !p->warpNode.empty() && p->isCoordinateInside(position))
				{
					Path *p2 = game->getPathByName(p->warpNode);
					if (p2)
					{
						if (affectAvatar)
						{
							// HACK: do something in the script to get the avatar position
							game->avatar->position = this->position;

							game->preLocalWarp(p->localWarpType);
						}
						position = p2->getPathNode(0)->position;
						if (affectAvatar)
						{
							// HACK: do something in the script to get the avatar position
							game->avatar->position = this->position;

							game->postLocalWarp();
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}

void Entity::warpLastPosition()
{
	position = lastPosition;
}

void Entity::spawnParticlesFromCollisionMask(const char *p, unsigned intv, int layer, float rotz)
{
	for (size_t i = 0; i < skeletalSprite.bones.size(); i++)
		skeletalSprite.bones[i]->spawnParticlesFromCollisionMask(p, intv, layer, rotz);
}

//Entity *e, Entity *attacker, Bone *bone, SpellType spell, int dmg)
// not sure why this returns bool
// return true if did damage, else false
bool Entity::damage(const DamageData &dmgData)
{
	DamageData d = dmgData;
	if (d.damageType == DT_NONE)
		return false;

	if (isEntityDead())
	{
		//DUPE: same as below
		//HACK: hackish
		return false;
	}
	onDamage(d);
	lastDamage = d;
	if (invincibleBreak && damageTimer.isActive() && dmgData.useTimer) return false;

	this->multColor = Vector(1,1,1);
	this->multColor.stop();

	if (dmgData.useTimer)
		damageTimer.start(damageTime);


	//DUPE: same as above
	//HACK: hackish

	if (d.damageType == DT_AVATAR_BITE)
	{
		debugLog("Entity::damage bittenEntities.push_back");
		game->avatar->bittenEntities.push_back(this);
	}

	if (d.damage > 0 && frozenTimer)
	{
		popBubble();
	}

	//HACK: fish form freeze
	if (d.damageType == DT_AVATAR_BUBBLE && isDamageTarget(DT_AVATAR_BUBBLE))
	{
		freeze(30);
	}

	if (d.damageType == DT_AVATAR_VINE)
	{
		if (vineDamageTimer.isDone())
		{
			vineDamageTimer.start(0.25);
		}
		else
			return false;
	}

	if (d.damageType == DT_ENEMY_POISON)
	{
		if (getEntityType() != ET_AVATAR)
		{
			setPoison(1, d.effectTime);
		}
	}

	bool doDamage = !invincible;

	if (entityType == ET_AVATAR)
		doDamage = (!invincible || !game->invincibleOnNested);

	if (doDamage)
	{
		if (d.damage>0)
		{
			if (entityType == ET_AVATAR)
				this->multColor.interpolateTo(Vector(1, 0.1f, 0.1f), 0.1f, 14, 1);
			else
				this->multColor.interpolateTo(Vector(1, 0.1f, 0.1f), 0.1f, 4, 1);
		}

		health -= d.damage;
		if (health <= 0)
		{
			health = 0;
			entityDead = true;
			if (deathScene)
				setState(STATE_DEATHSCENE, 0);
			else
				setState(STATE_DEAD);
		}

		onHealthChange(-d.damage);
	}

	return true;
}

void Entity::clampToHit()
{
	Vector dist = game->lastCollidePosition - position;
	dist.setLength2D(collideRadius);
	position = game->lastCollidePosition + dist;
	setv(EV_CRAWLING, 1);

}



void Entity::doEntityAvoidance(float dt, int range, float mod, Entity *ignore)
{
	Vector accum;
	int c = 0;
	Vector diff;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;

		if (e != this && e != ignore && e->ridingOnEntity != this && !e->getv(EV_NOAVOID))
		{
			diff = (this->position - e->position);
			if (diff.isLength2DIn(range) && !diff.isZero())
			{
				diff.setLength2D(range - diff.getLength2D());
				accum += diff;
				c++;
			}
		}
	}
	if (accum.x != 0 || accum.y != 0)
	{
		accum /= c;
		accum /= range;
		vel += accum*getMaxSpeed()*mod;
	}
}

void Entity::render(const RenderState& rsold) const
{
	// This is special-cased for entities:
	// An entity that has a renderpass set is supposed to apply this to all
	// children regardless of their setting.
	// (In earlier versions this functionality was implemented via an overrideRenderPass
	// but that doesn't exist anymore)
	// -> Wait for the correct pass until we even bother to try rendering this entity
	if(renderPass != RENDER_ALL && rsold.pass != renderPass)
		return;

	RenderState rs(rsold);
	rs.color *= color;
	rs.scale *= flipScale;
	if (multColor.isInterpolating())
		rs.color *= multColor;
	rs.alpha *= alpha.x;


	if (game->isSceneEditorActive() && game->sceneEditor.editType == ET_ENTITIES)
	{
		if (game->sceneEditor.editingEntity == this)
			rs.renderBorderColor = Vector(1,1,1);
		else
			rs.renderBorderColor = Vector(0.5,0.5,0.5);
		rs.forceRenderBorder = true;
	}

	// if we have an override render pass set:
	// from this point, render all children in this pass
	// regardless of what they specify
	if(renderPass != RENDER_ALL && rs.pass == renderPass)
		rs.pass = RENDER_ALL;

	Quad::render(rs);
}

void Entity::doGlint(const Vector &position, const Vector &scale, const std::string &tex, BlendType bt)
{
	float glintTime = 0.4f;
	Quad *glint = new Quad;

	glint->setBlendType(bt);
	glint->setTexture(tex);
	glint->scale = Vector(0.5f,0.5f);
	glint->position = position;
	glint->scale.interpolateTo(scale, glintTime);
	glint->alpha.ensureData();
	glint->alpha.data->path.addPathNode(1, 0);
	glint->alpha.data->path.addPathNode(1, 0.7f);
	glint->alpha.data->path.addPathNode(0, 1);
	glint->alpha.startPath(glintTime);

	glint->rotation.z = this->rotation.z;
	glint->setLife(glintTime);
	glint->setDecayRate(1);
	core->getTopStateData()->addRenderObject(glint, LR_PARTICLES);
}

void Entity::addIgnoreShotDamageType(DamageType dt)
{
	ignoreShotDamageTypes.push_back(dt);
}

void Entity::doSpellAvoidance(float dt, int range, float mod)
{
	Vector accum;

	int c = 0;
	for (Shot::Shots::iterator i = Shot::shots.begin(); i != Shot::shots.end(); i++)
	{
		Shot *s = (Shot*)(*i);

		if (s->isActive() && (s->position - this->position).getSquaredLength2D() < sqr(range))
		{
			for (size_t j = 0; j < ignoreShotDamageTypes.size(); j++)
			{
				if (s->getDamageType() == ignoreShotDamageTypes[j])
				{
					continue;
				}
			}
			Vector d = this->position - s->position;
			if (!d.isZero())
			{
				d.setLength2D(range - d.getLength2D());
			}
			accum += d;
			c++;
		}
	}
	if (accum.x != 0 || accum.y != 0)
	{
		accum /= c;
		accum /= range;
		vel += accum*getMaxSpeed()*mod;
	}
}

void Entity::fillGrid()
{
	if (fillGridFromQuad)
	{
		game->fillGridFromQuad(this, OT_INVISIBLEENT);
	}
}

// caller must make sure that the ID is unused
void Entity::setID(int id)
{
	entityID = id;
}

int Entity::getID()
{

	return entityID;
}

void Entity::shotHitEntity(Entity *hit, Shot *shot, Bone *b)
{
}

bool Entity::doCollisionAvoidance(float dt, int search, float mod, Vector *vp, float overrideMaxSpeed, int ignoreObs, bool onlyVP)
{
	Vector accum;
	int c = 0;
	bool isInWaterBubble = false;
	if (waterBubble && isUnderWater())
	{
		//debugLog("collision avoidance in bubble");
		isInWaterBubble = true;
		if (!canLeaveWater)
		{
			Vector a = position - waterBubble->nodes[0].position;
			Vector b = a;
			b.setLength2D((waterBubble->rect.getWidth()*0.5f) - b.getLength2D());
			if (b.isLength2DIn(search*TILE_SIZE))
			{

				accum -= b;
				c++;


			}
		}
	}
	if (!overrideMaxSpeed)
		overrideMaxSpeed = getMaxSpeed();
	if (vp == 0)
		vp = &this->vel;
	Vector vel = *vp;
	int minDist=-1;
	TileVector t(position);
	TileVector useTile;
	const int blockObs = ~ignoreObs;

	float totalDist = sqrtf(float(sqr((search*2)*TILE_SIZE)+sqr((search*2)*TILE_SIZE)));
	for (int x = -search; x <= search; x++)
	{
		for (int y = -search; y <= search; y++)
		{
			TileVector checkT(t.x+x, t.y+y);
			bool waterBlocked=false;
			if (!isInWaterBubble && !canLeaveWater && checkT.worldVector().y - collideRadius < game->getWaterLevel())
			{
				waterBlocked = true;
			}
			if (waterBlocked || game->isObstructed(checkT))
			{
				if (game->isObstructed(checkT, blockObs))
				{
					Vector vtc(t.x+x, t.y+y);
					Vector vt(t.x, t.y);
					int dist = (vt-vtc).getSquaredLength2D();
					if (minDist == -1 || dist<minDist)
					{
						minDist = dist;
						useTile = TileVector(t.x+x, t.y+y);
					}
					Vector v = position - checkT.worldVector();
					accum += v;

					c++;
				}
			}
		}
	}
	if (c > 0)
	{
		accum /= float(c) * (totalDist/2);
		accum.setLength2D(1.0f - accum.getLength2D());
		if (onlyVP)
		{
			*vp += accum*overrideMaxSpeed*mod;
			if (!(*vp).isLength2DIn(overrideMaxSpeed))
				(*vp).capLength2D(overrideMaxSpeed);
		}
		else
			vel += accum*overrideMaxSpeed*mod;
	}
	if (!onlyVP)
		*vp = vel;
	if (c > 0)
		return true;
	return false;
}

void Entity::initHair(int numSegments, float segmentLength, float width, const std::string &tex)
{
	if (hair)
	{
		errorLog("Trying to init hair when hair is already present");
	}
	hair = new Hair(numSegments, segmentLength, width);
	hair->setTexture(tex);
	game->addRenderObject(hair, layer);
}


void Entity::setHairHeadPosition(const Vector &pos)
{
	if (hair)
	{
		hair->setHeadPosition(pos);
	}
}

void Entity::updateHair(float dt)
{
	if (hair)
	{
		hair->updatePositions();
	}
}

void Entity::exertHairForce(const Vector &force, float dt)
{
	if (hair)
	{
		hair->exertForce(force, dt);
	}
}

bool Entity::isEntityInside() const
{
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e && e->life == 1 && e != this && e->ridingOnEntity != this && isCoordinateInside(e->position))
			return true;

	}
	return false;
}

void Entity::updateSoundPosition()
{
	SoundHolder::updateSoundPosition(position.x + offset.x, position.y + offset.y);
}

MinimapIcon *Entity::ensureMinimapIcon()
{
	if(!minimapIcon)
		minimapIcon = new MinimapIcon;
	return minimapIcon;
}

bool Entity::isNormalLayer() const
{
	return layer == LR_ENTITIES || layer == LR_ENTITIES0 || layer == LR_ENTITIES2 || layer == LR_ENTITIES_MINUS2 || layer == LR_ENTITIES_MINUS3;
}
