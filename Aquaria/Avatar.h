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

#include "Entity.h"

class TileVector;
class SongIcon;
class Web;
class Hair;

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
	SongIconParticle(Vector color, Vector pos, size_t note);
	int note;
	SongIcon *toIcon;
protected:
	void onUpdate(float dt);
};

class SongIcon : public Quad
{
public:
	SongIcon(size_t note);
	void destroy();
	size_t note;
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
	virtual ~Avatar();
	void destroy() OVERRIDE;
	void action(int actionID, int state, int source, InputDevice device) OVERRIDE;
	AvatarState state;
	float burst, burstTimer;
	float burstDelay;
	bool bursting;
	BurstType lastBurstType;

	bool isCharging();
	void setBlind(float time);

	void revive();

	bool canWarp;
	void entityDied(Entity *e) OVERRIDE;
	bool zoomOverriden;
	void clampPosition();

	void splash(bool down);
	InterpolatedVector myZoom;

	Entity *entityToActivate;
	Path *pathToActivate;

	void applyWorldEffects(WorldType type);

	void toggleMovement(bool on);

	void refreshModel(std::string file, const std::string &skin, bool forceIdle=false);
	void refreshDualFormModel();
	void switchDualFormMode();

	void enableInput() OVERRIDE;
	void disableInput() OVERRIDE;
	void clearTargets();
	bool singing;

	bool isActionAndGetDir(Vector& dir);

	void startBurstCommon();

	void openSingingInterface(InputDevice device);
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

	void formAbility();
	bool isMouseInputEnabled();

	void startCharge();
	int currentMaxSpeed;
	Vector getFakeCursorPosition();
	Vector getVectorToCursor(bool trueMouse=false);
	Vector getVectorToCursorFromScreenCentre();

	bool canDie;

	Vector warpInLocal;

	float biteDelay, urchinDelay, jellyDelay;
	bool movingOn;

	void render(const RenderState& rs) const OVERRIDE;
	void activateAura(AuraType aura);
	void stopAura();
	void setHeadTexture(const std::string &name, float t=0);
	float headTextureTimer;
	void updateDamageVisualEffects();
	int chargeLevelAttained;
	void updateFormVisualEffects(float dt);
	bool isSinging();
	bool isLockable();
	int getCursorQuadrant();
	void onWarp();
	int getBurstDistance();
	int getStopDistance();
	int looking;
	std::string getIdleAnimName() OVERRIDE;
	bool isRolling() { return rolling; }
	int rollDir;
	std::string getBurstAnimName();
	std::string getRollAnimName();

	void updateDualFormChargeEffects();

	TileVector wallLockTile;
	Vector wallNormal;

	void fallOffWall();

	float fireDelay;
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
	void onAnimationKeyPassed(int key) OVERRIDE;

	bool isSwimming();

	bool isBlockSinging() { return blockSinging; }

	float songInterfaceTimer;
	void removeEatData(int idx);

	typedef std::list<Entity*> BittenEntities;
	BittenEntities bittenEntities;
	Target getNearestTarget(const Vector &checkPos, const Vector &distPos, Entity *source, DamageType dt, bool override=false, std::vector<Target> *ignore=0);

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

	Bone *boneLeftArm, *boneFish2, *bone_head, *bone_dualFormGlow;
	Bone *boneLeftHand, *boneRightHand;

	void startFlourish();
	void applyTripEffects();
	void removeTripEffects();

	void stopBurst();

	void createWeb();
	void clearWeb();
	Web *web;
	float rollDelay;

	bool canSetBoneLock() OVERRIDE;

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

	int getLastActionSourceID() const { return _lastActionSourceID; }
	InputDevice getLastActionInputDevice() const { return _lastActionInputDevice; }

protected:
	void setSongIconPositions();

	Timer webBitTimer;
	int curWebPoint;
	void checkUpgradeForShot(Shot *s);
	size_t getNumShots();
	void lockToWallCommon();
	void onSetBoneLock() OVERRIDE;
	void onUpdateBoneLock() OVERRIDE;

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
	bool _isUnderWater;
	Path *lastWaterBubble;
	bool lastJumpOutFromWaterBubble;
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

	void onIdle() OVERRIDE;
	void onHeal(int type) OVERRIDE;
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
	void onDamage(DamageData &d) OVERRIDE;
	void updateHair(float dt);

	void lostTarget(int i, Entity *e);

	Vector shieldPosition;
	void updateAura(float dt);


	void onHealthChange(float change) OVERRIDE;
	void startWallBurst(bool useCursor=true);
	void startBurst();


	void startBackFlip();
	void stopBackFlip();

	void clampVelocity();

	bool canCharge();
	void formAbilityUpdate(float dt);
	float revertTimer;

	void endCharge();
	bool canMove;

	void onEnterState(int action) OVERRIDE;
	void onExitState(int action) OVERRIDE;
	std::vector<ParticleEffect*>targetQuads;
	Quad *blinder, *fader, *tripper;
	void applyBlindEffects();
	void removeBlindEffects();

	float zoomVel;
	Vector getWallNormal(TileVector t);
	bool checkWarpAreas();

	float splashDelay;



	void onUpdate(float dt) OVERRIDE;

	Quad *glow;
	bool swimming;

	void lmbd(int source, InputDevice device);
	void lmbu(int source, InputDevice device);

	void rmbd(int source, InputDevice device);
	void rmbu(int source, InputDevice device);

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

	int _lastActionSourceID;
	InputDevice _lastActionInputDevice;

};

#endif
