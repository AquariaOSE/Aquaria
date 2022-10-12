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
#ifndef SHOT_H
#define SHOT_H

#include "CollideEntity.h"
#include "Segmented.h"
#include "Scriptable.h"

class ParticleEffect;
class Script;

struct ShotData
{
	ShotData();
	std::string texture, name;
	std::string hitSfx, bounceSfx, fireSfx;
	std::string hitPrt, trailPrt, firePrt, bouncePrt;
	std::string spawnEntity;
	BounceType bounceType;
	BlendType blendType;
	bool segments;
	float damage;
	float maxSpeed, homing, homingMax;
	double homingIncr;
	DamageType damageType;
	Vector scale;

	bool ignoreShield;
	bool dieOnKill;

	float effectTime;

	float collideRadius;
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
	bool invisible, checkDamageTarget, hitWalls, hitEnts, alwaysDoHitEffects, alwaysMaxSpeed;
	float rotIncr;

	float avatarKickBack, avatarKickBackTime;

	Vector gravity;

	void bankLoad(const std::string &file, const std::string &path);

};

class Shot : public CollideQuad, public Segmented, public Scriptable
{
public:

	Shot();

	void reflectFromEntity(Entity *e);
	void setParticleEffect(const std::string &particleEffect);
	typedef std::vector<Shot*> Shots;
	static Shots shots, deleteShots;
	static unsigned int shotsIter; // for script
	static Shot *getFirstShot() { shotsIter = 0; return getNextShot(); }
	static Shot *getNextShot() { return shotsIter < shots.size() ? shots[shotsIter++] : NULL; }
	static std::string shotBankPath;
	static void targetDied(Entity *t);
	static void killAllShots();
	static void clearShotGarbage();
	Entity *target, *firer;
	int targetPt;
	float maxSpeed;

	void init();
	void fire(bool playSfx = true);
	void hitEntity(Entity *e, Bone *b);

	void noSegs();

	void rotateToVec(Vector addVec, float time, int offsetAngle);
	void doHitEffects();

	static void loadShotBank(const std::string &bank1, const std::string &bank2);
	static void clearShotBank();
	static ShotData* getShotData(const std::string &ident);
	static void loadBankShot(const std::string &ident, Shot *shot);
	void applyShotData(const ShotData& shotData);

	void setAimVector(const Vector &aim);
	void setTarget(Entity *target);
	void setTargetPoint(int pt);
	float getDamage() const;
	int getCollideRadius() const;
	DamageType getDamageType() const;
	const ShotData *shotData;
	void updatePosition();
	bool isHitEnts() const;
	bool canHit(Entity *e, Bone *b);
	bool isObstructed(float dt) const;
	inline bool isActive() const { return !dead; }
	inline const char *getName() const { return shotData ? shotData->name.c_str() : ""; }

	float extraDamage;
	float homingness;
	float lifeTime;
	DamageType damageType;
	bool checkDamageTarget;
	bool alwaysMaxSpeed;

protected:

	float waveTimer;

	void suicide();

	ParticleEffect *emitter;

	bool onHitWall(bool reflect);
	void onEndOfLife();

	bool dead;
	bool fired;
	bool enqueuedForDelete;
	void onUpdate(float dt);
	bool updateScript;

private:
	unsigned int shotIdx;
};

#endif
