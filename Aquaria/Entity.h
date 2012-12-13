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

#include <bitset>

#include "../BBGE/StateMachine.h"
#include "../ExternalLibs/tinyxml.h"
#include "../BBGE/SkeletalSprite.h"
#include "../BBGE/ScriptObject.h"

#include "DSQ.h"
#include "Path.h"
#include "Hair.h"

class ManaBall;
class Path;

struct BoneLock
{
	BoneLock() : entity(0), bone(0), on(false), origRot(0), offRot(0) {}
	Entity *entity;
	Bone *bone;
	Vector localOffset;
	bool on;
	float origRot;
	float offRot;
	Vector wallNormal, circleOffset;
	int collisionMaskIndex;
};

enum EV
{
	EV_WALLOUT				= 0,
	EV_WALLTRANS			= 1,
	EV_CLAMPING				= 2,
	EV_SWITCHCLAMP			= 3,
	EV_CLAMPTRANSF			= 4,
	EV_MOVEMENT				= 5,
	EV_COLLIDE				= 6,
	EV_TOUCHDMG				= 7,
	EV_FRICTION				= 8,
	EV_LOOKAT				= 9,
	EV_CRAWLING				= 10,
	EV_ENTITYDIED			= 11,
	EV_TYPEID				= 12,
	EV_COLLIDELEVEL			= 13,
	EV_BONELOCKED			= 14,
	EV_FLIPTOPATH			= 15,
	EV_NOINPUTNOVEL			= 16,
	EV_VINEPUSH				= 17,
	EV_BEASTBURST			= 18,			// if 1: will not collide with beast on touchAvatarDamage, if 0: will
	EV_MINIMAP				= 19,			// should the entity show up on the minimap?
	EV_SOULSCREAMRADIUS		= 20,			// 0-n: size of radius for naija's dual form scream attack, -1: always hit
	EV_WEBSLOW				= 21,			// 100 by default, multiplied by dt and then divided into vel
	EV_MAX					= 22
};

enum DamageType
{
	DT_NONE,
	DT_ENEMY,
	DT_ENEMY_ENERGYBLAST,
	DT_ENEMY_SHOCK,
	DT_ENEMY_BITE,
	DT_ENEMY_TRAP,
	DT_ENEMY_WEB,
	DT_ENEMY_BEAM,
	DT_ENEMY_GAS,
	DT_ENEMY_INK,
	DT_ENEMY_POISON,
	DT_ENEMY_ACTIVEPOISON,
	DT_ENEMY_CREATOR,
	DT_ENEMY_MANTISBOMB,
	DT_ENEMY_REALMAX,
	DT_ENEMY_MAX,

	DT_AVATAR,
	DT_AVATAR_ENERGYBLAST,
	DT_AVATAR_SHOCK,
	DT_AVATAR_BITE,
	DT_AVATAR_VOMIT,
	DT_AVATAR_ACID,
	DT_AVATAR_SPORECHILD,
	DT_AVATAR_LIZAP,
	DT_AVATAR_NATURE,
	DT_AVATAR_ENERGYROLL,
	DT_AVATAR_VINE,
	DT_AVATAR_EAT,
	DT_AVATAR_EAT_BASICSHOT,
	DT_AVATAR_EAT_MAX,
	DT_AVATAR_LANCEATTACH,
	DT_AVATAR_LANCE,
	DT_AVATAR_CREATORSHOT,
	DT_AVATAR_DUALFORMLI,
	DT_AVATAR_DUALFORMNAIJA,
	DT_AVATAR_BUBBLE,
	DT_AVATAR_SEED,
	DT_AVATAR_PET,
	DT_AVATAR_PETNAUTILUS,
	DT_AVATAR_PETBITE,
	DT_AVATAR_REALMAX,
	DT_AVATAR_MAX,
	DT_TOUCH,
	DT_CRUSH,
	DT_SPIKES,
	DT_STEAM,
	DT_REALMAX
};

