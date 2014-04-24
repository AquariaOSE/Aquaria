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
#include "Shot.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"

#include "../BBGE/MathFunctions.h"

Shot::Shots Shot::shots;
Shot::Shots Shot::deleteShots;
Shot::ShotBank Shot::shotBank;
unsigned int Shot::shotsIter = 0;

std::string Shot::shotBankPath = "";

ShotData::ShotData()
{
	avatarKickBack= 0;
	avatarKickBackTime = 0;
	effectTime = 0;
	rotIncr = 0;
	alwaysDoHitEffects = 1;
	bounceType = BOUNCE_NONE;
	segments = false;
	damage = 0;
	maxSpeed = 800;
	homing = 300;
	scale = Vector(1,1);
	segScale = Vector(1,1);
	numSegs = 0;
	segDist = 16;
	blendType = RenderObject::BLEND_DEFAULT;
	collideRadius = 8;
	damageType = DT_ENEMY_ENERGYBLAST;
	lifeTime = 8;
	spinSpeed = 0;
	segTaper = 0;
	hitWalls = true;
	invisible = checkDamageTarget = false;
	homingMax = 0;
	homingIncr = 0;
	dieOnHit = 1;
	dieOnKill = false;
	hitEnts = 1;
	wallHitRadius = 0;
	rotateToVel = 1;
	waveSpeed = waveMag = 0;
	ignoreShield = false;
}

template <typename T> void readEquals2(T &in)
{
	std::string temp;
	in >> temp;
}

void Shot::updatePosition()
{
	if (emitter)
	{
		emitter->position = this->position + offset;
	}
}

