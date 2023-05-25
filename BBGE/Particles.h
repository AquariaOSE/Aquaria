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
#ifndef __particles__
#define __particles__

#include "Core.h"
#include "Quad.h"

class Emitter;
class ParticleEffect;

struct SpawnParticleData
{
	SpawnParticleData();

	float randomScale1, randomScale2;
	float randomAlphaMod1, randomAlphaMod2;
	enum { NO_SPAWN = 999 };

	enum SpawnArea { SPAWN_CIRCLE=0, SPAWN_LINE };

	int pauseLevel;
	int flipH, flipV;
	SpawnArea spawnArea;

	float updateMultiplier;

	InterpolatedVector velocityMagnitude;
	int randomParticleAngleRange;
	int randomVelocityRange;

	float randomVelocityMagnitude;
	int randomRotationRange;
	InterpolatedVector initialVelocity, number, gravity;
	float randomSpawnRadius;
	Vector randomSpawnMod;
	int randomSpawnRadiusRange;
	bool didOne;
	int justOne;

	int copyParentRotation, copyParentFlip;
	bool useSpawnRate;
	bool calculateVelocityToCenter;
	bool fadeAlphaWithLife, addAsChild;
	int width, height;
	InterpolatedVector scale, rotation, color, alpha, spawnOffset;
	float life;
	InterpolatedVector spawnRate;
	std::string texture;
	BlendType blendType;
	float counter;
	float spawnTimeOffset;
	bool spawnLocal;
	bool inheritColor;
	bool inheritAlpha;

	float lastDTDifference;
	int influenced;
	std::string deathPrt;

	int suckIndex, suckStr;
};

struct Particle
{
	Particle()
	{
		reset();
	}
	void reset()
	{
		active = false;
		life = 1;
		emitter = 0;
		color.stop();
		scale.stop();
		rot.stop();
		alpha.stop();
		pos = Vector(0,0);
		vel = Vector(0,0);
		gvy = Vector(0,0);
		lpos = Vector(0,0);
		color = Vector(1,1,1);
		alpha = 1;
		scale = Vector(1,1);
		rot = Vector(0,0,0);
		emitter = 0;
		index = -1;
	}
	float life;
	bool active;
	Vector pos, vel, gvy, lpos;
	InterpolatedVector color, alpha, scale, rot;
	Emitter *emitter;
	int index;
};

typedef std::vector<Particle> Particles;

class Emitter : public Quad
{
public:
	Emitter(ParticleEffect *pe);
	void destroy() OVERRIDE;
	void addParticle(Particle *p);
	void removeParticle(Particle *p);

	SpawnParticleData data;

	void start();
	void stop();

	bool isEmpty() const {return particles.empty();}


	Vector getSpawnPosition();

	bool hasRot;
protected:
	Vector currentSpawn, lastSpawn;
	void onRender(const RenderState& rs) const OVERRIDE;
	void spawnParticle(float perc=1);
	void onUpdate(float dt) OVERRIDE;

	ParticleEffect *pe;

	typedef std::list<Particle*> Particles;
	Particles particles;
};

class ParticleEffect : public RenderObject
{
public:
	ParticleEffect();
	void load(const std::string &name);
	void bankLoad(const std::string &name, const std::string &path);
	void start();
	void stop();
	bool isRunning() const {return running;}
	void killParticleEffect();
	Emitter *addNewEmitter();
	void clearEmitters();
	void transfer(ParticleEffect *pe);

	void setDie(bool v);

	std::string name;
protected:
	bool die;
	bool waitForParticles;

	void onUpdate(float dt);

	float effectLife, effectLifeCounter;
	bool running;
	typedef std::list<Emitter*> Emitters;
	Emitters emitters;
};

struct ParticleInfluence
{
	ParticleInfluence(Vector pos, float spd, float size, bool pull)
		: pos(pos), size(size), spd(spd), pull(pull)
	{}
	ParticleInfluence() : size(0), spd(0), pull(false) {}
	Vector pos;
	float size;
	float spd;
	bool pull;
};

class ParticleManager
{
public:
	ParticleManager(int size);
	void setSize(size_t size);
	void loadParticleBank(const std::string &bank1, const std::string &bank2);
	void clearParticleBank();

	Particle *getFreeParticle(Emitter *emitter);
	int getSize();

	void update(float dt);

	void loadParticleEffectFromBank(const std::string &name, ParticleEffect *load);
	void updateParticle(Particle *p, float dt);

	void clearInfluences();
	void addInfluence(ParticleInfluence inf);
	int (*collideFunction)(Vector pos);
	void (*specialFunction)(Particle *me);

	void endParticle(Particle *p);

	void setFree(size_t free);

	size_t getFree() { return free; }
	size_t getNumActive() { return numActive; }

	void setNumSuckPositions(size_t num);
	void setSuckPosition(size_t idx, const Vector &pos);

	Vector *getSuckPosition(size_t idx);

	static std::string particleBankPath;

protected:



	std::vector<Vector> suckPositions;
	size_t numActive;
	Particle* stomp();

	void nextFree(size_t f=1);

	size_t oldFree;

	typedef std::vector<ParticleInfluence> Influences;
	Influences influences;

	size_t size, used, free, halfSize;
	Particles particles;


};

extern ParticleManager *particleManager;

#endif