enum EatType
{
	EAT_NONE,
	EAT_DEFAULT,
	EAT_FILE,
	EAT_MAX
};

enum ObsCheck
{
	OBSCHECK_RANGE,
	OBSCHECK_4DIR,
	OBSCHECK_DOWN,
	OBSCHECK_8DIR,
};

class Shot;

struct DamageData
{
	DamageData()
	{
		damage = 0;
		attacker = 0;
		bone = 0;
		damageType = DT_TOUCH;
		form = (FormType)0;
		shot = 0;
		effectTime = 0;
		useTimer = true;
	}
	FormType form;
	DamageType damageType;
	Entity *attacker;
	Bone *bone;
	float damage;
	Vector hitPos;
	Shot *shot;
	float effectTime;
	bool useTimer;
};

enum EntityType
{
	ET_NOTYPE,
	ET_AVATAR,
	ET_ENEMY,
	ET_PET,
	ET_FLOCK,
	ET_NEUTRAL,
	ET_INGREDIENT,
};

enum EntityProperty
{
	EP_SOLID			=0,
	EP_MOVABLE			=1,
	EP_BATTERY			=2,
	EP_BLOCKER			=3,
	EP_MAX				=4
};

enum BounceType
{
	BOUNCE_NONE		= -1,
	BOUNCE_SIMPLE	= 0,
	BOUNCE_REAL		= 1
};

class Entity : public Quad, public StateMachine
{
public:
	Entity();
	virtual void init(){}
	virtual void postInit(){}

	Vector lastPosition;
	// postInit gets called after the entity IDs are determined
	int entityTypeIdx;
	enum ActivationType
	{
		ACT_NONE = -1,
		ACT_CLICK = 0,
		ACT_RANGE = 1
	};
	void destroy();
	//void damage(int amount, Spell *spell=0, Entity *attacker=0);
	bool isEntityDead() const {return entityDead;}
	std::string name;
	Vector vel;
	InterpolatedVector vel2;
	int activationRadius;
	void render();
	void update(float dt);

	void spawnParticlesFromCollisionMask(const std::string &p, int intv=1);

	float health;
	float maxHealth;

	bool setBoneLock(const BoneLock &boneLock);

	void heal(float a, int type=0);

	void push(const Vector &vec, float time, int maxSpeed, float dmg);

	bool canSetState(int state);
	
	virtual void message(const std::string &msg, int v) {}
	virtual int messageVariadic(lua_State *L, int nparams) { return 0; }

	bool isUnderWater(const Vector &o=Vector());

	//virtual void onHitBySpell(Spell *spell) {}
	//virtual void onCollide(Entity *e);

	virtual bool damage(const DamageData &d);

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
	int activationRange;
	Entity *followEntity;
	Entity *ridingOnEntity;
	bool canBeTargetedByAvatar;
	virtual void saveExtraData(TiXmlElement *xml){}
	virtual void loadExtraData(TiXmlElement *xml){}
	Vector startPos;
	void getEXP(unsigned int exp);
	void rotateToVec(Vector addVec, float time, int offsetAngle=0);
	virtual void applyVariation(int variation){}

	void popBubble();
	void sound(const std::string &sound, float freq=1, float fadeOut=0);

	void freeze(float time);

	virtual void onSceneFlipped() {}

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
	//void followPath(Path *p, int spd, int loop, bool deleteOnEnd = false);
	void followPath(Path *p, int speedType, int dir, bool deleteOnEnd = false);
	Entity *attachedTo;
	bool touchAvatarDamage(int radius, float dmg, const Vector &override=Vector(-1,-1,-1), int speed=0, float pushTime = 0, Vector collidePos = Vector(0,0,0));

	void moveTowards(Vector p, float dt, int spd);
	void moveAround(Vector p, float dt, int spd, int d);