void ShotData::bankLoad(const std::string &file, const std::string &path)
{
	std::string usef = path + file + ".txt";

	// FIXME: Li's attack and the pet blaster's energy balls are missing
	// the CheckDamageTarget flag, preventing entities (such as seahorses)
	// from properly ignoring the shots.  In lieu of modifying the
	// separately-distributed data files, we add a hack here to set the
	// flag on those two shot types.
	if (nocasecmp(file,"li") == 0 || nocasecmp(file,"petblasterfire") == 0)
	{
		checkDamageTarget = true;
	}

	this->name = file;
	stringToLower(this->name);

	debugLog(usef);
	char *data = readFile(core->adjustFilenameCase(usef).c_str());
	if (!data)
		return;
	SimpleIStringStream inf(data, SimpleIStringStream::TAKE_OVER);
	std::string token;
	while (inf >> token)
	{
		readEquals2(inf);
		if (token == "TrailPrt")
			inf >> trailPrt;
		else if (token == "Texture")
			inf >> texture;
		else if (token == "Scale")
			inf >> scale.x >> scale.y;
		else if (token == "BounceSfx")
			inf >> bounceSfx;
		else if (token == "HitSfx")
			inf >> hitSfx;
		else if (token == "FireSfx")
			inf >> fireSfx;
		else if (token == "HitPrt")
			inf >> hitPrt;
		else if (token == "BouncePrt")
			inf >> bouncePrt;
		else if (token == "FirePrt")
			inf >> firePrt;
		else if (token == "NumSegs")
			inf >> numSegs;
		else if (token == "SegDist")
			inf >> segDist;
		else if (token == "SegTexture")
			inf >> segTexture;
		else if (token == "SegTaper")
			inf >> segTaper;
		else if (token == "SegScale")
			inf >> segScale.x >> segScale.y;
		else if (token == "Homing")
			inf >> homing;
		else if (token == "HomingIncr")
			inf >> homingIncr;
		else if (token == "HomingMax")
			inf >> homingMax;
		else if (token == "MaxSpeed")
			inf >> maxSpeed;
		else if (token == "Gravity")
			inf >> gravity.x >> gravity.y;
		else if (token == "Spin")
			inf >> spinSpeed;
		else if (token == "AvatarKickBack")
			inf >> avatarKickBack;
		else if (token == "AvatarKickBackTime")
			inf >> avatarKickBackTime;
		else if (token == "CollideRadius")
			inf >> collideRadius;
		else if (token == "RotIncr")
			inf >> rotIncr;
		else if (token == "RotateToVel")
			inf >> rotateToVel;
		else if (token == "WallHitRadius")
			inf >> wallHitRadius;
		else if (token == "Wave")
			inf >> waveMag >> waveSpeed;
		else if (token == "BounceType" || token == "Blend")
		{
			std::string bt;
			inf >> bt;
			if (bt == "BOUNCE_NONE")
				bounceType = BOUNCE_NONE;
			else if (bt == "BOUNCE_REAL")
				bounceType = BOUNCE_REAL;
		}
		else if (token == "BlendType")
		{
			std::string bt;
			inf >> bt;
			if (bt == "BLEND_ADD")
				blendType = RenderObject::BLEND_ADD;
		}
		else if (token == "Damage")
		{
			inf >> damage;
		}
		else if (token == "EffectTime")
			inf >> effectTime;
		else if (token == "LifeTime" || token == "Life")
			inf >> lifeTime;
		else if (token == "HitObs")
			inf >> hitWalls;
		else if (token == "HitEnts")
			inf >> hitEnts;
		else if (token == "SpawnEntity")
			inf >> spawnEntity;
		else if (token == "DamageType")
		{
			std::string bt;
			inf >> bt;
			if (bt == "DT_AVATAR_ENERGYBLAST")
				damageType = DT_AVATAR_ENERGYBLAST;
			else if (bt == "DT_AVATAR_SHOCK")
				damageType = DT_AVATAR_SHOCK;
			else if (bt == "DT_AVATAR_BITE")
				damageType = DT_AVATAR_BITE;
			else if (bt == "DT_AVATAR_PETBITE")
				damageType = DT_AVATAR_PETBITE;
			else if (bt == "DT_AVATAR_LANCE")
				damageType = DT_AVATAR_LANCE;
			else if (bt == "DT_AVATAR_CREATORSHOT")
				damageType = DT_AVATAR_CREATORSHOT;
			else if (bt == "DT_AVATAR_LIZAP")
				damageType = DT_AVATAR_LIZAP;
			else if (bt == "DT_AVATAR_DUALFORMLI")
				damageType = DT_AVATAR_DUALFORMLI;
			else if (bt == "DT_AVATAR_DUALFORMNAIJA")
				damageType = DT_AVATAR_DUALFORMNAIJA;
			else if (bt == "DT_AVATAR_BUBBLE")
				damageType = DT_AVATAR_BUBBLE;
			else if (bt == "DT_AVATAR_VINE")
				damageType = DT_AVATAR_VINE;
			else if (bt == "DT_NONE")
				damageType = DT_NONE;
			else if (bt == "DT_AVATAR_SEED")
				damageType = DT_AVATAR_SEED;
			else if (bt == "DT_ENEMY_INK")
				damageType = DT_ENEMY_INK;
			else if (bt == "DT_ENEMY_POISON")
				damageType = DT_ENEMY_POISON;
			else if (bt == "DT_ENEMY_CREATOR")
				damageType = DT_ENEMY_CREATOR;
			else if (bt == "DT_ENEMY_MANTISBOMB")
				damageType = DT_ENEMY_MANTISBOMB;
			else
				damageType = (DamageType)atoi(bt.c_str());
		}
		else if (token == "Invisible")
			inf >> invisible;
		else if (token == "CheckDamageTarget")
			inf >> checkDamageTarget;
		else if (token == "AlwaysDoHitEffects")
			inf >> alwaysDoHitEffects;
		else if (token == "DieOnHit")
			inf >> dieOnHit;
		else if (token == "IgnoreShield")
			inf >> ignoreShield;
		else if (token == "DieOnKill")
			inf >> dieOnKill;
		else
		{
			// if having weirdness, check for these
			errorLog(file + " : unidentified token: " + token);
		}


	}
}

