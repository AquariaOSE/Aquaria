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
#ifndef AVATAR_H
#define AVATAR_H

#include "../BBGE/Particles.h"

#include "DSQ.h"
#include "Hair.h"
#include "Entity.h"

#include "Web.h"

class TileVector;
class SongIcon;

struct Target
{
public:
	Target() { e = 0; targetPt = -1; }
	Entity *e;
	Vector pos;
	int targetPt;
	Vector getWorldPosition();
};

enum BurstType
{
	BURST_NONE		= 0,
	BURST_NORMAL	= 1,
	BURST_WALL		= 2
};

enum EnableInputType
{
	AVATARINPUT_DEFAULT		= 0,
	AVATARINPUT_NOCURSOR	= 1,
	AVATARINPUT_MAX
};

enum AvatarAnimLayers
{
	ANIMLAYER_FLOURISH		= 3,
	ANIMLAYER_OVERRIDE		= 4,
	ANIMLAYER_ARMOVERRIDE	= 5,
	ANIMLAYER_UPPERBODYIDLE	= 6,
	ANIMLAYER_HEADOVERRIDE	= 7,
	ANIMLAYER_MAX
};

enum SeeMapMode
{
	SEE_MAP_NEVER = 0,
	SEE_MAP_DEFAULT = 1,
	SEE_MAP_ALWAYS = 2,
};

class SongIconParticle : public Quad
{
public:
	SongIconParticle(Vector color, Vector pos, int note);
	int note;
	SongIcon *toIcon;
protected:
	void onUpdate(float dt);
};

class SongIcon : public Quad
{
public:
	SongIcon(int note);
	void destroy();
	int note;
	void openNote();
	void closeNote();
	void openInterface();
	void closeInterface();
	static int notesOpen;
	Vector noteColor;
	bool open;

protected:
	Quad *glow;
	float rippleTimer;
	float len;
	float ptimer;
	void spawnParticles(float dt);
	void *channel;
	float delay, counter, minTime;
	bool cursorIsIn;
	void onUpdate(float dt);
};

class AvatarState
{
public:
	AvatarState();
	Timer blindTimer;
	float abilityDelay;
	bool blind;
	bool wasUnderWater;
	float shotDelay;
	//Timer shockTimer;
	Timer useItemDelay;
	Timer lockToWallDelay;
	float spellCharge;
	bool lockedToWall;
	float leachTimer;
	bool nearWall;
	float swimTimer, rollTimer;
	float updateLookAtTime;
	float blinkTimer;
	Entity *lookAtEntity;
	Vector outOfWaterVel;
	float outOfWaterTimer;
	bool backFlip;
};

class Avatar : public Entity, public ActionMapper
{
public:
	Avatar();
	void postInit();
	virtual ~Avatar();
	void destroy();
	void action(int actionID, int state);
	AvatarState state;
	float burst, burstTimer;
	float burstDelay;
	bool bursting;
	BurstType lastBurstType;
	//void damage(int amount);
	bool isCharging();
	void setBlind(float time);

	void revive();

	bool canWarp;
	void entityDied(Entity *e);
	void onCollide(Entity *e);
	bool zoomOverriden;
	void clampPosition();

	void splash(bool down);
	InterpolatedVector myZoom;

	Entity *entityToActivate;
	Path *pathToActivate;

	void applyWorldEffects(WorldType type);

	void toggleMovement(bool on);

	Vector getFacing();

	void refreshModel(std::string file, const std::string &skin, bool forceIdle=false);
	void refreshDualFormModel();
	void switchDualFormMode();

	void enableInput();
	void disableInput();
	void clearTargets();
	bool singing;

	void doBounce();
	Vector getKeyDir();

	void startBurstCommon();
	void updateJoystick(float dt);

	int getNotesOpen();
	int getLastNote();

	int lastNote;

	void openSingingInterface();
	void closeSingingInterface();
	void updateSingingInterface(float dt);

	void playHitSound();