	void moveTowardsAngle(int angle, float dt, int spd);
	void moveAroundAngle(int angle, float dt, int spd, int dir);

	void moveTowardsTarget(float dt, int spd, int t=0);
	void moveAroundTarget(float dt, int spd, int d, int t=0);
	void moveAroundEntity(float dt, int spd, int d, Entity *e);
	void moveTowardsGroupCenter(float dt, int spd);
	void moveTowardsGroupHeading(float dt, int spd);
	bool doCollisionAvoidance(float dt, int search, float mod, Vector *v = 0, int overrideMaxSpeed=0, int ignoreObs=0, bool onlyVP=false);
	void doSpellAvoidance(float dt, int range, float mod);
	void doEntityAvoidance(float dt, int range, float mod, Entity *ignore =0);
	void setMaxSpeed(int ms);
	Entity *findTarget(int dist, int type, int t=0);
	//bool hasTarget() { return target != 0; }
	bool hasTarget(int t=0);
	bool isTargetInRange(int range, int t=0);
	void doGlint(const Vector &position, const Vector &scale=Vector(2,2), const std::string &tex="Glint", RenderObject::BlendTypes bt=BLEND_DEFAULT);
	Entity *getTargetEntity(int t=0);
	void setTargetEntity(Entity *e, int t=0);

	void attachEntity(Entity *e, Vector offset);
	void detachEntity(Entity *e);
	virtual void activate(){}

	SkeletalSprite skeletalSprite;

	void setEntityType(EntityType et);
	EntityType getEntityType();
	bool isOpposedTo(Entity *e);
	bool isCollideAgainst(Entity *e);
	void flipToTarget(Vector pos);
	bool isFollowingPath();
	void stopFollowingPath();
	void overideMaxSpeed(int ms, float time);
	void disableOverideMaxSpeed();
	int currentEntityTarget;
	void moveToNode(Path *path, int speedType, int dieOnPathEnd=0, bool swim = false);
	bool isHit();
	bool pathBurst(bool wallJump = false);
	Timer burstTimer;
	void revive(int a);
	void setName(const std::string &name);
	void doFriction(float dt);
	void doFriction(float dt, int len);

	bool isNormalLayer() const
	{
		return layer == LR_ENTITIES || layer == LR_ENTITIES0 || layer == LR_ENTITIES2 || layer == LR_ENTITIES_MINUS2 || layer == LR_ENTITIES_MINUS3;
	}
	void watchEntity(Entity *e);
	void idle();
	int followPos;
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
	bool fillGridFromQuad;

	void setID(int id);
	int getID();

	virtual void startPull() {}
	virtual void stopPull();

	bool isInvincible();

	InterpolatedVector maxSpeedLerp;
	Hair *hair;

	void assignUniqueID();
	int entityID;
	int getMaxSpeed();
	std::string deathSound;
	virtual std::string getIdleAnimName();

	void clearDamageTargets();
	void setAllDamageTargets(bool v);
	void setDamageTarget(DamageType dt, bool v);
	bool isDamageTarget(DamageType dt);

	typedef std::bitset<DT_REALMAX> DisabledDamageTypes;

	int targetRange;
	int getTargetRange() { return targetRange; }

	Vector getEnergyShotTargetPosition();
	int getRandomTargetPoint();

	Vector ridingOnEntityOffset;
	void moveOutOfWall();
	bool isSittingOnInvisibleIn();
	/*
	void setCrawling(bool on) { crawling = on; }
	bool isCrawling() { return crawling; }
	*/
	void flipToVel();
	bool isInCurrent() { return inCurrent; }
	void clearTargetPoints();
	void addTargetPoint(const Vector &point);
	int getNumTargetPoints();
	Vector getTargetPoint(int i);
	int targetPriority;
	virtual void shiftWorlds(WorldType lastWorld, WorldType worldType){}
	void setCanLeaveWater(bool v);
	void setSpiritFreeze(bool v);
	void setEatType(EatType et, const std::string &file="");
	EatType getEatType() { return eatType; }
	void setRiding(Entity *e);
	Entity *getRiding();