void Shot::fire(bool playSfx)
{
	if (shotData)
	{
		if (!shotData->firePrt.empty())
		{
			dsq->spawnParticleEffect(shotData->firePrt, position);
		}

		if (!fired)
		{
			if (!shotData->fireSfx.empty() && playSfx)
			{
				dsq->playPositionalSfx(shotData->fireSfx, position);
			}
			fired = true;
		}
	}
}


Shot::Shot() : Quad(), Segmented(0,0)
{
	addType(SCO_SHOT);
	extraDamage= 0;
	waveTimer = rand()%314;
	emitter = 0;
	lifeTime = 0;
	shotData = 0;
	targetPt = -1;
	fired = false;
	target = 0;
	dead = false;
	damageType = DT_NONE;
	checkDamageTarget = false;
	enqueuedForDelete = false;
	shotIdx = shots.size();
	shots.push_back(this);
}

void loadShotCallback(const std::string &filename, intptr_t param)
{
	ShotData shotData;

	std::string ident;
	int first = filename.find_last_of('/')+1;
	ident = filename.substr(first, filename.find_last_of('.')-first);
	stringToLower(ident);
	debugLog(ident);
	shotData.bankLoad(ident, Shot::shotBankPath);
	Shot::shotBank[ident] = shotData;
}

void Shot::loadShotBank(const std::string &bank1, const std::string &bank2)
{
	clearShotBank();

	shotBankPath = bank1;
	forEachFile(bank1, ".txt", loadShotCallback, 0);

	if (!bank2.empty())
	{
		shotBankPath = bank2;
		forEachFile(bank2, ".txt", loadShotCallback, 0);
	}

	shotBankPath = "";
}

void Shot::clearShotBank()
{
	shotBank.clear();
	for (Shot::Shots::iterator i = Shot::shots.begin(); i != Shot::shots.end(); i++)
	{
		(*i)->shotData = 0;
	}
}

ShotData* Shot::getShotData(const std::string &ident)
{
	std::string id = ident;
	stringToLower(id);
	return &shotBank[id];
}

void Shot::loadBankShot(const std::string &ident, Shot *setter)
{
	if (setter)
	{
		std::string id = ident;
		stringToLower(id);
		//setter->shotData = &shotBank[id];
		setter->applyShotData(&shotBank[id]);
	}
}

void Shot::applyShotData(ShotData *shotData)
{
	if (shotData)
	{
		this->shotData = shotData;
		this->setBlendType(shotData->blendType);
		this->homingness = shotData->homing;
		this->maxSpeed = shotData->maxSpeed;
		this->setTexture(shotData->texture);
		this->scale = shotData->scale;
		this->lifeTime = shotData->lifeTime;
		this->collideRadius = shotData->collideRadius;
		this->renderQuad = !shotData->invisible;
		this->gravity = shotData->gravity;
		this->damageType = shotData->damageType;
		this->checkDamageTarget = shotData->checkDamageTarget;
		if (!shotData->trailPrt.empty())
		{
			setParticleEffect(shotData->trailPrt);
		}

		if (shotData->numSegs > 0)
		{
			segments.resize(shotData->numSegs);
			for (int i = segments.size()-1; i >=0 ; i--)
			{
				Quad *flame = new Quad;
				flame->setTexture(shotData->segTexture);
				flame->scale = shotData->segScale - Vector(shotData->segTaper, shotData->segTaper)*(i);
				flame->setBlendType(this->blendType);
				flame->alpha = 0.5;
				dsq->game->addRenderObject(flame, LR_PARTICLES);
				segments[i] = flame;
				segments[i]->position = position;
			}

			maxDist = shotData->segDist;
			minDist = 0;

			initSegments(position);
		}
	}
}

void Shot::suicide()
{
	setLife(1);
	setDecayRate(20);
	velocity = 0;
	fadeAlphaWithLife = true;
	dead = true;
	onHitWall();
}

