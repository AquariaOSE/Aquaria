#ifndef GASCLOUD_H
#define GASCLOUD_H

#include "Entity.h"

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

#endif
