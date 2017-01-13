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


void Entity::stopPull()
{
	if (dsq->game->avatar->pullTarget == this)
	{
		dsq->game->avatar->pullTarget = 0;
	}
}

void Entity::setIngredientData(const std::string &name)
{
	ingredientData = dsq->continuity.getIngredientDataByName(name);
}

void Entity::entityDied(Entity *e)
{
	for (int i = 0; i < targets.size(); i++)
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
		for (int i = 0; i < skeletalSprite.bones.size(); i++)
		{
			if (skeletalSprite.bones[i]->generateCollisionMask)
			{
				dsq->game->generateCollisionMask(skeletalSprite.bones[i], ovrCollideRadius);
			}
		}
	}
}



bool Entity::setBoneLock(const BoneLock &boneLock)
{
	if (!canSetBoneLock()) return false;

	if (boneLock.on && boneLockDelay > 0) return false;
	if (this->boneLock.on && boneLock.on) return false;

	if (this->boneLock.on && !boneLock.on)
	{
		boneLockDelay = 0.1;
		this->boneLock = boneLock;
	}
	else
	{
		if (!boneLock.entity)
			return false;

		if (boneLock.entity && !boneLock.bone)
		{
			this->boneLock = boneLock;
			this->boneLock.circleOffset = this->position - (boneLock.entity->getWorldPosition());
			this->boneLock.circleOffset.setLength2D(boneLock.entity->collideRadius);
			this->boneLock.origRot = boneLock.entity->rotation.z;



		}
		else
		{
			this->boneLock = boneLock;

			this->boneLock.localOffset = this->position - (boneLock.bone->getWorldPosition());
			this->boneLock.localOffset = boneLock.bone->getInvRotPosition(this->boneLock.localOffset);
			this->boneLock.origRot = boneLock.bone->getWorldRotation();


		}
	}

	setv(EV_BONELOCKED, boneLock.on);

	onSetBoneLock();

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
	setvf(EV_CLAMPTRANSF, 0.2);
	setv(EV_FLIPTOPATH, 1);
	setv(EV_NOINPUTNOVEL, 1);
	setv(EV_VINEPUSH, 1);
	setv(EV_BEASTBURST, 1);
	setv(EV_WEBSLOW, 100);



	invincible = false;
	lanceDelay = 0;
	lance = 0;
	lanceTimer = 0;
	lanceGfx = 0;
	lanceBone = 0;
	beautyFlip = true;
	fhScale = fvScale = 0;
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
	entityProperties.resize(EP_MAX);
	for (int i = 0; i < entityProperties.size(); i++)
	{
		entityProperties[i] = false;
	}
	entityTypeIdx = -1;
	damageTime = vars->entityDamageTime;
	slowingToStopPathTimer = 0;
	slowingToStopPath = 0;
	followPos = 0;
	watchingEntity = 0;
	swimPath = false;
	currentEntityTarget = 0;
	deleteOnPathEnd = false;
	multColor = Vector(1,1,1);
	collideRadius = 24;
	entityType = EntityType(0);
	targets.resize(10);
	attachedTo = 0;

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


	followEntity = 0;
	bubble = 0;



	skeletalSprite.updateAfterParent = 1;
	skeletalSprite.setAnimationKeyNotify(this);
	addChild(&skeletalSprite, PM_STATIC);



	setDamageTarget(DT_AVATAR_NATURE, false);
	setDamageTarget(DT_AVATAR_LIZAP, true);

	setDamageTarget(DT_AVATAR_BUBBLE, false);
	setDamageTarget(DT_AVATAR_SEED, false);

	stopSoundsOnDeath = false;
	minimapIcon = 0;



}

Entity::~Entity()
{
	delete minimapIcon;
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
	if (wasUnderWater && !isUnderWater(o))
	{
		sound("splash-outof");
		changed = true;
		wasUnderWater = false;
	}
	else if (!wasUnderWater && isUnderWater(o))
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
	entityProperties[int(ep)] = value;
}

