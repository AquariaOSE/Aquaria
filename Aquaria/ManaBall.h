#ifndef MANABALL_H
#define MANABALL_H

#include "Quad.h"

class ManaBall : public Quad
{
public:
	ManaBall(Vector pos, float a);
	void destroy();
	bool isUsed();
	void use(Entity *entity);
	ParticleEffect healEmitter;
protected:
	float lifeSpan;
	bool used;
	float amount;
	void onUpdate(float dt);
};

#endif