void Shot::setParticleEffect(const std::string &particleEffect)
{
	if(dead)
		return;
	if(particleEffect.empty())
	{
		if(emitter)
			emitter->stop();
		return;
	}
	if(!emitter)
	{
		emitter = new ParticleEffect;
		dsq->game->addRenderObject(emitter, LR_PARTICLES);
	}
	emitter->load(particleEffect);
	emitter->start();
}

void Shot::onEndOfLife()
{
	destroySegments(0.2);
	dead = true;

	if (emitter)
	{
		emitter->killParticleEffect();
		emitter = 0;
	}

	if (!enqueuedForDelete)
	{
		enqueuedForDelete = true;
		deleteShots.push_back(this);
	}
}

void Shot::doHitEffects()
{
	BBGE_PROF(Shot_doHitEffects);
	if (shotData)
	{
		if (!shotData->hitPrt.empty())
			dsq->spawnParticleEffect(shotData->hitPrt, position);
		if (!shotData->hitSfx.empty())
			dsq->playPositionalSfx(shotData->hitSfx, position);
	}
}

void Shot::onHitWall()
{
	BBGE_PROF(Shot_onHitWall);
	doHitEffects();
	updateSegments(position);
	destroySegments(0.2);
	if (emitter)
	{
		emitter->killParticleEffect();
		emitter = 0;
	}

	if (shotData)
	{
		if (!shotData->spawnEntity.empty())
		{
			dsq->game->createEntity(shotData->spawnEntity, 0, position, 0, false, "", ET_ENEMY, true);
				//(shotData->spawnEntity, 0, position, 0, false, "");
			if (shotData->spawnEntity == "NatureFormFlowers")
			{
				dsq->game->registerSporeDrop(position, 0);
			}
			else
			{
				dsq->game->registerSporeDrop(position, 2);
			}
		}
	}
}

void Shot::killAllShots()
{
	for (Shots::iterator i = shots.begin(); i != shots.end(); ++i)
		(*i)->safeKill();
}

void Shot::clearShotGarbage()
{
	for(size_t i = 0; i < deleteShots.size(); ++i)
	{
		Shot *s = deleteShots[i];
		const unsigned int idx = s->shotIdx;
		// move last shot to deleted one and shorten vector
		if(idx < shots.size() && shots[idx] == s)
		{
			Shot *lastshot = shots.back();
			shots[idx] = lastshot;
			lastshot->shotIdx = idx;
			shots.pop_back();
		}
		else
			errorLog("Shot::clearShotGarbage(): wrong index in shot vector");
	}
	deleteShots.clear();
}


void Shot::reflectFromEntity(Entity *e)
{
	Entity *oldFirer = firer;
	DamageType dt = getDamageType();
	if (dt >= DT_ENEMY && dt < DT_ENEMY_MAX)
	{
		firer = e;
		target = oldFirer;
		//int d = (int)dt;
		//d += DT_AVATAR;oll
		//damageType = DamageType(d);
	}
}

void Shot::targetDied(Entity *target)
{
	int c = 0;
	for (Shots::iterator i = shots.begin(); i != shots.end(); i++)
	{
		if ((*i)->target == target)
		{
			debugLog("removing target from shot");
			(*i)->target = 0;
		}
		if ((*i)->firer == target)
		{
			(*i)->firer = 0;
		}
		c++;
	}


	/*
	std::ostringstream os;
	os << "# of shots in list: " << c;
	debugLog(os.str());
	*/
}

bool Shot::isHitEnts() const
{
	if (!shotData || shotData->hitEnts)
	{
		return true;
	}
	return false;
}