bool Entity::isEntityProperty(EntityProperty ep)
{
	return entityProperties[int(ep)];
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
		for (int i = 0; i < p->nodes.size(); i++)
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
	followEntity = 0;


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

void Entity::watchEntity(Entity *e)
{
	watchingEntity = e;
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
			v = dsq->game->getWallNormal(position);
		else
			v = dsq->game->getWallNormal(position, n);
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
				if (dsq->game->isObstructed(TileVector(pos - Vector(bit,0))))
				{
					TileVector t(pos - Vector(backBit,0));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (dsq->game->isObstructed(TileVector(pos - Vector(0,bit))))
				{
					TileVector t(pos - Vector(0,backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (dsq->game->isObstructed(TileVector(pos + Vector(bit,0))))
				{
					TileVector t(pos + Vector(backBit,0));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (dsq->game->isObstructed(TileVector(pos + Vector(0,bit))))
				{
					TileVector t(pos + Vector(0,backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (dsq->game->isObstructed(TileVector(pos + Vector(-bit,-bit))))
				{
					TileVector t(pos + Vector(-backBit,-backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (dsq->game->isObstructed(TileVector(pos + Vector(-bit,bit))))
				{
					TileVector t(pos + Vector(-backBit,backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (dsq->game->isObstructed(TileVector(pos + Vector(bit,-bit))))
				{
					TileVector t(pos + Vector(backBit,-backBit));
					pos = t.worldVector();
					clamped = true;
					break;
				}
				if (dsq->game->isObstructed(TileVector(pos + Vector(bit,bit))))
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
				if (dsq->game->isObstructed(TileVector(pos + v*TILE_SIZE*i)))
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
		Vector n = dsq->game->getWallNormal(pos);
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
				dsq->game->spawnManaBall(position, manaBallEnergy);
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
			alpha.interpolateTo(0.01,1);
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
			alpha.interpolateTo(0.01,1);
		}
		scale.interpolateTo(Vector(0,0), 1);
		stickToNaijasHead = true;
	}
	activationType = ACT_NONE;

	if (ingredientData)
	{
		dsq->game->spawnIngredientFromEntity(this, ingredientData);
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
				if (dsq->game->isObstructed(test))
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
			if (dsq->game->isObstructed(test))
			{
				v = true;
				break;
			}
		}
		for (int y = -sz; y <= sz; y++)
		{
			test = TileVector(t.x, t.y+y);
			if (dsq->game->isObstructed(test))
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
			if (dsq->game->isObstructed(test))
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
			if (dsq->game->isObstructed(test))
			{
				v = true;
				break;
			}
			test = TileVector(t.x-d, t.y);
			if (dsq->game->isObstructed(test))
			{
				v = true;
				break;
			}

			test = TileVector(t.x, t.y+d);
			if (dsq->game->isObstructed(test))
			{
				v = true;
				break;
			}
			test = TileVector(t.x, t.y-d);
			if (dsq->game->isObstructed(test))
			{
				v = true;
				break;
			}

			test = TileVector(t.x-d, t.y-d);
			if (dsq->game->isObstructed(test))
			{
				v = true;
				break;
			}
			test = TileVector(t.x-d, t.y+d);
			if (dsq->game->isObstructed(test))
			{
				v = true;
				break;
			}

			test = TileVector(t.x+d, t.y-d);
			if (dsq->game->isObstructed(test))
			{
				v = true;
				break;
			}
			test = TileVector(t.x+d, t.y+d);
			if (dsq->game->isObstructed(test))
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
	if (isv(EV_BEASTBURST, 1) && isDamageTarget(DT_AVATAR_BITE) && dsq->continuity.form == FORM_BEAST && dsq->game->avatar->bursting)
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
	if (radius == 0 || (dsq->game->avatar->getWorldPosition() - usePosition).getSquaredLength2D() < sqr(radius+dsq->game->avatar->collideRadius))
	{
		if (dmg > 0)
		{
			DamageData d;
			d.damage = dmg;
			d.attacker = this;
			dsq->game->avatar->damage(d);
		}
		if (pushTime > 0 && speed > 0)
		{
			Vector diff = dsq->game->avatar->position - position;
			diff.setLength2D(speed);


			dsq->game->avatar->push(diff, pushTime, speed, dmg);
		}
		else if (speed > 0)
		{
			dsq->game->avatar->fallOffWall();
			Vector diff = dsq->game->avatar->position - position;
			diff.setLength2D(speed);
			dsq->game->avatar->vel += diff;
		}


		return true;
	}
	return false;
}

const float sct = 0.15;

void Entity::onFHScale()
{
	flipScale.interpolateTo(Vector(1, 1), sct);
	_fh = !_fh;



	fhScale = 0;
}

void Entity::onFH()
{
	if (!beautyFlip) return;
	_fh = !_fh;
	if (!fhScale)
	{
		flipScale = Vector(1,1);



		flipScale.interpolateTo(Vector(0.6, 1), sct);



		fhScale = 1;
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
	if (doUpdate && !(pauseFreeze && dsq->game->isPaused()))
	{

		if (!(getEntityType() == ET_AVATAR || getEntityType() == ET_INGREDIENT))
		{
			if (spiritFreeze && dsq->game->isWorldPaused())
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
		dsq->game->playBurstSound(wallJump);
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
			offset.interpolateTo(Vector(0, 0), 0.4);
			rotateToVec(Vector(0,-1), 0.1, 0);
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
			rotateToVec(v, 0.05);
		else
			rotateToVec(v, 0.2);
		Animation *anim = skeletalSprite.getCurrentAnimation();
		if (!burstTimer.isActive())
		{
			if (!anim || anim->name != "swim")
				skeletalSprite.transitionAnimate("swim", 0.1, -1);
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

		if (!dsq->game->isWorldPaused())
		{
			for (Path *p = dsq->game->getFirstPathOfType(PATH_CURRENT); p; p = p->nextOfType)
			{
				if (p->active)
				{
					for (int n = 1; n < p->nodes.size(); n++)
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
			if (!(this->getEntityType() == ET_AVATAR && dsq->game->avatar->canSwimAgainstCurrents() && dsq->game->avatar->bursting))
			{
				doCollisionAvoidance(1, 4, 1, &vel2, useLen);
			}

			doCollisionAvoidance(dt, 3, 1, 0, useLen);

			Vector dist = -vel2;
			dist.normalize2D();
			float v = dist.x;
			float scale = 0.2;
			if (getEntityType() == ET_AVATAR)
			{
				if (v < 0)
					dsq->rumble((-v)*scale, (1.0f+v)*scale, 0.2, dsq->game->avatar->getLastActionSourceID());
				else
					dsq->rumble((1.0f-v)*scale, (v)*scale, 0.1, dsq->game->avatar->getLastActionSourceID());
			}
		}
	}
	if (this->getEntityType() == ET_AVATAR && dsq->game->avatar->canSwimAgainstCurrents())
	{
		int cap = 100;
		if (!vel.isZero())
		{
			if (vel.dot2D(vel2) < 0 ) // greater than 90 degrees
			{
				// against current
				if (dsq->game->avatar->bursting)
					vel2 = 0;
				else if (dsq->game->avatar->isSwimming())
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
			if (dsq->game->isObstructed(TileVector(t.x+x, t.y+y), OT_INVISIBLEIN))
				return true;
			if (dsq->game->isObstructed(TileVector(t.x-x, t.y+y), OT_INVISIBLEIN))
				return true;
			if (dsq->game->isObstructed(TileVector(t.x+x, t.y-y), OT_INVISIBLEIN))
				return true;
			if (dsq->game->isObstructed(TileVector(t.x-x, t.y-y), OT_INVISIBLEIN))
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
	while (dsq->game->isObstructed(t))
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
	if (v)
	{
		for(size_t i = 0; i < disabledDamageTypes.size(); ++i)
		{
			if(disabledDamageTypes[i] == dt)
			{
				disabledDamageTypes[i] = disabledDamageTypes.back();
				disabledDamageTypes.pop_back();
				break;
			}
		}
	}
	else
	{
		if(isDamageTarget(dt))
			disabledDamageTypes.push_back(dt);
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
	const size_t sz = disabledDamageTypes.size();
	for(size_t i = 0; i < sz; ++i)
		if(disabledDamageTypes[i] == dt)
			return false;
	return true;
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
		dsq->game->entityDied(this);
		calledEntityDied = true;
	}
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

		switch (fhScale)
		{
		case 1:
			if (!flipScale.isInterpolating())
				onFHScale();
		break;
		}


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
	else
	{
		if (watchingEntity)
		{
			Vector v = position - watchingEntity->position;
			if (v.x < 0)
			{
				if (!isfh())
					flipHorizontal();
			}
			else if (v.x > 0)
			{
				if (isfh())
					flipHorizontal();
			}
		}
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

	else if (followEntity)
	{
		Vector lastPos = position;
		Vector off;
		int sz = 96;
		if (followEntity->vel.getSquaredLength2D() > sqr(1))
		{
			off = followEntity->vel.getPerpendicularLeft();
			switch (followPos)
			{
			case 0:		off.setLength2D(sz);		break;
			case 1:		off.setLength2D(-sz);		break;
			}
		}
		else if (followEntity->lastMove.getSquaredLength2D() > sqr(1))
		{

			off = followEntity->lastMove.getPerpendicularLeft();
			switch (followPos)
			{
			case 0:		off.setLength2D(sz);		break;
			case 1:		off.setLength2D(-sz);		break;
			}
		}
		Vector mov = followEntity->position + off - this->position;
		if (mov.getSquaredLength2D() > sqr(96))
		{

			int spd = mov.getLength2D();
			spd -= 64;
			if (spd < 0)
				spd = 0;
			else if (spd < 400)
				spd *= 2;

			else
				spd = 800;

			mov.setLength2D(spd);
			position += mov * dt;
			Vector diff = position - lastPos;
			movementDetails(diff);
		}
		else
		{
			Animation *anim = skeletalSprite.getCurrentAnimation();
			if (!anim || anim->name != "idle")
				idle();
		}
	}

	if (stickToNaijasHead)
	{
		position = dsq->game->avatar->headPosition;

	}

	updateLance(dt);
}

void Entity::updateBoneLock()
{
	if (boneLock.on)
	{


		Vector lastPosition = position;

		if (boneLock.bone)
		{
			position = boneLock.bone->transformedCollisionMask[boneLock.collisionMaskIndex];
			boneLock.wallNormal = boneLock.bone->getCollisionMaskNormal(boneLock.collisionMaskIndex);
			rotateToVec(boneLock.wallNormal, 0.01);
		}
		else
		{
			Vector currentOffset = getRotatedVector(boneLock.circleOffset, boneLock.entity->rotation.z - boneLock.origRot);
			position = boneLock.entity->getWorldPosition() + currentOffset;
			boneLock.wallNormal = currentOffset;
			boneLock.wallNormal.normalize2D();
			rotateToVec(boneLock.wallNormal, 0.01);
		}

		if (dsq->game->collideCircleWithGrid(position, collideRadius))
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

	skeletalSprite.transitionAnimate(getIdleAnimName(), 0.3, -1);
	rotateToVec(Vector(0,-1),0.1);
	vel.capLength2D(50);

	setRiding(0);
}

void Entity::updateLance(float dt)
{
	if (lance == 1)
	{
		lanceTimer -= dt;
		if (lanceTimer < 0)
		{
			lance = 0;
			lanceGfx->setLife(1.0);
			lanceGfx->setDecayRate(2);
			lanceGfx->fadeAlphaWithLife = 1;
			lanceGfx = 0;
			lanceTimer = 0;
		}
		else
		{
			lanceGfx->fhTo(_fh);
			lanceDelay = lanceDelay + dt;
			if (lanceDelay > 0.1f)
			{
				lanceDelay = 0;
				dsq->game->fireShot("Lance", this, 0, lanceGfx->getWorldCollidePosition(Vector(-64, 0)));
			}

			if (lanceBone != 0)
			{
				lanceGfx->position = lanceBone->getWorldPosition();
				lanceGfx->rotation = lanceBone->getWorldRotation();
			}
			else
			{
				lanceGfx->position = getWorldPosition();
				lanceGfx->rotation = rotation;
			}

		}
	}
}

void Entity::attachLance()
{
	std::ostringstream os;
	os << "attaching lance to " << this->name;
	debugLog(os.str());
	lance = 1;
	lanceBone = 0;
	if (!lanceGfx)
	{
		lanceGfx = new PauseQuad();
		lanceGfx->setTexture("Particles/Lance");
		lanceGfx->alpha = 0;
		lanceGfx->alpha.interpolateTo(1, 0.5);
		dsq->game->addRenderObject(lanceGfx, LR_PARTICLES);
	}
	lanceTimer = 8;
	lanceBone = skeletalSprite.getBoneByName("Lance");
}

void Entity::setRiding(Entity *e)
{
	riding = e;
}

Entity* Entity::getRiding()
{
	return riding;
}

void Entity::attachEntity(Entity *e, Vector offset)
{
	attachedEntities.push_back(e);

	attachedEntitiesOffsets.push_back(offset);
	e->attachedTo = this;

}

void Entity::detachEntity(Entity *e)
{
	e->attachedTo = 0;
	std::vector<Entity*>copyEnts = attachedEntities;
	std::vector<Vector> copyOffs = attachedEntitiesOffsets;

	attachedEntities.clear();
	attachedEntitiesOffsets.clear();

	for (int i = 0; i < copyEnts.size(); i++)
	{
		if (copyEnts[i] != e)
		{
			attachedEntities.push_back(copyEnts[i]);
			attachedEntitiesOffsets.push_back(copyOffs[i]);
		}
	}
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

Vector Entity::getTargetPoint(int i)
{
	if (i >= targetPoints.size() || i < 0)
		return getEnergyShotTargetPosition();
	return targetPoints[i];
}

int Entity::getRandomTargetPoint()
{
	if (targetPoints.empty())
		return 0;
	return rand()%targetPoints.size();
}

bool Entity::isUnderWater(const Vector &override)
{
	Vector check = position;
	if (!override.isZero())
		check = override;

	if (dsq->game->useWaterLevel && dsq->game->waterLevel.x > 0 && check.y-collideRadius > dsq->game->waterLevel.x)
		return true;


	Path *p = dsq->game->getNearestPath(position, PATH_WATERBUBBLE);
	if (p && p->active && p->isCoordinateInside(position, collideRadius))
	{
		waterBubble = p;
		return true;
	}

	if (!dsq->game->useWaterLevel || dsq->game->waterLevel.x == 0) return true;
	else
	{
		if (check.y-collideRadius > dsq->game->waterLevel.x)
		{
			waterBubble = 0;
			return true;
		}
	}

	return false;
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

bool Entity::isTargetInRange(int range, int t)
{
	if (t < 0 || t >= targets.size())
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
		Vector d = dsq->game->avatar->position - this->position;
		if (d.getSquaredLength2D() < sqr(dist))
		{
			targets[t] = dsq->game->avatar;
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
			dsq->game->entityDied(this);
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
		bubble->scale = Vector(0.2,0.2);
		bubble->scale.interpolateTo(Vector(2,2), 0.5, 0, 0, 1);
		bubble->alpha.ensureData();
		bubble->alpha.data->path.addPathNode(0.5, 0);
		bubble->alpha.data->path.addPathNode(0.5, 0.75);
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
	for (int i = 0; i < dsq->game->getNumPaths(); i++)
	{
		Path *p = dsq->game->getPath(i);
		if (!p->nodes.empty())
		{
			PathNode *n = &p->nodes[0];
			if (p && n)
			{
				if (p->warpMap.empty() && !p->warpNode.empty() && p->isCoordinateInside(position))
				{
					Path *p2 = dsq->game->getPathByName(p->warpNode);
					if (p2)
					{
						if (affectAvatar)
						{
							// HACK: do something in the script to get the avatar position
							dsq->game->avatar->position = this->position;

							dsq->game->preLocalWarp(p->localWarpType);
						}
						position = p2->getPathNode(0)->position;
						if (affectAvatar)
						{
							// HACK: do something in the script to get the avatar position
							dsq->game->avatar->position = this->position;

							dsq->game->postLocalWarp();
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

void Entity::spawnParticlesFromCollisionMask(const std::string &p, int intv)
{
	for (int i = 0; i < skeletalSprite.bones.size(); i++)
	{
		for (int j = 0; j < skeletalSprite.bones[i]->collisionMask.size(); j+=intv)
		{
			Vector pos = skeletalSprite.bones[i]->getWorldCollidePosition(skeletalSprite.bones[i]->collisionMask[j]);
			dsq->spawnParticleEffect(p, pos);
		}
	}
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
		dsq->game->avatar->bittenEntities.push_back(this);
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
		doDamage = (!invincible || !dsq->game->invincibleOnNested);

	if (doDamage)
	{
		if (d.damage>0)
		{
			if (entityType == ET_AVATAR)
				this->multColor.interpolateTo(Vector(1, 0.1, 0.1), 0.1, 14, 1);
			else
				this->multColor.interpolateTo(Vector(1, 0.1, 0.1), 0.1, 4, 1);
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
	Vector dist = dsq->game->lastCollidePosition - position;
	dist.setLength2D(collideRadius);
	position = dsq->game->lastCollidePosition + dist;
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

void Entity::render()
{
	InterpolatedVector bcolor = color;
	InterpolatedVector bscale = scale;

	scale *= flipScale;
	if (multColor.isInterpolating())
	{
		color *= multColor;
	}

#ifdef AQUARIA_BUILD_SCENEEDITOR
	if (dsq->game->isSceneEditorActive() && dsq->game->sceneEditor.editType == ET_ENTITIES)
	{
		if (dsq->game->sceneEditor.editingEntity == this)
			renderBorderColor = Vector(1,1,1);
		else
			renderBorderColor = Vector(0.5,0.5,0.5);
		renderBorder = true;

	}
#endif

	// HACK: need to multiply base + etc
	skeletalSprite.setColorMult(this->color, this->alpha.x);


	Quad::render();



	renderBorder = false;
	skeletalSprite.clearColorMult();
	color = bcolor;
	scale = bscale;
}

void Entity::doGlint(const Vector &position, const Vector &scale, const std::string &tex, RenderObject::BlendTypes bt)
{
	float glintTime = 0.4;
	Quad *glint = new Quad;

	glint->setBlendType(bt);
	glint->setTexture(tex);
	glint->scale = Vector(0.5,0.5);
	glint->position = position;
	glint->scale.interpolateTo(scale, glintTime);
	glint->alpha.ensureData();
	glint->alpha.data->path.addPathNode(1, 0);
	glint->alpha.data->path.addPathNode(1, 0.7);
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
	BBGE_PROF(Entity_doSpellAvoidance);
	Vector accum;

	int c = 0;
	for (Shot::Shots::iterator i = Shot::shots.begin(); i != Shot::shots.end(); i++)
	{
		Shot *s = (Shot*)(*i);

		if (s->isActive() && (s->position - this->position).getSquaredLength2D() < sqr(range))
		{
			for (int j = 0; j < ignoreShotDamageTypes.size(); j++)
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
		dsq->game->fillGridFromQuad(this, OT_INVISIBLEENT);
	}
}

void Entity::assignUniqueID(bool temporary)
{
	const int inc = temporary ? -1 : 1;
	int id = inc;
	while (1)
	{
		bool isFree = true;
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e != this)
			{
				if (e->getID() == id)
				{
					isFree = false;
					break;
				}
			}
		}
		if (isFree)
		{
			break;
		}
		id += inc;
	}
	entityID = id;
}

void Entity::setID(int id)
{
	entityID = id;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e != this)
		{
			if (e->getID() == entityID)
			{
				std::ostringstream os;
				os << "ID conflict between " << name << " and " << e->name;
				debugLog(os.str());
				e->assignUniqueID(e->getID() < 0);
			}
		}
	}
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
			if (!isInWaterBubble && !canLeaveWater && checkT.worldVector().y - collideRadius < dsq->game->getWaterLevel())
			{
				waterBlocked = true;
			}
			if (waterBlocked || dsq->game->isObstructed(checkT))
			{
				if (dsq->game->isObstructed(checkT, blockObs))
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
	dsq->game->addRenderObject(hair, layer);
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

bool Entity::isEntityInside()
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
