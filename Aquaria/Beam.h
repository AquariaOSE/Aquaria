#ifndef AQUARIA_BEAM_H
#define AQUARIA_BEAM_H

#include "Quad.h"
#include "Damage.h"

#include <list>

class Entity;

class Beam : public Quad
{
public:
	Beam(Vector pos, float angle);
	typedef std::list<Beam*> Beams;
	static Beams beams;

	static void killAllBeams();

	float angle;
	void trace();
	Vector endPos;
	DamageData damageData;

	void setDamage(float dmg);
	void setFirer(Entity *e);
	void setBeamWidth(float w);
protected:
	float beamWidth;
	void onRender(const RenderState& rs) const OVERRIDE;
	void onEndOfLife() OVERRIDE;
	void onUpdate(float dt) OVERRIDE;
};

#endif