	void changeForm(FormType form, bool effects=true, bool onInit=false, FormType lastForm=FORM_NONE);
	void singNote(int note);
	std::vector<SongIcon*> songIcons;
	Song currentSong;
	int currentSongIdx;

	Entity *pullTarget;

	void setNearestPullTarget();

	void formAbility(int ability);
	bool isMouseInputEnabled();

	void startCharge(int ability);
	int currentMaxSpeed;
	Vector getFakeCursorPosition();
	Vector getVectorToCursor(bool trueMouse=false);
	Vector getVectorToCursorFromScreenCentre();

	bool canDie;

	Vector warpInLocal;

	float biteDelay, urchinDelay, jellyDelay;
	bool movingOn;

	void render();
	void activateAura(AuraType aura);
	void stopAura();
	void setHeadTexture(const std::string &name, float t=0);
	float headTextureTimer;
	void updateDamageVisualEffects();
	int chargeLevelAttained;
	void chargeVisualEffect(const std::string &tex);
	void updateFormVisualEffects(float dt);
	bool isSinging();
	bool isLockable();
	int getCursorQuadrant();
	void onWarp();
	int getBurstDistance();
	int getStopDistance();
	int looking;
	std::string getIdleAnimName();
	bool isRolling() { return rolling; }
	int rollDir;
	std::string getBurstAnimName();
	std::string getRollAnimName();

	void updateDualFormChargeEffects();

	TileVector wallLockTile;
	Vector wallNormal;

	void fallOffWall();

	float fireDelay;
	int getSingingInterfaceRadius();
	int getOpenSingingInterfaceRadius();
	AuraType activeAura;
	float auraTimer;
	bool fireAtNearestValidEntity(const std::string &shot);

	void checkNearWall();
	Vector getAim();
	Vector getForwardAim();
	void setWasUnderWater();
	Quad *lightFormGlow, *lightFormGlowCone;
	void setBlockSinging(bool v);
	Vector headPosition;
	void updatePosition();
	float quickSongCastDelay;
	void onAnimationKeyPassed(int key);

	bool isSwimming();

	bool isBlockSinging() { return blockSinging; }

	float songInterfaceTimer;
	void removeEatData(int idx);
	//std::list<Entity*>bittenEntities;
	typedef std::list<Entity*> BittenEntities;
	BittenEntities bittenEntities;
	void updateHeartbeatSfx(float t = 0);
	Target getNearestTarget(const Vector &checkPos, const Vector &distPos, Entity *source, DamageType dt, bool override=false, std::vector<Target> *ignore=0, EntityList *entityList=0);

	void toggleCape(bool on);
	void updateLookAt(float dt);
	void updateFoodParticleEffects();
	void endOfGameState();
	bool canQuickSong();
	bool canActivateStuff();
	void setCanActivateStuff(bool on);
	bool hasThingToActivate();

	float biteTimer;
	Timer flourishTimer;
	Timer flourishPowerTimer;
	Timer stillTimer;

	void refreshNormalForm();
	bool canChangeForm;

	void bindInput();

	Vector getHeadPosition();

	Bone *boneRightFoot, *boneLeftFoot, *boneRightArm, *boneLeftArm, *boneFish2, *bone_head;
	Bone *boneLeftHand, *boneRightHand;

	void startFlourish();
	void applyTripEffects();
	void removeTripEffects();

	void stopBurst();

	void createWeb();
	void clearWeb();
	Web *web;
	float rollDelay;

	void loseTargets();

	bool canSetBoneLock();
	
	void revert();
	void doBindSong();
	void doShieldSong();

	bool canBurst() const { return _canBurst; }
	void setCanBurst(bool b) { _canBurst = b; }

	bool canLockToWall() const { return _canLockToWall; }
	void setCanLockToWall(bool b) { _canLockToWall = b; }

	bool canSwimAgainstCurrents() const { return _canSwimAgainstCurrents; }
	void setCanSwimAgainstCurrents(bool b) { _canSwimAgainstCurrents = b; }

	bool canCollideWithShots() const { return _canCollideWithShots; }
	void setCollideWithShots(bool b) { _canCollideWithShots = b; }

