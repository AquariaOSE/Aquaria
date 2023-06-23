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
#ifndef ENTITY_H
#define ENTITY_H

#include "../BBGE/StateMachine.h"
#include "../BBGE/SkeletalSprite.h"
#include "../BBGE/ScriptObject.h"
#include "SoundManager.h"

#include "TileVector.h"
#include "Damage.h"
#include "GameStructs.h"

class ManaBall;
class Path;
struct MinimapIcon;
class Hair;
class Entity;
struct lua_State;

struct BoneLock
{
	BoneLock() : entity(0), bone(0), on(false), origRot(0) {}
	Entity *entity;
	Bone *bone;
	bool on;
	float origRot;
	Vector wallNormal, circleOffset;
};

// The Lance is an ability that never made it into the released version of the game.
// The code is functional but it was never used. Keeping it around for now
// in case any mods use it for whatever reason.
struct LanceData
{
	LanceData();
	~LanceData();

	PauseQuad *gfx;
	float timer;
	float delay;
	Bone *bone;
};

class Entity : public CollideQuad, public StateMachine, public SoundHolder
{
public:
	Entity();
	virtual ~Entity();
	virtual void init(){}
	virtual void postInit(){}

	Vector lastPosition;
	enum ActivationType
	{
		ACT_NONE = -1,
		ACT_CLICK = 0,
		ACT_RANGE = 1
	};
	void destroy() OVERRIDE;

	bool isEntityDead() const {return entityDead;}
	std::string name;
	Vector vel;
	InterpolatedVector vel2;
	float activationRadius;
	void render(const RenderState& rs) const OVERRIDE;
	void update(float dt) OVERRIDE;

	void spawnParticlesFromCollisionMask(const char *p, unsigned intv=1, int layer = LR_PARTICLES, float rotz = 0);

	float health;
	float maxHealth;

	bool setBoneLock(const BoneLock &boneLock);

	void heal(float a, int type=0);

	void push(const Vector &vec, float time, float maxSpeed, float dmg);

	bool canSetState(int state) OVERRIDE;

	virtual void message(const std::string &msg, int v) {}
	virtual int messageVariadic(lua_State *L, int nparams) { return 0; }
	virtual int activateVariadic(lua_State *L, int nparams) { return 0; }

	bool isUnderWater();
	bool isUnderWater(const Vector& overridePos);
	bool _isUnderWaterPos(const Vector& pos);



	virtual bool damage(const DamageData &d);
	virtual bool canShotHit(const DamageData &d) { return true; }

	virtual void songNote(int note);
	virtual void songNoteDone(int note, float len);
	virtual void lightFlare(){}
	virtual void sporesDropped(const Vector &pos, int type) {}

	bool isPullable();

	bool isInDarkness();

	bool isPresent() const
	{
		return !isDead() && !isEntityDead() && life == 1 && alpha.x != 0;
	}

	void frozenUpdate(float dt);
	void rotateToSurfaceNormal(float t, int n=0, int rot=0);

	ActivationType activationType;
	float activationRange;
	Entity *ridingOnEntity;
	Vector startPos;
	void rotateToVec(Vector addVec, float time, float offsetAngle=0);
	virtual void applyVariation(int variation){}

	void popBubble();
	void sound(const std::string &sound, float freq=1, float fadeOut=0);
	void setStopSoundsOnDeath(bool stop) { stopSoundsOnDeath = stop; }

	void freeze(float time);

	bool isNearObstruction(int sz, int type=0, TileVector *hitTile=0);

	enum
	{
		// MAIN STATES
		STATE_DEAD			=0,
		STATE_IDLE			=1,
		STATE_PUSH			=2,
		STATE_PUSHDELAY		=3,
		STATE_PLANTED		=4,
		STATE_TRANSFORM		=5,
		STATE_PULLED		=6,
		STATE_FOLLOWNAIJA	=7,
		STATE_DEATHSCENE	=8,
		STATE_ATTACK		=9,
		STATE_CHARGE0		=10,
		STATE_CHARGE1		=11,
		STATE_CHARGE2		=12,
		STATE_CHARGE3		=13,
		STATE_WAIT			=20,
		STATE_HUG			=21,
		STATE_EATING		=22,
		STATE_FOLLOW		=23,
		STATE_TITLE			=24
	};
	virtual void onNotify(Entity *notify){}

	float followPath(Path *p, float speed, int dir, bool deleteOnEnd = false);
	bool touchAvatarDamage(int radius, float dmg, const Vector &override=Vector(-1,-1,-1), float speed=0, float pushTime = 0, Vector collidePos = Vector(0,0,0));