void Shot::hitEntity(Entity *e, Bone *b)
{
	if (!dead)
	{
		bool die = true;
		bool doEffects=true;

		if (e)
		{
			DamageData d;
			d.attacker = firer;
			d.bone = b;
			d.damage = getDamage();
			d.damageType = getDamageType();
			d.hitPos = position;
			d.shot = this;
			if (shotData)
				d.effectTime = shotData->effectTime;
			if ((firer && firer->getEntityType() == ET_AVATAR))
				d.form = dsq->continuity.form;

			if (!e->canShotHit(d))
				return;


			if (damageType == DT_AVATAR_BITE)
			{
				//debugLog("Shot::hitEntity bittenEntities.push_back");
				dsq->game->avatar->bittenEntities.push_back(e);
			}

			bool damaged = e->damage(d);

			// doesn't have anything to do with effectTime
			if (shotData)
			{
				if (!damaged && checkDamageTarget && !shotData->alwaysDoHitEffects)
				{
					doEffects = false;
				}
			}

			if (e->isEntityDead())
			{
				die = shotData ? shotData->dieOnKill : false;
			}

			if (firer)
			{
				firer->shotHitEntity(e, this, b);
			}


			//debugLog("Shot hit enemy: " + e->name);
		}
		else
		{
			//debugLog("Shot hit 0 enemy");
		}

		if (doEffects)
			doHitEffects();

		target = 0;

		if ((!shotData || shotData->dieOnHit) && die)
		{
			lifeTime = 0;
			fadeAlphaWithLife = true;
			velocity = 0;
			setLife(1);
			setDecayRate(10);
			destroySegments(0.1);
			dead = true;
			if (emitter)
			{
				emitter->killParticleEffect();
				emitter = 0;
			}
		}
	}

	//d.bone = c.bone;
}

void Shot::noSegs()
{
	if (numSegments > 0)
	{
		destroySegments();
	}
}

int Shot::getCollideRadius() const
{
	if (shotData)
		return shotData->collideRadius;
	return 0;
}

float Shot::getDamage() const
{
	if (shotData)
	{
		return shotData->damage + extraDamage;
	}
	return 0;
}

DamageType Shot::getDamageType() const
{
	return damageType;
}

void Shot::setAimVector(const Vector &aim)
{
	velocity = aim;
	if (shotData)
	{
		velocity.setLength2D(shotData->maxSpeed);
	}
	/*
	std::ostringstream os;
	os << "setting aim vector(" << aim.x << ", " << aim.y << ") to vel(" << velocity.x << ", " << velocity.y << ")";
	debugLog(os.str());
	*/
}

void Shot::setTarget(Entity *target)
{
	this->target = target;
}

void Shot::setTargetPoint(int pt)
{
	targetPt = pt;
}

bool Shot::isObstructed(float dt) const
{
	if (shotData->wallHitRadius == 0)
	{
		TileVector t(position + velocity * dt);
		if (dsq->game->isObstructed(t)
			|| dsq->game->isObstructed(TileVector(t.x+1, t.y))
			|| dsq->game->isObstructed(TileVector(t.x-1, t.y))
			|| dsq->game->isObstructed(TileVector(t.x, t.y+1))
			|| dsq->game->isObstructed(TileVector(t.x, t.y-1)))
			return true;
	}
	else
	{
		if (dsq->game->collideCircleWithGrid(position, shotData->wallHitRadius))
		{
			return true;
		}
	}

	return false;
}