	void setCollisionAvoidanceData(int range, float mod);

	void setSeeMapMode(SeeMapMode mode) { _seeMapMode = mode; }
	SeeMapMode getSeeMapMode() const { return _seeMapMode; }

	int leaches;
	float shieldPoints;
	float elementEffectMult;

	bool blockBackFlip;
	
protected:
	void setSongIconPositions();

	Timer webBitTimer;
	int curWebPoint;
	void checkUpgradeForShot(Shot *s);
	int getNumShots();
	void lockToWallCommon();
	void onSetBoneLock();
	void onUpdateBoneLock();

	void adjustHeadRot();
	std::string lastHeadTexture;
	void updateDualFormGlow(float dt);
	Vector getTendrilAimVector(int i, int max);
	void applyRidingPosition();
	void stopWallJump();
	void updateWallJump(float dt);
	float wallBurstTimer;
	float targetUpdateDelay;
	std::vector<Target> targets;
	void updateTargets(float dt, bool override);
	void updateTargetQuads(float dt);
	Vector bodyOffset;
	bool flourish;
	bool blockSinging;
	int spiritEnergyAbsorbed;
	float formAbilityDelay;
	Vector bodyPosition;


	bool joystickMove;
	bool ripples;
	float rippleDelay, tripDelay;
	float formTimer;
	float fallGravityTimer;
	Vector fallGravity;
	int lastOutOfWaterMaxSpeed;

	void onIdle();
	void onHeal(int type);
	ParticleEffect biteLeftEmitter, biteRightEmitter, swimEmitter, auraHitEmitter;
	ParticleEffect auraEmitter, auraLowEmitter, wakeEmitter, healEmitter, hitEmitter, rollLeftEmitter, rollRightEmitter, spiritBeaconEmitter, plungeEmitter;
	ParticleEffect speedEmitter, defenseEmitter, invincibleEmitter, regenEmitter;
	ParticleEffect *leftHandEmitter, *rightHandEmitter;
	ParticleEffect *chargingEmitter, *chargeEmitter;
	void updateCursor(float dt);
	bool rolling;
	int rollDidOne;
	
	void startRoll(int dir);
	void stopRoll();
	int getQuadrantDirection(int lastQuad, int quad);
	void updateRoll(float dt);
	int lastQuad, lastQuadDir;
	void onDamage(DamageData &d);
	void updateHair(float dt);

	void lostTarget(int i, Entity *e);

	Vector shieldPosition;
	void updateAura(float dt);


	void onHealthChange(float change);
	void startWallBurst(bool useCursor=true);
	void startBurst();


	void startBackFlip();
	void stopBackFlip();

	void clampVelocity();

	bool canCharge(int ability);
	void formAbilityUpdate(float dt);
	float revertTimer;

	void endCharge();
	Entity *activateEntity;
	bool canMove;

	void onEnterState(int action);
	void onExitState(int action);
	std::vector<ParticleEffect*>targetQuads;
	Quad *blinder, *fader, *tripper;
	void applyBlindEffects();
	void removeBlindEffects();

	float zoomVel;
	// implement "bobbing" in a lower class
	int getBeamWidth();
	Vector getWallNormal(TileVector t);
	bool checkWarpAreas();

	float splashDelay;
	//Hair *hair;

	//Item *currentItem;
	void onUpdate(float dt);
	void onRender();

	Quad *glow;
	bool swimming;

	void lmbd();
	void lmbu();

	void rmbd();
	void rmbu();

	bool charging;

	float pushingOffWallEffect;
	float lockToWallFallTimer;

	Vector wallPushVec;



	void lockToWall();
	void doShock(const std::string &shotName);

	bool _canActivateStuff;
	bool _canBurst;
	bool _canLockToWall;
	bool _canSwimAgainstCurrents;
	bool _canCollideWithShots;
	SeeMapMode _seeMapMode;

	int _collisionAvoidRange;
	float _collisionAvoidMod;

};

#endif