	void moveTowards(Vector p, float dt, int spd);
	void moveAround(Vector p, float dt, int spd, int d);

	void moveTowardsAngle(int angle, float dt, int spd);
	void moveAroundAngle(int angle, float dt, int spd, int dir);

	void moveTowardsTarget(float dt, int spd, int t=0);
	void moveAroundTarget(float dt, int spd, int d, int t=0);
	void moveAroundEntity(float dt, int spd, int d, Entity *e);
	void moveTowardsGroupCenter(float dt, int spd);
	void moveTowardsGroupHeading(float dt, int spd);
	bool doCollisionAvoidance(float dt, int search, float mod, Vector *v = 0, float overrideMaxSpeed=0, int ignoreObs=0, bool onlyVP=false);
	void doSpellAvoidance(float dt, int range, float mod);
	void doEntityAvoidance(float dt, int range, float mod, Entity *ignore =0);
	void setMaxSpeed(float ms);
	Entity *findTarget(int dist, int type, int t=0);

	bool hasTarget(int t=0);
	bool isTargetInRange(int range, size_t t=0);
	void doGlint(const Vector &position, const Vector &scale=Vector(2,2), const std::string &tex="Glint", BlendType bt=BLEND_DEFAULT);
	Entity *getTargetEntity(int t=0);
	void setTargetEntity(Entity *e, int t=0);

	virtual void activate(Entity *by, int source){}

	SkeletalSprite skeletalSprite;

	void setEntityType(EntityType et);
	EntityType getEntityType();
	void flipToTarget(Vector pos);
	bool isFollowingPath();
	void stopFollowingPath();
	void overideMaxSpeed(int ms, float time);
	void disableOverideMaxSpeed();
	int currentEntityTarget;
	float moveToPos(Vector pos, float speed, int dieOnPathEnd=0, bool swim = false);
	bool isHit();
	bool pathBurst(bool wallJump = false);
	Timer burstTimer;
	void revive(float a);
	void setName(const std::string &name);
	void doFriction(float dt);
	void doFriction(float dt, float len);

	bool isNormalLayer() const;
	void idle();
	void slowToStopPath(float t);
	bool isSlowingToStopPath();
	Vector lastMove;
	float damageTime;

	void setEntityProperty(EntityProperty ep, bool value=true);
	bool isEntityProperty(EntityProperty ep);
	virtual void song(SongType songType){}
	bool updateCurrents(float dt);
	void updateVel2(float dt, bool override=false);
	bool isAvatarAttackTarget();
	int dropChance;
	void fillGrid();

	void setID(int id);
	int getID();

	virtual void startPull() {}
	virtual void stopPull();

	bool isInvincible();

	InterpolatedVector maxSpeedLerp;
	Hair *hair;

	int entityID;
	int getMaxSpeed();
	std::string deathSound;
	virtual std::string getIdleAnimName();

	void setAllDamageTargets(bool v);
	void setDamageTarget(DamageType dt, bool v);
	bool isDamageTarget(DamageType dt);

	typedef std::vector<DamageType> DisabledDamageTypes;

	int targetRange;
	int getTargetRange() { return targetRange; }

	Vector getEnergyShotTargetPosition();
	int getRandomTargetPoint();

	Vector ridingOnEntityOffset;
	void moveOutOfWall();
	bool isSittingOnInvisibleIn();

	void flipToVel();
	bool isInCurrent() { return inCurrent; }
	void clearTargetPoints();
	void addTargetPoint(const Vector &point);
	int getNumTargetPoints();
	Vector getTargetPoint(size_t i);
	int targetPriority;
	virtual void shiftWorlds(WorldType lastWorld, WorldType worldType){}
	void setCanLeaveWater(bool v);
	void setSpiritFreeze(bool v);
	void setPauseFreeze(bool v);
	void setEatType(EatType et, const std::string &file="");
	EatType getEatType() { return eatType; }
	void setRiding(Entity *e);
	Entity *getRiding();

	void setBounceType(BounceType bt);
	BounceType getBounceType();
	void setDieTimer(float v) { dieTimer = v; }
	float getHealthPerc();
	void setDeathScene(bool v);
	bool isDeathScene() const { return deathScene; }
	void generateCollisionMask(int ovrCollideRadius=0);
	DamageData lastDamage;
	bool checkSplash(const Vector &override=Vector(0,0,0));
	EatData eatData;
	InterpolatedVector flipScale;
	void attachLance();
	void setInvincible(bool inv);
	void clampToHit();
	bool updateLocalWarpAreas(bool affectAvatar);
	virtual void entityDied(Entity *e);

