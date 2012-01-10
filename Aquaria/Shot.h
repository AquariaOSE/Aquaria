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
#pragma once

#include "CollideEntity.h"
#include "Segmented.h"
#include "../BBGE/Particles.h"
#include "../BBGE/ScriptObject.h"

struct ShotData
{
	ShotData();
	std::string texture;
	std::string hitSfx, bounceSfx, fireSfx;
	std::string hitPrt, trailPrt, firePrt, bouncePrt;
	std::string spawnEntity;
	BounceType bounceType;
	int blendType;
	bool segments;
	float damage;
	int maxSpeed, homing, homingMax;
	double homingIncr;
	DamageType damageType;
	Vector scale;

	bool ignoreShield;

	float effectTime;

	int collideRadius;
	float lifeTime;

	int numSegs, segDist;
	int dieOnHit;
	int rotateToVel;
	int wallHitRadius;
	Vector segScale;
	std::string segTexture;
	float segTaper;

	float waveSpeed, waveMag;

	float spinSpeed;
	bool invisible, checkDamageTarget, hitWalls, hitEnts, alwaysDoHitEffects;
	int rotIncr;

	float avatarKickBack, avatarKickBackTime;

	Vector gravity;

	void bankLoad(const std::string &file, const std::string &path);
	
};

class Shot : public ScriptObject, public Quad, public Segmented
{
public:
	//Shot(DamageType damageType, Entity *firer, Vector pos, Entity *target, std::string tex="", float homingness=1000, int maxSpeed=400, int segments=10, float segMin=0.1, float segMax=5, float damage = 1, float lifeTime = 0);
	Shot();
	//void destroy();
	void reflectFromEntity(Entity *e);
	void setParticleEffect(const std::string &particleEffect);
	typedef std::list<Shot*> Shots;
	static Shots shots;
	static std::string shotBankPath;
	static void targetDied(Entity *t);
	static void killAllShots();
	Entity *target, *firer;
	int maxSpeed, targetPt;

	void fire(bool playSfx = true);
	void hitEntity(Entity *e, Bone *b, bool isValid=true);
	void setLifeTime(float l);
	void setBounceType(BounceType bt);
	
	void noSegs();

	void rotateToVec(Vector addVec, float time, int offsetAngle);
	void doHitEffects();

	typedef std::map<std::string, ShotData> ShotBank;
	static ShotBank shotBank;


	static void loadShotBank(const std::string &bank1, const std::string &bank2);
	static void clearShotBank();
	static ShotData* getShotData(const std::string &ident);
	static void loadBankShot(const std::string &ident, Shot *shot);
	void applyShotData(ShotData *shotData);

	void setAimVector(const Vector &aim);
	void setTarget(Entity *target);
	void setTargetPoint(int pt);
	float getDamage() const;
	int getCollideRadius() const;
	DamageType getDamageType() const;
	ShotData *shotData;
	void updatePosition();
	bool isHitEnts() const;
	bool isObstructed(float dt) const;

	float extraDamage;
protected:

	float waveTimer;
	bool fired;

	void suicide();

	ParticleEffect *emitter;

	double homingness;
	float lifeTime;

	void onHitWall();
	void onEndOfLife();

	bool dead;
	void onUpdate(float dt);
};

class Beam : public ScriptObject, public Quad
{
public:
	Beam(Vector pos, float angle);
	typedef std::list<Beam*> Beams;
	static Beams beams;
	//static void targetDied(Entity *t);
	static void killAllBeams();

	float angle;
	void trace();
	Vector endPos;
	void render();
	DamageData damageData;

	void setDamage(float dmg);

	void setBeamWidth(int w);
protected:
	int beamWidth;
	void onRender();
	void onEndOfLife();
	void onUpdate(float dt);
};

class GasCloud : public Entity
{
public:
	GasCloud(Entity *source, const Vector &position, const std::string &particles, const Vector &color, int radius, float life, float damage=0, bool isMoney=false, float poisonTime=0);
protected:
	ParticleEffect *emitter;
	std::string gfx, particles;
	int radius;
	float damage;
	float pTimer;
	void onUpdate(float dt);
	float poisonTime;
	Entity *sourceEntity;
};

class Spore : public CollideEntity
{
public:
	Spore(const Vector &position);
	typedef std::list<Spore*> Spores;
	static Spores spores;
	static void killAllSpores();
	static bool isPositionClear(const Vector &position);
	void destroy();
protected:
	void onEnterState(int state);
	void onUpdate(float dt);
	void onEndOfLife();
};

