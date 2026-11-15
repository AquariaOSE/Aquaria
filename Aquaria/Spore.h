#ifndef SPORE_H
#define SPORE_H

#include "CollideEntity.h"
#include <list>

class Spore : public CollideEntity
{
public:
	Spore(const Vector &position);
	typedef std::list<Spore*> Spores;
	static Spores spores;
	static void killAllSpores();
	static bool isPositionClear(const Vector &position);
	void destroy() OVERRIDE;
protected:
	void onEnterState(int state) OVERRIDE;
	void onUpdate(float dt) OVERRIDE;
	void onEndOfLife() OVERRIDE;
};


#endif