void Shot::onUpdate(float dt)
{
	if (dsq->game->isPaused()) return;
	if (dsq->game->isWorldPaused()) return;
	if (!shotData) return;


	if (target)
	{
		if (target->getState() == Entity::STATE_DEATHSCENE)
			target = 0;
		else if (target->alpha == 0)
			target = 0;
	}
	if (life >= 1.0f)
	{
		if (velocity.isZero())
		{
			//velocity = Vector(rand()%100-50, rand()%100-50);
		}
		else if (velocity.isLength2DIn(maxSpeed*0.75f))
		{
			velocity.setLength2D(maxSpeed);
		}
	}

	/*
	if (!gravity.isZero())
	{
		velocity += shotData->gravity * dt;
	}
	*/

	/*
	std::ostringstream os;
	os << "shotVel(" << velocity.x << ", " << velocity.y << ")";
	debugLog(os.str());
	*/

	homingness += shotData->homingIncr*dt;
	if (shotData->homingMax != 0 && homingness > shotData->homingMax)
	{
		homingness = shotData->homingMax;
	}

	if (shotData->waveMag)
	{
		waveTimer += shotData->waveSpeed * dt;
		float off = sinf(waveTimer)*shotData->waveMag;
		Vector side = velocity.getPerpendicularLeft();
		side.setLength2D(off);
		offset = side;
	}

	if (shotData->rotIncr)
	{
		Vector add = velocity.getPerpendicularRight();
		add.setLength2D(shotData->rotIncr);
		velocity += add * dt;
	}
	//emitter.update(dt);
	if (emitter)
	{
		emitter->position = position + offset;
		if (emitter->isDead())
			emitter = 0;
	}

	if (target && lifeTime > 0 && damageType != DT_NONE)
	{
		if (!target->isDamageTarget(damageType))
		{
			target = 0;
		}
	}

	Quad::onUpdate(dt);
	updateSegments(position);
	if (!dead)
	{
		if (lifeTime > 0)
		{
			lifeTime -= dt;
			if (lifeTime <= 0)
			{
				velocity = Vector(0,0);

				setDecayRate(10);
				destroySegments(0.1);
				lifeTime = 0;
				fadeAlphaWithLife = true;
				setLife(1);
				return;
			}
		}
		//TileVector t(position);
		Vector diff;
		if (target)
			diff = target->getTargetPoint(targetPt) - this->position;
		diff.z = 0;
		if (shotData->hitWalls)
		{
			if (isObstructed(dt))
			{
				switch(shotData->bounceType)
				{
				case BOUNCE_REAL:
				{
					// Should have been checked in last onUpdate()
					// If it is stuck now, it must have been fired from a bad position,
					// the obstruction map changed, or it was a bouncing beast form shot,
					// fired from avatar head - which may be inside a wall.
					// In any of these cases, there is nowhere to bounce, so we let the shot die. -- FG
					if (!isObstructed(0))
					{
						if (!shotData->bounceSfx.empty())
						{
							dsq->playPositionalSfx(shotData->bounceSfx, position);
							if(!shotData->bouncePrt.empty())
								dsq->spawnParticleEffect(shotData->bouncePrt, position);
						}
						float len = velocity.getLength2D();
						Vector I = velocity/len;
						Vector N = dsq->game->getWallNormal(position);

						if (!N.isZero())
						{
							//2*(-I dot N)*N + I
							velocity = 2*(-I.dot(N))*N + I;
							velocity *= len;
						}
						break;
					}
					// fall through
				}
				default:
				{
					suicide();
				}
				break;
				}
			}
		}

		if (!velocity.isZero() && target)
		{
			Vector add = diff;
			add.setLength2D(homingness*dt);
			velocity += add;
			velocity.capLength2D(maxSpeed);
		}

		if (!dead)
		{
			if (shotData->spinSpeed != 0)
			{
				if (velocity.x > 0)
				{
					rotation.z += shotData->spinSpeed*dt;
				}
				else if (velocity.x < 0)
				{
					rotation.z -= shotData->spinSpeed*dt;
				}
			}
			else
			{
				if (shotData->rotateToVel)
					rotateToVec(velocity, 0, 0);
			}
		}

	}
}

// HACK : move this to a common base shared with Entity
void Shot::rotateToVec(Vector addVec, float time, int offsetAngle)
{
	// HACK: this mucks up wall normals for some reason
//	if (vel.getSquaredLength2D() <= 0) return;
	if (addVec.x == 0 && addVec.y == 0)
	{
		rotation.interpolateTo(Vector(0,0,0), time, 0);
	}
	else
	{
		float angle=0;
		MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), addVec, angle);
		angle = 180-(360-angle);
		angle += offsetAngle;
		if (rotation.z <= -90 && angle >= 90)
		{
			rotation.z = 360 + rotation.z;
		}
		if (rotation.z >= 90 && angle <= -90)
			rotation.z = rotation.z - 360;


		rotation.interpolateTo(Vector(0,0,angle), time, 0);
	}
}