	void setBounceType(BounceType bt);
	BounceType getBounceType();
	void setDieTimer(float v) { dieTimer = v; }
	float getHealthPerc();
	void setDeathScene(bool v);
	void generateCollisionMask(int ovrCollideRadius=0);
	DamageData lastDamage;
	bool checkSplash(const Vector &override=Vector(0,0,0));
	EatData eatData;
	InterpolatedVector flipScale;
	bool beautyFlip;
	void attachLance();
	void setInvincible(bool inv);
	void clampToHit();
	bool updateLocalWarpAreas(bool affectAvatar);
	virtual void entityDied(Entity *e);
	//bool registerEntityDied;
	bool clampToSurface(int tcheck=0, Vector usePos=Vector(0,0), TileVector hitTile=TileVector(0,0));
	bool checkSurface(int tcheck, int state, float statet);
	static Shader blurShader;
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

	virtual bool canSetBoneLock();

protected:
	bool calledEntityDied;
	Path *waterBubble;
	bool ridingFlip;
	Vector ridingPosition;
	float ridingRotation;

	std::vector<DamageType> ignoreShotDamageTypes;
	Timer vineDamageTimer;
	float boneLockDelay;
	virtual void onSetBoneLock(){}
	virtual void onUpdateBoneLock(){}
	BoneLock boneLock;
	virtual void onDieNormal() {}
	virtual void onDieEaten() {}
	IngredientData *ingredientData;
	int vs[EV_MAX];
	void onEndOfLife();

	bool invincible;
	PauseQuad *lanceGfx;
	float lanceTimer;
	float lanceDelay;
	int lance;
	Bone *lanceBone;
	void updateLance(float dt);
	InterpolatedVector blurShaderAnim;


	int fhScale, fvScale;
	void onFHScale();
	void onFH();
	bool deathScene;
	float dieTimer;
	BounceType bounceType;
	Entity* riding;
	EatType eatType;
	bool stickToNaijasHead;

	bool spiritFreeze;
	bool canLeaveWater;
	bool wasUnderWater;

	std::vector<Vector>targetPoints;

	Vector getMoveVel();
	DisabledDamageTypes disabledDamageTypes;
	//bool crawling;

	//Vector backupPos, backupVel;
	virtual void onIdle() {}
	virtual void onHeal(int type){}
	virtual void onDamage(DamageData &d){}
	virtual void onHealthChange(float change){}
	bool inCurrent;
	std::vector<bool> entityProperties;
	float slowingToStopPathTimer, slowingToStopPath;

	void movementDetails(Vector v);
	Entity *watchingEntity;
	virtual void onPathEnd();
	bool swimPath;
	bool deleteOnPathEnd;
	InterpolatedVector multColor;
	EntityType entityType;
	std::vector<Entity*> attachedEntities;
	std::vector<Vector> attachedEntitiesOffsets;

	virtual void onFreeze(){}

	//Entity *target;
	std::vector<Entity*>targets;
	virtual void onAlwaysUpdate(float dt){}
	virtual void onUpdateFrozen(float dt){}
	float frozenTimer;
	Quad *bubble;

	void doDeathEffects(int manaBallEnergy=0, bool die=true);

	void onEnterState(int action);
	void onExitState(int action);
	//virtual bool onDamage(int amount, Spell *spell, Entity *attacker);
	bool invincibleBreak;

	bool entityDead;
	void onUpdate(float dt);

	Vector pushVec;
	float pushDamage;



	Timer damageTimer;

	void updateBoneLock();

	int pushMaxSpeed;
	std::string currentAnim;
	

protected:
	
	Timer poisonTimer, poisonBitTimer;
	float poison;
private:


	int maxSpeed;
	int oldMaxSpeed;

};

#endif
