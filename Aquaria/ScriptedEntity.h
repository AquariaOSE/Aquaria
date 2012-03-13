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
#include "Hair.h"
struct lua_State;

class ScriptedEntity : public CollideEntity, public Segmented
{
public:
	ScriptedEntity(const std::string &scriptName, Vector position, EntityType et = ET_ENEMY);
	void init();
	void postInit();
	void destroy();
	void stopTimer();
	void resetTimer(float t);
	void setEntityLayer(int layer);
	void setupEntity(const std::string &tex, int layer=0);
	void setupBasicEntity(const std::string& texture, int health, int manaBall, int exp, int money, int collideRadius, int state, int w, int h, int expType, bool hitEntity, int updateCull, int layer);
	void initHair(int numSegments, int segmentLength, int width, const std::string &tex);
	void initSegments(int numSegments, int minDist, int maxDist, std::string bodyTex, std::string tailTex, int w, int h, float taper, bool reverseSegments);
	void registerNewPart(RenderObject *r, const std::string &name);
	typedef std::map<std::string, RenderObject*> PartMap;
	PartMap partMap;
	bool surfaceMoveDir;
	void activate();
	void warpSegments();
	void lightFlare();
	void entityDied(Entity *e);
	void message(const std::string &msg, int v);
	void messageVariadic(lua_State *L, int nparams);
	
	static bool runningActivation;

	void sporesDropped(const Vector &pos, int type);

	bool damage(const DamageData &d);
	void song(SongType songType);

	void startPull();
	void stopPull();

	void songNote(int note);
	void songNoteDone(int note, float len);
	void onAnimationKeyPassed(int key);

	void initStrands(int num, int segs, int dist, int spacing, Vector color);
	typedef std::vector<Strand*> Strands;
	Strands strands;
	int strandSpacing;
	bool isEntityInside();
	void becomeSolid();

	void updateHair(float dt);
	void setHairHeadPosition(const Vector &pos);
	void exertHairForce(const Vector &force, float dt);
	std::string deathParticleEffect;

	ParticleEffect pullEmitter;
	int manaBallAmount;
	
	void initEmitter(int emit, const std::string &file);
	void startEmitter(int emit);
	void stopEmitter(int emit);

	void shiftWorlds(WorldType lastWorld, WorldType worldType);
	void setAutoSkeletalUpdate(bool v);

	void shotHitEntity(Entity *hit, Shot *shot, Bone *b);
protected:
	void onDieNormal();
	void onDieEaten();
	void luaDebugMsg(const std::string &func, const std::string &msg);
	float crushDelay;
	bool autoSkeletalSpriteUpdate;
	int beforePullMaxSpeed;
	bool songNoteFunction, preUpdateFunc;
	bool songNoteDoneFunction;
	std::vector<ParticleEffect*> emitters;
	bool becomeSolidDelay;
	void onAlwaysUpdate(float dt);
	void updateStrands(float dt);
	bool animKeyFunc;
	//void onPathEnd();
	
	void onExitTimer();
	float myTimer;
	void onHitWall();
	bool reverseSegments;
	Script *script;
	void onUpdate(float dt);
	void onEnterState(int action);
	void onExitState(int action);
	virtual void deathNotify(RenderObject *r);
};