	bool clampToSurface(int tcheck=0, Vector usePos=Vector(0,0), TileVector hitTile=TileVector(0,0));
	bool checkSurface(int tcheck, int state, float statet);

	std::string naijaReaction;
	Vector lookAtPoint;
	Vector getLookAtPoint();

	void setv(EV ev, int v);
	void setvf(EV ev, float v);
	int getv(EV ev);
	float getvf(EV ev);
	bool isv(EV ev, int v);
	void setIngredientData(const std::string &name);

	void postUpdate(float dt);
	BoneLock* getBoneLock() { return &boneLock; }

	virtual void shotHitEntity(Entity *hit, Shot *shot, Bone *b);

	void warpLastPosition();
	void addIgnoreShotDamageType(DamageType dt);

	Vector getRidingPosition();
	void setRidingPosition(const Vector &pos);
	void setRidingRotation(float rot);
	float getRidingRotation();
	void setRidingFlip(bool on);
	bool getRidingFlip();
	void setRidingData(const Vector &pos, float rot, bool fh);
	bool isGoingToBeEaten();
	void setPoison(float m, float t);
	inline float getPoison() const { return poison; }

	virtual bool canSetBoneLock();

	void initHair(int numSegments, float segmentLength, float width, const std::string &tex);
	void updateHair(float dt);
	void setHairHeadPosition(const Vector &pos);
	void exertHairForce(const Vector &force, float dt);

	bool isEntityInside() const;

	void updateSoundPosition();
	void updateBoneLock();
	float boneLockDelay;

	Vector getPushVec() const { return pushVec; }
	float getPushDamage() const { return pushDamage; }

	MinimapIcon *minimapIcon;
	MinimapIcon *ensureMinimapIcon();

protected:
	Path *waterBubble;
	Vector ridingPosition;
	float ridingRotation;

	std::vector<DamageType> ignoreShotDamageTypes;
	Timer vineDamageTimer;
	virtual void onSetBoneLock(){}
	virtual void onUpdateBoneLock(){}
	BoneLock boneLock;
	virtual void onDieNormal() {}
	virtual void onDieEaten() {}
	IngredientData *ingredientData;
	int vs[EV_MAX];
	void onEndOfLife() OVERRIDE;

	void updateLance(float dt);

	void onFHScale();
	void onFH() OVERRIDE;
	float dieTimer;
	BounceType bounceType;
	Entity* riding;
	EatType eatType;

	std::vector<Vector>targetPoints;

	Vector getMoveVel();
	DisabledDamageTypes disabledDamageTypes;



	virtual void onIdle() {}
	virtual void onHeal(int type){}
	virtual void onDamage(DamageData &d){}
	virtual void onHealthChange(float change){}

	float slowingToStopPathTimer, slowingToStopPath;

	void movementDetails(Vector v);
	virtual void onPathEnd();
	InterpolatedVector multColor;
	EntityType entityType;

	virtual void onFreeze(){}


	std::vector<Entity*>targets;
	virtual void onAlwaysUpdate(float dt){}
	virtual void onUpdateFrozen(float dt){}
	float frozenTimer;
	Quad *bubble;

	void doDeathEffects(float manaBallEnergy=0, bool die=true);

	void onEnterState(int action) OVERRIDE;
	void onExitState(int action) OVERRIDE;


	void onUpdate(float dt) OVERRIDE;

	Vector pushVec;
	float pushDamage;



	Timer damageTimer;

	float pushMaxSpeed;
	std::string currentAnim;
	LanceData *lancedata;

	Timer poisonTimer, poisonBitTimer;
	float poison;
	float maxSpeed;

	char entityProperties[EP_MAX];

	// TODO: this should be a bitmask
	bool invincible;
	bool invincibleBreak;
	bool stickToNaijasHead;
	bool spiritFreeze;
	bool pauseFreeze;
	bool canLeaveWater;
	bool wasUnderWater;
	bool entityDead;
	bool swimPath;
	bool deleteOnPathEnd;
	bool inCurrent;
	bool deathScene;
	bool fhScale;
	bool calledEntityDied;
	bool ridingFlip;
	bool canBeTargetedByAvatar;
	bool stopSoundsOnDeath;
public:
	bool fillGridFromQuad;
	bool beautyFlip;
};

#endif
